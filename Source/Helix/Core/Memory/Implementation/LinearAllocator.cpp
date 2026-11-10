#include <Helix/Core/Memory/LinearAllocator.h>
#include <Helix/Core/Basics/Assert.h>

namespace helix
{
    LinearAllocator::LinearAllocator(usize capacity, Allocator& parent)
        : parent_(parent), memory_(static_cast<u8*>(parent.Allocate(capacity, 64))), capacity_(capacity)
    {
    }

    LinearAllocator::~LinearAllocator() { parent_.Free(memory_, capacity_, 64); }

    void* LinearAllocator::Allocate(usize size, usize alignment)
    {
        HX_ASSERT(IsPowerOfTwo(alignment));
        uptr base = reinterpret_cast<uptr>(memory_);
        usize start = AlignUp(base + offset_, alignment) - base;
        if (start + size > capacity_)
        {
            HX_ASSERT_MSG(false, "linear allocator is full");
            return nullptr;
        }
        offset_ = start + size;
        return memory_ + start;
    }

    void LinearAllocator::Rewind(usize mark)
    {
        HX_ASSERT(mark <= offset_);
        offset_ = mark;
    }
}
