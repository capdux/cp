# zstd — AI Agent Quick Ref

- **Package**: `zstd` (`1.0.0`) | **Source**: crates.io `zstd` (native C `libzstd` FFI + RAII wrapper)
- **Import**: `import { Compressor, Decompressor, compressBound, versionNumber, ZstdError } from "zstd";`
- **Lifecycle**: `Compressor` & `Decompressor` auto-free native contexts via `__cp_deinit`.

## Core Signatures

```typescript
export error ZstdError {
    AllocContextFailed, InvalidContext, CompressFailed, DecompressFailed, UnknownContentSize, OutOfMemory
}

export fn versionNumber(): u32;
export fn compressBound(src_size: usize): usize;

export class Compressor {
    fn init(_: auto, level: i32): !void;
    fn compressInto(src: &[]u8, dst: []u8): !usize;
    fn compressAlloc(allocator: std.mem.Allocator, src: &[]u8): ![]u8;
}

export class Decompressor {
    fn init(_: auto): !void;
    fn decompressInto(src: &[]u8, dst: []u8): !usize;
    fn decompressAlloc(allocator: std.mem.Allocator, src: &[]u8): ![]u8;
}
```

## Constants & Raw FFI

- `ZSTD_CONTENTSIZE_UNKNOWN: u64`, `ZSTD_CONTENTSIZE_ERROR: u64`
- Raw C bindings also exported: `ZSTD_versionNumber`, `ZSTD_versionString`, `ZSTD_compressBound`, `ZSTD_isError`, `ZSTD_getErrorName`, `ZSTD_getFrameContentSize`, `ZSTD_createCCtx`, `ZSTD_freeCCtx`, `ZSTD_compressCCtx`, `ZSTD_createDCtx`, `ZSTD_freeDCtx`, `ZSTD_decompressDCtx`.

## Minimal Pattern

```typescript
import { Compressor, Decompressor } from "zstd";

let c: Compressor = undefined;
try c.init(3);
let cbuf: [1024]u8 = undefined;
const clen = try c.compressInto(src, cbuf[0..]);

let d: Decompressor = undefined;
try d.init();
let dbuf: [1024]u8 = undefined;
const dlen = try d.decompressInto(cbuf[0..clen], dbuf[0..]);
```

## Pitfalls

1. **Must call `init()` before use**: Uninitialized `Compressor`/`Decompressor` fails with `ZstdError.InvalidContext`.
2. **Buffer sizing**: Ensure `dst.len >= compressBound(src.len)` for `compressInto`, or `CompressFailed` is returned.
3. **Unknown frame size**: `decompressAlloc` requires the Zstd frame header to contain content size; streaming frames without content size return `ZstdError.UnknownContentSize` (use `decompressInto` with a known-capacity buffer instead).
4. **Free `*Alloc` slices**: Caller owns slices returned by `compressAlloc` / `decompressAlloc` and must call `allocator.free(...)`.
