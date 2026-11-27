[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hkdf

Zero-heap RFC 5869 HKDF (HMAC-based Extract-and-Expand Key Derivation Function) supporting SHA-256, SHA-384, and SHA-512, plus multi-info and TLS 1.3 `HkdfLabel` expansion in pure CP.

- **Package**: `hkdf` (`v1.0.0`)
- **Upstream Alignment**: Clean-room CP port of `crates.io/hkdf` (`MIT OR Apache-2.0`, `RustCrypto/KDFs`), composing `hmac` for underlying HMAC-SHA2 primitives.

## Installation & Import

```bash
cap add hkdf
```

```typescript
import {
    HKDF_HASH_LEN, HKDF_MAX_OKM_LEN,
    HKDF_SHA256_HASH_LEN, HKDF_SHA256_MAX_OKM_LEN,
    HKDF_SHA384_HASH_LEN, HKDF_SHA384_MAX_OKM_LEN,
    HKDF_SHA512_HASH_LEN, HKDF_SHA512_MAX_OKM_LEN,
    HkdfHashAlg, HkdfSha256, HkdfSha384, HkdfSha512,
    hkdfSha256, hkdfSha384, hkdfSha512,
    hkdfExtract, hkdfExpand, hkdfDerive,
} from "hkdf";
```

## Core API

### Constants & Algorithm Enum

| Symbol | Value / Signature | Description |
| --- | --- | --- |
| `HKDF_HASH_LEN` / `HKDF_SHA256_HASH_LEN` | `32` | SHA-256 PRK length in bytes. |
| `HKDF_MAX_OKM_LEN` / `HKDF_SHA256_MAX_OKM_LEN` | `8160` (`255 * 32`) | Maximum RFC 5869 OKM length for SHA-256. |
| `HKDF_SHA384_HASH_LEN` / `HKDF_SHA384_MAX_OKM_LEN` | `48` / `12240` | SHA-384 PRK length and max OKM length. |
| `HKDF_SHA512_HASH_LEN` / `HKDF_SHA512_MAX_OKM_LEN` | `64` / `16320` | SHA-512 PRK length and max OKM length. |
| `HkdfHashAlg` | `Sha256`, `Sha384`, `Sha512`; `hashLen(): usize`, `maxOkmLen(): usize` | Runtime hash algorithm selector for generic HKDF functions. |

### Algorithm Classes

| Class | Methods | Description |
| --- | --- | --- |
| `HkdfSha256` | `extract(salt: []u8, ikm: []u8): HkdfSha256`, `fromPrk(prk_in: []u8): HkdfSha256?`, `expand(info: []u8, okm: []u8): bool`, `expandMultiInfo(infos: &[]string, okm: []u8): bool`, `expandLabel(label: &[]u8, context: &[]u8, okm: []u8): bool`, `verifyPrk(expected: &[]u8): bool` | RFC 5869 HKDF-SHA256 with `prk: [32]u8`, multi-part info, TLS 1.3 label expansion, and constant-time PRK verification. |
| `HkdfSha384` | `extract(salt: &[]u8, ikm: &[]u8): HkdfSha384`, `fromPrk(prk_in: &[]u8): HkdfSha384?`, `expand(info: &[]u8, okm: []u8): bool`, `verifyPrk(expected: &[]u8): bool` | RFC 5869 HKDF-SHA384 with `prk: [48]u8`. |
| `HkdfSha512` | `extract(salt: &[]u8, ikm: &[]u8): HkdfSha512`, `fromPrk(prk_in: &[]u8): HkdfSha512?`, `expand(info: &[]u8, okm: []u8): bool`, `verifyPrk(expected: &[]u8): bool` | RFC 5869 HKDF-SHA512 with `prk: [64]u8`. |

### One-Shot & Algorithm-Dispatched Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hkdfSha256` | `(salt: []u8, ikm: []u8, info: []u8, okm: []u8): bool` | One-shot Extract + Expand with SHA-256. |
| `hkdfSha384` | `(salt: &[]u8, ikm: &[]u8, info: &[]u8, okm: []u8): bool` | One-shot Extract + Expand with SHA-384. |
| `hkdfSha512` | `(salt: &[]u8, ikm: &[]u8, info: &[]u8, okm: []u8): bool` | One-shot Extract + Expand with SHA-512. |
| `hkdfExtract` | `(alg: HkdfHashAlg, salt: &[]u8, ikm: &[]u8, prk_out: []u8): usize` | Generic Extract into `prk_out`; returns PRK bytes written (`0` if `prk_out` too small). |
| `hkdfExpand` | `(alg: HkdfHashAlg, prk: &[]u8, info: &[]u8, okm: []u8): bool` | Generic Expand from `prk` into `okm`. |
| `hkdfDerive` | `(alg: HkdfHashAlg, salt: &[]u8, ikm: &[]u8, info: &[]u8, okm: []u8): bool` | Generic one-shot Extract + Expand for any `HkdfHashAlg`. |

## Examples

### 1. Extract-Then-Expand with `HkdfSha256`

```typescript
import { HkdfSha256 } from "hkdf";

fn main() {
    let salt: [4]u8 = [0x00, 0x01, 0x02, 0x03];
    let ikm: [4]u8 = [0x0b, 0x0b, 0x0b, 0x0b];
    let info: [4]u8 = [0xf0, 0xf1, 0xf2, 0xf3];
    let okm: [42]u8 = undefined;

    let hk = HkdfSha256.extract(salt[0..], ikm[0..]);
    let ok = hk.expand(info[0..], okm[0..]); // true
}
```

### 2. TLS 1.3 Label Expansion & Multi-Info

```typescript
import { HkdfSha256 } from "hkdf";

fn deriveTrafficKey(prk_bytes: []u8, key_out: []u8): bool {
    if (let hk = HkdfSha256.fromPrk(prk_bytes)) {
        let label = "key";
        let ctx: [0]u8 = [];
        return hk.expandLabel(label, ctx[0..], key_out);
    }
    return false;
}
```

### 3. Algorithm-Agnostic Derivation (`SHA-384` / `SHA-512`)

```typescript
import { HkdfHashAlg, hkdfDerive } from "hkdf";

fn main() {
    let salt: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
    let ikm: [16]u8 = [0; 16];
    let info: [4]u8 = [0x61, 0x70, 0x70, 0x31];
    let okm: [64]u8 = undefined;

    let ok = hkdfDerive(HkdfHashAlg.Sha512, salt[0..], ikm[0..], info[0..], okm[0..]);
}
```

## Differences from Upstream & Highlights

- **Zero Heap Allocation**: All PRK and OKM buffers live on stack or caller-provided slices (`[32]u8` / `[48]u8` / `[64]u8` and `okm: []u8`).
- **Built-In TLS 1.3 `expandLabel`**: `HkdfSha256.expandLabel` encodes `HkdfLabel` (`length || "tls13 " + label || context`) directly without temporary heap buffers.
- **Constant-Time PRK Check**: `verifyPrk` compares expected PRK bytes in constant time to prevent timing leaks.
