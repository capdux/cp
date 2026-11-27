[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# simd-adler32

Zero-allocation 16-lane hardware SIMD Adler-32, IEEE CRC-32, Castagnoli CRC-32C, and O(log N) GF(2) CRC-32 combine engine for CP (Rust `simd-adler32` & `crc32fast` / `crc-catalog` parity).

Combines a 16-lane `@Vector(16, u8)` / `@Vector(16, u32)` SIMD Adler-32 kernel (with `NMAX = 5552` deferred modulo) with table-driven streaming `Crc32Hasher` (IEEE `0xEDB88320`), `Crc32cHasher` (Castagnoli `0x82F63B78`), and zlib-compatible `combineCrc32Ieee`.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** |
| Original Crates | `simd-adler32` / `crc-catalog` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/mcountryman/simd-adler32` · `https://github.com/akhilles/crc-catalog` |
| License | MIT |

Keep the exact crate name when migrating from Rust to CP:

```rust
// Rust (crates.io: simd-adler32 / crc32fast)
let a = simd_adler32::adler32(&b"Wikipedia"[..]);
let c = crc32fast::hash(b"123456789");
```

```cp
// CP (cap: simd-adler32) — 16-lane SIMD Adler-32 + IEEE/Castagnoli CRC-32 + O(log N) combine
import { adler32, crc32Ieee, crc32c, combineCrc32Ieee } from "simd-adler32";
const a = adler32("Wikipedia");   // 0x11E60398
const c = crc32Ieee("123456789"); // 0xCBF43926
```

## Installation

```bash
cap add simd-adler32
```

## Usage

### Basic Usage: 16-Lane SIMD Adler-32, IEEE CRC-32, and Castagnoli CRC-32C

```cp
import { adler32, crc32Ieee, crc32c } from "simd-adler32";

// 16-lane hardware SIMD Adler-32 (RFC 1950)
const a = adler32("Wikipedia");   // 0x11E60398

// IEEE 802.3 CRC-32 (gzip / PNG / ZIP)
const ieee = crc32Ieee("123456789"); // 0xCBF43926

// Castagnoli CRC-32C (iSCSI / RocksDB / LevelDB / gRPC)
const cast = crc32c("123456789");    // 0xE3069283
```

### Advanced Usage: Streaming Hashers & O(log N) Parallel CRC-32 Combine

```cp
import { Crc32Hasher, Crc32cHasher, crc32Ieee, combineCrc32Ieee } from "simd-adler32";

// 1. Streaming IEEE CRC-32 across chunks
let h = Crc32Hasher.init();
h.update("1234");
h.update("56789");
const sum = h.finalize(); // 0xCBF43926
h.reset();                // Reuse without rebuilding the 256-entry table

// 2. Streaming Castagnoli CRC-32C
let hc = Crc32cHasher.init();
hc.update("header:");
hc.update("payload");
const tag = hc.finalize();

// 3. O(log N) GF(2) matrix-squaring combine for parallel chunked CRC-32
const c1 = crc32Ieee("1234");
const c2 = crc32Ieee("56789");
const combined = combineCrc32Ieee(c1, c2, 5); // 0xCBF43926 == crc32Ieee("123456789")
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `adler32` | `fn(data: string): u32` | 16-lane SIMD RFC 1950 Adler-32 checksum. |
| `crc32Ieee` | `fn(data: string): u32` | One-shot IEEE 802.3 CRC-32 (`0xEDB88320`). |
| `crc32c` | `fn(data: string): u32` | One-shot Castagnoli CRC-32C (`0x82F63B78`). |
| `Crc32Hasher` | `class { init(), update(*_, data: string), finalize(): u32, reset(*_) }` | Streaming IEEE CRC-32 hasher with 256-entry LUT. |
| `Crc32cHasher` | `class { init(), update(*_, data: string), finalize(): u32, reset(*_) }` | Streaming Castagnoli CRC-32C hasher with 256-entry LUT. |
| `combineCrc32Ieee` | `fn(crc1: u32, crc2: u32, len2: usize): u32` | O(log N) GF(2) matrix-squaring CRC-32 combine (`zlib` `crc32_combine` parity). |

## Performance

- **16-lane hardware SIMD**: `adler32` widens 16 bytes to `@Vector(16, u32)` with weights `[16..1]` and defers modulo `65521` every `5552` bytes.
- **O(log N) combine**: `combineCrc32Ieee` merges arbitrary-length chunk checksums in at most 64 GF(2) 32x32 matrix multiplications with zero heap allocation.

## License

MIT — Clean-room CP implementation inspired by `simd-adler32`, `crc-catalog`, and `crc32fast`.
