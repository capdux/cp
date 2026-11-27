[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# crc64fast

Zero-allocation 64-bit CRC checksum engine for CP supporting **ECMA-182**, **CRC-64/XZ**, and **CRC-64/GO-ISO** with comptime **Slice-by-4** (`[4][256]u64`) lookup tables and streaming `Crc64Hasher` (Rust `crc64fast` / `crc` parity).

Precomputes 4×256 Slice-by-4 lookup tables at compile time for all three standard presets, processing 4 bytes per loop iteration with zero heap allocation and zero startup table-generation overhead.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** |
| Original Crate | `crc64fast` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/srijs/rust-crc64fast` |
| License | MIT |

Keep the exact crate name when migrating from Rust to CP:

```rust
// Rust (crates.io: crc64fast)
use crc64fast::Digest;
let mut d = Digest::new();
d.write(b"123456789");
let sum = d.sum64();
```

```cp
// CP (cap: crc64fast) — comptime Slice-by-4 LUT, zero heap
import { Crc64Hasher, crc64Ecma, crc64Xz, crc64Iso } from "crc64fast";
const sum = crc64Ecma(&"123456789"); // 0x6C40DF5F0B497347
```

## Installation

```bash
cap add crc64fast
```

## Usage

### Basic Usage: One-Shot Standard Checksums (Official `"123456789"` Vectors)

```cp
import { crc64Ecma, crc64Xz, crc64Iso } from "crc64fast";

const data: []const u8 = "123456789";

// ECMA-182 (normal, poly 0x42F0E1EBA9EA3693) -> 0x6C40DF5F0B497347
const ecma = crc64Ecma(&data);

// CRC-64/XZ (reflected, poly 0xC96C5795D7870F42, init/xorout ~0) -> 0x995DC9BBDF1939FA
const xz = crc64Xz(&data);

// CRC-64/GO-ISO (reflected, poly 0xD800000000000000, init/xorout ~0) -> 0xB90956C775A41001
const iso = crc64Iso(&data);
```

### Advanced Usage: Streaming Slice-by-4 Hasher & Custom Polynomials

```cp
import { Crc64Hasher, makeU64 } from "crc64fast";

// 1. Streaming CRC-64/XZ across network/file chunks (uses comptime Slice-by-4 LUT)
let h = Crc64Hasher.initXz();
const c1: []const u8 = "1234";
const c2: []const u8 = "56789";
h.update(&c1);
h.update(&c2);
const digest = h.finalize(); // 0x995DC9BBDF1939FA

h.reset(); // Reset to initial state for next stream

// 2. Custom 64-bit CRC polynomial (e.g. CRC-64/WE or Redis/NVMe custom variant)
const poly = makeU64(0x42F0E1EB, 0xA9EA3693);
const allOnes = makeU64(0xFFFFFFFF, 0xFFFFFFFF);
let custom = Crc64Hasher.initCustom(poly, allOnes, allOnes, false);
custom.update(&c1);
custom.update(&c2);
const customSum = custom.finalize();
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `crc64Ecma` | `fn(data: &[]u8): u64` | One-shot CRC-64/ECMA-182 (Slice-by-4 LUT). |
| `crc64Xz` | `fn(data: &[]u8): u64` | One-shot CRC-64/XZ (Slice-by-4 LUT). |
| `crc64Iso` | `fn(data: &[]u8): u64` | One-shot CRC-64/GO-ISO (Slice-by-4 LUT). |
| `Crc64Hasher.initEcma` | `static fn(): Crc64Hasher` | Streaming ECMA-182 hasher (comptime Slice-by-4). |
| `Crc64Hasher.initXz` | `static fn(): Crc64Hasher` | Streaming CRC-64/XZ hasher (comptime Slice-by-4). |
| `Crc64Hasher.initIso` | `static fn(): Crc64Hasher` | Streaming CRC-64/GO-ISO hasher (comptime Slice-by-4). |
| `Crc64Hasher.initCustom` | `static fn(poly: u64, init_val: u64, xorout: u64, reflected: bool): Crc64Hasher` | Custom 64-bit CRC hasher with runtime 256-entry LUT. |
| `Crc64Hasher.update` | `fn(*_, data: &[]u8)` | Feeds a byte slice into the streaming CRC state. |
| `Crc64Hasher.finalize` | `fn(): u64` | Returns `crc ^ xorout` (non-destructive; call `reset` to reuse). |
| `Crc64Hasher.reset` | `fn(*_)` | Resets `crc` back to `init_val`. |
| `makeU64` | `fn(hi: u32, lo: u32): u64` | Helper combining high/low 32-bit words into a `u64`. |

## Performance

- **Comptime Slice-by-4 LUT**: `CRC64_ECMA_LUT`, `CRC64_XZ_LUT`, and `CRC64_ISO_LUT` (`[4][256]u64`) are generated at compile time, processing 4 bytes per iteration (~4× faster than byte-at-a-time CRC).
- **Zero heap allocation**: All state lives on the stack or in read-only comptime data.

## License

MIT — Clean-room CP implementation inspired by Rust `crc64fast` and `crc`.
