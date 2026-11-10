#include <helix/core/strings/format.h>

#include <charconv>
#include <cstdio>
#include <cstring>

namespace helix
{
    namespace
    {
        struct Spec
        {
            char align = 0;
            bool zero = false;
            int width = 0;
            int precision = -1;
            char type = 0;
        };

        class Writer
        {
        public:
            virtual void Write(const char* data, usize size) = 0;
            void Write(char c, usize count)
            {
                char chunk[32];
                std::memset(chunk, c, sizeof(chunk));
                while (count)
                {
                    usize n = Min<usize>(count, sizeof(chunk));
                    Write(chunk, n);
                    count -= n;
                }
            }

        protected:
            ~Writer() = default;
        };

        class StringWriter final : public Writer
        {
        public:
            explicit StringWriter(String& out) : out_(out) {}
            void Write(const char* data, usize size) override { out_.Append(StringView(data, size)); }
            using Writer::Write;

        private:
            String& out_;
        };

        class BufferWriter final : public Writer
        {
        public:
            BufferWriter(char* buffer, usize size) : buffer_(buffer), size_(size) {}
            void Write(const char* data, usize size) override
            {
                if (written_ < size_)
                    std::memcpy(buffer_ + written_, data, Min(size, size_ - written_));
                written_ += size;
            }
            using Writer::Write;
            usize Written() const { return written_; }

        private:
            char* buffer_;
            usize size_;
            usize written_ = 0;
        };

        bool ParseSpec(StringView s, Spec& spec)
        {
            usize i = 0;
            if (i < s.Size() && (s[i] == '<' || s[i] == '>'))
                spec.align = s[i++];
            if (i < s.Size() && s[i] == '0')
            {
                spec.zero = true;
                i++;
            }
            while (i < s.Size() && s[i] >= '0' && s[i] <= '9')
                spec.width = spec.width * 10 + (s[i++] - '0');
            if (i < s.Size() && s[i] == '.')
            {
                i++;
                spec.precision = 0;
                while (i < s.Size() && s[i] >= '0' && s[i] <= '9')
                    spec.precision = spec.precision * 10 + (s[i++] - '0');
            }
            if (i < s.Size())
                spec.type = s[i++];
            return i == s.Size();
        }

        usize FormatUnsigned(char* out, u64 value, int base, bool upper)
        {
            const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
            char tmp[64];
            usize n = 0;
            do
            {
                tmp[n++] = digits[value % u64(base)];
                value /= u64(base);
            } while (value);
            for (usize i = 0; i < n; i++)
                out[i] = tmp[n - 1 - i];
            return n;
        }

        void Pad(Writer& w, const Spec& spec, const char* sign, usize signSize, const char* body, usize bodySize, bool numeric)
        {
            usize total = signSize + bodySize;
            usize pad = spec.width > 0 && usize(spec.width) > total ? usize(spec.width) - total : 0;
            bool right = spec.align == '>' || (spec.align == 0 && numeric);
            if (spec.zero && numeric && spec.align == 0)
            {
                w.Write(sign, signSize);
                w.Write('0', pad);
                w.Write(body, bodySize);
                return;
            }
            if (right)
                w.Write(' ', pad);
            w.Write(sign, signSize);
            w.Write(body, bodySize);
            if (!right)
                w.Write(' ', pad);
        }

        void WriteInteger(Writer& w, const Spec& spec, u64 magnitude, bool negative)
        {
            int base = spec.type == 'x' || spec.type == 'X' ? 16 : (spec.type == 'b' ? 2 : 10);
            char body[64];
            usize n = FormatUnsigned(body, magnitude, base, spec.type == 'X');
            Pad(w, spec, "-", negative ? 1 : 0, body, n, true);
        }

        void WriteDouble(Writer& w, const Spec& spec, double value)
        {
            char body[400];
            usize n = 0;
            if (spec.precision < 0 && spec.type == 0)
            {
                auto result = std::to_chars(body, body + sizeof(body), value);
                n = usize(result.ptr - body);
            }
            else
            {
                char fmt[8] = {'%', '.', '*', spec.type ? spec.type : 'f', 0};
                int written = std::snprintf(body, sizeof(body), fmt, spec.precision < 0 ? 6 : spec.precision, value);
                n = written > 0 ? Min<usize>(usize(written), sizeof(body) - 1) : 0;
            }
            bool negative = n > 0 && body[0] == '-';
            Pad(w, spec, "-", negative ? 1 : 0, body + (negative ? 1 : 0), n - (negative ? 1 : 0), true);
        }

        void WriteArg(Writer& w, const Spec& spec, const FormatArg& arg)
        {
            switch (arg.type)
            {
            case FormatArg::Type::Bool:
                Pad(w, spec, "", 0, arg.b ? "true" : "false", arg.b ? 4 : 5, false);
                break;
            case FormatArg::Type::Char:
                Pad(w, spec, "", 0, &arg.c, 1, false);
                break;
            case FormatArg::Type::Int:
                WriteInteger(w, spec, arg.i < 0 ? u64(0) - u64(arg.i) : u64(arg.i), arg.i < 0);
                break;
            case FormatArg::Type::UInt:
                WriteInteger(w, spec, arg.u, false);
                break;
            case FormatArg::Type::Double:
                WriteDouble(w, spec, arg.d);
                break;
            case FormatArg::Type::Text:
            {
                usize size = spec.precision >= 0 ? Min(arg.text.size, usize(spec.precision)) : arg.text.size;
                Pad(w, spec, "", 0, arg.text.data, size, false);
                break;
            }
            case FormatArg::Type::Pointer:
            {
                char body[2 + 16] = {'0', 'x'};
                usize n = FormatUnsigned(body + 2, u64(reinterpret_cast<uptr>(arg.p)), 16, false);
                Pad(w, spec, "", 0, body, n + 2, false);
                break;
            }
            case FormatArg::Type::None:
                break;
            }
        }

        void Run(Writer& w, StringView format, Span<const FormatArg> args)
        {
            usize next = 0;
            usize i = 0;
            while (i < format.Size())
            {
                char c = format[i];
                if (c == '{' && i + 1 < format.Size() && format[i + 1] == '{')
                {
                    w.Write("{", 1);
                    i += 2;
                    continue;
                }
                if (c == '}' && i + 1 < format.Size() && format[i + 1] == '}')
                {
                    w.Write("}", 1);
                    i += 2;
                    continue;
                }
                if (c != '{')
                {
                    usize start = i;
                    while (i < format.Size() && format[i] != '{' && format[i] != '}')
                        i++;
                    if (i == start)
                        i++;
                    w.Write(format.Data() + start, i - start);
                    continue;
                }
                usize close = format.Find('}', i);
                if (close == StringView::npos)
                {
                    HX_ASSERT_MSG(false, "unterminated format placeholder");
                    w.Write(format.Data() + i, format.Size() - i);
                    return;
                }
                StringView inner = format.Substr(i + 1, close - i - 1);
                Spec spec;
                bool ok = inner.Empty() || (inner[0] == ':' && ParseSpec(inner.Substr(1), spec));
                HX_ASSERT_MSG(ok, "invalid format spec");
                HX_ASSERT_MSG(next < args.Size(), "too few format arguments");
                if (ok && next < args.Size())
                    WriteArg(w, spec, args[next]);
                next++;
                i = close + 1;
            }
            HX_ASSERT_MSG(next == args.Size(), "too many format arguments");
        }
    }

    void FormatArgs(String& out, StringView format, Span<const FormatArg> args)
    {
        StringWriter w(out);
        Run(w, format, args);
    }

    usize FormatArgsToBuffer(char* buffer, usize size, StringView format, Span<const FormatArg> args)
    {
        BufferWriter w(buffer, size ? size - 1 : 0);
        Run(w, format, args);
        if (size)
            buffer[Min(w.Written(), size - 1)] = '\0';
        return w.Written();
    }
}
