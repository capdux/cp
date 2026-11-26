// packages/@cap/css-color/src/index.cp
// Zero-allocation CSS Color Level 4 functional color parser, formatter & RGB <-> HSL engine.
// MIT clean-room pure CP implementation.

/// Parsed RGBA color (`r, g, b` in `0..255`, `a` in `0.0..1.0`).
pub struct CssRgba {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: f64,
    pub valid: bool,
}

/// HSLA color (`h` in `0..360`, `s` in `0..100`, `l` in `0..100`, `a` in `0.0..1.0`).
pub struct CssHsla {
    pub h: f64,
    pub s: f64,
    pub l: f64,
    pub a: f64,
    pub valid: bool,
}

fn ccToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn ccIsSpace(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8) || c == ('\r' as u8) || c == ('\n' as u8);
}

fn ccTrim(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && ccIsSpace(s[start])) {
        start += 1;
    }
    while (end > start && ccIsSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn ccEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (ccToLower(a[i]) != ccToLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn ccAbsF64(x: f64): f64 {
    if (x < (0.0 as f64)) {
        return -x;
    }
    return x;
}

fn ccClampF64(x: f64, lo: f64, hi: f64): f64 {
    if (x < lo) { return lo; }
    if (x > hi) { return hi; }
    return x;
}

fn ccClampU8(v: f64): u8 {
    if (v <= (0.0 as f64)) { return 0 as u8; }
    if (v >= (255.0 as f64)) { return 255 as u8; }
    return ((v + (0.5 as f64)) as u32) as u8;
}

fn ccNormHue(h: f64): f64 {
    let cur: f64 = h;
    if (cur < (0.0 as f64)) {
        const wraps: u64 = (((-cur) / (360.0 as f64)) as u64) + (1 as u64);
        cur = cur + ((wraps as f64) * (360.0 as f64));
    }
    if (cur >= (360.0 as f64)) {
        const wraps: u64 = (cur / (360.0 as f64)) as u64;
        cur = cur - ((wraps as f64) * (360.0 as f64));
    }
    if (cur >= (360.0 as f64) || cur < (0.0 as f64)) {
        return 0.0 as f64;
    }
    return cur;
}

/// Convert HSLA (`h` degrees, `s` 0..100, `l` 0..100, `a` 0..1) into `CssRgba`.
pub fn hslToRgb(h: f64, s: f64, l: f64, a: f64): CssRgba {
    const hn: f64 = ccNormHue(h);
    const sn: f64 = ccClampF64(s, 0.0 as f64, 100.0 as f64) / (100.0 as f64);
    const ln: f64 = ccClampF64(l, 0.0 as f64, 100.0 as f64) / (100.0 as f64);
    const an: f64 = ccClampF64(a, 0.0 as f64, 1.0 as f64);

    const c: f64 = ((1.0 as f64) - ccAbsF64(((2.0 as f64) * ln) - (1.0 as f64))) * sn;
    const hp: f64 = hn / (60.0 as f64);
    const twoPairs: u64 = (hp / (2.0 as f64)) as u64;
    const hMod2: f64 = hp - ((twoPairs as f64) * (2.0 as f64));
    const x: f64 = c * ((1.0 as f64) - ccAbsF64(hMod2 - (1.0 as f64)));
    const m: f64 = ln - (c * (0.5 as f64));

    let r1: f64 = 0.0;
    let g1: f64 = 0.0;
    let b1: f64 = 0.0;

    if (hp < (1.0 as f64)) {
        r1 = c; g1 = x; b1 = 0.0 as f64;
    } else if (hp < (2.0 as f64)) {
        r1 = x; g1 = c; b1 = 0.0 as f64;
    } else if (hp < (3.0 as f64)) {
        r1 = 0.0 as f64; g1 = c; b1 = x;
    } else if (hp < (4.0 as f64)) {
        r1 = 0.0 as f64; g1 = x; b1 = c;
    } else if (hp < (5.0 as f64)) {
        r1 = x; g1 = 0.0 as f64; b1 = c;
    } else {
        r1 = c; g1 = 0.0 as f64; b1 = x;
    }

    return CssRgba{
        r: ccClampU8((r1 + m) * (255.0 as f64)),
        g: ccClampU8((g1 + m) * (255.0 as f64)),
        b: ccClampU8((b1 + m) * (255.0 as f64)),
        a: an,
        valid: true,
    };
}

/// Convert RGBA (`r, g, b` 0..255, `a` 0..1) into `CssHsla` (`h` 0..360, `s` 0..100, `l` 0..100).
pub fn rgbToHsl(r: u8, g: u8, b: u8, a: f64): CssHsla {
    const rn: f64 = (r as f64) / (255.0 as f64);
    const gn: f64 = (g as f64) / (255.0 as f64);
    const bn: f64 = (b as f64) / (255.0 as f64);

    let maxC: f64 = rn;
    if (gn > maxC) { maxC = gn; }
    if (bn > maxC) { maxC = bn; }

    let minC: f64 = rn;
    if (gn < minC) { minC = gn; }
    if (bn < minC) { minC = bn; }

    const d: f64 = maxC - minC;
    const l: f64 = (maxC + minC) * (0.5 as f64);

    let h: f64 = 0.0;
    let s: f64 = 0.0;

    if (d > (0.0000001 as f64)) {
        const denom: f64 = (1.0 as f64) - ccAbsF64(((2.0 as f64) * l) - (1.0 as f64));
        if (denom > (0.0000001 as f64)) {
            s = d / denom;
        }
        if (maxC == rn) {
            h = (60.0 as f64) * ((gn - bn) / d);
            if (h < (0.0 as f64)) {
                h = h + (360.0 as f64);
            }
        } else if (maxC == gn) {
            h = (60.0 as f64) * (((bn - rn) / d) + (2.0 as f64));
        } else {
            h = (60.0 as f64) * (((rn - gn) / d) + (4.0 as f64));
        }
    }

    return CssHsla{
        h: ccNormHue(h),
        s: ccClampF64(s * (100.0 as f64), 0.0 as f64, 100.0 as f64),
        l: ccClampF64(l * (100.0 as f64), 0.0 as f64, 100.0 as f64),
        a: ccClampF64(a, 0.0 as f64, 1.0 as f64),
        valid: true,
    };
}

/// Parse a numeric token with optional `%` or `deg` suffix.
fn ccParseNum(raw: []const u8, out_val: *f64, is_pct: *bool, allow_deg: bool): bool {
    let s = ccTrim(raw);
    if (s.len == 0) {
        return false;
    }

    is_pct.* = false;
    if (s[s.len - 1] == ('%' as u8)) {
        is_pct.* = true;
        s = s[0..(s.len - 1)];
    } else if (allow_deg && s.len > 3 && ccEqIgnoreCase(s[(s.len - 3)..s.len], "deg")) {
        s = s[0..(s.len - 3)];
    }
    if (s.len == 0) {
        return false;
    }

    let i: usize = 0;
    let neg = false;
    if (s[0] == ('-' as u8)) {
        neg = true;
        i = 1;
    } else if (s[0] == ('+' as u8)) {
        i = 1;
    }
    if (i >= s.len) {
        return false;
    }

    let intPart: f64 = 0.0;
    let fracPart: f64 = 0.0;
    let digits: usize = 0;

    while (i < s.len && s[i] >= ('0' as u8) && s[i] <= ('9' as u8)) {
        intPart = intPart * (10.0 as f64) + ((s[i] - ('0' as u8)) as f64);
        digits += 1;
        i += 1;
    }
    if (i < s.len) {
        if (s[i] != ('.' as u8)) {
            return false;
        }
        i += 1;
        let scale: f64 = 0.1;
        let fdigits: usize = 0;
        while (i < s.len && s[i] >= ('0' as u8) && s[i] <= ('9' as u8)) {
            fracPart = fracPart + ((s[i] - ('0' as u8)) as f64) * scale;
            scale = scale * (0.1 as f64);
            digits += 1;
            fdigits += 1;
            i += 1;
        }
        if (fdigits == 0 || i != s.len) {
            return false;
        }
    }
    if (digits == 0) {
        return false;
    }

    let val: f64 = intPart + fracPart;
    if (neg) {
        val = -val;
    }
    out_val.* = val;
    return true;
}

/// Split functional color arguments supporting both legacy comma syntax (`a, b, c[, alpha]`)
/// and CSS Color 4 space/slash syntax (`a b c [/ alpha]`).
/// Writes 3 or 4 token slices into `out_toks[0..4]` and returns count (or 0 on syntax error).
fn ccSplitArgs(body: []const u8, out_toks: [][]const u8): usize {
    const t = ccTrim(body);
    if (t.len == 0) {
        return 0;
    }

    let hasComma = false;
    let hasSlash = false;
    let slashIdx: usize = 0;
    let scan: usize = 0;
    while (scan < t.len) {
        if (t[scan] == (',' as u8)) {
            hasComma = true;
        } else if (t[scan] == ('/' as u8)) {
            if (hasSlash) {
                return 0;
            }
            hasSlash = true;
            slashIdx = scan;
        }
        scan += 1;
    }

    if (hasComma && hasSlash) {
        return 0;
    }

    if (hasComma) {
        let count: usize = 0;
        let pos: usize = 0;
        while (pos <= t.len) {
            let end = pos;
            while (end < t.len && t[end] != (',' as u8)) {
                end += 1;
            }
            const seg = ccTrim(t[pos..end]);
            if (seg.len == 0 || count >= 4) {
                return 0;
            }
            out_toks[count] = seg;
            count += 1;
            if (end < t.len) {
                pos = end + 1;
            } else {
                break;
            }
        }
        if (count != 3 && count != 4) {
            return 0;
        }
        return count;
    }

    // CSS Color 4 space-separated syntax: `c1 c2 c3` or `c1 c2 c3 / alpha`
    let mainPart = t;
    let alphaPart: []const u8 = "";
    if (hasSlash) {
        mainPart = ccTrim(t[0..slashIdx]);
        alphaPart = ccTrim(t[(slashIdx + 1)..t.len]);
        if (alphaPart.len == 0) {
            return 0;
        }
    }

    let count: usize = 0;
    let pos: usize = 0;
    while (pos < mainPart.len) {
        while (pos < mainPart.len && ccIsSpace(mainPart[pos])) {
            pos += 1;
        }
        if (pos >= mainPart.len) {
            break;
        }
        const start = pos;
        while (pos < mainPart.len && !ccIsSpace(mainPart[pos])) {
            pos += 1;
        }
        if (count >= 3) {
            return 0;
        }
        out_toks[count] = mainPart[start..pos];
        count += 1;
    }

    if (count != 3) {
        return 0;
    }
    if (hasSlash) {
        out_toks[3] = alphaPart;
        return 4;
    }
    return 3;
}

/// Parse a CSS Color Level 4 functional color (`rgb(...)`, `rgba(...)`, `hsl(...)`, `hsla(...)`).
/// Supports both comma syntax and modern space + `/` alpha syntax as well as percentage alpha.
pub fn parseCssColor(input: string): CssRgba {
    let invalid = CssRgba{
        r: 0 as u8,
        g: 0 as u8,
        b: 0 as u8,
        a: 0.0 as f64,
        valid: false,
    };

    const s = ccTrim(input);
    if (s.len < 6 || s[s.len - 1] != (')' as u8)) {
        return invalid;
    }

    let lparen: usize = 0;
    while (lparen < s.len && s[lparen] != ('(' as u8)) {
        lparen += 1;
    }
    if (lparen == 0 || lparen + 1 >= s.len - 1) {
        return invalid;
    }

    const fnName = s[0..lparen];
    const isRgb = ccEqIgnoreCase(fnName, "rgb") || ccEqIgnoreCase(fnName, "rgba");
    const isHsl = ccEqIgnoreCase(fnName, "hsl") || ccEqIgnoreCase(fnName, "hsla");
    if (!isRgb && !isHsl) {
        return invalid;
    }

    let toks: [4][]const u8 = undefined;
    const argc = ccSplitArgs(s[(lparen + 1)..(s.len - 1)], toks[0..4]);
    if (argc != 3 && argc != 4) {
        return invalid;
    }

    let alpha: f64 = 1.0;
    if (argc == 4) {
        let aVal: f64 = 0.0;
        let aPct = false;
        if (!ccParseNum(toks[3], &aVal, &aPct, false)) {
            return invalid;
        }
        if (aPct) {
            aVal = aVal / (100.0 as f64);
        }
        alpha = ccClampF64(aVal, 0.0 as f64, 1.0 as f64);
    }

    if (isRgb) {
        let v0: f64 = 0.0; let p0 = false;
        let v1: f64 = 0.0; let p1 = false;
        let v2: f64 = 0.0; let p2 = false;
        if (!ccParseNum(toks[0], &v0, &p0, false) || !ccParseNum(toks[1], &v1, &p1, false) || !ccParseNum(toks[2], &v2, &p2, false)) {
            return invalid;
        }
        if (p0) { v0 = v0 * (2.55 as f64); }
        if (p1) { v1 = v1 * (2.55 as f64); }
        if (p2) { v2 = v2 * (2.55 as f64); }
        return CssRgba{
            r: ccClampU8(v0),
            g: ccClampU8(v1),
            b: ccClampU8(v2),
            a: alpha,
            valid: true,
        };
    }

    // HSL / HSLA
    let hVal: f64 = 0.0; let hPct = false;
    let sVal: f64 = 0.0; let sPct = false;
    let lVal: f64 = 0.0; let lPct = false;
    if (!ccParseNum(toks[0], &hVal, &hPct, true) || !ccParseNum(toks[1], &sVal, &sPct, false) || !ccParseNum(toks[2], &lVal, &lPct, false)) {
        return invalid;
    }
    if (hPct) {
        return invalid;
    }
    return hslToRgb(hVal, sVal, lVal, alpha);
}

fn ccWriteRaw(out: []u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.*] = src[i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

fn ccWriteU8(out: []u8, pos: *usize, val: u8): bool {
    if (val == (0 as u8)) {
        return ccWriteRaw(out, pos, "0");
    }
    let rev: [4]u8 = undefined;
    let n: usize = 0;
    let cur: u32 = val as u32;
    while (cur > (0 as u32)) {
        rev[n] = ('0' as u8) + ((cur % (10 as u32)) as u8);
        n += 1;
        cur = cur / (10 as u32);
    }
    if (pos.* + n > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos.*] = rev[n - 1 - i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

fn ccWriteAlpha(out: []u8, pos: *usize, a: f64): bool {
    const clamped = ccClampF64(a, 0.0 as f64, 1.0 as f64);
    const milli: u32 = ((clamped * (1000.0 as f64)) + (0.5 as f64)) as u32;
    if (milli == (0 as u32)) {
        return ccWriteRaw(out, pos, "0");
    }
    if (milli >= (1000 as u32)) {
        return ccWriteRaw(out, pos, "1");
    }
    if (!ccWriteRaw(out, pos, "0.")) {
        return false;
    }
    let fbuf: [3]u8 = undefined;
    fbuf[0] = ('0' as u8) + (((milli / (100 as u32)) % (10 as u32)) as u8);
    fbuf[1] = ('0' as u8) + (((milli / (10 as u32)) % (10 as u32)) as u8);
    fbuf[2] = ('0' as u8) + ((milli % (10 as u32)) as u8);
    let end: usize = 3;
    while (end > 1 && fbuf[end - 1] == ('0' as u8)) {
        end -= 1;
    }
    return ccWriteRaw(out, pos, fbuf[0..end]);
}

/// Format `c` as `rgb(r, g, b)` when `c.a >= 0.999`, or `rgba(r, g, b, a)` otherwise.
/// Returns bytes written into `out`, or `0` on invalid color or insufficient buffer.
pub fn formatRgb(c: CssRgba, out: []u8): usize {
    if (!c.valid) {
        return 0;
    }
    let pos: usize = 0;
    const isOpaque = c.a >= (0.999 as f64);
    if (isOpaque) {
        if (!ccWriteRaw(out, &pos, "rgb(")) { return 0; }
    } else {
        if (!ccWriteRaw(out, &pos, "rgba(")) { return 0; }
    }
    if (!ccWriteU8(out, &pos, c.r) || !ccWriteRaw(out, &pos, ", ")) { return 0; }
    if (!ccWriteU8(out, &pos, c.g) || !ccWriteRaw(out, &pos, ", ")) { return 0; }
    if (!ccWriteU8(out, &pos, c.b)) { return 0; }
    if (!isOpaque) {
        if (!ccWriteRaw(out, &pos, ", ") || !ccWriteAlpha(out, &pos, c.a)) { return 0; }
    }
    if (!ccWriteRaw(out, &pos, ")")) { return 0; }
    return pos;
}

export {
    CssRgba,
    CssHsla,
    hslToRgb,
    rgbToHsl,
    parseCssColor,
    formatRgb,
};
