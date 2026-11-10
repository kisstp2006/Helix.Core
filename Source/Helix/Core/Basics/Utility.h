#pragma once

#include <Helix/Core/Basics/Types.h>

#include <type_traits>

namespace helix
{
    template <typename T>
    constexpr std::remove_reference_t<T>&& Move(T&& value) noexcept
    {
        return static_cast<std::remove_reference_t<T>&&>(value);
    }

    template <typename T>
    constexpr T&& Forward(std::remove_reference_t<T>& value) noexcept
    {
        return static_cast<T&&>(value);
    }

    template <typename T>
    constexpr T&& Forward(std::remove_reference_t<T>&& value) noexcept
    {
        return static_cast<T&&>(value);
    }

    template <typename T>
    constexpr void Swap(T& a, T& b) noexcept
    {
        T t = Move(a);
        a = Move(b);
        b = Move(t);
    }

    template <typename T>
    constexpr T Min(T a, T b) { return b < a ? b : a; }

    template <typename T>
    constexpr T Max(T a, T b) { return a < b ? b : a; }

    template <typename T>
    constexpr T Clamp(T v, T lo, T hi) { return v < lo ? lo : (hi < v ? hi : v); }

    constexpr bool IsPowerOfTwo(u64 v) { return v != 0 && (v & (v - 1)) == 0; }

    constexpr u64 NextPowerOfTwo(u64 v)
    {
        if (v <= 1)
            return 1;
        v--;
        for (u32 shift = 1; shift < 64; shift <<= 1)
            v |= v >> shift;
        return v + 1;
    }

    constexpr usize AlignUp(usize value, usize alignment) { return (value + alignment - 1) & ~(alignment - 1); }

    template <typename T, usize N>
    constexpr usize CountOf(const T (&)[N]) { return N; }
}
