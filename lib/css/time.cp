// Source: Absorbed from /data/dev/bun/src/css/values/time.cp
//! CSS `<time>` value (`s` / `ms`): parsing, unit conversion, arithmetic,
//! comparison, and shortest-form CSS serialization (`0.05s` -> `50ms`, `200ms` -> `0.2s`).

import std from "std";

export enum TimeUnit: u8 {
    seconds = 1,
    milliseconds = 2,
}

export struct CssTime {
    unit: TimeUnit = .seconds,
    value: f32 = 0.0,

    pub static fn seconds(v: f32): CssTime {
        return CssTime{ unit: .seconds, value: v };
    }

    pub static fn milliseconds(v: f32): CssTime {
        return CssTime{ unit: .milliseconds, value: v };
    }

    pub fn isZero(): bool {
        return _.value == 0.0;
    }

    pub fn toMs(): f32 {
        return if (_.unit == .seconds) _.value * 1000.0 else _.value;
    }

    pub fn toSeconds(): f32 {
        return if (_.unit == .seconds) _.value else _.value / 1000.0;
    }

    pub fn add(other: CssTime): CssTime {
        if (_.unit == .seconds) {
            return CssTime.seconds(_.value + other.toSeconds());
        }
        return CssTime.milliseconds(_.value + other.toMs());
    }

    pub fn sub(other: CssTime): CssTime {
        if (_.unit == .seconds) {
            return CssTime.seconds(_.value - other.toSeconds());
        }
        return CssTime.milliseconds(_.value - other.toMs());
    }

    pub fn mul(factor: f32): CssTime {
        return CssTime{ unit: _.unit, value: _.value * factor };
    }

    pub fn eql(other: CssTime): bool {
        return _.toMs() == other.toMs();
    }

    pub fn cmp(other: CssTime): i8 {
        const a = _.toMs();
        const b = other.toMs();
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }

    /// Parse a CSS `<time>` token such as `"250ms"`, `"0.5s"`, `"-1.25S"`, `".5s"`.
    pub static fn parse(input: []const u8): CssTime? {
        const s = trimAscii(input);
        if (s.len < 2) return null;

        let unit: TimeUnit = .seconds;
        let num_str: []const u8 = s;

        if (s.len >= 3 && (s[s.len - 2] == 'm' || s[s.len - 2] == 'M') && (s[s.len - 1] == 's' || s[s.len - 1] == 'S')) {
            unit = .milliseconds;
            num_str = s[0 .. s.len - 2];
        } else if (s[s.len - 1] == 's' || s[s.len - 1] == 'S') {
            unit = .seconds;
            num_str = s[0 .. s.len - 1];
        } else {
            return null;
        }

        if (num_str.len == 0) return null;
        const val = std.fmt.parseFloat(f32, num_str) catch return null;
        if (std.math.isNan(val) || std.math.isInf(val)) return null;
        return CssTime{ unit: unit, value: val };
    }

    /// Serialize to shortest canonical CSS form into `out` (`0.1s` is shorter than `100ms`,
    /// while `50ms` is shorter than `0.05s`).
    pub fn toCss(out: []u8): []const u8 {
        const ms = _.toMs();
        if (ms == 0.0) {
            return std.fmt.bufPrint(out, "0s", .{}) catch out[0..0];
        }
        const abs_ms = @abs(ms);
        if (abs_ms >= 100.0) {
            return std.fmt.bufPrint(out, "{d}s", .{ms / 1000.0}) catch out[0..0];
        }
        return std.fmt.bufPrint(out, "{d}ms", .{ms}) catch out[0..0];
    }
}

fn trimAscii(s: []const u8): []const u8 {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\n' || s[a] == '\r')) : (a += 1) {}
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\n' || s[b - 1] == '\r')) : (b -= 1) {}
    return s[a..b];
}
