#pragma once

#include <helix/core/base/assert.h>
#include <helix/core/hash/hash.h>
#include <helix/core/memory/allocator.h>

#include <cstring>

namespace helix
{
    // Open addressing with one control byte per slot: high bit set means empty or deleted,
    // otherwise the low 7 bits hold extra hash bits to skip most key compares.
    template <typename K, typename V, typename H = Hasher<K>>
    class HashMap
    {
        static constexpr u8 kEmpty = 0x80;
        static constexpr u8 kDeleted = 0xFE;

        struct Slot
        {
            K key;
            V value;
        };

    public:
        struct Entry
        {
            const K& key;
            V& value;
        };

        class Iterator
        {
        public:
            Iterator(const HashMap* map, usize index) : map_(map), index_(index) { Skip(); }
            Entry operator*() const { return {map_->slots_[index_].key, map_->slots_[index_].value}; }
            Iterator& operator++()
            {
                index_++;
                Skip();
                return *this;
            }
            bool operator!=(const Iterator& other) const { return index_ != other.index_; }

        private:
            void Skip()
            {
                while (index_ < map_->capacity_ && (map_->ctrl_[index_] & 0x80))
                    index_++;
            }

            const HashMap* map_;
            usize index_;
        };

        explicit HashMap(Allocator& allocator = HeapAllocator()) : allocator_(&allocator) {}

        HashMap(const HashMap& other) : allocator_(other.allocator_)
        {
            Reserve(other.size_);
            for (auto e : other)
                Set(e.key, e.value);
        }

        HashMap(HashMap&& other) noexcept
            : allocator_(other.allocator_), ctrl_(other.ctrl_), slots_(other.slots_), capacity_(other.capacity_), size_(other.size_), deleted_(other.deleted_)
        {
            other.ctrl_ = nullptr;
            other.slots_ = nullptr;
            other.capacity_ = other.size_ = other.deleted_ = 0;
        }

        ~HashMap()
        {
            Clear();
            Release();
        }

        HashMap& operator=(HashMap other) noexcept
        {
            Swap(allocator_, other.allocator_);
            Swap(ctrl_, other.ctrl_);
            Swap(slots_, other.slots_);
            Swap(capacity_, other.capacity_);
            Swap(size_, other.size_);
            Swap(deleted_, other.deleted_);
            return *this;
        }

        usize Size() const { return size_; }
        usize Capacity() const { return capacity_; }
        bool Empty() const { return size_ == 0; }

        Iterator begin() const { return Iterator(this, 0); }
        Iterator end() const { return Iterator(this, capacity_); }

        V* Find(const K& key)
        {
            usize i = FindIndex(key);
            return i == npos ? nullptr : &slots_[i].value;
        }

        const V* Find(const K& key) const { return const_cast<HashMap*>(this)->Find(key); }
        bool Contains(const K& key) const { return FindIndex(key) != npos; }

        template <typename KK, typename VV>
        V& Set(KK&& key, VV&& value)
        {
            bool inserted;
            usize i = FindOrPrepare(key, inserted);
            if (inserted)
                ::new (&slots_[i]) Slot{K(Forward<KK>(key)), V(Forward<VV>(value))};
            else
                slots_[i].value = V(Forward<VV>(value));
            return slots_[i].value;
        }

        template <typename KK, typename VV>
        bool TryAdd(KK&& key, VV&& value)
        {
            bool inserted;
            usize i = FindOrPrepare(key, inserted);
            if (inserted)
                ::new (&slots_[i]) Slot{K(Forward<KK>(key)), V(Forward<VV>(value))};
            return inserted;
        }

        V& operator[](const K& key)
        {
            bool inserted;
            usize i = FindOrPrepare(key, inserted);
            if (inserted)
                ::new (&slots_[i]) Slot{K(key), V()};
            return slots_[i].value;
        }

        bool Remove(const K& key)
        {
            usize i = FindIndex(key);
            if (i == npos)
                return false;
            slots_[i].~Slot();
            size_--;
            if (ctrl_[(i + 1) & (capacity_ - 1)] == kEmpty)
            {
                ctrl_[i] = kEmpty;
            }
            else
            {
                ctrl_[i] = kDeleted;
                deleted_++;
            }
            return true;
        }

        void Clear()
        {
            for (usize i = 0; i < capacity_; i++)
            {
                if (!(ctrl_[i] & 0x80))
                    slots_[i].~Slot();
            }
            if (ctrl_)
                std::memset(ctrl_, kEmpty, capacity_);
            size_ = deleted_ = 0;
        }

        void Reserve(usize count)
        {
            usize needed = NextPowerOfTwo(Max<usize>(16, count + count / 7 + 1));
            if (needed > capacity_)
                Rehash(needed);
        }

    private:
        static constexpr usize npos = ~usize(0);

        static u64 HashOf(const K& key) { return H{}(key); }

        usize FindIndex(const K& key) const
        {
            if (size_ == 0)
                return npos;
            u64 h = HashOf(key);
            u8 tag = u8(h & 0x7F);
            usize mask = capacity_ - 1;
            for (usize i = usize(h >> 7) & mask;; i = (i + 1) & mask)
            {
                u8 c = ctrl_[i];
                if (c == kEmpty)
                    return npos;
                if (c == tag && slots_[i].key == key)
                    return i;
            }
        }

        usize FindOrPrepare(const K& key, bool& inserted)
        {
            if ((size_ + deleted_ + 1) * 8 > capacity_ * 7)
                Rehash(size_ * 2 + 2 > capacity_ ? Max<usize>(16, capacity_ * 2) : capacity_);
            u64 h = HashOf(key);
            u8 tag = u8(h & 0x7F);
            usize mask = capacity_ - 1;
            usize target = npos;
            for (usize i = usize(h >> 7) & mask;; i = (i + 1) & mask)
            {
                u8 c = ctrl_[i];
                if (c == kEmpty)
                {
                    if (target == npos)
                        target = i;
                    break;
                }
                if (c == kDeleted)
                {
                    if (target == npos)
                        target = i;
                    continue;
                }
                if (c == tag && slots_[i].key == key)
                {
                    inserted = false;
                    return i;
                }
            }
            if (ctrl_[target] == kDeleted)
                deleted_--;
            ctrl_[target] = tag;
            size_++;
            inserted = true;
            return target;
        }

        void Rehash(usize capacity)
        {
            HX_ASSERT(IsPowerOfTwo(capacity));
            u8* oldCtrl = ctrl_;
            Slot* oldSlots = slots_;
            usize oldCapacity = capacity_;

            ctrl_ = static_cast<u8*>(allocator_->Allocate(capacity, 16));
            slots_ = static_cast<Slot*>(allocator_->Allocate(capacity * sizeof(Slot), alignof(Slot)));
            std::memset(ctrl_, kEmpty, capacity);
            capacity_ = capacity;
            deleted_ = 0;

            usize mask = capacity - 1;
            for (usize j = 0; j < oldCapacity; j++)
            {
                if (oldCtrl[j] & 0x80)
                    continue;
                u64 h = HashOf(oldSlots[j].key);
                usize i = usize(h >> 7) & mask;
                while (ctrl_[i] != kEmpty)
                    i = (i + 1) & mask;
                ctrl_[i] = u8(h & 0x7F);
                ::new (&slots_[i]) Slot(Move(oldSlots[j]));
                oldSlots[j].~Slot();
            }
            if (oldCtrl)
            {
                allocator_->Free(oldCtrl, oldCapacity, 16);
                allocator_->Free(oldSlots, oldCapacity * sizeof(Slot), alignof(Slot));
            }
        }

        void Release()
        {
            if (!ctrl_)
                return;
            allocator_->Free(ctrl_, capacity_, 16);
            allocator_->Free(slots_, capacity_ * sizeof(Slot), alignof(Slot));
            ctrl_ = nullptr;
            slots_ = nullptr;
            capacity_ = 0;
        }

        Allocator* allocator_;
        u8* ctrl_ = nullptr;
        Slot* slots_ = nullptr;
        usize capacity_ = 0;
        usize size_ = 0;
        usize deleted_ = 0;
    };
}
