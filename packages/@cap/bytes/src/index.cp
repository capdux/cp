// @cap/bytes — Zero-heap byte size parser & formatter (IEC binary & SI decimal)
// Clean-room MIT implementation for CP (compatible with npm bytes, pretty-bytes & Rust bytesize).

pub const KIB: u64 = 1024;
pub const MIB: u64 = 1048576;
pub const GIB: u64 = 1073741824;
pub const TIB: u64 = 1099511627776;
pub const PIB: u64 = 1125899906842624;

pub const KB: u64 = 1000;
pub const MB: u64 = 1000000;
pub const GB: u64 = 1000000000;
pub const TB: u64 = 1000000000000;
pub const PB: u64 = 1000000000000000;

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn unitMultiplier(unit: string, binary_default: bool): u64 {
    if (unit.len == 0) {
        return 1;
    }
    let c0: u8 = toLower(unit[0]);
    if (unit.len == 1) {
        if (c0 == 'b') { return 1; }
        if (c0 == 'k') { if (binary_default) { return KIB; } return KB; }
        if (c0 == 'm') { if (binary_default) { return MIB; } return MB; }
        if (c0 == 'g') { if (binary_default) { return GIB; } return GB; }
        if (c0 == 't') { if (binary_default) { return TIB; } return TB; }
        if (c0 == 'p') { if (binary_default) { return PIB; } return PB; }
        return 0;
    }
    let c1: u8 = toLower(unit[1]);
    if (unit.len == 2 && c1 == 'b') {
        if (c0 == 'k') { if (binary_default) { return KIB; } return KB; }
        if (c0 == 'm') { if (binary_default) { return MIB; } return MB; }
        if (c0 == 'g') { if (binary_default) { return GIB; } return GB; }
        if (c0 == 't') { if (binary_default) { return TIB; } return TB; }
        if (c0 == 'p') { if (binary_default) { return PIB; } return PB; }
        return 0;
    }
    if (unit.len == 3 && c1 == 'i' && toLower(unit[2]) == 'b') {
        if (c0 == 'k') { return KIB; }
        if (c0 == 'm') { return MIB; }
        if (c0 == 'g') { return GIB; }
        if (c0 == 't') { return TIB; }
        if (c0 == 'p') { return PIB; }
        return 0;
    }
    return 0;
}

/// Parse a human-readable byte string such as `"100KB"`, `"1.5 GiB"`, `"512mb"`, `"2048"`.
/// When `binary_default` is true (npm `bytes` compatible), `KB`/`MB`/`GB` use base 1024;
/// when false (SI / Rust `bytesize` compatible), `KB`/`MB`/`GB` use base 1000 while `KiB`/`MiB` always use 1024.
/// Returns byte count >= 0 on success, or -1 on invalid/overflow input.
pub fn parseBytes(input: string, binary_default: bool): i64 {
    let start: usize = 0;
    let end: usize = input.len;
    while (start < end && isSpace(input[start])) {
        start += 1;
    }
    while (end > start && isSpace(input[end - 1])) {
        end -= 1;
    }
    if (start >= end) {
        return -1;
    }

    let i: usize = start;
    let int_part: u64 = 0;
    let has_digit: bool = false;
    while (i < end && input[i] >= '0' && input[i] <= '9') {
        has_digit = true;
        let d: u64 = (input[i] - '0') as u64;
        if (int_part > 900000000000000000) {
            return -1;
        }
        int_part = int_part * 10 + d;
        i += 1;
    }

    let frac_part: u64 = 0;
    let frac_scale: u64 = 1;
    if (i < end && input[i] == '.') {
        i += 1;
        while (i < end && input[i] >= '0' && input[i] <= '9') {
            has_digit = true;
            if (frac_scale < 1000000) {
                frac_part = frac_part * 10 + ((input[i] - '0') as u64);
                frac_scale = frac_scale * 10;
            }
            i += 1;
        }
    }

    if (!has_digit) {
        return -1;
    }

    while (i < end && isSpace(input[i])) {
        i += 1;
    }
    let unit: string = input[i..end];
    let mult: u64 = unitMultiplier(unit, binary_default);
    if (mult == 0) {
        return -1;
    }

    let val_f: f64 = ((int_part as f64) + ((frac_part as f64) / (frac_scale as f64))) * (mult as f64);
    if (val_f > 9000000000000000000.0) {
        return -1;
    }
    return (val_f + 0.5) as i64;
}

/// Convenience wrapper matching npm `bytes.parse()` (base-1024 default for KB/MB/GB).
pub fn parse(input: string): i64 {
    return parseBytes(input, true);
}

fn writeU64(val: u64, out: []u8): usize {
    if (val == 0) {
        if (out.len == 0) { return 0; }
        out[0] = '0';
        return 1;
    }
    let tmp: [24]u8 = undefined;
    let n: usize = 0;
    let v: u64 = val;
    while (v > 0) {
        tmp[n] = ('0' as u8) + ((v % 10) as u8);
        v = v / 10;
        n += 1;
    }
    if (n > out.len) {
        return 0;
    }
    let i: usize = 0;
    while (i < n) {
        out[i] = tmp[n - 1 - i];
        i += 1;
    }
    return n;
}

fn writeStr(s: string, out: []u8): usize {
    if (s.len > out.len) {
        return 0;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[i] = s[i];
        i += 1;
    }
    return s.len;
}

/// Format a byte count into human-readable string (e.g. `"1.50 MiB"` or `"1.50 MB"`).
/// `binary = true` uses base 1024 (`B`, `KiB`, `MiB`, `GiB`, `TiB`, `PiB`);
/// `binary = false` uses base 1000 (`B`, `KB`, `MB`, `GB`, `TB`, `PB`).
/// `decimals` specifies 0..3 fractional digits (ignored for bytes `< base`).
pub fn formatBytes(bytes: u64, binary: bool, decimals: u32, out: []u8): usize {
    let base: u64 = KB;
    if (binary) {
        base = KIB;
    }
    if (bytes < base) {
        let pos: usize = writeU64(bytes, out);
        if (pos == 0 || pos + 2 > out.len) { return 0; }
        out[pos] = ' ';
        out[pos + 1] = 'B';
        return pos + 2;
    }

    let unit_idx: usize = 0;
    let divisor: u64 = 1;
    while (unit_idx < 5 && (bytes / divisor) >= base) {
        divisor = divisor * base;
        unit_idx += 1;
    }

    let dec_places: u32 = decimals;
    if (dec_places > 3) {
        dec_places = 3;
    }
    let pow10: u64 = 1;
    let d: u32 = 0;
    while (d < dec_places) {
        pow10 = pow10 * 10;
        d += 1;
    }

    let scaled_f: f64 = ((bytes as f64) / (divisor as f64)) * (pow10 as f64) + 0.5;
    let scaled: u64 = scaled_f as u64;
    let whole: u64 = scaled / pow10;
    let frac: u64 = scaled % pow10;

    let pos: usize = writeU64(whole, out);
    if (pos == 0) { return 0; }

    if (dec_places > 0) {
        if (pos + 1 + (dec_places as usize) > out.len) { return 0; }
        out[pos] = '.';
        pos += 1;
        let rem: u64 = frac;
        let div10: u64 = pow10 / 10;
        let k: u32 = 0;
        while (k < dec_places) {
            let dig: u64 = rem / div10;
            out[pos] = ('0' as u8) + (dig as u8);
            pos += 1;
            rem = rem % div10;
            if (div10 >= 10) {
                div10 = div10 / 10;
            }
            k += 1;
        }
    }

    if (pos >= out.len) { return 0; }
    out[pos] = ' ';
    pos += 1;

    let suffix: string = "B";
    if (binary) {
        if (unit_idx == 1) { suffix = "KiB"; }
        if (unit_idx == 2) { suffix = "MiB"; }
        if (unit_idx == 3) { suffix = "GiB"; }
        if (unit_idx == 4) { suffix = "TiB"; }
        if (unit_idx == 5) { suffix = "PiB"; }
    } else {
        if (unit_idx == 1) { suffix = "KB"; }
        if (unit_idx == 2) { suffix = "MB"; }
        if (unit_idx == 3) { suffix = "GB"; }
        if (unit_idx == 4) { suffix = "TB"; }
        if (unit_idx == 5) { suffix = "PB"; }
    }

    let sn: usize = writeStr(suffix, out[pos..]);
    if (sn == 0) { return 0; }
    return pos + sn;
}

/// Format using IEC binary units (`KiB`, `MiB`, `GiB`) with 2 decimal places.
pub fn formatIec(bytes: u64, out: []u8): usize {
    return formatBytes(bytes, true, 2, out);
}

/// Format using SI decimal units (`KB`, `MB`, `GB`) with 2 decimal places.
pub fn formatSi(bytes: u64, out: []u8): usize {
    return formatBytes(bytes, false, 2, out);
}
