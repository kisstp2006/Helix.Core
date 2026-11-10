#pragma once

#include <helix/core/base/types.h>

namespace helix
{
    struct Md5Digest
    {
        u8 bytes[16];

        bool operator==(const Md5Digest&) const = default;
    };

    class Md5
    {
    public:
        void Update(const void* data, usize size);
        Md5Digest Finish();

        static void ProcessBlock(u32 state[4], const u8 block[64]);
        static constexpr u32 kInitialState[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};

    private:
        u32 state_[4] = {kInitialState[0], kInitialState[1], kInitialState[2], kInitialState[3]};
        u8 buffer_[64] = {};
        u64 length_ = 0;
    };

    Md5Digest ComputeMd5(const void* data, usize size);
}
