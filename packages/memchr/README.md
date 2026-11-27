[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# memchr

Zero-allocation 16-lane hardware SIMD byte and substring search engine for CP (Rust `memchr` v2 parity).

Uses `@Vector(16, u8)` with `@splat` and `@reduce(.Or, ...)` to scan 16 bytes per iteration for forward/reverse single-, two-, and three-byte searches, byte frequency counting, offset collection, and substring search (`memmem`).

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** |
| Original Crate | `memchr` v2 (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/BurntSushi/memchr` |
| License | MIT (upstream: MIT OR Unlicense) |

Keep the exact crate name when migrating from Rust to CP:

```rust
// Rust (crates.io: memchr)
use memchr::{memchr, memchr2, memrchr, memmem};
let pos = memchr(b'\n', haystack);
let sub = memmem::find(haystack, b"needle");
```

```cp
// CP (cap: memchr) — 16-lane SIMD, zero heap allocation
import { memchr, memchr2, memrchr, memmemFind } from "memchr";
const pos = memchr(0x0A, &haystack);       // i64 offset or -1
const sub = memmemFind(&haystack, &needle); // i64 offset or -1
```

## Installation

```bash
cap add memchr
```

## Usage

### Basic Usage: 16-Lane SIMD Byte & Substring Search

```cp
import { memchr, memchr2, memchr3, memrchr, memmemFind, memmemRfind } from "memchr";

const data: []const u8 = "GET /api/v1/users HTTP/1.1\r\nHost: example.com\r\n\r\n";
const needle: []const u8 = "HTTP/1.1";

// First space or newline (16-lane SIMD)
const sp = memchr(' ' as u8, &data);              // 3
const delim = memchr2('\r' as u8, '\n' as u8, &data); // 26
const any3 = memchr3(':' as u8, '/' as u8, '?' as u8, &data); // 4

// Reverse search from the end
const lastSlash = memrchr('/' as u8, &data);      // 22

// Substring search (forward & reverse)
const protoPos = memmemFind(&data, &needle);      // 18
const lastProto = memmemRfind(&data, &needle);    // 18
```

### Advanced Usage: Batch Offset Extraction & Byte Counting

```cp
import { memchrCount, memchrFindAll, countByte } from "memchr";

const csv: []const u8 = "id,name,role,region,status\n1,alice,admin,us,ok\n";

// Count all commas in one 16-lane SIMD pass
const commas = memchrCount(',' as u8, &csv); // 8
const newlines = countByte('\n' as u8, &csv); // 2

// Extract up to 16 comma offsets into a caller-owned stack buffer (zero heap)
let offsets: [16]usize = undefined;
const found = memchrFindAll(',' as u8, &csv, offsets[0..16]);
// offsets[0..found] holds every byte index where ',' appears
```

## API Reference

| Function | Signature | Description |
| --- | --- | --- |
| `memchr` | `fn(needle: u8, haystack: &[]u8): i64` | First index of `needle`, or `-1` if absent. |
| `memchr2` | `fn(n1: u8, n2: u8, haystack: &[]u8): i64` | First index of `n1` or `n2`, or `-1`. |
| `memchr3` | `fn(n1: u8, n2: u8, n3: u8, haystack: &[]u8): i64` | First index of `n1`, `n2`, or `n3`, or `-1`. |
| `memrchr` | `fn(needle: u8, haystack: &[]u8): i64` | Last index of `needle`, or `-1`. |
| `memrchr2` | `fn(n1: u8, n2: u8, haystack: &[]u8): i64` | Last index of `n1` or `n2`, or `-1`. |
| `memrchr3` | `fn(n1: u8, n2: u8, n3: u8, haystack: &[]u8): i64` | Last index of `n1`, `n2`, or `n3`, or `-1`. |
| `memchrCount` | `fn(needle: u8, haystack: &[]u8): usize` | Total occurrences of `needle` in `haystack`. |
| `countByte` | `fn(needle: u8, haystack: &[]u8): usize` | Fast byte counter (alias/building block for `bytecount`). |
| `memchrFindAll` | `fn(needle: u8, haystack: &[]u8, out_offsets: []usize): usize` | Writes matching offsets into `out_offsets`; returns count written. |
| `memmemFind` | `fn(haystack: &[]u8, needle: &[]u8): i64` | First index of substring `needle`, or `-1`. |
| `memmemRfind` | `fn(haystack: &[]u8, needle: &[]u8): i64` | Last index of substring `needle`, or `-1`. |

## Performance

- **16-lane hardware SIMD**: Processes 16 bytes per loop iteration using `@Vector(16, u8)` with scalar tail fallback for inputs `< 16` bytes.
- **Zero heap allocation**: All searches operate directly on borrowed `&[]u8` slices.

## License

MIT — Clean-room CP implementation with Rust `memchr` v2 parity.
