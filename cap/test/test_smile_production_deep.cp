// Source: cp/cap/test/test_smile_production_deep.cp (Production deep tests for cap:smile)

import { test, expect_eq } from "cap:test";
import {
    SMILE_HEADER_MAGIC,
    SMILE_HEADER_SIZE,
    SMILE_FLAG_SHARED_NAMES,
    SMILE_FLAG_SHARED_VALUES,
    SMILE_FLAG_RAW_BINARY,
    SMILE_END_OF_STRING,
    SMILE_END_OF_CONTENT,
    SmileHeader,
    SmileVInt,
    SmileTokenKind,
    SmileKeyKind,
    SmileToken,
    SmileKeyToken,
    isSmile,
    parseSmileHeader,
    readSmileVInt,
    smileZigZagDecode,
    SmileReader,
} from "cap:smile";

fn writeBytes(dst: []u8, off: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
    return off + src.len;
}

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. parses 4B Smile header magic, version, and feature flags", () => {
    const hdr_bytes: [4]u8 = [0x3A, 0x29, 0x0A, 0x05]; // version=0, shared_names + raw_binary
    expect_eq(SMILE_HEADER_SIZE, 4);
    expect_eq(isSmile(hdr_bytes[0..4]), true);

    if (h := parseSmileHeader(hdr_bytes[0..4])) {
        expect_eq(h.version, 0);
        expect_eq(h.shared_names, true);
        expect_eq(h.shared_values, false);
        expect_eq(h.raw_binary, true);
        expect_eq(h.flags, SMILE_FLAG_SHARED_NAMES | SMILE_FLAG_RAW_BINARY);
    } else {
        expect_eq(true, false);
    }

    const hdr_v1: [4]u8 = [0x3A, 0x29, 0x0A, 0x13]; // version=1, shared_names + shared_values
    if (h2 := parseSmileHeader(hdr_v1[0..4])) {
        expect_eq(h2.version, 1);
        expect_eq(h2.shared_names, true);
        expect_eq(h2.shared_values, true);
        expect_eq(h2.raw_binary, false);
    } else {
        expect_eq(true, false);
    }
});

test("2. decodes Smile 7-bit+6-bit terminal VInt and ZigZag signed integers", () => {
    // 0 -> [0x80]
    const v0: [1]u8 = [0x80];
    if (r := readSmileVInt(v0[0..1], 0)) {
        expect_eq(r.value, 0);
        expect_eq(r.next_pos, 1);
    } else { expect_eq(true, false); }

    // 63 -> [0xBF]
    const v63: [1]u8 = [0xBF];
    if (r := readSmileVInt(v63[0..1], 0)) {
        expect_eq(r.value, 63);
    } else { expect_eq(true, false); }

    // 64 -> [0x01, 0x80] (1 << 6 | 0)
    const v64: [2]u8 = [0x01, 0x80];
    if (r := readSmileVInt(v64[0..2], 0)) {
        expect_eq(r.value, 64);
        expect_eq(r.next_pos, 2);
    } else { expect_eq(true, false); }

    // 200 -> 200 = (3 << 6) | 8 => [0x03, 0x88]
    const v200: [2]u8 = [0x03, 0x88];
    if (r := readSmileVInt(v200[0..2], 0)) {
        expect_eq(r.value, 200);
    } else { expect_eq(true, false); }

    // ZigZag decode
    expect_eq(smileZigZagDecode(0), 0);
    expect_eq(smileZigZagDecode(1), -1);
    expect_eq(smileZigZagDecode(2), 1);
    expect_eq(smileZigZagDecode(3), -2);
    expect_eq(smileZigZagDecode(30), 15);
    expect_eq(smileZigZagDecode(31), -16);
    expect_eq(smileZigZagDecode(199), -100);
});

test("3. decodes Tiny/Small/Long ASCII and Unicode strings", () => {
    let raw: [256]u8 = undefined;
    let buf = raw[0..256];
    let p = writeBytes(buf, 0, SMILE_HEADER_MAGIC);
    buf[p] = 0x00;
    p += 1;

    // Tiny ASCII "hi" (len=2 => 0x40 + 1 = 0x41)
    buf[p] = 0x41;
    p = writeBytes(buf, p + 1, "hi");

    // Small ASCII (len=33 => 0x60)
    buf[p] = 0x60;
    p = writeBytes(buf, p + 1, "012345678901234567890123456789012");

    // Tiny Unicode (len=6 bytes "中文" UTF-8 => 0x80 + (6 - 2) = 0x84)
    const zh = "中文";
    buf[p] = (0x80 + (zh.len - 2)) as u8;
    p = writeBytes(buf, p + 1, zh);

    // Long ASCII terminated by 0xFC
    buf[p] = 0xE0;
    p = writeBytes(buf, p + 1, "long_ascii_string_value");
    buf[p] = SMILE_END_OF_STRING;
    p += 1;

    let r = SmileReader.init(buf[0..p]);
    if (t := r.next()) {
        expect_eq(t.kind == SmileTokenKind.tiny_ascii, true);
        expect_eq(sliceEq(t.str_val, "hi"), true);
    } else { expect_eq(true, false); }

    if (t := r.next()) {
        expect_eq(t.kind == SmileTokenKind.small_ascii, true);
        expect_eq(t.str_val.len, 33);
    } else { expect_eq(true, false); }

    if (t := r.next()) {
        expect_eq(t.kind == SmileTokenKind.tiny_unicode, true);
        expect_eq(sliceEq(t.str_val, zh), true);
    } else { expect_eq(true, false); }

    if (t := r.next()) {
        expect_eq(t.kind == SmileTokenKind.long_ascii, true);
        expect_eq(sliceEq(t.str_val, "long_ascii_string_value"), true);
    } else { expect_eq(true, false); }

    expect_eq(r.next() == null, true);
    expect_eq(r.had_error, false);
});

test("4. parses full object/array stream { key: [tiny_int(-1), true, null, f32(1.5), shared_ref] }", () => {
    let raw: [128]u8 = undefined;
    let buf = raw[0..128];
    let p = writeBytes(buf, 0, SMILE_HEADER_MAGIC);
    buf[p] = SMILE_FLAG_SHARED_NAMES | SMILE_FLAG_SHARED_VALUES;
    p += 1;

    // '{'
    buf[p] = 0xFA;
    p += 1;
    // Key "key" (short ASCII len=3 => 0x80 + 2 = 0x82)
    buf[p] = 0x82;
    p = writeBytes(buf, p + 1, "key");
    // '['
    buf[p] = 0xF8;
    p += 1;
    // tiny_int(-1) => ZigZag(1) => 0xC1
    buf[p] = 0xC1;
    p += 1;
    // true (0x23), null (0x21)
    buf[p] = 0x23;
    buf[p + 1] = 0x21;
    p += 2;
    // f32(1.5) => IEEE 754 bits = 0x3FC00000
    // 7-bit split: b0=0x03, b1=0x7E, b2=0x00, b3=0x00, b4=0x00
    buf[p] = 0x28;
    buf[p + 1] = 0x03;
    buf[p + 2] = 0x7E;
    buf[p + 3] = 0x00;
    buf[p + 4] = 0x00;
    buf[p + 5] = 0x00;
    p += 6;
    // Short shared string ref index=4 => 0x05
    buf[p] = 0x05;
    p += 1;
    // ']' (0xF9), '}' (0xFB), EOF (0xFF)
    buf[p] = 0xF9;
    buf[p + 1] = 0xFB;
    buf[p + 2] = SMILE_END_OF_CONTENT;
    p += 3;

    let r = SmileReader.init(buf[0..p]);
    if (t := r.next()) { expect_eq(t.kind == SmileTokenKind.start_object, true); } else { expect_eq(true, false); }
    if (k := r.readKey()) {
        expect_eq(k.kind == SmileKeyKind.short_ascii, true);
        expect_eq(sliceEq(k.text, "key"), true);
    } else { expect_eq(true, false); }
    if (t := r.next()) { expect_eq(t.kind == SmileTokenKind.start_array, true); } else { expect_eq(true, false); }
    if (t := r.next()) {
        expect_eq(t.kind == SmileTokenKind.int_val, true);
        expect_eq(t.int_val, -1);
    } else { expect_eq(true, false); }
    if (t := r.next()) { expect_eq(t.kind == SmileTokenKind.true_val, true); } else { expect_eq(true, false); }
    if (t := r.next()) { expect_eq(t.kind == SmileTokenKind.null_val, true); } else { expect_eq(true, false); }
    if (t := r.next()) {
        expect_eq(t.kind == SmileTokenKind.float32, true);
        expect_eq(t.float_val > 1.499 && t.float_val < 1.501, true);
    } else { expect_eq(true, false); }
    if (t := r.next()) {
        expect_eq(t.kind == SmileTokenKind.shared_str_ref, true);
        expect_eq(t.ref_index, 4);
    } else { expect_eq(true, false); }
    if (t := r.next()) { expect_eq(t.kind == SmileTokenKind.end_array, true); } else { expect_eq(true, false); }
    if (k := r.readKey()) { expect_eq(k.kind == SmileKeyKind.end_object, true); } else { expect_eq(true, false); }
    if (t := r.next()) { expect_eq(t.kind == SmileTokenKind.end_of_content, true); } else { expect_eq(true, false); }
    expect_eq(r.had_error, false);
});

test("5. rejects bad magic, truncated VInt, missing 0xFC long-string terminator, and truncated f64", () => {
    expect_eq(isSmile("BAD!"), false);
    expect_eq(parseSmileHeader(":)") == null, true);

    // Unterminated VInt (no byte with high bit 0x80)
    const bad_vint: [3]u8 = [0x01, 0x02, 0x03];
    expect_eq(readSmileVInt(bad_vint[0..3], 0) == null, true);

    // Long ASCII missing 0xFC terminator
    const no_fc: [8]u8 = [0x3A, 0x29, 0x0A, 0x00, 0xE0, 'a', 'b', 'c'];
    let r1 = SmileReader.init(no_fc[0..8]);
    expect_eq(r1.next() == null, true);
    expect_eq(r1.had_error, true);

    // Truncated f64 (0x29 needs 10 bytes, only 3 available)
    const trunc_f64: [8]u8 = [0x3A, 0x29, 0x0A, 0x00, 0x29, 0x00, 0x00, 0x00];
    let r2 = SmileReader.init(trunc_f64[0..8]);
    expect_eq(r2.next() == null, true);
    expect_eq(r2.had_error, true);

    // Long key missing 0xFC terminator
    const no_fc_key: [7]u8 = [0x3A, 0x29, 0x0A, 0x00, 0x34, 'k', '1'];
    let r3 = SmileReader.init(no_fc_key[0..7]);
    expect_eq(r3.readKey() == null, true);
    expect_eq(r3.had_error, true);
});
