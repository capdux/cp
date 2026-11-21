// Source: cp/cap/test/test_code39_production_deep.cp (Production deep tests for cap:code39)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CODE39_ALPHABET_LEN,
    CODE39_START_STOP,
    CODE39_ELEMENTS_PER_CHAR,
    CODE39_WIDE_COUNT,
    code39Value,
    code39Char,
    code39Pattern9,
    code39CharFromPattern9,
    code39CheckDigit,
    verifyCode39Mod43,
    encodeCode39Modules,
    decodeCode39Modules,
} from "cap:code39";

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

fn popcount16(v: u16): usize {
    let x = v;
    let cnt: usize = 0;
    while (x != 0) {
        cnt += (x & 1) as usize;
        x >>= 1;
    }
    return cnt;
}

test("1. Complete 44-char table (43 alphabet + '*') 3-of-9 popcount invariant & unique bijection", () => {
    expect_eq(CODE39_ALPHABET_LEN, 43 as usize);
    expect_eq(CODE39_START_STOP, 0x2A as u8);
    expect_eq(CODE39_ELEMENTS_PER_CHAR, 9 as usize);
    expect_eq(CODE39_WIDE_COUNT, 3 as usize);

    const alpha = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%";
    expect_eq(alpha.len, 43 as usize);

    let seen_pat: [512]bool = std.mem.zeroes([512]bool);

    let i: usize = 0;
    while (i < 43) {
        const ch = alpha[i];
        expect_eq(code39Value(ch) ?? unreachable, i as u8);
        expect_eq(code39Char(i as u8) ?? unreachable, ch);

        const pat = code39Pattern9(ch) ?? unreachable;
        expect_eq(pat < 512, true);
        expect_eq(popcount16(pat), CODE39_WIDE_COUNT);
        expect_eq(seen_pat[pat as usize], false);
        seen_pat[pat as usize] = true;

        expect_eq(code39CharFromPattern9(pat) ?? unreachable, ch);
        i += 1;
    }

    // Start/Stop '*' is valid in pattern table (popcount == 3, unique), but NOT in Mod-43 data values
    expect_eq(code39Value(CODE39_START_STOP) == null, true);
    expect_eq(code39Char(43) == null, true);

    const star_pat = code39Pattern9(CODE39_START_STOP) ?? unreachable;
    expect_eq(star_pat, 0b010010100 as u16);
    expect_eq(popcount16(star_pat), CODE39_WIDE_COUNT);
    expect_eq(seen_pat[star_pat as usize], false);
    expect_eq(code39CharFromPattern9(star_pat) ?? unreachable, CODE39_START_STOP);
});

test("2. ISO/IEC 16388 Mod-43 check digit standard vectors & verification", () => {
    // "CODE39": C(12)+O(24)+D(13)+E(14)+3(3)+9(9) = 75 -> 75 % 43 = 32 -> 'W'
    expect_eq(code39CheckDigit("CODE39") ?? unreachable, 0x57 as u8);
    expect_eq(verifyCode39Mod43("CODE39W"), true);
    expect_eq(verifyCode39Mod43("CODE39X"), false);

    // "12345ABCDE": 15 + 60 = 75 -> 75 % 43 = 32 -> 'W'
    expect_eq(code39CheckDigit("12345ABCDE") ?? unreachable, 0x57 as u8);
    expect_eq(verifyCode39Mod43("12345ABCDEW"), true);

    // "CODE 39": 75 + 38 = 113 -> 113 % 43 = 27 -> 'R'
    expect_eq(code39CheckDigit("CODE 39") ?? unreachable, 0x52 as u8);
    expect_eq(verifyCode39Mod43("CODE 39R"), true);

    // Edge cases
    expect_eq(code39CheckDigit("") == null, true);
    expect_eq(verifyCode39Mod43(""), false);
    expect_eq(verifyCode39Mod43("A"), false);
});

test("3. End-to-end module stream encode/decode roundtrip for wide_ratio=2 and wide_ratio=3 (with & without Mod-43)", () => {
    let mods: [1024]u8 = std.mem.zeroes([1024]u8);
    let out: [64]u8 = std.mem.zeroes([64]u8);

    const msg = "CODE39-2026 $/+%";

    // Case A: wide_ratio=2, without Mod-43
    // Total chars = msg.len + 2 ('*' start/stop) = 18; each char = 3*2+6 = 12 modules; gaps = 17
    // Total modules = 18 * 12 + 17 = 233
    const len2_no = encodeCode39Modules(msg, false, 2, mods[0..]) ?? unreachable;
    expect_eq(len2_no, 233 as usize);
    const dec2_no = decodeCode39Modules(mods[0..len2_no], false, out[0..]) ?? unreachable;
    expect_eq(bytesEq(out[0..dec2_no], msg), true);

    // Case B: wide_ratio=2, with Mod-43
    // Total chars = 19; total modules = 19 * 12 + 18 = 246
    const len2_yes = encodeCode39Modules(msg, true, 2, mods[0..]) ?? unreachable;
    expect_eq(len2_yes, 246 as usize);
    const dec2_yes = decodeCode39Modules(mods[0..len2_yes], true, out[0..]) ?? unreachable;
    expect_eq(bytesEq(out[0..dec2_yes], msg), true);

    // Case C: wide_ratio=3, without Mod-43
    // Each char = 3*3+6 = 15 modules; total = 18 * 15 + 17 = 287
    const len3_no = encodeCode39Modules(msg, false, 3, mods[0..]) ?? unreachable;
    expect_eq(len3_no, 287 as usize);
    const dec3_no = decodeCode39Modules(mods[0..len3_no], false, out[0..]) ?? unreachable;
    expect_eq(bytesEq(out[0..dec3_no], msg), true);

    // Case D: wide_ratio=3, with Mod-43
    // Total chars = 19; total = 19 * 15 + 18 = 303
    const len3_yes = encodeCode39Modules(msg, true, 3, mods[0..]) ?? unreachable;
    expect_eq(len3_yes, 303 as usize);
    const dec3_yes = decodeCode39Modules(mods[0..len3_yes], true, out[0..]) ?? unreachable;
    expect_eq(bytesEq(out[0..dec3_yes], msg), true);

    // Full 43-character alphabet roundtrip at wide_ratio=3 with Mod-43
    const full_alpha = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%";
    const full_len = encodeCode39Modules(full_alpha, true, 3, mods[0..]) ?? unreachable;
    const full_dec = decodeCode39Modules(mods[0..full_len], true, out[0..]) ?? unreachable;
    expect_eq(bytesEq(out[0..full_dec], full_alpha), true);
});

test("4. Quiet zones & 2x oversampled module stream adaptive decoding", () => {
    let raw: [256]u8 = std.mem.zeroes([256]u8);
    let padded: [600]u8 = std.mem.zeroes([600]u8);
    let out: [32]u8 = std.mem.zeroes([32]u8);

    const raw_len = encodeCode39Modules("HELLO 39", true, 2, raw[0..]) ?? unreachable;

    // Prepend 10-module quiet zone, 2x oversample every module, append 12-module quiet zone
    let p: usize = 10;
    let i: usize = 0;
    while (i < raw_len) {
        padded[p] = raw[i];
        padded[p + 1] = raw[i];
        p += 2;
        i += 1;
    }
    const total_len = p + 12;

    const dlen = decodeCode39Modules(padded[0..total_len], true, out[0..]) ?? unreachable;
    expect_eq(bytesEq(out[0..dlen], "HELLO 39"), true);
});

test("5. Strict rejection of invalid chars, bad wide_ratio, missing sentinels, bad Mod-43 & buffer overflows", () => {
    let mods: [256]u8 = std.mem.zeroes([256]u8);
    let out: [32]u8 = std.mem.zeroes([32]u8);

    // Lowercase & unsupported characters rejected
    expect_eq(encodeCode39Modules("code39", false, 2, mods[0..]) == null, true);
    expect_eq(encodeCode39Modules("ABC_DEF", false, 3, mods[0..]) == null, true);
    expect_eq(encodeCode39Modules("A*B", false, 2, mods[0..]) == null, true);

    // Invalid wide_ratio (must be 2 or 3)
    expect_eq(encodeCode39Modules("ABC", false, 1, mods[0..]) == null, true);
    expect_eq(encodeCode39Modules("ABC", false, 4, mods[0..]) == null, true);

    // Encode buffer overflow: "A" with wide_ratio=2, no mod43 -> 3*12 + 2 = 38 modules
    expect_eq(encodeCode39Modules("A", false, 2, mods[0..37]) == null, true);
    const a_len = encodeCode39Modules("A", false, 2, mods[0..38]) ?? unreachable;
    expect_eq(a_len, 38 as usize);

    // Decode buffer overflow
    const abc_len = encodeCode39Modules("ABC", false, 2, mods[0..]) ?? unreachable;
    expect_eq(decodeCode39Modules(mods[0..abc_len], false, out[0..2]) == null, true);
    expect_eq(decodeCode39Modules(mods[0..abc_len], false, out[0..3]) ?? unreachable, 3 as usize);

    // Missing stop sentinel (truncate last character)
    expect_eq(decodeCode39Modules(mods[0..(abc_len - 13)], false, out[0..]) == null, true);

    // Missing start sentinel (skip first character + gap)
    expect_eq(decodeCode39Modules(mods[13..abc_len], false, out[0..]) == null, true);

    // Invalid module value (non-0/1)
    mods[0] = 2;
    expect_eq(decodeCode39Modules(mods[0..abc_len], false, out[0..]) == null, true);

    // Bad Mod-43 check digit: encode "CODE39X" without mod43, then try decoding with mod43=true
    const bad_chk_len = encodeCode39Modules("CODE39X", false, 3, mods[0..]) ?? unreachable;
    expect_eq(decodeCode39Modules(mods[0..bad_chk_len], true, out[0..]) == null, true);

    // Valid Mod-43 check digit encoded manually as "CODE39W" succeeds when decoded with mod43=true
    const good_chk_len = encodeCode39Modules("CODE39W", false, 3, mods[0..]) ?? unreachable;
    const dec_len = decodeCode39Modules(mods[0..good_chk_len], true, out[0..]) ?? unreachable;
    expect_eq(bytesEq(out[0..dec_len], "CODE39"), true);
});

test("6. Empty payload & all-zero module stream edge handling", () => {
    let mods: [64]u8 = std.mem.zeroes([64]u8);
    let out: [16]u8 = std.mem.zeroes([16]u8);

    // Empty stream or all-zero quiet zone -> null
    expect_eq(decodeCode39Modules(mods[0..0], false, out[0..]) == null, true);
    expect_eq(decodeCode39Modules(mods[0..32], false, out[0..]) == null, true);

    // Empty text with Mod-43 rejected; without Mod-43 encodes "**" (2*12 + 1 = 25 modules) -> ""
    expect_eq(encodeCode39Modules("", true, 2, mods[0..]) == null, true);
    const empty_len = encodeCode39Modules("", false, 2, mods[0..]) ?? unreachable;
    expect_eq(empty_len, 25 as usize);
    expect_eq(decodeCode39Modules(mods[0..empty_len], false, out[0..0]) ?? unreachable, 0 as usize);
    expect_eq(decodeCode39Modules(mods[0..empty_len], true, out[0..]) == null, true);
});
