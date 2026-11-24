// Reference-hardened against jix/varisat (MIT OR Apache-2.0)
// Source: cp/lib/expr/expr.cp (Safe Deterministic Expression & Formula Evaluator + 16-Lane SIMD)
// Pure CP zero-side-effect expression lexer, recursive-descent parser, AST,
// built-in pure functions, right-associative power (**), NaN/±Inf guards, and depth-bounded evaluator.

const std = @import("std");

type ExprSatVec16u8 = @Vector(16, u8);

pub const DEFAULT_MAX_DEPTH: usize = 64;
pub const EXPR_MAX_DEPTH: usize = 128;

/// Error set returned by expression compilation and evaluation.
pub err ExprError {
    UnexpectedToken,
    UnexpectedEof,
    DivisionByZero,
    NonFiniteResult,
    UndefinedVariable,
    UnknownFunction,
    InvalidArity,
    TypeMismatch,
    MaxDepthExceeded,
    OutOfMemory,
}

/// Tagged dynamic value produced and consumed by the expression engine.
pub enum ExprValue {
    number: f64,
    boolean: bool,
    string: []const u8,
    null_val,

    pub static fn makeNumber(v: f64): ExprValue {
        return ExprValue{ .number = v };
    }

    pub static fn makeBool(v: bool): ExprValue {
        return ExprValue{ .boolean = v };
    }

    pub static fn makeString(v: []const u8): ExprValue {
        return ExprValue{ .string = v };
    }

    pub static fn makeNull(): ExprValue {
        return .null_val;
    }

    pub fn isNull(): bool {
        return switch (_) {
            .null_val => true,
            else => false,
        };
    }

    pub fn asNumber(): f64? {
        return switch (_) {
            .number n => n,
            else => null,
        };
    }

    pub fn asBool(): bool? {
        return switch (_) {
            .boolean b => b,
            else => null,
        };
    }

    pub fn asString(): ([]const u8)? {
        return switch (_) {
            .string s => s,
            else => null,
        };
    }

    pub fn eql(other: ExprValue): bool {
        return switch (_) {
            .null_val => switch (other) {
                .null_val => true,
                else => false,
            },
            .boolean a => switch (other) {
                .boolean b => a == b,
                else => false,
            },
            .number a => switch (other) {
                .number b => a == b,
                else => false,
            },
            .string a => switch (other) {
                .string b => std.mem.eql(u8, a, b),
                else => false,
            },
        };
    }
}

/// Variable binding environment for expression evaluation.
pub struct ExprContext {
    allocator: std.mem.Allocator,
    mem_arena: std.heap.ArenaAllocator,
    vars: std.StringHashMapUnmanaged(ExprValue),

    pub static fn init(allocator: std.mem.Allocator): ExprContext {
        return ExprContext{
            allocator: allocator,
            mem_arena: std.heap.ArenaAllocator.init(allocator),
            vars: .{},
        };
    }

    pub fn deinit(*_) {
        _.vars.deinit(_.allocator);
        _.mem_arena.deinit();
    }

    fn setEntry(*_, name: []const u8, val: ExprValue): !void {
        const a = _.mem_arena.allocator();
        if (ptr := _.vars.getPtr(name)) {
            ptr.* = val;
            return;
        }
        const key_copy = a.dupe(u8, name) catch return ExprError.OutOfMemory;
        _.vars.put(_.allocator, key_copy, val) catch return ExprError.OutOfMemory;
    }

    pub fn setNumber(*_, name: []const u8, v: f64): !void {
        return _.setEntry(name, ExprValue.number(v));
    }

    pub fn setBool(*_, name: []const u8, v: bool): !void {
        return _.setEntry(name, ExprValue.boolean(v));
    }

    pub fn setString(*_, name: []const u8, v: []const u8): !void {
        const a = _.mem_arena.allocator();
        const val_copy = a.dupe(u8, v) catch return ExprError.OutOfMemory;
        return _.setEntry(name, ExprValue.string(val_copy));
    }

    pub fn setNull(*_, name: []const u8): !void {
        return _.setEntry(name, .null_val);
    }

    pub fn get(name: []const u8): ExprValue? {
        return _.vars.get(name);
    }
}

enum TokenKind {
    Eof,
    Null,
    BoolTrue,
    BoolFalse,
    Number,
    Str,
    Ident,
    LParen,
    RParen,
    Comma,
    Question,
    Colon,
    Plus,
    Minus,
    Star,
    StarStar,
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
    lexeme: []const u8 = "",
    num_val: f64 = 0.0,
}

enum UnaryOp {
    Neg,
    Not,
}

enum BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Pow,
    Lt,
    LtEq,
    Gt,
    GtEq,
    Eq,
    Neq,
    And,
    Or,
}

struct UnaryNode {
    op: UnaryOp,
    operand: usize,
}

struct BinaryNode {
    op: BinaryOp,
    left: usize,
    right: usize,
}

struct TernaryNode {
    cond: usize,
    then_br: usize,
    else_br: usize,
}

struct CallNode {
    name: []const u8,
    args: []usize,
}

enum AstNode {
    lit_number: f64,
    lit_boolean: bool,
    lit_string: []const u8,
    lit_null: void,
    ident: []const u8,
    unary: UnaryNode,
    binary: BinaryNode,
    ternary: TernaryNode,
    call: CallNode,
}

fn exprLoadVec16(data: []const u8, off: usize): ExprSatVec16u8 {
    const v: ExprSatVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

fn exprIsWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/// 16-lane `@Vector(16, u8)` SIMD whitespace skipper (`' '`, `'\t'`, `'\n'`, `'\r'`).
/// Hot path for `expr.cp` lexer tokenization.
pub fn simdExprSkipWhitespace(src: []const u8, start: usize): usize {
    let pos: usize = if (start > src.len) src.len else start;
    const v_sp: ExprSatVec16u8 = @splat(0x20);
    const v_tb: ExprSatVec16u8 = @splat(0x09);
    const v_lf: ExprSatVec16u8 = @splat(0x0A);
    const v_cr: ExprSatVec16u8 = @splat(0x0D);
    const v_one: ExprSatVec16u8 = @splat(1);
    const v_zero: ExprSatVec16u8 = @splat(0);
    while (pos + 16 <= src.len) {
        const v = exprLoadVec16(src, pos);
        const m_sp = (v == v_sp);
        const m_tb = (v == v_tb);
        const m_lf = (v == v_lf);
        const m_cr = (v == v_cr);
        const is_ws = @select(u8, m_sp, v_one, v_zero)
            | @select(u8, m_tb, v_one, v_zero)
            | @select(u8, m_lf, v_one, v_zero)
            | @select(u8, m_cr, v_one, v_zero);
        if (@reduce(.Min, is_ws) == 1) {
            pos += 16;
        } else {
            let j: usize = 0;
            while (j < 16) : (j += 1) {
                if (!exprIsWhitespace(src[pos + j])) {
                    return pos + j;
                }
            }
            pos += 16;
        }
    }
    while (pos < src.len && exprIsWhitespace(src[pos])) : (pos += 1) {}
    return pos;
}

/// Scalar reference for `simdExprSkipWhitespace` (differential verification).
pub fn scalarExprSkipWhitespace(src: []const u8, start: usize): usize {
    let pos: usize = if (start > src.len) src.len else start;
    while (pos < src.len && exprIsWhitespace(src[pos])) : (pos += 1) {}
    return pos;
}

fn exprCheckFinite(v: f64): !f64 {
    if (std.math.isNan(v) || std.math.isInf(v)) {
        return ExprError.NonFiniteResult;
    }
    return v;
}

fn exprIsDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn exprIsHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn exprHexVal(c: u8): u8 {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return c - 'A' + 10;
}

fn exprIsIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn exprIsIdentCont(c: u8): bool {
    return exprIsIdentStart(c) || exprIsDigit(c);
}

struct Lexer {
    src: []const u8,
    pos: usize = 0,
    mem_arena: std.mem.Allocator,

    fn skipWs(*_) {
        _.pos = simdExprSkipWhitespace(_.src, _.pos);
    }

    fn nextToken(*_): !Token {
        _.skipWs();
        if (_.pos >= _.src.len) {
            return Token{ kind: .Eof };
        }

        const start = _.pos;
        const c = _.src[_.pos];

        // Two-character operators
        if (_.pos + 1 < _.src.len) {
            const c2 = _.src[_.pos + 1];
            if (c == '=' && c2 == '=') {
                _.pos += 2;
                return Token{ kind: .EqEq, lexeme: _.src[start.._.pos] };
            }
            if (c == '!' && c2 == '=') {
                _.pos += 2;
                return Token{ kind: .BangEq, lexeme: _.src[start.._.pos] };
            }
            if (c == '<' && c2 == '=') {
                _.pos += 2;
                return Token{ kind: .LtEq, lexeme: _.src[start.._.pos] };
            }
            if (c == '>' && c2 == '=') {
                _.pos += 2;
                return Token{ kind: .GtEq, lexeme: _.src[start.._.pos] };
            }
            if (c == '&' && c2 == '&') {
                _.pos += 2;
                return Token{ kind: .AndAnd, lexeme: _.src[start.._.pos] };
            }
            if (c == '|' && c2 == '|') {
                _.pos += 2;
                return Token{ kind: .OrOr, lexeme: _.src[start.._.pos] };
            }
            if (c == '*' && c2 == '*') {
                _.pos += 2;
                return Token{ kind: .StarStar, lexeme: _.src[start.._.pos] };
            }
        }

        // Single-character punctuation & operators
        switch (c) {
            '(' => { _.pos += 1; return Token{ kind: .LParen, lexeme: _.src[start.._.pos] }; },
            ')' => { _.pos += 1; return Token{ kind: .RParen, lexeme: _.src[start.._.pos] }; },
            ',' => { _.pos += 1; return Token{ kind: .Comma, lexeme: _.src[start.._.pos] }; },
            '?' => { _.pos += 1; return Token{ kind: .Question, lexeme: _.src[start.._.pos] }; },
            ':' => { _.pos += 1; return Token{ kind: .Colon, lexeme: _.src[start.._.pos] }; },
            '+' => { _.pos += 1; return Token{ kind: .Plus, lexeme: _.src[start.._.pos] }; },
            '-' => { _.pos += 1; return Token{ kind: .Minus, lexeme: _.src[start.._.pos] }; },
            '*' => { _.pos += 1; return Token{ kind: .Star, lexeme: _.src[start.._.pos] }; },
            '/' => { _.pos += 1; return Token{ kind: .Slash, lexeme: _.src[start.._.pos] }; },
            '%' => { _.pos += 1; return Token{ kind: .Percent, lexeme: _.src[start.._.pos] }; },
            '!' => { _.pos += 1; return Token{ kind: .Bang, lexeme: _.src[start.._.pos] }; },
            '<' => { _.pos += 1; return Token{ kind: .Lt, lexeme: _.src[start.._.pos] }; },
            '>' => { _.pos += 1; return Token{ kind: .Gt, lexeme: _.src[start.._.pos] }; },
            else => {},
        }

        // String literal ('...' or "...")
        if (c == '"' || c == '\'') {
            return _.scanString(c);
        }

        // Number literal (integer, float, scientific notation)
        if (exprIsDigit(c)) {
            return _.scanNumber();
        }

        // Identifier / keyword / dotted identifier (e.g. user.tier)
        if (exprIsIdentStart(c)) {
            return _.scanIdent();
        }

        return ExprError.UnexpectedToken;
    }

    fn scanString(*_, quote: u8): !Token {
        _.pos += 1; // skip opening quote
        const content_start = _.pos;
        let has_escape = false;
        while (_.pos < _.src.len && _.src[_.pos] != quote) {
            if (_.src[_.pos] == '\\') {
                has_escape = true;
                _.pos += 1;
                if (_.pos >= _.src.len) return ExprError.UnexpectedEof;
            }
            _.pos += 1;
        }
        if (_.pos >= _.src.len) return ExprError.UnexpectedEof;
        const raw = _.src[content_start.._.pos];
        _.pos += 1; // skip closing quote

        if (!has_escape) {
            return Token{ kind: .Str, lexeme: raw };
        }

        let buf: std.ArrayListUnmanaged(u8) = .empty;
        let i: usize = 0;
        while (i < raw.len) {
            if (raw[i] == '\\') {
                i += 1;
                if (i >= raw.len) return ExprError.UnexpectedEof;
                const esc = raw[i];
                switch (esc) {
                    'n' => buf.append(_.mem_arena, '\n') catch return ExprError.OutOfMemory,
                    'r' => buf.append(_.mem_arena, '\r') catch return ExprError.OutOfMemory,
                    't' => buf.append(_.mem_arena, '\t') catch return ExprError.OutOfMemory,
                    '\\' => buf.append(_.mem_arena, '\\') catch return ExprError.OutOfMemory,
                    '"' => buf.append(_.mem_arena, '"') catch return ExprError.OutOfMemory,
                    '\'' => buf.append(_.mem_arena, '\'') catch return ExprError.OutOfMemory,
                    '0' => buf.append(_.mem_arena, 0) catch return ExprError.OutOfMemory,
                    'x' => {
                        if (i + 2 >= raw.len || !exprIsHexDigit(raw[i + 1]) || !exprIsHexDigit(raw[i + 2])) {
                            return ExprError.UnexpectedToken;
                        }
                        const byte_val: u8 = (exprHexVal(raw[i + 1]) << 4) | exprHexVal(raw[i + 2]);
                        buf.append(_.mem_arena, byte_val) catch return ExprError.OutOfMemory;
                        i += 2;
                    },
                    else => return ExprError.UnexpectedToken,
                }
                i += 1;
            } else {
                buf.append(_.mem_arena, raw[i]) catch return ExprError.OutOfMemory;
                i += 1;
            }
        }
        const slice = buf.toOwnedSlice(_.mem_arena) catch return ExprError.OutOfMemory;
        return Token{ kind: .Str, lexeme: slice };
    }

    fn scanNumber(*_): !Token {
        const start = _.pos;
        while (_.pos < _.src.len && exprIsDigit(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos < _.src.len && _.src[_.pos] == '.') {
            if (_.pos + 1 >= _.src.len || !exprIsDigit(_.src[_.pos + 1])) {
                return ExprError.UnexpectedToken;
            }
            _.pos += 1;
            while (_.pos < _.src.len && exprIsDigit(_.src[_.pos])) {
                _.pos += 1;
            }
        }
        if (_.pos < _.src.len && (_.src[_.pos] == 'e' || _.src[_.pos] == 'E')) {
            _.pos += 1;
            if (_.pos < _.src.len && (_.src[_.pos] == '+' || _.src[_.pos] == '-')) {
                _.pos += 1;
            }
            if (_.pos >= _.src.len || !exprIsDigit(_.src[_.pos])) {
                return ExprError.UnexpectedToken;
            }
            while (_.pos < _.src.len && exprIsDigit(_.src[_.pos])) {
                _.pos += 1;
            }
        }
        if (_.pos < _.src.len && exprIsIdentStart(_.src[_.pos])) {
            return ExprError.UnexpectedToken;
        }
        const text = _.src[start.._.pos];
        const parsed = std.fmt.parseFloat(f64, text) catch return ExprError.UnexpectedToken;
        const checked = exprCheckFinite(parsed)?;
        return Token{ kind: .Number, lexeme: text, num_val: checked };
    }

    fn scanIdent(*_): !Token {
        const start = _.pos;
        while (_.pos < _.src.len && exprIsIdentCont(_.src[_.pos])) {
            _.pos += 1;
        }
        // Support dotted property identifiers like user.tier
        while (_.pos < _.src.len && _.src[_.pos] == '.') {
            _.pos += 1;
            if (_.pos >= _.src.len) return ExprError.UnexpectedEof;
            if (!exprIsIdentStart(_.src[_.pos])) return ExprError.UnexpectedToken;
            while (_.pos < _.src.len && exprIsIdentCont(_.src[_.pos])) {
                _.pos += 1;
            }
        }
        const text = _.src[start.._.pos];
        if (std.mem.eql(u8, text, "true")) return Token{ kind: .BoolTrue, lexeme: text };
        if (std.mem.eql(u8, text, "false")) return Token{ kind: .BoolFalse, lexeme: text };
        if (std.mem.eql(u8, text, "null")) return Token{ kind: .Null, lexeme: text };
        return Token{ kind: .Ident, lexeme: text };
    }
}

struct Parser {
    lexer: Lexer,
    cur: Token,
    mem_arena: std.mem.Allocator,
    nodes: std.ArrayListUnmanaged(AstNode),
    max_depth: usize,

    fn advance(*_): !void {
        _.cur = _.lexer.nextToken()?;
    }

    fn addNode(*_, node: AstNode): !usize {
        const idx = _.nodes.items.len;
        _.nodes.append(_.mem_arena, node) catch return ExprError.OutOfMemory;
        return idx;
    }

    fn checkDepth(*_, depth: usize): !void {
        if (depth > _.max_depth) {
            return ExprError.MaxDepthExceeded;
        }
    }

    fn parseExpr(*_, depth: usize): !usize {
        return _.parseTernary(depth);
    }

    fn parseTernary(*_, depth: usize): !usize {
        _.checkDepth(depth)?;
        const cond = _.parseOr(depth)?;
        if (_.cur.kind == .Question) {
            _.advance()?;
            const then_br = _.parseTernary(depth + 1)?;
            if (_.cur.kind == .Eof) return ExprError.UnexpectedEof;
            if (_.cur.kind != .Colon) return ExprError.UnexpectedToken;
            _.advance()?;
            const else_br = _.parseTernary(depth + 1)?;
            return _.addNode(AstNode.ternary(TernaryNode{
                cond: cond,
                then_br: then_br,
                else_br: else_br,
            }));
        }
        return cond;
    }

    fn parseOr(*_, depth: usize): !usize {
        let left = _.parseAnd(depth)?;
        while (_.cur.kind == .OrOr) {
            _.advance()?;
            const right = _.parseAnd(depth)?;
            left = _.addNode(AstNode.binary(BinaryNode{
                op: .Or,
                left: left,
                right: right,
            }))?;
        }
        return left;
    }

    fn parseAnd(*_, depth: usize): !usize {
        let left = _.parseEquality(depth)?;
        while (_.cur.kind == .AndAnd) {
            _.advance()?;
            const right = _.parseEquality(depth)?;
            left = _.addNode(AstNode.binary(BinaryNode{
                op: .And,
                left: left,
                right: right,
            }))?;
        }
        return left;
    }

    fn parseEquality(*_, depth: usize): !usize {
        let left = _.parseRelational(depth)?;
        while (_.cur.kind == .EqEq || _.cur.kind == .BangEq) {
            const op: BinaryOp = if (_.cur.kind == .EqEq) .Eq else .Neq;
            _.advance()?;
            const right = _.parseRelational(depth)?;
            left = _.addNode(AstNode.binary(BinaryNode{
                op: op,
                left: left,
                right: right,
            }))?;
        }
        return left;
    }

    fn parseRelational(*_, depth: usize): !usize {
        let left = _.parseAdditive(depth)?;
        while (_.cur.kind == .Lt || _.cur.kind == .LtEq || _.cur.kind == .Gt || _.cur.kind == .GtEq) {
            const op: BinaryOp = switch (_.cur.kind) {
                .Lt => .Lt,
                .LtEq => .LtEq,
                .Gt => .Gt,
                .GtEq => .GtEq,
                else => .Lt,
            };
            _.advance()?;
            const right = _.parseAdditive(depth)?;
            left = _.addNode(AstNode.binary(BinaryNode{
                op: op,
                left: left,
                right: right,
            }))?;
        }
        return left;
    }

    fn parseAdditive(*_, depth: usize): !usize {
        let left = _.parseMultiplicative(depth)?;
        while (_.cur.kind == .Plus || _.cur.kind == .Minus) {
            const op: BinaryOp = if (_.cur.kind == .Plus) .Add else .Sub;
            _.advance()?;
            const right = _.parseMultiplicative(depth)?;
            left = _.addNode(AstNode.binary(BinaryNode{
                op: op,
                left: left,
                right: right,
            }))?;
        }
        return left;
    }

    fn parseMultiplicative(*_, depth: usize): !usize {
        let left = _.parsePower(depth)?;
        while (_.cur.kind == .Star || _.cur.kind == .Slash || _.cur.kind == .Percent) {
            const op: BinaryOp = switch (_.cur.kind) {
                .Star => .Mul,
                .Slash => .Div,
                .Percent => .Mod,
                else => .Mul,
            };
            _.advance()?;
            const right = _.parsePower(depth)?;
            left = _.addNode(AstNode.binary(BinaryNode{
                op: op,
                left: left,
                right: right,
            }))?;
        }
        return left;
    }

    /// Right-associative exponentiation (`a ** b ** c == a ** (b ** c)`), bounded by `max_depth`.
    fn parsePower(*_, depth: usize): !usize {
        _.checkDepth(depth)?;
        const base = _.parseUnary(depth)?;
        if (_.cur.kind == .StarStar) {
            _.advance()?;
            const exp = _.parsePower(depth + 1)?;
            return _.addNode(AstNode.binary(BinaryNode{
                op: .Pow,
                left: base,
                right: exp,
            }));
        }
        return base;
    }

    fn parseUnary(*_, depth: usize): !usize {
        _.checkDepth(depth)?;
        if (_.cur.kind == .Bang) {
            _.advance()?;
            const operand = _.parseUnary(depth + 1)?;
            return _.addNode(AstNode.unary(UnaryNode{ op: .Not, operand: operand }));
        }
        if (_.cur.kind == .Minus) {
            _.advance()?;
            const operand = _.parseUnary(depth + 1)?;
            return _.addNode(AstNode.unary(UnaryNode{ op: .Neg, operand: operand }));
        }
        return _.parsePrimary(depth);
    }

    fn parsePrimary(*_, depth: usize): !usize {
        _.checkDepth(depth)?;
        switch (_.cur.kind) {
            .Eof => return ExprError.UnexpectedEof,
            .Null => {
                _.advance()?;
                return _.addNode(.lit_null);
            },
            .BoolTrue => {
                _.advance()?;
                return _.addNode(AstNode.lit_boolean(true));
            },
            .BoolFalse => {
                _.advance()?;
                return _.addNode(AstNode.lit_boolean(false));
            },
            .Number => {
                const v = _.cur.num_val;
                _.advance()?;
                return _.addNode(AstNode.lit_number(v));
            },
            .Str => {
                const s = _.cur.lexeme;
                _.advance()?;
                return _.addNode(AstNode.lit_string(s));
            },
            .LParen => {
                _.advance()?;
                const inner = _.parseExpr(depth + 1)?;
                if (_.cur.kind == .Eof) return ExprError.UnexpectedEof;
                if (_.cur.kind != .RParen) return ExprError.UnexpectedToken;
                _.advance()?;
                return inner;
            },
            .Ident => {
                const name = _.cur.lexeme;
                _.advance()?;
                if (_.cur.kind == .LParen) {
                    _.advance()?;
                    let args: std.ArrayListUnmanaged(usize) = .empty;
                    if (_.cur.kind != .RParen) {
                        while (true) {
                            const arg_idx = _.parseExpr(depth + 1)?;
                            args.append(_.mem_arena, arg_idx) catch return ExprError.OutOfMemory;
                            if (_.cur.kind == .Comma) {
                                _.advance()?;
                            } else {
                                break;
                            }
                        }
                    }
                    if (_.cur.kind == .Eof) return ExprError.UnexpectedEof;
                    if (_.cur.kind != .RParen) return ExprError.UnexpectedToken;
                    _.advance()?;
                    const arg_slice = args.toOwnedSlice(_.mem_arena) catch return ExprError.OutOfMemory;
                    return _.addNode(AstNode.call(CallNode{ name: name, args: arg_slice }));
                }
                return _.addNode(AstNode.ident(name));
            },
            else => return ExprError.UnexpectedToken,
        }
    }
}

/// Compiled expression AST with arena-owned nodes and literal strings.
pub struct CompiledExpr {
    allocator: std.mem.Allocator,
    mem_arena: std.heap.ArenaAllocator,
    nodes: []AstNode,
    root: usize,
    max_depth: usize,

    pub fn deinit(*_) {
        _.mem_arena.deinit();
    }

    pub fn eval(ctx: ExprContext): !ExprValue {
        return _.evalNode(_.root, ctx, 0);
    }

    fn evalNode(idx: usize, ctx: ExprContext, depth: usize): !ExprValue {
        if (depth > _.max_depth) return ExprError.MaxDepthExceeded;
        const node = _.nodes[idx];
        return switch (node) {
            .lit_number n => ExprValue.makeNumber(n),
            .lit_boolean b => ExprValue.makeBool(b),
            .lit_string s => ExprValue.makeString(s),
            .lit_null => .null_val,
            .ident name => {
                const v = ctx.get(name) ?? return ExprError.UndefinedVariable;
                if (n := v.asNumber()) {
                    * = exprCheckFinite(n)?;
                }
                return v;
            },
            .unary u => {
                const v = _.evalNode(u.operand, ctx, depth + 1)?;
                return switch (u.op) {
                    .Neg => switch (v) {
                        .number n => ExprValue.makeNumber(-n),
                        else => ExprError.TypeMismatch,
                    },
                    .Not => switch (v) {
                        .boolean b => ExprValue.makeBool(!b),
                        else => ExprError.TypeMismatch,
                    },
                };
            },
            .ternary t => {
                const cond_val = _.evalNode(t.cond, ctx, depth + 1)?;
                const cond_bool = switch (cond_val) {
                    .boolean b => b,
                    else => return ExprError.TypeMismatch,
                };
                if (cond_bool) {
                    return _.evalNode(t.then_br, ctx, depth + 1);
                }
                return _.evalNode(t.else_br, ctx, depth + 1);
            },
            .binary b => _.evalBinary(b, ctx, depth + 1),
            .call c => _.evalCall(c, ctx, depth + 1),
        };
    }

    fn evalBinary(b: BinaryNode, ctx: ExprContext, depth: usize): !ExprValue {
        if (b.op == .And) {
            const lhs = _.evalNode(b.left, ctx, depth)?;
            const lb = switch (lhs) {
                .boolean v => v,
                else => return ExprError.TypeMismatch,
            };
            if (!lb) return ExprValue.makeBool(false);
            const rhs = _.evalNode(b.right, ctx, depth)?;
            const rb = switch (rhs) {
                .boolean v => v,
                else => return ExprError.TypeMismatch,
            };
            return ExprValue.makeBool(rb);
        }
        if (b.op == .Or) {
            const lhs = _.evalNode(b.left, ctx, depth)?;
            const lb = switch (lhs) {
                .boolean v => v,
                else => return ExprError.TypeMismatch,
            };
            if (lb) return ExprValue.makeBool(true);
            const rhs = _.evalNode(b.right, ctx, depth)?;
            const rb = switch (rhs) {
                .boolean v => v,
                else => return ExprError.TypeMismatch,
            };
            return ExprValue.makeBool(rb);
        }

        const lhs = _.evalNode(b.left, ctx, depth)?;
        const rhs = _.evalNode(b.right, ctx, depth)?;

        switch (b.op) {
            .Eq => return ExprValue.makeBool(lhs.eql(rhs)),
            .Neq => return ExprValue.makeBool(!lhs.eql(rhs)),
            .Add => {
                switch (lhs) {
                    .number a => switch (rhs) {
                        .number c => return ExprValue.makeNumber(exprCheckFinite(a + c)?),
                        else => return ExprError.TypeMismatch,
                    },
                    .string a => switch (rhs) {
                        .string c => {
                            const joined = std.fmt.allocPrint(ctx.allocator, "{s}{s}", .{ a, c }) catch return ExprError.OutOfMemory;
                            return ExprValue.makeString(joined);
                        },
                        else => return ExprError.TypeMismatch,
                    },
                    else => return ExprError.TypeMismatch,
                }
            },
            .Sub => {
                const a = lhs.asNumber() ?? return ExprError.TypeMismatch;
                const c = rhs.asNumber() ?? return ExprError.TypeMismatch;
                return ExprValue.makeNumber(exprCheckFinite(a - c)?);
            },
            .Mul => {
                const a = lhs.asNumber() ?? return ExprError.TypeMismatch;
                const c = rhs.asNumber() ?? return ExprError.TypeMismatch;
                return ExprValue.makeNumber(exprCheckFinite(a * c)?);
            },
            .Div => {
                const a = lhs.asNumber() ?? return ExprError.TypeMismatch;
                const c = rhs.asNumber() ?? return ExprError.TypeMismatch;
                if (c == 0.0) return ExprError.DivisionByZero;
                return ExprValue.makeNumber(exprCheckFinite(a / c)?);
            },
            .Mod => {
                const a = lhs.asNumber() ?? return ExprError.TypeMismatch;
                const c = rhs.asNumber() ?? return ExprError.TypeMismatch;
                if (c == 0.0) return ExprError.DivisionByZero;
                return ExprValue.makeNumber(exprCheckFinite(@rem(a, c))?);
            },
            .Pow => {
                const a = lhs.asNumber() ?? return ExprError.TypeMismatch;
                const c = rhs.asNumber() ?? return ExprError.TypeMismatch;
                return ExprValue.makeNumber(exprCheckFinite(std.math.pow(f64, a, c))?);
            },
            .Lt, .LtEq, .Gt, .GtEq => {
                switch (lhs) {
                    .number a => switch (rhs) {
                        .number c => {
                            const res = switch (b.op) {
                                .Lt => a < c,
                                .LtEq => a <= c,
                                .Gt => a > c,
                                .GtEq => a >= c,
                                else => false,
                            };
                            return ExprValue.makeBool(res);
                        },
                        else => return ExprError.TypeMismatch,
                    },
                    .string a => switch (rhs) {
                        .string c => {
                            const ord = std.mem.order(u8, a, c);
                            const res = switch (b.op) {
                                .Lt => ord == .lt,
                                .LtEq => ord == .lt || ord == .eq,
                                .Gt => ord == .gt,
                                .GtEq => ord == .gt || ord == .eq,
                                else => false,
                            };
                            return ExprValue.makeBool(res);
                        },
                        else => return ExprError.TypeMismatch,
                    },
                    else => return ExprError.TypeMismatch,
                }
            },
            else => return ExprError.TypeMismatch,
        }
    }

    fn evalCall(c: CallNode, ctx: ExprContext, depth: usize): !ExprValue {
        if (std.mem.eql(u8, c.name, "min")) {
            if (c.args.len == 0) return ExprError.InvalidArity;
            const v0 = _.evalNode(c.args[0], ctx, depth)?;
            let best = v0.asNumber() ?? return ExprError.TypeMismatch;
            let i: usize = 1;
            while (i < c.args.len) {
                const vi = _.evalNode(c.args[i], ctx, depth)?;
                const v = vi.asNumber() ?? return ExprError.TypeMismatch;
                if (v < best) best = v;
                i += 1;
            }
            return ExprValue.makeNumber(best);
        }
        if (std.mem.eql(u8, c.name, "max")) {
            if (c.args.len == 0) return ExprError.InvalidArity;
            const v0 = _.evalNode(c.args[0], ctx, depth)?;
            let best = v0.asNumber() ?? return ExprError.TypeMismatch;
            let i: usize = 1;
            while (i < c.args.len) {
                const vi = _.evalNode(c.args[i], ctx, depth)?;
                const v = vi.asNumber() ?? return ExprError.TypeMismatch;
                if (v > best) best = v;
                i += 1;
            }
            return ExprValue.makeNumber(best);
        }
        if (std.mem.eql(u8, c.name, "abs")) {
            if (c.args.len != 1) return ExprError.InvalidArity;
            const v0 = _.evalNode(c.args[0], ctx, depth)?;
            const v = v0.asNumber() ?? return ExprError.TypeMismatch;
            return ExprValue.makeNumber(@abs(v));
        }
        if (std.mem.eql(u8, c.name, "floor")) {
            if (c.args.len != 1) return ExprError.InvalidArity;
            const v0 = _.evalNode(c.args[0], ctx, depth)?;
            const v = v0.asNumber() ?? return ExprError.TypeMismatch;
            return ExprValue.makeNumber(@floor(v));
        }
        if (std.mem.eql(u8, c.name, "ceil")) {
            if (c.args.len != 1) return ExprError.InvalidArity;
            const v0 = _.evalNode(c.args[0], ctx, depth)?;
            const v = v0.asNumber() ?? return ExprError.TypeMismatch;
            return ExprValue.makeNumber(@ceil(v));
        }
        if (std.mem.eql(u8, c.name, "round")) {
            if (c.args.len != 1) return ExprError.InvalidArity;
            const v0 = _.evalNode(c.args[0], ctx, depth)?;
            const v = v0.asNumber() ?? return ExprError.TypeMismatch;
            return ExprValue.makeNumber(@round(v));
        }
        if (std.mem.eql(u8, c.name, "clamp")) {
            if (c.args.len != 3) return ExprError.InvalidArity;
            const vx = _.evalNode(c.args[0], ctx, depth)?;
            const x = vx.asNumber() ?? return ExprError.TypeMismatch;
            const vlo = _.evalNode(c.args[1], ctx, depth)?;
            const lo = vlo.asNumber() ?? return ExprError.TypeMismatch;
            const vhi = _.evalNode(c.args[2], ctx, depth)?;
            const hi = vhi.asNumber() ?? return ExprError.TypeMismatch;
            if (lo > hi) return ExprError.TypeMismatch;
            if (x < lo) return ExprValue.makeNumber(lo);
            if (x > hi) return ExprValue.makeNumber(hi);
            return ExprValue.makeNumber(x);
        }
        if (std.mem.eql(u8, c.name, "len")) {
            if (c.args.len != 1) return ExprError.InvalidArity;
            const vs = _.evalNode(c.args[0], ctx, depth)?;
            const s = vs.asString() ?? return ExprError.TypeMismatch;
            return ExprValue.makeNumber(@as(f64, @floatFromInt(s.len)));
        }
        if (std.mem.eql(u8, c.name, "contains")) {
            if (c.args.len != 2) return ExprError.InvalidArity;
            const vh = _.evalNode(c.args[0], ctx, depth)?;
            const haystack = vh.asString() ?? return ExprError.TypeMismatch;
            const vn = _.evalNode(c.args[1], ctx, depth)?;
            const needle = vn.asString() ?? return ExprError.TypeMismatch;
            return ExprValue.makeBool(std.mem.indexOf(u8, haystack, needle) != null);
        }
        if (std.mem.eql(u8, c.name, "startsWith")) {
            if (c.args.len != 2) return ExprError.InvalidArity;
            const vs = _.evalNode(c.args[0], ctx, depth)?;
            const s = vs.asString() ?? return ExprError.TypeMismatch;
            const vp = _.evalNode(c.args[1], ctx, depth)?;
            const prefix = vp.asString() ?? return ExprError.TypeMismatch;
            return ExprValue.makeBool(std.mem.startsWith(u8, s, prefix));
        }
        return ExprError.UnknownFunction;
    }
}

/// Compile an expression string into a reusable AST with default max recursion depth (64).
pub fn compile(allocator: std.mem.Allocator, source: []const u8): !CompiledExpr {
    return compileWithMaxDepth(allocator, source, DEFAULT_MAX_DEPTH);
}

/// Compile an expression string with a caller-specified maximum recursion depth (clamped to `EXPR_MAX_DEPTH`).
pub fn compileWithMaxDepth(allocator: std.mem.Allocator, source: []const u8, max_depth: usize): !CompiledExpr {
    const eff_depth: usize = if (max_depth > EXPR_MAX_DEPTH) EXPR_MAX_DEPTH else max_depth;
    let mem_arena = std.heap.ArenaAllocator.init(allocator);
    errdefer mem_arena.deinit();
    const a = mem_arena.allocator();

    let parser = Parser{
        lexer: Lexer{ src: source, pos: 0, mem_arena: a },
        cur: Token{},
        mem_arena: a,
        nodes: .empty,
        max_depth: eff_depth,
    };
    parser.advance()?;
    const root = parser.parseExpr(0)?;
    if (parser.cur.kind != .Eof) {
        return ExprError.UnexpectedToken;
    }
    const nodes = parser.nodes.toOwnedSlice(a) catch return ExprError.OutOfMemory;
    return CompiledExpr{
        allocator: allocator,
        mem_arena: mem_arena,
        nodes: nodes,
        root: root,
        max_depth: eff_depth,
    };
}

/// Compile and immediately evaluate an expression against `ctx`.
pub fn eval(source: []const u8, ctx: ExprContext): !ExprValue {
    let compiled = compile(ctx.allocator, source)?;
    defer compiled.deinit();
    return compiled.eval(ctx);
}
