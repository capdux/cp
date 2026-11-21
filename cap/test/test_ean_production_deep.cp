// Source: cp/cap/test/test_ean_production_deep.cp (Production deep tests for cap:ean)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    EAN13_MODULES,
    EAN8_MODULES,
    UPCA_MODULES,
    EAN_DIGIT_MODULES,
    GUARD_OUTER,
    GUARD_CENTER,
    eanPatternL,
    eanPatternG,
    eanPatternR,
    ean13ParityMask,
    eanCheckDigit,
    encodeEan13Modules,
    decodeEan13Modules,
    encodeEan8Modules,
    decodeEan8Modules,
    encodeUpcAModules,
} from "cap:ean";

fn popcnt8(x: u8): u8 {
    let v = x;
    let c: u8 = 0;
    while (v != 0) {
        v = v & (v - 1);
        c += 1;
    }
    return c;
}

fn rev7(x: u8): u8 {
    let out: u8 = 0;
    let i: usize = 0;
    while (i < 7) {
        if (((x >> (i as u3)) & 1) != 0) {
            out = out | ((1 as u8) << ((6 - i) as u3));
        }
        i += 1;
    }
    return out;
}

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

fn reverseSlice(src: []const u8, dst: []u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[src.len - 1 - i];
        i += 1;
    }
}

test("1. All digits 0..9 L/G/R 7-bit parity, complement & reversal invariants", () => {
    expect_eq(EAN13_MODULES, 95 as usize);
    expect_eq(EAN8_MODULES, 67 as usize);
    expect_eq(UPCA_MODULES, 95 as usize);
    expect_eq(EAN_DIGIT_MODULES, 7 as usize);
    expect_eq(GUARD_OUTER, 0b101 as u8);
    expect_eq(GUARD_CENTER, 0b01010 as u8);

    let d: u8 = 0;
    while (d < 10) {
        const l = eanPatternL(d) ?? unreachable;
        const g = eanPatternG(d) ?? unreachable;
        const r = eanPatternR(d) ?? unreachable;

        // L-code has odd parity (3 or 5 ones), starts with 0, ends with 1
        const lp = popcnt8(l);
        expect_eq(lp == 3 || lp == 5, true);
        expect_eq((l >> 6) & 1, 0 as u8);
        expect_eq(l & 1, 1 as u8);

        // R-code is 7-bit bitwise complement of L-code (even parity: 2 or 4 ones)
        expect_eq(r, (~l) & 0x7F);
        expect_eq((popcnt8(r) % 2), 0 as u8);

        // G-code is 7-bit reversal of R-code (even parity: 2 or 4 ones)
        expect_eq(g, rev7(r));
        expect_eq((popcnt8(g) % 2), 0 as u8);

        // Parity mask for first digit: MSB (leftmost digit 1) is always L (0)
        const pm = ean13ParityMask(d) ?? unreachable;
        expect_eq((pm >> 5) & 1, 0 as u8);

        d += 1;
    }

    // Out-of-range digit rejected
    expect_eq(eanPatternL(10) == null, true);
    expect_eq(eanPatternG(10) == null, true);
    expect_eq(eanPatternR(10) == null, true);
    expect_eq(ean13ParityMask(10) == null, true);
});

test("2. EAN-13 all 10 first-digit parity combinations (0..9) forward & reversed 95-module roundtrip", () => {
    let mods: [95]u8 = std.mem.zeroes([95]u8);
    let rev_mods: [95]u8 = std.mem.zeroes([95]u8);
    let out_fwd: [13]u8 = std.mem.zeroes([13]u8);
    let out_rev: [13]u8 = std.mem.zeroes([13]u8);

    // Classic standard EAN-13 check digit verification: 400638133393 -> 1
    expect_eq(eanCheckDigit("400638133393") ?? unreachable, 1 as u8);

    let first: u8 = 0;
    while (first < 10) {
        let code: [13]u8 = [
            first + 0x30,
            0x39, 0x37, 0x38, 0x30, 0x32, 0x30,
            0x31, 0x33, 0x37, 0x39, 0x36,
            0x30,
        ];
        const chk = eanCheckDigit(code[0..12]) ?? unreachable;
        code[12] = chk + 0x30;

        const mlen = encodeEan13Modules(code[0..], mods[0..]) ?? unreachable;
        expect_eq(mlen, 95 as usize);

        // Forward decode
        const df = decodeEan13Modules(mods[0..], out_fwd[0..]) ?? unreachable;
        expect_eq(df, 13 as usize);
        expect_eq(bytesEq(out_fwd[0..], code[0..]), true);

        // Upside-down (reversed) scan decode
        reverseSlice(mods[0..], rev_mods[0..]);
        const dr = decodeEan13Modules(rev_mods[0..], out_rev[0..]) ?? unreachable;
        expect_eq(dr, 13 as usize);
        expect_eq(bytesEq(out_rev[0..], code[0..]), true);

        first += 1;
    }
});

test("3. EAN-8 67-module forward & reversed scan roundtrip", () => {
    // 9638507 -> check digit 4 ("96385074")
    expect_eq(eanCheckDigit("9638507") ?? unreachable, 4 as u8);
    const ean8 = "96385074";

    let mods: [67]u8 = std.mem.zeroes([67]u8);
    let rev_mods: [67]u8 = std.mem.zeroes([67]u8);
    let out_fwd: [8]u8 = std.mem.zeroes([8]u8);
    let out_rev: [8]u8 = std.mem.zeroes([8]u8);

    const mlen = encodeEan8Modules(ean8, mods[0..]) ?? unreachable;
    expect_eq(mlen, 67 as usize);

    const df = decodeEan8Modules(mods[0..], out_fwd[0..]) ?? unreachable;
    expect_eq(df, 8 as usize);
    expect_eq(bytesEq(out_fwd[0..], ean8), true);

    reverseSlice(mods[0..], rev_mods[0..]);
    const dr = decodeEan8Modules(rev_mods[0..], out_rev[0..]) ?? unreachable;
    expect_eq(dr, 8 as usize);
    expect_eq(bytesEq(out_rev[0..], ean8), true);
});

test("4. UPC-A 12-digit 95-module encoding equivalence with leading-'0' EAN-13", () => {
    // Standard UPC-A: 03600029145 -> check digit 2 ("036000291452")
    expect_eq(eanCheckDigit("03600029145") ?? unreachable, 2 as u8);
    const upca = "036000291452";

    let mods: [95]u8 = std.mem.zeroes([95]u8);
    let out13: [13]u8 = std.mem.zeroes([13]u8);

    const mlen = encodeUpcAModules(upca, mods[0..]) ?? unreachable;
    expect_eq(mlen, 95 as usize);

    const dlen = decodeEan13Modules(mods[0..], out13[0..]) ?? unreachable;
    expect_eq(dlen, 13 as usize);
    expect_eq(bytesEq(out13[0..], "0036000291452"), true);
});

test("5. Corrupted guards, bad Mod-10, invalid 7-bit patterns & buffer/length error rejection", () => {
    let mods13: [95]u8 = std.mem.zeroes([95]u8);
    let out13: [13]u8 = std.mem.zeroes([13]u8);
    let mods8: [67]u8 = std.mem.zeroes([67]u8);
    let out8: [8]u8 = std.mem.zeroes([8]u8);

    // Bad Mod-10 rejected on encode
    expect_eq(encodeEan13Modules("4006381333932", mods13[0..]) == null, true);
    expect_eq(encodeEan8Modules("96385075", mods8[0..]) == null, true);
    expect_eq(encodeUpcAModules("036000291453", mods13[0..]) == null, true);

    // Non-digit / wrong length rejected
    expect_eq(eanCheckDigit("") == null, true);
    expect_eq(eanCheckDigit("40063813339A") == null, true);
    expect_eq(encodeEan13Modules("400638133393", mods13[0..]) == null, true);

    // Encode valid EAN-13 then corrupt guard / data / non-binary byte
    const mlen = encodeEan13Modules("4006381333931", mods13[0..]) ?? unreachable;
    expect_eq(mlen, 95 as usize);

    // Corrupt outer guard (module 0)
    mods13[0] = 0;
    expect_eq(decodeEan13Modules(mods13[0..], out13[0..]) == null, true);
    mods13[0] = 1;

    // Corrupt center guard (module 46)
    mods13[46] = 0;
    expect_eq(decodeEan13Modules(mods13[0..], out13[0..]) == null, true);
    mods13[46] = 1;

    // Non-binary module value (2)
    mods13[10] = 2;
    expect_eq(decodeEan13Modules(mods13[0..], out13[0..]) == null, true);

    // Corrupt 7-bit digit pattern to all zeros (illegal pattern)
    let k: usize = 3;
    while (k < 10) {
        mods13[k] = 0;
        k += 1;
    }
    expect_eq(decodeEan13Modules(mods13[0..], out13[0..]) == null, true);

    // Buffer overflow & wrong module length rejection
    let short_mods: [94]u8 = std.mem.zeroes([94]u8);
    let short_digits: [12]u8 = std.mem.zeroes([12]u8);
    expect_eq(encodeEan13Modules("4006381333931", short_mods[0..]) == null, true);
    expect_eq(decodeEan13Modules(short_mods[0..], out13[0..]) == null, true);
    expect_eq(decodeEan13Modules(mods13[0..], short_digits[0..]) == null, true);
    expect_eq(decodeEan8Modules(mods8[0..66], out8[0..]) == null, true);
});
