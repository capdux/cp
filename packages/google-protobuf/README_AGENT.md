# google-protobuf@1.0.0 (Agent Quickref)

> Zero-allocation Protocol Buffers Wire Format encoder/decoder reusing @cap/varint (ULEB128 + ZigZag) for CP | License: MIT
> Import: `import { ... } from "google-protobuf";` | Full types: `src/index.d.cp`
> Deps: `unsigned-varint`

## Functions
- `export fn isValidWireType(wt: u8): bool;`
- `export fn encodeTag(field_number: u32, wire_type: u8, out: []u8): usize;`
- `export fn decodeTag(buf: &[]u8): FieldTag;`
- `export fn encodeVarintField(field_number: u32, val: u64, out: []u8): usize;`
- `export fn encodeSint64Field(field_number: u32, val: i64, out: []u8): usize;`
- `export fn decodeVarintValue(buf: &[]u8, pos: usize): U64Decode;`
- `export fn decodeSint64Value(buf: &[]u8, pos: usize): Sint64Decode;`
- `export fn encodeFixed32Field(field_number: u32, val: u32, out: []u8): usize;`
- `export fn decodeFixed32(buf: &[]u8, pos: usize): u32;`
- `export fn encodeFixed64Field(field_number: u32, val: u64, out: []u8): usize;`
- `export fn decodeFixed64(buf: &[]u8, pos: usize): u64;`
- `export fn encodeBytesField(field_number: u32, payload: string, out: []u8): usize;`
- `export fn decodeBytesField(buf: &[]u8, pos: usize): BytesField;`
- `export fn skipField(buf: &[]u8, pos: usize, wire_type: u8): i64;`
