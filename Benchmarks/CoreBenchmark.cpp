#include <Helix/Core/Containers/Array.h>
#include <Helix/Core/Containers/HashMap.h>
#include <Helix/Core/Strings/Format.h>
#include <Helix/Testing/Bench.h>

using namespace helix;
using helix::testing::DoNotOptimize;

HX_BENCH(HashMapInsert10k)
{
    state.SetItemsPerIteration(10000);
    while (state.KeepRunning())
    {
        HashMap<u64, u64> map;
        map.Reserve(10000);
        for (u64 i = 0; i < 10000; i++)
            map.Set(i * 0x9e3779b97f4a7c15ull, i);
        DoNotOptimize(map.Size());
    }
}

HX_BENCH(HashMapFindHit10k)
{
    HashMap<u64, u64> map;
    for (u64 i = 0; i < 10000; i++)
        map.Set(i * 0x9e3779b97f4a7c15ull, i);
    state.SetItemsPerIteration(10000);
    while (state.KeepRunning())
    {
        u64 sum = 0;
        for (u64 i = 0; i < 10000; i++)
            sum += *map.Find(i * 0x9e3779b97f4a7c15ull);
        DoNotOptimize(sum);
    }
}

HX_BENCH(ArrayPush10k)
{
    state.SetItemsPerIteration(10000);
    while (state.KeepRunning())
    {
        Array<u32> a;
        for (u32 i = 0; i < 10000; i++)
            a.Push(i);
        DoNotOptimize(a.Data());
    }
}

HX_BENCH(Hash64_4KB)
{
    static u8 data[4096];
    for (usize i = 0; i < sizeof(data); i++)
        data[i] = u8(i);
    state.SetItemsPerIteration(4096);
    while (state.KeepRunning())
        DoNotOptimize(Hash64(data, sizeof(data)));
}

HX_BENCH(FormatToBuffer)
{
    char buffer[128];
    while (state.KeepRunning())
        DoNotOptimize(FormatToBuffer(buffer, sizeof(buffer), "{} {:08x} {} {}", 12345, 0xbeefu, "text", 2.5));
}
