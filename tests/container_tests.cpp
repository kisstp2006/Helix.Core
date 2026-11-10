#include <helix/core/containers/array.h>
#include <helix/core/containers/hash_map.h>
#include <helix/core/strings/string.h>
#include <helix/testing/test.h>

using namespace helix;

namespace
{
    struct Tracked
    {
        static inline int alive = 0;
        int value;
        Tracked(int v = 0) : value(v) { alive++; }
        Tracked(const Tracked& o) : value(o.value) { alive++; }
        Tracked(Tracked&& o) noexcept : value(o.value) { alive++; }
        Tracked& operator=(const Tracked&) = default;
        Tracked& operator=(Tracked&&) = default;
        ~Tracked() { alive--; }
        bool operator==(const Tracked& o) const { return value == o.value; }
    };
}

HX_TEST(Array, PushPopGrow)
{
    Array<int> a;
    for (int i = 0; i < 1000; i++)
        a.Push(i);
    HX_CHECK_EQ(a.Size(), 1000u);
    HX_CHECK(a.Capacity() >= 1000);
    int sum = 0;
    for (int v : a)
        sum += v;
    HX_CHECK_EQ(sum, 999 * 1000 / 2);
    a.Pop();
    HX_CHECK_EQ(a.Back(), 998);
}

HX_TEST(Array, InsertRemove)
{
    Array<int> a{1, 2, 4, 5};
    a.Insert(2, 3);
    HX_CHECK_EQ(a.Size(), 5u);
    for (int i = 0; i < 5; i++)
        HX_CHECK_EQ(a[usize(i)], i + 1);
    a.RemoveAt(0);
    HX_CHECK_EQ(a[0], 2);
    a.RemoveAtSwap(0);
    HX_CHECK_EQ(a[0], 5);
    HX_CHECK_EQ(a.Size(), 3u);
    HX_CHECK(a.Contains(4));
    HX_CHECK(!a.Contains(2));
}

HX_TEST(Array, LifetimesOfNonTrivialTypes)
{
    {
        Array<Tracked> a;
        for (int i = 0; i < 100; i++)
            a.Emplace(i);
        Array<Tracked> b = a;
        Array<Tracked> c = Move(a);
        HX_CHECK_EQ(Tracked::alive, 200);
        HX_CHECK(a.Empty());
        c.Resize(10);
        HX_CHECK_EQ(Tracked::alive, 110);
        b = c;
        HX_CHECK_EQ(Tracked::alive, 20);
        c.Push(c[0]);
        HX_CHECK_EQ(c.Back().value, 0);
    }
    HX_CHECK_EQ(Tracked::alive, 0);
}

HX_TEST(HashMap, InsertFindRemove)
{
    HashMap<u64, int> map;
    for (u64 i = 0; i < 10000; i++)
        map.Set(i * 7919, int(i));
    HX_CHECK_EQ(map.Size(), 10000u);
    for (u64 i = 0; i < 10000; i++)
    {
        int* v = map.Find(i * 7919);
        HX_REQUIRE(v != nullptr);
        HX_CHECK_EQ(*v, int(i));
    }
    HX_CHECK(map.Find(1) == nullptr);
    for (u64 i = 0; i < 10000; i += 2)
        HX_CHECK(map.Remove(i * 7919));
    HX_CHECK_EQ(map.Size(), 5000u);
    HX_CHECK(!map.Remove(0));
    HX_CHECK(map.Contains(7919));
    HX_CHECK(!map.Contains(2 * 7919));
}

HX_TEST(HashMap, OverwriteAndTryAdd)
{
    HashMap<int, int> map;
    map.Set(1, 10);
    map.Set(1, 11);
    HX_CHECK_EQ(map.Size(), 1u);
    HX_CHECK_EQ(*map.Find(1), 11);
    HX_CHECK(!map.TryAdd(1, 12));
    HX_CHECK(map.TryAdd(2, 20));
    map[3] += 5;
    HX_CHECK_EQ(map[3], 5);
}

HX_TEST(HashMap, TombstoneChurnKeepsWorking)
{
    HashMap<int, int> map;
    for (int round = 0; round < 50; round++)
    {
        for (int i = 0; i < 100; i++)
            map.Set(round * 1000 + i, i);
        for (int i = 0; i < 100; i++)
            HX_CHECK(map.Remove(round * 1000 + i));
    }
    HX_CHECK(map.Empty());
    HX_CHECK(map.Capacity() <= 512);
}

HX_TEST(HashMap, StringKeysAndIteration)
{
    HashMap<String, Tracked> map;
    map.Set(String("alpha"), Tracked(1));
    map.Set(String("beta"), Tracked(2));
    map.Set(String("a much longer key that does not fit inline"), Tracked(3));
    int sum = 0;
    usize count = 0;
    for (auto [key, value] : map)
    {
        sum += value.value;
        count += key.Size() > 0 ? 1 : 0;
    }
    HX_CHECK_EQ(sum, 6);
    HX_CHECK_EQ(count, 3u);
    HashMap<String, Tracked> copy = map;
    HX_CHECK_EQ(copy.Find(String("beta"))->value, 2);
    map.Clear();
    HX_CHECK_EQ(Tracked::alive, 3);
}

HX_TEST(Array, ResizeGrowsGeometrically)
{
    Array<u8> a;
    usize reallocations = 0, last = 0;
    for (usize i = 1; i <= 100000; i++)
    {
        a.Resize(i);
        if (a.Capacity() != last)
        {
            reallocations++;
            last = a.Capacity();
        }
    }
    HX_CHECK(reallocations < 40);
    HX_CHECK_EQ(a[99999], 0);
}
