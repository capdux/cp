[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# snap

> **Source Attribution**: Clean-room MIT CP implementation of [`crates.io:snap`](https://github.com/BurntSushi/rust-snappy) (BSD-3-Clause) | CP Package Version: `1.0.0` | Zero external dependencies

Zero-allocation Google Snappy Raw Block lossless compressor and decompressor for CP. Implements the full Snappy raw block specification: varint32 uncompressed length header, Literal / Copy-1 / Copy-2 / Copy-4 tags, and overlapping RLE back-reference decompression.

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`"snap"`) — no renaming required.

| Rust (`crates.io:snap::raw`) | CP (`snap`) |
| :--- | :--- |
| `use snap::raw::{Encoder, Decoder, max_compress_len, decompress_len};` | `import { compress, decompress, maxCompressedLen, uncompressedLen } from "snap";` |
| `max_compress_len(src.len())` | `maxCompressedLen(src.len)` |
| `Encoder::new().compress(src, dst)` | `compress(src, dst)` |
| `decompress_len(compressed)` | `uncompressedLen(compressed)` |
| `Decoder::new().decompress(compressed, out)` | `decompress(compressed, out)` |

## API Reference

| Signature | Description |
| :--- | :--- |
| `maxCompressedLen(input_len: usize): usize` | Worst-case compressed byte size (`32 + len + len / 6`). |
| `uncompressedLen(src: &[]u8): i64` | Read varint32 uncompressed length header (`-1` if malformed). |
| `compress(src: &[]u8, dst: []u8): i64` | Compress `src` into `dst`; returns compressed byte count or `-1` on error/short buffer. |
| `decompress(src: &[]u8, dst: []u8): i64` | Decompress Snappy raw block `src` into `dst`; returns decompressed byte count or `-1` on corrupt/short buffer. |

## Usage Examples

### Basic Compress & Decompress Round-Trip

```cp
import { compress, decompress, maxCompressedLen, uncompressedLen } from "snap";

fn demoRoundTrip() {
    const input = "hello snappy hello snappy hello snappy hello snappy!";
    let compressed: [128]u8 = undefined;

    const c_len = compress(input, compressed[0..]);
    if (c_len > 0) {
        const expected = uncompressedLen(compressed[0..(c_len as usize)]);
        let restored: [128]u8 = undefined;
        const d_len = decompress(compressed[0..(c_len as usize)], restored[0..]);
        // expected == input.len as i64 && d_len == expected
        * = expected; * = d_len;
    }
    * = maxCompressedLen(input.len);
}
```

### Handling Corrupt Inputs & Short Buffers

```cp
import { compress, decompress } from "snap";

fn demoErrorHandling() {
    const payload = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"; // RLE-friendly
    let tiny: [4]u8 = undefined;
    // Destination buffer too small -> returns -1 safely
    const rc = compress(payload, tiny[0..]);
    // rc == -1

    const bad_block = [0xFF as u8, 0xFF as u8, 0xFF as u8, 0xFF as u8, 0xFF as u8, 0xFF as u8];
    let out: [64]u8 = undefined;
    // Malformed varint / truncated block -> returns -1 without panic
    const rd = decompress(bad_block[0..], out[0..]);
    // rd == -1
    * = rc; * = rd;
}
```

## Performance & Safety Highlights

- **Zero Heap Allocation**: 4096-entry 16-bit hash table (`let table: [4096]u16 = undefined`) lives on the stack with zero dynamic allocation and zero startup zeroing.
- **Fast `@memcpy` Bulk Copy**: Literal runs and non-overlapping Copy-1/Copy-2/Copy-4 back-references use `@memcpy` directly; overlapping RLE copies (`offset < copy_len`) fall back cleanly to byte-wise RLE expansion.
- **Strict Bounds & Corruption Defense**: Validates varint32 length, offset non-zero/in-bounds, and output capacity on every tag; returns `-1` on any malformed input.
- **Wire-Format Compatible**: 100% interoperable with Google Snappy and Rust `snap::raw`.

## License

MIT
