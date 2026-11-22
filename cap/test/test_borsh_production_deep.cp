// Source: cp/cap/test/test_borsh_production_deep.cp (Production deep tests for cap:borsh)

import std from "std";
import { test, expect_eq } from "cap:test";
import { BorshWriter, BorshReader } from "cap:binfmt";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. Full scalar, signed negative, bool, string, and fixed-bytes roundtrip", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = BorshWriter.init(buf[0..]);

    expect_eq(w.writeBool(true), true);
    expect_eq(w.writeBool(false), true);
    expect_eq(w.writeU8(0xAB), true);
    expect_eq(w.writeI8(-42), true);
    expect_eq(w.writeU16(0x1234), true);
    expect_eq(w.writeI16(-12345), true);
    expect_eq(w.writeU32(0xDEADBEEF), true);
    expect_eq(w.writeI32(-1_000_000), true);
    expect_eq(w.writeU64(0x01234567_89ABCDEF), true);
    expect_eq(w.writeI64(-9_000_000_000), true);
    expect_eq(w.writeString("hello-borsh"), true);
    const fixed_in: [4]u8 = [0xCA, 0xFE, 0xBA, 0xBE];
    expect_eq(w.writeFixedBytes(fixed_in[0..]), true);

    const encoded = w.written();
    // Verify little-endian encoding of u16 0x1234 at offset 4..6
    expect_eq(encoded[4], 0x34);
    expect_eq(encoded[5], 0x12);

    let r = BorshReader.init(encoded);
    expect_eq(r.readBool() ?? false, true);
    expect_eq(r.readBool() ?? true, false);
    expect_eq(r.readU8() ?? 0, 0xAB);
    expect_eq(r.readI8() ?? 0, -42);
    expect_eq(r.readU16() ?? 0, 0x1234);
    expect_eq(r.readI16() ?? 0, -12345);
    expect_eq(r.readU32() ?? 0, 0xDEADBEEF);
    expect_eq(r.readI32() ?? 0, -1_000_000);
    expect_eq(r.readU64() ?? 0, 0x01234567_89ABCDEF);
    expect_eq(r.readI64() ?? 0, -9_000_000_000);

    const s = r.readString() ?? unreachable;
    expect_eq(sliceEq(s, "hello-borsh"), true);
    const fx = r.readFixedBytes(4) ?? unreachable;
    expect_eq(fx[0], 0xCA);
    expect_eq(fx[3], 0xBE);
    expect_eq(r.isEof(), true);
});

test("2. Nested Option + Enum + Vec (sequence) deterministic encoding and decoding", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = BorshWriter.init(buf[0..]);

    // Enum variant 2
    expect_eq(w.writeEnumTag(2), true);
    // Option::None
    expect_eq(w.writeOptionTag(false), true);
    // Option::Some(Vec<String> with 2 items)
    expect_eq(w.writeOptionTag(true), true);
    expect_eq(w.writeSeqLen(2), true);
    expect_eq(w.writeString("near"), true);
    expect_eq(w.writeString("solana"), true);

    let r = BorshReader.init(w.written());
    expect_eq(r.readEnumTag() ?? 0, 2);
    expect_eq(r.readOptionTag() ?? true, false);
    expect_eq(r.readOptionTag() ?? false, true);
    expect_eq(r.readSeqLen() ?? 0, 2);
    const item0 = r.readString() ?? unreachable;
    const item1 = r.readString() ?? unreachable;
    expect_eq(sliceEq(item0, "near"), true);
    expect_eq(sliceEq(item1, "solana"), true);
    expect_eq(r.isEof(), true);
});

test("3. IEEE-754 f32/f64 roundtrip and strict Borsh NaN rejection on both write and read", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    let w = BorshWriter.init(buf[0..]);

    expect_eq(w.writeF32(-3.25), true);
    expect_eq(w.writeF64(2.718281828), true);

    // Construct quiet NaN f32 and f64 via @bitCast and verify writeF32/writeF64 reject them
    let nan32: f32 = 0.0;
    let nan64: f64 = 0.0;
    const qnan32_bits: u32 = 0x7FC0_0001;
    const qnan64_bits: u64 = 0x7FF8_0000_0000_0001;
    unsafe {
        nan32 = @bitCast(qnan32_bits);
        nan64 = @bitCast(qnan64_bits);
    }
    const pos_before = w.written().len;
    expect_eq(w.writeF32(nan32), false);
    expect_eq(w.writeF64(nan64), false);
    expect_eq(w.written().len, pos_before);

    let r = BorshReader.init(w.written());
    const fv = r.readF32() ?? 0.0;
    expect_eq(fv > -3.26 && fv < -3.24, true);
    const dv = r.readF64() ?? 0.0;
    expect_eq(dv > 2.71828 && dv < 2.71829, true);
    expect_eq(r.isEof(), true);

    // Raw NaN bytes rejected by readF32 and readF64
    const raw_nan32: [4]u8 = [0x01, 0x00, 0xC0, 0x7F];
    let rn32 = BorshReader.init(raw_nan32[0..]);
    expect_eq(rn32.readF32() == null, true);

    const raw_nan64: [8]u8 = [0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x7F];
    let rn64 = BorshReader.init(raw_nan64[0..]);
    expect_eq(rn64.readF64() == null, true);
});

test("4. Strict bool and Option tag validation rejects non-0/1 bytes", () => {
    const bad_bool: [1]u8 = [0x02];
    let r1 = BorshReader.init(bad_bool[0..]);
    expect_eq(r1.readBool() == null, true);

    const bad_opt: [1]u8 = [0xFF];
    let r2 = BorshReader.init(bad_opt[0..]);
    expect_eq(r2.readOptionTag() == null, true);
});

test("5. Destination buffer overflow and truncated input protection", () => {
    // 5-byte buffer cannot fit 4B len + 3B payload ("abc") atomically
    let tiny: [5]u8 = std.mem.zeroes([5]u8);
    let w = BorshWriter.init(tiny[0..]);
    expect_eq(w.writeBytes("abc"), false);
    expect_eq(w.written().len, 0);
    expect_eq(w.remaining(), 5);

    // Exact-fit write succeeds, subsequent write fails
    expect_eq(w.writeU32(99), true);
    expect_eq(w.writeU8(7), true);
    expect_eq(w.remaining(), 0);
    expect_eq(w.writeU8(1), false);

    // Truncated bytes length prefix (claims 100 bytes, only 2 present)
    const trunc_bytes: [6]u8 = [100, 0, 0, 0, 0xAA, 0xBB];
    let rb = BorshReader.init(trunc_bytes[0..]);
    expect_eq(rb.readBytes() == null, true);

    // Truncated u64 (only 6 bytes)
    let r64 = BorshReader.init(trunc_bytes[0..]);
    expect_eq(r64.readU64() == null, true);
});
