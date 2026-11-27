# futoin-hkdf@1.0.0 (Agent Quickref)

> Pure CP zero-allocation RFC 5869 HKDF-SHA256 key derivation function (RustCrypto hkdf parity) | License: MIT
> Import: `import { ... } from "futoin-hkdf";` | Full types: `src/index.d.cp`
> Deps: `cookie-signature`

## Functions
- `export fn hkdfExtract(salt: []u8, ikm: []u8, out_prk32: []u8): bool;`
- `export fn hkdfExpand(prk32: []u8, info: []u8, out_okm: []u8): bool;`
- `export fn hkdfDerive(salt: []u8, ikm: []u8, info: []u8, out_okm: []u8): bool;`
