// Source: Absorbed from /data/dev/bun/src/fmt.zig | Synced: 2026-11-21T15:40:53Z (Human Integer Grouping, Radix & Ordinal Formatter)
// Pure-CP zero-allocation integer formatter: thousands grouping, fixed-point decimals, ordinals, and radix formatting.

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

/// Format an unsigned integer with 3-digit grouping separator (e.g. `1234567` + `','` -> `"1,234,567"`).
export fn formatGroupedU64(val: u64, sep: u8, out: []u8): ([]const u8)? {
    let digits: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        digits[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            digits[n] = @as(u8, '0') + @as(u8, v % 10);
            v /= 10;
            n += 1;
        }
    }

    let pos: usize = 0;
    let i: usize = 0;
    while (i < n) {
        const idx = n - 1 - i;
        if (i > 0 && ((n - i) % 3) == 0) {
            if (!writeByte(out, &pos, sep)) return null;
        }
        if (!writeByte(out, &pos, digits[idx])) return null;
        i += 1;
    }
    return out[0..pos];
}

/// Format a signed integer with 3-digit grouping separator (e.g. `-1234567` -> `"-1,234,567"`).
export fn formatGroupedI64(val: i64, sep: u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let mag: u64 = 0;
    if (val < 0) {
        if (!writeByte(out, &pos, '-')) return null;
        // Safe magnitude for i64::MIN (-9223372036854775808)
        if (val == -9223372036854775807 - 1) {
            mag = 9223372036854775808;
        } else {
            mag = @as(u64, 0 - val);
        }
    } else {
        mag = @as(u64, val);
    }
    const rest = formatGroupedU64(mag, sep, out[pos..]) ?? return null;
    return out[0..pos + rest.len];
}

/// English ordinal suffix for `n` (`"st"`, `"nd"`, `"rd"`, `"th"`).
export fn ordinalSuffix(n: u64): []const u8 {
    const mod100 = n % 100;
    if (mod100 >= 11 && mod100 <= 13) return "th";
    const mod10 = n % 10;
    if (mod10 == 1) return "st";
    if (mod10 == 2) return "nd";
    if (mod10 == 3) return "rd";
    return "th";
}

/// Format English ordinal (e.g. `1` -> `"1st"`, `11` -> `"11th"`, `22` -> `"22nd"`).
export fn formatOrdinal(n: u64, out: []u8): ([]const u8)? {
    let digits: [24]u8 = [0; 24];
    let len: usize = 0;
    let v = n;
    if (v == 0) {
        digits[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            digits[len] = @as(u8, '0') + @as(u8, v % 10);
            v /= 10;
            len += 1;
        }
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < len) {
        if (!writeByte(out, &pos, digits[len - 1 - i])) return null;
        i += 1;
    }
    if (!writeSlice(out, &pos, ordinalSuffix(n))) return null;
    return out[0..pos];
}

/// Format an unsigned integer in base `radix` (`2..=36`) with optional uppercase digits.
export fn formatRadixU64(val: u64, radix: u8, uppercase: bool, out: []u8): ([]const u8)? {
    if (radix < 2 || radix > 36) return null;
    const base: u64 = @as(u64, radix);
    let buf: [64]u8 = [0; 64];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        buf[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            const d: u8 = @as(u8, v % base);
            if (d < 10) {
                buf[n] = @as(u8, '0') + d;
            } else {
                const alpha: u8 = if (uppercase) @as(u8, 'A') else @as(u8, 'a');
                buf[n] = alpha + (d - 10);
            }
            v /= base;
            n += 1;
        }
    }
    if (n > out.len) return null;
    let i: usize = 0;
    while (i < n) {
        out[i] = buf[n - 1 - i];
        i += 1;
    }
    return out[0..n];
}

/// Format a fixed-point scaled integer `scaled_val` with `decimals` fractional digits and grouping separator
/// (e.g. `123456` with `decimals=2, sep=','` -> `"1,234.56"`).
export fn formatFixedPoint(scaled_val: i64, decimals: u8, sep: u8, out: []u8): ([]const u8)? {
    if (decimals > 9) return null;
    let scale: u64 = 1;
    let d: u8 = 0;
    while (d < decimals) {
        scale *= 10;
        d += 1;
    }

    let pos: usize = 0;
    let mag: u64 = 0;
    if (scaled_val < 0) {
        if (!writeByte(out, &pos, '-')) return null;
        if (scaled_val == -9223372036854775807 - 1) {
            mag = 9223372036854775808;
        } else {
            mag = @as(u64, 0 - scaled_val);
        }
    } else {
        mag = @as(u64, scaled_val);
    }

    const whole = mag / scale;
    const frac = mag % scale;
    const w_str = formatGroupedU64(whole, sep, out[pos..]) ?? return null;
    pos += w_str.len;

    if (decimals > 0) {
        if (!writeByte(out, &pos, '.')) return null;
        let fbuf: [16]u8 = [0; 16];
        let fv = frac;
        let idx: usize = @as(usize, decimals);
        while (idx > 0) {
            idx -= 1;
            fbuf[idx] = @as(u8, '0') + @as(u8, fv % 10);
            fv /= 10;
        }
        if (!writeSlice(out, &pos, fbuf[0..@as(usize, decimals)])) return null;
    }
    return out[0..pos];
}
