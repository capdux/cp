// Source: cp/cap/test/test_roman_production_deep.cp (Production deep tests for cap:roman)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ROMAN_MIN,
    ROMAN_MAX,
    ROMAN_MAX_STR_LEN,
    MAX_COL_LETTERS_LEN,
    romanEncode,
    romanEncodeLower,
    romanDecode,
    romanDecodeLenient,
    isValidRoman,
    colIndexToLetters,
    lettersToColIndex,
} from "cap:roman";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("1. Exhaustive 1..3999 Roman encode/decode bijection (upper & lower) & max length invariant", () => {
    expect_eq(ROMAN_MIN, 1 as u16);
    expect_eq(ROMAN_MAX, 3999 as u16);
    expect_eq(ROMAN_MAX_STR_LEN, 15 as usize);
    expect_eq(MAX_COL_LETTERS_LEN, 7 as usize);

    let up_buf: [16]u8 = std.mem.zeroes([16]u8);
    let lo_buf: [16]u8 = std.mem.zeroes([16]u8);
    let max_seen_len: usize = 0;

    let n: u16 = ROMAN_MIN;
    while (n <= ROMAN_MAX) {
        const ulen = romanEncode(n, up_buf[0..]) ?? unreachable;
        expect_eq(ulen >= 1 && ulen <= ROMAN_MAX_STR_LEN, true);
        if (ulen > max_seen_len) {
            max_seen_len = ulen;
        }
        expect_eq(isValidRoman(up_buf[0..ulen]), true);
        expect_eq(romanDecode(up_buf[0..ulen]) ?? unreachable, n);

        const llen = romanEncodeLower(n, lo_buf[0..]) ?? unreachable;
        expect_eq(llen, ulen);
        expect_eq(romanDecode(lo_buf[0..llen]) ?? unreachable, n);

        n += 1;
    }

    expect_eq(max_seen_len, 15 as usize);
});

test("2. Classic Roman numeral canonical vectors (upper & lower)", () => {
    let buf: [16]u8 = std.mem.zeroes([16]u8);

    const n4 = romanEncode(4, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n4], "IV"), true);

    const n9 = romanEncode(9, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n9], "IX"), true);

    const n14 = romanEncodeLower(14, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n14], "xiv"), true);

    const n42 = romanEncode(42, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n42], "XLII"), true);

    const n1994 = romanEncode(1994, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n1994], "MCMXCIV"), true);

    const n2026 = romanEncode(2026, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n2026], "MMXXVI"), true);

    const n3888 = romanEncode(3888, buf[0..]) ?? unreachable;
    expect_eq(n3888, 15 as usize);
    expect_eq(bytesEq(buf[0..n3888], "MMMDCCCLXXXVIII"), true);

    const n3999 = romanEncode(3999, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n3999], "MMMCMXCIX"), true);
});

test("3. Strict mode rejection of all non-canonical Roman forms vs lenient clockface decoding", () => {
    // Strict mode MUST reject every non-canonical spelling
    expect_eq(romanDecode("IIII") == null, true);
    expect_eq(romanDecode("VV") == null, true);
    expect_eq(romanDecode("IC") == null, true);
    expect_eq(romanDecode("IL") == null, true);
    expect_eq(romanDecode("IIV") == null, true);
    expect_eq(romanDecode("VX") == null, true);
    expect_eq(romanDecode("XM") == null, true);
    expect_eq(romanDecode("MMMM") == null, true);
    expect_eq(romanDecode("IVI") == null, true);
    expect_eq(romanDecode("IXI") == null, true);
    expect_eq(isValidRoman("IIII"), false);
    expect_eq(isValidRoman("MMMM"), false);

    // Lenient mode accepts traditional additive clockface forms like "IIII" -> 4, "VIIII" -> 9
    expect_eq(romanDecodeLenient("IIII") ?? unreachable, 4 as u16);
    expect_eq(romanDecodeLenient("iiii") ?? unreachable, 4 as u16);
    expect_eq(romanDecodeLenient("VIIII") ?? unreachable, 9 as u16);
    expect_eq(romanDecodeLenient("XXXX") ?? unreachable, 40 as u16);
    // Still rejects >= 4000 or non-Roman characters
    expect_eq(romanDecodeLenient("MMMM") == null, true);
    expect_eq(romanDecodeLenient("XIIZ") == null, true);
});

test("4. Bijective Base-26 spreadsheet columns exhaustive 1..16384 roundtrip & milestones", () => {
    let buf: [8]u8 = std.mem.zeroes([8]u8);

    // Exact milestones: A, Z, AA, AZ, BA, ZZ, AAA, XFD
    const l1 = colIndexToLetters(1, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..l1], "A"), true);
    const l26 = colIndexToLetters(26, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..l26], "Z"), true);
    const l27 = colIndexToLetters(27, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..l27], "AA"), true);
    const l52 = colIndexToLetters(52, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..l52], "AZ"), true);
    const l53 = colIndexToLetters(53, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..l53], "BA"), true);
    const l702 = colIndexToLetters(702, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..l702], "ZZ"), true);
    const l703 = colIndexToLetters(703, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..l703], "AAA"), true);
    const l16384 = colIndexToLetters(16384, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..l16384], "XFD"), true);

    // Case-insensitive decode
    expect_eq(lettersToColIndex("xfd") ?? unreachable, 16384 as u32);
    expect_eq(lettersToColIndex("AaA") ?? unreachable, 703 as u32);

    // Exhaustive roundtrip for Excel's entire 1..16384 column range
    let c: u32 = 1;
    while (c <= 16384) {
        const len = colIndexToLetters(c, buf[0..]) ?? unreachable;
        const back = lettersToColIndex(buf[0..len]) ?? unreachable;
        expect_eq(back, c);
        c += 1;
    }
});

test("5. Bijective Base-26 32-bit maximum boundary & overflow protection", () => {
    let buf: [8]u8 = std.mem.zeroes([8]u8);

    // u32::MAX (4294967295) <-> "FXSHRXW"
    const max_u32: u32 = 0xFFFFFFFF;
    const len = colIndexToLetters(max_u32, buf[0..]) ?? unreachable;
    expect_eq(len, 7 as usize);
    expect_eq(bytesEq(buf[0..len], "FXSHRXW"), true);
    expect_eq(lettersToColIndex("FXSHRXW") ?? unreachable, max_u32);

    // Overflowing u32::MAX by 1 ("FXSHRXX") or longer strings -> MUST return null
    expect_eq(lettersToColIndex("FXSHRXX") == null, true);
    expect_eq(lettersToColIndex("ZZZZZZZ") == null, true);
    expect_eq(lettersToColIndex("AAAAAAAA") == null, true);
});

test("6. Strict rejection of out-of-range values, empty/invalid inputs & buffer overflows", () => {
    let buf: [16]u8 = std.mem.zeroes([16]u8);

    // Out-of-range Roman values
    expect_eq(romanEncode(0, buf[0..]) == null, true);
    expect_eq(romanEncode(4000, buf[0..]) == null, true);
    expect_eq(romanEncodeLower(0, buf[0..]) == null, true);
    expect_eq(romanDecode("") == null, true);
    expect_eq(romanDecode("MMXX6") == null, true);
    expect_eq(romanDecodeLenient("") == null, true);

    // Roman buffer overflow: 3888 needs 15 bytes
    expect_eq(romanEncode(3888, buf[0..14]) == null, true);
    expect_eq(romanEncode(3888, buf[0..15]) ?? unreachable, 15 as usize);

    // Column index 0, empty, non-letter, and buffer overflow
    expect_eq(colIndexToLetters(0, buf[0..]) == null, true);
    expect_eq(lettersToColIndex("") == null, true);
    expect_eq(lettersToColIndex("A1") == null, true);
    expect_eq(lettersToColIndex("A B") == null, true);
    expect_eq(colIndexToLetters(16384, buf[0..2]) == null, true);
    expect_eq(colIndexToLetters(16384, buf[0..3]) ?? unreachable, 3 as usize);
});
