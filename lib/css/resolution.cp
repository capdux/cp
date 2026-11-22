// Source: Absorbed from /data/dev/bun/src/css/values/resolution.cp | Synced: 2026-11-22T18:14:02Z
// ============================================================================
// lib/css/resolution.cp — Zero-Alloc CSS Values Level 4 `<resolution>` Engine
// ============================================================================
// Implements W3C CSS Values and Units Level 4 `<resolution>` parsing (`dpi`,
// `dpcm`, `dppx`, `x` case-insensitively), exact physical unit conversion
// (`1dppx = 96dpi = 96/2.54 dpcm`), cross-unit comparison / media query range
// matching, arithmetic helpers, and Bun-aligned minimal CSS serialization
// (emitting `"x"` vs `"dppx"` according to browser `x_resolution_unit` target
// compatibility, with zero dynamic heap allocation).

import { CssNumber, CssNumberError } from "./number.cp";

/// W3C CSS Values Level 4: 1dppx == 96dpi.
pub const CSS_DPI_PER_DPPX: f64 = 96.0;
/// Exact physical length factor: 1in == 2.54cm.
pub const CSS_CM_PER_INCH: f64 = 2.54;
/// W3C CSS Values Level 4: 1dppx == 96 / 2.54 dpcm (~37.79527559055118).
pub const CSS_DPCM_PER_DPPX: f64 = 96.0 / 2.54;

pub err CssResolutionError {
    EmptyInput,
    InvalidSyntax,
    UnknownUnit,
    NegativeValue,
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

fn eqlIgnoreAsciiCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn trimSpaces(raw: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = raw.len;
    while (start < end && isSpace(raw[start])) start += 1;
    while (end > start && isSpace(raw[end - 1])) end -= 1;
    return raw[start..end];
}

fn isFiniteF64(x: f64): bool {
    return x == x && x <= 1.7976931348623157e308 && x >= -1.7976931348623157e308;
}

/// Canonical CSS `<resolution>` unit variants aligned with Bun `Resolution`.
pub enum CssResolutionUnitKind {
    dpi,
    dpcm,
    dppx,

    /// Case-insensitive ASCII unit parser accepting `dpi`, `dpcm`, `dppx`, and `x`.
    static fn parse(unit: []const u8): CssResolutionError!CssResolutionUnitKind {
        if (unit.len == 0) return CssResolutionError.UnknownUnit;
        if (eqlIgnoreAsciiCase(unit, "dpi")) return CssResolutionUnitKind.dpi;
        if (eqlIgnoreAsciiCase(unit, "dpcm")) return CssResolutionUnitKind.dpcm;
        if (eqlIgnoreAsciiCase(unit, "dppx") || eqlIgnoreAsciiCase(unit, "x")) {
            return CssResolutionUnitKind.dppx;
        }
        return CssResolutionError.UnknownUnit;
    }

    /// Returns the serialized unit suffix. When `use_x_alias` is true (matching
    /// Bun's `targets.isCompatible(.x_resolution_unit)`), `dppx` emits `"x"`.
    fn asStr(use_x_alias: bool): []const u8 {
        if (_ == CssResolutionUnitKind.dpi) return "dpi";
        if (_ == CssResolutionUnitKind.dpcm) return "dpcm";
        if (use_x_alias) return "x";
        return "dppx";
    }
}

/// W3C CSS Values Level 4 `<resolution>` dimension value.
pub struct CssResolutionVal {
    unit: CssResolutionUnitKind = CssResolutionUnitKind.dppx,
    value: f64 = 1.0,

    static fn dpi(v: f64): CssResolutionVal {
        return CssResolutionVal{ unit: CssResolutionUnitKind.dpi, value: v };
    }

    static fn dpcm(v: f64): CssResolutionVal {
        return CssResolutionVal{ unit: CssResolutionUnitKind.dpcm, value: v };
    }

    static fn dppx(v: f64): CssResolutionVal {
        return CssResolutionVal{ unit: CssResolutionUnitKind.dppx, value: v };
    }

    /// Constructs from a pre-tokenized CSS dimension `(value, unit)`, matching
    /// Bun's `Resolution.tryFromToken`.
    static fn tryFromDimension(value: f64, unit: []const u8): CssResolutionError!CssResolutionVal {
        if (!isFiniteF64(value)) return CssResolutionError.NotFinite;
        const parsed_unit = CssResolutionUnitKind.parse(unit)?;
        return CssResolutionVal{ unit: parsed_unit, value: value };
    }

    /// Parses a CSS `<resolution>` dimension token: `<number><unit>` with no
    /// internal whitespace between the number and unit.
    static fn parse(raw: []const u8): CssResolutionError!CssResolutionVal {
        const s = trimSpaces(raw);
        if (s.len == 0) return CssResolutionError.EmptyInput;

        // Locate boundary between `<number>` prefix and unit identifier.
        let i: usize = 0;
        if (s[i] == '+' || s[i] == '-') i += 1;
        if (i >= s.len) return CssResolutionError.InvalidSyntax;

        let has_digits = false;
        while (i < s.len && isDigit(s[i])) {
            has_digits = true;
            i += 1;
        }
        if (i < s.len && s[i] == '.') {
            i += 1;
            let frac_digits = false;
            while (i < s.len && isDigit(s[i])) {
                frac_digits = true;
                has_digits = true;
                i += 1;
            }
            if (!frac_digits) return CssResolutionError.InvalidSyntax;
        }
        if (!has_digits) return CssResolutionError.InvalidSyntax;

        // Optional scientific exponent `[eE][+-]?digits` (only consume if valid digits follow).
        if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
            let exp_idx = i + 1;
            if (exp_idx < s.len && (s[exp_idx] == '+' || s[exp_idx] == '-')) {
                exp_idx += 1;
            }
            let exp_digits: usize = 0;
            while (exp_idx < s.len && isDigit(s[exp_idx])) {
                exp_idx += 1;
                exp_digits += 1;
            }
            if (exp_digits > 0) {
                i = exp_idx;
            }
        }

        if (i >= s.len) return CssResolutionError.InvalidSyntax;
        // Disallow whitespace between number and unit (CSS dimension token rule).
        if (isSpace(s[i])) return CssResolutionError.InvalidSyntax;

        const num_slice = s[0..i];
        const unit_slice = s[i..s.len];

        const parsed_num = CssNumber.parse(num_slice) catch |e| {
            if (e == CssNumberError.NotFinite) return CssResolutionError.NotFinite;
            return CssResolutionError.InvalidSyntax;
        };
        const parsed_unit = CssResolutionUnitKind.parse(unit_slice)?;

        return CssResolutionVal{
            unit: parsed_unit,
            value: parsed_num.value,
        };
    }

    /// Parses a non-negative CSS `<resolution>` (required by Media Queries and `image-set()`).
    static fn parseNonNegative(raw: []const u8): CssResolutionError!CssResolutionVal {
        const res = CssResolutionVal.parse(raw)?;
        if (res.value < 0.0) return CssResolutionError.NegativeValue;
        return res;
    }

    /// Converts this resolution into `dppx` (dots per CSS pixel).
    fn toDppx(): f64 {
        if (_.unit == CssResolutionUnitKind.dppx) return _.value;
        if (_.unit == CssResolutionUnitKind.dpi) return _.value / CSS_DPI_PER_DPPX;
        return _.value / CSS_DPCM_PER_DPPX;
    }

    /// Converts this resolution into `dpi` (dots per inch).
    fn toDpi(): f64 {
        if (_.unit == CssResolutionUnitKind.dpi) return _.value;
        if (_.unit == CssResolutionUnitKind.dppx) return _.value * CSS_DPI_PER_DPPX;
        return _.value * CSS_CM_PER_INCH;
    }

    /// Converts this resolution into `dpcm` (dots per centimeter).
    fn toDpcm(): f64 {
        if (_.unit == CssResolutionUnitKind.dpcm) return _.value;
        if (_.unit == CssResolutionUnitKind.dpi) return _.value / CSS_CM_PER_INCH;
        return _.value * CSS_DPCM_PER_DPPX;
    }

    /// Converts this resolution into the requested target unit.
    fn convertTo(target: CssResolutionUnitKind): CssResolutionVal {
        if (_.unit == target) return _;
        if (target == CssResolutionUnitKind.dpi) {
            return CssResolutionVal{ unit: target, value: _.toDpi() };
        }
        if (target == CssResolutionUnitKind.dpcm) {
            return CssResolutionVal{ unit: target, value: _.toDpcm() };
        }
        return CssResolutionVal{ unit: target, value: _.toDppx() };
    }

    /// Adds a scalar delta in the current unit (aligned with Bun `Resolution.addF32`).
    fn addF64(delta: f64): CssResolutionVal {
        return CssResolutionVal{ unit: _.unit, value: _.value + delta };
    }

    /// Adds another resolution (converted into this value's unit).
    fn add(other: CssResolutionVal): CssResolutionVal {
        const converted = other.convertTo(_.unit);
        return CssResolutionVal{ unit: _.unit, value: _.value + converted.value };
    }

    /// Scales the resolution magnitude while preserving its unit.
    fn scale(factor: f64): CssResolutionVal {
        return CssResolutionVal{ unit: _.unit, value: _.value * factor };
    }

    fn isNegative(): bool {
        return _.value < 0.0;
    }

    fn isZero(): bool {
        return _.value == 0.0;
    }

    /// Strict structural equality (same unit and identical value), matching Bun `Resolution.eql`.
    fn eql(other: CssResolutionVal): bool {
        return _.unit == other.unit && _.value == other.value;
    }

    /// Physical resolution equality across units within `eps_dppx` tolerance.
    fn approxEqlPhysical(other: CssResolutionVal, eps_dppx: f64): bool {
        const a = _.toDppx();
        const b = other.toDppx();
        const d = if (a > b) (a - b) else (b - a);
        return d <= eps_dppx;
    }

    /// Compares physical resolution across units, returning `-1`, `0`, or `1`.
    fn comparePhysical(other: CssResolutionVal): i32 {
        const a = _.toDppx();
        const b = other.toDppx();
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }

    /// Evaluates whether this device resolution satisfies an optional `[min_res, max_res]`
    /// media query range (inclusive on both bounds).
    fn matchesRange(min_res: CssResolutionVal?, max_res: CssResolutionVal?): bool {
        const cur = _.toDppx();
        if (lo := min_res) {
            if (cur + 1.0e-9 < lo.toDppx()) return false;
        }
        if (hi := max_res) {
            if (cur - 1.0e-9 > hi.toDppx()) return false;
        }
        return true;
    }

    /// Serializes `<resolution>` in minimal CSS form into caller-provided `out`.
    /// When `use_x_alias` is true, `dppx` is shortened to `"x"` (matching Bun's
    /// `targets.isCompatible(.x_resolution_unit)` minification).
    fn toCss(use_x_alias: bool, out: []u8): CssResolutionError!usize {
        if (!isFiniteF64(_.value)) return CssResolutionError.NotFinite;
        const num_len = CssNumber.fromF64(_.value).toCss(out) catch |e| {
            if (e == CssNumberError.BufferTooSmall) return CssResolutionError.BufferTooSmall;
            if (e == CssNumberError.NotFinite) return CssResolutionError.NotFinite;
            return CssResolutionError.InvalidSyntax;
        };
        const suffix = _.unit.asStr(use_x_alias);
        if (num_len + suffix.len > out.len) return CssResolutionError.BufferTooSmall;
        let j: usize = 0;
        while (j < suffix.len) {
            out[num_len + j] = suffix[j];
            j += 1;
        }
        return num_len + suffix.len;
    }
}
