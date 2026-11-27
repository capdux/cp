import { test, expect_eq } from "cap:test";
import {
    WIRE_VARINT,
    WIRE_I64,
    WIRE_LEN,
    WIRE_I32,
    FieldTag,
    BytesField,
    Sint64Decode,
    encodeTag,
    decodeTag,
    encodeVarintField,
    encodeSint64Field,
    decodeVarintValue,
    decodeSint64Value,
    encodeFixed32Field,
    decodeFixed32,
    encodeFixed64Field,
    decodeFixed64,
    encodeBytesField,
    decodeBytesField,
    skipField,
} from "../../packages/google-protobuf/src/index.cp";
import { U64Decode } from "../../packages/unsigned-varint/src/index.cp";

fn sliceEq(buf: []const u8, start: usize, len: usize, expected: string): bool {
    if (len != expected.len || start + len > buf.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (buf[start + i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/prost-wire — Tag encode/decode & validation", () => {
        let buf: [16]u8 = [0; 16];
        // Field 1, varint -> (1 << 3) | 0 = 0x08
        let n1: usize = encodeTag(1, WIRE_VARINT, buf[0..16]);
        expect_eq(n1, 1 as usize);
        expect_eq(buf[0], 0x08 as u8);

        let t1: FieldTag = decodeTag(buf[0..n1]);
        expect_eq(t1.ok, true);
        expect_eq(t1.field_number, 1 as u32);
        expect_eq(t1.wire_type, WIRE_VARINT);
        expect_eq(t1.bytes_read, 1 as usize);

        // Large field number 2048, WIRE_LEN
        let n2: usize = encodeTag(2048, WIRE_LEN, buf[0..16]);
        expect_eq(n2 > 1, true);
        let t2: FieldTag = decodeTag(buf[0..n2]);
        expect_eq(t2.ok, true);
        expect_eq(t2.field_number, 2048 as u32);
        expect_eq(t2.wire_type, WIRE_LEN);

        // Invalid field number 0 & invalid wire type 7 rejected
        expect_eq(encodeTag(0, WIRE_VARINT, buf[0..16]), 0 as usize);
        expect_eq(encodeTag(1, 7, buf[0..16]), 0 as usize);
    });

    test("2. @cap/prost-wire — Varint & Sint64 ZigZag negative round-trip", () => {
        let buf: [32]u8 = [0; 32];
        // Protobuf canonical example: field 1, varint 150 -> 08 96 01
        let n1: usize = encodeVarintField(1, 150, buf[0..32]);
        expect_eq(n1, 3 as usize);
        expect_eq(buf[0], 0x08 as u8);
        expect_eq(buf[1], 0x96 as u8);
        expect_eq(buf[2], 0x01 as u8);

        let t1: FieldTag = decodeTag(buf[0..n1]);
        let v1: U64Decode = decodeVarintValue(buf[0..n1], t1.bytes_read);
        expect_eq(v1.ok, true);
        expect_eq(v1.value, 150 as u64);

        // Sint64 negative value -42 via @cap/zigzag + @cap/varint
        let n2: usize = encodeSint64Field(2, -42, buf[0..32]);
        expect_eq(n2 > 0, true);
        let t2: FieldTag = decodeTag(buf[0..n2]);
        expect_eq(t2.field_number, 2 as u32);
        let s2: Sint64Decode = decodeSint64Value(buf[0..n2], t2.bytes_read);
        expect_eq(s2.ok, true);
        expect_eq(s2.value, -42 as i64);
    });

    test("3. @cap/prost-wire — Fixed32 & Fixed64 little-endian codec", () => {
        let buf: [32]u8 = [0; 32];
        let n32: usize = encodeFixed32Field(3, 0x12345678, buf[0..32]);
        expect_eq(n32, 5 as usize);
        let t32: FieldTag = decodeTag(buf[0..n32]);
        expect_eq(t32.field_number, 3 as u32);
        expect_eq(t32.wire_type, WIRE_I32);
        expect_eq(decodeFixed32(buf[0..n32], t32.bytes_read), 0x12345678 as u32);

        let v64: u64 = ((0x01020304 as u64) << 32) | (0x05060708 as u64);
        let n64: usize = encodeFixed64Field(4, v64, buf[0..32]);
        expect_eq(n64, 9 as usize);
        let t64: FieldTag = decodeTag(buf[0..n64]);
        expect_eq(t64.field_number, 4 as u32);
        expect_eq(t64.wire_type, WIRE_I64);
        expect_eq(decodeFixed64(buf[0..n64], t64.bytes_read), v64);
    });

    test("4. @cap/prost-wire — Length-delimited string field codec", () => {
        let buf: [64]u8 = [0; 64];
        // Protobuf canonical example: field 2, "testing" -> 12 07 74 65 73 74 69 6e 67
        let n: usize = encodeBytesField(2, "testing", buf[0..64]);
        expect_eq(n, 9 as usize);
        expect_eq(buf[0], 0x12 as u8);
        expect_eq(buf[1], 0x07 as u8);

        let t: FieldTag = decodeTag(buf[0..n]);
        expect_eq(t.ok, true);
        expect_eq(t.field_number, 2 as u32);
        expect_eq(t.wire_type, WIRE_LEN);

        let bf: BytesField = decodeBytesField(buf[0..n], t.bytes_read);
        expect_eq(bf.ok, true);
        expect_eq(bf.len, 7 as usize);
        expect_eq(bf.next_pos, n);
        expect_eq(sliceEq(buf[0..n], bf.start, bf.len, "testing"), true);
    });

    test("5. @cap/prost-wire — multi-field message chaining & skipField", () => {
        let msg: [128]u8 = [0; 128];
        let pos: usize = 0;
        pos = pos + encodeVarintField(1, 999, msg[pos..128]);
        pos = pos + encodeFixed64Field(2, 123456789, msg[pos..128]);
        pos = pos + encodeBytesField(3, "unknown-payload", msg[pos..128]);
        pos = pos + encodeFixed32Field(4, 0xCAFEBABE, msg[pos..128]);
        pos = pos + encodeSint64Field(5, -100000, msg[pos..128]);

        // Walk all 5 fields using decodeTag + skipField
        let cursor: usize = 0;
        let seen_count: usize = 0;
        let last_fnum: u32 = 0;
        while (cursor < pos) {
            let tag: FieldTag = decodeTag(msg[cursor..pos]);
            expect_eq(tag.ok, true);
            last_fnum = tag.field_number;
            let next: i64 = skipField(msg[0..pos], cursor + tag.bytes_read, tag.wire_type);
            expect_eq(next > 0, true);
            cursor = next as usize;
            seen_count = seen_count + 1;
        }
        expect_eq(seen_count, 5 as usize);
        expect_eq(last_fnum, 5 as u32);
        expect_eq(cursor, pos);
    });

    test("6. @cap/prost-wire — truncated buffer & invalid wire_type rejection", () => {
        // Truncated WIRE_LEN claiming 10 bytes with only 2 available
        let trunc_len: [3]u8 = [10, 65, 66];
        let bf: BytesField = decodeBytesField(trunc_len[0..3], 0);
        expect_eq(bf.ok, false);
        expect_eq(skipField(trunc_len[0..3], 0, WIRE_LEN), -1 as i64);

        // Truncated fixed32 & fixed64
        let short2: [2]u8 = [1, 2];
        expect_eq(skipField(short2[0..2], 0, WIRE_I32), -1 as i64);
        expect_eq(skipField(short2[0..2], 0, WIRE_I64), -1 as i64);

        // Invalid wire type 6
        expect_eq(skipField(short2[0..2], 0, 6), -1 as i64);

        // Invalid tag byte with wire_type=7: (1<<3)|7 = 15
        let bad_tag: [1]u8 = [15];
        expect_eq(decodeTag(bad_tag[0..1]).ok, false);
    });
}
