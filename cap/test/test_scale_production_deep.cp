// Source: cp/cap/test/test_scale_production_deep.cp (Production deep tests for cap:scale)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ScaleOptionBool,
    compactU64Len,
    ScaleWriter,
    ScaleReader,
} from "cap:scale";

test("1. Compact<u64> all 4 modes boundary values round-trip with exact Parity SCALE vectors", () => {
    const vals: [10]u64 = [
        0,
        1,
        42,
        63,
        64,
        16383,
        16384,
        1073741823,
        1073741824,
        0xFFFFFFFFFFFFFFFF,
    ];
    const exp_lens: [10]usize = [1, 1, 1, 1, 2, 2, 4, 4, 5, 9];

    let buf: [64]u8 = [0; 64];
    let w = ScaleWriter.init(buf[0..64]);

    let i: usize = 0;
    while (i < 10) {
        expect_eq(compactU64Len(vals[i]), exp_lens[i]);
        expect_eq(w.writeCompactU64(vals[i]), true);
        i += 1;
    }

    // Verify canonical byte representation of key vectors
    const encoded = w.written();
    expect_eq(encoded[0], 0x00); // 0
    expect_eq(encoded[1], 0x04); // 1
    expect_eq(encoded[2], 0xA8); // 42
    expect_eq(encoded[3], 0xFC); // 63
    expect_eq(encoded[4], 0x01); // 64 low
    expect_eq(encoded[5], 0x01); // 64 high

    let r = ScaleReader.init(encoded);
    i = 0;
    while (i < 10) {
        expect_eq(r.readCompactU64() ?? 999999, vals[i]);
        i += 1;
    }
    expect_eq(r.isEof(), true);
});

test("2. SCALE Option<bool> single-byte tri-state, Result, Enum, String, and Bytes round-trip", () => {
    let buf: [128]u8 = [0; 128];
    let w = ScaleWriter.init(buf[0..128]);

    expect_eq(w.writeOptionBool(null), true);
    expect_eq(w.writeOptionBool(true), true);
    expect_eq(w.writeOptionBool(false), true);
    expect_eq(w.writeResultTag(true), true); // Ok => 0x00
    expect_eq(w.writeResultTag(false), true); // Err => 0x01
    expect_eq(w.writeOptionTag(false), true);
    expect_eq(w.writeOptionTag(true), true);
    expect_eq(w.writeEnumTag(7), true);
    expect_eq(w.writeString("substrate"), true);
    const raw_bytes: [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF];
    expect_eq(w.writeBytes(raw_bytes[0..4]), true);

    const enc = w.written();
    // First 3 bytes are exact single-byte Option<bool> encodings
    expect_eq(enc[0], 0x00);
    expect_eq(enc[1], 0x01);
    expect_eq(enc[2], 0x02);
    expect_eq(enc[3], 0x00); // Ok
    expect_eq(enc[4], 0x01); // Err

    let r = ScaleReader.init(enc);
    expect_eq((r.readOptionBool() ?? ScaleOptionBool.some_true) == ScaleOptionBool.none, true);
    expect_eq((r.readOptionBool() ?? ScaleOptionBool.none) == ScaleOptionBool.some_true, true);
    expect_eq((r.readOptionBool() ?? ScaleOptionBool.none) == ScaleOptionBool.some_false, true);
    expect_eq(r.readResultTag() ?? false, true);
    expect_eq(r.readResultTag() ?? true, false);
    expect_eq(r.readOptionTag() ?? true, false);
    expect_eq(r.readOptionTag() ?? false, true);
    expect_eq(r.readEnumTag() ?? 0, 7);
    const s = r.readString() ?? unreachable;
    expect_eq(std.mem.eql(u8, s, "substrate"), true);
    const b = r.readBytes() ?? unreachable;
    expect_eq(std.mem.eql(u8, b, raw_bytes[0..4]), true);
    expect_eq(r.isEof(), true);
});

test("3. fixed-width signed and unsigned little-endian integers round-trip accurately", () => {
    let buf: [64]u8 = [0; 64];
    let w = ScaleWriter.init(buf[0..64]);

    expect_eq(w.writeBool(true), true);
    expect_eq(w.writeU8(0xFE), true);
    expect_eq(w.writeI8(-42), true);
    expect_eq(w.writeU16(0x1234), true);
    expect_eq(w.writeI16(-12345), true);
    expect_eq(w.writeU32(0x12345678), true);
    expect_eq(w.writeI32(-987654321), true);
    expect_eq(w.writeU64(0x0123456789ABCDEF), true);
    expect_eq(w.writeI64(-123456789012345), true);

    let r = ScaleReader.init(w.written());
    expect_eq(r.readBool() ?? false, true);
    expect_eq(r.readU8() ?? 0, 0xFE);
    expect_eq(r.readI8() ?? 0, -42);
    expect_eq(r.readU16() ?? 0, 0x1234);
    expect_eq(r.readI16() ?? 0, -12345);
    expect_eq(r.readU32() ?? 0, 0x12345678);
    expect_eq(r.readI32() ?? 0, -987654321);
    expect_eq(r.readU64() ?? 0, 0x0123456789ABCDEF);
    expect_eq(r.readI64() ?? 0, -123456789012345);
    expect_eq(r.remaining(), 0);
});

test("4. strict canonical validation rejects non-minimal Compact<u64> and invalid tag bytes", () => {
    // Non-canonical 2B encoding of 0: [0x01, 0x00]
    const nc_2b_zero: [2]u8 = [0x01, 0x00];
    let r1 = ScaleReader.init(nc_2b_zero[0..2]);
    expect_eq(r1.readCompactU64() == null, true);

    // Non-canonical 2B encoding of 63: ((63 << 2) | 1) = 0xFD, 0x00
    const nc_2b_63: [2]u8 = [0xFD, 0x00];
    let r2 = ScaleReader.init(nc_2b_63[0..2]);
    expect_eq(r2.readCompactU64() == null, true);

    // Non-canonical 4B encoding of 64: ((64 << 2) | 2) = 0x02, 0x01, 0x00, 0x00
    const nc_4b_64: [4]u8 = [0x02, 0x01, 0x00, 0x00];
    let r3 = ScaleReader.init(nc_4b_64[0..4]);
    expect_eq(r3.readCompactU64() == null, true);

    // Non-canonical BigInt with trailing zero byte: 5B header (0x07 => n=5) ending with 0x00
    const nc_big_tz: [6]u8 = [0x07, 0x00, 0x00, 0x00, 0x40, 0x00];
    let r4 = ScaleReader.init(nc_big_tz[0..6]);
    expect_eq(r4.readCompactU64() == null, true);

    // Non-canonical BigInt encoding value < 1073741824 (e.g. 0x3FFFFFFF)
    const nc_big_small: [5]u8 = [0x03, 0xFF, 0xFF, 0xFF, 0x3F];
    let r5 = ScaleReader.init(nc_big_small[0..5]);
    expect_eq(r5.readCompactU64() == null, true);

    // BigInt > 8 bytes (n = 9 => header (5 << 2) | 3 = 0x17) rejected for u64
    const big_9b: [10]u8 = [0x17, 1, 2, 3, 4, 5, 6, 7, 8, 9];
    let r6 = ScaleReader.init(big_9b[0..10]);
    expect_eq(r6.readCompactU64() == null, true);

    // Invalid bool (2), invalid Option<bool> (3), invalid Result tag (2)
    const bad_tags: [3]u8 = [2, 3, 2];
    let rt1 = ScaleReader.init(bad_tags[0..1]);
    expect_eq(rt1.readBool() == null, true);
    let rt2 = ScaleReader.init(bad_tags[1..2]);
    expect_eq(rt2.readOptionBool() == null, true);
    let rt3 = ScaleReader.init(bad_tags[2..3]);
    expect_eq(rt3.readResultTag() == null, true);
});

test("5. truncated reads and writer buffer exhaustion fail atomically without corruption", () => {
    const empty: [0]u8 = [];
    let re = ScaleReader.init(empty[0..0]);
    expect_eq(re.isEof(), true);
    expect_eq(re.readU8() == null, true);
    expect_eq(re.readCompactU64() == null, true);
    expect_eq(re.readBytes() == null, true);

    // Truncated bytes payload (claims 10 bytes via compact 40 = 0x28, only 3 bytes follow)
    const trunc_bytes: [4]u8 = [0x28, 'a', 'b', 'c'];
    let rb = ScaleReader.init(trunc_bytes[0..4]);
    expect_eq(rb.readBytes() == null, true);
    expect_eq(rb.remaining(), 4); // rolled back cleanly

    // Writer atomic failure on insufficient buffer
    let tiny: [3]u8 = [0; 3];
    let wt = ScaleWriter.init(tiny[0..3]);
    expect_eq(wt.writeU32(0x11223344), false);
    expect_eq(wt.written().len, 0);
    expect_eq(wt.writeBytes("abcd"), false); // needs 1 + 4 = 5 bytes
    expect_eq(wt.written().len, 0);
    expect_eq(wt.writeCompactU64(16384), false); // needs 4 bytes
    expect_eq(wt.written().len, 0);
});
