# ed25519@1.0.0 (Agent Quickref)

> Zero-heap RFC 8032 Ed25519 signature & key containers, canonical scalar S < L validation, RFC 8410 SPKI/PKCS#8 DER and hex codecs in pure CP | License: Apache-2.0 OR MIT
> Import: `import { ... } from "ed25519";` | Full types: `src/index.d.cp`

## Functions
- `export fn ed25Zero32(): [32]u8;`
- `export fn ed25Zero64(): [64]u8;`
- `export fn ed25GroupOrderL(): [32]u8;`
- `export fn ed25SpkiPrefix(): [12]u8;`
- `export fn ed25Pkcs8Prefix(): [16]u8;`
- `export fn ed25HexNibble(ch: u8): i32;`
- `export fn ed25HexDigit(n: u8): u8;`
- `export fn ed25EncodeHex(src: &[]u8, out: []u8): usize;`
