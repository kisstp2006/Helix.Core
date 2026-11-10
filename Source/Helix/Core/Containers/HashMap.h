#pragma once

#include <Helix/Core/Basics/Assert.h>
#include <Helix/Core/Hashing/Hash.h>
#include <Helix/Core/Memory/Allocator.h>

#include <bit>
#include <cstring>

namespace helix
{
    // Open addressing with one control byte per slot: high bit set means empty or deleted,
    // otherwise the low 7 bits hold extra hash bits to skip most key compares.
    // Probing reads 8 control bytes at a time; the first 8 are mirrored after the end so a group never wraps.
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
                SetCtrl(i, kEmpty);
            }
            else
            {
                SetCtrl(i, kDeleted);
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
                std::memset(ctrl_, kEmpty, capacity_ + kGroup);
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

        static constexpr usize kGroup = 8;
        static constexpr u64 kLow7 = 0x7F7F7F7F7F7F7F7Full;

        static u64 LoadGroup(const u8* p)
        {
            u64 g;
            std::memcpy(&g, p, sizeof(g));
            return g;
        }

        // Bit 7 of every byte of the group that equals `value`, exact.
        static u64 MatchByte(u64 group, u8 value)
        {
            u64 x = group ^ (0x0101010101010101ull * value);
            return ~(((x & kLow7) + kLow7) | x | kLow7);
        }

        void SetCtrl(usize i, u8 value)
        {
            ctrl_[i] = value;
            if (i < kGroup)
                ctrl_[capacity_ + i] = value;
        }

        usize FindIndex(const K& key) const
        {
            if (size_ == 0)
                return npos;
            u64 h = HashOf(key);
            u8 tag = u8(h & 0x7F);
            usize mask = capacity_ - 1;
            for (usize i = usize(h >> 7) & mask;; i = (i + kGroup) & mask)
            {
                u64 group = LoadGroup(ctrl_ + i);
                for (u64 m = MatchByte(group, tag); m; m &= m - 1)
                {
                    usize at = (i + usize(std::countr_zero(m) >> 3)) & mask;
                    if (slots_[at].key == key)
                        return at;
                }
                if (MatchByte(group, kEmpty))
                    return npos;
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
            for (usize i = usize(h >> 7) & mask;; i = (i + kGroup) & mask)
            {
                u64 group = LoadGroup(ctrl_ + i);
                for (u64 m = MatchByte(group, tag); m; m &= m - 1)
                {
                    usize at = (i + usize(std::countr_zero(m) >> 3)) & mask;
                    if (slots_[at].key == key)
                    {
                        inserted = false;
                        return at;
                    }
                }
                // Empty and deleted bytes are the only ones with the high bit set; the first one is the insert position.
                u64 free = group & ~kLow7;
                if (target == npos && free)
                    target = (i + usize(std::countr_zero(free) >> 3)) & mask;
                if (MatchByte(group, kEmpty))
                    break;
            }
            if (ctrl_[target] == kDeleted)
                deleted_--;
            SetCtrl(target, tag);
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

            ctrl_ = static_cast<u8*>(allocator_->Allocate(capacity + kGroup, 16));
            slots_ = static_cast<Slot*>(allocator_->Allocate(capacity * sizeof(Slot), alignof(Slot)));
            std::memset(ctrl_, kEmpty, capacity + kGroup);
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
                SetCtrl(i, u8(h & 0x7F));
                ::new (&slots_[i]) Slot(Move(oldSlots[j]));
                oldSlots[j].~Slot();
            }
            if (oldCtrl)
            {
                allocator_->Free(oldCtrl, oldCapacity + kGroup, 16);
                allocator_->Free(oldSlots, oldCapacity * sizeof(Slot), alignof(Slot));
            }
        }

        void Release()
        {
            if (!ctrl_)
                return;
            allocator_->Free(ctrl_, capacity_ + kGroup, 16);
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
