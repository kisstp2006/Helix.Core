#include <helix/core/guid/guid.h>
#include <helix/core/hash/md5.h>
#include <helix/core/containers/hash_map.h>
#include <helix/core/log/log.h>
#include <helix/testing/test.h>

#include <cstring>

using namespace helix;

namespace
{
    void Pattern(u8* out, usize n)
    {
        for (usize i = 0; i < n; i++)
            out[i] = u8(i * 7 + 3);
    }

    String Hex(const Md5Digest& d)
    {
        String s;
        for (u8 b : d.bytes)
            FormatAppend(s, "{:02x}", b);
        return s;
    }
}

HX_TEST(Hash64, ReferenceVectors)
{
    u8 pattern[100];
    Pattern(pattern, sizeof(pattern));
    const char* fox = "The quick brown fox jumps over the lazy dog";
    HX_CHECK_EQ(Hash64("", 0), 0xef46db3751d8e999ull);
    HX_CHECK_EQ(Hash64("a", 1), 0xd24ec4f1a98c6e5bull);
    HX_CHECK_EQ(Hash64("abc", 3), 0x44bc2cf5ad770999ull);
    HX_CHECK_EQ(Hash64(fox, std::strlen(fox)), 0x0b242d361fda71bcull);
    HX_CHECK_EQ(Hash64(pattern, 100), 0xa61f8d4c170fe531ull);
    HX_CHECK_EQ(Hash64(pattern, 100, 0x9e3779b97f4a7c15ull), 0xf6d8f65c625abb4full);
    HX_CHECK_EQ(Hash64(pattern, 37), 0xe32ef63802f5a3fdull);
}

HX_TEST(Crc32, ReferenceVectors)
{
    u8 pattern[100];
    Pattern(pattern, sizeof(pattern));
    HX_CHECK_EQ(Crc32("123456789", 9), 0xcbf43926u);
    HX_CHECK_EQ(Crc32(pattern, 100), 0xaa316b09u);
    HX_CHECK_EQ(Crc32(pattern + 40, 60, Crc32(pattern, 40)), 0xaa316b09u);
}

HX_TEST(Md5, ReferenceVectors)
{
    u8 pattern[100];
    Pattern(pattern, sizeof(pattern));
    const char* fox = "The quick brown fox jumps over the lazy dog";
    HX_CHECK(Hex(ComputeMd5("", 0)) == "d41d8cd98f00b204e9800998ecf8427e");
    HX_CHECK(Hex(ComputeMd5("abc", 3)) == "900150983cd24fb0d6963f7d28e17f72");
    HX_CHECK(Hex(ComputeMd5(fox, std::strlen(fox))) == "9e107d9d372bb6826bd81d3542a419d6");
    HX_CHECK(Hex(ComputeMd5(pattern, 100)) == "f85d474760cb48f459327ba2209c185d");
    Md5 split;
    split.Update(pattern, 13);
    split.Update(pattern + 13, 64);
    split.Update(pattern + 77, 23);
    HX_CHECK(Hex(split.Finish()) == "f85d474760cb48f459327ba2209c185d");
}

HX_TEST(Fnv1a64, IsConstexpr)
{
    static_assert(Fnv1a64("", 0) == 0xcbf29ce484222325ull);
    static_assert(Fnv1a64("a", 1) == 0xaf63dc4c8601ec8cull);
    HX_CHECK(true);
}

HX_TEST(Guid, GenerateParseRoundTrip)
{
    Guid a = Guid::Generate();
    Guid b = Guid::Generate();
    HX_CHECK(a.IsValid());
    HX_CHECK(a != b);
    HX_CHECK_EQ(a.bytes[6] >> 4, 4);
    HX_CHECK_EQ(a.bytes[8] >> 6, 2);
    String text = a.ToString();
    HX_CHECK_EQ(text.Size(), 32u);
    Guid parsed;
    HX_CHECK(Guid::TryParse(text, parsed));
    HX_CHECK(parsed == a);
    HX_CHECK(Guid::TryParse("0123456789ABCDEFabcdef0123456789", parsed));
    HX_CHECK_EQ(parsed.bytes[0], 0x01);
    HX_CHECK_EQ(parsed.bytes[15], 0x89);
    HX_CHECK(!Guid::TryParse("0123", parsed));
    HX_CHECK(!Guid::TryParse("g123456789abcdefabcdef0123456789", parsed));
    HX_CHECK(!Guid().IsValid());
    HashMap<Guid, int> map;
    map.Set(a, 1);
    HX_CHECK(map.Contains(a) && !map.Contains(b));
}

HX_TEST(Log, SinksAndLevels)
{
    struct Capture
    {
        int count = 0;
        String last;
    } capture;
    auto sink = [](const LogMessage& m, void* user) {
        auto* c = static_cast<Capture*>(user);
        c->count++;
        c->last = m.text;
    };
    LogChannel channel{"Test", LogLevel::Info};
    SetConsoleLogging(false);
    HX_REQUIRE(AddLogSink(sink, &capture));
    HX_LOG_DEBUG(channel, "hidden {}", 1);
    HX_LOG_INFO(channel, "value {} and {}", 42, "text");
    String longText;
    longText.Append('x', 3000);
    HX_LOG_WARNING(channel, "{}", longText);
    RemoveLogSink(sink, &capture);
    HX_LOG_ERROR(channel, "after removal");
    SetConsoleLogging(true);
    HX_CHECK_EQ(capture.count, 2);
    HX_CHECK_EQ(capture.last.Size(), 3000u);
}

HX_TEST(Assert, HandlerCanContinue)
{
    static int reported = 0;
    AssertHandler previous = SetAssertHandler([](const char*, const char*, const char*, int) {
        reported++;
        return AssertAction::Continue;
    });
    volatile int two = 2;
    HX_VERIFY_MSG(two == 3, "expected failure");
    SetAssertHandler(previous);
    HX_CHECK_EQ(reported, 1);
}
