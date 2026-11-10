# Helix.Core

OS independent foundation library of the Helix engine: base types, asserts, allocators,
containers, strings, formatting, hashing, GUIDs and logging. The only OS calls are the random
source for GUIDs and the console log sink.

## Layout

| Path | Content |
|---|---|
| `Source/Helix/Core/Basics/` | `Types.h`, `Platform.h` (platform and compiler macros), `Assert.h`, `Utility.h`, `Span.h` |
| `Source/Helix/Core/Memory/` | `Allocator` interface, heap allocator with stats, `LinearAllocator`, `PoolAllocator` |
| `Source/Helix/Core/Containers/` | `Array<T>`, `HashMap<K, V>` (open addressing with control bytes) |
| `Source/Helix/Core/Strings/` | `StringView`, `String` (23 byte inline storage), `Format` |
| `Source/Helix/Core/Hashing/` | `Hash64` (XXH64 algorithm), `Crc32`, `Fnv1a64`, `Md5` |
| `Source/Helix/Core/Types/` | `Guid` (128 bit, 32 hex character text form) |
| `Source/Helix/Core/Logging/` | Log channels, levels, sinks, `HX_LOG_*` macros |
| `.../<Topic>/Implementation/` | Sources, `.../<Topic>/Platform/<Win\|Posix>/` for OS specific files |
| `Tests/<Topic>/`, `Benchmarks/` | Unit tests and benchmarks |

## Rules

- No exceptions, no RTTI.
- Every container takes an `Allocator&`, the heap allocator is the default.
- `HX_ASSERT` is compiled out in release builds, `HX_VERIFY` is always checked.

## Format

```cpp
helix::String s = helix::Format("{} {:08x} {:.3} {:>6}", 42, 0xbeefu, 3.14159, "right");
```

Spec: `{}` or `{:[<|>][0][width][.precision][x|X|b|e|f|g|p]}`, `{{` and `}}` are escapes.

## Build

```
git clone --recursive https://github.com/kisstp2006/Helix.Core
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
```

Other presets: `linux-*`, `android-arm64-*`, `web-*`, see Helix.CMake.

## License

GPL-3.0, see `LICENSE`.
