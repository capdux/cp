# object-hash@1.0.0 (Agent Quickref)

> Zero-allocation deterministic order-independent structured object hasher using FNV-1a 64-bit (npm object-hash parity) in pure CP | License: MIT
> Import: `import { ... } from "object-hash";` | Full types: `src/index.d.cp`

## Functions
- `export fn ohshFnvOffset(): u64;`
- `export fn ohshFnvPrime(): u64;`
- `export fn ohshFeedByte(h: u64, b: u8): u64;`
- `export fn ohshFeedBytes(h: u64, data: &[]u8): u64;`
- `export fn ohshCmpStr(a: string, b: string): i32;`
- `export fn ohshFieldLess(a: HashField, b: HashField): bool;`
- `export fn ohshHexDigit(nibble: u8): u8;`
- `export fn hashBytesFnv64(data: &[]u8): u64;`
- `export fn hashStringFnv64(s: string): u64;`
- `export fn hashFieldsFnv64(fields: &[]HashField): u64;`
- `export fn hashFieldsHex(fields: &[]HashField, out: []u8): usize;`
