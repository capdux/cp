// Source: cp/cap/test/test_postcard_production_deep.cp (Production deep tests for cap:postcard)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    zigzagEncodeI16,
    zigzagDecodeI16,
    zigzagEncodeI32,
    zigzagDecodeI32,
    zigzagEncodeI64,
    zigzagDecodeI64,
    PostcardWriter,
    PostcardReader,
    cobsEncode,
    cobsEncodeFramed,
    cobsDecode,
} from "cap:postcard";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. ZigZag and VarU16/32/64 + VarI16/32/64 extreme value roundtrip (MIN/MAX/-1/0/1)", () => {
    expect_eq(zigzagEncodeI32(0), 0);
    expect_eq(zigzagEncodeI32(-1), 1);
    expect_eq(zigzagEncodeI32(1), 2);
    expect_eq(zigzagDecodeI32(zigzagEncodeI32(-2147483648)), -2147483648);
    expect_eq(zigzagDecodeI32(zigzagEncodeI32(2147483647)), 2147483647);

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = PostcardWriter.init(buf[0..]);

    const i16_min: i16 = -32768;
    const i16_max: i16 = 32767;
    const i32_min: i32 = -2147483648;
    const i32_max: i32 = 2147483647;
    const i64_min: i64 = -9223372036854775807 - 1;
    const i64_max: i64 = 9223372036854775807;

    expect_eq(w.writeVarU16(0), true);
    expect_eq(w.writeVarU16(65535), true);
    expect_eq(w.writeVarU32(0xFFFF_FFFF), true);
    expect_eq(w.writeVarU64(0xFFFF_FFFF_FFFF_FFFF), true);
    expect_eq(w.writeVarI16(i16_min), true);
    expect_eq(w.writeVarI16(i16_max), true);
    expect_eq(w.writeVarI16(-1), true);
    expect_eq(w.writeVarI32(i32_min), true);
    expect_eq(w.writeVarI32(i32_max), true);
    expect_eq(w.writeVarI64(i64_min), true);
    expect_eq(w.writeVarI64(i64_max), true);
    expect_eq(w.writeVarI64(0), true);
    expect_eq(w.writeVarI64(1), true);

    let r = PostcardReader.init(w.written());
    expect_eq(r.readVarU16() ?? 1, 0);
    expect_eq(r.readVarU16() ?? 0, 65535);
    expect_eq(r.readVarU32() ?? 0, 0xFFFF_FFFF);
    expect_eq(r.readVarU64() ?? 0, 0xFFFF_FFFF_FFFF_FFFF);
    expect_eq(r.readVarI16() ?? 0, i16_min);
    expect_eq(r.readVarI16() ?? 0, i16_max);
    expect_eq(r.readVarI16() ?? 0, -1);
    expect_eq(r.readVarI32() ?? 0, i32_min);
    expect_eq(r.readVarI32() ?? 0, i32_max);
    expect_eq(r.readVarI64() ?? 0, i64_min);
    expect_eq(r.readVarI64() ?? 0, i64_max);
    expect_eq(r.readVarI64() ?? 9, 0);
    expect_eq(r.readVarI64() ?? 0, 1);
    expect_eq(r.isEof(), true);
});

test("2. Composite Struct + Option + Enum + String + Bytes + F32/F64 message roundtrip", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = PostcardWriter.init(buf[0..]);

    expect_eq(w.writeBool(true), true);
    expect_eq(w.writeU8(0x7F), true);
    expect_eq(w.writeI8(-128), true);
    expect_eq(w.writeVariantIdx(300), true);
    expect_eq(w.writeOptionTag(false), true);
    expect_eq(w.writeOptionTag(true), true);
    expect_eq(w.writeSeqLen(2), true);
    expect_eq(w.writeString("sensor-A"), true);
    expect_eq(w.writeBytes("raw\x00bin"), true);
    expect_eq(w.writeF32(23.5), true);
    expect_eq(w.writeF64(-98.125), true);

    let r = PostcardReader.init(w.written());
    expect_eq(r.readBool() ?? false, true);
    expect_eq(r.readU8() ?? 0, 0x7F);
    expect_eq(r.readI8() ?? 0, -128);
    expect_eq(r.readVariantIdx() ?? 0, 300);
    expect_eq(r.readOptionTag() ?? true, false);
    expect_eq(r.readOptionTag() ?? false, true);
    expect_eq(r.readSeqLen() ?? 0, 2);

    const s = r.readString() ?? unreachable;
    expect_eq(sliceEq(s, "sensor-A"), true);
    const b = r.readBytes() ?? unreachable;
    expect_eq(b.len, 7);
    expect_eq(b[3], 0);

    const fv = r.readF32() ?? 0.0;
    expect_eq(fv > 23.49 && fv < 23.51, true);
    const dv = r.readF64() ?? 0.0;
    expect_eq(dv > -98.126 && dv < -98.124, true);
    expect_eq(r.isEof(), true);
});

test("3. COBS roundtrip with consecutive 0x00 bytes, empty input, and >254B non-zero block", () => {
    // A: Consecutive zeros [0x00, 0x00, 0x11, 0x00, 0x22, 0x33, 0x00]
    const src1: [7]u8 = [0x00, 0x00, 0x11, 0x00, 0x22, 0x33, 0x00];
    let enc1: [16]u8 = std.mem.zeroes([16]u8);
    let dec1: [16]u8 = std.mem.zeroes([16]u8);
    const enc1_len = cobsEncodeFramed(src1[0..], enc1[0..]) ?? unreachable;
    expect_eq(enc1[enc1_len - 1], 0x00);
    // Body before delimiter must contain zero 0x00 bytes
    let i: usize = 0;
    while (i < enc1_len - 1) {
        expect_eq(enc1[i] != 0, true);
        i += 1;
    }
    const dec1_len = cobsDecode(enc1[0..(enc1_len - 1)], dec1[0..]) ?? unreachable;
    expect_eq(sliceEq(dec1[0..dec1_len], src1[0..]), true);

    // B: Empty input encodes to [0x01] and decodes to length 0
    let enc_empty: [4]u8 = std.mem.zeroes([4]u8);
    const ee_len = cobsEncode(src1[0..0], enc_empty[0..]) ?? unreachable;
    expect_eq(ee_len, 1);
    expect_eq(enc_empty[0], 1);
    const de_len = cobsDecode(enc_empty[0..ee_len], dec1[0..]) ?? unreachable;
    expect_eq(de_len, 0);

    // C: 300-byte non-zero block (> 254B COBS chunk limit)
    let big: [300]u8 = std.mem.zeroes([300]u8);
    let k: usize = 0;
    while (k < 300) {
        big[k] = ((k % 250) + 1) as u8;
        k += 1;
    }
    let big_enc: [320]u8 = std.mem.zeroes([320]u8);
    let big_dec: [320]u8 = std.mem.zeroes([320]u8);
    const be_len = cobsEncode(big[0..], big_enc[0..]) ?? unreachable;
    expect_eq(be_len, 302); // 1 overhead byte per 254 bytes => 2 overhead bytes
    expect_eq(big_enc[0], 0xFF);
    const bd_len = cobsDecode(big_enc[0..be_len], big_dec[0..]) ?? unreachable;
    expect_eq(sliceEq(big_dec[0..bd_len], big[0..]), true);
});

test("4. Varint bit-width overflow, invalid bool, and invalid COBS input rejection", () => {
    // VarU16 overflow: 3rd byte > 0x03 (0x04)
    const ov16: [3]u8 = [0x80, 0x80, 0x04];
    let r16 = PostcardReader.init(ov16[0..]);
    expect_eq(r16.readVarU16() == null, true);

    // VarU32 overflow: 5th byte > 0x0F (0x10)
    const ov32: [5]u8 = [0x80, 0x80, 0x80, 0x80, 0x10];
    let r32 = PostcardReader.init(ov32[0..]);
    expect_eq(r32.readVarU32() == null, true);

    // VarU64 overflow: 10th byte > 0x01 (0x02)
    const ov64: [10]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02];
    let r64 = PostcardReader.init(ov64[0..]);
    expect_eq(r64.readVarU64() == null, true);

    // Invalid bool byte
    const bad_bool: [1]u8 = [0x05];
    let rb = PostcardReader.init(bad_bool[0..]);
    expect_eq(rb.readBool() == null, true);

    // COBS decode rejects embedded 0x00 and out-of-bounds code byte
    let out: [8]u8 = std.mem.zeroes([8]u8);
    const cobs_zero: [3]u8 = [0x03, 0x11, 0x00];
    expect_eq(cobsDecode(cobs_zero[0..], out[0..]) == null, true);
    const cobs_oob: [2]u8 = [0x05, 0x11];
    expect_eq(cobsDecode(cobs_oob[0..], out[0..]) == null, true);
});

test("5. Atomic buffer overflow rollback on writer and truncated input handling on reader", () => {
    // 3-byte buffer cannot hold 1B varint(5) + 5B payload ("hello") => rollback pos to 0
    let small: [3]u8 = std.mem.zeroes([3]u8);
    let w = PostcardWriter.init(small[0..]);
    expect_eq(w.writeString("hello"), false);
    expect_eq(w.written().len, 0);
    expect_eq(w.remaining(), 3);

    // Truncated varint (continuation bit set on last byte)
    const trunc_v: [2]u8 = [0x80, 0x80];
    let rv = PostcardReader.init(trunc_v[0..]);
    expect_eq(rv.readVarU32() == null, true);

    // Truncated bytes (length=10, only 2 bytes present)
    const trunc_b: [3]u8 = [10, 0xAA, 0xBB];
    let rby = PostcardReader.init(trunc_b[0..]);
    expect_eq(rby.readBytes() == null, true);
});
