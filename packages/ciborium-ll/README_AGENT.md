# ciborium-ll@1.0.0 (Agent Quickref)

> Zero-heap RFC 8949 low-level streaming CBOR encoder and decoder primitives in pure CP | License: Apache-2.0
> Import: `import { ... } from "ciborium-ll";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum CborHeaderKind`

## Functions
- `export fn cblMaxI64AsU64(): u64;`
- `export fn cblReadBe16(data: &[]u8, pos: usize): u16;`
- `export fn cblReadBe32(data: &[]u8, pos: usize): u32;`
- `export fn cblReadBe64(data: &[]u8, pos: usize): u64;`
- `export fn cblDecodeHalf(bits: u16): f64;`
- `export fn cblArgSize(val: u64): usize;`
- `export fn cblWriteHead(buf: []u8, pos: usize, major: u8, val: u64): usize;`
