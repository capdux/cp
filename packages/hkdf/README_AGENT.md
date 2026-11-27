# hkdf — AI Agent Quick Reference

- **Package**: `hkdf` (`v1.0.0`) | **Source**: crates.io `hkdf` | **Import**: `import { ... } from "hkdf";`
- **Role**: Zero-heap RFC 5869 HKDF-SHA256 key derivation (`extract`, `fromPrk`, `expand`, one-shot `hkdfSha256`).

## Core Exports (`src/index.d.cp`)

```cp
class HkdfSha256 {
    prk: [32]u8,
    fn extract(salt: []u8, ikm: []u8): HkdfSha256;
    fn fromPrk(prk_in: []u8): HkdfSha256?;
    fn expand(info: []u8, okm: []u8): bool;
}

fn hkdfSha256(salt: []u8, ikm: []u8, info: []u8, okm: []u8): bool;
```

## Minimal Usage

```cp
import { HkdfSha256, hkdfSha256 } from "hkdf";

let salt: [16]u8 = [1; 16];
let ikm: [32]u8 = [2; 32];
let info: [4]u8 = ['a', 'p', 'p', '1'];
let okm: [32]u8 = undefined;

// One-shot
const ok = hkdfSha256(salt[0..16], ikm[0..32], info[0..4], okm[0..32]);

// Extract once, expand multiple times
const hk = HkdfSha256.extract(salt[0..16], ikm[0..32]);
const ok2 = hk.expand(info[0..4], okm[0..32]);
```

## Pitfalls & Constraints

1. **Max OKM length**: RFC 5869 caps `okm.len <= 8160` (`255 * 32` bytes). Passing a larger `okm` slice returns `false` without writing.
2. **Empty `salt` / `info` / `okm`**: `salt.len == 0` uses a 32-byte zero salt per RFC 5869; `info.len == 0` is valid; `okm.len == 0` returns `true` immediately.
3. **`fromPrk` validation**: Returns `null` if `prk_in.len < 32`; uses the first 32 bytes when `>= 32`.
4. **Reusable `HkdfSha256`**: `expand` does not mutate `prk` — you can safely call `hk.expand(...)` multiple times with distinct `info` labels to derive independent subkeys.
5. **Dependencies**: Built on `hmac` (`HmacSha256`), zero heap allocation throughout.
