# bendy@1.0.0 (Agent Quickref)

> BitTorrent BEP-3 deterministic binary Bencode streaming encoder, zero-copy decoder, validator, and dict lookup. | License: MIT
> Import: `import { ... } from "bendy";` | Full types: `src/index.d.cp`

## Functions
- `export fn peekKind(src: &[]u8, pos: usize): u8;`
- `export fn decodeInt(src: &[]u8, pos: usize): BencodeIntResult;`
- `export fn decodeBytes(src: &[]u8, pos: usize): BencodeBytesResult;`
- `export fn compareBytes(a: &[]u8, b: &[]u8): i32;`
- `export fn parseValueEnd(src: &[]u8, pos: usize, depth: usize): i64;`
- `export fn validate(src: &[]u8): bool;`
- `export fn dictLookup(src: &[]u8, key: &[]u8): BencodeBytesResult;`
