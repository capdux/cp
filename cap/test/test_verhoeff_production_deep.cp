// Source: cp/cap/test/test_verhoeff_production_deep.cp (Production deep tests for cap:verhoeff)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    verhoeffCheckDigit,
    isValidVerhoeff,
    verhoeffAppend,
    dammCheckDigit,
    isValidDamm,
    dammAppend,
    iso7064Mod11_2CheckDigit,
    isValidIso7064Mod11_2,
    iso7064Mod37_2CheckDigit,
    isValidIso7064Mod37_2,
} from "cap:codec";

fn slicesEq(a: []const u8, b: []const u8): bool {
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

test("1. Verhoeff D5 standard vectors ('236'->'3', '12345'->'1') + 100% single-digit & adjacent transposition detection", () => {
    if (cd1 := verhoeffCheckDigit("236")) {
        expect_eq(cd1, '3' as u8);
    } else {
        expect_eq(true, false);
    }
    expect_eq(isValidVerhoeff("2363"), true);

    if (cd2 := verhoeffCheckDigit("12345")) {
        expect_eq(cd2, '1' as u8);
    } else {
        expect_eq(true, false);
    }
    expect_eq(isValidVerhoeff("123451"), true);

    // Append and exhaustively test all single-digit mutations & all adjacent transpositions.
    let buf: [16]u8 = std.mem.zeroes([16]u8);
    let out: []u8 = buf[0..];
    if (n := verhoeffAppend("9876543210", *out)) {
        expect_eq(n, 11 as usize);
        expect_eq(isValidVerhoeff(buf[0..n]), true);

        // Single-digit mutation at every position across all 9 alternative digits.
        let pos: usize = 0;
        while (pos < n) {
            const orig: u8 = buf[pos];
            let alt: u8 = '0' as u8;
            while (alt <= ('9' as u8)) {
                if (alt != orig) {
                    buf[pos] = alt;
                    expect_eq(isValidVerhoeff(buf[0..n]), false);
                }
                alt += 1;
            }
            buf[pos] = orig;
            pos += 1;
        }

        // Adjacent transposition at every pair (pos, pos+1) where digits differ.
        let tp: usize = 0;
        while (tp + 1 < n) {
            if (buf[tp] != buf[tp + 1]) {
                const a: u8 = buf[tp];
                const b: u8 = buf[tp + 1];
                buf[tp] = b;
                buf[tp + 1] = a;
                expect_eq(isValidVerhoeff(buf[0..n]), false);
                buf[tp] = a;
                buf[tp + 1] = b;
            }
            tp += 1;
        }
    } else {
        expect_eq(true, false);
    }
});

test("2. Damm quasigroup standard vector ('572'->'4') + 100% single-digit & adjacent transposition detection", () => {
    if (cd := dammCheckDigit("572")) {
        expect_eq(cd, '4' as u8);
    } else {
        expect_eq(true, false);
    }
    expect_eq(isValidDamm("5724"), true);

    let buf: [16]u8 = std.mem.zeroes([16]u8);
    let out: []u8 = buf[0..];
    if (n := dammAppend("3141592653", *out)) {
        expect_eq(n, 11 as usize);
        expect_eq(isValidDamm(buf[0..n]), true);

        // Single-digit mutation at every position.
        let pos: usize = 0;
        while (pos < n) {
            const orig: u8 = buf[pos];
            let alt: u8 = '0' as u8;
            while (alt <= ('9' as u8)) {
                if (alt != orig) {
                    buf[pos] = alt;
                    expect_eq(isValidDamm(buf[0..n]), false);
                }
                alt += 1;
            }
            buf[pos] = orig;
            pos += 1;
        }

        // Adjacent transposition at every differing pair.
        let tp: usize = 0;
        while (tp + 1 < n) {
            if (buf[tp] != buf[tp + 1]) {
                const a: u8 = buf[tp];
                const b: u8 = buf[tp + 1];
                buf[tp] = b;
                buf[tp + 1] = a;
                expect_eq(isValidDamm(buf[0..n]), false);
                buf[tp] = a;
                buf[tp + 1] = b;
            }
            tp += 1;
        }
    } else {
        expect_eq(true, false);
    }
});

test("3. ISO/IEC 7064 Mod-11-2 real ORCID vectors (including 'X' check digit) and hyphen/space tolerance", () => {
    // Real ORCIDs from the ORCID registry specification.
    expect_eq(isValidIso7064Mod11_2("0000-0002-1825-0097"), true);
    expect_eq(isValidIso7064Mod11_2("0000-0001-5109-3700"), true);
    expect_eq(isValidIso7064Mod11_2("0000-0002-1694-233X"), true);
    expect_eq(isValidIso7064Mod11_2("0000 0002 1694 233x"), true);
    expect_eq(isValidIso7064Mod11_2("0000000218250097"), true);

    if (c1 := iso7064Mod11_2CheckDigit("0000-0002-1825-009")) {
        expect_eq(c1, '7' as u8);
    } else {
        expect_eq(true, false);
    }
    if (c2 := iso7064Mod11_2CheckDigit("0000-0001-5109-370")) {
        expect_eq(c2, '0' as u8);
    } else {
        expect_eq(true, false);
    }
    if (c3 := iso7064Mod11_2CheckDigit("0000-0002-1694-233")) {
        expect_eq(c3, 'X' as u8);
    } else {
        expect_eq(true, false);
    }

    // Tampered ORCIDs must fail.
    expect_eq(isValidIso7064Mod11_2("0000-0002-1825-0098"), false);
    expect_eq(isValidIso7064Mod11_2("0000-0002-1694-2339"), false);
    expect_eq(isValidIso7064Mod11_2("0000-0002-1852-0097"), false);
});

test("4. ISO/IEC 7064 Mod-37-2 alphanumeric round-trip, '*' check character, and single-char error detection", () => {
    const samples: [4][]const u8 = [
        "CAP2026",
        "HELLO-WORLD-99",
        "0000-0000-0001",
        "ZZ99AA11",
    ];

    let s: usize = 0;
    while (s < 4) {
        const body: []const u8 = samples[s];
        if (cd := iso7064Mod37_2CheckDigit(body)) {
            let full: [32]u8 = std.mem.zeroes([32]u8);
            let i: usize = 0;
            while (i < body.len) {
                full[i] = body[i];
                i += 1;
            }
            full[body.len] = cd;
            const total_len: usize = body.len + 1;
            expect_eq(isValidIso7064Mod37_2(full[0..total_len]), true);

            // Corrupt first character -> must fail validation.
            const orig: u8 = full[0];
            if (orig == '0') {
                full[0] = '1' as u8;
            } else {
                full[0] = '0' as u8;
            }
            expect_eq(isValidIso7064Mod37_2(full[0..total_len]), false);
        } else {
            expect_eq(true, false);
        }
        s += 1;
    }

    // Verify check character '*' (remainder 36) is produced and validated cleanly.
    // Find a 1-char body whose check digit is '*': (38 - (v * 2 % 37)) % 37 == 36 -> (2*v) % 37 == 2 -> v == 1 ('1').
    if (star_cd := iso7064Mod37_2CheckDigit("1")) {
        expect_eq(star_cd, '*' as u8);
    } else {
        expect_eq(true, false);
    }
    expect_eq(isValidIso7064Mod37_2("1*"), true);
    expect_eq(isValidIso7064Mod37_2("2*"), false);
});

test("5. Malformed input guards: empty, single-char, non-digits, invalid symbols, and short output buffers", () => {
    // Empty inputs rejected everywhere.
    if (_ := verhoeffCheckDigit("")) {
        expect_eq(true, false);
    }
    if (_ := dammCheckDigit("")) {
        expect_eq(true, false);
    }
    if (_ := iso7064Mod11_2CheckDigit("")) {
        expect_eq(true, false);
    }
    if (_ := iso7064Mod11_2CheckDigit("--- ")) {
        expect_eq(true, false);
    }
    if (_ := iso7064Mod37_2CheckDigit("")) {
        expect_eq(true, false);
    }

    expect_eq(isValidVerhoeff(""), false);
    expect_eq(isValidVerhoeff("0"), false);
    expect_eq(isValidDamm(""), false);
    expect_eq(isValidDamm("0"), false);
    expect_eq(isValidIso7064Mod11_2("7"), false);
    expect_eq(isValidIso7064Mod37_2("A"), false);

    // Non-digit / illegal symbol rejection.
    if (_ := verhoeffCheckDigit("12A4")) {
        expect_eq(true, false);
    }
    if (_ := dammCheckDigit("12-4")) {
        expect_eq(true, false);
    }
    if (_ := iso7064Mod11_2CheckDigit("123X")) {
        expect_eq(true, false);
    }
    if (_ := iso7064Mod37_2CheckDigit("123$4")) {
        expect_eq(true, false);
    }
    expect_eq(isValidVerhoeff("236A"), false);
    expect_eq(isValidDamm("572!"), false);
    expect_eq(isValidIso7064Mod11_2("0000-0002-1825-009Z"), false);
    expect_eq(isValidIso7064Mod37_2("CAP2026!"), false);

    // Short output buffers rejected by verhoeffAppend / dammAppend.
    let small: [3]u8 = std.mem.zeroes([3]u8);
    let small_slice: []u8 = small[0..];
    if (_ := verhoeffAppend("236", *small_slice)) {
        expect_eq(true, false);
    }
    if (_ := dammAppend("572", *small_slice)) {
        expect_eq(true, false);
    }

    // Exact-size buffer succeeds.
    let exact: [4]u8 = std.mem.zeroes([4]u8);
    let exact_slice: []u8 = exact[0..];
    if (vn := verhoeffAppend("236", *exact_slice)) {
        expect_eq(vn, 4 as usize);
        expect_eq(slicesEq(exact[0..4], "2363"), true);
    } else {
        expect_eq(true, false);
    }
    if (dn := dammAppend("572", *exact_slice)) {
        expect_eq(dn, 4 as usize);
        expect_eq(slicesEq(exact[0..4], "5724"), true);
    } else {
        expect_eq(true, false);
    }
});
