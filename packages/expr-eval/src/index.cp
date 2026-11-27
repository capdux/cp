// @cap/expr — Zero-allocation Recursive-Descent Expression Evaluation Engine
// Clean-room pure-CP implementation modeled after npm `expr-eval`/`filtrex`/`jexl`
// and Rust `evalexpr`/`meval` (MIT License).

import { isSpace, isDigit, isAlpha, isAlnum, strEq } from "cap:ascii";
const std = @import("std");

pub const MAX_VARS: usize = 64;
pub const MAX_DEPTH: usize = 32;
pub const MAX_CALL_ARGS: usize = 8;

pub const MATH_PI: f64 = 3.14159265358979323846;
pub const MATH_E: f64 = 2.71828182845904523536;

/// Structured error code produced during lexing, parsing, or evaluation.
pub enum ExprError: u8 {
    None = 0,
    SyntaxError = 1,
    UnexpectedToken = 2,
    DivisionByZero = 3,
    UnknownVariable = 4,
    UnknownFunction = 5,
    InvalidArgs = 6,
    MaxDepthExceeded = 7,
}

/// Structured evaluation outcome (zero heap allocation, no exceptions).
pub struct EvalResult {
    ok: bool,
    value: f64,
    err: ExprError,
    pos: usize,

    pub static fn success(v: f64): EvalResult {
        return EvalResult{
            ok: true,
            value: v,
            err: ExprError.None,
            pos: 0,
        };
    }

    pub static fn fail(err: ExprError, pos: usize): EvalResult {
        return EvalResult{
            ok: false,
            value: 0.0,
            err: err,
            pos: pos,
        };
    }
}

struct EnvEntry {
    name: string = "",
    value: f64 = 0.0,
}

/// Stack-allocated variable environment holding up to 64 named `f64` bindings.
pub struct ExprEnv {
    entries: [64]EnvEntry = undefined,
    count: usize = 0,

    /// Creates an empty `ExprEnv` on the stack.
    pub static fn init(): ExprEnv {
        return ExprEnv{
            entries: undefined,
            count: 0,
        };
    }

    /// Binds or updates a named `f64` variable (silently ignores overflow beyond `MAX_VARS`).
    pub fn set(*_, name: string, value: f64) {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (strEq(_.entries[i].name, name)) {
                _.entries[i].value = value;
                return;
            }
        }
        if (_.count >= MAX_VARS) return;
        _.entries[_.count] = EnvEntry{
            name: name,
            value: value,
        };
        _.count += 1;
    }

    /// Looks up a variable value by name, returning `null` if not bound.
    pub fn get(name: string): f64? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (strEq(_.entries[i].name, name)) {
                return _.entries[i].value;
            }
        }
        return null;
    }

    /// Returns `true` if a variable with `name` exists in this environment.
    pub fn has(name: string): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (strEq(_.entries[i].name, name)) {
                return true;
            }
        }
        return false;
    }

    /// Clears all bound variables for zero-allocation reuse.
    pub fn clear(*_) {
        _.count = 0;
    }

    /// Returns the number of bound variables.
    pub fn len(): usize {
        return _.count;
    }
}

enum TokenKind: u8 {
    Eof = 0,
    Number = 1,
    Ident = 2,
    Plus = 3,
    Minus = 4,
    Star = 5,
    Slash = 6,
    Percent = 7,
    Caret = 8,
    LParen = 9,
    RParen = 10,
    Comma = 11,
    Question = 12,
    Colon = 13,
    Bang = 14,
    EqEq = 15,
    BangEq = 16,
    Lt = 17,
    LtEq = 18,
    Gt = 19,
    GtEq = 20,
    AndAnd = 21,
    OrOr = 22,
    Invalid = 23,
}

struct Token {
    kind: TokenKind = TokenKind.Eof,
    start: usize = 0,
    end: usize = 0,
    num: f64 = 0.0,
}

fn isIdentStart(c: u8): bool {
    return isAlpha(c) || c == '_';
}

fn isIdentCont(c: u8): bool {
    return isAlnum(c) || c == '_';
}

fn isTruthy(v: f64): bool {
    return v != 0.0;
}

fn parseFloatLiteral(slice: string): f64? {
    return std.fmt.parseFloat(f64, slice) catch null;
}

struct Parser {
    src: string,
    pos: usize,
    tok: Token,
    env: *const ExprEnv,
    depth: usize,
    err: ExprError,
    err_pos: usize,

    static fn init(src: string, env: *const ExprEnv): Parser {
        let p = Parser{
            src: src,
            pos: 0,
            tok: Token{},
            env: env,
            depth: 0,
            err: ExprError.None,
            err_pos: 0,
        };
        p.advance();
        return p;
    }

    fn hasError(): bool {
        return _.err != ExprError.None;
    }

    fn setError(*_, err: ExprError, pos: usize) {
        if (_.err == ExprError.None) {
            _.err = err;
            _.err_pos = pos;
        }
    }

    fn enterDepth(*_, pos: usize): bool {
        _.depth += 1;
        if (_.depth > MAX_DEPTH) {
            _.setError(ExprError.MaxDepthExceeded, pos);
            return false;
        }
        return true;
    }

    fn leaveDepth(*_) {
        if (_.depth > 0) {
            _.depth -= 1;
        }
    }

    fn advance(*_) {
        const s = _.src;
        const n = s.len;
        while (_.pos < n && isSpace(s[_.pos])) {
            _.pos += 1;
        }

        if (_.pos >= n) {
            _.tok = Token{
                kind: TokenKind.Eof,
                start: _.pos,
                end: _.pos,
                num: 0.0,
            };
            return;
        }

        const start = _.pos;
        const c = s[start];

        // Number literal: digits or `.digits` with optional fractional & scientific exponent
        if (isDigit(c) || (c == '.' && start + 1 < n && isDigit(s[start + 1]))) {
            let i = start;
            while (i < n && isDigit(s[i])) {
                i += 1;
            }
            if (i < n && s[i] == '.') {
                i += 1;
                while (i < n && isDigit(s[i])) {
                    i += 1;
                }
            }
            if (i < n && (s[i] == 'e' || s[i] == 'E')) {
                let j = i + 1;
                if (j < n && (s[j] == '+' || s[j] == '-')) {
                    j += 1;
                }
                if (j < n && isDigit(s[j])) {
                    while (j < n && isDigit(s[j])) {
                        j += 1;
                    }
                    i = j;
                } else {
                    _.pos = i;
                    _.setError(ExprError.SyntaxError, start);
                    _.tok = Token{ kind: TokenKind.Invalid, start: start, end: j, num: 0.0 };
                    return;
                }
            }
            _.pos = i;
            if (val := parseFloatLiteral(s[start..i])) {
                _.tok = Token{
                    kind: TokenKind.Number,
                    start: start,
                    end: i,
                    num: val,
                };
            } else {
                _.setError(ExprError.SyntaxError, start);
                _.tok = Token{ kind: TokenKind.Invalid, start: start, end: i, num: 0.0 };
            }
            return;
        }

        // Identifier
        if (isIdentStart(c)) {
            let i = start + 1;
            while (i < n && isIdentCont(s[i])) {
                i += 1;
            }
            _.pos = i;
            _.tok = Token{
                kind: TokenKind.Ident,
                start: start,
                end: i,
                num: 0.0,
            };
            return;
        }

        // Two-character and single-character operators
        const next: u8 = if (start + 1 < n) s[start + 1] else 0;

        if (c == '=' && next == '=') {
            _.pos = start + 2;
            _.tok = Token{ kind: TokenKind.EqEq, start: start, end: start + 2, num: 0.0 };
            return;
        }
        if (c == '!' && next == '=') {
            _.pos = start + 2;
            _.tok = Token{ kind: TokenKind.BangEq, start: start, end: start + 2, num: 0.0 };
            return;
        }
        if (c == '<' && next == '=') {
            _.pos = start + 2;
            _.tok = Token{ kind: TokenKind.LtEq, start: start, end: start + 2, num: 0.0 };
            return;
        }
        if (c == '>' && next == '=') {
            _.pos = start + 2;
            _.tok = Token{ kind: TokenKind.GtEq, start: start, end: start + 2, num: 0.0 };
            return;
        }
        if (c == '&' && next == '&') {
            _.pos = start + 2;
            _.tok = Token{ kind: TokenKind.AndAnd, start: start, end: start + 2, num: 0.0 };
            return;
        }
        if (c == '|' && next == '|') {
            _.pos = start + 2;
            _.tok = Token{ kind: TokenKind.OrOr, start: start, end: start + 2, num: 0.0 };
            return;
        }

        _.pos = start + 1;
        switch (c) {
            '+' => { _.tok = Token{ kind: TokenKind.Plus, start: start, end: start + 1, num: 0.0 }; },
            '-' => { _.tok = Token{ kind: TokenKind.Minus, start: start, end: start + 1, num: 0.0 }; },
            '*' => { _.tok = Token{ kind: TokenKind.Star, start: start, end: start + 1, num: 0.0 }; },
            '/' => { _.tok = Token{ kind: TokenKind.Slash, start: start, end: start + 1, num: 0.0 }; },
            '%' => { _.tok = Token{ kind: TokenKind.Percent, start: start, end: start + 1, num: 0.0 }; },
            '^' => { _.tok = Token{ kind: TokenKind.Caret, start: start, end: start + 1, num: 0.0 }; },
            '(' => { _.tok = Token{ kind: TokenKind.LParen, start: start, end: start + 1, num: 0.0 }; },
            ')' => { _.tok = Token{ kind: TokenKind.RParen, start: start, end: start + 1, num: 0.0 }; },
            ',' => { _.tok = Token{ kind: TokenKind.Comma, start: start, end: start + 1, num: 0.0 }; },
            '?' => { _.tok = Token{ kind: TokenKind.Question, start: start, end: start + 1, num: 0.0 }; },
            ':' => { _.tok = Token{ kind: TokenKind.Colon, start: start, end: start + 1, num: 0.0 }; },
            '!' => { _.tok = Token{ kind: TokenKind.Bang, start: start, end: start + 1, num: 0.0 }; },
            '<' => { _.tok = Token{ kind: TokenKind.Lt, start: start, end: start + 1, num: 0.0 }; },
            '>' => { _.tok = Token{ kind: TokenKind.Gt, start: start, end: start + 1, num: 0.0 }; },
            else => {
                _.setError(ExprError.SyntaxError, start);
                _.tok = Token{ kind: TokenKind.Invalid, start: start, end: start + 1, num: 0.0 };
            },
        }
    }

    fn parseExpr(*_, active: bool): f64 {
        return _.parseTernary(active);
    }

    // Ternary: cond ? then_expr : else_expr (right-associative, short-circuiting)
    fn parseTernary(*_, active: bool): f64 {
        const cond = _.parseOr(active);
        if (_.hasError()) return 0.0;

        if (_.tok.kind == TokenKind.Question) {
            const q_pos = _.tok.start;
            if (!_.enterDepth(q_pos)) return 0.0;
            _.advance();
            const take_then = active && isTruthy(cond);
            const then_val = _.parseExpr(take_then);
            if (_.hasError()) {
                _.leaveDepth();
                return 0.0;
            }
            if (_.tok.kind != TokenKind.Colon) {
                _.setError(ExprError.SyntaxError, _.tok.start);
                _.leaveDepth();
                return 0.0;
            }
            _.advance();
            const take_else = active && !isTruthy(cond);
            const else_val = _.parseTernary(take_else);
            _.leaveDepth();
            if (_.hasError()) return 0.0;
            return if (isTruthy(cond)) then_val else else_val;
        }
        return cond;
    }

    // Logical OR: || (left-associative, short-circuiting)
    fn parseOr(*_, active: bool): f64 {
        let left = _.parseAnd(active);
        if (_.hasError()) return 0.0;

        while (_.tok.kind == TokenKind.OrOr) {
            _.advance();
            const left_truthy = isTruthy(left);
            const right = _.parseAnd(active && !left_truthy);
            if (_.hasError()) return 0.0;
            left = if (left_truthy || isTruthy(right)) 1.0 else 0.0;
        }
        return left;
    }

    // Logical AND: && (left-associative, short-circuiting)
    fn parseAnd(*_, active: bool): f64 {
        let left = _.parseEquality(active);
        if (_.hasError()) return 0.0;

        while (_.tok.kind == TokenKind.AndAnd) {
            _.advance();
            const left_truthy = isTruthy(left);
            const right = _.parseEquality(active && left_truthy);
            if (_.hasError()) return 0.0;
            left = if (left_truthy && isTruthy(right)) 1.0 else 0.0;
        }
        return left;
    }

    // Equality: == != (left-associative)
    fn parseEquality(*_, active: bool): f64 {
        let left = _.parseComparison(active);
        if (_.hasError()) return 0.0;

        while (_.tok.kind == TokenKind.EqEq || _.tok.kind == TokenKind.BangEq) {
            const op = _.tok.kind;
            _.advance();
            const right = _.parseComparison(active);
            if (_.hasError()) return 0.0;
            if (op == TokenKind.EqEq) {
                left = if (left == right) 1.0 else 0.0;
            } else {
                left = if (left != right) 1.0 else 0.0;
            }
        }
        return left;
    }

    // Comparison: < <= > >= (left-associative)
    fn parseComparison(*_, active: bool): f64 {
        let left = _.parseAdditive(active);
        if (_.hasError()) return 0.0;

        while (
            _.tok.kind == TokenKind.Lt
                || _.tok.kind == TokenKind.LtEq
                || _.tok.kind == TokenKind.Gt
                || _.tok.kind == TokenKind.GtEq
        ) {
            const op = _.tok.kind;
            _.advance();
            const right = _.parseAdditive(active);
            if (_.hasError()) return 0.0;
            switch (op) {
                TokenKind.Lt => { left = if (left < right) 1.0 else 0.0; },
                TokenKind.LtEq => { left = if (left <= right) 1.0 else 0.0; },
                TokenKind.Gt => { left = if (left > right) 1.0 else 0.0; },
                TokenKind.GtEq => { left = if (left >= right) 1.0 else 0.0; },
                else => {},
            }
        }
        return left;
    }

    // Additive: + - (left-associative)
    fn parseAdditive(*_, active: bool): f64 {
        let left = _.parseMultiplicative(active);
        if (_.hasError()) return 0.0;

        while (_.tok.kind == TokenKind.Plus || _.tok.kind == TokenKind.Minus) {
            const op = _.tok.kind;
            _.advance();
            const right = _.parseMultiplicative(active);
            if (_.hasError()) return 0.0;
            if (op == TokenKind.Plus) {
                left = left + right;
            } else {
                left = left - right;
            }
        }
        return left;
    }

    // Multiplicative: * / % (left-associative)
    fn parseMultiplicative(*_, active: bool): f64 {
        let left = _.parsePower(active);
        if (_.hasError()) return 0.0;

        while (
            _.tok.kind == TokenKind.Star
                || _.tok.kind == TokenKind.Slash
                || _.tok.kind == TokenKind.Percent
        ) {
            const op = _.tok.kind;
            const op_pos = _.tok.start;
            _.advance();
            const right = _.parsePower(active);
            if (_.hasError()) return 0.0;
            if (!active) {
                left = 0.0;
            } else if (op == TokenKind.Star) {
                left = left * right;
            } else if (op == TokenKind.Slash) {
                if (right == 0.0) {
                    _.setError(ExprError.DivisionByZero, op_pos);
                    return 0.0;
                }
                left = left / right;
            } else {
                if (right == 0.0) {
                    _.setError(ExprError.DivisionByZero, op_pos);
                    return 0.0;
                }
                left = @rem(left, right);
            }
        }
        return left;
    }

    // Power: ^ (right-associative, lower precedence than unary - + !)
    fn parsePower(*_, active: bool): f64 {
        const base = _.parseUnary(active);
        if (_.hasError()) return 0.0;

        if (_.tok.kind == TokenKind.Caret) {
            const op_pos = _.tok.start;
            if (!_.enterDepth(op_pos)) return 0.0;
            _.advance();
            const exp = _.parsePower(active);
            _.leaveDepth();
            if (_.hasError()) return 0.0;
            if (!active) return 0.0;
            return std.math.pow(f64, base, exp);
        }
        return base;
    }

    // Unary: - + ! (right-recursive)
    fn parseUnary(*_, active: bool): f64 {
        if (
            _.tok.kind == TokenKind.Minus
                || _.tok.kind == TokenKind.Plus
                || _.tok.kind == TokenKind.Bang
        ) {
            const op = _.tok.kind;
            const op_pos = _.tok.start;
            if (!_.enterDepth(op_pos)) return 0.0;
            _.advance();
            const v = _.parseUnary(active);
            _.leaveDepth();
            if (_.hasError()) return 0.0;
            if (op == TokenKind.Minus) return -v;
            if (op == TokenKind.Plus) return v;
            return if (isTruthy(v)) 0.0 else 1.0;
        }
        return _.parsePrimary(active);
    }

    // Primary: number | true | false | PI | E | var | func(args...) | (expr)
    fn parsePrimary(*_, active: bool): f64 {
        const tok = _.tok;
        switch (tok.kind) {
            TokenKind.Number => {
                _.advance();
                return tok.num;
            },
            TokenKind.LParen => {
                if (!_.enterDepth(tok.start)) return 0.0;
                _.advance();
                const val = _.parseExpr(active);
                if (_.hasError()) {
                    _.leaveDepth();
                    return 0.0;
                }
                if (_.tok.kind != TokenKind.RParen) {
                    _.setError(ExprError.SyntaxError, _.tok.start);
                    _.leaveDepth();
                    return 0.0;
                }
                _.advance();
                _.leaveDepth();
                return val;
            },
            TokenKind.Ident => {
                const name = _.src[tok.start..tok.end];
                const name_pos = tok.start;
                _.advance();

                // Function call
                if (_.tok.kind == TokenKind.LParen) {
                    return _.parseCall(name, name_pos, active);
                }

                // Built-in literals & mathematical constants
                if (strEq(name, "true")) return 1.0;
                if (strEq(name, "false")) return 0.0;
                if (strEq(name, "PI")) return MATH_PI;
                if (strEq(name, "E")) return MATH_E;

                // Variable lookup
                if (!active) return 0.0;
                if (val := _.env.get(name)) {
                    return val;
                }
                _.setError(ExprError.UnknownVariable, name_pos);
                return 0.0;
            },
            else => {
                _.setError(ExprError.SyntaxError, tok.start);
                return 0.0;
            },
        }
    }

    fn parseCall(*_, name: string, name_pos: usize, active: bool): f64 {
        if (!_.enterDepth(name_pos)) return 0.0;
        // Consume '('
        _.advance();

        let args: [8]f64 = undefined;
        let argc: usize = 0;

        if (_.tok.kind != TokenKind.RParen) {
            while (true) {
                const arg_val = _.parseExpr(active);
                if (_.hasError()) {
                    _.leaveDepth();
                    return 0.0;
                }
                if (argc >= MAX_CALL_ARGS) {
                    _.setError(ExprError.InvalidArgs, name_pos);
                    _.leaveDepth();
                    return 0.0;
                }
                args[argc] = arg_val;
                argc += 1;

                if (_.tok.kind == TokenKind.Comma) {
                    _.advance();
                } else {
                    break;
                }
            }
        }

        if (_.tok.kind != TokenKind.RParen) {
            _.setError(ExprError.SyntaxError, _.tok.start);
            _.leaveDepth();
            return 0.0;
        }
        _.advance();
        _.leaveDepth();

        if (!active) return 0.0;
        return _.evalBuiltin(name, name_pos, args[0..argc]);
    }

    fn evalBuiltin(*_, name: string, pos: usize, args: []const f64): f64 {
        if (strEq(name, "abs")) {
            if (args.len != 1) {
                _.setError(ExprError.InvalidArgs, pos);
                return 0.0;
            }
            return @abs(args[0]);
        }
        if (strEq(name, "min")) {
            if (args.len != 2) {
                _.setError(ExprError.InvalidArgs, pos);
                return 0.0;
            }
            return if (args[0] < args[1]) args[0] else args[1];
        }
        if (strEq(name, "max")) {
            if (args.len != 2) {
                _.setError(ExprError.InvalidArgs, pos);
                return 0.0;
            }
            return if (args[0] > args[1]) args[0] else args[1];
        }
        if (strEq(name, "floor")) {
            if (args.len != 1) {
                _.setError(ExprError.InvalidArgs, pos);
                return 0.0;
            }
            return @floor(args[0]);
        }
        if (strEq(name, "ceil")) {
            if (args.len != 1) {
                _.setError(ExprError.InvalidArgs, pos);
                return 0.0;
            }
            return @ceil(args[0]);
        }
        if (strEq(name, "round")) {
            if (args.len != 1) {
                _.setError(ExprError.InvalidArgs, pos);
                return 0.0;
            }
            return @round(args[0]);
        }
        if (strEq(name, "sqrt")) {
            if (args.len != 1 || args[0] < 0.0) {
                _.setError(ExprError.InvalidArgs, pos);
                return 0.0;
            }
            return @sqrt(args[0]);
        }
        if (strEq(name, "pow")) {
            if (args.len != 2) {
                _.setError(ExprError.InvalidArgs, pos);
                return 0.0;
            }
            return std.math.pow(f64, args[0], args[1]);
        }
        if (strEq(name, "clamp")) {
            if (args.len != 3 || args[1] > args[2]) {
                _.setError(ExprError.InvalidArgs, pos);
                return 0.0;
            }
            if (args[0] < args[1]) return args[1];
            if (args[0] > args[2]) return args[2];
            return args[0];
        }
        if (strEq(name, "sign")) {
            if (args.len != 1) {
                _.setError(ExprError.InvalidArgs, pos);
                return 0.0;
            }
            if (args[0] > 0.0) return 1.0;
            if (args[0] < 0.0) return -1.0;
            return 0.0;
        }
        if (strEq(name, "avg")) {
            if (args.len != 2) {
                _.setError(ExprError.InvalidArgs, pos);
                return 0.0;
            }
            return (args[0] + args[1]) * 0.5;
        }
        if (strEq(name, "if")) {
            if (args.len != 3) {
                _.setError(ExprError.InvalidArgs, pos);
                return 0.0;
            }
            return if (isTruthy(args[0])) args[1] else args[2];
        }

        _.setError(ExprError.UnknownFunction, pos);
        return 0.0;
    }
}

/// Evaluates an expression string with an empty variable environment.
pub fn evalExpr(source: string): EvalResult {
    const empty = ExprEnv.init();
    return evalExprWith(source, empty);
}

/// Evaluates an expression string against the given `ExprEnv` variable bindings.
pub fn evalExprWith(source: string, env: ExprEnv): EvalResult {
    let p = Parser.init(source, &env);
    if (p.hasError()) {
        return EvalResult.fail(p.err, p.err_pos);
    }
    if (p.tok.kind == TokenKind.Eof) {
        return EvalResult.fail(ExprError.SyntaxError, p.tok.start);
    }

    const val = p.parseExpr(true);
    if (p.hasError()) {
        return EvalResult.fail(p.err, p.err_pos);
    }
    if (p.tok.kind != TokenKind.Eof) {
        return EvalResult.fail(ExprError.UnexpectedToken, p.tok.start);
    }
    return EvalResult.success(val);
}

/// Evaluates a boolean predicate expression against `env`.
/// Returns `true` iff evaluation succeeds and the result is non-zero.
pub fn evalBool(source: string, env: ExprEnv): bool {
    const res = evalExprWith(source, env);
    return res.ok && isTruthy(res.value);
}
