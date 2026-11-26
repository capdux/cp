// @cap/humansize — Zero-heap human-readable byte size formatter (crates.io:humansize v2)

pub enum BaseUnit {
    Decimal,
    Binary,
    Windows,
}

pub struct FormatSizeOptions {
    base: BaseUnit,
    decimal_places: u8,
    space_after_value: bool,
    long_units: bool,
}

pub const DECIMAL = FormatSizeOptions{
    base: BaseUnit.Decimal,
    decimal_places: 2,
    space_after_value: true,
    long_units: false,
};

pub const BINARY = FormatSizeOptions{
    base: BaseUnit.Binary,
    decimal_places: 2,
    space_after_value: true,
    long_units: false,
};

pub const WINDOWS = FormatSizeOptions{
    base: BaseUnit.Windows,
    decimal_places: 2,
    space_after_value: true,
    long_units: false,
};

fn hszWriteSlice(out: []u8, pos: usize, s: string): usize {
    let w = pos;
    let i: usize = 0;
    while (i < s.len && w < out.len) {
        out[w] = s[i];
        w += 1;
        i += 1;
    }
    return w;
}

fn hszWriteU64(out: []u8, pos: usize, val: u64): usize {
    let digits: [24]u8 = undefined;
    let len: usize = 0;
    let v = val;
    if (v == 0) {
        digits[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            digits[len] = ('0' as u8) + ((v % 10) as u8);
            v = v / 10;
            len += 1;
        }
    }
    let w = pos;
    let k = len;
    while (k > 0 && w < out.len) {
        k -= 1;
        out[w] = digits[k];
        w += 1;
    }
    return w;
}

fn hszWriteFixedFrac(out: []u8, pos: usize, frac: u64, places: u8): usize {
    let digits: [8]u8 = undefined;
    let v = frac;
    let p: usize = places as usize;
    let i: usize = p;
    while (i > 0) {
        i -= 1;
        digits[i] = ('0' as u8) + ((v % 10) as u8);
        v = v / 10;
    }
    let w = pos;
    let j: usize = 0;
    while (j < p && w < out.len) {
        out[w] = digits[j];
        w += 1;
        j += 1;
    }
    return w;
}

fn hszPow10(places: u8): u64 {
    let res: u64 = 1;
    let i: u8 = 0;
    while (i < places) {
        res = res * 10;
        i += 1;
    }
    return res;
}

fn hszUnitStr(base: BaseUnit, idx: usize, long_units: bool, is_one: bool): string {
    if (idx == 0) {
        if (!long_units) {
            return "B";
        }
        if (is_one) {
            return "Byte";
        }
        return "Bytes";
    }
    if (base == BaseUnit.Binary) {
        if (long_units) {
            if (idx == 1) return "Kibibytes";
            if (idx == 2) return "Mebibytes";
            if (idx == 3) return "Gibibytes";
            if (idx == 4) return "Tebibytes";
            if (idx == 5) return "Pebibytes";
            return "Exbibytes";
        }
        if (idx == 1) return "KiB";
        if (idx == 2) return "MiB";
        if (idx == 3) return "GiB";
        if (idx == 4) return "TiB";
        if (idx == 5) return "PiB";
        return "EiB";
    }
    if (long_units) {
        if (idx == 1) return "Kilobytes";
        if (idx == 2) return "Megabytes";
        if (idx == 3) return "Gigabytes";
        if (idx == 4) return "Terabytes";
        if (idx == 5) return "Petabytes";
        return "Exabytes";
    }
    if (base == BaseUnit.Windows) {
        if (idx == 1) return "KB";
        if (idx == 2) return "MB";
        if (idx == 3) return "GB";
        if (idx == 4) return "TB";
        if (idx == 5) return "PB";
        return "EB";
    }
    if (idx == 1) return "kB";
    if (idx == 2) return "MB";
    if (idx == 3) return "GB";
    if (idx == 4) return "TB";
    if (idx == 5) return "PB";
    return "EB";
}

/// Format `bytes` into human-readable form inside `out` according to `opts`.
/// Returns the number of bytes written.
pub fn formatSize(bytes: u64, opts: FormatSizeOptions, out: []u8): usize {
    const divisor: u64 = if (opts.base == BaseUnit.Decimal) 1000 else 1024;
    let w: usize = 0;
    let unit_idx: usize = 0;

    if (bytes < divisor) {
        w = hszWriteU64(out, w, bytes);
    } else {
        const div_f: f64 = @as(f64, @floatFromInt(divisor));
        let val: f64 = @as(f64, @floatFromInt(bytes));
        while (val >= div_f && unit_idx < 6) {
            val = val / div_f;
            unit_idx += 1;
        }
        const dp: u8 = if (opts.decimal_places > 6) 6 else opts.decimal_places;
        const scale: u64 = hszPow10(dp);
        const scale_f: f64 = @as(f64, @floatFromInt(scale));
        let scaled = @as(u64, @intFromFloat(val * scale_f + 0.5));
        if (scaled >= divisor * scale && unit_idx < 6) {
            scaled = scale;
            unit_idx += 1;
        }
        const int_part: u64 = scaled / scale;
        const frac_part: u64 = scaled % scale;
        w = hszWriteU64(out, w, int_part);
        if (dp > 0) {
            if (w < out.len) {
                out[w] = '.';
                w += 1;
            }
            w = hszWriteFixedFrac(out, w, frac_part, dp);
        }
    }

    if (opts.space_after_value && w < out.len) {
        out[w] = ' ';
        w += 1;
    }

    const unit = hszUnitStr(opts.base, unit_idx, opts.long_units, bytes == 1);
    w = hszWriteSlice(out, w, unit);
    return w;
}

/// Format `bytes` using default SI decimal options (`1000`, 2 decimal places, `"kB"`/`"MB"`).
pub fn formatDecimal(bytes: u64, out: []u8): usize {
    return formatSize(bytes, DECIMAL, out);
}

/// Format `bytes` using default IEC binary options (`1024`, 2 decimal places, `"KiB"`/`"MiB"`).
pub fn formatBinary(bytes: u64, out: []u8): usize {
    return formatSize(bytes, BINARY, out);
}
