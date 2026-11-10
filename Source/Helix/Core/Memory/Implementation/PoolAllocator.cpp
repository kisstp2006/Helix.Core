#include <Helix/Core/Memory/PoolAllocator.h>
#include <Helix/Core/Basics/Assert.h>

namespace helix
{
    PoolAllocator::PoolAllocator(usize elementSize, usize elementAlignment, usize elementsPerBlock, Allocator& parent)
        : parent_(parent),
          stride_(AlignUp(Max(elementSize, sizeof(FreeNode)), Max(elementAlignment, alignof(FreeNode)))),
          alignment_(Max(elementAlignment, alignof(FreeNode))),
          elementsPerBlock_(elementsPerBlock),
          headerSize_(AlignUp(sizeof(Block), Max(elementAlignment, alignof(FreeNode))))
    {
        HX_ASSERT(IsPowerOfTwo(elementAlignment) && elementsPerBlock > 0);
        blockSize_ = headerSize_ + stride_ * elementsPerBlock_;
    }

    PoolAllocator::~PoolAllocator()
    {
        HX_ASSERT_MSG(live_ == 0, "pool destroyed with live elements");
        while (firstBlock_)
        {
            Block* next = firstBlock_->next;
            parent_.Free(firstBlock_, blockSize_, alignment_);
            firstBlock_ = next;
        }
    }

    void PoolAllocator::Grow()
    {
        auto* block = static_cast<Block*>(parent_.Allocate(blockSize_, Max(alignment_, alignof(Block))));
        block->next = firstBlock_;
        firstBlock_ = block;
        blocks_++;
        u8* elements = reinterpret_cast<u8*>(block) + headerSize_;
        for (usize i = elementsPerBlock_; i-- > 0;)
        {
            auto* node = reinterpret_cast<FreeNode*>(elements + i * stride_);
            node->next = freeList_;
            freeList_ = node;
        }
    }

    void* PoolAllocator::Allocate(usize size, usize alignment)
    {
        HX_ASSERT(size <= stride_ && alignment <= alignment_);
        (void)size;
        (void)alignment;
        if (!freeList_)
            Grow();
        FreeNode* node = freeList_;
        freeList_ = node->next;
        live_++;
        return node;
    }

    void PoolAllocator::Free(void* memory, usize, usize)
    {
        if (!memory)
            return;
        auto* node = static_cast<FreeNode*>(memory);
        node->next = freeList_;
        freeList_ = node;
        live_--;
    }
}
