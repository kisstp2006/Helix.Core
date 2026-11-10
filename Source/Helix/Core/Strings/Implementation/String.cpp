#include <Helix/Core/Strings/String.h>

#include <cstring>

namespace helix
{
    String::String(Allocator& allocator) : allocator_(&allocator), data_(inline_) { inline_[0] = '\0'; }

    String::String(StringView text, Allocator& allocator) : String(allocator) { Append(text); }

    String::String(const String& other) : String(*other.allocator_) { Append(other.View()); }

    String::String(String&& other) noexcept : allocator_(other.allocator_), data_(inline_), size_(other.size_)
    {
        if (other.IsInline())
        {
            std::memcpy(inline_, other.inline_, other.size_ + 1);
        }
        else
        {
            data_ = other.data_;
            capacity_ = other.capacity_;
            other.data_ = other.inline_;
            other.capacity_ = kInlineCapacity;
        }
        other.size_ = 0;
        other.inline_[0] = '\0';
    }

    String::~String() { ReleaseHeap(); }

    String& String::operator=(const String& other)
    {
        if (this != &other)
        {
            Clear();
            Append(other.View());
        }
        return *this;
    }

    String& String::operator=(String&& other) noexcept
    {
        if (this == &other)
            return *this;
        ReleaseHeap();
        allocator_ = other.allocator_;
        size_ = other.size_;
        if (other.IsInline())
        {
            data_ = inline_;
            capacity_ = kInlineCapacity;
            std::memcpy(inline_, other.inline_, other.size_ + 1);
        }
        else
        {
            data_ = other.data_;
            capacity_ = other.capacity_;
            other.data_ = other.inline_;
            other.capacity_ = kInlineCapacity;
        }
        other.size_ = 0;
        other.inline_[0] = '\0';
        return *this;
    }

    String& String::operator=(StringView text)
    {
        if (text.Data() >= data_ && text.Data() < data_ + size_)
        {
            String copy(text, *allocator_);
            return *this = Move(copy);
        }
        Clear();
        return Append(text);
    }

    void String::Reserve(usize capacity)
    {
        if (capacity > capacity_)
            Grow(capacity);
    }

    void String::Resize(usize size, char fill)
    {
        Reserve(size);
        if (size > size_)
            std::memset(data_ + size_, fill, size - size_);
        size_ = size;
        data_[size_] = '\0';
    }

    void String::Clear()
    {
        size_ = 0;
        data_[0] = '\0';
    }

    String& String::Append(StringView text)
    {
        if (text.Empty())
            return *this;
        if (size_ + text.Size() > capacity_)
        {
            if (text.Data() >= data_ && text.Data() < data_ + size_)
            {
                String copy(text, *allocator_);
                return Append(copy.View());
            }
            Grow(size_ + text.Size());
        }
        std::memmove(data_ + size_, text.Data(), text.Size());
        size_ += text.Size();
        data_[size_] = '\0';
        return *this;
    }

    String& String::Append(char c) { return Append(c, 1); }

    String& String::Append(char c, usize count)
    {
        if (size_ + count > capacity_)
            Grow(size_ + count);
        std::memset(data_ + size_, c, count);
        size_ += count;
        data_[size_] = '\0';
        return *this;
    }

    void String::Grow(usize needed)
    {
        usize capacity = Max(needed, capacity_ + capacity_ / 2);
        char* memory = static_cast<char*>(allocator_->Allocate(capacity + 1, 1));
        std::memcpy(memory, data_, size_ + 1);
        ReleaseHeap();
        data_ = memory;
        capacity_ = capacity;
    }

    void String::ReleaseHeap()
    {
        if (!IsInline())
            allocator_->Free(data_, capacity_ + 1, 1);
        data_ = inline_;
        capacity_ = kInlineCapacity;
    }
}
