// Source: cp/cap/test/test_code128_production_deep.cp (Production deep tests for cap:code128)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CODE128_START_A,
    CODE128_START_B,
    CODE128_START_C,
    CODE128_STOP,
    CODE128_FNC1,
    CODE128_CODE_C,
    CODE128_CODE_B,
    CODE128_CODE_A,
    CODE128_SHIFT,
    CODE128_MOD_PER_SYMBOL,
    CODE128_STOP_MODULES,
    code128Pattern,
    code128SymbolFromPattern,
    code128Checksum,
    encodeCode128B,
    encodeCode128C,
    encodeCode128Auto,
    renderCode128Modules,
    decodeCode128Symbols,
    decodeCode128Modules,
} from "cap:code128";

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

test("1. All 106 Code 128 11-bit patterns: uniqueness, 3-bar/3-space balance, even bar parity, and reverse lookup", () => {
    expect_eq(CODE128_START_A, 103 as u8);
    expect_eq(CODE128_START_B, 104 as u8);
    expect_eq(CODE128_START_C, 105 as u8);
    expect_eq(CODE128_STOP, 106 as u8);
    expect_eq(CODE128_MOD_PER_SYMBOL, 11 as usize);
    expect_eq(CODE128_STOP_MODULES, 13 as usize);

    let sym: u8 = 0;
    while (sym < 106) {
        if (pat := code128Pattern(sym)) {
            // Must fit in 11 bits, start with bar (bit 10 == 1) and end with space (bit 0 == 0).
            expect_eq(pat & 0xF800, 0 as u16);
            expect_eq((pat >> 10) & 1, 1 as u16);
            expect_eq(pat & 1, 0 as u16);

            // Count bars (1s) and transitions (must be exactly 5 transitions -> 3 bars, 3 spaces).
            let bars: u8 = 0;
            let transitions: u8 = 0;
            let prev: u16 = (pat >> 10) & 1;
            let bit: usize = 0;
            while (bit < 11) {
                const shift: u4 = (10 - bit) as u4;
                const cur: u16 = (pat >> shift) & 1;
                if (cur == 1) {
                    bars += 1;
                }
                if (bit > 0 && cur != prev) {
                    transitions += 1;
                }
                prev = cur;
                bit += 1;
            }
            expect_eq((bars & 1) == 0, true);
            expect_eq(transitions, 5 as u8);

            // Reverse lookup must return exact symbol (proving uniqueness across all 106).
            if (rev := code128SymbolFromPattern(pat)) {
                expect_eq(rev, sym);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
        sym += 1;
    }

    // Stop (106) is 13-bit 0b1100011101011 = 0x18EB; > 106 returns null.
    if (stop_pat := code128Pattern(CODE128_STOP)) {
        expect_eq(stop_pat, 0x18EB as u16);
    } else {
        expect_eq(true, false);
    }
    if (_ := code128Pattern(107)) {
        expect_eq(true, false);
    }
    if (_ := code128SymbolFromPattern(0x0000)) {
        expect_eq(true, false);
    }
    if (_ := code128SymbolFromPattern(0x18EB)) {
        expect_eq(true, false);
    }
});

test("2. Code 128B ('Wikipedia' Mod-103 == 88, 'Hello-128!') and Code 128C ('202611220915') round-trip", () => {
    let sym_arr: [64]u8 = std.mem.zeroes([64]u8);
    let syms: []u8 = sym_arr[0..];
    let out_arr: [64]u8 = std.mem.zeroes([64]u8);
    let out: []u8 = out_arr[0..];

    // Standard ISO/Wikipedia vector: "Wikipedia" -> Start B (104), 9 data syms, Checksum 88, Stop 106.
    if (n := encodeCode128B("Wikipedia", *syms)) {
        expect_eq(n, 12 as usize);
        expect_eq(sym_arr[0], CODE128_START_B);
        expect_eq(sym_arr[10], 88 as u8);
        expect_eq(sym_arr[11], CODE128_STOP);

        if (chk := code128Checksum(sym_arr[0..10])) {
            expect_eq(chk, 88 as u8);
        } else {
            expect_eq(true, false);
        }

        if (dlen := decodeCode128Symbols(sym_arr[0..n], *out)) {
            expect_eq(slicesEq(out_arr[0..dlen], "Wikipedia"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // "Hello-128!" round-trip in Code 128B.
    if (n2 := encodeCode128B("Hello-128!", *syms)) {
        if (dlen2 := decodeCode128Symbols(sym_arr[0..n2], *out)) {
            expect_eq(slicesEq(out_arr[0..dlen2], "Hello-128!"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Code 128C numeric compression: 12 digits -> 6 data symbols + 3 framing = 9 symbols.
    if (nc := encodeCode128C("202611220915", *syms)) {
        expect_eq(nc, 9 as usize);
        expect_eq(sym_arr[0], CODE128_START_C);
        expect_eq(sym_arr[1], 20 as u8);
        expect_eq(sym_arr[2], 26 as u8);
        expect_eq(sym_arr[6], 15 as u8);
        expect_eq(sym_arr[8], CODE128_STOP);

        if (dlen_c := decodeCode128Symbols(sym_arr[0..nc], *out)) {
            expect_eq(slicesEq(out_arr[0..dlen_c], "202611220915"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("3. encodeCode128Auto B/C mixed switching is shorter than pure Code 128B and decodes identically", () => {
    const mixed: []const u8 = "ORD-202611220915-XY";
    let syms_b_arr: [64]u8 = std.mem.zeroes([64]u8);
    let syms_b: []u8 = syms_b_arr[0..];
    let syms_auto_arr: [64]u8 = std.mem.zeroes([64]u8);
    let syms_auto: []u8 = syms_auto_arr[0..];
    let out_arr: [64]u8 = std.mem.zeroes([64]u8);
    let out: []u8 = out_arr[0..];

    if (nb := encodeCode128B(mixed, *syms_b)) {
        if (na := encodeCode128Auto(mixed, *syms_auto)) {
            // Pure B = 19 + 3 = 22 symbols; Auto switches to C for 12 digits -> 18 symbols.
            expect_eq(na < nb, true);
            if (dlen := decodeCode128Symbols(syms_auto_arr[0..na], *out)) {
                expect_eq(slicesEq(out_arr[0..dlen], mixed), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Odd-length digit run inside mixed string (5 digits) + leading digits.
    const odd_mix: []const u8 = "987654321-SKU12345Z";
    if (na2 := encodeCode128Auto(odd_mix, *syms_auto)) {
        if (nb2 := encodeCode128B(odd_mix, *syms_b)) {
            expect_eq(na2 < nb2, true);
        } else {
            expect_eq(true, false);
        }
        if (dlen2 := decodeCode128Symbols(syms_auto_arr[0..na2], *out)) {
            expect_eq(slicesEq(out_arr[0..dlen2], odd_mix), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("4. Full pipeline renderCode128Modules -> decodeCode128Modules round-trip and Code A/Shift decoding", () => {
    const msg: []const u8 = "CAP-128-99887766-OK!";
    let sym_arr: [64]u8 = std.mem.zeroes([64]u8);
    let syms: []u8 = sym_arr[0..];
    let mod_arr: [512]u8 = std.mem.zeroes([512]u8);
    let mods: []u8 = mod_arr[0..];
    let out_arr: [64]u8 = std.mem.zeroes([64]u8);
    let out: []u8 = out_arr[0..];

    if (sn := encodeCode128Auto(msg, *syms)) {
        if (mn := renderCode128Modules(sym_arr[0..sn], *mods)) {
            expect_eq(mn, (sn - 1) * CODE128_MOD_PER_SYMBOL + CODE128_STOP_MODULES);

            if (dlen := decodeCode128Modules(mod_arr[0..mn], *out)) {
                expect_eq(slicesEq(out_arr[0..dlen], msg), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Verify Start A + Shift to B (single lowercase 'a') + back to A ('B') + FNC1 ignored.
    let a_syms: [8]u8 = [
        CODE128_START_A,
        CODE128_FNC1,
        33,            // 'A' in Code A (33 + 32 = 65)
        CODE128_SHIFT, // temporary switch to B for 1 char
        65,            // 'a' in Code B (65 + 32 = 97)
        34,            // 'B' in Code A (34 + 32 = 66)
        0,             // placeholder checksum
        CODE128_STOP,
    ];
    if (chk := code128Checksum(a_syms[0..6])) {
        a_syms[6] = chk;
        if (mn_a := renderCode128Modules(a_syms[0..8], *mods)) {
            if (dlen_a := decodeCode128Modules(mod_arr[0..mn_a], *out)) {
                expect_eq(slicesEq(out_arr[0..dlen_a], "AaB"), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("5. Malformed input guards: bad Mod-103, odd-length Code C, invalid chars, corrupted modules, truncation", () => {
    let sym_arr: [32]u8 = std.mem.zeroes([32]u8);
    let syms: []u8 = sym_arr[0..];
    let mod_arr: [256]u8 = std.mem.zeroes([256]u8);
    let mods: []u8 = mod_arr[0..];
    let out_arr: [32]u8 = std.mem.zeroes([32]u8);
    let out: []u8 = out_arr[0..];

    // 1) Odd-length and non-digit Code 128C rejected.
    if (_ := encodeCode128C("12345", *syms)) {
        expect_eq(true, false);
    }
    if (_ := encodeCode128C("12AB", *syms)) {
        expect_eq(true, false);
    }

    // 2) Control character (< 0x20) or high byte (> 0x7F) rejected by Code 128B and Auto.
    if (_ := encodeCode128B("BAD\nCHAR", *syms)) {
        expect_eq(true, false);
    }
    if (_ := encodeCode128Auto("BAD\x80BYTE", *syms)) {
        expect_eq(true, false);
    }

    // 3) Bad Mod-103 checksum rejected by both decodeCode128Symbols and decodeCode128Modules.
    if (sn := encodeCode128B("TEST", *syms)) {
        sym_arr[sn - 2] = (sym_arr[sn - 2] + 1) % 103;
        if (_ := decodeCode128Symbols(sym_arr[0..sn], *out)) {
            expect_eq(true, false);
        }
        if (mn := renderCode128Modules(sym_arr[0..sn], *mods)) {
            if (_ := decodeCode128Modules(mod_arr[0..mn], *out)) {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 4) Single-bit module corruption, non-binary module byte, and truncated module stream rejected.
    if (sn2 := encodeCode128B("VALID", *syms)) {
        if (mn2 := renderCode128Modules(sym_arr[0..sn2], *mods)) {
            // Flip 1 module bit -> invalid pattern or checksum mismatch.
            mod_arr[15] = mod_arr[15] ^ 1;
            if (_ := decodeCode128Modules(mod_arr[0..mn2], *out)) {
                expect_eq(true, false);
            }
            mod_arr[15] = mod_arr[15] ^ 1;

            // Non-binary byte (2) rejected.
            mod_arr[0] = 2;
            if (_ := decodeCode128Modules(mod_arr[0..mn2], *out)) {
                expect_eq(true, false);
            }
            mod_arr[0] = 1;

            // Truncated stream rejected.
            if (_ := decodeCode128Modules(mod_arr[0..(mn2 - 1)], *out)) {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});
