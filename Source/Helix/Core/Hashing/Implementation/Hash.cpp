#include <Helix/Core/Hashing/Hash.h>

#include <cstring>

namespace helix
{
    namespace
    {
        constexpr u64 P1 = 11400714785074694791ull;
        constexpr u64 P2 = 14029467366897019727ull;
        constexpr u64 P3 = 1609587929392839161ull;
        constexpr u64 P4 = 9650029242287828579ull;
        constexpr u64 P5 = 2870177450012600261ull;

        inline u64 Rotl(u64 x, int r) { return (x << r) | (x >> (64 - r)); }

        inline u64 Read64(const u8* p)
        {
            u64 v;
            std::memcpy(&v, p, 8);
            return v;
        }

        inline u32 Read32(const u8* p)
        {
            u32 v;
            std::memcpy(&v, p, 4);
            return v;
        }

        inline u64 Round(u64 acc, u64 input) { return Rotl(acc + input * P2, 31) * P1; }
        inline u64 Merge(u64 acc, u64 value) { return (acc ^ Round(0, value)) * P1 + P4; }

        struct CrcTable
        {
            u32 values[256];

            constexpr CrcTable() : values()
            {
                for (u32 i = 0; i < 256; i++)
                {
                    u32 c = i;
                    for (int k = 0; k < 8; k++)
                        c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
                    values[i] = c;
                }
            }
        };

        constexpr CrcTable crcTable;
    }

    u64 Hash64(const void* data, usize size, u64 seed)
    {
        const u8* p = static_cast<const u8*>(data);
        const u8* end = p + size;
        u64 h;
        if (size >= 32)
        {
            u64 v1 = seed + P1 + P2, v2 = seed + P2, v3 = seed, v4 = seed - P1;
            for (; p + 32 <= end; p += 32)
            {
                v1 = Round(v1, Read64(p));
                v2 = Round(v2, Read64(p + 8));
                v3 = Round(v3, Read64(p + 16));
                v4 = Round(v4, Read64(p + 24));
            }
            h = Rotl(v1, 1) + Rotl(v2, 7) + Rotl(v3, 12) + Rotl(v4, 18);
            h = Merge(h, v1);
            h = Merge(h, v2);
            h = Merge(h, v3);
            h = Merge(h, v4);
        }
        else
        {
            h = seed + P5;
        }
        h += u64(size);
        for (; p + 8 <= end; p += 8)
            h = Rotl(h ^ Round(0, Read64(p)), 27) * P1 + P4;
        if (p + 4 <= end)
        {
            h = Rotl(h ^ (u64(Read32(p)) * P1), 23) * P2 + P3;
            p += 4;
        }
        for (; p < end; p++)
            h = Rotl(h ^ (u64(*p) * P5), 11) * P1;
        h ^= h >> 33;
        h *= P2;
        h ^= h >> 29;
        h *= P3;
        h ^= h >> 32;
        return h;
    }

    u32 Crc32(const void* data, usize size, u32 crc)
    {
        const u8* p = static_cast<const u8*>(data);
        crc = ~crc;
        for (usize i = 0; i < size; i++)
            crc = crcTable.values[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
        return ~crc;
    }
}
