# hkdf@1.0.0 (Agent Quickref)

> Zero-heap RFC 5869 HKDF-SHA256 key derivation function (extract, expand, one-shot) (crates.io hkdf equivalent, reuses @cap/hmac-crate) | License: MIT
> Import: `import { ... } from "hkdf";` | Full types: `src/index.d.cp`
> Deps: `hmac`

## Functions
- `export fn hkdfSha256(salt: []u8, ikm: []u8, info: []u8, okm: []u8): bool;`
