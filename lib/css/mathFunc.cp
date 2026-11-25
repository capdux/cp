// Source: Absorbed from /data/dev/bun/src/css/values/calc.cp | Synced: 2026-11-23T22:59:33Z
// Pure-CP zero-allocation CSS Values & Units Level 4 Math Functions & Constants evaluator.
// Supports all 21 W3C CSS Math functions:
//   Basic & comparison: calc(), min(), max(), clamp()
//   Stepped value:      round([nearest|up|down|to-zero], a, b), mod(a, b), rem(a, b)
//   Sign-related:       abs(x), sign(x)
//   Trigonometric:      sin(), cos(), tan(), asin(), acos(), atan(), atan2(y, x)
//   Exponential:        pow(a, b), sqrt(x), hypot(a, b, ...), log(x [, base]), exp(x)
// Plus mathematical constants (pi, e, infinity, -infinity, NaN) and angle units (deg/rad/grad/turn).

const std = @import("std");

/// 递归下降求值最大嵌套深度上限（防止畸形超深表达式栈溢出）
pub const CSS_MATH_MAX_DEPTH: usize = 32;
/// 单个多参数学函数（`min`/`max`/`hypot`）支持的最大参数个数
pub const CSS_MATH_MAX_ARGS: usize = 16;
/// W3C CSS 数学常量 `pi`
pub const CSS_MATH_PI: f64 = 3.14159265358979323846;
/// W3C CSS 数学常量 `e`
pub const CSS_MATH_E: f64 = 2.71828182845904523536;

/// CSS Math Level 4 求值与解析错误集
pub err CssMathFuncError {
    EmptyExpression,
    UnexpectedToken,
    UnknownFunction,
    UnknownConstant,
    InvalidArity,
    InvalidRoundingStrategy,
    UnclosedParen,
    TrailingTokens,
    DivisionByZero,
    MaxDepthExceeded,
    DomainError,
}

/// W3C CSS `round()` 取整策略（对齐 Bun `calc.RoundingStrategy`）
pub enum CssMathRoundingStrategy: u8 {
    Nearest = 0,
    Up = 1,
    Down = 2,
    ToZero = 3,

    /// 按 ASCII 大小写无关解析取整策略关键字
    static fn parse(s: []const u8): CssMathRoundingStrategy? {
        if (eqlAsciiIgnoreCase(s, "nearest")) return .Nearest;
        if (eqlAsciiIgnoreCase(s, "up")) return .Up;
        if (eqlAsciiIgnoreCase(s, "down")) return .Down;
        if (eqlAsciiIgnoreCase(s, "to-zero")) return .ToZero;
        return null;
    }

    /// 返回规范 CSS 关键字字符串
    fn asStr(): []const u8 {
        return switch (_) {
            .Nearest => "nearest",
            .Up => "up",
            .Down => "down",
            .ToZero => "to-zero",
        };
    }
}

/// W3C CSS 数学常量（对齐 Bun `calc.Constant`）
pub enum CssMathConstant: u8 {
    E = 0,
    Pi = 1,
    Infinity = 2,
    NegInfinity = 3,
    NaN = 4,

    /// 按 ASCII 大小写无关解析 CSS 数学常量关键字
    static fn parse(s: []const u8): CssMathConstant? {
        if (eqlAsciiIgnoreCase(s, "e")) return .E;
        if (eqlAsciiIgnoreCase(s, "pi")) return .Pi;
        if (eqlAsciiIgnoreCase(s, "infinity")) return .Infinity;
        if (eqlAsciiIgnoreCase(s, "-infinity")) return .NegInfinity;
        if (eqlAsciiIgnoreCase(s, "nan")) return .NaN;
        return null;
    }

    /// 返回规范 CSS 常量字符串
    fn asStr(): []const u8 {
        return switch (_) {
            .E => "e",
            .Pi => "pi",
            .Infinity => "infinity",
            .NegInfinity => "-infinity",
            .NaN => "NaN",
        };
    }

    /// 转换为 IEEE-754 f64 数值
    fn toF64(): f64 {
        return switch (_) {
            .E => CSS_MATH_E,
            .Pi => CSS_MATH_PI,
            .Infinity => std.math.inf(f64),
            .NegInfinity => -std.math.inf(f64),
            .NaN => std.math.nan(f64),
        };
    }
}

/// W3C CSS Values & Units Level 4 全部 21 种数学函数标识（对齐 Bun `CalcUnit`）
pub enum CssMathFuncKind: u8 {
    Calc = 0,
    Min = 1,
    Max = 2,
    Clamp = 3,
    Round = 4,
    Mod = 5,
    Rem = 6,
    Abs = 7,
    Sign = 8,
    Sin = 9,
    Cos = 10,
    Tan = 11,
    Asin = 12,
    Acos = 13,
    Atan = 14,
    Atan2 = 15,
    Pow = 16,
    Sqrt = 17,
    Hypot = 18,
    Log = 19,
    Exp = 20,

    /// 按 ASCII 大小写无关识别函数名
    static fn parse(raw: []const u8): CssMathFuncKind? {
        if (eqlAsciiIgnoreCase(raw, "calc")) return .Calc;
        if (eqlAsciiIgnoreCase(raw, "min")) return .Min;
        if (eqlAsciiIgnoreCase(raw, "max")) return .Max;
        if (eqlAsciiIgnoreCase(raw, "clamp")) return .Clamp;
        if (eqlAsciiIgnoreCase(raw, "round")) return .Round;
        if (eqlAsciiIgnoreCase(raw, "mod")) return .Mod;
        if (eqlAsciiIgnoreCase(raw, "rem")) return .Rem;
        if (eqlAsciiIgnoreCase(raw, "abs")) return .Abs;
        if (eqlAsciiIgnoreCase(raw, "sign")) return .Sign;
        if (eqlAsciiIgnoreCase(raw, "sin")) return .Sin;
        if (eqlAsciiIgnoreCase(raw, "cos")) return .Cos;
        if (eqlAsciiIgnoreCase(raw, "tan")) return .Tan;
        if (eqlAsciiIgnoreCase(raw, "asin")) return .Asin;
        if (eqlAsciiIgnoreCase(raw, "acos")) return .Acos;
        if (eqlAsciiIgnoreCase(raw, "atan")) return .Atan;
        if (eqlAsciiIgnoreCase(raw, "atan2")) return .Atan2;
        if (eqlAsciiIgnoreCase(raw, "pow")) return .Pow;
        if (eqlAsciiIgnoreCase(raw, "sqrt")) return .Sqrt;
        if (eqlAsciiIgnoreCase(raw, "hypot")) return .Hypot;
        if (eqlAsciiIgnoreCase(raw, "log")) return .Log;
        if (eqlAsciiIgnoreCase(raw, "exp")) return .Exp;
        return null;
    }

    /// 返回规范小写函数名
    fn asStr(): []const u8 {
        return switch (_) {
            .Calc => "calc",
            .Min => "min",
            .Max => "max",
            .Clamp => "clamp",
            .Round => "round",
            .Mod => "mod",
            .Rem => "rem",
            .Abs => "abs",
            .Sign => "sign",
            .Sin => "sin",
            .Cos => "cos",
            .Tan => "tan",
            .Asin => "asin",
            .Acos => "acos",
            .Atan => "atan",
            .Atan2 => "atan2",
            .Pow => "pow",
            .Sqrt => "sqrt",
            .Hypot => "hypot",
            .Log => "log",
            .Exp => "exp",
        };
    }

    /// 是否为三角/反三角函数
    fn isTrig(): bool {
        return switch (_) {
            .Sin, .Cos, .Tan, .Asin, .Acos, .Atan, .Atan2 => true,
            else => false,
        };
    }

    /// 是否为步进取整函数（`round`/`mod`/`rem`）
    fn isStepped(): bool {
        return switch (_) {
            .Round, .Mod, .Rem => true,
            else => false,
        };
    }

    /// 是否为指数/对数函数（`pow`/`sqrt`/`hypot`/`log`/`exp`）
    fn isExponential(): bool {
        return switch (_) {
            .Pow, .Sqrt, .Hypot, .Log, .Exp => true,
            else => false,
        };
    }
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
    }
    return true;
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isIdentChar(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9') || c == '-';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

/// W3C CSS `round(strategy, value, step)` 求值
pub fn cssMathRound(strategy: CssMathRoundingStrategy, value: f64, step: f64): f64! {
    if (step == 0.0) {
        return err.DivisionByZero;
    }
    const q = value / step;
    const rounded = switch (strategy) {
        .Nearest => std.math.round(q),
        .Up => std.math.ceil(q),
        .Down => std.math.floor(q),
        .ToZero => std.math.trunc(q),
    };
    return rounded * step;
}

/// W3C CSS `mod(dividend, divisor)`：结果与 `divisor` 同号
pub fn cssMathMod(dividend: f64, divisor: f64): f64! {
    if (divisor == 0.0) {
        return err.DivisionByZero;
    }
    const r = dividend - std.math.floor(dividend / divisor) * divisor;
    return if (r == 0.0) 0.0 else r;
}

/// W3C CSS `rem(dividend, divisor)`：结果与 `dividend` 同号
pub fn cssMathRem(dividend: f64, divisor: f64): f64! {
    if (divisor == 0.0) {
        return err.DivisionByZero;
    }
    const r = dividend - std.math.trunc(dividend / divisor) * divisor;
    return if (r == 0.0) 0.0 else r;
}

/// W3C CSS `sign(x)`：返回 `-1`、`0` 或 `1`（NaN 透传 NaN）
pub fn cssMathSign(x: f64): f64 {
    if (std.math.isNan(x)) return x;
    if (x > 0.0) return 1.0;
    if (x < 0.0) return -1.0;
    return 0.0;
}

/// W3C CSS `clamp(min, val, max)`：严格等价于 `max(min, min(val, max))`
/// 当 `min > max` 时规范要求 `min` 胜出。
pub fn cssMathClamp(min_v: f64, val: f64, max_v: f64): f64 {
    const inner = if (val < max_v) val else max_v;
    return if (min_v > inner) min_v else inner;
}

/// W3C CSS `hypot(a, b, ...)`：多维欧几里得范数 `sqrt(sum(v_i^2))`
pub fn cssMathHypot(values: []const f64): f64! {
    if (values.len == 0) {
        return err.InvalidArity;
    }
    let sum_sq: f64 = 0.0;
    for (i of 0..values.len) {
        sum_sq += values[i] * values[i];
    }
    return std.math.sqrt(sum_sq);
}

/// W3C CSS `log(x [, base])`：缺省 base 为自然对数 `ln(x)`
pub fn cssMathLog(x: f64, base: f64?): f64! {
    if (x <= 0.0) {
        return err.DomainError;
    }
    const ln_x = std.math.log(f64, CSS_MATH_E, x);
    if (b := base) {
        if (b <= 0.0 || b == 1.0) {
            return err.DomainError;
        }
        return ln_x / std.math.log(f64, CSS_MATH_E, b);
    }
    return ln_x;
}

/// 将 CSS `<angle>` 单位（`deg`/`rad`/`grad`/`turn`）转换为弧度
pub fn cssMathAngleToRad(value: f64, unit: []const u8): f64! {
    if (unit.len == 0 || eqlAsciiIgnoreCase(unit, "rad")) {
        return value;
    }
    if (eqlAsciiIgnoreCase(unit, "deg")) {
        return value * (CSS_MATH_PI / 180.0);
    }
    if (eqlAsciiIgnoreCase(unit, "grad")) {
        return value * (CSS_MATH_PI / 200.0);
    }
    if (eqlAsciiIgnoreCase(unit, "turn")) {
        return value * (2.0 * CSS_MATH_PI);
    }
    return err.UnexpectedToken;
}

struct MathExprParser {
    src: []const u8,
    pos: usize = 0,
    depth: usize = 0,

    fn skipWs(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn peek(): u8? {
        if (_.pos >= _.src.len) return null;
        return _.src[_.pos];
    }

    fn parseSum(*_): f64! {
        let acc = _.parseProduct()?;
        while (true) {
            _.skipWs();
            const ch = _.peek() ?? break;
            if (ch == '+') {
                _.pos += 1;
                acc += _.parseProduct()?;
            } else if (ch == '-') {
                _.pos += 1;
                acc -= _.parseProduct()?;
            } else {
                break;
            }
        }
        return acc;
    }

    fn parseProduct(*_): f64! {
        let acc = _.parseUnary()?;
        while (true) {
            _.skipWs();
            const ch = _.peek() ?? break;
            if (ch == '*') {
                _.pos += 1;
                acc *= _.parseUnary()?;
            } else if (ch == '/') {
                _.pos += 1;
                const rhs = _.parseUnary()?;
                if (rhs == 0.0) {
                    return err.DivisionByZero;
                }
                acc /= rhs;
            } else {
                break;
            }
        }
        return acc;
    }

    fn parseUnary(*_): f64! {
        _.skipWs();
        const ch = _.peek() ?? return err.EmptyExpression;
        if (ch == '+') {
            _.pos += 1;
            return _.parseUnary();
        }
        if (ch == '-') {
            // 特殊识别 `-infinity` 常量
            if (_.pos + 9 <= _.src.len && eqlAsciiIgnoreCase(_.src[_.pos.._.pos + 9], "-infinity")) {
                const after = _.pos + 9;
                if (after >= _.src.len || !isIdentChar(_.src[after])) {
                    _.pos = after;
                    return -std.math.inf(f64);
                }
            }
            _.pos += 1;
            return -(_.parseUnary()?);
        }
        return _.parsePrimary();
    }

    fn parseNumber(*_): f64! {
        const start = _.pos;
        let has_digit = false;
        while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
            has_digit = true;
            _.pos += 1;
        }
        if (_.pos < _.src.len && _.src[_.pos] == '.') {
            _.pos += 1;
            while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                has_digit = true;
                _.pos += 1;
            }
        }
        if (!has_digit) {
            return err.UnexpectedToken;
        }
        // 可选科学计数法指数 e/E[+-]digits（避免误吞单位如 1em）
        if (_.pos < _.src.len && (_.src[_.pos] == 'e' || _.src[_.pos] == 'E')) {
            let exp_pos = _.pos + 1;
            if (exp_pos < _.src.len && (_.src[exp_pos] == '+' || _.src[exp_pos] == '-')) {
                exp_pos += 1;
            }
            if (exp_pos < _.src.len && isDigit(_.src[exp_pos])) {
                _.pos = exp_pos;
                while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                    _.pos += 1;
                }
            }
        }
        const num_slice = _.src[start.._.pos];
        const parsed = std.fmt.parseFloat(f64, num_slice) catch return err.UnexpectedToken;

        // 可选角度单位后缀（deg/rad/grad/turn），直接转换为弧度
        const unit_start = _.pos;
        while (_.pos < _.src.len && isIdentStart(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos > unit_start) {
            return cssMathAngleToRad(parsed, _.src[unit_start.._.pos]);
        }
        return parsed;
    }

    fn parsePrimary(*_): f64! {
        _.skipWs();
        if (_.depth >= CSS_MATH_MAX_DEPTH) {
            return err.MaxDepthExceeded;
        }
        const ch = _.peek() ?? return err.EmptyExpression;

        if (ch == '(') {
            _.pos += 1;
            _.depth += 1;
            const inner = _.parseSum()?;
            _.depth -= 1;
            _.skipWs();
            if ((_.peek() ?? 0) != ')') {
                return err.UnclosedParen;
            }
            _.pos += 1;
            return inner;
        }

        if (isDigit(ch) || ch == '.') {
            return _.parseNumber();
        }

        if (isIdentStart(ch)) {
            const start = _.pos;
            while (_.pos < _.src.len && isIdentChar(_.src[_.pos])) {
                _.pos += 1;
            }
            const ident = _.src[start.._.pos];
            if ((_.peek() ?? 0) == '(') {
                const kind = CssMathFuncKind.parse(ident) ?? return err.UnknownFunction;
                _.pos += 1;
                _.depth += 1;
                const val = _.parseFunctionCall(kind)?;
                _.depth -= 1;
                return val;
            }
            const constant = CssMathConstant.parse(ident) ?? return err.UnknownConstant;
            return constant.toF64();
        }

        return err.UnexpectedToken;
    }

    fn parseArgList(*_, args: []f64): usize! {
        let count: usize = 0;
        _.skipWs();
        if ((_.peek() ?? 0) == ')') {
            _.pos += 1;
            return 0;
        }
        while (true) {
            if (count >= args.len) {
                return err.InvalidArity;
            }
            args[count] = _.parseSum()?;
            count += 1;
            _.skipWs();
            const ch = _.peek() ?? return err.UnclosedParen;
            if (ch == ',') {
                _.pos += 1;
                continue;
            }
            if (ch == ')') {
                _.pos += 1;
                return count;
            }
            return err.UnclosedParen;
        }
    }

    fn parseRoundCall(*_): f64! {
        _.skipWs();
        let strategy = CssMathRoundingStrategy.Nearest;
        const saved = _.pos;
        if (_.pos < _.src.len && isIdentStart(_.src[_.pos])) {
            while (_.pos < _.src.len && isIdentChar(_.src[_.pos])) {
                _.pos += 1;
            }
            const ident = _.src[saved.._.pos];
            _.skipWs();
            if ((_.peek() ?? 0) == ',') {
                if (st := CssMathRoundingStrategy.parse(ident)) {
                    strategy = st;
                    _.pos += 1;
                } else {
                    // 可能是数学常量（如 `round(pi, 1)`）而非策略名，回退重解析
                    if (CssMathConstant.parse(ident) != null) {
                        _.pos = saved;
                    } else {
                        return err.InvalidRoundingStrategy;
                    }
                }
            } else {
                _.pos = saved;
            }
        }
        let args: [4]f64 = [0.0; 4];
        const argc = _.parseArgList(args[0..])?;
        if (argc == 1) {
            return cssMathRound(strategy, args[0], 1.0);
        }
        if (argc == 2) {
            return cssMathRound(strategy, args[0], args[1]);
        }
        return err.InvalidArity;
    }

    fn parseFunctionCall(*_, kind: CssMathFuncKind): f64! {
        if (kind == .Round) {
            return _.parseRoundCall();
        }
        let args: [CSS_MATH_MAX_ARGS]f64 = [0.0; CSS_MATH_MAX_ARGS];
        const argc = _.parseArgList(args[0..])?;

        switch (kind) {
            .Calc => {
                if (argc != 1) return err.InvalidArity;
                return args[0];
            },
            .Min => {
                if (argc < 1) return err.InvalidArity;
                let best = args[0];
                for (i of 1..argc) {
                    if (args[i] < best) best = args[i];
                }
                return best;
            },
            .Max => {
                if (argc < 1) return err.InvalidArity;
                let best = args[0];
                for (i of 1..argc) {
                    if (args[i] > best) best = args[i];
                }
                return best;
            },
            .Clamp => {
                if (argc != 3) return err.InvalidArity;
                return cssMathClamp(args[0], args[1], args[2]);
            },
            .Mod => {
                if (argc != 2) return err.InvalidArity;
                return cssMathMod(args[0], args[1]);
            },
            .Rem => {
                if (argc != 2) return err.InvalidArity;
                return cssMathRem(args[0], args[1]);
            },
            .Abs => {
                if (argc != 1) return err.InvalidArity;
                const v = args[0];
                return if (v < 0.0) -v else v;
            },
            .Sign => {
                if (argc != 1) return err.InvalidArity;
                return cssMathSign(args[0]);
            },
            .Sin => {
                if (argc != 1) return err.InvalidArity;
                return std.math.sin(args[0]);
            },
            .Cos => {
                if (argc != 1) return err.InvalidArity;
                return std.math.cos(args[0]);
            },
            .Tan => {
                if (argc != 1) return err.InvalidArity;
                return std.math.tan(args[0]);
            },
            .Asin => {
                if (argc != 1) return err.InvalidArity;
                if (args[0] < -1.0 || args[0] > 1.0) return err.DomainError;
                return std.math.asin(args[0]);
            },
            .Acos => {
                if (argc != 1) return err.InvalidArity;
                if (args[0] < -1.0 || args[0] > 1.0) return err.DomainError;
                return std.math.acos(args[0]);
            },
            .Atan => {
                if (argc != 1) return err.InvalidArity;
                return std.math.atan(args[0]);
            },
            .Atan2 => {
                if (argc != 2) return err.InvalidArity;
                return std.math.atan2(args[0], args[1]);
            },
            .Pow => {
                if (argc != 2) return err.InvalidArity;
                return std.math.pow(f64, args[0], args[1]);
            },
            .Sqrt => {
                if (argc != 1) return err.InvalidArity;
                if (args[0] < 0.0) return err.DomainError;
                return std.math.sqrt(args[0]);
            },
            .Hypot => {
                if (argc < 1) return err.InvalidArity;
                return cssMathHypot(args[0..argc]);
            },
            .Log => {
                if (argc == 1) return cssMathLog(args[0], null);
                if (argc == 2) return cssMathLog(args[0], args[1]);
                return err.InvalidArity;
            },
            .Exp => {
                if (argc != 1) return err.InvalidArity;
                return std.math.exp(args[0]);
            },
            .Round => return err.InvalidArity,
        }
    }
}

/// 零堆分配求值 W3C CSS Values & Units Level 4 数学表达式
/// 支持全部 21 种函数、嵌套四则运算、数学常量（`pi`/`e`/`infinity`/`-infinity`/`NaN`）与角度单位（`deg`/`rad`/`grad`/`turn`）。
pub fn evalCssMathExpr(expr: []const u8): f64! {
    let parser = MathExprParser{ src: expr };
    parser.skipWs();
    if (parser.pos >= parser.src.len) {
        return err.EmptyExpression;
    }
    const result = parser.parseSum()?;
    parser.skipWs();
    if (parser.pos < parser.src.len) {
        return err.TrailingTokens;
    }
    return result;
}

const CSS_MATH_UNITS: [49][]const u8 = [
    "px", "em", "rem", "ex", "rex", "cap", "rcap", "ch", "rch", "ic", "ric", "lh", "rlh",
    "vw", "vh", "vi", "vb", "vmin", "vmax", "svw", "svh", "lvw", "lvh", "dvw", "dvh",
    "cqw", "cqh", "cqi", "cqb", "cqmin", "cqmax",
    "cm", "mm", "q", "in", "pt", "pc",
    "deg", "rad", "grad", "turn",
    "s", "ms", "hz", "khz", "dpi", "dpcm", "dppx", "fr",
];

fn isValidCssMathUnit(unit: []const u8): bool {
    if (unit.len == 0) return false;
    for (u of CSS_MATH_UNITS) {
        if (eqlAsciiIgnoreCase(unit, u)) {
            return true;
        }
    }
    return false;
}

struct CssMathSyntaxValidator {
    src: []const u8,
    pos: usize = 0,
    depth: usize = 0,

    fn skipWs(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0c) {
                _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn peek(): u8? {
        if (_.pos >= _.src.len) return null;
        return _.src[_.pos];
    }

    fn consumeBalancedFallback(*_): bool {
        let p_depth: usize = 0;
        let b_depth: usize = 0;
        let c_depth: usize = 0;
        while (_.pos < _.src.len) {
            const ch = _.src[_.pos];
            if (ch == '(') {
                p_depth += 1;
                _.pos += 1;
            } else if (ch == ')') {
                if (p_depth == 0) {
                    if (b_depth != 0 || c_depth != 0) return false;
                    _.pos += 1;
                    return true;
                }
                p_depth -= 1;
                _.pos += 1;
            } else if (ch == '[') {
                b_depth += 1;
                _.pos += 1;
            } else if (ch == ']') {
                if (b_depth == 0) return false;
                b_depth -= 1;
                _.pos += 1;
            } else if (ch == '{') {
                c_depth += 1;
                _.pos += 1;
            } else if (ch == '}') {
                if (c_depth == 0) return false;
                c_depth -= 1;
                _.pos += 1;
            } else {
                _.pos += 1;
            }
        }
        return false;
    }

    fn validateZeroArgCall(*_): bool {
        _.skipWs();
        if ((_.peek() ?? 0) != ')') return false;
        _.pos += 1;
        return true;
    }

    fn validateVarCall(*_): bool {
        _.skipWs();
        if (_.pos + 2 >= _.src.len) return false;
        if (_.src[_.pos] != '-' || _.src[_.pos + 1] != '-') return false;
        _.pos += 2;
        const name_start = _.pos;
        while (_.pos < _.src.len && isIdentChar(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos == name_start) return false;
        _.skipWs();
        const ch = _.peek() ?? return false;
        if (ch == ')') {
            _.pos += 1;
            return true;
        }
        if (ch != ',') return false;
        _.pos += 1;
        return _.consumeBalancedFallback();
    }

    fn validateEnvCall(*_): bool {
        _.skipWs();
        const first = _.peek() ?? return false;
        if (!isIdentStart(first) && first != '-') return false;
        const name_start = _.pos;
        while (_.pos < _.src.len && isIdentChar(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos == name_start) return false;
        while (true) {
            _.skipWs();
            const ch = _.peek() ?? return false;
            if (ch == ')') {
                _.pos += 1;
                return true;
            }
            if (ch == ',') {
                _.pos += 1;
                return _.consumeBalancedFallback();
            }
            if (isDigit(ch)) {
                while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                    _.pos += 1;
                }
                continue;
            }
            return false;
        }
    }

    fn validateNumberWithUnit(*_): bool {
        let has_digit = false;
        while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
            has_digit = true;
            _.pos += 1;
        }
        if (_.pos < _.src.len && _.src[_.pos] == '.') {
            _.pos += 1;
            while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                has_digit = true;
                _.pos += 1;
            }
        }
        if (!has_digit) return false;

        if (_.pos < _.src.len && (_.src[_.pos] == 'e' || _.src[_.pos] == 'E')) {
            let exp_pos = _.pos + 1;
            if (exp_pos < _.src.len && (_.src[exp_pos] == '+' || _.src[exp_pos] == '-')) {
                exp_pos += 1;
            }
            if (exp_pos < _.src.len && isDigit(_.src[exp_pos])) {
                _.pos = exp_pos;
                while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                    _.pos += 1;
                }
            }
        }

        const next = _.peek() ?? return true;
        if (next == '%') {
            _.pos += 1;
            if (after := _.peek()) {
                if (isIdentStart(after) || isDigit(after)) return false;
            }
            return true;
        }
        if (isIdentStart(next)) {
            const unit_start = _.pos;
            while (_.pos < _.src.len && (isIdentStart(_.src[_.pos]) || isDigit(_.src[_.pos]))) {
                _.pos += 1;
            }
            return isValidCssMathUnit(_.src[unit_start.._.pos]);
        }
        return true;
    }

    fn validatePrimary(*_): bool {
        _.skipWs();
        if (_.depth >= CSS_MATH_MAX_DEPTH) return false;
        const ch = _.peek() ?? return false;

        if (ch == '(') {
            _.pos += 1;
            _.depth += 1;
            if (!_.validateSum()) return false;
            _.depth -= 1;
            _.skipWs();
            if ((_.peek() ?? 0) != ')') return false;
            _.pos += 1;
            return true;
        }

        if (isDigit(ch) || ch == '.') {
            return _.validateNumberWithUnit();
        }

        if (isIdentStart(ch)) {
            const start = _.pos;
            while (_.pos < _.src.len && isIdentChar(_.src[_.pos])) {
                _.pos += 1;
            }
            const ident = _.src[start.._.pos];
            if ((_.peek() ?? 0) == '(') {
                _.pos += 1;
                _.depth += 1;
                let ok = false;
                if (eqlAsciiIgnoreCase(ident, "var")) {
                    ok = _.validateVarCall();
                } else if (eqlAsciiIgnoreCase(ident, "env")) {
                    ok = _.validateEnvCall();
                } else if (eqlAsciiIgnoreCase(ident, "sibling-index") || eqlAsciiIgnoreCase(ident, "sibling-count")) {
                    ok = _.validateZeroArgCall();
                } else if (kind := CssMathFuncKind.parse(ident)) {
                    ok = _.validateFunctionCall(kind);
                }
                _.depth -= 1;
                return ok;
            }
            return CssMathConstant.parse(ident) != null;
        }

        return false;
    }

    fn validateUnary(*_): bool {
        _.skipWs();
        const ch = _.peek() ?? return false;
        if (ch == '+' || ch == '-') {
            if (ch == '-' && _.pos + 9 <= _.src.len && eqlAsciiIgnoreCase(_.src[_.pos.._.pos + 9], "-infinity")) {
                const after = _.pos + 9;
                if (after >= _.src.len || !isIdentChar(_.src[after])) {
                    _.pos = after;
                    return true;
                }
            }
            _.pos += 1;
            _.skipWs();
            if (next := _.peek()) {
                if (next == '+' || next == '-') return false;
            }
            return _.validatePrimary();
        }
        return _.validatePrimary();
    }

    fn validateProduct(*_): bool {
        if (!_.validateUnary()) return false;
        while (true) {
            _.skipWs();
            const ch = _.peek() ?? break;
            if (ch == '*' || ch == '/') {
                _.pos += 1;
                if (!_.validateUnary()) return false;
            } else {
                break;
            }
        }
        return true;
    }

    fn validateSum(*_): bool {
        if (!_.validateProduct()) return false;
        while (true) {
            _.skipWs();
            const ch = _.peek() ?? break;
            if (ch == '+' || ch == '-') {
                _.pos += 1;
                _.skipWs();
                if (next := _.peek()) {
                    if (next == '+') return false;
                    if (next == '-') {
                        const after = _.pos + 1;
                        if (after >= _.src.len) return false;
                        const c2 = _.src[after];
                        const is_neg_inf = (_.pos + 9 <= _.src.len && eqlAsciiIgnoreCase(_.src[_.pos.._.pos + 9], "-infinity"));
                        if (!isDigit(c2) && c2 != '.' && !is_neg_inf) return false;
                    }
                }
                if (!_.validateProduct()) return false;
            } else {
                break;
            }
        }
        return true;
    }

    fn validateRoundCall(*_): bool {
        _.skipWs();
        let has_strategy = false;
        const saved = _.pos;
        if (_.pos < _.src.len && isIdentStart(_.src[_.pos])) {
            while (_.pos < _.src.len && isIdentChar(_.src[_.pos])) {
                _.pos += 1;
            }
            const ident = _.src[saved.._.pos];
            _.skipWs();
            if ((_.peek() ?? 0) == ',') {
                if (CssMathRoundingStrategy.parse(ident) != null) {
                    has_strategy = true;
                    _.pos += 1;
                } else {
                    _.pos = saved;
                }
            } else {
                _.pos = saved;
            }
        }
        const argc = _.validateArgList() ?? return false;
        if (has_strategy) {
            return argc == 2;
        }
        return argc == 2;
    }

    fn validateArgList(*_): usize? {
        let count: usize = 0;
        _.skipWs();
        if ((_.peek() ?? 0) == ')') {
            _.pos += 1;
            return 0;
        }
        while (true) {
            if (!_.validateSum()) return null;
            count += 1;
            _.skipWs();
            const ch = _.peek() ?? return null;
            if (ch == ',') {
                _.pos += 1;
                continue;
            }
            if (ch == ')') {
                _.pos += 1;
                return count;
            }
            return null;
        }
    }

    fn validateFunctionCall(*_, kind: CssMathFuncKind): bool {
        if (_.depth > CSS_MATH_MAX_DEPTH) return false;
        if (kind == .Round) {
            return _.validateRoundCall();
        }
        const argc = _.validateArgList() ?? return false;
        return switch (kind) {
            .Calc, .Abs, .Sign, .Sin, .Cos, .Tan, .Asin, .Acos, .Atan, .Sqrt, .Exp => argc == 1,
            .Min, .Max, .Hypot => argc >= 1,
            .Clamp => argc == 3,
            .Mod, .Rem, .Atan2, .Pow => argc == 2,
            .Log => argc == 1 || argc == 2,
            .Round => false,
        };
    }
}

/// 零堆分配校验 W3C CSS Values & Units Level 4 数学表达式语法合法性
/// 要求顶层必须是合法 CSS 数学函数（`calc`/`min`/`max`/`clamp`/`round`/`mod`/`rem`/`sin`..`exp`/`abs`/`sign`），
/// 校验括号匹配、参数个数、W3C CSS 单位白名单、数学常量及 `var()`/`env()`/`sibling-index()`/`sibling-count()`。
pub fn isValidCssMathSyntax(expr: []const u8): bool {
    let v = CssMathSyntaxValidator{ src: expr };
    v.skipWs();
    const first = v.peek() ?? return false;
    if (!isIdentStart(first)) return false;
    const start = v.pos;
    while (v.pos < v.src.len && isIdentChar(v.src[v.pos])) {
        v.pos += 1;
    }
    const ident = v.src[start..v.pos];
    if ((v.peek() ?? 0) != '(') return false;
    const kind = CssMathFuncKind.parse(ident) ?? return false;
    v.pos += 1;
    v.depth = 1;
    if (!v.validateFunctionCall(kind)) return false;
    v.skipWs();
    return v.pos == v.src.len;
}
