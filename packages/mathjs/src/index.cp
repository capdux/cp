// @cap/mathjs-expr — Zero-allocation mathematical expression evaluator for CP
// Clean-room MIT implementation with parity to npm `mathjs` expression evaluation.
// Composes `@cap/libm` for all elementary & transcendental functions with zero duplication.

import {
    PI,
    TAU,
    E,
    fabs,
    floor,
    ceil,
    round,
    fmod,
    fmin,
    fmax,
    sqrt,
    exp,
    ln,
    pow,
    sin,
    cos,
    tan,
} from "../../libm/src/index.cp";

pub const MJ_OK: i32 = 0;
pub const MJ_ERR_SYNTAX: i32 = 1;
pub const MJ_ERR_UNCLOSED_PAREN: i32 = 2;
pub const MJ_ERR_DIV_ZERO: i32 = 3;
pub const MJ_ERR_UNKNOWN_IDENT: i32 = 4;
pub const MJ_ERR_ARITY: i32 = 5;
pub const MJ_ERR_DEPTH: i32 = 6;
pub const MJ_ERR_DOMAIN: i32 = 7;

const MJ_MAX_VARS: usize = 32;
const MJ_VAR_NAME_CAP: usize = 32;
const MJ_MAX_DEPTH: usize = 64;
const MJ_PHI: f64 = 1.61803398874989484820;

fn mjStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn mjCopyBuf(dst: []u8, src: string): usize {
    let n = src.len;
    if (n > dst.len) {
        n = dst.len;
    }
    let i: usize = 0;
    while (i < n) {
        dst[i] = src[i];
        i = i + 1;
    }
    return n;
}

fn mjIsSpace(ch: u8): bool {
    return ch == 32 || ch == 9 || ch == 10 || ch == 13;
}

fn mjIsDigit(ch: u8): bool {
    return ch >= 48 && ch <= 57;
}

fn mjIsIdentStart(ch: u8): bool {
    return (ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122) || ch == 95;
}

fn mjIsIdentChar(ch: u8): bool {
    return mjIsIdentStart(ch) || mjIsDigit(ch);
}

/// Single variable entry stored on the stack inside `MathScope`.
pub struct ScopeVar {
    pub name: [32]u8,
    pub name_len: usize,
    pub value: f64,
}

/// Lookup result returned by `MathScope.get`.
pub struct ScopeLookup {
    pub found: bool,
    pub value: f64,
}

/// Evaluation result returned by `evalExpr` and `evalExprWithScope`.
pub struct EvalResult {
    pub ok: bool,
    pub value: f64,
    pub err_code: i32,
}

fn mjOk(v: f64): EvalResult {
    if (v != v) {
        return EvalResult { ok: false, value: 0.0, err_code: MJ_ERR_DOMAIN };
    }
    return EvalResult { ok: true, value: v, err_code: MJ_OK };
}

fn mjErr(code: i32): EvalResult {
    return EvalResult { ok: false, value: 0.0, err_code: code };
}

/// Zero-allocation variable scope (up to 32 bindings) with built-in constants `pi`, `e`, `tau`, `phi`.
pub struct MathScope {
    vars: [32]ScopeVar,
    pub count: usize,

    /// Create an empty `MathScope`.
    pub static fn new(): MathScope {
        let vars: [32]ScopeVar = undefined;
        return MathScope {
            vars: vars,
            count: 0,
        };
    }

    /// Bind or update a variable `name` to `val`. Returns `false` if full or invalid name.
    pub fn set(*_, name: string, val: f64): bool {
        if (name.len == 0 || name.len > MJ_VAR_NAME_CAP) {
            return false;
        }
        let i: usize = 0;
        while (i < _.count) {
            const v = _.vars[i];
            if (mjStrEq(v.name[0..v.name_len], name)) {
                _.vars[i].value = val;
                return true;
            }
            i = i + 1;
        }
        if (_.count >= MJ_MAX_VARS) {
            return false;
        }
        let nbuf: [32]u8 = undefined;
        nbuf[0] = 0;
        const nlen = mjCopyBuf(nbuf[0..MJ_VAR_NAME_CAP], name);
        _.vars[_.count] = ScopeVar {
            name: nbuf,
            name_len: nlen,
            value: val,
        };
        _.count = _.count + 1;
        return true;
    }

    /// Look up `name` in user bindings first, then built-in mathematical constants (`pi`, `e`, `tau`, `phi`).
    pub fn get(name: string): ScopeLookup {
        let i: usize = 0;
        while (i < _.count) {
            const v = _.vars[i];
            if (mjStrEq(v.name[0..v.name_len], name)) {
                return ScopeLookup { found: true, value: v.value };
            }
            i = i + 1;
        }
        if (mjStrEq(name, "pi") || mjStrEq(name, "PI")) {
            return ScopeLookup { found: true, value: PI };
        }
        if (mjStrEq(name, "e") || mjStrEq(name, "E")) {
            return ScopeLookup { found: true, value: E };
        }
        if (mjStrEq(name, "tau") || mjStrEq(name, "TAU")) {
            return ScopeLookup { found: true, value: TAU };
        }
        if (mjStrEq(name, "phi") || mjStrEq(name, "PHI")) {
            return ScopeLookup { found: true, value: MJ_PHI };
        }
        return ScopeLookup { found: false, value: 0.0 };
    }
}

struct MjParser {
    src: string,
    pos: usize,
    depth: usize,
    scope: MathScope,
}

fn mjSkipWs(p: *MjParser) {
    while (p.pos < p.src.len && mjIsSpace(p.src[p.pos])) {
        p.pos = p.pos + 1;
    }
}

fn mjParseNumber(p: *MjParser): EvalResult {
    const start = p.pos;
    let int_part: f64 = 0.0;
    let has_digits = false;

    while (p.pos < p.src.len && mjIsDigit(p.src[p.pos])) {
        int_part = (int_part * 10.0) + ((p.src[p.pos] - 48) as f64);
        p.pos = p.pos + 1;
        has_digits = true;
    }

    let frac_part: f64 = 0.0;
    let frac_scale: f64 = 0.1;
    if (p.pos < p.src.len && p.src[p.pos] == 46) { // '.'
        p.pos = p.pos + 1;
        while (p.pos < p.src.len && mjIsDigit(p.src[p.pos])) {
            frac_part = frac_part + (((p.src[p.pos] - 48) as f64) * frac_scale);
            frac_scale = frac_scale * 0.1;
            p.pos = p.pos + 1;
            has_digits = true;
        }
    }

    if (!has_digits || p.pos == start) {
        return mjErr(MJ_ERR_SYNTAX);
    }

    let val = int_part + frac_part;

    // Optional scientific notation exponent `e[+-]?digits`
    if (p.pos < p.src.len && (p.src[p.pos] == 101 || p.src[p.pos] == 69)) {
        p.pos = p.pos + 1;
        let exp_neg = false;
        if (p.pos < p.src.len && p.src[p.pos] == 45) {
            exp_neg = true;
            p.pos = p.pos + 1;
        } else if (p.pos < p.src.len && p.src[p.pos] == 43) {
            p.pos = p.pos + 1;
        }
        if (p.pos >= p.src.len || !mjIsDigit(p.src[p.pos])) {
            return mjErr(MJ_ERR_SYNTAX);
        }
        let exp_val: f64 = 0.0;
        while (p.pos < p.src.len && mjIsDigit(p.src[p.pos])) {
            exp_val = (exp_val * 10.0) + ((p.src[p.pos] - 48) as f64);
            p.pos = p.pos + 1;
        }
        if (exp_neg) {
            exp_val = -exp_val;
        }
        val = val * pow(10.0, exp_val);
    }

    return mjOk(val);
}

fn mjEvalBuiltin(name: string, args: []const f64, argc: usize): EvalResult {
    // 1-argument functions (delegated to @cap/libm)
    if (mjStrEq(name, "sin")) {
        if (argc != 1) { return mjErr(MJ_ERR_ARITY); }
        return mjOk(sin(args[0]));
    }
    if (mjStrEq(name, "cos")) {
        if (argc != 1) { return mjErr(MJ_ERR_ARITY); }
        return mjOk(cos(args[0]));
    }
    if (mjStrEq(name, "tan")) {
        if (argc != 1) { return mjErr(MJ_ERR_ARITY); }
        return mjOk(tan(args[0]));
    }
    if (mjStrEq(name, "sqrt")) {
        if (argc != 1) { return mjErr(MJ_ERR_ARITY); }
        if (args[0] < 0.0) { return mjErr(MJ_ERR_DOMAIN); }
        return mjOk(sqrt(args[0]));
    }
    if (mjStrEq(name, "abs")) {
        if (argc != 1) { return mjErr(MJ_ERR_ARITY); }
        return mjOk(fabs(args[0]));
    }
    if (mjStrEq(name, "floor")) {
        if (argc != 1) { return mjErr(MJ_ERR_ARITY); }
        return mjOk(floor(args[0]));
    }
    if (mjStrEq(name, "ceil")) {
        if (argc != 1) { return mjErr(MJ_ERR_ARITY); }
        return mjOk(ceil(args[0]));
    }
    if (mjStrEq(name, "round")) {
        if (argc != 1) { return mjErr(MJ_ERR_ARITY); }
        return mjOk(round(args[0]));
    }
    if (mjStrEq(name, "exp")) {
        if (argc != 1) { return mjErr(MJ_ERR_ARITY); }
        return mjOk(exp(args[0]));
    }
    if (mjStrEq(name, "log")) {
        if (argc != 1) { return mjErr(MJ_ERR_ARITY); }
        if (args[0] <= 0.0) { return mjErr(MJ_ERR_DOMAIN); }
        return mjOk(ln(args[0]));
    }

    // 2-argument functions (delegated to @cap/libm)
    if (mjStrEq(name, "pow")) {
        if (argc != 2) { return mjErr(MJ_ERR_ARITY); }
        return mjOk(pow(args[0], args[1]));
    }
    if (mjStrEq(name, "min")) {
        if (argc != 2) { return mjErr(MJ_ERR_ARITY); }
        return mjOk(fmin(args[0], args[1]));
    }
    if (mjStrEq(name, "max")) {
        if (argc != 2) { return mjErr(MJ_ERR_ARITY); }
        return mjOk(fmax(args[0], args[1]));
    }

    // 3-argument function
    if (mjStrEq(name, "clamp")) {
        if (argc != 3) { return mjErr(MJ_ERR_ARITY); }
        return mjOk(fmin(fmax(args[0], args[1]), args[2]));
    }

    return mjErr(MJ_ERR_UNKNOWN_IDENT);
}

fn mjParsePrimary(p: *MjParser): EvalResult {
    if (p.depth >= MJ_MAX_DEPTH) {
        return mjErr(MJ_ERR_DEPTH);
    }
    mjSkipWs(p);
    if (p.pos >= p.src.len) {
        return mjErr(MJ_ERR_SYNTAX);
    }

    const ch = p.src[p.pos];

    // Parenthesized expression `(...)`
    if (ch == 40) { // '('
        p.pos = p.pos + 1;
        p.depth = p.depth + 1;
        const inner = mjParseAddSub(p);
        p.depth = p.depth - 1;
        if (!inner.ok) {
            return inner;
        }
        mjSkipWs(p);
        if (p.pos >= p.src.len || p.src[p.pos] != 41) { // ')'
            return mjErr(MJ_ERR_UNCLOSED_PAREN);
        }
        p.pos = p.pos + 1;
        return inner;
    }

    // Number literal
    if (mjIsDigit(ch) || ch == 46) {
        return mjParseNumber(p);
    }

    // Identifier (function call or variable/constant lookup)
    if (mjIsIdentStart(ch)) {
        const start = p.pos;
        p.pos = p.pos + 1;
        while (p.pos < p.src.len && mjIsIdentChar(p.src[p.pos])) {
            p.pos = p.pos + 1;
        }
        const ident = p.src[start..p.pos];
        mjSkipWs(p);

        // Function call `ident(arg1, arg2, ...)`
        if (p.pos < p.src.len && p.src[p.pos] == 40) {
            p.pos = p.pos + 1;
            p.depth = p.depth + 1;
            let args: [4]f64 = [0.0, 0.0, 0.0, 0.0];
            let argc: usize = 0;

            mjSkipWs(p);
            if (p.pos < p.src.len && p.src[p.pos] == 41) {
                p.pos = p.pos + 1;
                p.depth = p.depth - 1;
                return mjEvalBuiltin(ident, args[0..4], 0);
            }

            while (true) {
                const ares = mjParseAddSub(p);
                if (!ares.ok) {
                    p.depth = p.depth - 1;
                    return ares;
                }
                if (argc < 4) {
                    args[argc] = ares.value;
                }
                argc = argc + 1;
                mjSkipWs(p);
                if (p.pos >= p.src.len) {
                    p.depth = p.depth - 1;
                    return mjErr(MJ_ERR_UNCLOSED_PAREN);
                }
                if (p.src[p.pos] == 44) { // ','
                    p.pos = p.pos + 1;
                    continue;
                }
                if (p.src[p.pos] == 41) { // ')'
                    p.pos = p.pos + 1;
                    break;
                }
                p.depth = p.depth - 1;
                return mjErr(MJ_ERR_UNCLOSED_PAREN);
            }
            p.depth = p.depth - 1;
            return mjEvalBuiltin(ident, args[0..4], argc);
        }

        // Variable or built-in constant
        const found = p.scope.get(ident);
        if (!found.found) {
            return mjErr(MJ_ERR_UNKNOWN_IDENT);
        }
        return mjOk(found.value);
    }

    return mjErr(MJ_ERR_SYNTAX);
}

fn mjParsePower(p: *MjParser): EvalResult {
    const left = mjParsePrimary(p);
    if (!left.ok) {
        return left;
    }
    mjSkipWs(p);
    if (p.pos < p.src.len && p.src[p.pos] == 94) { // '^' (right-associative)
        if (p.depth >= MJ_MAX_DEPTH) {
            return mjErr(MJ_ERR_DEPTH);
        }
        p.pos = p.pos + 1;
        p.depth = p.depth + 1;
        const right = mjParseUnary(p);
        p.depth = p.depth - 1;
        if (!right.ok) {
            return right;
        }
        return mjOk(pow(left.value, right.value));
    }
    return left;
}

fn mjParseUnary(p: *MjParser): EvalResult {
    if (p.depth >= MJ_MAX_DEPTH) {
        return mjErr(MJ_ERR_DEPTH);
    }
    mjSkipWs(p);
    if (p.pos < p.src.len && p.src[p.pos] == 43) { // unary '+'
        p.pos = p.pos + 1;
        p.depth = p.depth + 1;
        const sub = mjParseUnary(p);
        p.depth = p.depth - 1;
        return sub;
    }
    if (p.pos < p.src.len && p.src[p.pos] == 45) { // unary '-'
        p.pos = p.pos + 1;
        p.depth = p.depth + 1;
        const sub = mjParseUnary(p);
        p.depth = p.depth - 1;
        if (!sub.ok) {
            return sub;
        }
        return mjOk(-sub.value);
    }
    return mjParsePower(p);
}

fn mjParseMulDiv(p: *MjParser): EvalResult {
    let acc = mjParseUnary(p);
    if (!acc.ok) {
        return acc;
    }
    while (true) {
        mjSkipWs(p);
        if (p.pos >= p.src.len) {
            break;
        }
        const op = p.src[p.pos];
        if (op != 42 && op != 47 && op != 37) { // '*', '/', '%'
            break;
        }
        p.pos = p.pos + 1;
        const rhs = mjParseUnary(p);
        if (!rhs.ok) {
            return rhs;
        }
        if (op == 42) {
            acc = mjOk(acc.value * rhs.value);
        } else if (op == 47) {
            if (rhs.value == 0.0) {
                return mjErr(MJ_ERR_DIV_ZERO);
            }
            acc = mjOk(acc.value / rhs.value);
        } else {
            if (rhs.value == 0.0) {
                return mjErr(MJ_ERR_DIV_ZERO);
            }
            acc = mjOk(fmod(acc.value, rhs.value));
        }
        if (!acc.ok) {
            return acc;
        }
    }
    return acc;
}

fn mjParseAddSub(p: *MjParser): EvalResult {
    let acc = mjParseMulDiv(p);
    if (!acc.ok) {
        return acc;
    }
    while (true) {
        mjSkipWs(p);
        if (p.pos >= p.src.len) {
            break;
        }
        const op = p.src[p.pos];
        if (op != 43 && op != 45) { // '+', '-'
            break;
        }
        p.pos = p.pos + 1;
        const rhs = mjParseMulDiv(p);
        if (!rhs.ok) {
            return rhs;
        }
        if (op == 43) {
            acc = mjOk(acc.value + rhs.value);
        } else {
            acc = mjOk(acc.value - rhs.value);
        }
        if (!acc.ok) {
            return acc;
        }
    }
    return acc;
}

/// Evaluate a mathematical expression string with a custom `MathScope`.
pub fn evalExprWithScope(expr: string, scope: MathScope): EvalResult {
    let parser = MjParser {
        src: expr,
        pos: 0,
        depth: 0,
        scope: scope,
    };
    mjSkipWs(&parser);
    if (parser.pos >= parser.src.len) {
        return mjErr(MJ_ERR_SYNTAX);
    }
    const res = mjParseAddSub(&parser);
    if (!res.ok) {
        return res;
    }
    mjSkipWs(&parser);
    if (parser.pos < parser.src.len) {
        return mjErr(MJ_ERR_SYNTAX);
    }
    return res;
}

/// Evaluate a mathematical expression string using the default built-in constants scope.
pub fn evalExpr(expr: string): EvalResult {
    return evalExprWithScope(expr, MathScope.new());
}

export {
    MJ_OK,
    MJ_ERR_SYNTAX,
    MJ_ERR_UNCLOSED_PAREN,
    MJ_ERR_DIV_ZERO,
    MJ_ERR_UNKNOWN_IDENT,
    MJ_ERR_ARITY,
    MJ_ERR_DEPTH,
    MJ_ERR_DOMAIN,
    ScopeVar,
    ScopeLookup,
    EvalResult,
    MathScope,
    evalExpr,
    evalExprWithScope,
};
