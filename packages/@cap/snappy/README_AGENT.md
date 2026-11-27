# @cap/snappy — AI Agent Quick Ref

Google Snappy 1.2.1 fast compression/decompression library (Kafka/Parquet/LevelDB/RocksDB) for CP (`v1.2.1`, `BSD-3-Clause`)

## Import

```cp
import {
    SnappyError,
    snappy_compress,
    snappy_uncompress,
    snappy_max_compressed_length,
    snappy_uncompressed_length,
    snappy_validate_compressed_buffer,
    maxCompressedLength,
    uncompressedLength,
} from "@cap/snappy";
```

## Core Types & Constants

```cp
error SnappyError { InvalidInput, BufferTooSmall }
```

## API Signatures

```cp
fn snappy_compress(input: [*]&u8!, input_length: usize, compressed: [*]u8!, compressed_length: *usize!): i32;
fn snappy_uncompress(compressed: [*]&u8!, compressed_length: usize, uncompressed: [*]u8!, uncompressed_length: *usize!): i32;
fn snappy_max_compressed_length(source_length: usize): usize;
fn snappy_uncompressed_length(compressed: [*]&u8!, compressed_length: usize, result: *usize!): i32;
fn snappy_validate_compressed_buffer(compressed: [*]&u8!, compressed_length: usize): i32;
fn maxCompressedLength(source_len: usize): usize;
fn uncompressedLength(compressed: &[]u8): !usize;
fn compressInto(src: &[]u8, dst: []u8): !&[]u8;
fn uncompressInto(compressed: &[]u8, dst: []u8): !&[]u8;
fn validateCompressedBuffer(compressed: &[]u8): bool;
```

## Minimal Usage

```cp
import { validateCompressedBuffer } from "@cap/snappy";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = validateCompressedBuffer(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/snappy"`; install via `cap add @cap/snappy`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
