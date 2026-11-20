// Source: /data/dev/bun/src/css/values/values.cp
// lib/css/engine/values.cp
// Industrial CSS Value Types: Angle, Time, Resolution, Ratio, AlphaValue, UrlValue, Position, EasingFunction, Gradient, CssRect

const std = @import("std");
import { CssColor } from "../color.cp";
import { engAppendBytes, engFormatFloat, engSplitCommaTop, engSplitSpaceTop } from "./_utils.cp";

// ============================================================================
// 1. Angle (<angle>: deg, rad, grad, turn)
// ============================================================================

pub enum AngleUnit {
    Deg,
    Rad,
    Grad,
    Turn,
}

pub struct Angle {
    value: f64 = 0.0;
    unit: AngleUnit = AngleUnit.Deg;

    pub static fn deg(v: f64): Angle { return Angle { value: v, unit: AngleUnit.Deg }; }
    pub static fn rad(v: f64): Angle { return Angle { value: v, unit: AngleUnit.Rad }; }
    pub static fn grad(v: f64): Angle { return Angle { value: v, unit: AngleUnit.Grad }; }
    pub static fn turn(v: f64): Angle { return Angle { value: v, unit: AngleUnit.Turn }; }
    pub static fn zero(): Angle { return Angle { value: 0.0, unit: AngleUnit.Deg }; }

    pub static fn parse(raw: []const u8): Angle? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        if (std.mem.eql(u8, s, "0")) return Angle.zero();

        if (s.len > 4 && std.ascii.eqlIgnoreCase(s[s.len - 4 ..], "grad")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 4]) catch return null;
            return Angle.grad(v);
        }
        if (s.len > 4 && std.ascii.eqlIgnoreCase(s[s.len - 4 ..], "turn")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 4]) catch return null;
            return Angle.turn(v);
        }
        if (s.len > 3 && std.ascii.eqlIgnoreCase(s[s.len - 3 ..], "deg")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 3]) catch return null;
            return Angle.deg(v);
        }
        if (s.len > 3 && std.ascii.eqlIgnoreCase(s[s.len - 3 ..], "rad")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 3]) catch return null;
            return Angle.rad(v);
        }
        return null;
    }

    pub fn toDegrees(): f64 {
        return switch (_.unit) {
            .Deg => _.value,
            .Rad => _.value * (180.0 / std.math.pi),
            .Grad => _.value * 0.9,
            .Turn => _.value * 360.0,
        };
    }

    pub fn toRadians(): f64 {
        return _.toDegrees() * (std.math.pi / 180.0);
    }

    pub fn toTurns(): f64 {
        return _.toDegrees() / 360.0;
    }

    pub fn toGradians(): f64 {
        return _.toDegrees() / 0.9;
    }

    pub fn normalizeDeg(): f64 {
        let d = @mod(_.toDegrees(), 360.0);
        if (d < 0.0) d += 360.0;
        return d;
    }

    pub fn isZero(): bool {
        return @abs(_.value) < 0.000000001;
    }

    pub fn add(other: Angle): Angle {
        if (_.unit == other.unit) {
            return Angle { value: _.value + other.value, unit: _.unit };
        }
        return Angle.deg(_.toDegrees() + other.toDegrees());
    }

    pub fn eql(other: Angle): bool {
        return @abs(_.toDegrees() - other.toDegrees()) < 0.000001;
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        if (_.isZero()) {
            return std.fmt.bufPrint(out_buf, "0deg", .{}) catch "";
        }
        let num_buf: [64]u8 = [0; 64];
        if (minify && _.unit == AngleUnit.Rad) {
            let deg_buf: [64]u8 = [0; 64];
            const deg_s = engFormatFloat(_.toDegrees(), deg_buf[0..64]);
            const rad_s = engFormatFloat(_.value, num_buf[0..64]);
            if (deg_s.len + 3 < rad_s.len + 3) {
                return std.fmt.bufPrint(out_buf, "{s}deg", .{deg_s}) catch "";
            }
            return std.fmt.bufPrint(out_buf, "{s}rad", .{rad_s}) catch "";
        }
        const n = engFormatFloat(_.value, num_buf[0..64]);
        const sfx: []const u8 = switch (_.unit) {
            .Deg => "deg",
            .Rad => "rad",
            .Grad => "grad",
            .Turn => "turn",
        };
        return std.fmt.bufPrint(out_buf, "{s}{s}", .{ n, sfx }) catch "";
    }
}

// ============================================================================
// 2. Time (<time>: s, ms)
// ============================================================================

pub enum TimeUnit {
    Seconds,
    Milliseconds,
}

pub struct Time {
    value: f64 = 0.0;
    unit: TimeUnit = TimeUnit.Seconds;

    pub static fn seconds(v: f64): Time { return Time { value: v, unit: TimeUnit.Seconds }; }
    pub static fn ms(v: f64): Time { return Time { value: v, unit: TimeUnit.Milliseconds }; }
    pub static fn zero(): Time { return Time { value: 0.0, unit: TimeUnit.Seconds }; }

    pub static fn parse(raw: []const u8): Time? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "ms")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 2]) catch return null;
            return Time.ms(v);
        }
        if (s.len > 1 && (s[s.len - 1] == 's' || s[s.len - 1] == 'S')) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 1]) catch return null;
            return Time.seconds(v);
        }
        return null;
    }

    pub fn toMs(): f64 {
        return switch (_.unit) {
            .Seconds => _.value * 1000.0,
            .Milliseconds => _.value,
        };
    }

    pub fn toSeconds(): f64 {
        return switch (_.unit) {
            .Seconds => _.value,
            .Milliseconds => _.value / 1000.0,
        };
    }

    pub fn isZero(): bool {
        return @abs(_.value) < 0.000000001;
    }

    pub fn eql(other: Time): bool {
        return @abs(_.toMs() - other.toMs()) < 0.000001;
    }

    /// LightningCSS canonical shortest formatting: 0.1s is shorter than 100ms, 50ms shorter than 0.05s.
    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        if (_.isZero()) {
            return std.fmt.bufPrint(out_buf, "0s", .{}) catch "";
        }
        let num_buf: [64]u8 = [0; 64];
        if (!minify) {
            const n = engFormatFloat(_.value, num_buf[0..64]);
            const u: []const u8 = if (_.unit == TimeUnit.Seconds) "s" else "ms";
            return std.fmt.bufPrint(out_buf, "{s}{s}", .{ n, u }) catch "";
        }
        const total_ms = _.toMs();
        if (total_ms >= 100.0 || total_ms <= -100.0) {
            const sec = total_ms / 1000.0;
            const n = engFormatFloat(sec, num_buf[0..64]);
            if (n.len >= 2 && n[0] == '0' && n[1] == '.') {
                return std.fmt.bufPrint(out_buf, "{s}s", .{n[1..]}) catch "";
            }
            return std.fmt.bufPrint(out_buf, "{s}s", .{n}) catch "";
        }
        const n = engFormatFloat(total_ms, num_buf[0..64]);
        return std.fmt.bufPrint(out_buf, "{s}ms", .{n}) catch "";
    }
}

// ============================================================================
// 3. Resolution (<resolution>: dpi, dpcm, dppx, x)
// ============================================================================

pub enum ResolutionUnit {
    Dpi,
    Dpcm,
    Dppx,
}

pub struct Resolution {
    value: f64 = 1.0;
    unit: ResolutionUnit = ResolutionUnit.Dppx;

    pub static fn parse(raw: []const u8): Resolution? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        if (s.len > 4 && std.ascii.eqlIgnoreCase(s[s.len - 4 ..], "dpcm")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 4]) catch return null;
            return Resolution { value: v, unit: ResolutionUnit.Dpcm };
        }
        if (s.len > 4 && std.ascii.eqlIgnoreCase(s[s.len - 4 ..], "dppx")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 4]) catch return null;
            return Resolution { value: v, unit: ResolutionUnit.Dppx };
        }
        if (s.len > 3 && std.ascii.eqlIgnoreCase(s[s.len - 3 ..], "dpi")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 3]) catch return null;
            return Resolution { value: v, unit: ResolutionUnit.Dpi };
        }
        if (s.len > 1 && (s[s.len - 1] == 'x' || s[s.len - 1] == 'X')) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 1]) catch return null;
            return Resolution { value: v, unit: ResolutionUnit.Dppx };
        }
        return null;
    }

    pub fn toDpi(): f64 {
        return switch (_.unit) {
            .Dpi => _.value,
            .Dpcm => _.value * 2.54,
            .Dppx => _.value * 96.0,
        };
    }

    pub fn toDppx(): f64 {
        return switch (_.unit) {
            .Dpi => _.value / 96.0,
            .Dpcm => (_.value * 2.54) / 96.0,
            .Dppx => _.value,
        };
    }

    pub fn eql(other: Resolution): bool {
        return @abs(_.toDpi() - other.toDpi()) < 0.0001;
    }

    pub fn format(out_buf: []u8, use_x: bool): []const u8 {
        let num_buf: [64]u8 = [0; 64];
        const n = engFormatFloat(_.value, num_buf[0..64]);
        const u: []const u8 = switch (_.unit) {
            .Dpi => "dpi",
            .Dpcm => "dpcm",
            .Dppx => if (use_x) "x" else "dppx",
        };
        return std.fmt.bufPrint(out_buf, "{s}{s}", .{ n, u }) catch "";
    }
}

// ============================================================================
// 4. AlphaValue, Ratio, UrlValue, CssRect
// ============================================================================

pub struct AlphaValue {
    v: f64 = 1.0;

    pub static fn parse(raw: []const u8): AlphaValue? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        let val: f64 = 0.0;
        if (s[s.len - 1] == '%') {
            const pct = std.fmt.parseFloat(f64, s[0 .. s.len - 1]) catch return null;
            val = pct / 100.0;
        } else {
            val = std.fmt.parseFloat(f64, s) catch return null;
        }
        if (val < 0.0) val = 0.0;
        if (val > 1.0) val = 1.0;
        return AlphaValue { v: val };
    }

    pub fn format(out_buf: []u8): []const u8 {
        return engFormatFloat(_.v, out_buf);
    }
}

pub struct Ratio {
    numerator: f64 = 1.0;
    denominator: f64 = 1.0;

    pub static fn parse(raw: []const u8): Ratio? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        if (slash := std.mem.indexOfScalar(u8, s, '/')) {
            const num = std.fmt.parseFloat(f64, std.mem.trim(u8, s[0..slash], " \t")) catch return null;
            const den = std.fmt.parseFloat(f64, std.mem.trim(u8, s[slash + 1 ..], " \t")) catch return null;
            if (num < 0.0 || den <= 0.0) return null;
            return Ratio { numerator: num, denominator: den };
        }
        const num = std.fmt.parseFloat(f64, s) catch return null;
        if (num < 0.0) return null;
        return Ratio { numerator: num, denominator: 1.0 };
    }

    pub fn value(): f64 {
        if (_.denominator == 0.0) return 0.0;
        return _.numerator / _.denominator;
    }

    pub fn eql(other: Ratio): bool {
        return @abs(_.value() - other.value()) < 0.000001;
    }

    pub fn format(out_buf: []u8): []const u8 {
        let nb: [64]u8 = [0; 64];
        const ns = engFormatFloat(_.numerator, nb[0..64]);
        if (@abs(_.denominator - 1.0) < 0.000001) {
            return std.fmt.bufPrint(out_buf, "{s}", .{ns}) catch "";
        }
        let db: [64]u8 = [0; 64];
        const ds = engFormatFloat(_.denominator, db[0..64]);
        return std.fmt.bufPrint(out_buf, "{s} / {s}", .{ ns, ds }) catch "";
    }
}

pub struct UrlValue {
    url: []const u8 = "";
    was_quoted: bool = false;

    pub static fn parse(raw: []const u8): UrlValue? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len < 5 || !std.ascii.startsWithIgnoreCase(s, "url(") || s[s.len - 1] != ')') {
            return null;
        }
        const inner = std.mem.trim(u8, s[4 .. s.len - 1], " \t\r\n");
        if (inner.len >= 2 && ((inner[0] == '"' && inner[inner.len - 1] == '"') || (inner[0] == '\'' && inner[inner.len - 1] == '\''))) {
            return UrlValue { url: inner[1 .. inner.len - 1], was_quoted: true };
        }
        return UrlValue { url: inner, was_quoted: false };
    }

    pub fn isAbsolute(): bool {
        if (_.url.len == 0) return false;
        if (_.url[0] == '.' ) return false;
        if (_.url[0] == '/' || _.url[0] == '#') return true;
        return std.mem.indexOfScalar(u8, _.url, ':') != null;
    }

    pub fn isDataUrl(): bool {
        return std.ascii.startsWithIgnoreCase(_.url, "data:");
    }

    pub fn isFragment(): bool {
        return _.url.len > 0 && _.url[0] == '#';
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        let needs_quotes = !minify;
        if (minify) {
            for (c of _.url) {
                if (c == ' ' || c == ')' || c == '(' || c == '"' || c == '\'' || c == '\t' || c == '\n') {
                    needs_quotes = true;
                    break;
                }
            }
        }
        if (needs_quotes) {
            return std.fmt.bufPrint(out_buf, "url(\"{s}\")", .{_.url}) catch "";
        }
        return std.fmt.bufPrint(out_buf, "url({s})", .{_.url}) catch "";
    }
}

pub struct CssRect {
    top: []const u8 = "0";
    right: []const u8 = "0";
    bottom: []const u8 = "0";
    left: []const u8 = "0";

    pub static fn all(v: []const u8): CssRect {
        return CssRect { top: v, right: v, bottom: v, left: v };
    }

    pub static fn parse(raw: []const u8): CssRect? {
        let parts: [8][]const u8 = ["", "", "", "", "", "", "", ""];
        const n = engSplitSpaceTop(raw, parts[0..8]);
        if (n == 0 || n > 4) return null;
        if (n == 1) return CssRect { top: parts[0], right: parts[0], bottom: parts[0], left: parts[0] };
        if (n == 2) return CssRect { top: parts[0], right: parts[1], bottom: parts[0], left: parts[1] };
        if (n == 3) return CssRect { top: parts[0], right: parts[1], bottom: parts[2], left: parts[1] };
        return CssRect { top: parts[0], right: parts[1], bottom: parts[2], left: parts[3] };
    }

    pub fn format(out_buf: []u8): []const u8 {
        const same_v = std.mem.eql(u8, _.top, _.bottom);
        const same_h = std.mem.eql(u8, _.right, _.left);
        if (same_v && same_h && std.mem.eql(u8, _.top, _.right)) {
            return std.fmt.bufPrint(out_buf, "{s}", .{_.top}) catch "";
        }
        if (same_v && same_h) {
            return std.fmt.bufPrint(out_buf, "{s} {s}", .{ _.top, _.right }) catch "";
        }
        if (same_h) {
            return std.fmt.bufPrint(out_buf, "{s} {s} {s}", .{ _.top, _.right, _.bottom }) catch "";
        }
        return std.fmt.bufPrint(out_buf, "{s} {s} {s} {s}", .{ _.top, _.right, _.bottom, _.left }) catch "";
    }
}

// ============================================================================
// 5. Position (<position>: 1/2/3/4-value keywords + offsets)
// ============================================================================

pub enum HorizontalKeyword {
    Left,
    Center,
    Right,

    static fn parse(s: []const u8): HorizontalKeyword? {
        if (std.ascii.eqlIgnoreCase(s, "left")) return HorizontalKeyword.Left;
        if (std.ascii.eqlIgnoreCase(s, "center")) return HorizontalKeyword.Center;
        if (std.ascii.eqlIgnoreCase(s, "right")) return HorizontalKeyword.Right;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Left => "left",
            .Center => "center",
            .Right => "right",
        };
    }

    fn toPercent(): f64 {
        return switch (_) {
            .Left => 0.0,
            .Center => 50.0,
            .Right => 100.0,
        };
    }
}

pub enum VerticalKeyword {
    Top,
    Center,
    Bottom,

    static fn parse(s: []const u8): VerticalKeyword? {
        if (std.ascii.eqlIgnoreCase(s, "top")) return VerticalKeyword.Top;
        if (std.ascii.eqlIgnoreCase(s, "center")) return VerticalKeyword.Center;
        if (std.ascii.eqlIgnoreCase(s, "bottom")) return VerticalKeyword.Bottom;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Top => "top",
            .Center => "center",
            .Bottom => "bottom",
        };
    }

    fn toPercent(): f64 {
        return switch (_) {
            .Top => 0.0,
            .Center => 50.0,
            .Bottom => 100.0,
        };
    }
}

pub struct Position {
    x_keyword: HorizontalKeyword = HorizontalKeyword.Center;
    x_offset: []const u8 = "";
    y_keyword: VerticalKeyword = VerticalKeyword.Center;
    y_offset: []const u8 = "";
    x_is_value: bool = false;
    y_is_value: bool = false;

    pub static fn center(): Position {
        return Position {};
    }

    pub static fn parse(raw: []const u8): Position? {
        let parts: [6][]const u8 = ["", "", "", "", "", ""];
        const n = engSplitSpaceTop(raw, parts[0..6]);
        if (n == 0 || n > 4) return null;

        if (n == 1) {
            if (hk := HorizontalKeyword.parse(parts[0])) {
                return Position { x_keyword: hk, y_keyword: VerticalKeyword.Center };
            }
            if (vk := VerticalKeyword.parse(parts[0])) {
                return Position { x_keyword: HorizontalKeyword.Center, y_keyword: vk };
            }
            return Position { x_offset: parts[0], x_is_value: true, y_keyword: VerticalKeyword.Center };
        }

        if (n == 2) {
            const p0_h = HorizontalKeyword.parse(parts[0]);
            const p0_v = VerticalKeyword.parse(parts[0]);
            const p1_h = HorizontalKeyword.parse(parts[1]);
            const p1_v = VerticalKeyword.parse(parts[1]);

            if (p0_h != null && p1_v != null) {
                return Position { x_keyword: p0_h.?, y_keyword: p1_v.? };
            }
            if (p0_v != null && p1_h != null) {
                return Position { x_keyword: p1_h.?, y_keyword: p0_v.? };
            }
            if (p0_h != null && p1_h == null && p1_v == null) {
                return Position { x_keyword: p0_h.?, y_offset: parts[1], y_is_value: true };
            }
            if (p0_v != null && p1_h == null && p1_v == null) {
                return Position { x_keyword: HorizontalKeyword.Center, y_keyword: p0_v.?, y_offset: parts[1] };
            }
            if (p0_h == null && p0_v == null && p1_v != null) {
                return Position { x_offset: parts[0], x_is_value: true, y_keyword: p1_v.? };
            }
            if (p0_h == null && p0_v == null && p1_h == null && p1_v == null) {
                return Position { x_offset: parts[0], x_is_value: true, y_offset: parts[1], y_is_value: true };
            }
            return null;
        }

        // 3 or 4 value form: <side> <offset>? <side> <offset>?
        let res = Position {};
        let idx: usize = 0;
        let got_x = false;
        let got_y = false;
        while (idx < n) {
            const tok = parts[idx];
            if (hk := HorizontalKeyword.parse(tok)) {
                if (got_x) return null;
                got_x = true;
                res.x_keyword = hk;
                if (idx + 1 < n && HorizontalKeyword.parse(parts[idx + 1]) == null && VerticalKeyword.parse(parts[idx + 1]) == null) {
                    res.x_offset = parts[idx + 1];
                    idx += 2;
                    continue;
                }
                idx += 1;
            } else if (vk := VerticalKeyword.parse(tok)) {
                if (got_y) return null;
                got_y = true;
                res.y_keyword = vk;
                if (idx + 1 < n && HorizontalKeyword.parse(parts[idx + 1]) == null && VerticalKeyword.parse(parts[idx + 1]) == null) {
                    res.y_offset = parts[idx + 1];
                    idx += 2;
                    continue;
                }
                idx += 1;
            } else {
                return null;
            }
        }
        return res;
    }

    pub fn isCenter(): bool {
        return !_.x_is_value && !_.y_is_value &&
            _.x_keyword == HorizontalKeyword.Center && _.y_keyword == VerticalKeyword.Center &&
            _.x_offset.len == 0 && _.y_offset.len == 0;
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        if (_.isCenter()) {
            return std.fmt.bufPrint(out_buf, "{s}", .{ if (minify) "50%" else "center" }) catch "";
        }
        if (minify && !_.x_is_value && !_.y_is_value && _.x_offset.len == 0 && _.y_offset.len == 0) {
            const xp = _.x_keyword.toPercent();
            const yp = _.y_keyword.toPercent();
            if (xp == 0.0 && yp == 0.0) return std.fmt.bufPrint(out_buf, "0 0", .{}) catch "";
            let xb: [32]u8 = [0; 32];
            let yb: [32]u8 = [0; 32];
            const xs = if (xp == 0.0) "0" else std.fmt.bufPrint(xb[0..32], "{d}%", .{@as(i32, @intFromFloat(xp))}) catch "0";
            const ys = if (yp == 0.0) "0" else std.fmt.bufPrint(yb[0..32], "{d}%", .{@as(i32, @intFromFloat(yp))}) catch "0";
            return std.fmt.bufPrint(out_buf, "{s} {s}", .{ xs, ys }) catch "";
        }
        let pos: usize = 0;
        if (_.x_is_value) {
            pos = engAppendBytes(out_buf, pos, _.x_offset);
        } else {
            pos = engAppendBytes(out_buf, pos, _.x_keyword.asStr());
            if (_.x_offset.len > 0) {
                pos = engAppendBytes(out_buf, pos, " ");
                pos = engAppendBytes(out_buf, pos, _.x_offset);
            }
        }
        if (_.y_is_value) {
            pos = engAppendBytes(out_buf, pos, " ");
            pos = engAppendBytes(out_buf, pos, _.y_offset);
        } else if (_.y_keyword != VerticalKeyword.Center || _.y_offset.len > 0 || _.x_offset.len > 0) {
            pos = engAppendBytes(out_buf, pos, " ");
            pos = engAppendBytes(out_buf, pos, _.y_keyword.asStr());
            if (_.y_offset.len > 0) {
                pos = engAppendBytes(out_buf, pos, " ");
                pos = engAppendBytes(out_buf, pos, _.y_offset);
            }
        }
        return out_buf[0..pos];
    }
}

// ============================================================================
// 6. EasingFunction (linear, ease*, cubic-bezier(), steps() + eval(t))
// ============================================================================

pub enum StepPosition {
    Start,
    End,
    JumpStart,
    JumpEnd,
    JumpNone,
    JumpBoth,

    static fn parse(raw: []const u8): StepPosition? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "start")) return StepPosition.Start;
        if (std.ascii.eqlIgnoreCase(s, "end")) return StepPosition.End;
        if (std.ascii.eqlIgnoreCase(s, "jump-start")) return StepPosition.JumpStart;
        if (std.ascii.eqlIgnoreCase(s, "jump-end")) return StepPosition.JumpEnd;
        if (std.ascii.eqlIgnoreCase(s, "jump-none")) return StepPosition.JumpNone;
        if (std.ascii.eqlIgnoreCase(s, "jump-both")) return StepPosition.JumpBoth;
        return null;
    }

    fn canonicalStr(): []const u8 {
        return switch (_) {
            .Start, .JumpStart => "start",
            .End, .JumpEnd => "end",
            .JumpNone => "jump-none",
            .JumpBoth => "jump-both",
        };
    }
}

pub enum EasingKind {
    Linear,
    Ease,
    EaseIn,
    EaseOut,
    EaseInOut,
    CubicBezier,
    Steps,
}

fn bezierSample(a: f64, b: f64, s: f64): f64 {
    const u = 1.0 - s;
    return 3.0 * u * u * s * a + 3.0 * u * s * s * b + s * s * s;
}

fn bezierDeriv(a: f64, b: f64, s: f64): f64 {
    const u = 1.0 - s;
    return 3.0 * u * u * a + 6.0 * u * s * (b - a) + 3.0 * s * s * (1.0 - b);
}

fn solveCubicBezier(x1: f64, y1: f64, x2: f64, y2: f64, t: f64): f64 {
    if (t <= 0.0) return 0.0;
    if (t >= 1.0) return 1.0;

    // Initial guess + Newton-Raphson + bisection fallback
    let lo: f64 = 0.0;
    let hi: f64 = 1.0;
    let s: f64 = t;
    let iter: usize = 0;
    while (iter < 12) {
        const x = bezierSample(x1, x2, s) - t;
        if (@abs(x) < 0.0000001) break;
        if (x < 0.0) lo = s else hi = s;
        const dx = bezierDeriv(x1, x2, s);
        if (@abs(dx) > 0.000001) {
            const next_s = s - x / dx;
            if (next_s > lo && next_s < hi) {
                s = next_s;
            } else {
                s = 0.5 * (lo + hi);
            }
        } else {
            s = 0.5 * (lo + hi);
        }
        iter += 1;
    }
    return bezierSample(y1, y2, s);
}

pub struct EasingFunction {
    kind: EasingKind = EasingKind.Ease;
    x1: f64 = 0.25;
    y1: f64 = 0.1;
    x2: f64 = 0.25;
    y2: f64 = 1.0;
    step_count: i32 = 1;
    step_pos: StepPosition = StepPosition.End;

    pub static fn linear(): EasingFunction {
        return EasingFunction { kind: EasingKind.Linear, x1: 0.0, y1: 0.0, x2: 1.0, y2: 1.0 };
    }

    pub static fn ease(): EasingFunction {
        return EasingFunction { kind: EasingKind.Ease, x1: 0.25, y1: 0.1, x2: 0.25, y2: 1.0 };
    }

    pub static fn easeIn(): EasingFunction {
        return EasingFunction { kind: EasingKind.EaseIn, x1: 0.42, y1: 0.0, x2: 1.0, y2: 1.0 };
    }

    pub static fn easeOut(): EasingFunction {
        return EasingFunction { kind: EasingKind.EaseOut, x1: 0.0, y1: 0.0, x2: 0.58, y2: 1.0 };
    }

    pub static fn easeInOut(): EasingFunction {
        return EasingFunction { kind: EasingKind.EaseInOut, x1: 0.42, y1: 0.0, x2: 0.58, y2: 1.0 };
    }

    pub static fn cubicBezier(x1: f64, y1: f64, x2: f64, y2: f64): EasingFunction? {
        if (x1 < 0.0 || x1 > 1.0 || x2 < 0.0 || x2 > 1.0) return null;
        return EasingFunction { kind: EasingKind.CubicBezier, x1: x1, y1: y1, x2: x2, y2: y2 };
    }

    pub static fn steps(count: i32, pos: StepPosition): EasingFunction? {
        if (count < 1) return null;
        if (pos == StepPosition.JumpNone && count < 2) return null;
        return EasingFunction { kind: EasingKind.Steps, step_count: count, step_pos: pos };
    }

    pub static fn parse(raw: []const u8): EasingFunction? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        if (std.ascii.eqlIgnoreCase(s, "linear")) return EasingFunction.linear();
        if (std.ascii.eqlIgnoreCase(s, "ease")) return EasingFunction.ease();
        if (std.ascii.eqlIgnoreCase(s, "ease-in")) return EasingFunction.easeIn();
        if (std.ascii.eqlIgnoreCase(s, "ease-out")) return EasingFunction.easeOut();
        if (std.ascii.eqlIgnoreCase(s, "ease-in-out")) return EasingFunction.easeInOut();
        if (std.ascii.eqlIgnoreCase(s, "step-start")) return EasingFunction.steps(1, StepPosition.Start);
        if (std.ascii.eqlIgnoreCase(s, "step-end")) return EasingFunction.steps(1, StepPosition.End);

        const open = std.mem.indexOfScalar(u8, s, '(') ?? return null;
        if (s[s.len - 1] != ')') return null;
        const fn_name = std.mem.trim(u8, s[0..open], " \t");
        const inner = s[open + 1 .. s.len - 1];

        let args: [4][]const u8 = ["", "", "", ""];
        const argc = engSplitCommaTop(inner, args[0..4]);

        if (std.ascii.eqlIgnoreCase(fn_name, "cubic-bezier")) {
            if (argc != 4) return null;
            const x1 = std.fmt.parseFloat(f64, args[0]) catch return null;
            const y1 = std.fmt.parseFloat(f64, args[1]) catch return null;
            const x2 = std.fmt.parseFloat(f64, args[2]) catch return null;
            const y2 = std.fmt.parseFloat(f64, args[3]) catch return null;
            return EasingFunction.cubicBezier(x1, y1, x2, y2);
        }
        if (std.ascii.eqlIgnoreCase(fn_name, "steps")) {
            if (argc < 1 || argc > 2) return null;
            const cnt = std.fmt.parseInt(i32, args[0], 10) catch return null;
            let pos = StepPosition.End;
            if (argc == 2) {
                if (p_opt := StepPosition.parse(args[1])) {
                    pos = p_opt;
                } else {
                    return null;
                }
            }
            return EasingFunction.steps(cnt, pos);
        }
        return null;
    }

    pub fn isEase(): bool {
        if (_.kind == EasingKind.Ease) return true;
        if (_.kind == EasingKind.CubicBezier) {
            return @abs(_.x1 - 0.25) < 0.000001 && @abs(_.y1 - 0.1) < 0.000001 &&
                @abs(_.x2 - 0.25) < 0.000001 && @abs(_.y2 - 1.0) < 0.000001;
        }
        return false;
    }

    /// Evaluates the easing function at normalized input progress `t in [0, 1]`.
    pub fn eval(t: f64): f64 {
        return switch (_.kind) {
            .Linear => if (t < 0.0) 0.0 else if (t > 1.0) 1.0 else t,
            .Ease => solveCubicBezier(0.25, 0.1, 0.25, 1.0, t),
            .EaseIn => solveCubicBezier(0.42, 0.0, 1.0, 1.0, t),
            .EaseOut => solveCubicBezier(0.0, 0.0, 0.58, 1.0, t),
            .EaseInOut => solveCubicBezier(0.42, 0.0, 0.58, 1.0, t),
            .CubicBezier => solveCubicBezier(_.x1, _.y1, _.x2, _.y2, t),
            .Steps => {
                const n_f: f64 = @floatFromInt(_.step_count);
                let step: f64 = @floor(t * n_f);
                if (_.step_pos == StepPosition.Start || _.step_pos == StepPosition.JumpStart || _.step_pos == StepPosition.JumpBoth) {
                    step += 1.0;
                }
                if (t < 0.0) step = 0.0;
                if (t >= 1.0) {
                    if (_.step_pos == StepPosition.JumpNone) return 1.0;
                }
                let jumps = n_f;
                if (_.step_pos == StepPosition.JumpNone) jumps = n_f - 1.0;
                if (_.step_pos == StepPosition.JumpBoth) jumps = n_f + 1.0;
                const out = step / jumps;
                if (out < 0.0) return 0.0;
                if (out > 1.0) return 1.0;
                return out;
            },
        };
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        if (_.isEase()) return std.fmt.bufPrint(out_buf, "ease", .{}) catch "";
        if (_.kind == EasingKind.Linear) return std.fmt.bufPrint(out_buf, "linear", .{}) catch "";
        if (_.kind == EasingKind.EaseIn) return std.fmt.bufPrint(out_buf, "ease-in", .{}) catch "";
        if (_.kind == EasingKind.EaseOut) return std.fmt.bufPrint(out_buf, "ease-out", .{}) catch "";
        if (_.kind == EasingKind.EaseInOut) return std.fmt.bufPrint(out_buf, "ease-in-out", .{}) catch "";

        if (_.kind == EasingKind.CubicBezier) {
            if (@abs(_.x1 - 0.42) < 0.000001 && @abs(_.y1) < 0.000001 && @abs(_.x2 - 1.0) < 0.000001 && @abs(_.y2 - 1.0) < 0.000001) {
                return std.fmt.bufPrint(out_buf, "ease-in", .{}) catch "";
            }
            if (@abs(_.x1) < 0.000001 && @abs(_.y1) < 0.000001 && @abs(_.x2 - 0.58) < 0.000001 && @abs(_.y2 - 1.0) < 0.000001) {
                return std.fmt.bufPrint(out_buf, "ease-out", .{}) catch "";
            }
            if (@abs(_.x1 - 0.42) < 0.000001 && @abs(_.y1) < 0.000001 && @abs(_.x2 - 0.58) < 0.000001 && @abs(_.y2 - 1.0) < 0.000001) {
                return std.fmt.bufPrint(out_buf, "ease-in-out", .{}) catch "";
            }
            let b1: [32]u8 = [0; 32];
            let b2: [32]u8 = [0; 32];
            let b3: [32]u8 = [0; 32];
            let b4: [32]u8 = [0; 32];
            const s1 = engFormatFloat(_.x1, b1[0..32]);
            const s2 = engFormatFloat(_.y1, b2[0..32]);
            const s3 = engFormatFloat(_.x2, b3[0..32]);
            const s4 = engFormatFloat(_.y2, b4[0..32]);
            if (minify) {
                return std.fmt.bufPrint(out_buf, "cubic-bezier({s},{s},{s},{s})", .{ s1, s2, s3, s4 }) catch "";
            }
            return std.fmt.bufPrint(out_buf, "cubic-bezier({s}, {s}, {s}, {s})", .{ s1, s2, s3, s4 }) catch "";
        }

        // Steps
        if (_.step_count == 1 && (_.step_pos == StepPosition.Start || _.step_pos == StepPosition.JumpStart)) {
            return std.fmt.bufPrint(out_buf, "step-start", .{}) catch "";
        }
        if (_.step_count == 1 && (_.step_pos == StepPosition.End || _.step_pos == StepPosition.JumpEnd)) {
            return std.fmt.bufPrint(out_buf, "step-end", .{}) catch "";
        }
        if (_.step_pos == StepPosition.End || _.step_pos == StepPosition.JumpEnd) {
            return std.fmt.bufPrint(out_buf, "steps({d})", .{_.step_count}) catch "";
        }
        if (minify) {
            return std.fmt.bufPrint(out_buf, "steps({d},{s})", .{ _.step_count, _.step_pos.canonicalStr() }) catch "";
        }
        return std.fmt.bufPrint(out_buf, "steps({d}, {s})", .{ _.step_count, _.step_pos.canonicalStr() }) catch "";
    }
}

// ============================================================================
// 7. Gradients (LinearGradient, RadialGradient, ConicGradient, ColorStop)
// ============================================================================

pub struct ColorStop {
    color: []const u8 = "";
    pos1: []const u8 = "";
    pos2: []const u8 = "";

    pub static fn parse(raw: []const u8): ColorStop? {
        let parts: [4][]const u8 = ["", "", "", ""];
        const n = engSplitSpaceTop(raw, parts[0..4]);
        if (n == 0 || n > 3) return null;
        return ColorStop {
            color: parts[0],
            pos1: if (n >= 2) parts[1] else "",
            pos2: if (n >= 3) parts[2] else "",
        };
    }

    pub fn hasDoublePosition(): bool {
        return _.pos1.len > 0 && _.pos2.len > 0;
    }

    pub fn format(out_buf: []u8): []const u8 {
        if (_.pos1.len == 0) return std.fmt.bufPrint(out_buf, "{s}", .{_.color}) catch "";
        if (_.pos2.len == 0) return std.fmt.bufPrint(out_buf, "{s} {s}", .{ _.color, _.pos1 }) catch "";
        return std.fmt.bufPrint(out_buf, "{s} {s} {s}", .{ _.color, _.pos1, _.pos2 }) catch "";
    }
}

pub enum GradientKind {
    Linear,
    RepeatingLinear,
    Radial,
    RepeatingRadial,
    Conic,
    RepeatingConic,
}

pub struct Gradient {
    kind: GradientKind = GradientKind.Linear;
    direction: []const u8 = "";
    stops: [16]ColorStop = std.mem.zeroes([16]ColorStop);
    stop_count: usize = 0;

    pub static fn parse(raw: []const u8): Gradient? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        const open = std.mem.indexOfScalar(u8, s, '(') ?? return null;
        if (s[s.len - 1] != ')') return null;
        const fn_name = std.mem.trim(u8, s[0..open], " \t");
        const inner = s[open + 1 .. s.len - 1];

        let kind = GradientKind.Linear;
        if (std.ascii.eqlIgnoreCase(fn_name, "linear-gradient") || std.ascii.eqlIgnoreCase(fn_name, "-webkit-linear-gradient") || std.ascii.eqlIgnoreCase(fn_name, "-moz-linear-gradient")) {
            kind = GradientKind.Linear;
        } else if (std.ascii.eqlIgnoreCase(fn_name, "repeating-linear-gradient")) {
            kind = GradientKind.RepeatingLinear;
        } else if (std.ascii.eqlIgnoreCase(fn_name, "radial-gradient") || std.ascii.eqlIgnoreCase(fn_name, "-webkit-radial-gradient")) {
            kind = GradientKind.Radial;
        } else if (std.ascii.eqlIgnoreCase(fn_name, "repeating-radial-gradient")) {
            kind = GradientKind.RepeatingRadial;
        } else if (std.ascii.eqlIgnoreCase(fn_name, "conic-gradient")) {
            kind = GradientKind.Conic;
        } else if (std.ascii.eqlIgnoreCase(fn_name, "repeating-conic-gradient")) {
            kind = GradientKind.RepeatingConic;
        } else {
            return null;
        }

        let parts: [18][]const u8 = [""; 18];
        const n = engSplitCommaTop(inner, parts[0..18]);
        if (n == 0) return null;

        let grad = Gradient { kind: kind };
        let start_idx: usize = 0;

        // Detect prelude / direction in parts[0]
        const first = parts[0];
        let is_prelude = false;
        if (kind == GradientKind.Linear || kind == GradientKind.RepeatingLinear) {
            if (std.ascii.startsWithIgnoreCase(first, "to ") || Angle.parse(first) != null) {
                is_prelude = true;
            }
        } else if (kind == GradientKind.Radial || kind == GradientKind.RepeatingRadial) {
            if (std.ascii.startsWithIgnoreCase(first, "circle") ||
                std.ascii.startsWithIgnoreCase(first, "ellipse") ||
                std.ascii.startsWithIgnoreCase(first, "at ") ||
                std.ascii.startsWithIgnoreCase(first, "closest-") ||
                std.ascii.startsWithIgnoreCase(first, "farthest-"))
            {
                is_prelude = true;
            }
        } else if (kind == GradientKind.Conic || kind == GradientKind.RepeatingConic) {
            if (std.ascii.startsWithIgnoreCase(first, "from ") || std.ascii.startsWithIgnoreCase(first, "at ")) {
                is_prelude = true;
            }
        }

        if (is_prelude) {
            grad.direction = first;
            start_idx = 1;
        }

        let i = start_idx;
        while (i < n && grad.stop_count < 16) {
            if (cs := ColorStop.parse(parts[i])) {
                grad.stops[grad.stop_count] = cs;
                grad.stop_count += 1;
            }
            i += 1;
        }
        if (grad.stop_count == 0) return null;
        return grad;
    }

    pub fn hasDoublePositionStops(): bool {
        let i: usize = 0;
        while (i < _.stop_count) {
            if (_.stops[i].hasDoublePosition()) return true;
            i += 1;
        }
        return false;
    }

    pub fn format(out_buf: []u8, minify: bool, lower_double_stops: bool): []const u8 {
        let pos: usize = 0;
        const name: []const u8 = switch (_.kind) {
            .Linear => "linear-gradient(",
            .RepeatingLinear => "repeating-linear-gradient(",
            .Radial => "radial-gradient(",
            .RepeatingRadial => "repeating-radial-gradient(",
            .Conic => "conic-gradient(",
            .RepeatingConic => "repeating-conic-gradient(",
        };
        pos = engAppendBytes(out_buf, pos, name);
        const sep: []const u8 = if (minify) "," else ", ";

        let emitted_any = false;
        if (_.direction.len > 0) {
            // Omit default `to bottom` / `180deg` when minifying linear gradients
            const omit_dir = minify && (_.kind == GradientKind.Linear || _.kind == GradientKind.RepeatingLinear) &&
                (std.ascii.eqlIgnoreCase(_.direction, "to bottom") || std.ascii.eqlIgnoreCase(_.direction, "180deg"));
            if (!omit_dir) {
                pos = engAppendBytes(out_buf, pos, _.direction);
                emitted_any = true;
            }
        }

        let sb: [128]u8 = [0; 128];
        let i: usize = 0;
        while (i < _.stop_count) {
            const st = _.stops[i];
            if (lower_double_stops && st.hasDoublePosition()) {
                if (emitted_any) pos = engAppendBytes(out_buf, pos, sep);
                pos = engAppendBytes(out_buf, pos, st.color);
                pos = engAppendBytes(out_buf, pos, " ");
                pos = engAppendBytes(out_buf, pos, st.pos1);
                pos = engAppendBytes(out_buf, pos, sep);
                pos = engAppendBytes(out_buf, pos, st.color);
                pos = engAppendBytes(out_buf, pos, " ");
                pos = engAppendBytes(out_buf, pos, st.pos2);
                emitted_any = true;
            } else {
                if (emitted_any) pos = engAppendBytes(out_buf, pos, sep);
                const s = st.format(sb[0..128]);
                pos = engAppendBytes(out_buf, pos, s);
                emitted_any = true;
            }
            i += 1;
        }
        pos = engAppendBytes(out_buf, pos, ")");
        return out_buf[0..pos];
    }
}
