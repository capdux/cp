// Source: cp/lib/cel/parser.cp (CEL Lexer & Recursive-Descent AST Compiler)
const std = @import("std");
import { CelError } from "./types.cp";

pub const CelUnaryOp = enum {
    not_op,
    neg_op,
};

pub const CelBinaryOp = enum {
    or_op,
    and_op,
    eq_op,
    ne_op,
    lt_op,
    le_op,
    gt_op,
    ge_op,
    in_op,
    add_op,
    sub_op,
    mul_op,
    div_op,
    mod_op,
};

pub const CelMacroKind = enum {
    all,
    exists,
    exists_one,
    filter,
    map,
};

pub struct CelMapAstEntry {
    key: *CelExpr,
    value: *CelExpr,
}

pub struct CelUnaryExpr {
    op: CelUnaryOp,
    operand: *CelExpr,
}

pub struct CelBinaryExpr {
    op: CelBinaryOp,
    left: *CelExpr,
    right: *CelExpr,
}

pub struct CelTernaryExpr {
    cond: *CelExpr,
    true_expr: *CelExpr,
    false_expr: *CelExpr,
}

pub struct CelMemberExpr {
    target: *CelExpr,
    field: []const u8,
}

pub struct CelIndexExpr {
    target: *CelExpr,
    idx: *CelExpr,
}

pub struct CelCallExpr {
    target: (*CelExpr)?,
    name: []const u8,
    args: []*CelExpr,
}

pub struct CelHasFieldExpr {
    target: *CelExpr,
    field: []const u8,
}

pub struct CelMacroExpr {
    kind: CelMacroKind,
    target: *CelExpr,
    iter_var: []const u8,
    body: *CelExpr,
}

pub const CelExpr = union(enum) {
    null_lit: void,
    bool_lit: bool,
    int_lit: i64,
    uint_lit: u64,
    double_lit: f64,
    string_lit: []const u8,
    ident: []const u8,
    list_lit: []*CelExpr,
    map_lit: []CelMapAstEntry,
    unary: CelUnaryExpr,
    binary: CelBinaryExpr,
    ternary: CelTernaryExpr,
    member: CelMemberExpr,
    index: CelIndexExpr,
    call: CelCallExpr,
    has_field: CelHasFieldExpr,
    macro_call: CelMacroExpr,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.*) {
            .null_lit, .bool_lit, .int_lit, .uint_lit, .double_lit => {},
            .string_lit => |s| alloc.free(s),
            .ident => |s| alloc.free(s),
            .list_lit => |items| {
                for (it of items) {
                    it.deinit(alloc);
                }
                alloc.free(items);
            },
            .map_lit => |entries| {
                for (e of entries) {
                    e.key.deinit(alloc);
                    e.value.deinit(alloc);
                }
                alloc.free(entries);
            },
            .unary => |u| {
                u.operand.deinit(alloc);
            },
            .binary => |b| {
                b.left.deinit(alloc);
                b.right.deinit(alloc);
            },
            .ternary => |t| {
                t.cond.deinit(alloc);
                t.true_expr.deinit(alloc);
                t.false_expr.deinit(alloc);
            },
            .member => |m| {
                m.target.deinit(alloc);
                alloc.free(m.field);
            },
            .index => |ix| {
                ix.target.deinit(alloc);
                ix.idx.deinit(alloc);
            },
            .call => |c| {
                if (tgt := c.target) {
                    tgt.deinit(alloc);
                }
                alloc.free(c.name);
                for (a of c.args) {
                    a.deinit(alloc);
                }
                alloc.free(c.args);
            },
            .has_field => |h| {
                h.target.deinit(alloc);
                alloc.free(h.field);
            },
            .macro_call => |mc| {
                mc.target.deinit(alloc);
                alloc.free(mc.iter_var);
                mc.body.deinit(alloc);
            },
        }
        alloc.destroy(_);
    }
};

const TokenKind = enum {
    eof,
    ident,
    int_lit,
    uint_lit,
    double_lit,
    string_lit,
    true_kw,
    false_kw,
    null_kw,
    in_kw,
    lparen,
    rparen,
    lbracket,
    rbracket,
    lbrace,
    rbrace,
    dot,
    comma,
    colon,
    question,
    plus,
    minus,
    star,
    slash,
    percent,
    bang,
    eq_eq,
    bang_eq,
    lt,
    lt_eq,
    gt,
    gt_eq,
    and_and,
    or_or,
};

struct Token {
    kind: TokenKind,
    lexeme: []const u8,
    string_owned: ([]const u8)?,
    int_val: i64,
    uint_val: u64,
    double_val: f64,
}

struct Lexer {
    src: []const u8,
    pos: usize,

    static fn init(src: []const u8): Lexer {
        return Lexer{ .src = src, .pos = 0 };
    }

    fn skipWsAndComments(*_: *Lexer)  {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' or c == '\t' or c == '\n' or c == '\r') {
                _.pos += 1;
            } else if (c == '/' and _.pos + 1 < _.src.len and _.src[_.pos + 1] == '/') {
                _.pos += 2;
                while (_.pos < _.src.len and _.src[_.pos] != '\n') {
                    _.pos += 1;
                }
            } else {
                break;
            }
        }
    }

    fn nextToken(*_: *Lexer, alloc: std.mem.Allocator): CelError!Token {
        _.skipWsAndComments();
        if (_.pos >= _.src.len) {
            return Token{ .kind = .eof, .lexeme = "", .string_owned = null, .int_val = 0, .uint_val = 0, .double_val = 0.0 };
        }
        const start = _.pos;
        const c = _.src[_.pos];

        if (c == '"' or c == '\'') {
            return _.lexString(alloc, c);
        }

        if (isDigit(c)) {
            return _.lexNumber();
        }

        if (isIdentStart(c)) {
            _.pos += 1;
            while (_.pos < _.src.len and isIdentCont(_.src[_.pos])) {
                _.pos += 1;
            }
            const word = _.src[start.._.pos];
            const kind: TokenKind = if (std.mem.eql(u8, word, "true"))
                .true_kw
            else if (std.mem.eql(u8, word, "false"))
                .false_kw
            else if (std.mem.eql(u8, word, "null"))
                .null_kw
            else if (std.mem.eql(u8, word, "in"))
                .in_kw
            else
                .ident;
            return Token{ .kind = kind, .lexeme = word, .string_owned = null, .int_val = 0, .uint_val = 0, .double_val = 0.0 };
        }

        // Two-char operators
        if (_.pos + 1 < _.src.len) {
            const c2 = _.src[_.pos + 1];
            if (c == '=' and c2 == '=') {
                _.pos += 2;
                return makeSimple(.eq_eq, _.src[start.._.pos]);
            }
            if (c == '!' and c2 == '=') {
                _.pos += 2;
                return makeSimple(.bang_eq, _.src[start.._.pos]);
            }
            if (c == '<' and c2 == '=') {
                _.pos += 2;
                return makeSimple(.lt_eq, _.src[start.._.pos]);
            }
            if (c == '>' and c2 == '=') {
                _.pos += 2;
                return makeSimple(.gt_eq, _.src[start.._.pos]);
            }
            if (c == '&' and c2 == '&') {
                _.pos += 2;
                return makeSimple(.and_and, _.src[start.._.pos]);
            }
            if (c == '|' and c2 == '|') {
                _.pos += 2;
                return makeSimple(.or_or, _.src[start.._.pos]);
            }
        }

        // Single-char operators
        _.pos += 1;
        const kind: TokenKind = switch (c) {
            '(' => .lparen,
            ')' => .rparen,
            '[' => .lbracket,
            ']' => .rbracket,
            '{' => .lbrace,
            '}' => .rbrace,
            '.' => .dot,
            ',' => .comma,
            ':' => .colon,
            '?' => .question,
            '+' => .plus,
            '-' => .minus,
            '*' => .star,
            '/' => .slash,
            '%' => .percent,
            '!' => .bang,
            '<' => .lt,
            '>' => .gt,
            else => return error.SyntaxError,
        };
        return makeSimple(kind, _.src[start.._.pos]);
    }

    fn lexString(*_: *Lexer, alloc: std.mem.Allocator, quote: u8): CelError!Token {
        _.pos += 1; // skip opening quote
        let buf = std.ArrayList(u8).empty;
        errdefer buf.deinit(alloc);

        while (_.pos < _.src.len) {
            const ch = _.src[_.pos];
            if (ch == quote) {
                _.pos += 1;
                const owned = buf.toOwnedSlice(alloc) catch return error.OutOfMemory;
                return Token{ .kind = .string_lit, .lexeme = "", .string_owned = owned, .int_val = 0, .uint_val = 0, .double_val = 0.0 };
            }
            if (ch == '\\') {
                _.pos += 1;
                if (_.pos >= _.src.len) {
                    return error.SyntaxError;
                }
                const esc = _.src[_.pos];
                _.pos += 1;
                const mapped: u8 = switch (esc) {
                    'n' => '\n',
                    't' => '\t',
                    'r' => '\r',
                    '\\' => '\\',
                    '"' => '"',
                    '\'' => '\'',
                    else => return error.SyntaxError,
                };
                buf.append(alloc, mapped) catch return error.OutOfMemory;
            } else if (ch == '\n') {
                return error.SyntaxError;
            } else {
                _.pos += 1;
                buf.append(alloc, ch) catch return error.OutOfMemory;
            }
        }
        return error.SyntaxError;
    }

    fn lexNumber(*_: *Lexer): CelError!Token {
        const start = _.pos;
        if (_.src[_.pos] == '0' and _.pos + 1 < _.src.len and (_.src[_.pos + 1] == 'x' or _.src[_.pos + 1] == 'X')) {
            _.pos += 2;
            const hex_start = _.pos;
            while (_.pos < _.src.len and isHexDigit(_.src[_.pos])) {
                _.pos += 1;
            }
            if (_.pos == hex_start) {
                return error.SyntaxError;
            }
            if (_.pos < _.src.len and (_.src[_.pos] == 'u' or _.src[_.pos] == 'U')) {
                const digits = _.src[hex_start.._.pos];
                _.pos += 1;
                const uval = std.fmt.parseInt(u64, digits, 16) catch return error.SyntaxError;
                return Token{ .kind = .uint_lit, .lexeme = _.src[start.._.pos], .string_owned = null, .int_val = 0, .uint_val = uval, .double_val = 0.0 };
            }
            const digits = _.src[hex_start.._.pos];
            const ival = std.fmt.parseInt(i64, digits, 16) catch return error.SyntaxError;
            return Token{ .kind = .int_lit, .lexeme = _.src[start.._.pos], .string_owned = null, .int_val = ival, .uint_val = 0, .double_val = 0.0 };
        }

        while (_.pos < _.src.len and isDigit(_.src[_.pos])) {
            _.pos += 1;
        }
        let is_float = false;
        if (_.pos + 1 < _.src.len and _.src[_.pos] == '.' and isDigit(_.src[_.pos + 1])) {
            is_float = true;
            _.pos += 1;
            while (_.pos < _.src.len and isDigit(_.src[_.pos])) {
                _.pos += 1;
            }
        }
        if (_.pos < _.src.len and (_.src[_.pos] == 'e' or _.src[_.pos] == 'E')) {
            is_float = true;
            _.pos += 1;
            if (_.pos < _.src.len and (_.src[_.pos] == '+' or _.src[_.pos] == '-')) {
                _.pos += 1;
            }
            const exp_start = _.pos;
            while (_.pos < _.src.len and isDigit(_.src[_.pos])) {
                _.pos += 1;
            }
            if (_.pos == exp_start) {
                return error.SyntaxError;
            }
        }

        if (is_float) {
            const fval = std.fmt.parseFloat(f64, _.src[start.._.pos]) catch return error.SyntaxError;
            return Token{ .kind = .double_lit, .lexeme = _.src[start.._.pos], .string_owned = null, .int_val = 0, .uint_val = 0, .double_val = fval };
        }

        if (_.pos < _.src.len and (_.src[_.pos] == 'u' or _.src[_.pos] == 'U')) {
            const digits = _.src[start.._.pos];
            _.pos += 1;
            const uval = std.fmt.parseInt(u64, digits, 10) catch return error.SyntaxError;
            return Token{ .kind = .uint_lit, .lexeme = _.src[start.._.pos], .string_owned = null, .int_val = 0, .uint_val = uval, .double_val = 0.0 };
        }

        const ival = std.fmt.parseInt(i64, _.src[start.._.pos], 10) catch return error.SyntaxError;
        return Token{ .kind = .int_lit, .lexeme = _.src[start.._.pos], .string_owned = null, .int_val = ival, .uint_val = 0, .double_val = 0.0 };
    }
}

fn makeSimple(kind: TokenKind, lexeme: []const u8): Token {
    return Token{ .kind = kind, .lexeme = lexeme, .string_owned = null, .int_val = 0, .uint_val = 0, .double_val = 0.0 };
}

fn isDigit(c: u8): bool {
    return c >= '0' and c <= '9';
}

fn isHexDigit(c: u8): bool {
    return (c >= '0' and c <= '9') or (c >= 'a' and c <= 'f') or (c >= 'A' and c <= 'F');
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or c == '_';
}

fn isIdentCont(c: u8): bool {
    return isIdentStart(c) or isDigit(c);
}

struct Parser {
    alloc: std.mem.Allocator,
    lexer: Lexer,
    cur: Token,
    depth: usize,
    max_depth: usize,

    static fn init(alloc: std.mem.Allocator, src: []const u8, max_depth: usize): CelError!Parser {
        let lex = Lexer.init(src);
        const first = try lex.nextToken(alloc);
        return Parser{
            .alloc = alloc,
            .lexer = lex,
            .cur = first,
            .depth = 0,
            .max_depth = max_depth,
        };
    }

    fn deinitCur(*_: *Parser)  {
        if (s := _.cur.string_owned) {
            _.alloc.free(s);
            _.cur.string_owned = null;
        }
    }

    fn advance(*_: *Parser): CelError!void {
        _.deinitCur();
        _.cur = try _.lexer.nextToken(_.alloc);
    }

    fn enterDepth(*_: *Parser): CelError!void {
        _.depth += 1;
        if (_.depth > _.max_depth) {
            return error.MaxDepthExceeded;
        }
    }

    fn leaveDepth(*_: *Parser)  {
        if (_.depth > 0) {
            _.depth -= 1;
        }
    }

    fn allocNode(*_: *Parser, val: CelExpr): CelError!*CelExpr {
        let node = _.alloc.create(CelExpr) catch return error.OutOfMemory;
        node.* = val;
        return node;
    }

    fn parseExpr(*_: *Parser): CelError!*CelExpr {
        try _.enterDepth();
        defer _.leaveDepth();
        return _.parseTernary();
    }

    fn parseTernary(*_: *Parser): CelError!*CelExpr {
        const cond = try _.parseOr();
        if (_.cur.kind == .question) {
            errdefer cond.deinit(_.alloc);
            try _.advance();
            const t_expr = try _.parseExpr();
            errdefer t_expr.deinit(_.alloc);
            if (_.cur.kind != .colon) {
                return error.SyntaxError;
            }
            try _.advance();
            const f_expr = try _.parseTernary();
            errdefer f_expr.deinit(_.alloc);
            return _.allocNode(CelExpr{
                .ternary = .{ .cond = cond, .true_expr = t_expr, .false_expr = f_expr },
            });
        }
        return cond;
    }

    fn parseOr(*_: *Parser): CelError!*CelExpr {
        let left = try _.parseAnd();
        while (_.cur.kind == .or_or) {
            errdefer left.deinit(_.alloc);
            try _.advance();
            const right = try _.parseAnd();
            errdefer right.deinit(_.alloc);
            left = try _.allocNode(CelExpr{
                .binary = .{ .op = .or_op, .left = left, .right = right },
            });
        }
        return left;
    }

    fn parseAnd(*_: *Parser): CelError!*CelExpr {
        let left = try _.parseRelation();
        while (_.cur.kind == .and_and) {
            errdefer left.deinit(_.alloc);
            try _.advance();
            const right = try _.parseRelation();
            errdefer right.deinit(_.alloc);
            left = try _.allocNode(CelExpr{
                .binary = .{ .op = .and_op, .left = left, .right = right },
            });
        }
        return left;
    }

    fn parseRelation(*_: *Parser): CelError!*CelExpr {
        let left = try _.parseAdd();
        while (true) {
            const op: CelBinaryOp? = switch (_.cur.kind) {
                .eq_eq => .eq_op,
                .bang_eq => .ne_op,
                .lt => .lt_op,
                .lt_eq => .le_op,
                .gt => .gt_op,
                .gt_eq => .ge_op,
                .in_kw => .in_op,
                else => null,
            };
            if (bop := op) {
                errdefer left.deinit(_.alloc);
                try _.advance();
                const right = try _.parseAdd();
                errdefer right.deinit(_.alloc);
                left = try _.allocNode(CelExpr{
                    .binary = .{ .op = bop, .left = left, .right = right },
                });
            } else {
                break;
            }
        }
        return left;
    }

    fn parseAdd(*_: *Parser): CelError!*CelExpr {
        let left = try _.parseMul();
        while (_.cur.kind == .plus or _.cur.kind == .minus) {
            const bop: CelBinaryOp = if (_.cur.kind == .plus) .add_op else .sub_op;
            errdefer left.deinit(_.alloc);
            try _.advance();
            const right = try _.parseMul();
            errdefer right.deinit(_.alloc);
            left = try _.allocNode(CelExpr{
                .binary = .{ .op = bop, .left = left, .right = right },
            });
        }
        return left;
    }

    fn parseMul(*_: *Parser): CelError!*CelExpr {
        let left = try _.parseUnary();
        while (_.cur.kind == .star or _.cur.kind == .slash or _.cur.kind == .percent) {
            const bop: CelBinaryOp = switch (_.cur.kind) {
                .star => .mul_op,
                .slash => .div_op,
                .percent => .mod_op,
                else => .mul_op,
            };
            errdefer left.deinit(_.alloc);
            try _.advance();
            const right = try _.parseUnary();
            errdefer right.deinit(_.alloc);
            left = try _.allocNode(CelExpr{
                .binary = .{ .op = bop, .left = left, .right = right },
            });
        }
        return left;
    }

    fn parseUnary(*_: *Parser): CelError!*CelExpr {
        if (_.cur.kind == .bang or _.cur.kind == .minus) {
            try _.enterDepth();
            defer _.leaveDepth();
            const uop: CelUnaryOp = if (_.cur.kind == .bang) .not_op else .neg_op;
            try _.advance();
            const operand = try _.parseUnary();
            errdefer operand.deinit(_.alloc);
            return _.allocNode(CelExpr{
                .unary = .{ .op = uop, .operand = operand },
            });
        }
        return _.parsePostfix();
    }

    fn parsePostfix(*_: *Parser): CelError!*CelExpr {
        let expr = try _.parsePrimary();
        while (true) {
            if (_.cur.kind == .dot) {
                errdefer expr.deinit(_.alloc);
                try _.advance();
                if (_.cur.kind != .ident) {
                    return error.SyntaxError;
                }
                const field_name = _.alloc.dupe(u8, _.cur.lexeme) catch return error.OutOfMemory;
                errdefer _.alloc.free(field_name);
                try _.advance();

                if (_.cur.kind == .lparen) {
                    // Method call or macro call
                    if (mk := macroFromName(field_name)) {
                        _.alloc.free(field_name);
                        try _.advance(); // consume '('
                        if (_.cur.kind != .ident) {
                            return error.SyntaxError;
                        }
                        const iter_var = _.alloc.dupe(u8, _.cur.lexeme) catch return error.OutOfMemory;
                        errdefer _.alloc.free(iter_var);
                        try _.advance();
                        if (_.cur.kind != .comma) {
                            return error.SyntaxError;
                        }
                        try _.advance();
                        const body = try _.parseExpr();
                        errdefer body.deinit(_.alloc);
                        if (_.cur.kind != .rparen) {
                            return error.SyntaxError;
                        }
                        try _.advance();
                        expr = try _.allocNode(CelExpr{
                            .macro_call = .{ .kind = mk, .target = expr, .iter_var = iter_var, .body = body },
                        });
                    } else {
                        const args = try _.parseArgList();
                        errdefer {
                            for (a of args) {
                                a.deinit(_.alloc);
                            }
                            _.alloc.free(args);
                        }
                        expr = try _.allocNode(CelExpr{
                            .call = .{ .target = expr, .name = field_name, .args = args },
                        });
                    }
                } else {
                    expr = try _.allocNode(CelExpr{
                        .member = .{ .target = expr, .field = field_name },
                    });
                }
            } else if (_.cur.kind == .lbracket) {
                errdefer expr.deinit(_.alloc);
                try _.advance();
                const idx = try _.parseExpr();
                errdefer idx.deinit(_.alloc);
                if (_.cur.kind != .rbracket) {
                    return error.SyntaxError;
                }
                try _.advance();
                expr = try _.allocNode(CelExpr{
                    .index = .{ .target = expr, .idx = idx },
                });
            } else {
                break;
            }
        }
        return expr;
    }

    fn parseArgList(*_: *Parser): CelError![]*CelExpr {
        if (_.cur.kind != .lparen) {
            return error.SyntaxError;
        }
        try _.advance();
        let args = std.ArrayList(*CelExpr).empty;
        errdefer {
            for (a of args.items) {
                a.deinit(_.alloc);
            }
            args.deinit(_.alloc);
        }
        if (_.cur.kind != .rparen) {
            while (true) {
                const arg = try _.parseExpr();
                args.append(_.alloc, arg) catch {
                    arg.deinit(_.alloc);
                    return error.OutOfMemory;
                };
                if (_.cur.kind == .comma) {
                    try _.advance();
                    if (_.cur.kind == .rparen) {
                        break;
                    }
                } else {
                    break;
                }
            }
        }
        if (_.cur.kind != .rparen) {
            return error.SyntaxError;
        }
        try _.advance();
        return args.toOwnedSlice(_.alloc) catch return error.OutOfMemory;
    }

    fn parsePrimary(*_: *Parser): CelError!*CelExpr {
        switch (_.cur.kind) {
            .null_kw => {
                try _.advance();
                return _.allocNode(.null_lit);
            },
            .true_kw => {
                try _.advance();
                return _.allocNode(CelExpr{ .bool_lit = true });
            },
            .false_kw => {
                try _.advance();
                return _.allocNode(CelExpr{ .bool_lit = false });
            },
            .int_lit => {
                const v = _.cur.int_val;
                try _.advance();
                return _.allocNode(CelExpr{ .int_lit = v });
            },
            .uint_lit => {
                const v = _.cur.uint_val;
                try _.advance();
                return _.allocNode(CelExpr{ .uint_lit = v });
            },
            .double_lit => {
                const v = _.cur.double_val;
                try _.advance();
                return _.allocNode(CelExpr{ .double_lit = v });
            },
            .string_lit => {
                const s = _.cur.string_owned orelse "";
                _.cur.string_owned = null;
                errdefer _.alloc.free(s);
                try _.advance();
                return _.allocNode(CelExpr{ .string_lit = s });
            },
            .ident => {
                const name = _.alloc.dupe(u8, _.cur.lexeme) catch return error.OutOfMemory;
                errdefer _.alloc.free(name);
                try _.advance();
                if (_.cur.kind == .lparen) {
                    if (std.mem.eql(u8, name, "has")) {
                        _.alloc.free(name);
                        try _.advance(); // '('
                        const inner = try _.parseExpr();
                        errdefer inner.deinit(_.alloc);
                        if (_.cur.kind != .rparen) {
                            return error.SyntaxError;
                        }
                        try _.advance();
                        switch (inner.*) {
                            .member => |m| {
                                const tgt = m.target;
                                const fld = m.field;
                                _.alloc.destroy(inner);
                                return _.allocNode(CelExpr{
                                    .has_field = .{ .target = tgt, .field = fld },
                                });
                            },
                            else => return error.SyntaxError,
                        }
                    }
                    const args = try _.parseArgList();
                    errdefer {
                        for (a of args) {
                            a.deinit(_.alloc);
                        }
                        _.alloc.free(args);
                    }
                    return _.allocNode(CelExpr{
                        .call = .{ .target = null, .name = name, .args = args },
                    });
                }
                return _.allocNode(CelExpr{ .ident = name });
            },
            .lparen => {
                try _.advance();
                const inner = try _.parseExpr();
                errdefer inner.deinit(_.alloc);
                if (_.cur.kind != .rparen) {
                    return error.SyntaxError;
                }
                try _.advance();
                return inner;
            },
            .lbracket => {
                try _.advance();
                let items = std.ArrayList(*CelExpr).empty;
                errdefer {
                    for (it of items.items) {
                        it.deinit(_.alloc);
                    }
                    items.deinit(_.alloc);
                }
                if (_.cur.kind != .rbracket) {
                    while (true) {
                        const it = try _.parseExpr();
                        items.append(_.alloc, it) catch {
                            it.deinit(_.alloc);
                            return error.OutOfMemory;
                        };
                        if (_.cur.kind == .comma) {
                            try _.advance();
                            if (_.cur.kind == .rbracket) {
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                }
                if (_.cur.kind != .rbracket) {
                    return error.SyntaxError;
                }
                try _.advance();
                const slice = items.toOwnedSlice(_.alloc) catch return error.OutOfMemory;
                return _.allocNode(CelExpr{ .list_lit = slice });
            },
            .lbrace => {
                try _.advance();
                let entries = std.ArrayList(CelMapAstEntry).empty;
                errdefer {
                    for (e of entries.items) {
                        e.key.deinit(_.alloc);
                        e.value.deinit(_.alloc);
                    }
                    entries.deinit(_.alloc);
                }
                if (_.cur.kind != .rbrace) {
                    while (true) {
                        const k = try _.parseExpr();
                        errdefer k.deinit(_.alloc);
                        if (_.cur.kind != .colon) {
                            return error.SyntaxError;
                        }
                        try _.advance();
                        const v = try _.parseExpr();
                        errdefer v.deinit(_.alloc);
                        entries.append(_.alloc, CelMapAstEntry{ .key = k, .value = v }) catch return error.OutOfMemory;
                        if (_.cur.kind == .comma) {
                            try _.advance();
                            if (_.cur.kind == .rbrace) {
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                }
                if (_.cur.kind != .rbrace) {
                    return error.SyntaxError;
                }
                try _.advance();
                const slice = entries.toOwnedSlice(_.alloc) catch return error.OutOfMemory;
                return _.allocNode(CelExpr{ .map_lit = slice });
            },
            else => return error.SyntaxError,
        }
    }
}

fn macroFromName(name: []const u8): CelMacroKind? {
    if (std.mem.eql(u8, name, "all")) {
        return .all;
    }
    if (std.mem.eql(u8, name, "exists")) {
        return .exists;
    }
    if (std.mem.eql(u8, name, "exists_one")) {
        return .exists_one;
    }
    if (std.mem.eql(u8, name, "filter")) {
        return .filter;
    }
    if (std.mem.eql(u8, name, "map")) {
        return .map;
    }
    return null;
}

pub fn compileCel(alloc: std.mem.Allocator, source: []const u8): CelError!*CelExpr {
    return compileCelWithMaxDepth(alloc, source, 64);
}

pub fn compileCelWithMaxDepth(alloc: std.mem.Allocator, source: []const u8, max_depth: usize): CelError!*CelExpr {
    let parser = try Parser.init(alloc, source, max_depth);
    defer parser.deinitCur();
    const expr = try parser.parseExpr();
    errdefer expr.deinit(alloc);
    if (parser.cur.kind != .eof) {
        return error.SyntaxError;
    }
    return expr;
}
