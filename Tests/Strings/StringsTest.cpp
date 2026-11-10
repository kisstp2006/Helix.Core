#include <Helix/Core/Strings/Format.h>
#include <Helix/Testing/Test.h>

using namespace helix;
using namespace helix::literals;

HX_TEST(StringView, Basics)
{
    StringView s = "  hello world  ";
    StringView t = s.Trimmed();
    HX_CHECK(t == "hello world");
    HX_CHECK_EQ(t.Find('o'), 4u);
    HX_CHECK_EQ(t.Find("world"), 6u);
    HX_CHECK_EQ(t.FindLast('o'), 7u);
    HX_CHECK_EQ(t.Find("xyz"), StringView::npos);
    HX_CHECK(t.StartsWith("hello"));
    HX_CHECK(t.EndsWith("world"));
    HX_CHECK(t.Substr(6) == "world");
    HX_CHECK(t.Substr(100).Empty());
    HX_CHECK("abc"_sv < "abd"_sv);
    HX_CHECK("ab"_sv < "abc"_sv);
    static_assert(StringView("constexpr").Size() == 9);
}

HX_TEST(String, InlineAndHeap)
{
    String s("12345678901234567890123");
    HX_CHECK(s.IsInline());
    s.Append('4');
    HX_CHECK(!s.IsInline());
    HX_CHECK_EQ(s.Size(), 24u);
    HX_CHECK_STR(s.CStr(), "123456789012345678901234");
    String moved = Move(s);
    HX_CHECK(s.Empty());
    HX_CHECK_EQ(moved.Size(), 24u);
    String small("abc");
    String movedSmall = Move(small);
    HX_CHECK(movedSmall == "abc");
    HX_CHECK(movedSmall.IsInline());
}

HX_TEST(String, AppendSelfAndResize)
{
    String s("abc");
    for (int i = 0; i < 5; i++)
        s.Append(s.View());
    HX_CHECK_EQ(s.Size(), 96u);
    HX_CHECK(s.View().Substr(93) == "abc");
    s.Resize(2);
    HX_CHECK(s == "ab");
    s.Resize(4, 'x');
    HX_CHECK(s == "abxx");
    s = s.View().Substr(1);
    HX_CHECK(s == "bxx");
    s = "literal";
    HX_CHECK(s == "literal");
}

HX_TEST(Format, Integers)
{
    HX_CHECK(Format("{} {} {}", 1, -2, 3u) == "1 -2 3");
    HX_CHECK(Format("{:x} {:X} {:b}", 255, 255u, 5) == "ff FF 101");
    HX_CHECK(Format("{:08x}", 0xbeefu) == "0000beef");
    HX_CHECK(Format("[{:5}] [{:<5}] [{:>5}]", 42, 42, "ab") == "[   42] [42   ] [   ab]");
    HX_CHECK(Format("{:05}", -42) == "-0042");
    HX_CHECK(Format("{}", i64(-9223372036854775807 - 1)) == "-9223372036854775808");
    HX_CHECK(Format("{}", ~u64(0)) == "18446744073709551615");
}

HX_TEST(Format, FloatsAndOthers)
{
    HX_CHECK(Format("{}", 0.1) == "0.1");
    HX_CHECK(Format("{}", 1.5f) == "1.5");
    HX_CHECK(Format("{:.3}", 3.14159) == "3.142");
    HX_CHECK(Format("{:.2e}", 12345.0) == "1.23e+04");
    HX_CHECK(Format("{} {} {}", true, 'c', "text") == "true c text");
    HX_CHECK(Format("{{}} {}", String("s")) == "{} s");
    HX_CHECK(Format("{:.2}", "abcdef") == "ab");
    HX_CHECK(Format("{}", nullptr) == "0x0");
}

HX_TEST(Format, ToBufferTruncates)
{
    char buffer[8];
    usize n = FormatToBuffer(buffer, sizeof(buffer), "value={}", 123456);
    HX_CHECK_EQ(n, 12u);
    HX_CHECK_STR(buffer, "value=1");
}
