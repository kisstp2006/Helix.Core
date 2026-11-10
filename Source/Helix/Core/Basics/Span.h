#pragma once

#include <Helix/Core/Basics/Assert.h>
#include <Helix/Core/Basics/Types.h>

#include <type_traits>

namespace helix
{
    template <typename T>
    class Span
    {
    public:
        constexpr Span() = default;
        constexpr Span(T* data, usize size) : data_(data), size_(size) {}

        template <usize N>
        constexpr Span(T (&array)[N]) : data_(array), size_(N) {}

        template <typename U>
            requires std::is_same_v<const U, T>
        constexpr Span(Span<U> other) : data_(other.Data()), size_(other.Size()) {}

        constexpr T* Data() const { return data_; }
        constexpr usize Size() const { return size_; }
        constexpr usize SizeInBytes() const { return size_ * sizeof(T); }
        constexpr bool Empty() const { return size_ == 0; }
        constexpr T* begin() const { return data_; }
        constexpr T* end() const { return data_ + size_; }

        constexpr T& operator[](usize i) const
        {
            HX_ASSERT(i < size_);
            return data_[i];
        }

        constexpr Span Subspan(usize offset, usize count) const
        {
            HX_ASSERT(offset + count <= size_);
            return Span(data_ + offset, count);
        }

    private:
        T* data_ = nullptr;
        usize size_ = 0;
    };
}
