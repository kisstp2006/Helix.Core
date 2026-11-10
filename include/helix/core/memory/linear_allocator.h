#pragma once

#include <helix/core/memory/allocator.h>

namespace helix
{
    class LinearAllocator final : public Allocator
    {
    public:
        explicit LinearAllocator(usize capacity, Allocator& parent = HeapAllocator());
        ~LinearAllocator() override;

        LinearAllocator(const LinearAllocator&) = delete;
        LinearAllocator& operator=(const LinearAllocator&) = delete;

        void* Allocate(usize size, usize alignment) override;
        void Free(void*, usize, usize) override {}
        const char* Name() const override { return "Linear"; }

        void Reset() { offset_ = 0; }
        usize Mark() const { return offset_; }
        void Rewind(usize mark);
        usize Used() const { return offset_; }
        usize Capacity() const { return capacity_; }

    private:
        Allocator& parent_;
        u8* memory_;
        usize capacity_;
        usize offset_ = 0;
    };
}
