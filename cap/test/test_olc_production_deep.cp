// Source: cp/cap/test/test_olc_production_deep.cp (Production deep tests for cap:olc)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    OLC_ALPHABET,
    OLC_SEPARATOR,
    OLC_SEPARATOR_POS,
    OLC_PADDING,
    OLC_DEFAULT_LEN,
    OLC_MAX_LEN,
    OlcArea,
    isValidOlc,
    isShortOlc,
    isFullOlc,
    olcEncode,
    olcDecode,
    olcShorten,
    olcRecoverNearest,
} from "cap:geo";

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

fn absDiff(a: f64, b: f64): f64 {
    if (a >= b) {
        return a - b;
    }
    return b - a;
}

test("1. Google official OLC vectors: Zurich, Equator, North/South Poles, padded 6-char, and 11/12-char grid", () => {
    expect_eq(OLC_ALPHABET.len, 20 as usize);
    expect_eq(OLC_SEPARATOR, '+' as u8);
    expect_eq(OLC_SEPARATOR_POS, 8 as usize);
    expect_eq(OLC_PADDING, '0' as u8);
    expect_eq(OLC_DEFAULT_LEN, 10 as usize);
    expect_eq(OLC_MAX_LEN, 15 as usize);

    let buf: [20]u8 = std.mem.zeroes([20]u8);
    let out: []u8 = buf[0..];

    // Zurich: (47.365590, 8.524997) -> "8FVC9G8F+6X"
    if (n := olcEncode(47.365590, 8.524997, 10, *out)) {
        expect_eq(slicesEq(buf[0..n], "8FVC9G8F+6X"), true);
    } else {
        expect_eq(true, false);
    }

    // 11-char and 12-char 4x5 grid refinements.
    if (n11 := olcEncode(47.365590, 8.524997, 11, *out)) {
        expect_eq(slicesEq(buf[0..n11], "8FVC9G8F+6XQ"), true);
    } else {
        expect_eq(true, false);
    }
    if (n12 := olcEncode(47.365590, 8.524997, 12, *out)) {
        expect_eq(slicesEq(buf[0..n12], "8FVC9G8F+6XQQ"), true);
    } else {
        expect_eq(true, false);
    }

    // Padded 6-char code -> "8FVC9G00+"
    if (n6 := olcEncode(47.365590, 8.524997, 6, *out)) {
        expect_eq(slicesEq(buf[0..n6], "8FVC9G00+"), true);
    } else {
        expect_eq(true, false);
    }

    // Equator / Prime Meridian -> "6FG22222+22"
    if (ne := olcEncode(0.0, 0.0, 10, *out)) {
        expect_eq(slicesEq(buf[0..ne], "6FG22222+22"), true);
    } else {
        expect_eq(true, false);
    }

    // North Pole (90.0, 0.0) -> "CFX2X2X2+X2"
    if (np := olcEncode(90.0, 0.0, 10, *out)) {
        expect_eq(slicesEq(buf[0..np], "CFX2X2X2+X2"), true);
    } else {
        expect_eq(true, false);
    }

    // South Pole / Antimeridian (-90.0, -180.0) -> "22222222+22"; longitude wrap (+180.0 -> -180.0).
    if (sp := olcEncode(-90.0, -180.0, 10, *out)) {
        expect_eq(slicesEq(buf[0..sp], "22222222+22"), true);
    } else {
        expect_eq(true, false);
    }
    if (sp_wrap := olcEncode(-90.0, 180.0, 10, *out)) {
        expect_eq(slicesEq(buf[0..sp_wrap], "22222222+22"), true);
    } else {
        expect_eq(true, false);
    }
});

test("2. olcEncode/olcDecode precision round-trip across code_len 2, 4, 6, 8, 10, 11, 13, 15", () => {
    const lat: f64 = 35.689487;
    const lng: f64 = 139.691706;
    const lens: [8]usize = [2, 4, 6, 8, 10, 11, 13, 15];

    let buf: [20]u8 = std.mem.zeroes([20]u8);
    let out: []u8 = buf[0..];

    let i: usize = 0;
    while (i < 8) {
        const clen: usize = lens[i];
        if (n := olcEncode(lat, lng, clen, *out)) {
            expect_eq(isFullOlc(buf[0..n]), true);
            expect_eq(isShortOlc(buf[0..n]), false);

            if (area := olcDecode(buf[0..n])) {
                expect_eq(area.code_len, clen);
                expect_eq(lat >= area.lat_lo && lat <= area.lat_hi, true);
                expect_eq(lng >= area.lng_lo && lng <= area.lng_hi, true);
                const half_lat: f64 = (area.lat_hi - area.lat_lo) * 0.5 + 1.0e-12;
                const half_lng: f64 = (area.lng_hi - area.lng_lo) * 0.5 + 1.0e-12;
                expect_eq(absDiff(area.latCenter(), lat) <= half_lat, true);
                expect_eq(absDiff(area.lngCenter(), lng) <= half_lng, true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
        i += 1;
    }
});

test("3. olcShorten and olcRecoverNearest round-trip at 4/6/8-char trim distances and cross-quadrant recovery", () => {
    const full: []const u8 = "8FVC9G8F+6X";
    let short_buf: [16]u8 = std.mem.zeroes([16]u8);
    let s_out: []u8 = short_buf[0..];
    let rec_buf: [16]u8 = std.mem.zeroes([16]u8);
    let r_out: []u8 = rec_buf[0..];

    // Very close reference (~0.0002 deg away) -> trims 8 chars to "+6X".
    if (sn8 := olcShorten(full, 47.3657, 8.5251, *s_out)) {
        expect_eq(slicesEq(short_buf[0..sn8], "+6X"), true);
        expect_eq(isShortOlc(short_buf[0..sn8]), true);
        if (rn8 := olcRecoverNearest(short_buf[0..sn8], 47.3657, 8.5251, *r_out)) {
            expect_eq(slicesEq(rec_buf[0..rn8], full), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Medium distance (~0.005 deg away) -> trims 6 chars to "8F+6X".
    if (sn6 := olcShorten(full, 47.370, 8.530, *s_out)) {
        expect_eq(slicesEq(short_buf[0..sn6], "8F+6X"), true);
        if (rn6 := olcRecoverNearest(short_buf[0..sn6], 47.370, 8.530, *r_out)) {
            expect_eq(slicesEq(rec_buf[0..rn6], full), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Farther reference (~0.1 deg away) -> trims 4 chars to "9G8F+6X".
    if (sn4 := olcShorten(full, 47.45, 8.60, *s_out)) {
        expect_eq(slicesEq(short_buf[0..sn4], "9G8F+6X"), true);
        if (rn4 := olcRecoverNearest(short_buf[0..sn4], 47.45, 8.60, *r_out)) {
            expect_eq(slicesEq(rec_buf[0..rn4], full), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Cross-quadrant boundary recovery: "8FVC9G8F+6X" shortened to "9G8F+6X" recovered from lat 46.9 (south of 47.0 boundary!).
    if (rn_cross := olcRecoverNearest("9G8F+6X", 46.95, 8.50, *r_out)) {
        expect_eq(slicesEq(rec_buf[0..rn_cross], full), true);
    } else {
        expect_eq(true, false);
    }

    // Lowercase short code and already-full code recovery.
    if (rn_low := olcRecoverNearest("9g8f+6x", 47.45, 8.60, *r_out)) {
        expect_eq(slicesEq(rec_buf[0..rn_low], full), true);
    } else {
        expect_eq(true, false);
    }
    if (rn_full := olcRecoverNearest("8fvc9g8f+6x", 0.0, 0.0, *r_out)) {
        expect_eq(slicesEq(rec_buf[0..rn_full], full), true);
    } else {
        expect_eq(true, false);
    }
});

test("4. Case-insensitive validation and full vs short classification", () => {
    expect_eq(isValidOlc("8FVC9G8F+6X"), true);
    expect_eq(isValidOlc("8fvc9g8f+6x"), true);
    expect_eq(isFullOlc("8fvc9g8f+6x"), true);
    expect_eq(isShortOlc("8fvc9g8f+6x"), false);

    expect_eq(isValidOlc("9G8F+6X"), true);
    expect_eq(isShortOlc("9G8F+6X"), true);
    expect_eq(isFullOlc("9G8F+6X"), false);

    expect_eq(isValidOlc("8FVC0000+"), true);
    expect_eq(isFullOlc("8FVC0000+"), true);
    expect_eq(isShortOlc("8FVC0000+"), false);
});

test("5. Malformed OLC guards: multiple '+', odd '+', bad Base-20 chars, out-of-range first pair, invalid padding", () => {
    // Missing '+', multiple '+', odd '+' position, 1 char after '+'.
    expect_eq(isValidOlc(""), false);
    expect_eq(isValidOlc("+"), false);
    expect_eq(isValidOlc("8FVC9G8F6X"), false);
    expect_eq(isValidOlc("8FVC+9G8F+6X"), false);
    expect_eq(isValidOlc("8FVC9G8+6X"), false);
    expect_eq(isValidOlc("8FVC9G8F+6"), false);

    // Non-Base-20 characters ('1', 'A', 'I', 'O', 'U', 'L').
    expect_eq(isValidOlc("8FVC9G81+6X"), false);
    expect_eq(isValidOlc("8FVC9G8A+6X"), false);
    expect_eq(isValidOlc("8FVC9G8I+6X"), false);
    expect_eq(isValidOlc("8FVC9G8O+6X"), false);

    // Invalid padding: odd padding start, non-zero after zero, padding with suffix, padding in short code.
    expect_eq(isValidOlc("8FV00000+"), false);
    expect_eq(isValidOlc("8F009G00+"), false);
    expect_eq(isValidOlc("8FVC0000+6X"), false);
    expect_eq(isValidOlc("9G00+"), false);

    // Out-of-range first lat (>= 9, e.g. 'F' = 9) or first lng (>= 18, e.g. 'W' = 18) -> valid syntax, NOT full!
    expect_eq(isValidOlc("FFVC9G8F+6X"), true);
    expect_eq(isFullOlc("FFVC9G8F+6X"), false);
    expect_eq(isFullOlc("8WVC9G8F+6X"), false);
    if (_ := olcDecode("FFVC9G8F+6X")) {
        expect_eq(true, false);
    }

    // Invalid encode code_len (1, 3, 5, 7, 9, 16) and short buffer rejected.
    let buf: [20]u8 = std.mem.zeroes([20]u8);
    let out: []u8 = buf[0..];
    if (_ := olcEncode(0.0, 0.0, 1, *out)) { expect_eq(true, false); }
    if (_ := olcEncode(0.0, 0.0, 3, *out)) { expect_eq(true, false); }
    if (_ := olcEncode(0.0, 0.0, 9, *out)) { expect_eq(true, false); }
    if (_ := olcEncode(0.0, 0.0, 16, *out)) { expect_eq(true, false); }

    let tiny: [5]u8 = std.mem.zeroes([5]u8);
    let tiny_slice: []u8 = tiny[0..];
    if (_ := olcEncode(47.365590, 8.524997, 10, *tiny_slice)) {
        expect_eq(true, false);
    }
    // Padded code cannot be shortened.
    if (_ := olcShorten("8FVC9G00+", 47.365590, 8.524997, *out)) {
        expect_eq(true, false);
    }
});
