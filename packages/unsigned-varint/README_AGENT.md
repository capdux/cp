# unsigned-varint@1.0.0 (Agent Quickref)

> Zero-allocation 3-in-1 variable-length integer codec (ULEB128, SLEB128, Protobuf ZigZag, and QUIC RFC 9000 varint) for CP | License: MIT
> Import: `import { ... } from "unsigned-varint";` | Full types: `src/index.d.cp`

## Functions
- `export fn i64ToBits(v: i64): u64;`
- `export fn bitsToI64(u: u64): i64;`
- `export fn uleb128Size(v: u64): usize;`
- `export fn encodeUleb128(v: u64, out: []u8): usize?;`
- `export fn decodeUleb128(data: string): U64Decode;`
- `export fn zigzagEncode(v: i64): u64;`
- `export fn zigzagDecode(u: u64): i64;`
- `export fn encodeSleb128(v: i64, out: []u8): usize?;`
- `export fn decodeSleb128(data: string): I64Decode;`
- `export fn encodeZigzagVarint(v: i64, out: []u8): usize?;`
- `export fn decodeZigzagVarint(data: string): I64Decode;`
- `export fn encodeQuicVarint(v: u64, out: []u8): usize?;`
- `export fn decodeQuicVarint(data: string): U64Decode;`
