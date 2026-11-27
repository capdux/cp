# miniz_oxide@3.0.2 (Agent Quickref)

> Single-file lossless compression & ZIP archive creation/extraction engine (miniz 3.0.2) for CP | License: MIT
> Import: `import { ... } from "miniz_oxide";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_miniz_version(): [*:0]&u8!;`
- `export fn cp_miniz_zip_create_single(entry_name: [*:0]&u8!, data: [*]&u8!, data_len: usize, out_zip_len: *usize!): [*]u8!?;`
- `export fn cp_miniz_zip_extract_entry(zip_bytes: [*]&u8!, zip_len: usize, entry_name: [*:0]&u8!, out_len: *usize!): [*]u8!?;`
- `export fn cp_miniz_free(ptr: *auto!?);`
- `export fn tdefl_create_comp_flags_from_zip_params(level: i32, window_bits: i32, strategy: i32): u32;`
- `export fn tdefl_compress_mem_to_heap(pSrc_buf: [*]&u8!, src_buf_len: usize, pOut_len: *usize!, flags: i32): [*]u8!?;`
- `export fn tdefl_compress_mem_to_mem(pOut_buf: [*]u8!, out_buf_len: usize, pSrc_buf: [*]&u8!, src_buf_len: usize, flags: i32): usize;`
- `export fn tinfl_decompress_mem_to_heap(pSrc_buf: [*]&u8!, src_buf_len: usize, pOut_len: *usize!, flags: i32): [*]u8!?;`
- `export fn tinfl_decompress_mem_to_mem(pOut_buf: [*]u8!, out_buf_len: usize, pSrc_buf: [*]&u8!, src_buf_len: usize, flags: i32): usize;`
- `export fn mz_compressBound(source_len: usize): usize;`
- `export fn mz_crc32(crc: usize, ptr: [*]&u8!, buf_len: usize): usize;`
- `export fn mz_adler32(adler: usize, ptr: [*]&u8!, buf_len: usize): usize;`
- `export fn versionString(): string;`
- `export fn compressBound(source_len: usize): usize;`
- `export fn crc32(data: &[]u8): u32;`
- `export fn crc32Update(prev: u32, data: &[]u8): u32;`
- `export fn adler32(data: &[]u8): u32;`
- `export fn adler32Update(prev: u32, data: &[]u8): u32;`
- `export fn makeDeflateFlags(level: u8, write_zlib_header: bool): !i32;`
- `export fn compressToVec(input: &[]u8, level: u8): !MinizBuf;`
- `export fn compressToVecZlib(input: &[]u8, level: u8): !MinizBuf;`
- `export fn decompressToVec(input: &[]u8): !MinizBuf;`
- `export fn decompressToVecZlib(input: &[]u8): !MinizBuf;`
- `export fn decompressToVecWithLimit(input: &[]u8, max_size: usize): !MinizBuf;`
- *(+9 more in `src/index.d.cp`)*
