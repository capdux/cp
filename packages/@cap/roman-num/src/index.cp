// @cap/roman-num — Zero-allocation Strict Roman Numeral (1..3999) Parser & Formatter
// Clean-room MIT implementation in pure CP.

fn toUpperAscii(b: u8) -> u8 {
    if (b >= 97 && b <= 122) {
        return b - 32;
    }
    return b;
}

fn romanCharVal(b: u8) -> i32 {
    const u = toUpperAscii(b);
    if (u == 73) { return 1; }    // I
    if (u == 86) { return 5; }    // V
    if (u == 88) { return 10; }   // X
    if (u == 76) { return 50; }   // L
    if (u == 67) { return 100; }  // C
    if (u == 68) { return 500; }  // D
    if (u == 77) { return 1000; } // M
    return -1;
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
            let k: usize = 0;
            while (k < sym.len) {
                out[w] = sym[k];
                w += 1;
                k += 1;
            }
            rem -= vals[i];
        }
        i += 1;
    }
    return w;
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
        const cur = romanCharVal(input[i]);
        if (cur < 0) {
            return -1;
        }
        if (i + 1 < n) {
            const nxt = romanCharVal(input[i + 1]);
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
    if (clen != n) {
        return -1;
    }
    let k: usize = 0;
    while (k < n) {
        if (toUpperAscii(input[k]) != canon[k]) {
            return -1;
        }
        k += 1;
    }

    return total;
}

/// Returns true if `input` is a strictly valid canonical Roman numeral (`1..3999`).
pub fn isValidRoman(input: string) -> bool {
    return parseRoman(input) > 0;
}
