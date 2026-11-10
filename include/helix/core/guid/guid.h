#pragma once

#include <helix/core/strings/string.h>

#include <compare>

namespace helix
{
    struct Guid
    {
        u8 bytes[16] = {};

        static Guid Generate();
        static bool TryParse(StringView text, Guid& out);

        void ToChars(char out[33]) const;
        String ToString() const;
        bool IsValid() const;

        bool operator==(const Guid&) const = default;
        auto operator<=>(const Guid&) const = default;
    };

    template <>
    struct Hasher<Guid>
    {
        u64 operator()(const Guid& g) const { return Hash64(g.bytes, sizeof(g.bytes)); }
    };

    void FillRandomBytes(void* buffer, usize size);
}
