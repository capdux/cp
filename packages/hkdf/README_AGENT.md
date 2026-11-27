# hkdf (AI Agent Quick Ref)

Zero-heap RFC 5869 HKDF (SHA-256 / SHA-384 / SHA-512) with multi-info and TLS 1.3 `expandLabel`.

## Import
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

## API Signatures
- Consts: `HKDF_HASH_LEN=32`, `HKDF_MAX_OKM_LEN=8160`, `HKDF_SHA384_HASH_LEN=48`, `HKDF_SHA384_MAX_OKM_LEN=12240`, `HKDF_SHA512_HASH_LEN=64`, `HKDF_SHA512_MAX_OKM_LEN=16320`
- `enum HkdfHashAlg { Sha256, Sha384, Sha512 }` -> `hashLen(): usize`, `maxOkmLen(): usize`
- `class HkdfSha256 { prk: [32]u8 }`:
  - `extract(salt: []u8, ikm: []u8): HkdfSha256`, `fromPrk(prk_in: []u8): HkdfSha256?`
  - `expand(info: []u8, okm: []u8): bool`, `expandMultiInfo(infos: &[]string, okm: []u8): bool`
  - `expandLabel(label: &[]u8, context: &[]u8, okm: []u8): bool`, `verifyPrk(expected: &[]u8): bool`
- `class HkdfSha384 { prk: [48]u8 }` / `class HkdfSha512 { prk: [64]u8 }`:
  - `extract(salt: &[]u8, ikm: &[]u8)`, `fromPrk(prk_in: &[]u8): Self?`, `expand(info: &[]u8, okm: []u8): bool`, `verifyPrk(expected: &[]u8): bool`
- Functions: `hkdfSha256([]u8, []u8, []u8, []u8): bool`, `hkdfSha384/hkdfSha512(&[]u8, &[]u8, &[]u8, []u8): bool`, `hkdfExtract(HkdfHashAlg, &[]u8, &[]u8, []u8): usize`, `hkdfExpand(HkdfHashAlg, &[]u8, &[]u8, []u8): bool`, `hkdfDerive(HkdfHashAlg, &[]u8, &[]u8, &[]u8, []u8): bool`

## Minimal Example
```typescript
import { HkdfHashAlg, hkdfDerive } from "hkdf";

fn deriveKey(salt: &[]u8, ikm: &[]u8, info: &[]u8, out: []u8): bool {
    return hkdfDerive(HkdfHashAlg.Sha256, salt, ikm, info, out);
}
```

## Pitfalls & Borrowing Rules
- Notice slice mutability difference: `HkdfSha256.extract/fromPrk/expand` and `hkdfSha256` take `[]u8` (mutable slice), whereas `HkdfSha384`/`HkdfSha512` and `hkdfExtract`/`hkdfExpand`/`hkdfDerive` take read-only `&[]u8` inputs; prefer `hkdfDerive` if passing `&[]u8` for SHA-256.
- `fromPrk` returns an optional (`HkdfSha*?`); returns `null` when `prk_in.len < hashLen`.
- `expand` / `hkdfDerive` return `false` if `okm.len > 255 * hashLen` (RFC 5869 limit).
- Empty salt (`salt.len == 0`) is automatically treated as `hashLen` zero bytes per RFC 5869.
- `hkdfExtract` returns `0` if `prk_out.len < alg.hashLen()`.
