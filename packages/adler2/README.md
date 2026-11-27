[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# adler2

Zero-heap RFC 1950 Adler-32 checksum engine with streaming updates, rolling window updates (rsync-style), and checksum combining in pure CP.

Aligned with Rust crates.io [`adler2`](https://github.com/oyvindln/adler2) (MIT).

## Installation & Import

```bash
cap add adler2
```

```cp
import {
    ADLER32_MOD,
    ADLER32_NMAX,
    Adler32,
    adler32,
    adler32Slice,
    adler32Str,
    combineAdler32,
    adler32Combine,
} from "adler2";
```

## Core API

### Constants

| Symbol | Type | Description |
| --- | --- | --- |
| `ADLER32_MOD` | `u32 = 65521` | Largest prime smaller than 65536 (RFC 1950 modulus). |
| `ADLER32_NMAX` | `usize = 5552` | Maximum bytes processed per deferred modulo chunk without 32-bit overflow. |

### `Adler32` Streaming Class

| Member | Signature | Description |
| --- | --- | --- |
| Fields | `s1: u32, s2: u32` | Internal Adler-32 accumulators (`s1 = 1, s2 = 0` initially). |
| `Adler32.init` / `new` | `(): Adler32` | Creates a fresh Adler-32 hasher initialized to `1`. |
| `Adler32.fromValue` / `fromChecksum` | `(value: u32): Adler32` | Restores hasher state from an existing 32-bit Adler-32 checksum. |
| `update` / `writeSlice` | `(*self, data: []const u8)` | Feeds a read-only byte slice into the checksum. |
| `writeStr` | `(*self, s: string)` | Feeds a UTF-8 string into the checksum. |
| `updateByte` / `writeByte` | `(*self, b: u8)` | Feeds a single byte into the checksum. |
| `roll` | `(*self, window_len: usize, out_byte: u8, in_byte: u8)` | Updates rolling checksum in O(1) as `out_byte` leaves and `in_byte` enters a window of length `window_len`. |
| `finalize` / `checksum` | `(): u32` | Returns `(s2 << 16) | s1` without mutating state. |
| `reset` | `(*self)` | Resets state back to `(s1 = 1, s2 = 0)`. |

### One-Shot & Combine Functions

| Function | Signature | Description |
| --- | --- | --- |
| `adler32` / `adler32Slice` | `(data: []const u8): u32` | Computes Adler-32 checksum of a byte slice. |
| `adler32Str` | `(s: string): u32` | Computes Adler-32 checksum of a string. |
| `combineAdler32` / `adler32Combine` | `(adler1: u32, adler2: u32, len2: u64): u32` | Combines two sequential Adler-32 checksums given the byte length `len2` of the second block. |

## Examples

### 1. One-Shot & Streaming Checksum

```cp
import { print } from "cap:io";
import { Adler32, adler32Str } from "adler2";

pub fn main() {
    let direct = adler32Str("Wikipedia");
    let h = Adler32.new();
    h.writeStr("Wiki");
    h.writeStr("pedia");
    print("direct={}, stream={}\n", direct, h.checksum()); // both 0x11e60398
}
```

### 2. Rolling Window Checksum

```cp
import { print } from "cap:io";
import { Adler32 } from "adler2";

pub fn main() {
    let h = Adler32.new();
    h.writeStr("abcd");
    h.roll(4, 97, 101); // slide window from "abcd" to "bcde"
    print("rolled={}\n", h.checksum());
}
```

### 3. Combining Parallel Chunk Checksums

```cp
import { print } from "cap:io";
import { adler32Str, combineAdler32 } from "adler2";

pub fn main() {
    let c1 = adler32Str("hello ");
    let c2 = adler32Str("world");
    let combined = combineAdler32(c1, c2, 5);
    print("combined={}, full={}\n", combined, adler32Str("hello world"));
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Zero heap allocation**: `Adler32` is an 8-byte stack struct (`s1: u32, s2: u32`) with deferred modulo batching (`ADLER32_NMAX = 5552`) for high throughput.
- **Built-in rolling & combine**: Extends standard `adler2` streaming with O(1) sliding-window `roll` (for rsync/content-defined chunking) and `combineAdler32` (for parallel zlib streams).
- **Non-destructive `checksum` / `finalize`**: Reading the digest does not consume the hasher, allowing incremental checkpoints.
