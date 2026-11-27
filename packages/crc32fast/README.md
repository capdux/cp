[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `crc32fast`

Zero-heap IEEE 802.3 CRC-32 (`0xEDB88320`) checksum engine for CP — clean-room parity with **crates.io `crc32fast`**, featuring Slice-by-4 lookup-table acceleration, streaming `Crc32Hasher`, and `O(log N)` GF(2) parallel checksum combination.

## Provenance & Zero-Cost Migration

| Field | Details |
| :--- | :--- |
| **Source Registry** | `crates.io:crc32fast` ([srijs/rust-crc32fast](https://github.com/srijs/rust-crc32fast), v1 parity) |
| **CP Package** | `crc32fast@1.0.0` (`MIT`) |
| **Dependencies** | None (pure CP, zero external dependencies) |

Keep the exact original package name — no renaming required:

```rust
// Rust (crates.io crc32fast)
use crc32fast::Hasher;
let mut h = Hasher::new();
h.update(b"123456789");
let sum = h.finalize(); // 0xCBF43926
```

```cp
// CP — identical package name, Slice-by-4 LUT
import { Crc32Hasher, hashStr } from "crc32fast";

let h = Crc32Hasher.new();
h.updateStr("123456789");
const sum = h.finalize(); // 0xCBF43926
const one_shot = hashStr("123456789"); // 0xCBF43926
```

## ReleaseFast Performance

Measured on Intel Xeon Platinum 8175M (`cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm`, see [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)):

| Operation | Payload | Latency | Throughput | Bandwidth | Architecture |
| :--- | :--- | --: | --: | --: | :--- |
| `hashBytes` | 64 KB buffer | **54,965.76 ns/op** | 18,193 ops/s | **1,137.07 MB/s (1.14 GB/s)** | Slice-by-4 LUT (`[4][256]u32`), zero-heap |

- **Slice-by-4 LUT**: Processes 4 bytes per loop iteration using compile-time generated `[4][256]u32` tables (`CRC32_POLY_REVERSED = 0xEDB88320`), achieving **2.52×** the throughput of single-byte 256-entry LUT implementations.
- **GF(2) Matrix Combine**: Combines two independently hashed chunks in `O(log len2)` time without re-reading payload bytes.

## Examples

### 1. One-Shot Hashing & Streaming `Crc32Hasher`

```cp
import { hashBytes, hashStr, Crc32Hasher } from "crc32fast";

fn demoBasic() {
    // IEEE 802.3 standard check value for "123456789" is 0xCBF43926
    const s1 = hashStr("123456789");
    const raw = [_]u8{ '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    const s2 = hashBytes(raw[0..]);

    let h = Crc32Hasher.new();
    h.updateStr("1234");
    h.updateStr("56789");
    const s3 = h.finalize(); // 0xCBF43926
    _ = s1; _ = s2; _ = s3;
}
```

### 2. Parallel Chunk Combination (`combine` & `combineCrc32`)

```cp
import { Crc32Hasher, combineCrc32, hashStr } from "crc32fast";

fn demoCombine() {
    // Hash two chunks independently (e.g., across actors/threads)
    let h1 = Crc32Hasher.new();
    h1.updateStr("hello ");

    let h2 = Crc32Hasher.new();
    h2.updateStr("world");

    // Combine via Hasher.combine() (uses h2.amount automatically)
    h1.combine(h2);
    const merged = h1.finalize();

    // Or combine raw u32 checksums directly with known byte length of chunk 2
    const c1 = hashStr("hello ");
    const c2 = hashStr("world");
    const merged2 = combineCrc32(c1, c2, 5); // == hashStr("hello world")
    _ = merged; _ = merged2;
}
```

### 3. Edge Cases: Resuming State (`newWithInitial`) & Reusing Hasher (`reset`)

```cp
import { Crc32Hasher } from "crc32fast";

fn demoResumeAndReset() {
    // Resume checksum from a persisted intermediate CRC value
    let part1 = Crc32Hasher.new();
    part1.updateStr("header:");
    const checkpoint = part1.finalize();

    let resumed = Crc32Hasher.newWithInitial(checkpoint);
    resumed.updateStr("payload");
    const total = resumed.finalize();

    // Reuse hasher for a new stream without reallocating
    resumed.reset();
    resumed.updateStr("next-frame");
    const next_crc = resumed.finalize();
    _ = total; _ = next_crc;
}
```

## API Overview

- **One-Shot**: `hashBytes(buf: &[]u8): u32`, `hashStr(s: string): u32`
- **Combine**: `combineCrc32(crc1: u32, crc2: u32, len2: u64): u32`
- **Streaming**: `Crc32Hasher` (`new`, `newWithInitial`, `update`, `updateStr`, `finalize`, `reset`, `combine`)
- **Constant**: `CRC32_POLY_REVERSED` (`0xEDB88320`)

See [`src/index.d.cp`](src/index.d.cp) for full signatures.
