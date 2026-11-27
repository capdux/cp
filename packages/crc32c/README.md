[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `crc32c`

Zero-heap pure-CP CRC-32C (Castagnoli, `0x82F63B78`) checksum library — clean-room parity with **crates.io `crc32c`**, featuring streaming `crc32cAppend` & `Crc32cHasher`, `O(log N)` GF(2) combine, and LevelDB / RocksDB / Google Cloud Storage masking (`maskCrc32c` / `unmaskCrc32c`).

## Provenance & Zero-Cost Migration

| Field | Details |
| :--- | :--- |
| **Source Registry** | `crates.io:crc32c` ([zowens/crc32c](https://github.com/zowens/crc32c), v0.6 parity) |
| **CP Package** | `crc32c@1.0.0` (`MIT`) |
| **Dependencies** | None (pure CP, zero external dependencies) |

Keep the exact original package name — no renaming required:

```rust
// Rust (crates.io crc32c)
let sum = crc32c::crc32c(b"123456789"); // 0xE3069283
let chained = crc32c::crc32c_append(sum, b"more");
```

```cp
// CP — identical package name, zero-heap
import { crc32cStr, crc32cAppend, Crc32cHasher } from "crc32c";

const sum = crc32cStr("123456789"); // 0xE3069283
let h = Crc32cHasher.new();
h.updateStr("123456789");
const s2 = h.finalize();            // 0xE3069283
```

## ReleaseFast Performance

Measured on Intel Xeon Platinum 8175M (`cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm`, see [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)):

| Operation | Payload | Latency | Throughput | Bandwidth | Architecture |
| :--- | :--- | --: | --: | --: | :--- |
| `crc32c` | 64 KB buffer | **138,707.48 ns/op** | 7,209 ops/s | **450.59 MB/s** | 256-entry Castagnoli LUT, zero-heap |

- **Pure-CP portability**: Deterministic Castagnoli polynomial (`CRC32C_POLY_REVERSED = 0x82F63B78`) with 256-entry compile-time LUT across all architectures.
- **Storage-engine ready**: Built-in LevelDB / RocksDB / Snappy / GCS mask/unmask formula (`CRC32C_MASK_DELTA = 0xA282EAD8`).

## Examples

### 1. One-Shot & Incremental Append (`crc32c`, `crc32cStr`, `crc32cAppend`)

```cp
import { crc32c, crc32cStr, crc32cAppend } from "crc32c";

fn demoBasic() {
    // Castagnoli standard check value for "123456789" is 0xE3069283
    const s1 = crc32cStr("123456789");

    let p1 = [_]u8{ '1', '2', '3', '4' };
    let p2 = [_]u8{ '5', '6', '7', '8', '9' };
    const partial = crc32c(p1[0..]);
    const total = crc32cAppend(partial, p2[0..]); // 0xE3069283
    _ = s1; _ = total;
}
```

### 2. Streaming `Crc32cHasher` & Parallel GF(2) Combine

```cp
import { Crc32cHasher, crc32cCombine, crc32cStr } from "crc32c";

fn demoStreamingAndCombine() {
    let h1 = Crc32cHasher.new();
    h1.updateStr("block-A:");

    let h2 = Crc32cHasher.new();
    h2.updateStr("block-B");

    h1.combine(h2);
    const merged = h1.finalize();

    // Direct u32 combine with known byte length of chunk 2
    const c1 = crc32cStr("block-A:");
    const c2 = crc32cStr("block-B");
    const merged2 = crc32cCombine(c1, c2, 7); // == crc32cStr("block-A:block-B")
    _ = merged; _ = merged2;
}
```

### 3. Storage-Engine Masking (`maskCrc32c` / `unmaskCrc32c`) & Reset

```cp
import { Crc32cHasher, maskCrc32c, unmaskCrc32c } from "crc32c";

fn demoMasking() {
    // LevelDB / RocksDB / Snappy / GCS mask prevents CRC-of-CRC cancellation
    let h = Crc32cHasher.new();
    h.updateStr("wal-record-payload");
    const raw_crc = h.finalize();

    const stored = maskCrc32c(raw_crc);
    const restored = unmaskCrc32c(stored); // restored == raw_crc

    // Reuse hasher for next WAL record
    h.reset();
    h.updateStr("next-wal-record");
    const next_crc = h.finalize();
    _ = restored; _ = next_crc;
}
```

## API Overview

- **One-Shot / Append**: `crc32c(data: []u8): u32`, `crc32cStr(s: string): u32`, `crc32cAppend(crc: u32, data: []u8): u32`
- **Combine**: `crc32cCombine(crc1: u32, crc2: u32, len2: u64): u32`
- **Masking**: `maskCrc32c(crc: u32): u32`, `unmaskCrc32c(masked: u32): u32`
- **Streaming**: `Crc32cHasher` (`new`, `newWithInitial`, `update`, `updateStr`, `finalize`, `reset`, `combine`)
- **Constants**: `CRC32C_POLY_REVERSED` (`0x82F63B78`), `CRC32C_MASK_DELTA` (`0xA282EAD8`)

See [`src/index.d.cp`](src/index.d.cp) for full signatures.
