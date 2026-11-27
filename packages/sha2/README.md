[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# sha2

Zero-heap FIPS 180-4 **SHA-256** and **SHA-224** streaming cryptographic hashers with lowercase hex output for CP (RustCrypto `sha2` parity).

All internal hash states, 64-byte string chunk buffers, and 32/28-byte digest outputs live strictly on the stack with zero heap allocations.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** |
| Original Crate | `sha2` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/RustCrypto/hashes` |
| License | MIT (upstream: MIT OR Apache-2.0) |

Keep the exact crate name when migrating from Rust to CP:

```rust
// Rust (crates.io: sha2)
use sha2::{Sha256, Sha224, Digest};
let mut h = Sha256::new();
h.update(b"hello world");
let digest = h.finalize();
```

```cp
// CP (cap: sha2) — zero-heap streaming SHA-256 & SHA-224
import { Sha256, Sha224, sha256Hex, sha224Hex } from "sha2";
let h = Sha256.new();
h.updateStr("hello world");
let digest: [32]u8 = undefined;
h.finalize(digest[0..32]);
```

## Installation

```bash
cap add sha2
```

## Usage

### Basic Usage: One-Shot Hex & Streaming SHA-256 / SHA-224

```cp
import { Sha256, Sha224, sha256Hex, sha224Hex } from "sha2";

// 1. One-shot lowercase hex (FIPS 180-4 "abc" vectors)
let hex256: [64]u8 = undefined;
const n256 = sha256Hex("abc", hex256[0..64]);
// n256 == 64 -> "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"

let hex224: [56]u8 = undefined;
const n224 = sha224Hex("abc", hex224[0..56]);
// n224 == 56 -> "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7"

// 2. Streaming multi-chunk SHA-256 into raw 32-byte digest
let h = Sha256.new();
let part1: [5]u8 = ['h', 'e', 'l', 'l', 'o'];
h.update(part1[0..5]);
h.updateStr(" world");
let raw32: [32]u8 = undefined;
h.finalize(raw32[0..32]);
```

### Advanced & Edge Cases: Hasher Reuse (`reset`) & Truncated SHA-224

```cp
import { Sha256, Sha224 } from "sha2";

// 1. Reuse the same stack Sha256 hasher across multiple payloads via reset()
let hasher = Sha256.new();
let outHex: [64]u8 = undefined;

hasher.updateStr("chunk-A");
const lenA = hasher.finalizeHex(outHex[0..64]); // 64

hasher.reset(); // Restores initial FIPS 180-4 SHA-256 IV
hasher.updateStr("chunk-B");
const lenB = hasher.finalizeHex(outHex[0..64]); // 64

// 2. Streaming SHA-224 (28-byte raw digest / 56-byte lowercase hex)
let h224 = Sha224.new();
h224.updateStr("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
let dig28: [28]u8 = undefined;
h224.finalize(dig28[0..28]);
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `Sha256.new` | `static fn(): Sha256` | Creates a new FIPS 180-4 SHA-256 hasher. |
| `Sha256.update` | `fn(*_, data: []u8)` | Feeds raw bytes into SHA-256 state. |
| `Sha256.updateStr` | `fn(*_, s: string)` | Feeds a `string` in 64-byte stack chunks. |
| `Sha256.finalize` | `fn(*_, out32: []u8)` | Writes 32-byte SHA-256 digest into `out32`. |
| `Sha256.finalizeHex` | `fn(*_, out64: []u8): usize` | Writes 64-byte lowercase hex into `out64`; returns `64` or `0`. |
| `Sha256.reset` | `fn(*_)` | Resets internal state back to SHA-256 IV. |
| `Sha224.new` | `static fn(): Sha224` | Creates a new FIPS 180-4 SHA-224 hasher. |
| `Sha224.update` / `updateStr` | `fn(*_, data: []u8)` / `fn(*_, s: string)` | Feeds bytes or string into SHA-224 state. |
| `Sha224.finalize` | `fn(*_, out28: []u8)` | Writes 28-byte SHA-224 digest into `out28`. |
| `Sha224.finalizeHex` | `fn(*_, out56: []u8): usize` | Writes 56-byte lowercase hex into `out56`; returns `56` or `0`. |
| `sha256Hex` | `fn(s: string, out64: []u8): usize` | One-shot SHA-256 lowercase hex (64 bytes). |
| `sha224Hex` | `fn(s: string, out56: []u8): usize` | One-shot SHA-224 lowercase hex (56 bytes). |

## Security & Performance

- **Zero heap allocation**: Operates strictly on stack buffers (`[32]u8`, `[28]u8`, `[64]u8`).
- **FIPS 180-4 compliant**: Verified against NIST SHA-256 and SHA-224 standard IVs and test vectors.

## License

MIT — Clean-room CP implementation with RustCrypto `sha2` parity.
