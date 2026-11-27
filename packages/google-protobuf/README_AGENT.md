# google-protobuf — AI Agent Quick Ref

Zero-allocation Protocol Buffers Wire Format encoder/decoder reusing varint (ULEB128 + ZigZag) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    FieldTag,
    BytesField,
    Sint64Decode,
    isValidWireType,
    encodeTag,
    decodeTag,
    encodeVarintField,
    encodeSint64Field,
} from "google-protobuf";
```

## Core Types & Constants

```cp
class FieldTag { field_number: u32, wire_type: u8, bytes_read: usize, ok: bool }
class BytesField { start: usize, len: usize, next_pos: usize, ok: bool }
class Sint64Decode { value: i64, bytes_read: usize, ok: bool }
```

## API Signatures

```cp
fn isValidWireType(wt: u8): bool;
fn encodeTag(field_number: u32, wire_type: u8, out: []u8): usize;
fn decodeTag(buf: &[]u8): FieldTag;
fn encodeVarintField(field_number: u32, val: u64, out: []u8): usize;
fn encodeSint64Field(field_number: u32, val: i64, out: []u8): usize;
fn decodeVarintValue(buf: &[]u8, pos: usize): U64Decode;
fn decodeSint64Value(buf: &[]u8, pos: usize): Sint64Decode;
fn encodeFixed32Field(field_number: u32, val: u32, out: []u8): usize;
fn decodeFixed32(buf: &[]u8, pos: usize): u32;
fn encodeFixed64Field(field_number: u32, val: u64, out: []u8): usize;
fn decodeFixed64(buf: &[]u8, pos: usize): u64;
fn encodeBytesField(field_number: u32, payload: string, out: []u8): usize;
fn decodeBytesField(buf: &[]u8, pos: usize): BytesField;
fn skipField(buf: &[]u8, pos: usize, wire_type: u8): i64;
```

## Minimal Usage

```cp
import { encodeBytesField } from "google-protobuf";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeBytesField(0, "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"google-protobuf"`; install via `cap add google-protobuf`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
