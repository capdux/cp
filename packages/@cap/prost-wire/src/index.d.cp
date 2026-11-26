// Type definitions for CP

import { U64Decode, encodeUleb128, decodeUleb128, zigzagEncode, zigzagDecode } from "../../varint/src/index.cp";
export class FieldTag {
    field_number: u32,
    wire_type: u8,
    bytes_read: usize,
    ok: bool,
}

export class BytesField {
    start: usize,
    len: usize,
    next_pos: usize,
    ok: bool,
}

export class Sint64Decode {
    value: i64,
    bytes_read: usize,
    ok: bool,
}

export fn isValidWireType(wt: u8): bool;
export fn encodeTag(field_number: u32, wire_type: u8, out: []u8): usize;
export fn decodeTag(buf: &[]u8): FieldTag;
export fn encodeVarintField(field_number: u32, val: u64, out: []u8): usize;
export fn encodeSint64Field(field_number: u32, val: i64, out: []u8): usize;
export fn decodeVarintValue(buf: &[]u8, pos: usize): U64Decode;
export fn decodeSint64Value(buf: &[]u8, pos: usize): Sint64Decode;
export fn encodeFixed32Field(field_number: u32, val: u32, out: []u8): usize;
export fn decodeFixed32(buf: &[]u8, pos: usize): u32;
export fn encodeFixed64Field(field_number: u32, val: u64, out: []u8): usize;
export fn decodeFixed64(buf: &[]u8, pos: usize): u64;
export fn encodeBytesField(field_number: u32, payload: string, out: []u8): usize;
export fn decodeBytesField(buf: &[]u8, pos: usize): BytesField;
export fn skipField(buf: &[]u8, pos: usize, wire_type: u8): i64;
