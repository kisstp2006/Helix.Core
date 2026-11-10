#include <helix/core/guid/guid.h>

namespace helix
{
    namespace
    {
        int HexValue(char c)
        {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            return -1;
        }
    }

    Guid Guid::Generate()
    {
        Guid g;
        FillRandomBytes(g.bytes, sizeof(g.bytes));
        g.bytes[6] = u8((g.bytes[6] & 0x0F) | 0x40);
        g.bytes[8] = u8((g.bytes[8] & 0x3F) | 0x80);
        return g;
    }

    bool Guid::TryParse(StringView text, Guid& out)
    {
        if (text.Size() != 32)
            return false;
        Guid g;
        for (usize i = 0; i < 16; i++)
        {
            int hi = HexValue(text[i * 2]);
            int lo = HexValue(text[i * 2 + 1]);
            if (hi < 0 || lo < 0)
                return false;
            g.bytes[i] = u8(hi << 4 | lo);
        }
        out = g;
        return true;
    }

    void Guid::ToChars(char out[33]) const
    {
        const char* digits = "0123456789abcdef";
        for (usize i = 0; i < 16; i++)
        {
            out[i * 2] = digits[bytes[i] >> 4];
            out[i * 2 + 1] = digits[bytes[i] & 15];
        }
        out[32] = '\0';
    }

    String Guid::ToString() const
    {
        char text[33];
        ToChars(text);
        return String(StringView(text, 32));
    }

    bool Guid::IsValid() const
    {
        for (u8 b : bytes)
            if (b)
                return true;
        return false;
    }
}
