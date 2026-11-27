# zigzag@1.0.0 (Agent Quickref)

> Zero-allocation 32-bit/64-bit ZigZag bijection and ZigZag-Varint streaming codec (Protobuf/Avro/Parquet/CBOR) in pure CP | License: MIT
> Import: `import { ... } from "zigzag";` | Full types: `src/index.d.cp`

## Functions
- `export fn encodeI32(n: i32): u32;`
- `export fn decodeI32(u: u32): i32;`
- `export fn encodeI64(n: i64): u64;`
- `export fn decodeI64(u: u64): i64;`
- `export fn encodeZigzagVarint(n: i64, out: []u8): usize;`
- `export fn decodeZigzagVarint(data: &[]u8, start: usize): VarintResult;`
