[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# adler2

Zero-heap RFC 1950 Adler-32 checksum engine for CP with 16-lane hardware SIMD, streaming updates, O(1) rolling window hash, and O(1) checksum combine (Rust `adler2` parity).

Processes 16 bytes per iteration using `@Vector(16, u8)` / `@Vector(16, u32)` weighted horizontal reductions with `NMAX = 5552` deferred modulo reduction.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** |
| Original Crate | `adler2` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/oyvindln/adler2` |
| License | MIT |

Keep the exact crate name when migrating from Rust to CP:

```rust
// Rust (crates.io: adler2)
use adler2::{Adler32, adler32_slice};
let sum = adler32_slice(b"Wikipedia");
```

```cp
// CP (cap: adler2) — 16-lane SIMD + rolling window + combine
import { Adler32, adler32, combineAdler32 } from "adler2";
const sum = adler32(&"Wikipedia"); // 0x11E60398
```

## Installation

```bash
cap add adler2
```

## Usage

### Basic Usage: One-Shot & Streaming 16-Lane SIMD Checksum

```cp
import { Adler32, adler32 } from "adler2";

// One-shot RFC 1950 check vector ("Wikipedia" -> 0x11E60398)
const data: []const u8 = "Wikipedia";
const sum = adler32(&data); // 0x11E60398

// Streaming across arbitrary chunks
let h = Adler32.init();
const p1: []const u8 = "Wiki";
const p2: []const u8 = "pedia";
h.update(&p1);
h.update(&p2);
const streamed = h.finalize(); // 0x11E60398

h.reset(); // Reuse the same stack struct without allocation
```

### Advanced Usage: O(1) Rolling Window (rsync / Deduplication) & Parallel Combine

```cp
import { Adler32, adler32, combineAdler32 } from "adler2";

// 1. O(1) rolling checksum over a 4-byte sliding window ("abcd" -> "bcde")
let roll = Adler32.init();
const win0: []const u8 = "abcd";
roll.update(&win0);
roll.roll(4, 'a' as u8, 'e' as u8);
// roll.finalize() == adler32(&"bcde")

// 2. Combine checksums computed independently in parallel
const chunkA: []const u8 = "hello ";
const chunkB: []const u8 = "world!";
const cA = adler32(&chunkA);
const cB = adler32(&chunkB);
const merged = combineAdler32(cA, cB, chunkB.len as u64);
// merged == adler32(&"hello world!")

// 3. Resume from a previously saved u32 checksum
let resumed = Adler32.fromValue(cA);
resumed.update(&chunkB);
// resumed.finalize() == merged
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `Adler32.init` | `static fn(): Adler32` | Creates a new hasher initialized to `s1 = 1, s2 = 0` (checksum `1`). |
| `Adler32.fromValue` | `static fn(checksum: u32): Adler32` | Restores a hasher from an existing Adler-32 value. |
| `Adler32.update` | `fn(*_, data: &[]u8)` | Feeds a byte slice using 16-lane SIMD with `NMAX = 5552` deferred modulo. |
| `Adler32.updateByte` | `fn(*_, b: u8)` | Feeds a single byte incrementally. |
| `Adler32.roll` | `fn(*_, window_len: usize, out_byte: u8, in_byte: u8)` | O(1) sliding-window update removing `out_byte` and appending `in_byte`. |
| `Adler32.finalize` | `fn(): u32` | Returns `(s2 << 16) \| s1`. |
| `Adler32.reset` | `fn(*_)` | Resets state back to `s1 = 1, s2 = 0`. |
| `adler32` | `fn(data: &[]u8): u32` | One-shot RFC 1950 Adler-32 checksum. |
| `combineAdler32` | `fn(adler1: u32, adler2: u32, len2: u64): u32` | O(1) combines `adler32(A)` and `adler32(B)` into `adler32(A \|\| B)`. |

## Performance

- **16-lane hardware SIMD**: Uses `@Vector(16, u8)` widened to `@Vector(16, u32)` with weights `[16, 15, ..., 1]`.
- **Deferred modulo**: Reduces modulo `65521` only once every `5552` bytes (largest `NMAX` avoiding 32-bit overflow).

## License

MIT — Clean-room CP implementation modeled after Rust `adler2`.
