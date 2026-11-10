#include <helix/core/hash/md5.h>

#include <cstring>

namespace helix
{
    namespace
    {
        constexpr u32 K[64] = {
            0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
            0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
            0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
            0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
            0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
            0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
            0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
            0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

        constexpr int R[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                               5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
                               4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                               6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

        inline u32 Rotl(u32 x, int s) { return (x << s) | (x >> (32 - s)); }
    }

    void Md5::ProcessBlock(u32 state[4], const u8 block[64])
    {
        u32 m[16];
        for (int i = 0; i < 16; i++)
            m[i] = u32(block[i * 4]) | (u32(block[i * 4 + 1]) << 8) | (u32(block[i * 4 + 2]) << 16) | (u32(block[i * 4 + 3]) << 24);
        u32 a = state[0], b = state[1], c = state[2], d = state[3];
        for (int i = 0; i < 64; i++)
        {
            u32 f;
            int g;
            if (i < 16) { f = (b & c) | (~b & d); g = i; }
            else if (i < 32) { f = (d & b) | (~d & c); g = (5 * i + 1) & 15; }
            else if (i < 48) { f = b ^ c ^ d; g = (3 * i + 5) & 15; }
            else { f = c ^ (b | ~d); g = (7 * i) & 15; }
            u32 t = d;
            d = c;
            c = b;
            b = b + Rotl(a + f + K[i] + m[g], R[i]);
            a = t;
        }
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
    }

    void Md5::Update(const void* data, usize size)
    {
        const u8* p = static_cast<const u8*>(data);
        usize used = usize(length_ & 63);
        length_ += size;
        if (used)
        {
            usize take = 64 - used < size ? 64 - used : size;
            std::memcpy(buffer_ + used, p, take);
            p += take;
            size -= take;
            if (used + take < 64)
                return;
            ProcessBlock(state_, buffer_);
        }
        for (; size >= 64; p += 64, size -= 64)
            ProcessBlock(state_, p);
        std::memcpy(buffer_, p, size);
    }

    Md5Digest Md5::Finish()
    {
        u64 bits = length_ * 8;
        u8 pad[72] = {0x80};
        usize used = usize(length_ & 63);
        usize padLength = used < 56 ? 56 - used : 120 - used;
        Update(pad, padLength);
        u8 tail[8];
        for (int i = 0; i < 8; i++)
            tail[i] = u8(bits >> (8 * i));
        Update(tail, 8);
        Md5Digest digest;
        for (int i = 0; i < 16; i++)
            digest.bytes[i] = u8(state_[i / 4] >> (8 * (i % 4)));
        return digest;
    }

    Md5Digest ComputeMd5(const void* data, usize size)
    {
        Md5 md5;
        md5.Update(data, size);
        return md5.Finish();
    }
}
