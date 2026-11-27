// @cap/roman-num — Zero-allocation Strict Roman Numeral (1..3999) Parser & Formatter
// Clean-room MIT implementation of npm `romans` in pure CP, reusing `cap:ascii` & `cap:fmt`.

import { eqIgnoreCase, toUpperByte, toLowerByte } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub const MIN_ROMAN: u16 = 1;
pub const MAX_ROMAN: u16 = 3999;
pub const MAX_ROMAN_LEN: usize = 15; // "MMMDCCCLXXXVIII" (3888)

/// Returns the numeric value of a single Roman numeral character (`I..M`, case-insensitive),
/// or `-1` if `b` is not a Roman numeral character.
pub fn romanCharValue(b: u8) -> i32 {
    const u = toUpperByte(b);
    if (u == 73) { return 1; }    // I
    if (u == 86) { return 5; }    // V
    if (u == 88) { return 10; }   // X
    if (u == 76) { return 50; }   // L
    if (u == 67) { return 100; }  // C
    if (u == 68) { return 500; }  // D
    if (u == 77) { return 1000; } // M
    return -1;
}

/// Computes the exact byte length of the canonical Roman numeral for `num` (`1..3999`),
/// or `0` if `num` is out of range.
pub fn romanLen(num: u16) -> usize {
    if (num == 0 || num >= 4000) { return 0; }
    const vals: [13]u16 = [1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1];
    const lens: [13]usize = [1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1];
    let rem = num;
    let total: usize = 0;
    let i: usize = 0;
    while (i < 13 && rem > 0) {
        while (rem >= vals[i]) {
            total += lens[i];
            rem -= vals[i];
        }
        i += 1;
    }
    return total;
}

/// Formats an integer in `1..3999` as a canonical uppercase Roman numeral into `out`.
/// Returns bytes written, or `0` if `num == 0`, `num >= 4000`, or `out` is too small.
pub fn toRoman(num: u16, out: []u8) -> usize {
    if (num == 0 || num >= 4000) {
        return 0;
    }
    const vals: [13]u16 = [1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1];
    const syms: [13]string = ["M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"];

    let rem = num;
    let w: usize = 0;
    let i: usize = 0;
    while (i < 13 && rem > 0) {
        while (rem >= vals[i]) {
            const sym = syms[i];
            if (w + sym.len > out.len) {
                return 0;
            }
            w = writeStr(sym, out, w);
            rem -= vals[i];
        }
        i += 1;
    }
    return w;
}

/// Formats an integer in `1..3999` as a canonical lowercase Roman numeral into `out`.
pub fn toRomanLower(num: u16, out: []u8) -> usize {
    let n = toRoman(num, out);
    let i: usize = 0;
    while (i < n) {
        out[i] = toLowerByte(out[i]);
        i += 1;
    }
    return n;
}

/// Upstream npm `romans.romanize(n)`: formats `1..3999` into `out`, returning bytes written
/// or `0` if `num < 1`, `num > 3999`, or `out` is too small.
pub fn romanize(num: i32, out: []u8) -> usize {
    if (num < 1 || num > 3999) { return 0; }
    return toRoman(num as u16, out);
}

/// Checked variant of `romanize`: returns `null` if `num` is out of `1..3999` or `out` is too small.
pub fn romanizeChecked(num: i32, out: []u8) -> usize? {
    let n = romanize(num, out);
    if (n == 0) { return null; }
    return n;
}

/// Strictly parses a canonical Roman numeral (`I..MMMCMXCIX`, case-insensitive).
/// Rejects non-canonical forms such as `IIII`, `VV`, `IL`, `IC`, `XM`, or out-of-range strings.
/// Returns `1..3999`, or `-1` if invalid.
pub fn parseRoman(input: string) -> i32 {
    const n = input.len;
    if (n == 0 || n > 15) {
        return -1;
    }

    let total: i32 = 0;
    let i: usize = 0;
    while (i < n) {
        const cur = romanCharValue(input[i]);
        if (cur < 0) {
            return -1;
        }
        if (i + 1 < n) {
            const nxt = romanCharValue(input[i + 1]);
            if (nxt < 0) {
                return -1;
            }
            if (cur < nxt) {
                total -= cur;
            } else {
                total += cur;
            }
        } else {
            total += cur;
        }
        i += 1;
    }

    if (total < 1 || total > 3999) {
        return -1;
    }

    // Canonical round-trip check enforces strict Roman numeral grammar
    let canon: [16]u8 = undefined;
    const clen = toRoman(total as u16, canon[0..16]);
    if (!eqIgnoreCase(input, canon[0..clen])) {
        return -1;
    }

    return total;
}

/// Upstream npm `romans.deromanize(s)`: strictly parses a canonical Roman numeral (`1..3999`).
/// Returns `null` for non-canonical or invalid inputs (`IIII`, `VV`, `IC`, empty, etc.).
pub fn deromanize(input: string) -> i32? {
    let v = parseRoman(input);
    if (v < 0) { return null; }
    return v;
}

/// Returns true if `input` is a strictly valid canonical Roman numeral (`1..3999`).
pub fn isValidRoman(input: string) -> bool {
    return parseRoman(input) > 0;
}
