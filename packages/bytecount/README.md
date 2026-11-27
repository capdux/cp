[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# bytecount

Zero-heap 16-lane hardware SIMD byte frequency counter, UTF-8 scalar counter, line counter, and range counter for CP (Rust `bytecount` parity).

Uses `@Vector(16, u8)` with `@select` and batched 255-iteration `u8` lane accumulators (zero overflow, minimal horizontal reductions) to count single bytes, byte ranges `[lo, hi]`, UTF-8 characters (`(b & 0xC0) != 0x80`), lines, and ASCII whitespace.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** |
| Original Crate | `bytecount` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/llogiq/bytecount` |
| License | MIT (upstream: Apache-2.0 OR MIT) |

Keep the exact crate name when migrating from Rust to CP:

```rust
// Rust (crates.io: bytecount)
let n = bytecount::count(haystack, b'\n');
let chars = bytecount::num_chars(utf8_bytes);
```

```cp
// CP (cap: bytecount) — 16-lane SIMD with 255-step lane accumulation
import { count, countFast, numChars, countLines } from "bytecount";
const n = countFast(&haystack, 0x0A);
const chars = numChars(&utf8_bytes);
```

## Installation

```bash
cap add bytecount
```

## Usage

### Basic Usage: 16-Lane SIMD Byte, UTF-8 Character & Line Counting

```cp
import { count, countFast, numChars, countLines } from "bytecount";

const text: []const u8 = "line1\nline2\nline3";

// Fast 16-lane SIMD byte frequency counting
const nls1 = count(&text, '\n' as u8);     // 2 (delegates to memchr.countByte)
const nls2 = countFast(&text, '\n' as u8); // 2 (255-step SIMD accumulator)

// Count lines (handles trailing newline vs no trailing newline)
const lines = countLines(&text);           // 3

// Count UTF-8 Unicode scalar values (non-continuation bytes)
const utf8: []const u8 = "hello 世界!";
const chars = numChars(&utf8);             // 9
```

### Advanced Usage: Byte-Range & Whitespace Analytics

```cp
import { countInRange, countAsciiWhitespace } from "bytecount";

const logLine: []const u8 = "2026-11-27 16:45:00 INFO status=200 latency=12ms\n";

// 16-lane SIMD range counting: count ASCII digits '0'..'9'
const digits = countInRange(&logLine, '0' as u8, '9' as u8);

// Count uppercase ASCII letters 'A'..'Z'
const uppers = countInRange(&logLine, 'A' as u8, 'Z' as u8);

// Count ASCII whitespace (' ', '\t', '\n', '\r', '\f')
const ws = countAsciiWhitespace(&logLine);
```

## API Reference

| Function | Signature | Description |
| --- | --- | --- |
| `count` | `fn(haystack: &[]u8, needle: u8): usize` | Counts occurrences of `needle` (reuses `memchr.countByte`). |
| `countFast` | `fn(haystack: &[]u8, needle: u8): usize` | 16-lane SIMD counter with 255-step `u8` vector accumulator. |
| `numChars` | `fn(utf8_bytes: &[]u8): usize` | 16-lane SIMD UTF-8 character counter (counts bytes where `(b & 0xC0) != 0x80`). |
| `countLines` | `fn(buf: &[]u8): usize` | Counts logical lines (`0` for empty buffer; trailing `\n` does not add an extra line). |
| `countAsciiWhitespace` | `fn(buf: &[]u8): usize` | Counts ASCII whitespace bytes (`0x20`, `0x09`, `0x0A`, `0x0D`, `0x0C`). |
| `countInRange` | `fn(buf: &[]u8, lo: u8, hi: u8): usize` | 16-lane SIMD count of bytes `lo <= b <= hi` (returns `0` if `lo > hi`). |

## Performance

- **16-lane SIMD + 255-step batching**: Accumulates up to `255 × 16 = 4080` bytes directly in a `@Vector(16, u8)` register before a single horizontal sum, avoiding per-vector reduction stalls.
- **Zero heap allocation**: Operates strictly on `&[]u8` slices.

## License

MIT — Clean-room CP implementation with Rust `bytecount` parity.
