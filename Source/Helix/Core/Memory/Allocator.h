#pragma once

#include <Helix/Core/Basics/Utility.h>

#include <new>

namespace helix
{
    inline constexpr usize kDefaultAlignment = alignof(std::max_align_t);

    class Allocator
    {
    public:
        virtual ~Allocator() = default;
        virtual void* Allocate(usize size, usize alignment) = 0;
        virtual void Free(void* memory, usize size, usize alignment) = 0;
        virtual const char* Name() const = 0;
    };

    struct HeapStats
    {
        i64 liveAllocations;
        i64 liveBytes;
        u64 totalAllocations;
    };

    Allocator& HeapAllocator();
    HeapStats GetHeapStats();

    template <typename T, typename... Args>
    T* New(Allocator& allocator, Args&&... args)
    {
        void* memory = allocator.Allocate(sizeof(T), alignof(T));
        return ::new (memory) T(Forward<Args>(args)...);
    }

    template <typename T>
    void Delete(Allocator& allocator, T* object)
    {
        if (!object)
            return;
        object->~T();
        allocator.Free(object, sizeof(T), alignof(T));
    }
}
