// Source: cp/lib/expr/eval.cp (Precedence-Climbing Sandboxed Expression Evaluator)
// Pure CP zero-side-effect expression lexer, precedence-climbing evaluator,
// built-in pure functions, and depth/step-bounded sandbox guardrails for `cap:expr`.

const std = @import("std");
import { ExprError, ExprValue, ExprEnv } from "./value.cp";

pub const MAX_DEPTH: usize = 64;
pub const MAX_STEPS: usize = 10000;

enum TokenKind {
    Eof,
    Null,
    BoolTrue,
    BoolFalse,
    In,
    Int,
    Float,
    Str,
    Ident,
    LParen,
    RParen,
    LBracket,
    RBracket,
    Comma,
    Question,
    Colon,
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Bang,
    EqEq,
    BangEq,
    Lt,
    LtEq,
    Gt,
    GtEq,
    AndAnd,
    OrOr,
}

struct Token {
    kind: TokenKind = .Eof,
    lexeme: string = "",
    int_val: i64 = 0,
    float_val: f64 = 0.0,
}

enum OrdResult {
    Lt,
    Eq,
    Gt,
    Unordered,
}

fn isWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn hexValue(c: u8): u8 {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return c - 'A' + 10;
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isIdentCont(c: u8): bool {
    return isIdentStart(c) || isDigit(c);
}

fn opPrecedence(kind: TokenKind): u8 {
    return switch (kind) {
        .OrOr => 1,
        .AndAnd => 2,
        .EqEq, .BangEq => 3,
        .Lt, .LtEq, .Gt, .GtEq, .In => 4,
        .Plus, .Minus => 5,
        .Star, .Slash, .Percent => 6,
        else => 0,
    };
}

fn compareIntFloat(a: i64, b: f64): OrdResult {
    if (std.math.isNan(b)) return .Unordered;
    const pos_inf: f64 = std.math.inf(f64);
    const neg_inf: f64 = -pos_inf;
    if (b == pos_inf) return .Lt;
    if (b == neg_inf) return .Gt;
    if (b < -9223372036854775808.0) return .Gt;
    if (b >= 9223372036854775808.0) return .Lt;
    const trunc_b = @trunc(b);
    const bi: i64 = @intFromFloat(trunc_b);
    if (a < bi) return .Lt;
    if (a > bi) return .Gt;
    if (b > trunc_b) return .Lt;
    if (b < trunc_b) return .Gt;
    return .Eq;
}

fn compareOrdered(lhs: ExprValue, rhs: ExprValue): OrdResult! {
    return switch (lhs) {
        .Int a => switch (rhs) {
            .Int b => {
                if (a < b) return .Lt;
                if (a > b) return .Gt;
                return .Eq;
            },
            .Float b => compareIntFloat(a, b),
            else => return ExprError.TypeMismatch,
        },
        .Float a => switch (rhs) {
            .Float b => {
                if (std.math.isNan(a) || std.math.isNan(b)) return .Unordered;
                if (a < b) return .Lt;
                if (a > b) return .Gt;
                return .Eq;
            },
            .Int b => switch (compareIntFloat(b, a)) {
                .Lt => .Gt,
                .Gt => .Lt,
                .Eq => .Eq,
                .Unordered => .Unordered,
            },
            else => return ExprError.TypeMismatch,
        },
        .Str a => switch (rhs) {
            .Str b => switch (std.mem.order(u8, a, b)) {
                .lt => .Lt,
                .eq => .Eq,
                .gt => .Gt,
            },
            else => return ExprError.TypeMismatch,
        },
        else => return ExprError.TypeMismatch,
    };
}

struct Scanner {
    alloc: std.mem.Allocator,
    src: string,
    pos: usize = 0,

    fn peek(): u8? {
        if (_.pos >= _.src.len) return null;
        return _.src[_.pos];
    }

    fn peekAt(offset: usize): u8? {
        const idx = _.pos + offset;
        if (idx >= _.src.len) return null;
        return _.src[idx];
    }

    fn skipWs(*_) {
        while (_.pos < _.src.len && isWhitespace(_.src[_.pos])) {
            _.pos += 1;
        }
    }

    fn scanString(*_, quote: u8, active: bool): Token! {
        _.pos += 1; // consume opening quote
        const content_start = _.pos;
        let has_escape = false;
        let decoded_len: usize = 0;

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == quote) {
                const content_end = _.pos;
                _.pos += 1; // consume closing quote
                const raw = _.src[content_start..content_end];
                if (!active) {
                    return Token{ kind: .Str, lexeme: "" };
                }
                if (!has_escape) {
                    return Token{ kind: .Str, lexeme: raw };
                }
                let out = _.alloc.alloc(u8, decoded_len) catch return ExprError.OutOfMemory;
                let ri: usize = 0;
                let wi: usize = 0;
                while (ri < raw.len) {
                    if (raw[ri] == '\\') {
                        const esc = raw[ri + 1];
                        if (esc == '\\') {
                            out[wi] = '\\';
                            ri += 2;
                        } else if (esc == '"') {
                            out[wi] = '"';
                            ri += 2;
                        } else if (esc == '\'') {
                            out[wi] = '\'';
                            ri += 2;
                        } else if (esc == 'n') {
                            out[wi] = '\n';
                            ri += 2;
                        } else if (esc == 'r') {
                            out[wi] = '\r';
                            ri += 2;
                        } else if (esc == 't') {
                            out[wi] = '\t';
                            ri += 2;
                        } else if (esc == '0') {
                            out[wi] = 0;
                            ri += 2;
                        } else if (esc == 'x') {
                            const b = (hexValue(raw[ri + 2]) << 4) | hexValue(raw[ri + 3]);
                            out[wi] = b;
                            ri += 4;
                        } else {
                            return ExprError.SyntaxError;
                        }
                        wi += 1;
                    } else {
                        out[wi] = raw[ri];
                        ri += 1;
                        wi += 1;
                    }
                }
                return Token{ kind: .Str, lexeme: out[0..wi] };
            } else if (c == '\\') {
                has_escape = true;
                if (_.pos + 1 >= _.src.len) return ExprError.SyntaxError;
                const esc = _.src[_.pos + 1];
                if (esc == '\\' || esc == '"' || esc == '\'' || esc == 'n' || esc == 'r' || esc == 't' || esc == '0') {
                    _.pos += 2;
                    decoded_len += 1;
                } else if (esc == 'x') {
                    if (_.pos + 3 >= _.src.len) return ExprError.SyntaxError;
                    if (!isHexDigit(_.src[_.pos + 2]) || !isHexDigit(_.src[_.pos + 3])) {
                        return ExprError.SyntaxError;
                    }
                    _.pos += 4;
                    decoded_len += 1;
                } else {
                    return ExprError.SyntaxError;
                }
            } else {
                _.pos += 1;
                decoded_len += 1;
            }
        }
        return ExprError.SyntaxError;
    }

    fn scanNumber(*_): Token! {
        const start = _.pos;
        if (_.src[_.pos] == '0' && _.pos + 1 < _.src.len) {
            const p1 = _.src[_.pos + 1];
            if (p1 == 'x' || p1 == 'X') {
                _.pos += 2;
                const hex_start = _.pos;
                while (_.pos < _.src.len && isHexDigit(_.src[_.pos])) {
                    _.pos += 1;
                }
                if (_.pos == hex_start) return ExprError.SyntaxError;
                if (_.pos < _.src.len && (isIdentStart(_.src[_.pos]) || _.src[_.pos] == '.')) {
                    return ExprError.SyntaxError;
                }
                const v = std.fmt.parseInt(i64, _.src[hex_start.._.pos], 16) catch return ExprError.SyntaxError;
                return Token{ kind: .Int, lexeme: _.src[start.._.pos], int_val: v };
            } else if (p1 == 'b' || p1 == 'B') {
                _.pos += 2;
                const bin_start = _.pos;
                while (_.pos < _.src.len && (_.src[_.pos] == '0' || _.src[_.pos] == '1')) {
                    _.pos += 1;
                }
                if (_.pos == bin_start) return ExprError.SyntaxError;
                if (_.pos < _.src.len && (isIdentCont(_.src[_.pos]) || _.src[_.pos] == '.')) {
                    return ExprError.SyntaxError;
                }
                const v = std.fmt.parseInt(i64, _.src[bin_start.._.pos], 2) catch return ExprError.SyntaxError;
                return Token{ kind: .Int, lexeme: _.src[start.._.pos], int_val: v };
            } else if (p1 == 'o' || p1 == 'O') {
                _.pos += 2;
                const oct_start = _.pos;
                while (_.pos < _.src.len && _.src[_.pos] >= '0' && _.src[_.pos] <= '7') {
                    _.pos += 1;
                }
                if (_.pos == oct_start) return ExprError.SyntaxError;
                if (_.pos < _.src.len && (isIdentCont(_.src[_.pos]) || _.src[_.pos] == '.')) {
                    return ExprError.SyntaxError;
                }
                const v = std.fmt.parseInt(i64, _.src[oct_start.._.pos], 8) catch return ExprError.SyntaxError;
                return Token{ kind: .Int, lexeme: _.src[start.._.pos], int_val: v };
            }
        }

        while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
            _.pos += 1;
        }

        let is_float = false;
        if (_.pos + 1 < _.src.len && _.src[_.pos] == '.' && isDigit(_.src[_.pos + 1])) {
            is_float = true;
            _.pos += 1;
            while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                _.pos += 1;
            }
        } else if (_.pos < _.src.len && _.src[_.pos] == '.') {
            return ExprError.SyntaxError;
        }

        if (_.pos < _.src.len && (_.src[_.pos] == 'e' || _.src[_.pos] == 'E')) {
            is_float = true;
            _.pos += 1;
            if (_.pos < _.src.len && (_.src[_.pos] == '+' || _.src[_.pos] == '-')) {
                _.pos += 1;
            }
            const exp_start = _.pos;
            while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                _.pos += 1;
            }
            if (_.pos == exp_start) return ExprError.SyntaxError;
        }

        if (_.pos < _.src.len && isIdentStart(_.src[_.pos])) {
            return ExprError.SyntaxError;
        }

        const slice = _.src[start.._.pos];
        if (is_float) {
            const f = std.fmt.parseFloat(f64, slice) catch return ExprError.SyntaxError;
            return Token{ kind: .Float, lexeme: slice, float_val: f };
        }
        const n = std.fmt.parseInt(i64, slice, 10) catch return ExprError.SyntaxError;
        return Token{ kind: .Int, lexeme: slice, int_val: n };
    }

    fn scanIdent(*_): Token! {
        const start = _.pos;
        let has_dot = false;
        while (_.pos < _.src.len && isIdentCont(_.src[_.pos])) {
            _.pos += 1;
        }
        while (_.pos < _.src.len && _.src[_.pos] == '.') {
            has_dot = true;
            _.pos += 1;
            if (_.pos >= _.src.len || !isIdentStart(_.src[_.pos])) {
                return ExprError.SyntaxError;
            }
            while (_.pos < _.src.len && isIdentCont(_.src[_.pos])) {
                _.pos += 1;
            }
        }
        const slice = _.src[start.._.pos];
        if (!has_dot) {
            if (std.mem.eql(u8, slice, "null")) return Token{ kind: .Null, lexeme: slice };
            if (std.mem.eql(u8, slice, "true")) return Token{ kind: .BoolTrue, lexeme: slice };
            if (std.mem.eql(u8, slice, "false")) return Token{ kind: .BoolFalse, lexeme: slice };
            if (std.mem.eql(u8, slice, "in")) return Token{ kind: .In, lexeme: slice };
        }
        return Token{ kind: .Ident, lexeme: slice };
    }

    fn nextToken(*_, active: bool): Token! {
        _.skipWs();
        if (_.pos >= _.src.len) {
            return Token{ kind: .Eof };
        }
        const c = _.src[_.pos];
        if (c == '"' || c == '\'') {
            return _.scanString(c, active);
        }
        if (isDigit(c)) {
            return _.scanNumber();
        }
        if (isIdentStart(c)) {
            return _.scanIdent();
        }

        if (c == '(') { _.pos += 1; return Token{ kind: .LParen, lexeme: "(" }; }
        if (c == ')') { _.pos += 1; return Token{ kind: .RParen, lexeme: ")" }; }
        if (c == '[') { _.pos += 1; return Token{ kind: .LBracket, lexeme: "[" }; }
        if (c == ']') { _.pos += 1; return Token{ kind: .RBracket, lexeme: "]" }; }
        if (c == ',') { _.pos += 1; return Token{ kind: .Comma, lexeme: "," }; }
        if (c == '?') { _.pos += 1; return Token{ kind: .Question, lexeme: "?" }; }
        if (c == ':') { _.pos += 1; return Token{ kind: .Colon, lexeme: ":" }; }
        if (c == '+') { _.pos += 1; return Token{ kind: .Plus, lexeme: "+" }; }
        if (c == '-') { _.pos += 1; return Token{ kind: .Minus, lexeme: "-" }; }
        if (c == '*') { _.pos += 1; return Token{ kind: .Star, lexeme: "*" }; }
        if (c == '/') { _.pos += 1; return Token{ kind: .Slash, lexeme: "/" }; }
        if (c == '%') { _.pos += 1; return Token{ kind: .Percent, lexeme: "%" }; }

        if (c == '!') {
            if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '=') {
                _.pos += 2;
                return Token{ kind: .BangEq, lexeme: "!=" };
            }
            _.pos += 1;
            return Token{ kind: .Bang, lexeme: "!" };
        }
        if (c == '=') {
            if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '=') {
                _.pos += 2;
                return Token{ kind: .EqEq, lexeme: "==" };
            }
            return ExprError.SyntaxError;
        }
        if (c == '<') {
            if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '=') {
                _.pos += 2;
                return Token{ kind: .LtEq, lexeme: "<=" };
            }
            _.pos += 1;
            return Token{ kind: .Lt, lexeme: "<" };
        }
        if (c == '>') {
            if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '=') {
                _.pos += 2;
                return Token{ kind: .GtEq, lexeme: ">=" };
            }
            _.pos += 1;
            return Token{ kind: .Gt, lexeme: ">" };
        }
        if (c == '&') {
            if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '&') {
                _.pos += 2;
                return Token{ kind: .AndAnd, lexeme: "&&" };
            }
            return ExprError.SyntaxError;
        }
        if (c == '|') {
            if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '|') {
                _.pos += 2;
                return Token{ kind: .OrOr, lexeme: "||" };
            }
            return ExprError.SyntaxError;
        }
        return ExprError.SyntaxError;
    }
}

struct EvalEngine {
    alloc: std.mem.Allocator,
    scanner: Scanner,
    env: ExprEnv,
    current: Token = Token{},
    depth: usize = 0,
    steps: usize = 0,

    fn step(*_): ! {
        _.steps += 1;
        if (_.steps > MAX_STEPS) {
            return ExprError.StepLimitExceeded;
        }
    }

    fn enterDepth(*_): ! {
        _.depth += 1;
        if (_.depth > MAX_DEPTH) {
            return ExprError.MaxDepthExceeded;
        }
    }

    fn leaveDepth(*_) {
        if (_.depth > 0) {
            _.depth -= 1;
        }
    }

    fn advance(*_, active: bool): ! {
        _.current = _.scanner.nextToken(active)?;
    }

    fn evalRoot(*_): ExprValue! {
        _.advance(true)?;
        if (_.current.kind == .Eof) return ExprError.SyntaxError;
        const val = _.parseTernary(true)?;
        if (_.current.kind != .Eof) return ExprError.SyntaxError;
        return val;
    }

    fn parseTernary(*_, active: bool): ExprValue! {
        const cond = _.parseBinary(1, active)?;
        if (_.current.kind != .Question) {
            return cond;
        }
        _.step()?;
        _.enterDepth()?;
        const cond_truthy = if (active) cond.isTruthy() else false;
        const then_active = active && cond_truthy;
        const else_active = active && !cond_truthy;

        _.advance(then_active)?;
        const then_val = _.parseTernary(then_active)?;
        if (_.current.kind != .Colon) {
            _.leaveDepth();
            return ExprError.SyntaxError;
        }
        _.advance(else_active)?;
        const else_val = _.parseTernary(else_active)?;
        _.leaveDepth();

        if (!active) return .Null;
        return if (cond_truthy) then_val else else_val;
    }

    fn parseBinary(*_, min_prec: u8, active: bool): ExprValue! {
        let lhs = _.parseUnary(active)?;
        while (true) {
            const op = _.current.kind;
            const prec = opPrecedence(op);
            if (prec < min_prec) break;
            _.step()?;

            if (op == .AndAnd) {
                const lhs_truthy = if (active) lhs.isTruthy() else false;
                const rhs_active = active && lhs_truthy;
                _.advance(rhs_active)?;
                const rhs = _.parseBinary(prec + 1, rhs_active)?;
                if (active) {
                    lhs = ExprValue.Bool(lhs_truthy && rhs.isTruthy());
                }
            } else if (op == .OrOr) {
                const lhs_truthy = if (active) lhs.isTruthy() else false;
                const rhs_active = active && !lhs_truthy;
                _.advance(rhs_active)?;
                const rhs = _.parseBinary(prec + 1, rhs_active)?;
                if (active) {
                    lhs = ExprValue.Bool(lhs_truthy || rhs.isTruthy());
                }
            } else {
                _.advance(active)?;
                const rhs = _.parseBinary(prec + 1, active)?;
                if (active) {
                    lhs = _.applyBinary(op, lhs, rhs)?;
                }
            }
        }
        return lhs;
    }

    fn applyBinary(*_, op: TokenKind, lhs: ExprValue, rhs: ExprValue): ExprValue! {
        return switch (op) {
            .EqEq => ExprValue.Bool(lhs.eql(rhs)),
            .BangEq => ExprValue.Bool(!lhs.eql(rhs)),
            .Lt => {
                const ord = compareOrdered(lhs, rhs)?;
                return ExprValue.Bool(ord == .Lt);
            },
            .LtEq => {
                const ord = compareOrdered(lhs, rhs)?;
                return ExprValue.Bool(ord == .Lt || ord == .Eq);
            },
            .Gt => {
                const ord = compareOrdered(lhs, rhs)?;
                return ExprValue.Bool(ord == .Gt);
            },
            .GtEq => {
                const ord = compareOrdered(lhs, rhs)?;
                return ExprValue.Bool(ord == .Gt || ord == .Eq);
            },
            .In => _.applyIn(lhs, rhs),
            .Plus => _.applyPlus(lhs, rhs),
            .Minus => _.applyNumericBinary(op, lhs, rhs),
            .Star => _.applyNumericBinary(op, lhs, rhs),
            .Slash => _.applyNumericBinary(op, lhs, rhs),
            .Percent => _.applyNumericBinary(op, lhs, rhs),
            else => ExprError.SyntaxError,
        };
    }

    fn applyIn(*_, lhs: ExprValue, rhs: ExprValue): ExprValue! {
        return switch (rhs) {
            .List items => {
                for (item of items) {
                    if (lhs.eql(item)) return ExprValue.Bool(true);
                }
                return ExprValue.Bool(false);
            },
            .Str hay => switch (lhs) {
                .Str needle => ExprValue.Bool(std.mem.indexOf(u8, hay, needle) != null),
                else => ExprError.TypeMismatch,
            },
            else => ExprError.TypeMismatch,
        };
    }

    fn applyPlus(*_, lhs: ExprValue, rhs: ExprValue): ExprValue! {
        switch (lhs) {
            .Str a => switch (rhs) {
                .Str b => {
                    let out = _.alloc.alloc(u8, a.len + b.len) catch return ExprError.OutOfMemory;
                    @memcpy(out[0..a.len], a);
                    @memcpy(out[a.len..a.len + b.len], b);
                    return ExprValue.Str(out);
                },
                else => return ExprError.TypeMismatch,
            },
            else => return _.applyNumericBinary(.Plus, lhs, rhs),
        }
    }

    fn applyNumericBinary(*_, op: TokenKind, lhs: ExprValue, rhs: ExprValue): ExprValue! {
        switch (lhs) {
            .Int a => switch (rhs) {
                .Int b => {
                    return switch (op) {
                        .Plus => ExprValue.Int(std.math.add(i64, a, b) catch return ExprError.TypeMismatch),
                        .Minus => ExprValue.Int(std.math.sub(i64, a, b) catch return ExprError.TypeMismatch),
                        .Star => ExprValue.Int(std.math.mul(i64, a, b) catch return ExprError.TypeMismatch),
                        .Slash => {
                            if (b == 0) return ExprError.DivisionByZero;
                            if (a == std.math.minInt(i64) && b == -1) return ExprError.TypeMismatch;
                            return ExprValue.Int(@divTrunc(a, b));
                        },
                        .Percent => {
                            if (b == 0) return ExprError.DivisionByZero;
                            if (a == std.math.minInt(i64) && b == -1) return ExprError.TypeMismatch;
                            return ExprValue.Int(@rem(a, b));
                        },
                        else => ExprError.SyntaxError,
                    };
                },
                .Float b => return _.applyFloatBinary(op, @as(f64, @floatFromInt(a)), b),
                else => return ExprError.TypeMismatch,
            },
            .Float a => switch (rhs) {
                .Float b => return _.applyFloatBinary(op, a, b),
                .Int b => return _.applyFloatBinary(op, a, @as(f64, @floatFromInt(b))),
                else => return ExprError.TypeMismatch,
            },
            else => return ExprError.TypeMismatch,
        }
    }

    fn applyFloatBinary(*_, op: TokenKind, a: f64, b: f64): ExprValue! {
        return switch (op) {
            .Plus => ExprValue.Float(a + b),
            .Minus => ExprValue.Float(a - b),
            .Star => ExprValue.Float(a * b),
            .Slash => {
                if (b == 0.0) return ExprError.DivisionByZero;
                return ExprValue.Float(a / b);
            },
            .Percent => {
                if (b == 0.0) return ExprError.DivisionByZero;
                return ExprValue.Float(@rem(a, b));
            },
            else => ExprError.SyntaxError,
        };
    }

    fn parseUnary(*_, active: bool): ExprValue! {
        const tok_kind = _.current.kind;
        if (tok_kind == .Minus || tok_kind == .Bang) {
            _.step()?;
            _.enterDepth()?;
            _.advance(active)?;
            const inner = _.parseUnary(active)?;
            _.leaveDepth();
            if (!active) return .Null;
            if (tok_kind == .Bang) {
                return ExprValue.Bool(!inner.isTruthy());
            }
            return switch (inner) {
                .Int n => {
                    if (n == std.math.minInt(i64)) return ExprError.TypeMismatch;
                    return ExprValue.Int(-n);
                },
                .Float f => ExprValue.Float(-f),
                else => ExprError.TypeMismatch,
            };
        }
        return _.parsePrimary(active);
    }

    fn parsePrimary(*_, active: bool): ExprValue! {
        _.step()?;
        const tok = _.current;
        switch (tok.kind) {
            .Null => {
                _.advance(active)?;
                return .Null;
            },
            .BoolTrue => {
                _.advance(active)?;
                return ExprValue.Bool(true);
            },
            .BoolFalse => {
                _.advance(active)?;
                return ExprValue.Bool(false);
            },
            .Int => {
                _.advance(active)?;
                return ExprValue.Int(tok.int_val);
            },
            .Float => {
                _.advance(active)?;
                return ExprValue.Float(tok.float_val);
            },
            .Str => {
                _.advance(active)?;
                return ExprValue.Str(tok.lexeme);
            },
            .LParen => {
                _.enterDepth()?;
                _.advance(active)?;
                const inner = _.parseTernary(active)?;
                if (_.current.kind != .RParen) {
                    _.leaveDepth();
                    return ExprError.SyntaxError;
                }
                _.advance(active)?;
                _.leaveDepth();
                return inner;
            },
            .LBracket => return _.parseListLiteral(active),
            .Ident => {
                const name = tok.lexeme;
                _.advance(active)?;
                if (_.current.kind == .LParen) {
                    return _.parseFunctionCall(name, active);
                }
                if (!active) return .Null;
                if (val := _.env.get(name)) {
                    return val;
                }
                return ExprError.UnknownIdentifier;
            },
            else => return ExprError.SyntaxError,
        }
    }

    fn parseListLiteral(*_, active: bool): ExprValue! {
        _.enterDepth()?;
        _.advance(active)?;
        let list = std.ArrayListUnmanaged(ExprValue).empty;
        if (_.current.kind == .RBracket) {
            _.advance(active)?;
            _.leaveDepth();
            if (!active) return .Null;
            const empty_slice = list.toOwnedSlice(_.alloc) catch return ExprError.OutOfMemory;
            return ExprValue.List(empty_slice);
        }
        while (true) {
            const elem = _.parseTernary(active)?;
            if (active) {
                list.append(_.alloc, elem) catch return ExprError.OutOfMemory;
            }
            if (_.current.kind == .Comma) {
                _.advance(active)?;
                if (_.current.kind == .RBracket) break;
                continue;
            }
            break;
        }
        if (_.current.kind != .RBracket) {
            _.leaveDepth();
            return ExprError.SyntaxError;
        }
        _.advance(active)?;
        _.leaveDepth();
        if (!active) return .Null;
        const owned = list.toOwnedSlice(_.alloc) catch return ExprError.OutOfMemory;
        return ExprValue.List(owned);
    }

    fn parseFunctionCall(*_, name: string, active: bool): ExprValue! {
        _.enterDepth()?;
        _.advance(active)?; // consume '('
        let args = std.ArrayListUnmanaged(ExprValue).empty;
        if (_.current.kind != .RParen) {
            while (true) {
                const arg_val = _.parseTernary(active)?;
                if (active) {
                    args.append(_.alloc, arg_val) catch return ExprError.OutOfMemory;
                }
                if (_.current.kind == .Comma) {
                    _.advance(active)?;
                    continue;
                }
                break;
            }
        }
        if (_.current.kind != .RParen) {
            _.leaveDepth();
            return ExprError.SyntaxError;
        }
        _.advance(active)?; // consume ')'
        _.leaveDepth();

        if (!active) return .Null;
        return _.callBuiltin(name, args.items);
    }

    fn callBuiltin(*_, name: string, args: []ExprValue): ExprValue! {
        if (std.mem.eql(u8, name, "len")) {
            if (args.len != 1) return ExprError.InvalidArity;
            return switch (args[0]) {
                .Str s => ExprValue.Int(s.len as i64),
                .List items => ExprValue.Int(items.len as i64),
                else => ExprError.TypeMismatch,
            };
        }
        if (std.mem.eql(u8, name, "contains")) {
            if (args.len != 2) return ExprError.InvalidArity;
            return switch (args[0]) {
                .Str hay => switch (args[1]) {
                    .Str needle => ExprValue.Bool(std.mem.indexOf(u8, hay, needle) != null),
                    else => ExprError.TypeMismatch,
                },
                .List items => {
                    for (item of items) {
                        if (item.eql(args[1])) return ExprValue.Bool(true);
                    }
                    return ExprValue.Bool(false);
                },
                else => ExprError.TypeMismatch,
            };
        }
        if (std.mem.eql(u8, name, "startsWith")) {
            if (args.len != 2) return ExprError.InvalidArity;
            const s = args[0].asStr() ?? return ExprError.TypeMismatch;
            const pre = args[1].asStr() ?? return ExprError.TypeMismatch;
            return ExprValue.Bool(std.mem.startsWith(u8, s, pre));
        }
        if (std.mem.eql(u8, name, "endsWith")) {
            if (args.len != 2) return ExprError.InvalidArity;
            const s = args[0].asStr() ?? return ExprError.TypeMismatch;
            const suf = args[1].asStr() ?? return ExprError.TypeMismatch;
            return ExprValue.Bool(std.mem.endsWith(u8, s, suf));
        }
        if (std.mem.eql(u8, name, "lower")) {
            if (args.len != 1) return ExprError.InvalidArity;
            const s = args[0].asStr() ?? return ExprError.TypeMismatch;
            let out = _.alloc.alloc(u8, s.len) catch return ExprError.OutOfMemory;
            let i: usize = 0;
            while (i < s.len) : (i += 1) {
                const ch = s[i];
                out[i] = if (ch >= 'A' && ch <= 'Z') ch + 32 else ch;
            }
            return ExprValue.Str(out);
        }
        if (std.mem.eql(u8, name, "upper")) {
            if (args.len != 1) return ExprError.InvalidArity;
            const s = args[0].asStr() ?? return ExprError.TypeMismatch;
            let out = _.alloc.alloc(u8, s.len) catch return ExprError.OutOfMemory;
            let i: usize = 0;
            while (i < s.len) : (i += 1) {
                const ch = s[i];
                out[i] = if (ch >= 'a' && ch <= 'z') ch - 32 else ch;
            }
            return ExprValue.Str(out);
        }
        if (std.mem.eql(u8, name, "abs")) {
            if (args.len != 1) return ExprError.InvalidArity;
            return switch (args[0]) {
                .Int n => {
                    if (n == std.math.minInt(i64)) return ExprError.TypeMismatch;
                    return ExprValue.Int(if (n < 0) -n else n);
                },
                .Float f => ExprValue.Float(@abs(f)),
                else => ExprError.TypeMismatch,
            };
        }
        if (std.mem.eql(u8, name, "min")) {
            if (args.len != 2) return ExprError.InvalidArity;
            return _.callMinMax(args[0], args[1], true);
        }
        if (std.mem.eql(u8, name, "max")) {
            if (args.len != 2) return ExprError.InvalidArity;
            return _.callMinMax(args[0], args[1], false);
        }
        return ExprError.UnknownFunction;
    }

    fn callMinMax(*_, a: ExprValue, b: ExprValue, is_min: bool): ExprValue! {
        switch (a) {
            .Int ia => switch (b) {
                .Int ib => {
                    if (is_min) return ExprValue.Int(if (ia <= ib) ia else ib);
                    return ExprValue.Int(if (ia >= ib) ia else ib);
                },
                .Float fb => {
                    if (std.math.isNan(fb)) return ExprValue.Float(fb);
                    const ord = compareIntFloat(ia, fb);
                    if (is_min) {
                        return if (ord != .Gt) ExprValue.Float(@as(f64, @floatFromInt(ia))) else ExprValue.Float(fb);
                    }
                    return if (ord != .Lt) ExprValue.Float(@as(f64, @floatFromInt(ia))) else ExprValue.Float(fb);
                },
                else => return ExprError.TypeMismatch,
            },
            .Float fa => switch (b) {
                .Float fb => {
                    if (std.math.isNan(fa)) return ExprValue.Float(fa);
                    if (std.math.isNan(fb)) return ExprValue.Float(fb);
                    if (is_min) return ExprValue.Float(if (fa <= fb) fa else fb);
                    return ExprValue.Float(if (fa >= fb) fa else fb);
                },
                .Int ib => {
                    if (std.math.isNan(fa)) return ExprValue.Float(fa);
                    const ord = compareIntFloat(ib, fa);
                    if (is_min) {
                        return if (ord == .Gt) ExprValue.Float(fa) else ExprValue.Float(@as(f64, @floatFromInt(ib)));
                    }
                    return if (ord == .Lt) ExprValue.Float(fa) else ExprValue.Float(@as(f64, @floatFromInt(ib)));
                },
                else => return ExprError.TypeMismatch,
            },
            .Str sa => switch (b) {
                .Str sb => {
                    const order = std.mem.order(u8, sa, sb);
                    if (is_min) return ExprValue.Str(if (order != .gt) sa else sb);
                    return ExprValue.Str(if (order != .lt) sa else sb);
                },
                else => return ExprError.TypeMismatch,
            },
            else => return ExprError.TypeMismatch,
        }
    }
}

/// Precedence-climbing sandboxed expression evaluator.
/// Zero side-effects, depth-bounded (`MAX_DEPTH = 64`), and step-bounded (`MAX_STEPS = 10000`).
pub static class ExprEvaluator {
    pub const MAX_DEPTH: usize = 64;
    pub const MAX_STEPS: usize = 10000;

    /// Evaluates `source` against read-only `env`, allocating any produced strings/lists from `alloc`.
    pub fn eval(alloc: std.mem.Allocator, source: string, env: ExprEnv): ExprValue! {
        let engine = EvalEngine{
            alloc: alloc,
            scanner: Scanner{ alloc: alloc, src: source },
            env: env,
        };
        return engine.evalRoot();
    }

    /// Evaluates `source` as a boolean using `ExprValue.isTruthy()` inside a temporary arena
    /// so intermediate allocations are immediately reclaimed on return.
    pub fn evalBool(alloc: std.mem.Allocator, source: string, env: ExprEnv): bool! {
        let tmp_arena = std.heap.ArenaAllocator.init(alloc);
        defer tmp_arena.deinit();
        const val = ExprEvaluator.eval(tmp_arena.allocator(), source, env)?;
        return val.isTruthy();
    }
}
