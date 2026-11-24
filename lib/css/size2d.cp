// Source: Absorbed from /data/dev/bun/src/css/values/size.cp | Synced: 2026-11-23T21:43:43Z
// ============================================================================
// lib/css/size2d.cp — Zero-Alloc CSS Typed 2D Pairs (`Size2D<Number>`,
// `Size2D<LengthPercentage>`) & Elliptical 4-Corner Radii
// ============================================================================
// Implements Bun `css/values/size.cp` (`Size2D(T)`) in pure zero-allocation CP:
// - `CssNumberPair`: 2-component `<number> <number>?` with single-value collapse
// - `CssSizeComponent` & `CssLengthPctPair`: typed `<length-percentage>` 2D pairs
//   for `border-radius`, `border-spacing`, `gap`, `background-size`, etc.
// - `CssCornerRadii`: full W3C CSS Backgrounds Level 3 `border-radius` shorthand
//   (`1..4 horizontal [ / 1..4 vertical ]?`) backed by four `CssLengthPctPair`
//   corners with optimal 1/2/3/4-value CSS serialization into caller `[]u8`.

pub err CssSize2DError {
    EmptyInput,
    InvalidSyntax,
    UnknownUnit,
    TooManyComponents,
    NegativeNotAllowed,
    NotFinite,
    Overflow,
    BufferTooSmall,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn trimSpaces(raw: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = raw.len;
    while (start < end && isSpace(raw[start])) start += 1;
    while (end > start && isSpace(raw[end - 1])) end -= 1;
    return raw[start..end];
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn isFiniteF64(x: f64): bool {
    return x == x && x <= 1.7976931348623157e308 && x >= -1.7976931348623157e308;
}

fn normalizeZero(v: f64): f64 {
    if (v == 0.0) return 0.0;
    return v;
}

fn appendByte(out: []u8, pos: usize, ch: u8): CssSize2DError!usize {
    if (pos >= out.len) return CssSize2DError.BufferTooSmall;
    out[pos] = ch;
    return pos + 1;
}

fn appendSlice(out: []u8, pos: usize, src: []const u8): CssSize2DError!usize {
    if (src.len > out.len - pos) return CssSize2DError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeUint(out: []u8, pos: usize, val_in: u64): CssSize2DError!usize {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val_in;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v /= 10;
            n += 1;
        }
    }
    if (n > out.len - pos) return CssSize2DError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) {
        out[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

fn formatNumber(out: []u8, pos_in: usize, val_in: f64): CssSize2DError!usize {
    if (!isFiniteF64(val_in)) return CssSize2DError.NotFinite;
    let pos = pos_in;
    if (val_in == 0.0) {
        return appendByte(out, pos, '0');
    }
    const neg = val_in < 0.0;
    const mag = if (neg) -val_in else val_in;
    if (mag >= 9.0e12) return CssSize2DError.Overflow;

    const scaled: u64 = @as(u64, @intFromFloat(mag * 1000000.0 + 0.5));
    if (scaled == 0) {
        return appendByte(out, pos, '0');
    }

    const int_part: u64 = scaled / 1000000;
    let frac_part: u64 = scaled % 1000000;

    if (neg) {
        pos = appendByte(out, pos, '-')?;
    }
    if (int_part > 0 || frac_part == 0) {
        pos = writeUint(out, pos, int_part)?;
    }
    if (frac_part > 0) {
        pos = appendByte(out, pos, '.')?;
        let digits: [6]u8 = [0; 6];
        let k: usize = 6;
        while (k > 0) {
            k -= 1;
            digits[k] = @as(u8, @intCast('0' + (frac_part % 10)));
            frac_part /= 10;
        }
        let frac_len: usize = 6;
        while (frac_len > 0 && digits[frac_len - 1] == '0') {
            frac_len -= 1;
        }
        if (frac_len > out.len - pos) return CssSize2DError.BufferTooSmall;
        let j: usize = 0;
        while (j < frac_len) {
            out[pos + j] = digits[j];
            j += 1;
        }
        pos += frac_len;
    }
    return pos;
}

struct ParsedNum {
    value: f64,
    next: usize,
}

fn parseNumberPrefix(s: []const u8): CssSize2DError!ParsedNum {
    if (s.len == 0) return CssSize2DError.EmptyInput;
    let i: usize = 0;
    let neg = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
    } else if (s[i] == '+') {
        i += 1;
    }
    if (i >= s.len) return CssSize2DError.InvalidSyntax;

    let int_part: f64 = 0.0;
    let has_int = false;
    while (i < s.len && isDigit(s[i])) {
        has_int = true;
        int_part = int_part * 10.0 + @as(f64, @floatFromInt(s[i] - '0'));
        i += 1;
    }

    let frac_part: f64 = 0.0;
    let has_frac = false;
    if (i < s.len && s[i] == '.') {
        i += 1;
        let scale: f64 = 0.1;
        while (i < s.len && isDigit(s[i])) {
            has_frac = true;
            frac_part += @as(f64, @floatFromInt(s[i] - '0')) * scale;
            scale *= 0.1;
            i += 1;
        }
        if (!has_frac) return CssSize2DError.InvalidSyntax;
    }

    if (!has_int && !has_frac) return CssSize2DError.InvalidSyntax;

    let val = int_part + frac_part;
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        // Only treat 'e'/'E' as exponent if followed by digit or [+-]digit
        // (avoids consuming "em" / "ex" unit suffix!)
        let looks_exp = false;
        if (i + 1 < s.len && isDigit(s[i + 1])) {
            looks_exp = true;
        } else if (i + 2 < s.len && (s[i + 1] == '+' || s[i + 1] == '-') && isDigit(s[i + 2])) {
            looks_exp = true;
        }
        if (looks_exp) {
            i += 1;
            let exp_neg = false;
            if (s[i] == '-') {
                exp_neg = true;
                i += 1;
            } else if (s[i] == '+') {
                i += 1;
            }
            let exp_val: u32 = 0;
            let exp_ov = false;
            while (i < s.len && isDigit(s[i])) {
                if (exp_val < 10000) {
                    exp_val = exp_val * 10 + @as(u32, s[i] - '0');
                } else {
                    exp_ov = true;
                }
                i += 1;
            }
            if (exp_ov || exp_val > 308) {
                if (exp_neg) {
                    val = 0.0;
                } else if (val != 0.0) {
                    return CssSize2DError.Overflow;
                }
            } else {
                let k: u32 = 0;
                while (k < exp_val) {
                    if (exp_neg) {
                        val *= 0.1;
                    } else {
                        val *= 10.0;
                    }
                    k += 1;
                }
            }
        }
    }

    if (!isFiniteF64(val)) return CssSize2DError.Overflow;
    if (neg && val != 0.0) val = -val;
    return ParsedNum{ value: normalizeZero(val), next: i };
}

fn splitTokens(raw: []const u8, out: [][]const u8): CssSize2DError!usize {
    const s = trimSpaces(raw);
    if (s.len == 0) return CssSize2DError.EmptyInput;
    let count: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        while (i < s.len && isSpace(s[i])) i += 1;
        if (i >= s.len) break;
        const start = i;
        while (i < s.len && !isSpace(s[i])) i += 1;
        if (count >= out.len) return CssSize2DError.TooManyComponents;
        out[count] = s[start..i];
        count += 1;
    }
    return count;
}

/// Typed 2-component CSS `<number> <number>?` pair (Bun `Size2D(f32)`).
pub struct CssNumberPair {
    a: f64 = 0.0,
    b: f64 = 0.0,

    static fn uniform(v: f64): CssSize2DError!CssNumberPair {
        if (!isFiniteF64(v)) return CssSize2DError.NotFinite;
        const nv = normalizeZero(v);
        return CssNumberPair{ a: nv, b: nv };
    }

    static fn fromParts(a: f64, b: f64): CssSize2DError!CssNumberPair {
        if (!isFiniteF64(a) || !isFiniteF64(b)) return CssSize2DError.NotFinite;
        return CssNumberPair{ a: normalizeZero(a), b: normalizeZero(b) };
    }

    static fn parse(raw: []const u8): CssSize2DError!CssNumberPair {
        let toks: [2][]const u8 = [""; 2];
        const n = splitTokens(raw, toks[0..])?;
        const p1 = parseNumberPrefix(toks[0])?;
        if (p1.next != toks[0].len) return CssSize2DError.InvalidSyntax;
        if (n == 1) {
            return CssNumberPair{ a: p1.value, b: p1.value };
        }
        const p2 = parseNumberPrefix(toks[1])?;
        if (p2.next != toks[1].len) return CssSize2DError.InvalidSyntax;
        return CssNumberPair{ a: p1.value, b: p2.value };
    }

    fn isUniform(): bool {
        return _.a == _.b;
    }

    fn isZero(): bool {
        return _.a == 0.0 && _.b == 0.0;
    }

    fn scale(factor: f64): CssSize2DError!CssNumberPair {
        if (!isFiniteF64(factor)) return CssSize2DError.NotFinite;
        return CssNumberPair.fromParts(_.a * factor, _.b * factor);
    }

    fn add(other: CssNumberPair): CssSize2DError!CssNumberPair {
        return CssNumberPair.fromParts(_.a + other.a, _.b + other.b);
    }

    fn eql(other: CssNumberPair): bool {
        return _.a == other.a && _.b == other.b;
    }

    /// Serializes `<a [ b ]?>`, omitting `b` when `a == b`.
    fn toCss(out: []u8): CssSize2DError!usize {
        let pos = formatNumber(out, 0, _.a)?;
        if (_.b != _.a) {
            pos = appendByte(out, pos, ' ')?;
            pos = formatNumber(out, pos, _.b)?;
        }
        return pos;
    }
}

/// CSS `<length-percentage>` unit tag for `CssSizeComponent`.
pub enum CssLengthPctUnit: u8 {
    UnitlessZero = 0,
    Px = 1,
    Em = 2,
    Rem = 3,
    Vw = 4,
    Vh = 5,
    Vmin = 6,
    Vmax = 7,
    Percent = 8,
    Cm = 9,
    Mm = 10,
    In = 11,
    Pt = 12,
    Pc = 13,
    Ch = 14,
    Ex = 15,
}

fn unitSuffix(u: CssLengthPctUnit): []const u8 {
    if (u == CssLengthPctUnit.UnitlessZero) return "";
    if (u == CssLengthPctUnit.Px) return "px";
    if (u == CssLengthPctUnit.Em) return "em";
    if (u == CssLengthPctUnit.Rem) return "rem";
    if (u == CssLengthPctUnit.Vw) return "vw";
    if (u == CssLengthPctUnit.Vh) return "vh";
    if (u == CssLengthPctUnit.Vmin) return "vmin";
    if (u == CssLengthPctUnit.Vmax) return "vmax";
    if (u == CssLengthPctUnit.Percent) return "%";
    if (u == CssLengthPctUnit.Cm) return "cm";
    if (u == CssLengthPctUnit.Mm) return "mm";
    if (u == CssLengthPctUnit.In) return "in";
    if (u == CssLengthPctUnit.Pt) return "pt";
    if (u == CssLengthPctUnit.Pc) return "pc";
    if (u == CssLengthPctUnit.Ch) return "ch";
    return "ex";
}

fn parseUnitSuffix(s: []const u8): CssSize2DError!CssLengthPctUnit {
    if (eqlIgnoreCase(s, "px")) return CssLengthPctUnit.Px;
    if (eqlIgnoreCase(s, "em")) return CssLengthPctUnit.Em;
    if (eqlIgnoreCase(s, "rem")) return CssLengthPctUnit.Rem;
    if (eqlIgnoreCase(s, "vw")) return CssLengthPctUnit.Vw;
    if (eqlIgnoreCase(s, "vh")) return CssLengthPctUnit.Vh;
    if (eqlIgnoreCase(s, "vmin")) return CssLengthPctUnit.Vmin;
    if (eqlIgnoreCase(s, "vmax")) return CssLengthPctUnit.Vmax;
    if (eqlIgnoreCase(s, "%")) return CssLengthPctUnit.Percent;
    if (eqlIgnoreCase(s, "cm")) return CssLengthPctUnit.Cm;
    if (eqlIgnoreCase(s, "mm")) return CssLengthPctUnit.Mm;
    if (eqlIgnoreCase(s, "in")) return CssLengthPctUnit.In;
    if (eqlIgnoreCase(s, "pt")) return CssLengthPctUnit.Pt;
    if (eqlIgnoreCase(s, "pc")) return CssLengthPctUnit.Pc;
    if (eqlIgnoreCase(s, "ch")) return CssLengthPctUnit.Ch;
    if (eqlIgnoreCase(s, "ex")) return CssLengthPctUnit.Ex;
    return CssSize2DError.UnknownUnit;
}

/// Single `<length-percentage>` component inside `CssLengthPctPair`.
pub struct CssSizeComponent {
    value: f64 = 0.0,
    unit: CssLengthPctUnit = CssLengthPctUnit.UnitlessZero,

    static fn zero(): CssSizeComponent {
        return CssSizeComponent{ value: 0.0, unit: CssLengthPctUnit.UnitlessZero };
    }

    static fn px(v: f64): CssSize2DError!CssSizeComponent {
        if (!isFiniteF64(v)) return CssSize2DError.NotFinite;
        return CssSizeComponent{ value: normalizeZero(v), unit: CssLengthPctUnit.Px };
    }

    static fn pct(v: f64): CssSize2DError!CssSizeComponent {
        if (!isFiniteF64(v)) return CssSize2DError.NotFinite;
        return CssSizeComponent{ value: normalizeZero(v), unit: CssLengthPctUnit.Percent };
    }

    static fn parse(raw: []const u8): CssSize2DError!CssSizeComponent {
        const s = trimSpaces(raw);
        if (s.len == 0) return CssSize2DError.EmptyInput;
        const p = parseNumberPrefix(s)?;
        const rest = s[p.next..];
        if (rest.len == 0) {
            if (p.value == 0.0) {
                return CssSizeComponent.zero();
            }
            return CssSize2DError.InvalidSyntax;
        }
        const unit = parseUnitSuffix(rest)?;
        return CssSizeComponent{ value: p.value, unit: unit };
    }

    fn isZero(): bool {
        return _.value == 0.0;
    }

    fn isPercentage(): bool {
        return _.unit == CssLengthPctUnit.Percent;
    }

    fn isNegative(): bool {
        return _.value < 0.0;
    }

    /// Resolves the component to CSS pixels given basis, font size, and viewport dimensions.
    /// Returns `null` for font-metric units (`ch`, `ex`) or non-finite inputs.
    fn resolvePx(basis_px: f64, font_px: f64, root_font_px: f64, vw_px: f64, vh_px: f64): f64? {
        if (!isFiniteF64(_.value)) return null;
        if (_.value == 0.0) return 0.0;
        if (_.unit == CssLengthPctUnit.UnitlessZero) return 0.0;
        if (_.unit == CssLengthPctUnit.Px) return _.value;
        if (_.unit == CssLengthPctUnit.Percent) return (_.value / 100.0) * basis_px;
        if (_.unit == CssLengthPctUnit.Em) return _.value * font_px;
        if (_.unit == CssLengthPctUnit.Rem) return _.value * root_font_px;
        if (_.unit == CssLengthPctUnit.Vw) return (_.value / 100.0) * vw_px;
        if (_.unit == CssLengthPctUnit.Vh) return (_.value / 100.0) * vh_px;
        if (_.unit == CssLengthPctUnit.Vmin) {
            const m = if (vw_px < vh_px) vw_px else vh_px;
            return (_.value / 100.0) * m;
        }
        if (_.unit == CssLengthPctUnit.Vmax) {
            const m = if (vw_px > vh_px) vw_px else vh_px;
            return (_.value / 100.0) * m;
        }
        if (_.unit == CssLengthPctUnit.In) return _.value * 96.0;
        if (_.unit == CssLengthPctUnit.Cm) return _.value * (96.0 / 2.54);
        if (_.unit == CssLengthPctUnit.Mm) return _.value * (96.0 / 25.4);
        if (_.unit == CssLengthPctUnit.Pt) return _.value * (96.0 / 72.0);
        if (_.unit == CssLengthPctUnit.Pc) return _.value * 16.0;
        return null;
    }

    fn scale(factor: f64): CssSize2DError!CssSizeComponent {
        if (!isFiniteF64(factor)) return CssSize2DError.NotFinite;
        const nv = _.value * factor;
        if (!isFiniteF64(nv)) return CssSize2DError.Overflow;
        return CssSizeComponent{ value: normalizeZero(nv), unit: _.unit };
    }

    fn eql(other: CssSizeComponent): bool {
        if (_.value == 0.0 && other.value == 0.0) {
            // In CSS, `0%` is distinct from `0px`/`0` for calc/serialization fidelity
            if (_.unit == CssLengthPctUnit.Percent || other.unit == CssLengthPctUnit.Percent) {
                return _.unit == other.unit;
            }
            return true;
        }
        return _.value == other.value && _.unit == other.unit;
    }

    fn toCss(out: []u8, pos_in: usize, minify: bool): CssSize2DError!usize {
        if (_.value == 0.0) {
            if (minify && _.unit != CssLengthPctUnit.Percent) {
                return appendByte(out, pos_in, '0');
            }
            if (_.unit == CssLengthPctUnit.UnitlessZero) {
                return appendByte(out, pos_in, '0');
            }
        }
        let pos = formatNumber(out, pos_in, _.value)?;
        return appendSlice(out, pos, unitSuffix(_.unit));
    }
}

/// Typed 2-component CSS `<length-percentage> <length-percentage>?` pair
/// (Bun `Size2D(LengthPercentage)`). Collapses to a single token when `a == b`.
pub struct CssLengthPctPair {
    a: CssSizeComponent = CssSizeComponent.zero(),
    b: CssSizeComponent = CssSizeComponent.zero(),

    static fn zero(): CssLengthPctPair {
        return CssLengthPctPair{
            a: CssSizeComponent.zero(),
            b: CssSizeComponent.zero(),
        };
    }

    static fn uniform(v: CssSizeComponent): CssLengthPctPair {
        return CssLengthPctPair{ a: v, b: v };
    }

    static fn fromParts(a: CssSizeComponent, b: CssSizeComponent): CssLengthPctPair {
        return CssLengthPctPair{ a: a, b: b };
    }

    static fn parse(raw: []const u8): CssSize2DError!CssLengthPctPair {
        let toks: [2][]const u8 = [""; 2];
        const n = splitTokens(raw, toks[0..])?;
        const first = CssSizeComponent.parse(toks[0])?;
        if (n == 1) {
            return CssLengthPctPair{ a: first, b: first };
        }
        const second = CssSizeComponent.parse(toks[1])?;
        return CssLengthPctPair{ a: first, b: second };
    }

    fn isUniform(): bool {
        return _.a.eql(_.b);
    }

    fn isZero(): bool {
        return _.a.isZero() && _.b.isZero();
    }

    fn scale(factor: f64): CssSize2DError!CssLengthPctPair {
        const na = _.a.scale(factor)?;
        const nb = _.b.scale(factor)?;
        return CssLengthPctPair{ a: na, b: nb };
    }

    fn eql(other: CssLengthPctPair): bool {
        return _.a.eql(other.a) && _.b.eql(other.b);
    }

    /// Serializes `a` alone when `a == b`, otherwise `a b`.
    fn toCss(out: []u8, minify: bool): CssSize2DError!usize {
        let pos = _.a.toCss(out, 0, minify)?;
        if (!_.b.eql(_.a)) {
            pos = appendByte(out, pos, ' ')?;
            pos = _.b.toCss(out, pos, minify)?;
        }
        return pos;
    }
}

fn expandFourComponents(raw: []const u8): CssSize2DError![4]CssSizeComponent {
    let toks: [4][]const u8 = [""; 4];
    const n = splitTokens(raw, toks[0..])?;
    let parsed: [4]CssSizeComponent = [CssSizeComponent.zero(); 4];
    let i: usize = 0;
    while (i < n) {
        const c = CssSizeComponent.parse(toks[i])?;
        if (c.isNegative()) return CssSize2DError.NegativeNotAllowed;
        parsed[i] = c;
        i += 1;
    }
    if (n == 1) {
        return [parsed[0], parsed[0], parsed[0], parsed[0]];
    }
    if (n == 2) {
        return [parsed[0], parsed[1], parsed[0], parsed[1]];
    }
    if (n == 3) {
        return [parsed[0], parsed[1], parsed[2], parsed[1]];
    }
    return [parsed[0], parsed[1], parsed[2], parsed[3]];
}

fn writeFourCollapsed(out: []u8, pos_in: usize, vals: [4]CssSizeComponent, minify: bool): CssSize2DError!usize {
    let count: usize = 4;
    if (vals[3].eql(vals[1])) {
        count = 3;
        if (vals[2].eql(vals[0])) {
            count = 2;
            if (vals[1].eql(vals[0])) {
                count = 1;
            }
        }
    }
    let pos = pos_in;
    let i: usize = 0;
    while (i < count) {
        if (i > 0) {
            pos = appendByte(out, pos, ' ')?;
        }
        pos = vals[i].toCss(out, pos, minify)?;
        i += 1;
    }
    return pos;
}

/// W3C CSS Backgrounds Level 3 4-corner elliptical `border-radius` backed by
/// four `CssLengthPctPair` (`horizontal`, `vertical`) radii.
pub struct CssCornerRadii {
    top_left: CssLengthPctPair = CssLengthPctPair.zero(),
    top_right: CssLengthPctPair = CssLengthPctPair.zero(),
    bottom_right: CssLengthPctPair = CssLengthPctPair.zero(),
    bottom_left: CssLengthPctPair = CssLengthPctPair.zero(),

    static fn uniform(c: CssSizeComponent): CssSize2DError!CssCornerRadii {
        if (c.isNegative()) return CssSize2DError.NegativeNotAllowed;
        const p = CssLengthPctPair.uniform(c);
        return CssCornerRadii{
            top_left: p,
            top_right: p,
            bottom_right: p,
            bottom_left: p,
        };
    }

    /// Parses CSS `border-radius` shorthand: `<1..4 horiz> [ / <1..4 vert> ]?`.
    static fn parse(raw: []const u8): CssSize2DError!CssCornerRadii {
        const s = trimSpaces(raw);
        if (s.len == 0) return CssSize2DError.EmptyInput;

        let slash_idx: usize = s.len;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == '/') {
                if (slash_idx != s.len) return CssSize2DError.InvalidSyntax;
                slash_idx = i;
            }
            i += 1;
        }

        if (slash_idx == s.len) {
            const h = expandFourComponents(s)?;
            return CssCornerRadii{
                top_left: CssLengthPctPair.uniform(h[0]),
                top_right: CssLengthPctPair.uniform(h[1]),
                bottom_right: CssLengthPctPair.uniform(h[2]),
                bottom_left: CssLengthPctPair.uniform(h[3]),
            };
        }

        const h = expandFourComponents(s[0..slash_idx])?;
        const v = expandFourComponents(s[(slash_idx + 1)..])?;
        return CssCornerRadii{
            top_left: CssLengthPctPair.fromParts(h[0], v[0]),
            top_right: CssLengthPctPair.fromParts(h[1], v[1]),
            bottom_right: CssLengthPctPair.fromParts(h[2], v[2]),
            bottom_left: CssLengthPctPair.fromParts(h[3], v[3]),
        };
    }

    /// True if all four corners have equal horizontal and vertical radii.
    fn isCircular(): bool {
        return _.top_left.isUniform()
            && _.top_right.isUniform()
            && _.bottom_right.isUniform()
            && _.bottom_left.isUniform();
    }

    fn isZero(): bool {
        return _.top_left.isZero()
            && _.top_right.isZero()
            && _.bottom_right.isZero()
            && _.bottom_left.isZero();
    }

    fn eql(other: CssCornerRadii): bool {
        return _.top_left.eql(other.top_left)
            && _.top_right.eql(other.top_right)
            && _.bottom_right.eql(other.bottom_right)
            && _.bottom_left.eql(other.bottom_left);
    }

    /// Serializes optimal CSS `border-radius` shorthand, omitting `/ vertical` when circular.
    fn toCss(out: []u8, minify: bool): CssSize2DError!usize {
        const h: [4]CssSizeComponent = [
            _.top_left.a,
            _.top_right.a,
            _.bottom_right.a,
            _.bottom_left.a,
        ];
        let pos = writeFourCollapsed(out, 0, h, minify)?;
        if (!_.isCircular()) {
            if (minify) {
                pos = appendByte(out, pos, '/')?;
            } else {
                pos = appendSlice(out, pos, " / ")?;
            }
            const v: [4]CssSizeComponent = [
                _.top_left.b,
                _.top_right.b,
                _.bottom_right.b,
                _.bottom_left.b,
            ];
            pos = writeFourCollapsed(out, pos, v, minify)?;
        }
        return pos;
    }
}
