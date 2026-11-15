// lib/css/length.cp
// CSS Values & Units Level 4: CssUnit, CssLength, and CssCalc Evaluator
// Absorbed and unified from Bun src/css/values/length.cp, angle.cp, time.cp, percentage.cp, calc.cp

const std = @import("std");

pub enum CssUnit {
    Px,
    Rem,
    Em,
    Vw,
    Vh,
    Vmin,
    Vmax,
    Percent,
    Pt,
    Cm,
    Mm,
    In,
    Deg,
    Rad,
    Turn,
    Sec,
    Ms,
    Unitless,

    pub static fn fromSuffix(sfx: []const u8): CssUnit? {
        if (sfx.len == 0) return CssUnit.Unitless;
        if (std.ascii.eqlIgnoreCase(sfx, "px")) return CssUnit.Px;
        if (std.ascii.eqlIgnoreCase(sfx, "rem")) return CssUnit.Rem;
        if (std.ascii.eqlIgnoreCase(sfx, "em")) return CssUnit.Em;
        if (std.ascii.eqlIgnoreCase(sfx, "vw")) return CssUnit.Vw;
        if (std.ascii.eqlIgnoreCase(sfx, "vh")) return CssUnit.Vh;
        if (std.ascii.eqlIgnoreCase(sfx, "vmin")) return CssUnit.Vmin;
        if (std.ascii.eqlIgnoreCase(sfx, "vmax")) return CssUnit.Vmax;
        if (std.mem.eql(u8, sfx, "%")) return CssUnit.Percent;
        if (std.ascii.eqlIgnoreCase(sfx, "pt")) return CssUnit.Pt;
        if (std.ascii.eqlIgnoreCase(sfx, "cm")) return CssUnit.Cm;
        if (std.ascii.eqlIgnoreCase(sfx, "mm")) return CssUnit.Mm;
        if (std.ascii.eqlIgnoreCase(sfx, "in")) return CssUnit.In;
        if (std.ascii.eqlIgnoreCase(sfx, "deg")) return CssUnit.Deg;
        if (std.ascii.eqlIgnoreCase(sfx, "rad")) return CssUnit.Rad;
        if (std.ascii.eqlIgnoreCase(sfx, "turn")) return CssUnit.Turn;
        if (std.ascii.eqlIgnoreCase(sfx, "ms")) return CssUnit.Ms;
        if (std.ascii.eqlIgnoreCase(sfx, "s")) return CssUnit.Sec;
        return null;
    }

    pub fn suffix(): []const u8 {
        return switch (_) {
            .Px => "px",
            .Rem => "rem",
            .Em => "em",
            .Vw => "vw",
            .Vh => "vh",
            .Vmin => "vmin",
            .Vmax => "vmax",
            .Percent => "%",
            .Pt => "pt",
            .Cm => "cm",
            .Mm => "mm",
            .In => "in",
            .Deg => "deg",
            .Rad => "rad",
            .Turn => "turn",
            .Sec => "s",
            .Ms => "ms",
            .Unitless => "",
        };
    }
}

pub struct CssLength {
    value: f64 = 0.0;
    unit: CssUnit = CssUnit.Px;

    pub static fn init(value: f64, unit: CssUnit): CssLength {
        return CssLength { value: value, unit: unit };
    }

    pub static fn px(value: f64): CssLength {
        return CssLength { value: value, unit: CssUnit.Px };
    }

    pub static fn rem(value: f64): CssLength {
        return CssLength { value: value, unit: CssUnit.Rem };
    }

    pub static fn parse(raw: []const u8): CssLength? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) {
            return null;
        }

        // Find split between numeric prefix and unit suffix
        let num_end: usize = 0;
        while (num_end < s.len) {
            const c = s[num_end];
            if ((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+') {
                num_end += 1;
            } else {
                break;
            }
        }
        if (num_end == 0) {
            return null;
        }

        const num_str = s[0..num_end];
        const unit_str = std.mem.trim(u8, s[num_end..], " \t");
        const val = std.fmt.parseFloat(f64, num_str) catch return null;
        if (unit_str.len == 0) {
            if (val == 0.0) {
                return CssLength { value: 0.0, unit: CssUnit.Px };
            }
            return CssLength { value: val, unit: CssUnit.Unitless };
        }
        const u = CssUnit.fromSuffix(unit_str) orelse return null;
        return CssLength { value: val, unit: u };
    }

    /// Resolves any CSS length unit to canonical pixels (`px`).
    pub fn toPx(root_font_px: f64, viewport_w: f64, viewport_h: f64): f64 {
        return switch (_.unit) {
            .Px, .Unitless => _.value,
            .Rem, .Em => _.value * root_font_px,
            .Vw => (_.value / 100.0) * viewport_w,
            .Vh => (_.value / 100.0) * viewport_h,
            .Vmin => (_.value / 100.0) * (if (viewport_w < viewport_h) viewport_w else viewport_h),
            .Vmax => (_.value / 100.0) * (if (viewport_w > viewport_h) viewport_w else viewport_h),
            .Percent => (_.value / 100.0) * viewport_w,
            .In => _.value * 96.0,
            .Pt => _.value * (96.0 / 72.0),
            .Cm => _.value * (96.0 / 2.54),
            .Mm => _.value * (96.0 / 25.4),
            else => _.value,
        };
    }

    /// Resolves `<time>` units (`s`, `ms`) to milliseconds.
    pub fn toMs(): f64 {
        return switch (_.unit) {
            .Sec => _.value * 1000.0,
            .Ms => _.value,
            else => _.value,
        };
    }

    /// Resolves `<angle>` units (`deg`, `rad`, `turn`) to degrees.
    pub fn toDeg(): f64 {
        return switch (_.unit) {
            .Deg => _.value,
            .Turn => _.value * 360.0,
            .Rad => _.value * (180.0 / std.math.pi),
            else => _.value,
        };
    }

    pub fn format(out_buf: []u8): []const u8 {
        if (_.value == 0.0 && _.unit == CssUnit.Px) {
            return std.fmt.bufPrint(out_buf, "0px", .{}) catch "";
        }
        const int_val: i64 = @intFromFloat(_.value);
        const int_f: f64 = @floatFromInt(int_val);
        if (int_f == _.value) {
            return std.fmt.bufPrint(out_buf, "{d}{s}", .{ int_val, _.unit.suffix() }) catch "";
        }
        return std.fmt.bufPrint(out_buf, "{d:.2}{s}", .{ _.value, _.unit.suffix() }) catch "";
    }
}

pub struct CssCalc {
    /// Evaluates a `calc(...)` expression (such as `calc(100vw - 2rem)` or `calc(16px + 2 * 1.5rem)`)
    /// into canonical pixels given `root_font_px`, `viewport_w`, and `viewport_h`.
    pub static fn evalPx(expr: []const u8, root_font_px: f64, viewport_w: f64, viewport_h: f64): f64? {
        let s = std.mem.trim(u8, expr, " \t\r\n");
        if (std.ascii.startsWithIgnoreCase(s, "calc(") && s[s.len - 1] == ')') {
            s = std.mem.trim(u8, s[5 .. s.len - 1], " \t\r\n");
        }
        if (s.len == 0) {
            return null;
        }

        // Two-pass precedence evaluation over space-separated tokens:
        // First resolve terms into values and operators (+, -, *, /)
        let values: [16]f64 = std.mem.zeroes([16]f64);
        let ops: [16]u8 = std.mem.zeroes([16]u8);
        let val_count: usize = 0;
        let op_count: usize = 0;

        let it = std.mem.tokenizeAny(u8, s, " \t\r\n");
        while (tok := it.next()) {
            if (tok.len == 1 && (tok[0] == '+' || tok[0] == '-' || tok[0] == '*' || tok[0] == '/')) {
                if (op_count >= 16) return null;
                ops[op_count] = tok[0];
                op_count += 1;
            } else {
                if (val_count >= 16) return null;
                const len_val = CssLength.parse(tok)?;
                values[val_count] = len_val.toPx(root_font_px, viewport_w, viewport_h);
                val_count += 1;
            }
        }

        if (val_count == 0 || op_count + 1 != val_count) {
            return null;
        }

        // Pass 1: handle '*' and '/'
        let collapsed_vals: [16]f64 = std.mem.zeroes([16]f64);
        let collapsed_ops: [16]u8 = std.mem.zeroes([16]u8);
        collapsed_vals[0] = values[0];
        let c_idx: usize = 0;

        let i: usize = 0;
        while (i < op_count) {
            const op = ops[i];
            const rhs = values[i + 1];
            if (op == '*') {
                collapsed_vals[c_idx] = collapsed_vals[c_idx] * rhs;
            } else if (op == '/') {
                if (rhs == 0.0) return null;
                collapsed_vals[c_idx] = collapsed_vals[c_idx] / rhs;
            } else {
                collapsed_ops[c_idx] = op;
                c_idx += 1;
                collapsed_vals[c_idx] = rhs;
            }
            i += 1;
        }

        // Pass 2: handle '+' and '-'
        let acc = collapsed_vals[0];
        let j: usize = 0;
        while (j < c_idx) {
            if (collapsed_ops[j] == '+') {
                acc += collapsed_vals[j + 1];
            } else {
                acc -= collapsed_vals[j + 1];
            }
            j += 1;
        }
        return acc;
    }
}
