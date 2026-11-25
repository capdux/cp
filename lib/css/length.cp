// Source: Absorbed from /data/dev/bun/src/css/values/length.cp | Synced: 2026-11-23T22:35:04Z
// lib/css/length.cp
// CSS Values & Units Level 4: CssUnit, CssLength, and Recursive-Descent CssCalc Evaluator
// Absorbed from Bun src/css/values/{length,angle,time,percentage,calc}.cp

const std = @import("std");
import { isValidCssMathSyntax } from "./mathFunc.cp";

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

const CSS_LEN_UNITS: [37][]const u8 = [
    "px", "em", "rem", "ex", "rex", "cap", "rcap", "ch", "rch", "ic", "ric", "lh", "rlh",
    "vw", "vh", "vi", "vb", "vmin", "vmax", "svw", "svh", "lvw", "lvh", "dvw", "dvh",
    "cqw", "cqh", "cqi", "cqb", "cqmin", "cqmax",
    "cm", "mm", "q", "in", "pt", "pc",
];

const CSS_NON_LEN_MATH_UNITS: [12][]const u8 = [
    "deg", "rad", "grad", "turn",
    "s", "ms", "hz", "khz", "dpi", "dpcm", "dppx", "fr",
];

fn isCssWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0c;
}

fn isCssIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0x80;
}

fn isCssIdentChar(c: u8): bool {
    return isCssIdentStart(c) || (c >= '0' && c <= '9') || c == '-';
}

fn isCssDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isCssLengthUnit(unit: []const u8): bool {
    for (u of CSS_LEN_UNITS) {
        if (std.ascii.eqlIgnoreCase(unit, u)) return true;
    }
    return false;
}

fn isCssNonLengthMathUnit(unit: []const u8): bool {
    for (u of CSS_NON_LEN_MATH_UNITS) {
        if (std.ascii.eqlIgnoreCase(unit, u)) return true;
    }
    return false;
}

fn consumeBalancedCallRest(s: []const u8, start: usize): bool {
    let i = start;
    let p_depth: usize = 0;
    let b_depth: usize = 0;
    let c_depth: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch == '(') {
            p_depth += 1;
        } else if (ch == ')') {
            if (p_depth == 0) {
                return b_depth == 0 && c_depth == 0 && i + 1 == s.len;
            }
            p_depth -= 1;
        } else if (ch == '[') {
            b_depth += 1;
        } else if (ch == ']') {
            if (b_depth == 0) return false;
            b_depth -= 1;
        } else if (ch == '{') {
            c_depth += 1;
        } else if (ch == '}') {
            if (c_depth == 0) return false;
            c_depth -= 1;
        }
        i += 1;
    }
    return false;
}

fn isValidVarOrEnvCall(s: []const u8): bool {
    if (s.len >= 5 && std.ascii.eqlIgnoreCase(s[0..4], "var(")) {
        let i: usize = 4;
        while (i < s.len && isCssWs(s[i])) i += 1;
        if (i + 2 >= s.len || s[i] != '-' || s[i + 1] != '-') return false;
        i += 2;
        const name_start = i;
        while (i < s.len && isCssIdentChar(s[i])) i += 1;
        if (i == name_start) return false;
        while (i < s.len && isCssWs(s[i])) i += 1;
        if (i >= s.len) return false;
        if (s[i] == ')') return i + 1 == s.len;
        if (s[i] != ',') return false;
        return consumeBalancedCallRest(s, i + 1);
    }
    if (s.len >= 5 && std.ascii.eqlIgnoreCase(s[0..4], "env(")) {
        let i: usize = 4;
        while (i < s.len && isCssWs(s[i])) i += 1;
        if (i >= s.len || (!isCssIdentStart(s[i]) && s[i] != '-')) return false;
        const name_start = i;
        while (i < s.len && isCssIdentChar(s[i])) i += 1;
        if (i == name_start) return false;
        while (i < s.len) {
            while (i < s.len && isCssWs(s[i])) i += 1;
            if (i >= s.len) return false;
            if (s[i] == ')') return i + 1 == s.len;
            if (s[i] == ',') return consumeBalancedCallRest(s, i + 1);
            if (isCssDigit(s[i])) {
                while (i < s.len && isCssDigit(s[i])) i += 1;
                continue;
            }
            return false;
        }
        return false;
    }
    return false;
}

fn skipBalancedParenGroup(s: []const u8, open_paren_pos: usize): usize {
    let i = open_paren_pos + 1;
    let depth: usize = 1;
    while (i < s.len && depth > 0) {
        if (s[i] == '(') depth += 1;
        else if (s[i] == ')') depth -= 1;
        i += 1;
    }
    return i;
}

fn checkMathUnitsAllowed(s: []const u8, allow_length_units: bool): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (isCssIdentStart(c) || (c == '-' && i + 1 < s.len && (isCssIdentStart(s[i + 1]) || s[i + 1] == '-'))) {
            const start = i;
            i += 1;
            while (i < s.len && isCssIdentChar(s[i])) i += 1;
            const ident = s[start..i];
            if (i < s.len && s[i] == '(' && (std.ascii.eqlIgnoreCase(ident, "var") || std.ascii.eqlIgnoreCase(ident, "env"))) {
                i = skipBalancedParenGroup(s, i);
            }
            continue;
        }
        if (isCssDigit(c) || (c == '.' && i + 1 < s.len && isCssDigit(s[i + 1]))) {
            while (i < s.len && isCssDigit(s[i])) i += 1;
            if (i < s.len && s[i] == '.') {
                i += 1;
                while (i < s.len && isCssDigit(s[i])) i += 1;
            }
            if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
                let exp_i = i + 1;
                if (exp_i < s.len && (s[exp_i] == '+' || s[exp_i] == '-')) exp_i += 1;
                if (exp_i < s.len && isCssDigit(s[exp_i])) {
                    i = exp_i;
                    while (i < s.len && isCssDigit(s[i])) i += 1;
                }
            }
            if (i < s.len && s[i] == '%') {
                i += 1;
                continue;
            }
            if (i < s.len && isCssIdentStart(s[i])) {
                const u_start = i;
                while (i < s.len && (isCssIdentStart(s[i]) || isCssDigit(s[i]))) i += 1;
                const unit = s[u_start..i];
                if (!allow_length_units) return false;
                if (isCssNonLengthMathUnit(unit)) return false;
            }
            continue;
        }
        i += 1;
    }
    return true;
}

fn scanOpacityMinMaxClampArg(arg: []const u8): bool {
    // Recursively validate any nested min/max/clamp calls in arg
    return checkOpacityMinMaxClampNoMixedPct(arg);
}

fn checkOpacityMinMaxClampNoMixedPct(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (isCssIdentStart(c) || (c == '-' && i + 1 < s.len && (isCssIdentStart(s[i + 1]) || s[i + 1] == '-'))) {
            const start = i;
            i += 1;
            while (i < s.len && isCssIdentChar(s[i])) i += 1;
            const ident = s[start..i];
            if (i < s.len && s[i] == '(') {
                if (std.ascii.eqlIgnoreCase(ident, "var") || std.ascii.eqlIgnoreCase(ident, "env")) {
                    i = skipBalancedParenGroup(s, i);
                    continue;
                }
                if (std.ascii.eqlIgnoreCase(ident, "min") || std.ascii.eqlIgnoreCase(ident, "max") || std.ascii.eqlIgnoreCase(ident, "clamp")) {
                    const call_end = skipBalancedParenGroup(s, i);
                    const inner = s[i + 1 .. call_end - 1];
                    let seen_pct = false;
                    let seen_num = false;
                    let arg_start: usize = 0;
                    let depth: usize = 0;
                    let j: usize = 0;
                    while (j <= inner.len) {
                        const at_sep = (j == inner.len) || (inner[j] == ',' && depth == 0);
                        if (at_sep) {
                            const arg = inner[arg_start..j];
                            if (!scanOpacityMinMaxClampArg(arg)) return false;
                            let arg_has_pct = false;
                            let arg_has_num = false;
                            let k: usize = 0;
                            while (k < arg.len) {
                                const ac = arg[k];
                                if (isCssIdentStart(ac) || (ac == '-' && k + 1 < arg.len && (isCssIdentStart(arg[k + 1]) || arg[k + 1] == '-'))) {
                                    const id_s = k;
                                    k += 1;
                                    while (k < arg.len && isCssIdentChar(arg[k])) k += 1;
                                    const sub_id = arg[id_s..k];
                                    if (k < arg.len && arg[k] == '(' && (std.ascii.eqlIgnoreCase(sub_id, "var") || std.ascii.eqlIgnoreCase(sub_id, "env"))) {
                                        k = skipBalancedParenGroup(arg, k);
                                    } else if (std.ascii.eqlIgnoreCase(sub_id, "pi") || std.ascii.eqlIgnoreCase(sub_id, "e") || std.ascii.eqlIgnoreCase(sub_id, "infinity") || std.ascii.eqlIgnoreCase(sub_id, "-infinity") || std.ascii.eqlIgnoreCase(sub_id, "nan")) {
                                        arg_has_num = true;
                                    }
                                    continue;
                                }
                                if (ac == '%') {
                                    arg_has_pct = true;
                                    k += 1;
                                    continue;
                                }
                                if (isCssDigit(ac) || (ac == '.' && k + 1 < arg.len && isCssDigit(arg[k + 1]))) {
                                    arg_has_num = true;
                                    while (k < arg.len && isCssDigit(arg[k])) k += 1;
                                    if (k < arg.len && arg[k] == '.') {
                                        k += 1;
                                        while (k < arg.len && isCssDigit(arg[k])) k += 1;
                                    }
                                    if (k < arg.len && (arg[k] == 'e' || arg[k] == 'E')) {
                                        let exp_k = k + 1;
                                        if (exp_k < arg.len && (arg[exp_k] == '+' || arg[exp_k] == '-')) exp_k += 1;
                                        if (exp_k < arg.len && isCssDigit(arg[exp_k])) {
                                            k = exp_k;
                                            while (k < arg.len && isCssDigit(arg[k])) k += 1;
                                        }
                                    }
                                    continue;
                                }
                                k += 1;
                            }
                            if (arg_has_pct) {
                                seen_pct = true;
                            } else if (arg_has_num) {
                                seen_num = true;
                            }
                            if (seen_pct && seen_num) return false;
                            arg_start = j + 1;
                        } else if (inner[j] == '(') {
                            depth += 1;
                        } else if (inner[j] == ')') {
                            if (depth > 0) depth -= 1;
                        }
                        j += 1;
                    }
                    i = call_end;
                    continue;
                }
            }
            continue;
        }
        i += 1;
    }
    return true;
}

/// Zero-allocation validator for CSS `<length-percentage>` tokens.
pub fn isValidCssLenPct(tok: []const u8, allow_negative: bool): bool {
    const s = std.mem.trim(u8, tok, " \t\r\n\x0c");
    if (s.len == 0) return false;

    if (isValidVarOrEnvCall(s)) return true;

    if (isValidCssMathSyntax(s)) {
        return checkMathUnitsAllowed(s, true);
    }

    let i: usize = 0;
    let is_neg = false;
    if (s[i] == '+' || s[i] == '-') {
        is_neg = (s[i] == '-');
        i += 1;
    }
    if (i >= s.len) return false;

    let has_digit = false;
    let has_nonzero = false;
    while (i < s.len && isCssDigit(s[i])) {
        has_digit = true;
        if (s[i] != '0') has_nonzero = true;
        i += 1;
    }
    if (i < s.len && s[i] == '.') {
        i += 1;
        if (i >= s.len || !isCssDigit(s[i])) return false;
        while (i < s.len && isCssDigit(s[i])) {
            has_digit = true;
            if (s[i] != '0') has_nonzero = true;
            i += 1;
        }
    }
    if (!has_digit) return false;

    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        let exp_i = i + 1;
        if (exp_i < s.len && (s[exp_i] == '+' || s[exp_i] == '-')) exp_i += 1;
        if (exp_i < s.len && isCssDigit(s[exp_i])) {
            i = exp_i;
            while (i < s.len && isCssDigit(s[i])) i += 1;
        }
    }

    if (!allow_negative && is_neg && has_nonzero) return false;

    const unit = s[i..];
    if (unit.len == 0) {
        return !has_nonzero;
    }
    if (std.mem.eql(u8, unit, "%")) return true;
    return isCssLengthUnit(unit);
}

/// Zero-allocation validator for CSS `translate` property values (`none` or 1..3 `<length-percentage>` / z `<length>`).
pub fn isValidCssTranslateValue(val: []const u8): bool {
    const s = std.mem.trim(u8, val, " \t\r\n\x0c");
    if (s.len == 0) return false;
    if (std.ascii.eqlIgnoreCase(s, "none")) return true;

    let i: usize = 0;
    let tok_count: usize = 0;
    while (i < s.len) {
        while (i < s.len && isCssWs(s[i])) i += 1;
        if (i >= s.len) break;

        const tok_start = i;
        let p_depth: usize = 0;
        let b_depth: usize = 0;
        let c_depth: usize = 0;
        while (i < s.len) {
            const ch = s[i];
            if (p_depth == 0 && b_depth == 0 && c_depth == 0 && isCssWs(ch)) {
                break;
            }
            if (ch == '(') p_depth += 1;
            else if (ch == ')') {
                if (p_depth == 0) return false;
                p_depth -= 1;
            } else if (ch == '[') b_depth += 1;
            else if (ch == ']') {
                if (b_depth == 0) return false;
                b_depth -= 1;
            } else if (ch == '{') c_depth += 1;
            else if (ch == '}') {
                if (c_depth == 0) return false;
                c_depth -= 1;
            }
            i += 1;
        }
        if (p_depth != 0 || b_depth != 0 || c_depth != 0) return false;

        const tok = s[tok_start..i];
        tok_count += 1;
        if (tok_count > 3) return false;
        if (tok_count == 3 && std.mem.indexOfScalar(u8, tok, '%') != null) {
            return false;
        }
        if (!isValidCssLenPct(tok, true)) return false;
    }

    return tok_count >= 1 && tok_count <= 3;
}

/// Zero-allocation validator for CSS `opacity` property values.
fn hasSubstrIgnoreCase(hay: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (hay.len < needle.len) return false;
    let i: usize = 0;
    while (i + needle.len <= hay.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(hay[i .. i + needle.len], needle)) return true;
    }
    return false;
}

pub fn isValidCssOpacityValue(val: []const u8): bool {
    const s = std.mem.trim(u8, val, " \t\r\n\x0c");
    if (s.len == 0) return false;

    if (isValidVarOrEnvCall(s)) return true;

    if (isValidCssMathSyntax(s)) {
        if (!checkMathUnitsAllowed(s, false)) return false;
        if (std.mem.indexOfScalar(u8, s, '%') != null) {
            if (hasSubstrIgnoreCase(s, "log(") || hasSubstrIgnoreCase(s, "exp(") ||
                hasSubstrIgnoreCase(s, "pow(") || hasSubstrIgnoreCase(s, "sqrt(") ||
                hasSubstrIgnoreCase(s, "sin(") || hasSubstrIgnoreCase(s, "cos(") ||
                hasSubstrIgnoreCase(s, "tan(") || hasSubstrIgnoreCase(s, "asin(") ||
                hasSubstrIgnoreCase(s, "acos(") || hasSubstrIgnoreCase(s, "atan(") ||
                hasSubstrIgnoreCase(s, "atan2(")) {
                return false;
            }
        }
        return checkOpacityMinMaxClampNoMixedPct(s);
    }

    let i: usize = 0;
    if (s[i] == '+' || s[i] == '-') i += 1;
    if (i >= s.len) return false;

    let has_digit = false;
    while (i < s.len && isCssDigit(s[i])) {
        has_digit = true;
        i += 1;
    }
    if (i < s.len && s[i] == '.') {
        i += 1;
        if (i >= s.len || !isCssDigit(s[i])) return false;
        while (i < s.len && isCssDigit(s[i])) {
            has_digit = true;
            i += 1;
        }
    }
    if (!has_digit) return false;

    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        let exp_i = i + 1;
        if (exp_i < s.len && (s[exp_i] == '+' || s[exp_i] == '-')) exp_i += 1;
        if (exp_i < s.len && isCssDigit(s[exp_i])) {
            i = exp_i;
            while (i < s.len && isCssDigit(s[i])) i += 1;
        }
    }

    const suffix = s[i..];
    return suffix.len == 0 || std.mem.eql(u8, suffix, "%");
}
