# Helix.Core

OS independent foundation library of the Helix engine: base types, asserts, allocators,
containers, strings, formatting, hashing, GUIDs and logging. The only OS calls are the random
source for GUIDs and the console log sink.

## Layout

| Path | Content |
|---|---|
| `include/helix/core/base/` | `types.h`, `platform.h` (platform and compiler macros), `assert.h`, `utility.h`, `span.h` |
| `include/helix/core/memory/` | `Allocator` interface, heap allocator with stats, `LinearAllocator`, `PoolAllocator` |
| `include/helix/core/containers/` | `Array<T>`, `HashMap<K, V>` (open addressing with control bytes) |
| `include/helix/core/strings/` | `StringView`, `String` (23 byte inline storage), `Format` |
| `include/helix/core/hash/` | `Hash64` (XXH64 algorithm), `Crc32`, `Fnv1a64`, `Md5` |
| `include/helix/core/guid/` | `Guid` (128 bit, 32 hex character text form) |
| `include/helix/core/log/` | Log channels, levels, sinks, `HX_LOG_*` macros |
| `src/` | Implementation, `src/platform/<platform>/` for OS specific files |
| `tests/`, `bench/` | Unit tests and benchmarks |

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
