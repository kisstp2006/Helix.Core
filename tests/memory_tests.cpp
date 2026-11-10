#include <helix/core/memory/linear_allocator.h>
#include <helix/core/memory/pool_allocator.h>
#include <helix/testing/test.h>

using namespace helix;

HX_TEST(HeapAllocator, AlignmentAndStats)
{
    HeapStats before = GetHeapStats();
    void* a = HeapAllocator().Allocate(100, 16);
    void* b = HeapAllocator().Allocate(10, 256);
    HX_CHECK_EQ(reinterpret_cast<uptr>(a) % 16, 0u);
    HX_CHECK_EQ(reinterpret_cast<uptr>(b) % 256, 0u);
    HX_CHECK_EQ(GetHeapStats().liveAllocations, before.liveAllocations + 2);
    HeapAllocator().Free(a, 100, 16);
    HeapAllocator().Free(b, 10, 256);
    HX_CHECK_EQ(GetHeapStats().liveAllocations, before.liveAllocations);
    HX_CHECK_EQ(GetHeapStats().liveBytes, before.liveBytes);
}

HX_TEST(HeapAllocator, NewDelete)
{
    struct Counter
    {
        int* count;
        explicit Counter(int* c) : count(c) { ++*count; }
        ~Counter() { --*count; }
    };
    int count = 0;
    Counter* c = New<Counter>(HeapAllocator(), &count);
    HX_CHECK_EQ(count, 1);
    Delete(HeapAllocator(), c);
    HX_CHECK_EQ(count, 0);
}

HX_TEST(LinearAllocator, BumpResetRewind)
{
    LinearAllocator linear(1024);
    void* a = linear.Allocate(10, 1);
    void* b = linear.Allocate(8, 64);
    HX_CHECK(a != nullptr && b != nullptr);
    HX_CHECK_EQ(reinterpret_cast<uptr>(b) % 64, 0u);
    usize mark = linear.Mark();
    linear.Allocate(100, 4);
    HX_CHECK(linear.Used() > mark);
    linear.Rewind(mark);
    HX_CHECK_EQ(linear.Used(), mark);
    linear.Reset();
    HX_CHECK_EQ(linear.Used(), 0u);
    HX_CHECK_EQ(linear.Allocate(10, 1), a);
}

HX_TEST(PoolAllocator, ReusesFreedElements)
{
    PoolAllocator pool(24, 8, 4);
    void* items[10];
    for (void*& p : items)
        p = pool.Allocate(24, 8);
    HX_CHECK_EQ(pool.LiveCount(), 10u);
    HX_CHECK_EQ(pool.BlockCount(), 3u);
    for (void* p : items)
        HX_CHECK_EQ(reinterpret_cast<uptr>(p) % 8, 0u);
    pool.Free(items[3], 24, 8);
    HX_CHECK_EQ(pool.Allocate(24, 8), items[3]);
    for (void* p : items)
        pool.Free(p, 24, 8);
    HX_CHECK_EQ(pool.LiveCount(), 0u);
}
