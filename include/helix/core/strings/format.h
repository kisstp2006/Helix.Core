#pragma once

#include <helix/core/base/span.h>
#include <helix/core/strings/string.h>

#include <type_traits>

namespace helix
{
    struct FormatArg
    {
        enum class Type : u8
        {
            None,
            Bool,
            Char,
            Int,
            UInt,
            Double,
            Text,
            Pointer,
        };

        struct TextValue
        {
            const char* data;
            usize size;
        };

        Type type = Type::None;
        union
        {
            bool b;
            char c;
            i64 i;
            u64 u;
            double d;
            const void* p;
            TextValue text;
        };

        constexpr FormatArg() : u(0) {}
    };

    template <typename T>
    FormatArg MakeFormatArg(const T& value)
    {
        using U = std::remove_cvref_t<T>;
        FormatArg a;
        if constexpr (std::is_same_v<U, bool>)
        {
            a.type = FormatArg::Type::Bool;
            a.b = value;
        }
        else if constexpr (std::is_same_v<U, char>)
        {
            a.type = FormatArg::Type::Char;
            a.c = value;
        }
        else if constexpr (std::is_enum_v<U>)
        {
            a.type = FormatArg::Type::Int;
            a.i = static_cast<i64>(value);
        }
        else if constexpr (std::is_integral_v<U> && std::is_signed_v<U>)
        {
            a.type = FormatArg::Type::Int;
            a.i = value;
        }
        else if constexpr (std::is_integral_v<U>)
        {
            a.type = FormatArg::Type::UInt;
            a.u = value;
        }
        else if constexpr (std::is_floating_point_v<U>)
        {
            a.type = FormatArg::Type::Double;
            a.d = double(value);
        }
        else if constexpr (std::is_null_pointer_v<U>)
        {
            a.type = FormatArg::Type::Pointer;
            a.p = nullptr;
        }
        else if constexpr (std::is_convertible_v<const U&, StringView>)
        {
            StringView s = value;
            a.type = FormatArg::Type::Text;
            a.text = {s.Data(), s.Size()};
        }
        else if constexpr (std::is_pointer_v<U>)
        {
            a.type = FormatArg::Type::Pointer;
            a.p = value;
        }
        else
        {
            static_assert(sizeof(U) == 0, "type is not formattable");
        }
        return a;
    }

    void FormatArgs(String& out, StringView format, Span<const FormatArg> args);
    usize FormatArgsToBuffer(char* buffer, usize size, StringView format, Span<const FormatArg> args);

    // Spec: {} or {:[<|>][0][width][.precision][x|X|b|e|f|g|p]}, {{ and }} are escapes.
    template <typename... Args>
    void FormatAppend(String& out, StringView format, const Args&... args)
    {
        FormatArg list[sizeof...(Args) + 1] = {MakeFormatArg(args)...};
        FormatArgs(out, format, Span<const FormatArg>(list, sizeof...(Args)));
    }

    template <typename... Args>
    String Format(StringView format, const Args&... args)
    {
        String out;
        FormatAppend(out, format, args...);
        return out;
    }

    template <typename... Args>
    usize FormatToBuffer(char* buffer, usize size, StringView format, const Args&... args)
    {
        FormatArg list[sizeof...(Args) + 1] = {MakeFormatArg(args)...};
        return FormatArgsToBuffer(buffer, size, format, Span<const FormatArg>(list, sizeof...(Args)));
    }
}
