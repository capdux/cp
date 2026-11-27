// @cap/pretty-bytes — Pure CP zero-allocation human-readable byte/bit formatter
// Parity with npm `pretty-bytes` (SI 1000 / IEC 1024, bytes / bits, signed, compact, custom precision).

pub struct PrettyBytesOpts {
    pub binary: bool,
    pub bits: bool,
    pub signed_val: bool,
    pub space: bool,
    pub precision: u8,

    pub static fn defaults(): PrettyBytesOpts {
        return PrettyBytesOpts {
            binary: false,
            bits: false,
            signed_val: false,
            space: true,
            precision: 2,
        };
    }
}

fn pbUnitStr(idx: usize, binary: bool, bits: bool): string {
    if (bits) {
        if (binary) {
            if (idx == 0) { return "b"; }
            if (idx == 1) { return "kibit"; }
            if (idx == 2) { return "Mibit"; }
            if (idx == 3) { return "Gibit"; }
            if (idx == 4) { return "Tibit"; }
            if (idx == 5) { return "Pibit"; }
            return "Eibit";
        }
        if (idx == 0) { return "b"; }
        if (idx == 1) { return "kbit"; }
        if (idx == 2) { return "Mbit"; }
        if (idx == 3) { return "Gbit"; }
        if (idx == 4) { return "Tbit"; }
        if (idx == 5) { return "Pbit"; }
        return "Ebit";
    }
    if (binary) {
        if (idx == 0) { return "B"; }
        if (idx == 1) { return "KiB"; }
        if (idx == 2) { return "MiB"; }
        if (idx == 3) { return "GiB"; }
        if (idx == 4) { return "TiB"; }
        if (idx == 5) { return "PiB"; }
        return "EiB";
    }
    if (idx == 0) { return "B"; }
    if (idx == 1) { return "kB"; }
    if (idx == 2) { return "MB"; }
    if (idx == 3) { return "GB"; }
    if (idx == 4) { return "TB"; }
    if (idx == 5) { return "PB"; }
    return "EB";
}

fn pbAppendStr(out: []u8, pos: *usize, s: string): bool {
    if (pos.* + s.len > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.*] = s[i];
        pos.* = pos.* + 1;
        i = i + 1;
    }
    return true;
}

fn pbAppendByte(out: []u8, pos: *usize, ch: u8): bool {
    if (pos.* >= out.len) {
        return false;
    }
    out[pos.*] = ch;
    pos.* = pos.* + 1;
    return true;
}

fn pbAppendU64(out: []u8, pos: *usize, v: u64): bool {
    let tmp: [24]u8 = undefined;
    let n: usize = 0;
    if (v == 0) {
        tmp[0] = '0' as u8;
        n = 1;
    } else {
        let cur = v;
        while (cur > 0 && n < 24) {
            tmp[n] = ('0' as u8) + ((cur % 10) as u8);
            cur = cur / 10;
            n = n + 1;
        }
    }
    if (pos.* + n > out.len) {
        return false;
    }
    let k = n;
    while (k > 0) {
        k = k - 1;
        out[pos.*] = tmp[k];
        pos.* = pos.* + 1;
    }
    return true;
}

pub fn prettyBytesWith(num: f64, opts: PrettyBytesOpts, out: []u8): usize {
    if (num != num || out.len == 0) {
        return 0;
    }

    let pos: usize = 0;
    if (num == 0.0) {
        if (opts.signed_val) {
            if (!pbAppendByte(out, &pos, ' ' as u8)) {
                return 0;
            }
        }
        if (!pbAppendByte(out, &pos, '0' as u8)) {
            return 0;
        }
        if (opts.space) {
            if (!pbAppendByte(out, &pos, ' ' as u8)) {
                return 0;
            }
        }
        if (!pbAppendStr(out, &pos, pbUnitStr(0, opts.binary, opts.bits))) {
            return 0;
        }
        return pos;
    }

    let is_neg = num < 0.0;
    let val = num;
    if (is_neg) {
        val = -num;
    }

    if (is_neg) {
        if (!pbAppendByte(out, &pos, '-' as u8)) {
            return 0;
        }
    } else if (opts.signed_val) {
        if (!pbAppendByte(out, &pos, '+' as u8)) {
            return 0;
        }
    }

    let base: f64 = 1000.0;
    let base_u: u64 = 1000;
    if (opts.binary) {
        base = 1024.0;
        base_u = 1024;
    }

    let unit_idx: usize = 0;
    while (val >= base && unit_idx < 6) {
        val = val / base;
        unit_idx = unit_idx + 1;
    }

    let prec: u8 = opts.precision;
    if (prec > 6) {
        prec = 6;
    }
    let scale: u64 = 1;
    let pi: u8 = 0;
    while (pi < prec) {
        scale = scale * 10;
        pi = pi + 1;
    }

    let scaled: u64 = (val * (scale as f64) + 0.5) as u64;
    if (unit_idx < 6 && scaled >= base_u * scale) {
        val = val / base;
        unit_idx = unit_idx + 1;
        scaled = (val * (scale as f64) + 0.5) as u64;
    }

    let int_part: u64 = scaled / scale;
    let frac_part: u64 = scaled % scale;

    if (!pbAppendU64(out, &pos, int_part)) {
        return 0;
    }

    if (prec > 0 && frac_part > 0) {
        let fbuf: [8]u8 = undefined;
        let rem = frac_part;
        let fi: usize = prec as usize;
        while (fi > 0) {
            fi = fi - 1;
            fbuf[fi] = ('0' as u8) + ((rem % 10) as u8);
            rem = rem / 10;
        }
        let flen: usize = prec as usize;
        while (flen > 0 && fbuf[flen - 1] == ('0' as u8)) {
            flen = flen - 1;
        }
        if (flen > 0) {
            if (!pbAppendByte(out, &pos, '.' as u8)) {
                return 0;
            }
            let j: usize = 0;
            while (j < flen) {
                if (!pbAppendByte(out, &pos, fbuf[j])) {
                    return 0;
                }
                j = j + 1;
            }
        }
    }

    if (opts.space) {
        if (!pbAppendByte(out, &pos, ' ' as u8)) {
            return 0;
        }
    }
    if (!pbAppendStr(out, &pos, pbUnitStr(unit_idx, opts.binary, opts.bits))) {
        return 0;
    }
    return pos;
}

pub fn prettyBytes(num: f64, out: []u8): usize {
    return prettyBytesWith(num, PrettyBytesOpts.defaults(), out);
}

export {
    PrettyBytesOpts,
    prettyBytes,
    prettyBytesWith
};
