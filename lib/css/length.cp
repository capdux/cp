// Source: Absorbed from /data/dev/bun/src/css/values/length.cp | Synced: 2026-11-20T16:13:49Z
// lib/css/length.cp
// CSS Values & Units Level 4: CssUnit, CssLength, and Recursive-Descent CssCalc Evaluator
// Absorbed from Bun src/css/values/{length,angle,time,percentage,calc}.cp

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
    Pc,
    Q,
    Ch,
    Ex,
    Deg,
    Rad,
    Turn,
    Grad,
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
        if (std.ascii.eqlIgnoreCase(sfx, "pc")) return CssUnit.Pc;
        if (std.ascii.eqlIgnoreCase(sfx, "q")) return CssUnit.Q;
        if (std.ascii.eqlIgnoreCase(sfx, "ch")) return CssUnit.Ch;
        if (std.ascii.eqlIgnoreCase(sfx, "ex")) return CssUnit.Ex;
        if (std.ascii.eqlIgnoreCase(sfx, "deg")) return CssUnit.Deg;
        if (std.ascii.eqlIgnoreCase(sfx, "rad")) return CssUnit.Rad;
        if (std.ascii.eqlIgnoreCase(sfx, "turn")) return CssUnit.Turn;
        if (std.ascii.eqlIgnoreCase(sfx, "grad")) return CssUnit.Grad;
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
            .Pc => "pc",
            .Q => "q",
            .Ch => "ch",
            .Ex => "ex",
            .Deg => "deg",
            .Rad => "rad",
            .Turn => "turn",
            .Grad => "grad",
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
        const u = CssUnit.fromSuffix(unit_str) ?? return null;
        return CssLength { value: val, unit: u };
    }

    pub fn eql(other: CssLength): bool {
        return _.value == other.value && _.unit == other.unit;
    }

    pub fn scale(factor: f64): CssLength {
        return CssLength { value: _.value * factor, unit: _.unit };
    }

    /// Resolves any CSS length unit to canonical pixels (`px`).
    pub fn toPx(root_font_px: f64, viewport_w: f64, viewport_h: f64): f64 {
        return switch (_.unit) {
            .Px, .Unitless => _.value,
            .Rem, .Em => _.value * root_font_px,
            .Ch, .Ex => _.value * root_font_px * 0.5,
            .Vw => (_.value / 100.0) * viewport_w,
            .Vh => (_.value / 100.0) * viewport_h,
            .Vmin => (_.value / 100.0) * (if (viewport_w < viewport_h) viewport_w else viewport_h),
            .Vmax => (_.value / 100.0) * (if (viewport_w > viewport_h) viewport_w else viewport_h),
            .Percent => (_.value / 100.0) * viewport_w,
            .In => _.value * 96.0,
            .Pt => _.value * (96.0 / 72.0),
            .Pc => _.value * 16.0,
            .Cm => _.value * (96.0 / 2.54),
            .Mm => _.value * (96.0 / 25.4),
            .Q => _.value * (96.0 / 101.6),
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

    /// Resolves `<angle>` units (`deg`, `rad`, `turn`, `grad`) to degrees.
    pub fn toDeg(): f64 {
        return switch (_.unit) {
            .Deg => _.value,
            .Turn => _.value * 360.0,
            .Rad => _.value * (180.0 / std.math.pi),
            .Grad => _.value * 0.9,
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

struct CalcParser {
    src: []const u8;
    pos: usize = 0;
    root_font_px: f64 = 16.0;
    viewport_w: f64 = 1920.0;
    viewport_h: f64 = 1080.0;
    depth: usize = 0;

    fn skipWs(*_) {
        while (_.pos < _.src.len && (_.src[_.pos] == ' ' || _.src[_.pos] == '\t' || _.src[_.pos] == '\r' || _.src[_.pos] == '\n')) {
            _.pos += 1;
        }
    }

    fn peek(*_): u8 {
        _.skipWs();
        if (_.pos >= _.src.len) return 0;
        return _.src[_.pos];
    }

    fn parseExpr(*_): f64? {
        let left = _.parseTerm()?;
        while (true) {
            const c = _.peek();
            if (c == '+') {
                _.pos += 1;
                const right = _.parseTerm()?;
                left = left + right;
            } else if (c == '-') {
                _.pos += 1;
                const right = _.parseTerm()?;
                left = left - right;
            } else {
                break;
            }
        }
        return left;
    }

    fn parseTerm(*_): f64? {
        let left = _.parseUnary()?;
        while (true) {
            const c = _.peek();
            if (c == '*') {
                _.pos += 1;
                const right = _.parseUnary()?;
                left = left * right;
            } else if (c == '/') {
                _.pos += 1;
                const right = _.parseUnary()?;
                if (right == 0.0) return null;
                left = left / right;
            } else {
                break;
            }
        }
        return left;
    }

    fn parseUnary(*_): f64? {
        const c = _.peek();
        if (c == '+') {
            _.pos += 1;
            return _.parseUnary();
        }
        if (c == '-') {
            _.pos += 1;
            const v = _.parseUnary()?;
            return -v;
        }
        return _.parsePrimary();
    }

    fn parsePrimary(*_): f64? {
        if (_.depth >= 32) return null;
        const c = _.peek();
        if (c == 0) return null;

        // Parenthesized expression
        if (c == '(') {
            _.pos += 1;
            _.depth += 1;
            const val = _.parseExpr();
            _.depth -= 1;
            const v = val?;
            if (_.peek() != ')') return null;
            _.pos += 1;
            return v;
        }

        // Math functions: calc(...), min(...), max(...), clamp(...)
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            const name_start = _.pos;
            while (_.pos < _.src.len && ((_.src[_.pos] >= 'a' && _.src[_.pos] <= 'z') || (_.src[_.pos] >= 'A' && _.src[_.pos] <= 'Z'))) {
                _.pos += 1;
            }
            const fn_name = _.src[name_start.._.pos];
            if (_.peek() != '(') return null;
            _.pos += 1;

            _.depth += 1;
            let args: [8]f64 = std.mem.zeroes([8]f64);
            let argc: usize = 0;
            while (argc < 8) {
                const v_opt = _.parseExpr();
                if (v_opt == null) {
                    _.depth -= 1;
                    return null;
                }
                args[argc] = v_opt.?;
                argc += 1;
                if (_.peek() == ',') {
                    _.pos += 1;
                    continue;
                }
                break;
            }
            _.depth -= 1;
            if (_.peek() != ')' || argc == 0) return null;
            _.pos += 1;

            if (std.ascii.eqlIgnoreCase(fn_name, "calc")) {
                return args[0];
            }
            if (std.ascii.eqlIgnoreCase(fn_name, "min")) {
                let best = args[0];
                let i: usize = 1;
                while (i < argc) {
                    if (args[i] < best) best = args[i];
                    i += 1;
                }
                return best;
            }
            if (std.ascii.eqlIgnoreCase(fn_name, "max")) {
                let best = args[0];
                let i: usize = 1;
                while (i < argc) {
                    if (args[i] > best) best = args[i];
                    i += 1;
                }
                return best;
            }
            if (std.ascii.eqlIgnoreCase(fn_name, "clamp") && argc == 3) {
                const lo = args[0];
                const val = args[1];
                const hi = args[2];
                const clamped = if (val < lo) lo else val;
                return if (clamped > hi) hi else clamped;
            }
            return null;
        }

        // Numeric literal + optional unit suffix
        const tok_start = _.pos;
        while (_.pos < _.src.len) {
            const ch = _.src[_.pos];
            if ((ch >= '0' && ch <= '9') || ch == '.' || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '%') {
                _.pos += 1;
            } else {
                break;
            }
        }
        if (_.pos == tok_start) return null;
        const len_val = CssLength.parse(_.src[tok_start.._.pos])?;
        return len_val.toPx(_.root_font_px, _.viewport_w, _.viewport_h);
    }
}

pub struct CssCalc {
    /// Evaluates CSS math expressions (`calc(...)`, `min(...)`, `max(...)`, `clamp(...)`, parentheses,
    /// unary signs, and `+`, `-`, `*`, `/`) into canonical pixels given `root_font_px`, `viewport_w`, and `viewport_h`.
    pub static fn evalPx(expr: []const u8, root_font_px: f64, viewport_w: f64, viewport_h: f64): f64? {
        const s = std.mem.trim(u8, expr, " \t\r\n");
        if (s.len == 0) {
            return null;
        }
        let parser = CalcParser {
            src: s,
            pos: 0,
            root_font_px: root_font_px,
            viewport_w: viewport_w,
            viewport_h: viewport_h,
        };
        const res = parser.parseExpr()?;
        parser.skipWs();
        if (parser.pos != parser.src.len) {
            return null;
        }
        return res;
    }
}
