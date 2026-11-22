// Source: Absorbed from /data/dev/bun/src/css/values/percentage.cp | Synced: 2026-11-22T18:45:00Z
// ============================================================================
// lib/css/percentage.cp — Zero-Alloc CSS Values Level 4 `<percentage>`,
// `<number-or-percentage>` & `<length-percentage>`
// ============================================================================
// Implements W3C CSS Values & Units Level 4 percentage parsing, normalized
// unit-value arithmetic, Bun-aligned minimal CSS serialization (normalizing
// `-0` to `0%`, omitting leading `0` for `0 < |pct| < 1` when minifying such
// as `.5%` / `-.25%`), `CssNumberOrPercentage`, and zero-allocation
// `CssLengthOrPercentage` with static `calc(length +/- percentage)` folding.

pub err CssPercentageError {
    EmptyInput,
    InvalidSyntax,
    MissingPercentSign,
    UnknownUnit,
    NotFinite,
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

const EPSILON: f64 = 0.000000000001;

fn isFiniteF64(x: f64): bool {
    return x == x && x <= 1.7976931348623157e308 && x >= -1.7976931348623157e308;
}

fn normalizeZero(v: f64): f64 {
    if (v == 0.0) return 0.0;
    return v;
}

fn absF64(v: f64): f64 {
    if (v < 0.0) return -v;
    return v;
}

fn appendByte(out: []u8, pos: usize, ch: u8): CssPercentageError!usize {
    if (pos >= out.len) return CssPercentageError.BufferTooSmall;
    out[pos] = ch;
    return pos + 1;
}

fn appendSlice(out: []u8, pos: usize, src: []const u8): CssPercentageError!usize {
    if (src.len > out.len - pos) return CssPercentageError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeUint(out: []u8, pos: usize, val_in: u64): CssPercentageError!usize {
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
    if (pos + n > out.len) return CssPercentageError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) {
        out[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

struct ParsedNumberPrefix {
    value: f64,
    consumed: usize,
}

/// Parses a CSS `<number-token>` prefix starting at index 0 of `s`.
fn parseNumberPrefix(s: []const u8): CssPercentageError!ParsedNumberPrefix {
    if (s.len == 0) return CssPercentageError.EmptyInput;

    let i: usize = 0;
    let neg = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
    } else if (s[i] == '+') {
        i += 1;
    }
    if (i >= s.len) return CssPercentageError.InvalidSyntax;

    let int_part: f64 = 0.0;
    let has_int_digits = false;
    while (i < s.len && isDigit(s[i])) {
        has_int_digits = true;
        int_part = int_part * 10.0 + @as(f64, @floatFromInt(s[i] - '0'));
        i += 1;
    }

    let frac_part: f64 = 0.0;
    let has_frac_digits = false;
    if (i < s.len && s[i] == '.') {
        i += 1;
        let scale: f64 = 0.1;
        while (i < s.len && isDigit(s[i])) {
            has_frac_digits = true;
            frac_part += @as(f64, @floatFromInt(s[i] - '0')) * scale;
            scale *= 0.1;
            i += 1;
        }
        if (!has_frac_digits) return CssPercentageError.InvalidSyntax;
    }

    if (!has_int_digits && !has_frac_digits) return CssPercentageError.InvalidSyntax;

    let val: f64 = int_part + frac_part;

    // Optional exponent `[eE] [+-]? digits`
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        const exp_mark = i;
        let j = i + 1;
        let exp_neg = false;
        if (j < s.len && (s[j] == '-' || s[j] == '+')) {
            exp_neg = s[j] == '-';
            j += 1;
        }
        let exp_val: i32 = 0;
        let has_exp_digits = false;
        while (j < s.len && isDigit(s[j])) {
            has_exp_digits = true;
            if (exp_val < 10000) {
                exp_val = exp_val * 10 + @as(i32, @intCast(s[j] - '0'));
            }
            j += 1;
        }
        if (has_exp_digits) {
            i = j;
            if (exp_val > 308) {
                if (exp_neg) {
                    val = 0.0;
                } else {
                    return CssPercentageError.NotFinite;
                }
            } else {
                let mult: f64 = 1.0;
                let k: i32 = 0;
                while (k < exp_val) {
                    mult *= 10.0;
                    k += 1;
                }
                if (exp_neg) {
                    val /= mult;
                } else {
                    val *= mult;
                }
            }
        } else {
            // Not a valid exponent (e.g. unit `em` or `ex` starting with 'e').
            i = exp_mark;
        }
    }

    if (neg) val = 0.0 - val;
    if (!isFiniteF64(val)) return CssPercentageError.NotFinite;
    const final_val: f64 = normalizeZero(val);
    return ParsedNumberPrefix{ value: final_val, consumed: i };
}

/// Serializes a finite f64 in Bun-aligned CSS minimal form (up to 6 decimal digits,
/// stripping trailing zeros, normalizing `-0` to `0`, and optionally omitting leading `0`
/// for `0 < |v| < 1` when `minify` is true).
fn writeCssF64(v_in: f64, minify: bool, out: []u8, start_pos: usize): CssPercentageError!usize {
    if (!isFiniteF64(v_in)) return CssPercentageError.NotFinite;
    let pos = start_pos;
    let v = normalizeZero(v_in);
    if (v == 0.0) {
        return appendByte(out, pos, '0');
    }

    let neg = false;
    if (v < 0.0) {
        neg = true;
        v = -v;
    }

    // Round to 6 fractional digits for deterministic CSS serialization
    const scaled_f = v * 1000000.0 + 0.5;
    if (scaled_f > 1.8e18) return CssPercentageError.NotFinite;
    const scaled: u64 = @as(u64, @intFromFloat(scaled_f));
    const int_part: u64 = scaled / 1000000;
    let frac_part: u64 = scaled % 1000000;

    if (int_part == 0 && frac_part == 0) {
        return appendByte(out, pos, '0');
    }

    if (neg) {
        pos = appendByte(out, pos, '-')?;
    }

    if (int_part > 0 || !minify || frac_part == 0) {
        pos = writeUint(out, pos, int_part)?;
    }

    if (frac_part > 0) {
        pos = appendByte(out, pos, '.')?;
        let digits: [6]u8 = [0; 6];
        let d: usize = 6;
        while (d > 0) {
            d -= 1;
            digits[d] = @as(u8, @intCast('0' + (frac_part % 10)));
            frac_part /= 10;
        }
        let end_d: usize = 6;
        while (end_d > 0 && digits[end_d - 1] == '0') {
            end_d -= 1;
        }
        pos = appendSlice(out, pos, digits[0..end_d])?;
    }
    return pos;
}

/// W3C CSS Values Level 4 `<percentage>` value.
/// Stores normalized `unit_value` where `100% == 1.0`, `50% == 0.5`, `0.5% == 0.005`
/// (matching Bun's `css.css_values.percentage.Percentage`).
pub struct CssPercentage {
    unit_value: f64 = 0.0,

    static fn zero(): CssPercentage {
        return CssPercentage{ unit_value: 0.0 };
    }

    static fn fromUnitValue(v: f64): CssPercentage {
        return CssPercentage{ unit_value: normalizeZero(v) };
    }

    static fn fromPercent(pct: f64): CssPercentage {
        return CssPercentage{ unit_value: normalizeZero(pct / 100.0) };
    }

    /// Parses a CSS `<percentage-token>` (e.g. `"50%"`, `"-0.25%"`, `".5%"`, `"1e2%"`).
    static fn parse(raw: []const u8): CssPercentageError!CssPercentage {
        const s = trimSpaces(raw);
        if (s.len == 0) return CssPercentageError.EmptyInput;

        const parsed = parseNumberPrefix(s)?;
        if (parsed.consumed >= s.len) return CssPercentageError.MissingPercentSign;
        if (s[parsed.consumed] != '%' || parsed.consumed + 1 != s.len) {
            return CssPercentageError.InvalidSyntax;
        }
        return CssPercentage.fromPercent(parsed.value);
    }

    /// Returns the percentage display value (e.g. `0.5` -> `50.0`).
    fn toPercent(): f64 {
        return normalizeZero(_.unit_value * 100.0);
    }

    fn isZero(): bool {
        return _.unit_value == 0.0;
    }

    /// Returns `+1.0`, `-1.0`, or `0.0` matching CSS `sign()`.
    fn sign(): f64 {
        if (_.unit_value > 0.0) return 1.0;
        if (_.unit_value < 0.0) return -1.0;
        return 0.0;
    }

    fn isSignPositive(): bool {
        return _.unit_value > 0.0;
    }

    fn isSignNegative(): bool {
        return _.unit_value < 0.0;
    }

    fn add(other: CssPercentage): CssPercentage {
        return CssPercentage.fromUnitValue(_.unit_value + other.unit_value);
    }

    fn sub(other: CssPercentage): CssPercentage {
        return CssPercentage.fromUnitValue(_.unit_value - other.unit_value);
    }

    fn mulF64(factor: f64): CssPercentage {
        return CssPercentage.fromUnitValue(_.unit_value * factor);
    }

    fn clamp(min_unit: f64, max_unit: f64): CssPercentage {
        if (_.unit_value < min_unit) return CssPercentage.fromUnitValue(min_unit);
        if (_.unit_value > max_unit) return CssPercentage.fromUnitValue(max_unit);
        return CssPercentage.fromUnitValue(_.unit_value);
    }

    fn clampZeroOne(): CssPercentage {
        return _.clamp(0.0, 1.0);
    }

    /// Resolves this percentage against a reference `base` value (`base * unit_value`).
    fn resolve(base: f64): f64 {
        return normalizeZero(base * _.unit_value);
    }

    fn eql(other: CssPercentage): bool {
        return absF64(_.unit_value - other.unit_value) <= EPSILON;
    }

    /// Returns `-1` (less), `0` (equal), or `1` (greater).
    fn partialCmp(other: CssPercentage): i32 {
        if (_.eql(other)) return 0;
        if (_.unit_value < other.unit_value) return -1;
        return 1;
    }

    /// Serializes `<percentage>` to `out`.
    /// When `minify` is true and `0 < |unit_value| < 0.01` (i.e. `0 < |pct| < 1`),
    /// omits the leading `0` (e.g. `0.5%` -> `.5%`, `-0.25%` -> `-.25%`), matching Bun.
    fn toCss(minify: bool, out: []u8): CssPercentageError!usize {
        const pct = _.toPercent();
        let pos = writeCssF64(pct, minify, out, 0)?;
        pos = appendByte(out, pos, '%')?;
        return pos;
    }
}

/// Discriminator for `CssNumberOrPercentage`.
pub enum CssNumberOrPercentageKind {
    number,
    percentage,
}

/// W3C CSS `<number-or-percentage>` (aligned with Bun's `NumberOrPercentage`).
/// When `kind == .percentage`, `raw_value` stores the normalized `unit_value` (`50%` == `0.5`),
/// so `intoF64()` uniformly yields the normalized scalar value.
pub struct CssNumberOrPercentage {
    kind: CssNumberOrPercentageKind = CssNumberOrPercentageKind.number,
    raw_value: f64 = 0.0,

    static fn fromNumber(v: f64): CssNumberOrPercentage {
        return CssNumberOrPercentage{
            kind: CssNumberOrPercentageKind.number,
            raw_value: normalizeZero(v),
        };
    }

    static fn fromPercentage(p: CssPercentage): CssNumberOrPercentage {
        return CssNumberOrPercentage{
            kind: CssNumberOrPercentageKind.percentage,
            raw_value: normalizeZero(p.unit_value),
        };
    }

    static fn fromPercent(pct: f64): CssNumberOrPercentage {
        return CssNumberOrPercentage.fromPercentage(CssPercentage.fromPercent(pct));
    }

    static fn parse(raw: []const u8): CssPercentageError!CssNumberOrPercentage {
        const s = trimSpaces(raw);
        if (s.len == 0) return CssPercentageError.EmptyInput;

        const parsed = parseNumberPrefix(s)?;
        if (parsed.consumed == s.len) {
            return CssNumberOrPercentage.fromNumber(parsed.value);
        }
        if (s[parsed.consumed] == '%' && parsed.consumed + 1 == s.len) {
            return CssNumberOrPercentage.fromPercent(parsed.value);
        }
        return CssPercentageError.InvalidSyntax;
    }

    fn isNumber(): bool {
        return _.kind == CssNumberOrPercentageKind.number;
    }

    fn isPercentage(): bool {
        return _.kind == CssNumberOrPercentageKind.percentage;
    }

    fn isZero(): bool {
        return _.raw_value == 0.0;
    }

    /// Returns the normalized f64 (`0.5` for both `0.5` and `50%`).
    fn intoF64(): f64 {
        return _.raw_value;
    }

    fn asPercentage(): CssPercentage {
        return CssPercentage.fromUnitValue(_.raw_value);
    }

    fn clampZeroOne(): CssNumberOrPercentage {
        let v = _.raw_value;
        if (v < 0.0) v = 0.0;
        if (v > 1.0) v = 1.0;
        return CssNumberOrPercentage{ kind: _.kind, raw_value: normalizeZero(v) };
    }

    fn eql(other: CssNumberOrPercentage): bool {
        return _.kind == other.kind && absF64(_.raw_value - other.raw_value) <= EPSILON;
    }

    fn toCss(minify: bool, out: []u8): CssPercentageError!usize {
        if (_.kind == CssNumberOrPercentageKind.percentage) {
            return _.asPercentage().toCss(minify, out);
        }
        return writeCssF64(_.raw_value, minify, out, 0);
    }
}

/// Supported CSS length unit tags for zero-allocation `CssLengthOrPercentage`.
pub enum CssLengthUnitTag {
    px,
    em,
    rem,
    vw,
    vh,
    vmin,
    vmax,
    ch,
    ex,
    cm,
    mm,
    in_unit,
    pt,
    pc,
}

fn parseLengthUnitTag(unit_str: []const u8): CssPercentageError!CssLengthUnitTag {
    if (eqlIgnoreCase(unit_str, "px")) return CssLengthUnitTag.px;
    if (eqlIgnoreCase(unit_str, "em")) return CssLengthUnitTag.em;
    if (eqlIgnoreCase(unit_str, "rem")) return CssLengthUnitTag.rem;
    if (eqlIgnoreCase(unit_str, "vw")) return CssLengthUnitTag.vw;
    if (eqlIgnoreCase(unit_str, "vh")) return CssLengthUnitTag.vh;
    if (eqlIgnoreCase(unit_str, "vmin")) return CssLengthUnitTag.vmin;
    if (eqlIgnoreCase(unit_str, "vmax")) return CssLengthUnitTag.vmax;
    if (eqlIgnoreCase(unit_str, "ch")) return CssLengthUnitTag.ch;
    if (eqlIgnoreCase(unit_str, "ex")) return CssLengthUnitTag.ex;
    if (eqlIgnoreCase(unit_str, "cm")) return CssLengthUnitTag.cm;
    if (eqlIgnoreCase(unit_str, "mm")) return CssLengthUnitTag.mm;
    if (eqlIgnoreCase(unit_str, "in")) return CssLengthUnitTag.in_unit;
    if (eqlIgnoreCase(unit_str, "pt")) return CssLengthUnitTag.pt;
    if (eqlIgnoreCase(unit_str, "pc")) return CssLengthUnitTag.pc;
    return CssPercentageError.UnknownUnit;
}

pub fn cssLengthUnitName(unit: CssLengthUnitTag): []const u8 {
    return switch (unit) {
        .px => "px",
        .em => "em",
        .rem => "rem",
        .vw => "vw",
        .vh => "vh",
        .vmin => "vmin",
        .vmax => "vmax",
        .ch => "ch",
        .ex => "ex",
        .cm => "cm",
        .mm => "mm",
        .in_unit => "in",
        .pt => "pt",
        .pc => "pc",
    };
}

/// Discriminator for zero-allocation `CssLengthOrPercentage`.
pub enum CssLengthOrPercentageKind {
    length,
    percentage,
    calc_sum,
}

/// Zero-allocation CSS `<length-percentage>` (aligned with Bun's
/// `DimensionPercentage(LengthValue)`). Supports pure `<length>`, pure `<percentage>`,
/// and static 2-term `calc(length +/- percentage)` folding without heap allocation.
pub struct CssLengthOrPercentage {
    kind: CssLengthOrPercentageKind = CssLengthOrPercentageKind.length,
    length_val: f64 = 0.0,
    length_unit: CssLengthUnitTag = CssLengthUnitTag.px,
    percent_unit: f64 = 0.0,

    static fn zero(): CssLengthOrPercentage {
        return CssLengthOrPercentage{
            kind: CssLengthOrPercentageKind.length,
            length_val: 0.0,
            length_unit: CssLengthUnitTag.px,
            percent_unit: 0.0,
        };
    }

    static fn fromLength(val: f64, unit: CssLengthUnitTag): CssLengthOrPercentage {
        return CssLengthOrPercentage{
            kind: CssLengthOrPercentageKind.length,
            length_val: normalizeZero(val),
            length_unit: unit,
            percent_unit: 0.0,
        };
    }

    static fn fromPercentage(p: CssPercentage): CssLengthOrPercentage {
        return CssLengthOrPercentage{
            kind: CssLengthOrPercentageKind.percentage,
            length_val: 0.0,
            length_unit: CssLengthUnitTag.px,
            percent_unit: normalizeZero(p.unit_value),
        };
    }

    static fn fromCalcSum(len_val: f64, unit: CssLengthUnitTag, pct: CssPercentage): CssLengthOrPercentage {
        const lv = normalizeZero(len_val);
        const pv = normalizeZero(pct.unit_value);
        if (lv == 0.0 && pv == 0.0) return CssLengthOrPercentage.zero();
        if (lv == 0.0) return CssLengthOrPercentage.fromPercentage(pct);
        if (pv == 0.0) return CssLengthOrPercentage.fromLength(lv, unit);
        return CssLengthOrPercentage{
            kind: CssLengthOrPercentageKind.calc_sum,
            length_val: lv,
            length_unit: unit,
            percent_unit: pv,
        };
    }

    /// Parses `<length>`, `<percentage>`, or unitless `0`.
    static fn parse(raw: []const u8): CssPercentageError!CssLengthOrPercentage {
        const s = trimSpaces(raw);
        if (s.len == 0) return CssPercentageError.EmptyInput;

        const parsed = parseNumberPrefix(s)?;
        if (parsed.consumed == s.len) {
            if (parsed.value == 0.0) return CssLengthOrPercentage.zero();
            return CssPercentageError.UnknownUnit;
        }
        const suffix = s[parsed.consumed..s.len];
        if (suffix.len == 1 && suffix[0] == '%') {
            return CssLengthOrPercentage.fromPercentage(CssPercentage.fromPercent(parsed.value));
        }
        const unit = parseLengthUnitTag(suffix)?;
        return CssLengthOrPercentage.fromLength(parsed.value, unit);
    }

    fn isZero(): bool {
        if (_.kind == CssLengthOrPercentageKind.length) return _.length_val == 0.0;
        if (_.kind == CssLengthOrPercentageKind.percentage) return _.percent_unit == 0.0;
        return _.length_val == 0.0 && _.percent_unit == 0.0;
    }

    fn mulF64(factor: f64): CssLengthOrPercentage {
        if (_.kind == CssLengthOrPercentageKind.length) {
            return CssLengthOrPercentage.fromLength(_.length_val * factor, _.length_unit);
        }
        if (_.kind == CssLengthOrPercentageKind.percentage) {
            return CssLengthOrPercentage.fromPercentage(CssPercentage.fromUnitValue(_.percent_unit * factor));
        }
        return CssLengthOrPercentage.fromCalcSum(
            _.length_val * factor,
            _.length_unit,
            CssPercentage.fromUnitValue(_.percent_unit * factor),
        );
    }

    /// Attempts to add two `<length-percentage>` values with zero-allocation folding.
    /// Returns `null` if two distinct length units cannot be statically combined (e.g. `px + rem`).
    fn tryAdd(other: CssLengthOrPercentage): CssLengthOrPercentage? {
        if (_.isZero()) return other;
        if (other.isZero()) return _;

        if (_.kind == CssLengthOrPercentageKind.length && other.kind == CssLengthOrPercentageKind.length) {
            if (_.length_unit != other.length_unit) return null;
            return CssLengthOrPercentage.fromLength(_.length_val + other.length_val, _.length_unit);
        }
        if (_.kind == CssLengthOrPercentageKind.percentage && other.kind == CssLengthOrPercentageKind.percentage) {
            return CssLengthOrPercentage.fromPercentage(CssPercentage.fromUnitValue(_.percent_unit + other.percent_unit));
        }
        if (_.kind == CssLengthOrPercentageKind.length && other.kind == CssLengthOrPercentageKind.percentage) {
            return CssLengthOrPercentage.fromCalcSum(_.length_val, _.length_unit, CssPercentage.fromUnitValue(other.percent_unit));
        }
        if (_.kind == CssLengthOrPercentageKind.percentage && other.kind == CssLengthOrPercentageKind.length) {
            return CssLengthOrPercentage.fromCalcSum(other.length_val, other.length_unit, CssPercentage.fromUnitValue(_.percent_unit));
        }
        if (_.kind == CssLengthOrPercentageKind.calc_sum && other.kind == CssLengthOrPercentageKind.length) {
            if (_.length_unit != other.length_unit) return null;
            return CssLengthOrPercentage.fromCalcSum(_.length_val + other.length_val, _.length_unit, CssPercentage.fromUnitValue(_.percent_unit));
        }
        if (_.kind == CssLengthOrPercentageKind.calc_sum && other.kind == CssLengthOrPercentageKind.percentage) {
            return CssLengthOrPercentage.fromCalcSum(_.length_val, _.length_unit, CssPercentage.fromUnitValue(_.percent_unit + other.percent_unit));
        }
        if (_.kind == CssLengthOrPercentageKind.length && other.kind == CssLengthOrPercentageKind.calc_sum) {
            if (_.length_unit != other.length_unit) return null;
            return CssLengthOrPercentage.fromCalcSum(_.length_val + other.length_val, _.length_unit, CssPercentage.fromUnitValue(other.percent_unit));
        }
        if (_.kind == CssLengthOrPercentageKind.percentage && other.kind == CssLengthOrPercentageKind.calc_sum) {
            return CssLengthOrPercentage.fromCalcSum(other.length_val, other.length_unit, CssPercentage.fromUnitValue(_.percent_unit + other.percent_unit));
        }
        // Both calc_sum
        if (_.length_unit != other.length_unit) return null;
        return CssLengthOrPercentage.fromCalcSum(
            _.length_val + other.length_val,
            _.length_unit,
            CssPercentage.fromUnitValue(_.percent_unit + other.percent_unit),
        );
    }

    /// Returns sign (`-1.0`, `0.0`, `+1.0`) when statically determinable, or `null` for mixed-sign `calc_sum`.
    fn trySign(): f64? {
        if (_.kind == CssLengthOrPercentageKind.length) {
            if (_.length_val > 0.0) return 1.0;
            if (_.length_val < 0.0) return -1.0;
            return 0.0;
        }
        if (_.kind == CssLengthOrPercentageKind.percentage) {
            return CssPercentage.fromUnitValue(_.percent_unit).sign();
        }
        if (_.length_val > 0.0 && _.percent_unit > 0.0) return 1.0;
        if (_.length_val < 0.0 && _.percent_unit < 0.0) return -1.0;
        return null;
    }

    /// Partial ordering for same-unit lengths or percentages; returns `null` if incomparable.
    fn partialCmp(other: CssLengthOrPercentage): i32? {
        if (_.kind == CssLengthOrPercentageKind.length && other.kind == CssLengthOrPercentageKind.length) {
            if (_.length_unit != other.length_unit) return null;
            if (absF64(_.length_val - other.length_val) <= EPSILON) return 0;
            if (_.length_val < other.length_val) return -1;
            return 1;
        }
        if (_.kind == CssLengthOrPercentageKind.percentage && other.kind == CssLengthOrPercentageKind.percentage) {
            return CssPercentage.fromUnitValue(_.percent_unit).partialCmp(CssPercentage.fromUnitValue(other.percent_unit));
        }
        return null;
    }

    fn eql(other: CssLengthOrPercentage): bool {
        if (_.kind != other.kind) return false;
        if (_.kind == CssLengthOrPercentageKind.length) {
            return _.length_unit == other.length_unit && absF64(_.length_val - other.length_val) <= EPSILON;
        }
        if (_.kind == CssLengthOrPercentageKind.percentage) {
            return absF64(_.percent_unit - other.percent_unit) <= EPSILON;
        }
        return _.length_unit == other.length_unit &&
            absF64(_.length_val - other.length_val) <= EPSILON &&
            absF64(_.percent_unit - other.percent_unit) <= EPSILON;
    }

    fn toCss(minify: bool, out: []u8): CssPercentageError!usize {
        if (_.kind == CssLengthOrPercentageKind.percentage) {
            return CssPercentage.fromUnitValue(_.percent_unit).toCss(minify, out);
        }
        if (_.kind == CssLengthOrPercentageKind.length) {
            if (_.length_val == 0.0) {
                return appendByte(out, 0, '0');
            }
            let pos = writeCssF64(_.length_val, minify, out, 0)?;
            pos = appendSlice(out, pos, cssLengthUnitName(_.length_unit))?;
            return pos;
        }

        // calc_sum: positive operand placed first when possible (matching Bun's DimensionPercentage.addImpl)
        let pos = appendSlice(out, 0, "calc(")?;
        if (_.length_val < 0.0 && _.percent_unit > 0.0) {
            pos = writeCssF64(_.percent_unit * 100.0, minify, out, pos)?;
            pos = appendSlice(out, pos, "% - ")?;
            pos = writeCssF64(-_.length_val, minify, out, pos)?;
            pos = appendSlice(out, pos, cssLengthUnitName(_.length_unit))?;
        } else {
            pos = writeCssF64(_.length_val, minify, out, pos)?;
            pos = appendSlice(out, pos, cssLengthUnitName(_.length_unit))?;
            if (_.percent_unit < 0.0) {
                pos = appendSlice(out, pos, " - ")?;
                pos = writeCssF64(-_.percent_unit * 100.0, minify, out, pos)?;
            } else {
                pos = appendSlice(out, pos, " + ")?;
                pos = writeCssF64(_.percent_unit * 100.0, minify, out, pos)?;
            }
            pos = appendByte(out, pos, '%')?;
        }
        pos = appendByte(out, pos, ')')?;
        return pos;
    }
}
