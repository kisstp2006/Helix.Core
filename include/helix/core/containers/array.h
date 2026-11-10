#pragma once

#include <helix/core/base/span.h>
#include <helix/core/memory/allocator.h>

#include <cstring>
#include <initializer_list>
#include <type_traits>

namespace helix
{
    template <typename T>
    class Array
    {
    public:
        explicit Array(Allocator& allocator = HeapAllocator()) : allocator_(&allocator) {}

        Array(std::initializer_list<T> values, Allocator& allocator = HeapAllocator()) : allocator_(&allocator)
        {
            Reserve(values.size());
            for (const T& v : values)
                ::new (data_ + size_++) T(v);
        }

        Array(const Array& other) : allocator_(other.allocator_) { CopyFrom(other); }

        Array(Array&& other) noexcept : allocator_(other.allocator_), data_(other.data_), size_(other.size_), capacity_(other.capacity_)
        {
            other.data_ = nullptr;
            other.size_ = other.capacity_ = 0;
        }

        ~Array()
        {
            Clear();
            Release();
        }

        Array& operator=(const Array& other)
        {
            if (this != &other)
            {
                Clear();
                CopyFrom(other);
            }
            return *this;
        }

        Array& operator=(Array&& other) noexcept
        {
            if (this != &other)
            {
                Clear();
                Release();
                allocator_ = other.allocator_;
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;
                other.data_ = nullptr;
                other.size_ = other.capacity_ = 0;
            }
            return *this;
        }

        T* Data() { return data_; }
        const T* Data() const { return data_; }
        usize Size() const { return size_; }
        usize Capacity() const { return capacity_; }
        bool Empty() const { return size_ == 0; }
        Allocator& GetAllocator() const { return *allocator_; }

        T* begin() { return data_; }
        T* end() { return data_ + size_; }
        const T* begin() const { return data_; }
        const T* end() const { return data_ + size_; }

        T& operator[](usize i)
        {
            HX_ASSERT(i < size_);
            return data_[i];
        }

        const T& operator[](usize i) const
        {
            HX_ASSERT(i < size_);
            return data_[i];
        }

        T& Front() { return (*this)[0]; }
        T& Back() { return (*this)[size_ - 1]; }
        const T& Back() const { return (*this)[size_ - 1]; }

        operator Span<T>() { return {data_, size_}; }
        operator Span<const T>() const { return {data_, size_}; }

        void Reserve(usize capacity)
        {
            if (capacity > capacity_)
                Reallocate(capacity);
        }

        template <typename... Args>
        T& Emplace(Args&&... args)
        {
            if (size_ == capacity_)
                Reallocate(GrowCapacity(size_ + 1));
            return *::new (data_ + size_++) T(Forward<Args>(args)...);
        }

        void Push(const T& value)
        {
            if (size_ == capacity_ && &value >= data_ && &value < data_ + size_)
            {
                T copy(value);
                Emplace(Move(copy));
                return;
            }
            Emplace(value);
        }

        void Push(T&& value) { Emplace(Move(value)); }

        void Pop()
        {
            HX_ASSERT(size_ > 0);
            data_[--size_].~T();
        }

        void Insert(usize index, T value)
        {
            HX_ASSERT(index <= size_);
            Emplace(Move(value));
            for (usize i = size_ - 1; i > index; i--)
                Swap(data_[i], data_[i - 1]);
        }

        void RemoveAt(usize index)
        {
            HX_ASSERT(index < size_);
            for (usize i = index; i + 1 < size_; i++)
                data_[i] = Move(data_[i + 1]);
            Pop();
        }

        void RemoveAtSwap(usize index)
        {
            HX_ASSERT(index < size_);
            if (index != size_ - 1)
                data_[index] = Move(data_[size_ - 1]);
            Pop();
        }

        void Resize(usize size)
        {
            if (size > capacity_)
                Reallocate(GrowCapacity(size));
            if constexpr (std::is_trivially_default_constructible_v<T> && std::is_trivially_destructible_v<T>)
            {
                if (size > size_)
                    std::memset(static_cast<void*>(data_ + size_), 0, (size - size_) * sizeof(T));
                size_ = size;
            }
            else
            {
                while (size_ < size)
                    ::new (data_ + size_++) T();
                while (size_ > size)
                    Pop();
            }
        }

        void Resize(usize size, const T& value)
        {
            if (size > capacity_)
                Reallocate(GrowCapacity(size));
            while (size_ < size)
                ::new (data_ + size_++) T(value);
            while (size_ > size)
                Pop();
        }

        void Clear()
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
                for (usize i = 0; i < size_; i++)
                    data_[i].~T();
            size_ = 0;
        }

        void ShrinkToFit()
        {
            if (size_ < capacity_)
                Reallocate(size_);
        }

        T* Find(const T& value)
        {
            for (T& v : *this)
                if (v == value)
                    return &v;
            return nullptr;
        }

        bool Contains(const T& value) const { return const_cast<Array*>(this)->Find(value) != nullptr; }

    private:
        static usize GrowCapacity(usize needed)
        {
            return Max<usize>(needed, Max<usize>(8, needed + needed / 2));
        }

        void Reallocate(usize capacity)
        {
            T* memory = capacity ? static_cast<T*>(allocator_->Allocate(capacity * sizeof(T), alignof(T))) : nullptr;
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                if (size_)
                    std::memcpy(static_cast<void*>(memory), data_, size_ * sizeof(T));
            }
            else
            {
                for (usize i = 0; i < size_; i++)
                {
                    ::new (memory + i) T(Move(data_[i]));
                    data_[i].~T();
                }
            }
            Release();
            data_ = memory;
            capacity_ = capacity;
        }

        void Release()
        {
            if (data_)
                allocator_->Free(data_, capacity_ * sizeof(T), alignof(T));
            data_ = nullptr;
            capacity_ = 0;
        }

        void CopyFrom(const Array& other)
        {
            Reserve(other.size_);
            for (usize i = 0; i < other.size_; i++)
                ::new (data_ + i) T(other.data_[i]);
            size_ = other.size_;
        }

        Allocator* allocator_;
        T* data_ = nullptr;
        usize size_ = 0;
        usize capacity_ = 0;
    };
}
