#pragma once

#include <Helix/Core/Basics/Types.h>

#include <type_traits>

namespace helix
{
    u64 Hash64(const void* data, usize size, u64 seed = 0);
    u32 Crc32(const void* data, usize size, u32 crc = 0);

    constexpr u64 Fnv1a64(const char* data, usize size)
    {
        u64 h = 0xcbf29ce484222325ull;
        for (usize i = 0; i < size; i++)
        {
            h ^= u8(data[i]);
            h *= 0x100000001b3ull;
        }
        return h;
    }

    constexpr u64 MixHash(u64 x)
    {
        x ^= x >> 33;
        x *= 0xff51afd7ed558ccdull;
        x ^= x >> 33;
        x *= 0xc4ceb9fe1a85ec53ull;
        x ^= x >> 33;
        return x;
    }

    constexpr u64 CombineHash(u64 seed, u64 value) { return MixHash(seed ^ (value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2))); }

    template <typename T>
    struct Hasher;

    template <typename T>
        requires std::is_integral_v<T> || std::is_enum_v<T>
    struct Hasher<T>
    {
        constexpr u64 operator()(T value) const { return MixHash(u64(value)); }
    };

    template <typename T>
    struct Hasher<T*>
    {
        u64 operator()(const T* value) const { return MixHash(u64(reinterpret_cast<uptr>(value))); }
    };
}
