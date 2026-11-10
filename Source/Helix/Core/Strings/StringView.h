#pragma once

#include <Helix/Core/Basics/Assert.h>
#include <Helix/Core/Hashing/Hash.h>

namespace helix
{
    class StringView
    {
    public:
        static constexpr usize npos = ~usize(0);

        constexpr StringView() = default;
        constexpr StringView(const char* text) : data_(text ? text : ""), size_(Length(text)) {}
        constexpr StringView(const char* text, usize size) : data_(text), size_(size) {}

        constexpr const char* Data() const { return data_; }
        constexpr usize Size() const { return size_; }
        constexpr bool Empty() const { return size_ == 0; }
        constexpr const char* begin() const { return data_; }
        constexpr const char* end() const { return data_ + size_; }

        constexpr char operator[](usize i) const
        {
            HX_ASSERT(i < size_);
            return data_[i];
        }

        constexpr StringView Substr(usize offset, usize count = npos) const
        {
            if (offset > size_)
                offset = size_;
            return {data_ + offset, count < size_ - offset ? count : size_ - offset};
        }

        constexpr usize Find(char c, usize from = 0) const
        {
            for (usize i = from; i < size_; i++)
                if (data_[i] == c)
                    return i;
            return npos;
        }

        constexpr usize Find(StringView needle, usize from = 0) const
        {
            if (needle.size_ == 0)
                return from <= size_ ? from : npos;
            for (usize i = from; i + needle.size_ <= size_; i++)
                if (Substr(i, needle.size_) == needle)
                    return i;
            return npos;
        }

        constexpr usize FindLast(char c) const
        {
            for (usize i = size_; i-- > 0;)
                if (data_[i] == c)
                    return i;
            return npos;
        }

        constexpr bool StartsWith(StringView prefix) const { return prefix.size_ <= size_ && Substr(0, prefix.size_) == prefix; }
        constexpr bool EndsWith(StringView suffix) const { return suffix.size_ <= size_ && Substr(size_ - suffix.size_) == suffix; }
        constexpr bool Contains(StringView needle) const { return Find(needle) != npos; }

        constexpr StringView Trimmed() const
        {
            usize a = 0, b = size_;
            while (a < b && IsSpace(data_[a]))
                a++;
            while (b > a && IsSpace(data_[b - 1]))
                b--;
            return Substr(a, b - a);
        }

        constexpr int Compare(StringView other) const
        {
            usize n = size_ < other.size_ ? size_ : other.size_;
            for (usize i = 0; i < n; i++)
                if (data_[i] != other.data_[i])
                    return u8(data_[i]) < u8(other.data_[i]) ? -1 : 1;
            return size_ == other.size_ ? 0 : (size_ < other.size_ ? -1 : 1);
        }

        constexpr bool operator==(StringView other) const { return size_ == other.size_ && Compare(other) == 0; }
        constexpr bool operator<(StringView other) const { return Compare(other) < 0; }

    private:
        static constexpr usize Length(const char* s)
        {
            usize n = 0;
            if (s)
                while (s[n])
                    n++;
            return n;
        }

        static constexpr bool IsSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }

        const char* data_ = "";
        usize size_ = 0;
    };

    template <>
    struct Hasher<StringView>
    {
        u64 operator()(StringView s) const { return Hash64(s.Data(), s.Size()); }
    };

    namespace literals
    {
        constexpr StringView operator""_sv(const char* text, usize size) { return {text, size}; }
    }
}
