#pragma once

#include <helix/core/memory/allocator.h>
#include <helix/core/strings/string_view.h>

namespace helix
{
    class String
    {
    public:
        static constexpr usize kInlineCapacity = 23;

        explicit String(Allocator& allocator = HeapAllocator());
        String(StringView text, Allocator& allocator = HeapAllocator());
        String(const char* text, Allocator& allocator = HeapAllocator()) : String(StringView(text), allocator) {}
        String(const String& other);
        String(String&& other) noexcept;
        ~String();

        String& operator=(const String& other);
        String& operator=(String&& other) noexcept;
        String& operator=(StringView text);

        const char* Data() const { return data_; }
        char* Data() { return data_; }
        const char* CStr() const { return data_; }
        usize Size() const { return size_; }
        usize Capacity() const { return capacity_; }
        bool Empty() const { return size_ == 0; }
        bool IsInline() const { return data_ == inline_; }
        StringView View() const { return {data_, size_}; }
        operator StringView() const { return View(); }

        char& operator[](usize i)
        {
            HX_ASSERT(i < size_);
            return data_[i];
        }

        char operator[](usize i) const
        {
            HX_ASSERT(i < size_);
            return data_[i];
        }

        void Reserve(usize capacity);
        void Resize(usize size, char fill = '\0');
        void Clear();
        String& Append(StringView text);
        String& Append(char c);
        String& Append(char c, usize count);
        String& operator+=(StringView text) { return Append(text); }
        String& operator+=(char c) { return Append(c); }

        bool operator==(StringView other) const { return View() == other; }
        bool operator==(const String& other) const { return View() == other.View(); }
        bool operator==(const char* other) const { return View() == StringView(other); }
        bool operator<(const String& other) const { return View() < other.View(); }

    private:
        void Grow(usize needed);
        void ReleaseHeap();

        Allocator* allocator_;
        char* data_;
        usize size_ = 0;
        usize capacity_ = kInlineCapacity;
        char inline_[kInlineCapacity + 1];
    };

    template <>
    struct Hasher<String>
    {
        u64 operator()(const String& s) const { return Hash64(s.Data(), s.Size()); }
    };
}
