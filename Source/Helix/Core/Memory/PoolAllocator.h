#pragma once

#include <Helix/Core/Memory/Allocator.h>

namespace helix
{
    class PoolAllocator final : public Allocator
    {
    public:
        PoolAllocator(usize elementSize, usize elementAlignment, usize elementsPerBlock = 256, Allocator& parent = HeapAllocator());
        ~PoolAllocator() override;

        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;

        void* Allocate(usize size, usize alignment) override;
        void Free(void* memory, usize size, usize alignment) override;
        const char* Name() const override { return "Pool"; }

        usize LiveCount() const { return live_; }
        usize BlockCount() const { return blocks_; }

    private:
        struct FreeNode
        {
            FreeNode* next;
        };

        struct Block
        {
            Block* next;
        };

        void Grow();

        Allocator& parent_;
        usize stride_;
        usize alignment_;
        usize elementsPerBlock_;
        usize blockSize_;
        usize headerSize_;
        Block* firstBlock_ = nullptr;
        FreeNode* freeList_ = nullptr;
        usize live_ = 0;
        usize blocks_ = 0;
    };
}
