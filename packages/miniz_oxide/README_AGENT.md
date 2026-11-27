# miniz_oxide — AI Agent Quick Ref

- **Import**: `import { MinizBuf, MinizError, compressToVecZlib, decompressToVecZlib, ... } from "miniz_oxide";`
- **Origin**: crates.io `miniz_oxide` (v3.0.2, MIT, native C `miniz 3.0.2` + RAII)
- **Full Types**: `src/index.d.cp`

## Errors & RAII Buffer
```cp
error MinizError {
    ZipCreateFailed, ZipExtractFailed, OutOfMemory,
    CompressFailed, DecompressFailed, BufferTooSmall,
    InvalidLevel, OutputLimitExceeded
}
class MinizBuf {
    raw: [*]u8!?, len: usize,
    fn zipSingleFile(entry_name: string, payload: &[]u8): !MinizBuf;
    fn extractFile(zip_bytes: &[]u8, entry_name: string): !MinizBuf;
    fn slice(): &[]u8;
    fn __cp_deinit(_: &mut MinizBuf);
}
```

## High-Level Functions
- **Heap-Backed (`!MinizBuf`, auto-freed)**:
  - `compressToVec(input: &[]u8, level: u8): !MinizBuf` (alias: `compress_to_vec`)
  - `compressToVecZlib(input: &[]u8, level: u8): !MinizBuf` (alias: `compress_to_vec_zlib`)
  - `decompressToVec(input: &[]u8): !MinizBuf` (alias: `decompress_to_vec`)
  - `decompressToVecZlib(input: &[]u8): !MinizBuf` (alias: `decompress_to_vec_zlib`)
  - `decompressToVecWithLimit(input: &[]u8, max_size: usize): !MinizBuf`
  - `decompressToVecZlibWithLimit(input: &[]u8, max_size: usize): !MinizBuf`
- **Zero-Heap Slice-to-Slice (`!usize` bytes written)**:
  - `compressToSlice(input: &[]u8, out: []u8, level: u8): !usize`
  - `compressToSliceZlib(input: &[]u8, out: []u8, level: u8): !usize`
  - `decompressToSlice(input: &[]u8, out: []u8): !usize`
  - `decompressToSliceZlib(input: &[]u8, out: []u8): !usize`
- **Utilities & Checksums**:
  - `versionString(): string` | `compressBound(source_len: usize): usize`
  - `makeDeflateFlags(level: u8, write_zlib_header: bool): !i32`
  - `crc32(data: &[]u8): u32` | `crc32Update(prev: u32, data: &[]u8): u32`
  - `adler32(data: &[]u8): u32` | `adler32Update(prev: u32, data: &[]u8): u32`

## Gotchas
1. Valid compression `level` is `0..10`; higher values return `MinizError.InvalidLevel`.
2. Never call `cp_miniz_free` manually on a `MinizBuf` — `__cp_deinit` frees it automatically.
3. For untrusted compressed input, always prefer `decompressToVecWithLimit` / `decompressToVecZlibWithLimit` (returns `MinizError.OutputLimitExceeded` if exceeded) or `decompressToSlice*`.
4. `compressToVec` / `decompressToVec` use raw DEFLATE; `compressToVecZlib` / `decompressToVecZlib` include the zlib header/trailer — always match pairs.
