# `cap:lzw` — Pure CP Zero-Allocation LZW Codec (GIF LSB & TIFF/PDF MSB)

Pure CP, zero-heap-allocation Lempel-Ziv-Welch (LZW) compressor and decompressor backed by a fixed 4096-entry stack dictionary. Supports both GIF89a (LSB-first, late code-width increase) and TIFF 6.0 / PDF (MSB-first, early code-width increase) bitstreams with automatic Clear/EOI codes and 9..12-bit code widening.

## Protocol Constants & Types

- `LZW_MAX_BITS = 12`, `LZW_TABLE_SIZE = 4096`, `LZW_MIN_CODE_BITS = 2`, `LZW_MAX_INITIAL_BITS = 8`
- `LzwBitOrder: u8` — `lsb = 0` (GIF style), `msb = 1` (TIFF / PDF style)

## Core Functions

| Symbol | Description |
| :--- | :--- |
| `lzwCompress(src, dst: *[]u8, lit_width, order): usize?` | Compresses `src` into `dst.*` (`2 <= lit_width <= 8`), auto-emitting Clear on start/full table and EOI at end. |
| `lzwDecompress(src, dst: *[]u8, lit_width, order): usize?` | Decompresses LZW stream with `KwKwK` (`code == next_code`) support, Clear resets, EOI termination, and bounds/invalid-code protection. |
| `lzwCompressGif(src, dst, lit_width)` / `lzwDecompressGif(src, dst, lit_width)` | Convenience LSB-first GIF wrappers. |
| `lzwCompressTiff(src, dst)` / `lzwDecompressTiff(src, dst)` | Convenience MSB-first 8-bit TIFF/PDF wrappers. |

## Example

```cp
import { lzwCompressTiff, lzwDecompressTiff } from "cap:lzw";

fn roundTrip(input: []const u8, comp_buf: *[]u8, out_buf: *[]u8) {
    if (c_len := lzwCompressTiff(input, comp_buf)) {
        * = lzwDecompressTiff(comp_buf.*[0..c_len], out_buf);
    }
}
```
