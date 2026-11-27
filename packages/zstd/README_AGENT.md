# zstd@1.0.0 (Agent Quickref)

> CP package zstd | License: BSD-3-Clause
> Import: `import { ... } from "zstd";` | Full types: `src/index.d.cp`

## Functions
- `export fn ZSTD_versionNumber(): u32;`
- `export fn ZSTD_versionString(): [*:0]&u8!;`
- `export fn ZSTD_compressBound(srcSize: usize): usize;`
- `export fn ZSTD_isError(code: usize): u32;`
- `export fn ZSTD_getErrorName(code: usize): [*:0]&u8!;`
- `export fn ZSTD_getFrameContentSize(src: [*]&u8!, srcSize: usize): u64;`
- `export fn ZSTD_createCCtx(): *ZSTD_CCtx!?;`
- `export fn ZSTD_freeCCtx(cctx: *ZSTD_CCtx!): usize;`
- `export fn ZSTD_compressCCtx(cctx: *ZSTD_CCtx!, dst: [*]u8!, dstCapacity: usize, src: [*]&u8!, srcSize: usize, compressionLevel: i32): usize;`
- `export fn ZSTD_createDCtx(): *ZSTD_DCtx!?;`
- `export fn ZSTD_freeDCtx(dctx: *ZSTD_DCtx!): usize;`
- `export fn ZSTD_decompressDCtx(dctx: *ZSTD_DCtx!, dst: [*]u8!, dstCapacity: usize, src: [*]&u8!, srcSize: usize): usize;`
- `export fn versionNumber(): u32;`
- `export fn compressBound(src_size: usize): usize;`
