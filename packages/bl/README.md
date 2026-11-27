[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bl

Zero-heap segmented `BufferList` for streaming byte reads, cross-chunk big/little-endian integer decoding, `indexOf` search, `copyRange`, and `consume`.

Aligned with npm [`bl`](https://github.com/rvagg/bl) `6.1.0` (MIT).

## Installation & Import

```bash
cap add bl
```

```cp
import {
    BL_MAX_CHUNKS,
    BufferList,
    newBufferList,
} from "bl";
```

## Core API

### Constants & Constructors

| Symbol | Signature / Value | Description |
| --- | --- | --- |
| `BL_MAX_CHUNKS` | `usize = 64` | Maximum number of borrowed byte chunks held concurrently in a `BufferList`. |
| `newBufferList` | `(): BufferList` | Creates an empty `BufferList` on the stack. |

### `BufferList` Class

| Member | Signature | Description |
| --- | --- | --- |
| Fields | `chunks: [64]&[]u8`, `chunk_count: usize`, `total_len: usize` | Fixed-capacity array of borrowed chunk views, active count, and total byte length. |
| `len` | `(): usize` | Returns total readable bytes across all appended chunks. |
| `isEmpty` | `(): bool` | Returns `true` when `total_len == 0`. |
| `append` | `(*self, chunk: &[]u8): bool` | Appends a non-empty borrowed chunk without copying; returns `false` if `chunk_count == 64`. |
| `get` | `(index: usize): u8?` | Random-access byte read across chunks; returns `null` if `index >= total_len`. |
| `copyRange` | `(start: usize, end: usize, dst: []u8): usize` | Copies logical byte range `[start, end)` across chunk boundaries into `dst`; returns bytes copied. |
| `consume` | `(*self, n: usize)` | Advances and drops the first `n` bytes in-place (slicing partial head chunks as needed). |
| `readU16BE` / `readU16LE` | `(offset: usize): u16?` | Reads a big- or little-endian `u16` seamlessly across chunk boundaries; returns `null` if out of bounds. |
| `readU32BE` / `readU32LE` | `(offset: usize): u32?` | Reads a big- or little-endian `u32` seamlessly across chunk boundaries; returns `null` if out of bounds. |
| `indexOf` | `(needle: &[]u8, from_offset: usize): usize?` | Finds the first occurrence of `needle` at or after `from_offset` across chunk boundaries; returns `null` if not found. |

## Examples

### 1. Cross-Chunk Binary Frame Decoding

```cp
import { print } from "cap:io";
import { newBufferList } from "bl";

pub fn main() {
    let bl = newBufferList();
    let c1: [2]u8 = [0x01, 0x02];
    let c2: [4]u8 = [0x03, 0x04, 0xAA, 0xBB];
    let _ = bl.append(c1[0..2]);
    let _ = bl.append(c2[0..4]);

    let magic = bl.readU32BE(0);
    if (magic != null) {
        print("magic=0x{:x}, len={}\n", magic!, bl.len()); // 0x01020304, len=6
    }
    bl.consume(4);
    print("remaining={}\n", bl.len()); // 2
}
```

### 2. Streaming Delimiter Search (`indexOf`) & `copyRange`

```cp
import { print } from "cap:io";
import { newBufferList } from "bl";

pub fn main() {
    let bl = newBufferList();
    let _ = bl.append("GET /index");
    let _ = bl.append(".html HTTP/1.1\r\n");

    let pos = bl.indexOf("\r\n", 0);
    if (pos != null) {
        let line: [64]u8 = undefined;
        let n = bl.copyRange(0, pos!, line[0..64]);
        print("request line: {}\n", line[0..n]);
        bl.consume(pos! + 2);
    }
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Zero heap allocation**: `BufferList` stores up to `BL_MAX_CHUNKS = 64` borrowed `&[]u8` chunk slices directly on the stack—no `Buffer.concat` copies and no linked-list node allocations.
- **Seamless cross-boundary reads**: `readU16BE/LE`, `readU32BE/LE`, `copyRange`, and `indexOf` transparently bridge arbitrary chunk splits.
- **Optional bounds safety**: Out-of-range reads return `null` (`u8?`, `u16?`, `u32?`, `usize?`) instead of throwing `RangeError`.
