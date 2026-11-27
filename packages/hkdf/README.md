[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# hkdf

Zero-heap RFC 5869 **HKDF-SHA256** (HMAC-based Extract-and-Expand Key Derivation Function) for CP (RustCrypto `hkdf` parity).

Supports two-phase `extract` + `expand`, `fromPrk` resumption from an existing 32-byte pseudorandom key, and one-shot `hkdfSha256` — all on the stack with zero heap allocations.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** |
| Original Crate | `hkdf` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/RustCrypto/KDFs` |
| License | MIT (upstream: MIT OR Apache-2.0) |

Keep the exact crate name when migrating from Rust to CP:

```rust
// Rust (crates.io: hkdf)
use hkdf::Hkdf;
let hk = Hkdf::<Sha256>::new(Some(salt), ikm);
hk.expand(info, &mut okm)?;
```

```cp
// CP (cap: hkdf) — zero-heap RFC 5869 HKDF-SHA256
import { HkdfSha256, hkdfSha256 } from "hkdf";
const hk = HkdfSha256.extract(salt, ikm);
const ok = hk.expand(info, okm);
```

## Installation

```bash
cap add hkdf
```

## Usage

### Basic Usage: One-Shot & Multi-Key Extract-Then-Expand

```cp
import { HkdfSha256, hkdfSha256 } from "hkdf";

let salt: [13]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C];
let ikm: [22]u8 = [0x0B; 22];
let info: [10]u8 = [0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9];

// 1. One-shot derivation (RFC 5869 Test Case 1 -> 42-byte OKM)
let okm: [42]u8 = undefined;
const ok = hkdfSha256(salt[0..13], ikm[0..22], info[0..10], okm[0..42]);

// 2. Extract once, expand multiple domain-separated subkeys
const hk = HkdfSha256.extract(salt[0..13], ikm[0..22]);
let encInfo: [7]u8 = ['e', 'n', 'c', '-', 'k', 'e', 'y'];
let macInfo: [7]u8 = ['m', 'a', 'c', '-', 'k', 'e', 'y'];
let encKey: [32]u8 = undefined;
let macKey: [32]u8 = undefined;
const ok1 = hk.expand(encInfo[0..7], encKey[0..32]);
const ok2 = hk.expand(macInfo[0..7], macKey[0..32]);
```

### Security & Edge Cases: Empty Salt, `fromPrk` Validation & 8160-Byte RFC Limit

```cp
import { HkdfSha256 } from "hkdf";

// 1. Empty salt automatically defaults to 32 zero bytes per RFC 5869 §2.2
let emptySalt: [0]u8 = [];
let ikm: [16]u8 = [0x42; 16];
const hk = HkdfSha256.extract(emptySalt[0..0], ikm[0..16]);

// 2. Resume directly from a pre-extracted PRK (returns null if prk_in.len < 32)
let shortPrk: [16]u8 = [0; 16];
const invalid = HkdfSha256.fromPrk(shortPrk[0..16]); // null!

if (resumed := HkdfSha256.fromPrk(hk.prk[0..32])) {
    let key: [32]u8 = undefined;
    const derived = resumed.expand(emptySalt[0..0], key[0..32]); // true
}

// 3. RFC 5869 enforces max OKM length = 255 * 32 = 8160 bytes; larger slices return false
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `HkdfSha256.extract` | `static fn(salt: []u8, ikm: []u8): HkdfSha256` | RFC 5869 HKDF-Extract (`salt.len == 0` uses 32 zero bytes). |
| `HkdfSha256.fromPrk` | `static fn(prk_in: []u8): HkdfSha256?` | Constructs from a `>= 32`-byte PRK; returns `null` if shorter than 32 bytes. |
| `HkdfSha256.expand` | `fn(info: []u8, okm: []u8): bool` | RFC 5869 HKDF-Expand; returns `false` if `okm.len > 8160`. |
| `hkdfSha256` | `fn(salt: []u8, ikm: []u8, info: []u8, okm: []u8): bool` | One-shot Extract + Expand into `okm`. |

## Security Properties

- **Zero heap allocation**: PRK (`[32]u8`), counter blocks, and HMAC states live strictly on the stack.
- **RFC 5869 compliant**: Enforces `<= 255 × 32 = 8160` bytes OKM limit and zero-filled default salt.

## License

MIT — Clean-room CP implementation with RustCrypto `hkdf` parity.
