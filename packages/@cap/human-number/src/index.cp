// @cap/human-number — Zero-allocation human-readable number formatting engine for CP
// Clean-room MIT implementation inspired by npm `numeral`, `millify`, `number-to-words`,
// and Rust `num-format`.

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn absU64(n: i64) -> u64 {
    if (n >= 0) {
        return n as u64;
    }
    // Avoid overflow for i64::MIN (-9223372036854775808)
    return ((0 - (n + 1)) as u64) + 1;
}

fn appendU64(out: []u8, pos: usize, val: u64) -> usize {
    if (val == 0) {
        return appendByte(out, pos, '0');
    }
    let digits: [24]u8 = undefined;
    let len: usize = 0;
    let v = val;
    while (v > 0) {
        digits[len] = ('0' as u64 + (v % 10)) as u8;
        v /= 10;
        len += 1;
    }
    let p = pos;
    let k = len;
    while (k > 0) {
        k -= 1;
        p = appendByte(out, p, digits[k]);
    }
    return p;
}

// ---------------------------------------------------------------------------
// 1. Grouped Thousands Formatting
// ---------------------------------------------------------------------------

/// Formats `n` with a thousands separator `sep` (e.g. `','` -> `"1,234,567"`, `"-987,654"`).
pub fn formatGrouped(n: i64, sep: u8, out: []u8) -> string {
    let pos: usize = 0;
    if (n == 0) {
        pos = appendByte(out, pos, '0');
        return out[0..pos];
    }
    if (n < 0) {
        pos = appendByte(out, pos, '-');
    }
    let v = absU64(n);
    let digits: [24]u8 = undefined;
    let len: usize = 0;
    while (v > 0) {
        digits[len] = ('0' as u64 + (v % 10)) as u8;
        v /= 10;
        len += 1;
    }
    let k = len;
    while (k > 0) {
        k -= 1;
        pos = appendByte(out, pos, digits[k]);
        if (k > 0 && (k % 3) == 0 && sep != 0) {
            pos = appendByte(out, pos, sep);
        }
    }
    return out[0..pos];
}

// ---------------------------------------------------------------------------
// 2. Compact Formatting (English & Chinese)
// ---------------------------------------------------------------------------

fn writeCompactScaled(
    abs: u64,
    unit: u64,
    suffix: string,
    next_threshold: u64,
    next_suffix: string,
    negative: bool,
    out: []u8,
) -> string {
    let whole = abs / unit;
    const rem = abs % unit;
    let tenths = (rem * 10 + (unit / 2)) / unit;
    if (tenths >= 10) {
        whole += 1;
        tenths = 0;
    }

    let pos: usize = 0;
    if (negative) {
        pos = appendByte(out, pos, '-');
    }
    if (next_threshold > 0 && whole >= next_threshold) {
        pos = appendU64(out, pos, whole / next_threshold);
        pos = appendStr(out, pos, next_suffix);
        return out[0..pos];
    }
    pos = appendU64(out, pos, whole);
    if (tenths > 0) {
        pos = appendByte(out, pos, '.');
        pos = appendByte(out, pos, ('0' as u64 + tenths) as u8);
    }
    pos = appendStr(out, pos, suffix);
    return out[0..pos];
}

/// Formats `n` with English compact suffixes (`K`, `M`, `B`, `T`), e.g. `1200` -> `"1.2K"`.
pub fn formatCompactEn(n: i64, out: []u8) -> string {
    const neg = n < 0;
    const abs = absU64(n);
    if (abs < 1000) {
        let pos: usize = 0;
        if (neg) {
            pos = appendByte(out, pos, '-');
        }
        pos = appendU64(out, pos, abs);
        return out[0..pos];
    }
    if (abs < 1000000) {
        return writeCompactScaled(abs, 1000, "K", 1000, "M", neg, out);
    }
    if (abs < 1000000000) {
        return writeCompactScaled(abs, 1000000, "M", 1000, "B", neg, out);
    }
    if (abs < 1000000000000) {
        return writeCompactScaled(abs, 1000000000, "B", 1000, "T", neg, out);
    }
    return writeCompactScaled(abs, 1000000000000, "T", 0, "", neg, out);
}

/// Formats `n` with Chinese compact units (`万`, `亿`, `万亿`), e.g. `250000000` -> `"2.5亿"`.
pub fn formatCompactZh(n: i64, out: []u8) -> string {
    const neg = n < 0;
    const abs = absU64(n);
    if (abs < 10000) {
        let pos: usize = 0;
        if (neg) {
            pos = appendByte(out, pos, '-');
        }
        pos = appendU64(out, pos, abs);
        return out[0..pos];
    }
    if (abs < 100000000) {
        return writeCompactScaled(abs, 10000, "万", 10000, "亿", neg, out);
    }
    if (abs < 1000000000000) {
        return writeCompactScaled(abs, 100000000, "亿", 10000, "万亿", neg, out);
    }
    return writeCompactScaled(abs, 1000000000000, "万亿", 0, "", neg, out);
}

// ---------------------------------------------------------------------------
// 3. Percentage Formatting
// ---------------------------------------------------------------------------

/// Formats `ratio` (e.g. `0.9995`) as a percentage string with `decimals` fractional digits
/// (e.g. `formatPercent(0.9995, 2, out)` -> `"99.95%"`).
pub fn formatPercent(ratio: f64, decimals: u8, out: []u8) -> string {
    let d: u8 = decimals;
    if (d > 9) {
        d = 9;
    }
    let scale: u64 = 1;
    let s_i: u8 = 0;
    while (s_i < d) {
        scale *= 10;
        s_i += 1;
    }

    const pct = ratio * 100.0;
    const neg = pct < 0.0;
    const abs_pct = if (neg) -pct else pct;
    const scaled_f = abs_pct * (scale as f64) + 0.5;
    const scaled = scaled_f as u64;

    let pos: usize = 0;
    if (neg && scaled > 0) {
        pos = appendByte(out, pos, '-');
    }

    const int_part = scaled / scale;
    const frac_part = scaled % scale;
    pos = appendU64(out, pos, int_part);

    if (d > 0) {
        pos = appendByte(out, pos, '.');
        let frac_buf: [12]u8 = undefined;
        let f_val = frac_part;
        let idx: usize = d as usize;
        while (idx > 0) {
            idx -= 1;
            frac_buf[idx] = ('0' as u64 + (f_val % 10)) as u8;
            f_val /= 10;
        }
        let k: usize = 0;
        while (k < (d as usize)) {
            pos = appendByte(out, pos, frac_buf[k]);
            k += 1;
        }
    }

    pos = appendByte(out, pos, '%');
    return out[0..pos];
}

// ---------------------------------------------------------------------------
// 4. Strict Roman Numerals (1..3999)
// ---------------------------------------------------------------------------

/// Converts `n` (`1..=3999`) to a canonical uppercase Roman numeral string.
/// Returns `""` if `n == 0` or `n > 3999`.
pub fn toRoman(n: u16, out: []u8) -> string {
    if (n == 0 || n > 3999) {
        return out[0..0];
    }
    let rem = n;
    let pos: usize = 0;
    while (rem >= 1000) {
        pos = appendByte(out, pos, 'M');
        rem -= 1000;
    }
    if (rem >= 900) {
        pos = appendStr(out, pos, "CM");
        rem -= 900;
    }
    if (rem >= 500) {
        pos = appendByte(out, pos, 'D');
        rem -= 500;
    }
    if (rem >= 400) {
        pos = appendStr(out, pos, "CD");
        rem -= 400;
    }
    while (rem >= 100) {
        pos = appendByte(out, pos, 'C');
        rem -= 100;
    }
    if (rem >= 90) {
        pos = appendStr(out, pos, "XC");
        rem -= 90;
    }
    if (rem >= 50) {
        pos = appendByte(out, pos, 'L');
        rem -= 50;
    }
    if (rem >= 40) {
        pos = appendStr(out, pos, "XL");
        rem -= 40;
    }
    while (rem >= 10) {
        pos = appendByte(out, pos, 'X');
        rem -= 10;
    }
    if (rem >= 9) {
        pos = appendStr(out, pos, "IX");
        rem -= 9;
    }
    if (rem >= 5) {
        pos = appendByte(out, pos, 'V');
        rem -= 5;
    }
    if (rem >= 4) {
        pos = appendStr(out, pos, "IV");
        rem -= 4;
    }
    while (rem >= 1) {
        pos = appendByte(out, pos, 'I');
        rem -= 1;
    }
    return out[0..pos];
}

fn romanCharVal(c: u8) -> u16 {
    switch (c) {
        'I', 'i' => return 1,
        'V', 'v' => return 5,
        'X', 'x' => return 10,
        'L', 'l' => return 50,
        'C', 'c' => return 100,
        'D', 'd' => return 500,
        'M', 'm' => return 1000,
        else => return 0,
    }
}

fn toUpperAscii(c: u8) -> u8 {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

/// Strictly parses a canonical Roman numeral (`1..=3999`, case-insensitive).
/// Returns `null` for empty strings, non-Roman characters, or non-canonical forms (e.g. `"IIII"`, `"IC"`, `"VV"`).
pub fn fromRoman(s: string) -> u16? {
    if (s.len == 0 || s.len > 15) {
        return null;
    }
    let total: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const cur = romanCharVal(s[i]);
        if (cur == 0) {
            return null;
        }
        if (i + 1 < s.len) {
            const next = romanCharVal(s[i + 1]);
            if (next == 0) {
                return null;
            }
            if (cur < next) {
                total += (next - cur) as u32;
                i += 2;
                continue;
            }
        }
        total += cur as u32;
        i += 1;
    }
    if (total == 0 || total > 3999) {
        return null;
    }

    // Canonical round-trip verification guarantees strict RFC/standard Roman syntax
    const val = total as u16;
    let canon_buf: [16]u8 = undefined;
    const canon = toRoman(val, canon_buf[0..16]);
    if (canon.len != s.len) {
        return null;
    }
    let k: usize = 0;
    while (k < s.len) {
        if (toUpperAscii(s[k]) != canon[k]) {
            return null;
        }
        k += 1;
    }
    return val;
}

// ---------------------------------------------------------------------------
// 5. English Number Words
// ---------------------------------------------------------------------------

fn onesWord(n: u64) -> string {
    switch (n) {
        1 => return "one",
        2 => return "two",
        3 => return "three",
        4 => return "four",
        5 => return "five",
        6 => return "six",
        7 => return "seven",
        8 => return "eight",
        9 => return "nine",
        10 => return "ten",
        11 => return "eleven",
        12 => return "twelve",
        13 => return "thirteen",
        14 => return "fourteen",
        15 => return "fifteen",
        16 => return "sixteen",
        17 => return "seventeen",
        18 => return "eighteen",
        19 => return "nineteen",
        else => return "",
    }
}

fn tensWord(n: u64) -> string {
    switch (n) {
        2 => return "twenty",
        3 => return "thirty",
        4 => return "forty",
        5 => return "fifty",
        6 => return "sixty",
        7 => return "seventy",
        8 => return "eighty",
        9 => return "ninety",
        else => return "",
    }
}

fn appendUnder1000(v: u64, out: []u8, start_pos: usize) -> usize {
    let pos = start_pos;
    const hundreds = v / 100;
    const rem = v % 100;
    if (hundreds > 0) {
        pos = appendStr(out, pos, onesWord(hundreds));
        pos = appendStr(out, pos, " hundred");
        if (rem > 0) {
            pos = appendByte(out, pos, ' ');
        }
    }
    if (rem >= 20) {
        pos = appendStr(out, pos, tensWord(rem / 10));
        const unit = rem % 10;
        if (unit > 0) {
            pos = appendByte(out, pos, '-');
            pos = appendStr(out, pos, onesWord(unit));
        }
    } else if (rem > 0) {
        pos = appendStr(out, pos, onesWord(rem));
    }
    return pos;
}

fn appendScaleGroup(chunk: u64, label: string, out: []u8, start_pos: usize, has_prev: bool) -> usize {
    if (chunk == 0) {
        return start_pos;
    }
    let pos = start_pos;
    if (has_prev) {
        pos = appendByte(out, pos, ' ');
    }
    pos = appendUnder1000(chunk, out, pos);
    if (label.len > 0) {
        pos = appendByte(out, pos, ' ');
        pos = appendStr(out, pos, label);
    }
    return pos;
}

/// Spells out `n` in English words (e.g. `42` -> `"forty-two"`, `-42` -> `"minus forty-two"`,
/// `1234` -> `"one thousand two hundred thirty-four"`). Supports the full `i64` range.
pub fn toEnglishWords(n: i64, out: []u8) -> string {
    if (n == 0) {
        const len = appendStr(out, 0 as usize, "zero");
        return out[0..len];
    }

    let pos: usize = 0;
    if (n < 0) {
        pos = appendStr(out, pos, "minus ");
    }

    let rem = absU64(n);
    let has_prev = false;

    const quintillions = rem / 1000000000000000000;
    rem %= 1000000000000000000;
    if (quintillions > 0) {
        pos = appendScaleGroup(quintillions, "quintillion", out, pos, has_prev);
        has_prev = true;
    }

    const quadrillions = rem / 1000000000000000;
    rem %= 1000000000000000;
    if (quadrillions > 0) {
        pos = appendScaleGroup(quadrillions, "quadrillion", out, pos, has_prev);
        has_prev = true;
    }

    const trillions = rem / 1000000000000;
    rem %= 1000000000000;
    if (trillions > 0) {
        pos = appendScaleGroup(trillions, "trillion", out, pos, has_prev);
        has_prev = true;
    }

    const billions = rem / 1000000000;
    rem %= 1000000000;
    if (billions > 0) {
        pos = appendScaleGroup(billions, "billion", out, pos, has_prev);
        has_prev = true;
    }

    const millions = rem / 1000000;
    rem %= 1000000;
    if (millions > 0) {
        pos = appendScaleGroup(millions, "million", out, pos, has_prev);
        has_prev = true;
    }

    const thousands = rem / 1000;
    rem %= 1000;
    if (thousands > 0) {
        pos = appendScaleGroup(thousands, "thousand", out, pos, has_prev);
        has_prev = true;
    }

    if (rem > 0) {
        pos = appendScaleGroup(rem, "", out, pos, has_prev);
    }

    return out[0..pos];
}
