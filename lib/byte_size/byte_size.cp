// Source: Absorbed from /data/dev/bun/src/fmt.zig (Human Byte-Size Parser & SI/IEC Formatter)
// Pure-CP zero-allocation parser for byte sizes ("10KB", "1.5GiB", "512B") and SI/IEC formatters.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) start += 1;
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn unitMultiplier(unit: []const u8): u64? {
    if (unit.len == 0 || eqIgnoreCase(unit, "b")) return 1;
    // IEC binary units (1024-based)
    if (eqIgnoreCase(unit, "kib")) return 1024;
    if (eqIgnoreCase(unit, "mib")) return 1048576;
    if (eqIgnoreCase(unit, "gib")) return 1073741824;
    if (eqIgnoreCase(unit, "tib")) return 1099511627776;
    if (eqIgnoreCase(unit, "pib")) return 1125899906842624;
    // SI decimal units (1000-based)
    if (eqIgnoreCase(unit, "k") || eqIgnoreCase(unit, "kb")) return 1000;
    if (eqIgnoreCase(unit, "m") || eqIgnoreCase(unit, "mb")) return 1000000;
    if (eqIgnoreCase(unit, "g") || eqIgnoreCase(unit, "gb")) return 1000000000;
    if (eqIgnoreCase(unit, "t") || eqIgnoreCase(unit, "tb")) return 1000000000000;
    if (eqIgnoreCase(unit, "p") || eqIgnoreCase(unit, "pb")) return 1000000000000000;
    return null;
}

/// Parse a human-readable byte size (e.g. `"512"`, `"10KB"`, `"1.5 GiB"`, `"2.25MB"`) into exact `u64` bytes.
/// Uses pure integer arithmetic to avoid floating-point drift; rejects negative, unknown units, and overflow.
export fn parseByteSize(input: []const u8): u64? {
    const s = trimWs(input);
    if (s.len == 0) return null;

    let i: usize = 0;
    let int_part: u64 = 0;
    let has_int_digit = false;
    while (i < s.len && s[i] >= '0' && s[i] <= '9') {
        has_int_digit = true;
        const d: u64 = @as(u64, s[i] - '0');
        if (int_part > 1844674407370955161) return null;
        if (int_part == 1844674407370955161 && d > 5) return null;
        int_part = int_part * 10 + d;
        i += 1;
    }

    let frac_num: u64 = 0;
    let frac_den: u64 = 1;
    let has_frac_digit = false;
    if (i < s.len && s[i] == '.') {
        i += 1;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') {
            has_frac_digit = true;
            if (frac_den < 1000000000) {
                frac_num = frac_num * 10 + @as(u64, s[i] - '0');
                frac_den *= 10;
            }
            i += 1;
        }
        if (!has_frac_digit) return null;
    }
    if (!has_int_digit && !has_frac_digit) return null;

    const unit = trimWs(s[i..]);
    const mult = unitMultiplier(unit) ?? return null;

    // Overflow check for int_part * mult
    if (int_part > 0 && mult > 18446744073709551615 / int_part) return null;
    const base = int_part * mult;

    // Add fractional bytes: (frac_num * mult) / frac_den without u64 overflow
    let frac_bytes: u64 = 0;
    if (frac_num > 0) {
        if (mult >= frac_den) {
            frac_bytes = (mult / frac_den) * frac_num + ((mult % frac_den) * frac_num) / frac_den;
        } else {
            frac_bytes = (frac_num * mult) / frac_den;
        }
    }

    if (frac_bytes > 18446744073709551615 - base) return null;
    return base + frac_bytes;
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

fn writeU64(out: []u8, pos: *usize, val: u64): bool {
    if (val == 0) return writeSlice(out, pos, "0");
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = @as(u8, '0') + @as(u8, v % 10);
        v /= 10;
        n += 1;
    }
    if (pos.* + n > out.len) return false;
    let i: usize = 0;
    while (i < n) {
        out[pos.* + i] = tmp[n - 1 - i];
        i += 1;
    }
    pos.* += n;
    return true;
}

fn formatScaled(bytes: u64, unit_base: u64, suffix: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    if (unit_base == 1) {
        if (!writeU64(out, &pos, bytes)) return null;
        if (!writeSlice(out, &pos, suffix)) return null;
        return out[0..pos];
    }
    // Scale to hundredths (2 decimal places) with half-up rounding
    const whole = bytes / unit_base;
    const rem = bytes % unit_base;
    const hundredths = (rem * 100 + unit_base / 2) / unit_base;
    let adj_whole = whole;
    let adj_h = hundredths;
    if (adj_h >= 100) {
        adj_whole += 1;
        adj_h -= 100;
    }
    if (!writeU64(out, &pos, adj_whole)) return null;
    if (adj_h > 0) {
        if (!writeSlice(out, &pos, ".")) return null;
        const d1: u8 = @as(u8, adj_h / 10);
        const d2: u8 = @as(u8, adj_h % 10);
        if (pos >= out.len) return null;
        out[pos] = @as(u8, '0') + d1;
        pos += 1;
        if (d2 != 0) {
            if (pos >= out.len) return null;
            out[pos] = @as(u8, '0') + d2;
            pos += 1;
        }
    }
    if (!writeSlice(out, &pos, suffix)) return null;
    return out[0..pos];
}

/// Format byte count using binary IEC units (`B`, `KiB`, `MiB`, `GiB`, `TiB`, `PiB`).
export fn formatBytesIec(bytes: u64, out: []u8): ([]const u8)? {
    if (bytes >= 1125899906842624) return formatScaled(bytes, 1125899906842624, " PiB", out);
    if (bytes >= 1099511627776) return formatScaled(bytes, 1099511627776, " TiB", out);
    if (bytes >= 1073741824) return formatScaled(bytes, 1073741824, " GiB", out);
    if (bytes >= 1048576) return formatScaled(bytes, 1048576, " MiB", out);
    if (bytes >= 1024) return formatScaled(bytes, 1024, " KiB", out);
    return formatScaled(bytes, 1, " B", out);
}

/// Format byte count using decimal SI units (`B`, `KB`, `MB`, `GB`, `TB`, `PB`).
export fn formatBytesSi(bytes: u64, out: []u8): ([]const u8)? {
    if (bytes >= 1000000000000000) return formatScaled(bytes, 1000000000000000, " PB", out);
    if (bytes >= 1000000000000) return formatScaled(bytes, 1000000000000, " TB", out);
    if (bytes >= 1000000000) return formatScaled(bytes, 1000000000, " GB", out);
    if (bytes >= 1000000) return formatScaled(bytes, 1000000, " MB", out);
    if (bytes >= 1000) return formatScaled(bytes, 1000, " KB", out);
    return formatScaled(bytes, 1, " B", out);
}
