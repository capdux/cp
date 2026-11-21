// Source: Absorbed from /data/dev/bun/src/css/values/alpha.cp, /data/dev/bun/src/css/values/percentage.cp, /data/dev/bun/src/css/values/url.cp
//! CSS primitive value types: `<alpha-value>`, `<percentage>`, `<number-percentage>`,
//! and `<url>` absolute/relative/fragment/scheme classification.

import std from "std";

/// A CSS `<percentage>` value stored as a unit fraction (`50%` => `0.5`).
export struct Percentage {
    v: f32 = 0.0,

    pub static fn fromFraction(f: f32): Percentage {
        return Percentage{ v: f };
    }

    pub static fn fromPercent(p: f32): Percentage {
        return Percentage{ v: p / 100.0 };
    }

    pub fn asPercent(): f32 {
        return _.v * 100.0;
    }

    pub fn isZero(): bool {
        return _.v == 0.0;
    }

    pub fn clamp01(): Percentage {
        const c: f32 = if (_.v < 0.0) 0.0 else if (_.v > 1.0) 1.0 else _.v;
        return Percentage{ v: c };
    }

    pub fn add(other: Percentage): Percentage {
        return Percentage{ v: _.v + other.v };
    }

    pub fn mul(factor: f32): Percentage {
        return Percentage{ v: _.v * factor };
    }

    pub fn eql(other: Percentage): bool {
        return _.v == other.v;
    }

    pub static fn parse(input: []const u8): Percentage? {
        const s = trimAscii(input);
        if (s.len < 2 || s[s.len - 1] != '%') return null;
        const num = std.fmt.parseFloat(f32, s[0 .. s.len - 1]) catch return null;
        if (std.math.isNan(num) || std.math.isInf(num)) return null;
        return Percentage.fromPercent(num);
    }

    pub fn toCss(out: []u8): []const u8 {
        return std.fmt.bufPrint(out, "{d}%", .{_.asPercent()}) catch out[0..0];
    }
}

export enum NumOrPctKind: u8 {
    number = 0,
    percentage = 1,
}

/// A CSS `<number> | <percentage>` union value.
export struct NumberOrPercentage {
    kind: NumOrPctKind = .number,
    value: f32 = 0.0, // unit fraction when percentage

    pub static fn fromNumber(n: f32): NumberOrPercentage {
        return NumberOrPercentage{ kind: .number, value: n };
    }

    pub static fn fromPercentage(p: Percentage): NumberOrPercentage {
        return NumberOrPercentage{ kind: .percentage, value: p.v };
    }

    pub fn asUnitValue(): f32 {
        return _.value;
    }

    pub static fn parse(input: []const u8): NumberOrPercentage? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        if (s[s.len - 1] == '%') {
            const p = Percentage.parse(s) ?? return null;
            return NumberOrPercentage.fromPercentage(p);
        }
        const n = std.fmt.parseFloat(f32, s) catch return null;
        if (std.math.isNan(n) || std.math.isInf(n)) return null;
        return NumberOrPercentage.fromNumber(n);
    }

    pub fn toCss(out: []u8): []const u8 {
        if (_.kind == .percentage) {
            const p = Percentage{ v: _.value };
            return p.toCss(out);
        }
        return std.fmt.bufPrint(out, "{d}", .{_.value}) catch out[0..0];
    }
}

/// A CSS `<alpha-value>` (`<number> | <percentage>`, always stored/serialized as a `[0, 1]` number).
export struct AlphaValue {
    v: f32 = 1.0,

    pub static fn parse(input: []const u8): AlphaValue? {
        const np = NumberOrPercentage.parse(input) ?? return null;
        return AlphaValue{ v: np.asUnitValue() };
    }

    pub fn clamped(): AlphaValue {
        const c: f32 = if (_.v < 0.0) 0.0 else if (_.v > 1.0) 1.0 else _.v;
        return AlphaValue{ v: c };
    }

    pub fn toByte(): u8 {
        return @intFromFloat(@round(_.clamped().v * 255.0));
    }

    pub fn eql(other: AlphaValue): bool {
        return _.v == other.v;
    }

    pub fn toCss(out: []u8): []const u8 {
        return std.fmt.bufPrint(out, "{d}", .{_.v}) catch out[0..0];
    }
}

/// Classify whether a CSS `url(...)` target is absolute (root-relative `/`, fragment `#`, or RFC 3986 scheme)
/// vs relative (`./`, `../`, or bare path).
export fn isAbsoluteCssUrl(url: []const u8): bool {
    if (url.len == 0) return false;
    if (url[0] == '.') return false;
    if (url[0] == '/' || url[0] == '#') return true;
    const c0 = url[0];
    const is_alpha = (c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z');
    if (!is_alpha) return false;

    let i: usize = 1;
    while (i < url.len) : (i += 1) {
        const c = url[i];
        if (c == ':') return true;
        const ok = (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9')
            || c == '+'
            || c == '-'
            || c == '.';
        if (!ok) return false;
    }
    return false;
}

/// Check if a CSS URL is a data URI (`data:...`).
export fn isDataCssUrl(url: []const u8): bool {
    if (url.len < 5) return false;
    return toLower(url[0]) == 'd'
        && toLower(url[1]) == 'a'
        && toLower(url[2]) == 't'
        && toLower(url[3]) == 'a'
        && url[4] == ':';
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn trimAscii(s: []const u8): []const u8 {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\n' || s[a] == '\r')) : (a += 1) {}
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\n' || s[b - 1] == '\r')) : (b -= 1) {}
    return s[a..b];
}
