# postcard@1.1.3 (Agent Quickref)

> Zero-heap compact binary wire protocol codecs (Varint, ZigZag, and COBS framing) | License: MIT
> Import: `import { ... } from "postcard";` | Full types: `src/index.d.cp`

## Functions
- `export fn pcrdU64Max(): u64;`
- `export fn encodeVarintU64(val: u64, out: []u8): usize;`
- `export fn decodeVarintU64(data: &[]u8): VarintU64Res;`
- `export fn zigzagEncode(val: i64): u64;`
- `export fn zigzagDecode(val: u64): i64;`
- `export fn encodeVarintI64(val: i64, out: []u8): usize;`
- `export fn decodeVarintI64(data: &[]u8): VarintI64Res;`
- `export fn cobsEncode(src: &[]u8, out: []u8): usize;`
- `export fn cobsDecode(src: &[]u8, out: []u8): i64;`
