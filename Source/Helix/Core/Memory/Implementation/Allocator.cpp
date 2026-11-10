#include <Helix/Core/Memory/Allocator.h>
#include <Helix/Core/Basics/Assert.h>

#include <atomic>
#include <cstdlib>

namespace helix
{
    namespace
    {
        std::atomic<i64> liveAllocations{0};
        std::atomic<i64> liveBytes{0};
        std::atomic<u64> totalAllocations{0};

        class SystemHeap final : public Allocator
        {
        public:
            void* Allocate(usize size, usize alignment) override
            {
                HX_ASSERT(IsPowerOfTwo(alignment));
                if (size == 0)
                    size = 1;
                alignment = Max(alignment, sizeof(void*));
#if HX_PLATFORM_WINDOWS
                void* memory = _aligned_malloc(size, alignment);
#else
                void* memory = nullptr;
                if (posix_memalign(&memory, alignment, size) != 0)
                    memory = nullptr;
#endif
                if (!memory)
                    HX_FATAL("out of memory");
                liveAllocations.fetch_add(1, std::memory_order_relaxed);
                liveBytes.fetch_add(i64(size), std::memory_order_relaxed);
                totalAllocations.fetch_add(1, std::memory_order_relaxed);
                return memory;
            }

            void Free(void* memory, usize size, usize) override
            {
                if (!memory)
                    return;
                liveAllocations.fetch_sub(1, std::memory_order_relaxed);
                liveBytes.fetch_sub(i64(size ? size : 1), std::memory_order_relaxed);
#if HX_PLATFORM_WINDOWS
                _aligned_free(memory);
#else
                std::free(memory);
#endif
            }

            const char* Name() const override { return "Heap"; }
        };
    }

    Allocator& HeapAllocator()
    {
        static SystemHeap heap;
        return heap;
    }

    HeapStats GetHeapStats()
    {
        return {liveAllocations.load(std::memory_order_relaxed), liveBytes.load(std::memory_order_relaxed),
                totalAllocations.load(std::memory_order_relaxed)};
    }
}
