// @cap/number-prefix — Zero-heap SI decimal & IEC binary numeric prefix formatter in pure CP
// Clean-room MIT implementation of crates.io `number_prefix` (#738, 167.7M downloads).

pub enum PrefixKind {
    Standalone,
    Kilo,
    Mega,
    Giga,
    Tera,
    Peta,
    Exa,
    Kibi,
    Mebi,
    Gibi,
    Tebi,
    Pebi,
    Exbi,
}

pub struct PrefixedNumber {
    pub value: f64,
    pub prefix: PrefixKind,
    pub symbol: string,
    pub is_standalone: bool,
}

/// Computes SI decimal prefix (base 1000: k, M, G, T, P, E) for `amount`.
pub fn decimalPrefix(amount: f64): PrefixedNumber {
    let abs_v = amount;
    if (abs_v < 0.0) { abs_v = -abs_v; }
    if (abs_v < 1000.0) {
        return PrefixedNumber { value: amount, prefix: .Standalone, symbol: "", is_standalone: true };
    }
    const sign: f64 = if (amount < 0.0) -1.0 else 1.0;
    let v = abs_v;
    let idx: usize = 0;
    while (v >= 1000.0 && idx < 6) {
        v = v / 1000.0;
        idx += 1;
    }
    if (idx == 1) { return PrefixedNumber { value: sign * v, prefix: .Kilo, symbol: "k", is_standalone: false }; }
    if (idx == 2) { return PrefixedNumber { value: sign * v, prefix: .Mega, symbol: "M", is_standalone: false }; }
    if (idx == 3) { return PrefixedNumber { value: sign * v, prefix: .Giga, symbol: "G", is_standalone: false }; }
    if (idx == 4) { return PrefixedNumber { value: sign * v, prefix: .Tera, symbol: "T", is_standalone: false }; }
    if (idx == 5) { return PrefixedNumber { value: sign * v, prefix: .Peta, symbol: "P", is_standalone: false }; }
    return PrefixedNumber { value: sign * v, prefix: .Exa, symbol: "E", is_standalone: false };
}

/// Computes IEC binary prefix (base 1024: Ki, Mi, Gi, Ti, Pi, Ei) for `amount`.
pub fn binaryPrefix(amount: f64): PrefixedNumber {
    let abs_v = amount;
    if (abs_v < 0.0) { abs_v = -abs_v; }
    if (abs_v < 1024.0) {
        return PrefixedNumber { value: amount, prefix: .Standalone, symbol: "", is_standalone: true };
    }
    const sign: f64 = if (amount < 0.0) -1.0 else 1.0;
    let v = abs_v;
    let idx: usize = 0;
    while (v >= 1024.0 && idx < 6) {
        v = v / 1024.0;
        idx += 1;
    }
    if (idx == 1) { return PrefixedNumber { value: sign * v, prefix: .Kibi, symbol: "Ki", is_standalone: false }; }
    if (idx == 2) { return PrefixedNumber { value: sign * v, prefix: .Mebi, symbol: "Mi", is_standalone: false }; }
    if (idx == 3) { return PrefixedNumber { value: sign * v, prefix: .Gibi, symbol: "Gi", is_standalone: false }; }
    if (idx == 4) { return PrefixedNumber { value: sign * v, prefix: .Tebi, symbol: "Ti", is_standalone: false }; }
    if (idx == 5) { return PrefixedNumber { value: sign * v, prefix: .Pebi, symbol: "Pi", is_standalone: false }; }
    return PrefixedNumber { value: sign * v, prefix: .Exbi, symbol: "Ei", is_standalone: false };
}

fn npfWriteStr(s: string, out: []u8, pos: usize): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn npfWriteU64(val: u64, out: []u8, pos: usize): usize {
    let digits: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        digits[0] = 48;
        n = 1;
    } else {
        while (v > 0 && n < 24) {
            digits[n] = ((v % 10) as u8) + 48;
            v = v / 10;
            n += 1;
        }
    }
    let p = pos;
    let k = n;
    while (k > 0 && p < out.len) {
        k -= 1;
        out[p] = digits[k];
        p += 1;
    }
    return p;
}

/// Formats a `PrefixedNumber` with `decimals` (0..3) and a unit suffix (e.g. `"B"` -> `"1.50 KiB"`).
pub fn formatPrefixed(pn: PrefixedNumber, decimals: u8, unit: string, out: []u8): usize {
    let p: usize = 0;
    let val = pn.value;
    if (val < 0.0) {
        p = npfWriteStr("-", out, p);
        val = -val;
    }
    let d = decimals;
    if (d > 3) { d = 3; }
    if (pn.is_standalone) {
        d = 0;
    }
    let mult: u64 = 1;
    if (d == 1) { mult = 10; }
    else if (d == 2) { mult = 100; }
    else if (d == 3) { mult = 1000; }

    const scaled: u64 = (val * (mult as f64) + 0.5) as u64;
    const whole = scaled / mult;
    const frac = scaled % mult;
    p = npfWriteU64(whole, out, p);
    if (d > 0) {
        p = npfWriteStr(".", out, p);
        if (d == 2 && frac < 10) { p = npfWriteStr("0", out, p); }
        if (d == 3 && frac < 100) { p = npfWriteStr("0", out, p); }
        if (d == 3 && frac < 10) { p = npfWriteStr("0", out, p); }
        p = npfWriteU64(frac, out, p);
    }
    if (pn.symbol.len > 0 || unit.len > 0) {
        p = npfWriteStr(" ", out, p);
        p = npfWriteStr(pn.symbol, out, p);
        p = npfWriteStr(unit, out, p);
    }
    return p;
}

export {
    PrefixKind,
    PrefixedNumber,
    decimalPrefix,
    binaryPrefix,
    formatPrefixed,
};
