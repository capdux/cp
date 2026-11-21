// Source: cp/cap/test/test_bincode_production_deep.cp (Production deep tests for cap:bincode)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BINCODE_VARINT_U16_TAG,
    BINCODE_VARINT_U32_TAG,
    BINCODE_VARINT_U64_TAG,
    BincodeEndian,
    BincodeIntEncoding,
    BincodeConfig,
    BincodeWriter,
    BincodeReader,
} from "cap:bincode";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. bincode v1 Legacy (LE Fixed) and Big-Endian Fixed full scalar roundtrip", () => {
    let buf_le: [64]u8 = std.mem.zeroes([64]u8);
    let wle = BincodeWriter.initLegacy(buf_le[0..]);
    expect_eq(wle.writeBool(true), true);
    expect_eq(wle.writeU8(0xAA), true);
    expect_eq(wle.writeI8(-12), true);
    expect_eq(wle.writeU16(0x1234), true);
    expect_eq(wle.writeI16(-3000), true);
    expect_eq(wle.writeU32(0x89ABCDEF), true);
    expect_eq(wle.writeI32(-99999), true);
    expect_eq(wle.writeU64(0x01020304_05060708), true);
    expect_eq(wle.writeI64(-1234567890123), true);
    expect_eq(wle.writeF32(1.25), true);
    expect_eq(wle.writeF64(-42.5), true);

    const enc_le = wle.written();
    expect_eq(enc_le[3], 0x34); // LE u16 low byte
    expect_eq(enc_le[4], 0x12); // LE u16 high byte

    let rle = BincodeReader.initLegacy(enc_le);
    expect_eq(rle.readBool() ?? false, true);
    expect_eq(rle.readU8() ?? 0, 0xAA);
    expect_eq(rle.readI8() ?? 0, -12);
    expect_eq(rle.readU16() ?? 0, 0x1234);
    expect_eq(rle.readI16() ?? 0, -3000);
    expect_eq(rle.readU32() ?? 0, 0x89ABCDEF);
    expect_eq(rle.readI32() ?? 0, -99999);
    expect_eq(rle.readU64() ?? 0, 0x01020304_05060708);
    expect_eq(rle.readI64() ?? 0, -1234567890123);
    const f32v = rle.readF32() ?? 0.0;
    expect_eq(f32v > 1.24 && f32v < 1.26, true);
    const f64v = rle.readF64() ?? 0.0;
    expect_eq(f64v > -42.51 && f64v < -42.49, true);
    expect_eq(rle.isEof(), true);

    // Big-Endian Fixed
    let buf_be: [16]u8 = std.mem.zeroes([16]u8);
    let wbe = BincodeWriter.init(buf_be[0..], BincodeConfig.bigEndianFixed());
    expect_eq(wbe.writeU16(0x1234), true);
    expect_eq(wbe.writeU32(0xAABBCCDD), true);
    const enc_be = wbe.written();
    expect_eq(enc_be[0], 0x12);
    expect_eq(enc_be[1], 0x34);
    let rbe = BincodeReader.init(enc_be, BincodeConfig.bigEndianFixed());
    expect_eq(rbe.readU16() ?? 0, 0x1234);
    expect_eq(rbe.readU32() ?? 0, 0xAABBCCDD);
    expect_eq(rbe.isEof(), true);
});

test("2. bincode v2 Standard Varint boundary values (0/250/251/65535/65536/u32::MAX/u64::MAX) and ZigZag signed ints", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = BincodeWriter.initStandard(buf[0..]);

    expect_eq(w.writeU64(0), true);                    // 1B: 0
    expect_eq(w.writeU64(250), true);                  // 1B: 250
    expect_eq(w.writeU64(251), true);                  // 3B: 0xFB + u16(251)
    expect_eq(w.writeU64(65535), true);                // 3B: 0xFB + u16(65535)
    expect_eq(w.writeU64(65536), true);                // 5B: 0xFC + u32(65536)
    expect_eq(w.writeU64(0xFFFF_FFFF), true);          // 5B: 0xFC + u32::MAX
    expect_eq(w.writeU64(0xFFFF_FFFF_FFFF_FFFF), true); // 9B: 0xFD + u64::MAX

    // Signed ZigZag values
    const i64_min: i64 = -9223372036854775807 - 1;
    expect_eq(w.writeI16(-1), true);
    expect_eq(w.writeI16(-32768), true);
    expect_eq(w.writeI32(-2147483648), true);
    expect_eq(w.writeI64(i64_min), true);
    expect_eq(w.writeI64(9223372036854775807), true);

    const enc = w.written();
    expect_eq(enc[0], 0);
    expect_eq(enc[1], 250);
    expect_eq(enc[2], BINCODE_VARINT_U16_TAG);
    expect_eq(enc[8], BINCODE_VARINT_U32_TAG);
    expect_eq(enc[18], BINCODE_VARINT_U64_TAG);

    let r = BincodeReader.initStandard(enc);
    expect_eq(r.readU64() ?? 9, 0);
    expect_eq(r.readU64() ?? 0, 250);
    expect_eq(r.readU64() ?? 0, 251);
    expect_eq(r.readU64() ?? 0, 65535);
    expect_eq(r.readU64() ?? 0, 65536);
    expect_eq(r.readU64() ?? 0, 0xFFFF_FFFF);
    expect_eq(r.readU64() ?? 0, 0xFFFF_FFFF_FFFF_FFFF);
    expect_eq(r.readI16() ?? 0, -1);
    expect_eq(r.readI16() ?? 0, -32768);
    expect_eq(r.readI32() ?? 0, -2147483648);
    expect_eq(r.readI64() ?? 0, i64_min);
    expect_eq(r.readI64() ?? 0, 9223372036854775807);
    expect_eq(r.isEof(), true);
});

test("3. Struct + Option + Enum + String + Bytes composite roundtrip in v2 Standard mode", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = BincodeWriter.initStandard(buf[0..]);

    expect_eq(w.writeVariantIdx(3), true);
    expect_eq(w.writeOptionTag(false), true);
    expect_eq(w.writeOptionTag(true), true);
    expect_eq(w.writeSeqLen(2), true);
    expect_eq(w.writeString("rust"), true);
    expect_eq(w.writeBytes("cp\x00v2"), true);

    let r = BincodeReader.initStandard(w.written());
    expect_eq(r.readVariantIdx() ?? 0, 3);
    expect_eq(r.readOptionTag() ?? true, false);
    expect_eq(r.readOptionTag() ?? false, true);
    expect_eq(r.readSeqLen() ?? 0, 2);
    const s = r.readString() ?? unreachable;
    const b = r.readBytes() ?? unreachable;
    expect_eq(sliceEq(s, "rust"), true);
    expect_eq(b.len, 5);
    expect_eq(b[2], 0);
    expect_eq(r.isEof(), true);
});

test("4. Invalid varint prefixes (0xFE/0xFF), out-of-range target bit width, and invalid bool rejected", () => {
    const tag_fe: [1]u8 = [0xFE];
    let rfe = BincodeReader.initStandard(tag_fe[0..]);
    expect_eq(rfe.readU64() == null, true);

    const tag_ff: [1]u8 = [0xFF];
    let rff = BincodeReader.initStandard(tag_ff[0..]);
    expect_eq(rff.readU64() == null, true);

    // Value 70000 encoded as 0xFC + u32(70000) must be rejected by readU16
    let buf: [16]u8 = std.mem.zeroes([16]u8);
    let w = BincodeWriter.initStandard(buf[0..]);
    expect_eq(w.writeU32(70000), true);
    let r16 = BincodeReader.initStandard(w.written());
    expect_eq(r16.readU16() == null, true);

    // Invalid bool 0x02 rejected
    const bad_bool: [1]u8 = [0x02];
    let rb = BincodeReader.initStandard(bad_bool[0..]);
    expect_eq(rb.readBool() == null, true);
});

test("5. Atomic destination buffer overflow rollback and truncated input handling", () => {
    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    let w = BincodeWriter.initStandard(tiny[0..]);
    // 1B varint len(5) + 5B payload = 6B > 4B => atomic rollback
    expect_eq(w.writeString("hello"), false);
    expect_eq(w.written().len, 0);
    expect_eq(w.remaining(), 4);

    // Truncated 0xFB (needs 2 more bytes, only 1 present)
    const trunc_v: [2]u8 = [BINCODE_VARINT_U16_TAG, 0x01];
    let rv = BincodeReader.initStandard(trunc_v[0..]);
    expect_eq(rv.readU16() == null, true);

    // Truncated bytes (claims 10 bytes, only 2 present)
    const trunc_b: [3]u8 = [10, 0xAA, 0xBB];
    let rby = BincodeReader.initStandard(trunc_b[0..]);
    expect_eq(rby.readBytes() == null, true);
});
