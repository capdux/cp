// Source: cp/lib/jq/ast.cp (Filter AST Compiler for cap:jq)
const std = @import("std");
import { JqError, JqTag } from "./value.cp";

pub const MAX_AST_DEPTH: usize = 64;
pub const INVALID_NODE: u32 = 0xFFFF_FFFF;

pub enum JqBinOp {
    Eq,
    Neq,
    Lt,
    Le,
    Gt,
    Ge,
    And,
    Or,
    Alt,
    Add,
    Sub,
    Mul,
    Div,
}

pub enum JqBuiltin {
    Select,
    Map,
    Length,
    Keys,
    Values,
    Has,
    Contains,
    Empty,
    Type,
    Reverse,
    Unique,
    Sort,
    Not,
}

pub enum JqNodeKind {
    Identity,
    Literal,
    Field,
    Index,
    Slice,
    Iterate,
    Pipe,
    Comma,
    ArrayConstruct,
    ObjectConstruct,
    BinOp,
    Neg,
    Call,
}

pub struct JqObjField {
    key_str_idx: u32 = 0,
    key_node: u32 = INVALID_NODE,
    val_node: u32 = INVALID_NODE,
}

pub struct JqNode {
    kind: JqNodeKind = .Identity,
    optional: bool = false,
    lit_tag: JqTag = .Null,
    lit_bool: bool = false,
    lit_int: i64 = 0,
    lit_float: f64 = 0.0,
    str_idx: u32 = 0,
    lhs: u32 = INVALID_NODE,
    rhs: u32 = INVALID_NODE,
    has_start: bool = false,
    has_end: bool = false,
    slice_start: i64 = 0,
    slice_end: i64 = 0,
    bin_op: JqBinOp = .Eq,
    builtin: JqBuiltin = .Empty,
    field_start: u32 = 0,
    field_len: u32 = 0,
}

pub struct JqExpr {
    nodes: []JqNode = [],
    strings: [][]u8 = [],
    obj_fields: []JqObjField = [],
    root: u32 = 0,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        for (s of _.strings) {
            alloc.free(s);
        }
        if (_.strings.len > 0) alloc.free(_.strings);
        if (_.nodes.len > 0) alloc.free(_.nodes);
        if (_.obj_fields.len > 0) alloc.free(_.obj_fields);
        _.* = .{};
    }
}

enum TokKind {
    Eof,
    Dot,
    DotDot,
    Pipe,
    Comma,
    Colon,
    Semicolon,
    Question,
    SlashSlash,
    LParen,
    RParen,
    LBracket,
    RBracket,
    LBrace,
    RBrace,
    EqEq,
    NotEq,
    Lt,
    Le,
    Gt,
    Ge,
    Plus,
    Minus,
    Star,
    Slash,
    Ident,
    String,
    Int,
    Float,
}

struct Token {
    kind: TokKind = .Eof,
    start: usize = 0,
    end: usize = 0,
    int_val: i64 = 0,
    float_val: f64 = 0.0,
    str_idx: u32 = 0,
}

struct JqParser {
    alloc: std.mem.Allocator,
    src: []const u8,
    pos: usize = 0,
    depth: usize = 0,
    nodes: std.ArrayList(JqNode) = std.ArrayList(JqNode).empty,
    strings: std.ArrayList([]u8) = std.ArrayList([]u8).empty,
    obj_fields: std.ArrayList(JqObjField) = std.ArrayList(JqObjField).empty,
    cur: Token = .{},

    fn deinitLists(*_) {
        for (s of _.strings.items) _.alloc.free(s);
        _.strings.deinit(_.alloc);
        _.nodes.deinit(_.alloc);
        _.obj_fields.deinit(_.alloc);
    }

    fn addString(*_, s: []const u8): JqError!u32 {
        const dup = _.alloc.dupe(u8, s) catch return JqError.OutOfMemory;
        _.strings.append(_.alloc, dup) catch {
            _.alloc.free(dup);
            return JqError.OutOfMemory;
        };
        return @as(u32, @intCast(_.strings.items.len - 1));
    }

    fn addNode(*_, node: JqNode): JqError!u32 {
        _.nodes.append(_.alloc, node) catch return JqError.OutOfMemory;
        return @as(u32, @intCast(_.nodes.items.len - 1));
    }

    fn skipWsAndComments(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' or c == '\t' or c == '\n' or c == '\r') {
                _.pos += 1;
            } else if (c == '#') {
                while (_.pos < _.src.len and _.src[_.pos] != '\n') _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn isIdentStart(c: u8): bool {
        return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or c == '_';
    }

    fn isIdentCont(c: u8): bool {
        return isIdentStart(c) or (c >= '0' and c <= '9');
    }

    fn nextToken(*_): JqError!Token {
        _.skipWsAndComments();
        if (_.pos >= _.src.len) return .{ .kind = .Eof, .start = _.pos, .end = _.pos };

        const start = _.pos;
        const c = _.src[_.pos];
        switch (c) {
            '.' => {
                _.pos += 1;
                if (_.pos < _.src.len and _.src[_.pos] == '.') {
                    _.pos += 1;
                    return .{ .kind = .DotDot, .start = start, .end = _.pos };
                }
                return .{ .kind = .Dot, .start = start, .end = _.pos };
            },
            '|' => { _.pos += 1; return .{ .kind = .Pipe, .start = start, .end = _.pos }; },
            ',' => { _.pos += 1; return .{ .kind = .Comma, .start = start, .end = _.pos }; },
            ':' => { _.pos += 1; return .{ .kind = .Colon, .start = start, .end = _.pos }; },
            ';' => { _.pos += 1; return .{ .kind = .Semicolon, .start = start, .end = _.pos }; },
            '?' => { _.pos += 1; return .{ .kind = .Question, .start = start, .end = _.pos }; },
            '(' => { _.pos += 1; return .{ .kind = .LParen, .start = start, .end = _.pos }; },
            ')' => { _.pos += 1; return .{ .kind = .RParen, .start = start, .end = _.pos }; },
            '[' => { _.pos += 1; return .{ .kind = .LBracket, .start = start, .end = _.pos }; },
            ']' => { _.pos += 1; return .{ .kind = .RBracket, .start = start, .end = _.pos }; },
            '{' => { _.pos += 1; return .{ .kind = .LBrace, .start = start, .end = _.pos }; },
            '}' => { _.pos += 1; return .{ .kind = .RBrace, .start = start, .end = _.pos }; },
            '+' => { _.pos += 1; return .{ .kind = .Plus, .start = start, .end = _.pos }; },
            '-' => { _.pos += 1; return .{ .kind = .Minus, .start = start, .end = _.pos }; },
            '*' => { _.pos += 1; return .{ .kind = .Star, .start = start, .end = _.pos }; },
            '/' => {
                _.pos += 1;
                if (_.pos < _.src.len and _.src[_.pos] == '/') {
                    _.pos += 1;
                    return .{ .kind = .SlashSlash, .start = start, .end = _.pos };
                }
                return .{ .kind = .Slash, .start = start, .end = _.pos };
            },
            '=' => {
                _.pos += 1;
                if (_.pos < _.src.len and _.src[_.pos] == '=') {
                    _.pos += 1;
                    return .{ .kind = .EqEq, .start = start, .end = _.pos };
                }
                return JqError.InvalidSyntax;
            },
            '!' => {
                _.pos += 1;
                if (_.pos < _.src.len and _.src[_.pos] == '=') {
                    _.pos += 1;
                    return .{ .kind = .NotEq, .start = start, .end = _.pos };
                }
                return JqError.InvalidSyntax;
            },
            '<' => {
                _.pos += 1;
                if (_.pos < _.src.len and _.src[_.pos] == '=') {
                    _.pos += 1;
                    return .{ .kind = .Le, .start = start, .end = _.pos };
                }
                return .{ .kind = .Lt, .start = start, .end = _.pos };
            },
            '>' => {
                _.pos += 1;
                if (_.pos < _.src.len and _.src[_.pos] == '=') {
                    _.pos += 1;
                    return .{ .kind = .Ge, .start = start, .end = _.pos };
                }
                return .{ .kind = .Gt, .start = start, .end = _.pos };
            },
            '"' => {
                _.pos += 1;
                var buf = std.ArrayList(u8).empty;
                defer buf.deinit(_.alloc);
                while (_.pos < _.src.len) {
                    const ch = _.src[_.pos];
                    if (ch == '"') {
                        _.pos += 1;
                        const sidx = try _.addString(buf.items);
                        return .{ .kind = .String, .start = start, .end = _.pos, .str_idx = sidx };
                    }
                    if (ch == '\\') {
                        _.pos += 1;
                        if (_.pos >= _.src.len) return JqError.UnexpectedEof;
                        const esc = _.src[_.pos];
                        _.pos += 1;
                        const mapped: u8 = switch (esc) {
                            '"' => '"',
                            '\\' => '\\',
                            '/' => '/',
                            'n' => '\n',
                            'r' => '\r',
                            't' => '\t',
                            else => return JqError.InvalidSyntax,
                        };
                        buf.append(_.alloc, mapped) catch return JqError.OutOfMemory;
                    } else {
                        buf.append(_.alloc, ch) catch return JqError.OutOfMemory;
                        _.pos += 1;
                    }
                }
                return JqError.UnexpectedEof;
            },
            '0'...'9' => {
                while (_.pos < _.src.len and _.src[_.pos] >= '0' and _.src[_.pos] <= '9') _.pos += 1;
                let is_float = false;
                if (_.pos < _.src.len and _.src[_.pos] == '.') {
                    is_float = true;
                    _.pos += 1;
                    if (_.pos >= _.src.len or _.src[_.pos] < '0' or _.src[_.pos] > '9') return JqError.InvalidSyntax;
                    while (_.pos < _.src.len and _.src[_.pos] >= '0' and _.src[_.pos] <= '9') _.pos += 1;
                }
                const slice = _.src[start.._.pos];
                if (!is_float) {
                    const iv = std.fmt.parseInt(i64, slice, 10) catch return JqError.InvalidSyntax;
                    return .{ .kind = .Int, .start = start, .end = _.pos, .int_val = iv };
                } else {
                    const fv = std.fmt.parseFloat(f64, slice) catch return JqError.InvalidSyntax;
                    return .{ .kind = .Float, .start = start, .end = _.pos, .float_val = fv };
                }
            },
            else => {
                if (isIdentStart(c)) {
                    while (_.pos < _.src.len and isIdentCont(_.src[_.pos])) _.pos += 1;
                    return .{ .kind = .Ident, .start = start, .end = _.pos };
                }
                return JqError.InvalidSyntax;
            },
        }
    }

    fn advance(*_): JqError!void {
        _.cur = try _.nextToken();
    }

    fn tokText(t: Token): []const u8 {
        return _.src[t.start..t.end];
    }

    fn parsePipe(*_): JqError!u32 {
        let left = try _.parseComma();
        while (_.cur.kind == .Pipe) {
            try _.advance();
            const right = try _.parseComma();
            left = try _.addNode(.{ .kind = .Pipe, .lhs = left, .rhs = right });
        }
        return left;
    }

    fn parseComma(*_): JqError!u32 {
        let left = try _.parseAlt();
        while (_.cur.kind == .Comma) {
            try _.advance();
            const right = try _.parseAlt();
            left = try _.addNode(.{ .kind = .Comma, .lhs = left, .rhs = right });
        }
        return left;
    }

    fn parseAlt(*_): JqError!u32 {
        let left = try _.parseOr();
        while (_.cur.kind == .SlashSlash) {
            try _.advance();
            const right = try _.parseOr();
            left = try _.addNode(.{ .kind = .BinOp, .bin_op = .Alt, .lhs = left, .rhs = right });
        }
        return left;
    }

    fn isIdentEq(s: []const u8): bool {
        return _.cur.kind == .Ident and std.mem.eql(u8, _.tokText(_.cur), s);
    }

    fn parseOr(*_): JqError!u32 {
        let left = try _.parseAnd();
        while (_.isIdentEq("or")) {
            try _.advance();
            const right = try _.parseAnd();
            left = try _.addNode(.{ .kind = .BinOp, .bin_op = .Or, .lhs = left, .rhs = right });
        }
        return left;
    }

    fn parseAnd(*_): JqError!u32 {
        let left = try _.parseCmp();
        while (_.isIdentEq("and")) {
            try _.advance();
            const right = try _.parseCmp();
            left = try _.addNode(.{ .kind = .BinOp, .bin_op = .And, .lhs = left, .rhs = right });
        }
        return left;
    }

    fn parseCmp(*_): JqError!u32 {
        let left = try _.parseAdd();
        while (true) {
            const op: JqBinOp? = switch (_.cur.kind) {
                .EqEq => .Eq,
                .NotEq => .Neq,
                .Lt => .Lt,
                .Le => .Le,
                .Gt => .Gt,
                .Ge => .Ge,
                else => null,
            };
            if (bop := op) {
                try _.advance();
                const right = try _.parseAdd();
                left = try _.addNode(.{ .kind = .BinOp, .bin_op = bop, .lhs = left, .rhs = right });
            } else {
                break;
            }
        }
        return left;
    }

    fn parseAdd(*_): JqError!u32 {
        let left = try _.parseMul();
        while (_.cur.kind == .Plus or _.cur.kind == .Minus) {
            const bop: JqBinOp = if (_.cur.kind == .Plus) .Add else .Sub;
            try _.advance();
            const right = try _.parseMul();
            left = try _.addNode(.{ .kind = .BinOp, .bin_op = bop, .lhs = left, .rhs = right });
        }
        return left;
    }

    fn parseMul(*_): JqError!u32 {
        let left = try _.parseUnary();
        while (_.cur.kind == .Star or _.cur.kind == .Slash) {
            const bop: JqBinOp = if (_.cur.kind == .Star) .Mul else .Div;
            try _.advance();
            const right = try _.parseUnary();
            left = try _.addNode(.{ .kind = .BinOp, .bin_op = bop, .lhs = left, .rhs = right });
        }
        return left;
    }

    fn parseUnary(*_): JqError!u32 {
        if (_.cur.kind == .Minus) {
            try _.advance();
            const inner = try _.parseUnary();
            return _.addNode(.{ .kind = .Neg, .lhs = inner });
        }
        return _.parsePostfix();
    }

    fn parseBracketPostfix(*_): JqError!u32 {
        try _.advance(); // '['
        if (_.cur.kind == .RBracket) {
            try _.advance();
            let opt = false;
            if (_.cur.kind == .Question) {
                opt = true;
                try _.advance();
            }
            return _.addNode(.{ .kind = .Iterate, .optional = opt });
        }

        // String key: .["foo"]
        if (_.cur.kind == .String) {
            const sidx = _.cur.str_idx;
            try _.advance();
            if (_.cur.kind != .RBracket) return JqError.InvalidSyntax;
            try _.advance();
            let opt = false;
            if (_.cur.kind == .Question) {
                opt = true;
                try _.advance();
            }
            return _.addNode(.{ .kind = .Field, .str_idx = sidx, .optional = opt });
        }

        // Slice or Index: .[start:end], .[:end], .[start:], .[idx]
        let has_start = false;
        let start_val: i64 = 0;
        if (_.cur.kind != .Colon) {
            let neg = false;
            if (_.cur.kind == .Minus) {
                neg = true;
                try _.advance();
            }
            if (_.cur.kind != .Int) return JqError.InvalidSyntax;
            start_val = if (neg) -_.cur.int_val else _.cur.int_val;
            has_start = true;
            try _.advance();
        }

        if (_.cur.kind == .Colon) {
            try _.advance();
            let has_end = false;
            let end_val: i64 = 0;
            if (_.cur.kind != .RBracket) {
                let neg = false;
                if (_.cur.kind == .Minus) {
                    neg = true;
                    try _.advance();
                }
                if (_.cur.kind != .Int) return JqError.InvalidSyntax;
                end_val = if (neg) -_.cur.int_val else _.cur.int_val;
                has_end = true;
                try _.advance();
            }
            if (_.cur.kind != .RBracket) return JqError.InvalidSyntax;
            try _.advance();
            let opt = false;
            if (_.cur.kind == .Question) {
                opt = true;
                try _.advance();
            }
            return _.addNode(.{
                .kind = .Slice,
                .has_start = has_start,
                .has_end = has_end,
                .slice_start = start_val,
                .slice_end = end_val,
                .optional = opt,
            });
        }

        if (!has_start or _.cur.kind != .RBracket) return JqError.InvalidSyntax;
        try _.advance();
        let opt = false;
        if (_.cur.kind == .Question) {
            opt = true;
            try _.advance();
        }
        return _.addNode(.{ .kind = .Index, .lit_int = start_val, .optional = opt });
    }

    fn parsePostfix(*_): JqError!u32 {
        let expr = try _.parsePrimary();
        while (true) {
            if (_.cur.kind == .Dot) {
                try _.advance();
                const step = try _.parseDotStep();
                expr = try _.addNode(.{ .kind = .Pipe, .lhs = expr, .rhs = step });
            } else if (_.cur.kind == .LBracket) {
                const step = try _.parseBracketPostfix();
                expr = try _.addNode(.{ .kind = .Pipe, .lhs = expr, .rhs = step });
            } else {
                break;
            }
        }
        return expr;
    }

    fn parseDotStep(*_): JqError!u32 {
        if (_.cur.kind == .Ident) {
            const sidx = try _.addString(_.tokText(_.cur));
            try _.advance();
            let opt = false;
            if (_.cur.kind == .Question) {
                opt = true;
                try _.advance();
            }
            return _.addNode(.{ .kind = .Field, .str_idx = sidx, .optional = opt });
        }
        if (_.cur.kind == .String) {
            const sidx = _.cur.str_idx;
            try _.advance();
            let opt = false;
            if (_.cur.kind == .Question) {
                opt = true;
                try _.advance();
            }
            return _.addNode(.{ .kind = .Field, .str_idx = sidx, .optional = opt });
        }
        if (_.cur.kind == .LBracket) {
            return _.parseBracketPostfix();
        }
        return JqError.InvalidSyntax;
    }

    fn parsePrimary(*_): JqError!u32 {
        if (_.depth >= MAX_AST_DEPTH) return JqError.MaxDepthExceeded;
        _.depth += 1;
        defer _.depth -= 1;

        switch (_.cur.kind) {
            .Dot => {
                try _.advance();
                if (_.cur.kind == .Ident or _.cur.kind == .String or _.cur.kind == .LBracket) {
                    return _.parseDotStep();
                }
                return _.addNode(.{ .kind = .Identity });
            },
            .Int => {
                const v = _.cur.int_val;
                try _.advance();
                return _.addNode(.{ .kind = .Literal, .lit_tag = .Int, .lit_int = v });
            },
            .Float => {
                const v = _.cur.float_val;
                try _.advance();
                return _.addNode(.{ .kind = .Literal, .lit_tag = .Float, .lit_float = v });
            },
            .String => {
                const sidx = _.cur.str_idx;
                try _.advance();
                return _.addNode(.{ .kind = .Literal, .lit_tag = .String, .str_idx = sidx });
            },
            .LParen => {
                try _.advance();
                const inner = try _.parsePipe();
                if (_.cur.kind != .RParen) return JqError.InvalidSyntax;
                try _.advance();
                return inner;
            },
            .LBracket => {
                try _.advance();
                if (_.cur.kind == .RBracket) {
                    try _.advance();
                    return _.addNode(.{ .kind = .ArrayConstruct, .lhs = INVALID_NODE });
                }
                const inner = try _.parsePipe();
                if (_.cur.kind != .RBracket) return JqError.InvalidSyntax;
                try _.advance();
                return _.addNode(.{ .kind = .ArrayConstruct, .lhs = inner });
            },
            .LBrace => {
                try _.advance();
                let local_fields = std.ArrayList(JqObjField).empty;
                defer local_fields.deinit(_.alloc);

                if (_.cur.kind != .RBrace) {
                    while (true) {
                        let f: JqObjField = .{};
                        if (_.cur.kind == .Ident) {
                            const key_name = _.tokText(_.cur);
                            const kidx = try _.addString(key_name);
                            try _.advance();
                            if (_.cur.kind == .Colon) {
                                try _.advance();
                                f.key_str_idx = kidx;
                                f.val_node = try _.parseAlt();
                            } else {
                                // Shorthand: {name} -> {"name": .name}
                                f.key_str_idx = kidx;
                                f.val_node = try _.addNode(.{ .kind = .Field, .str_idx = kidx });
                            }
                        } else if (_.cur.kind == .String) {
                            const kidx = _.cur.str_idx;
                            try _.advance();
                            if (_.cur.kind != .Colon) return JqError.InvalidSyntax;
                            try _.advance();
                            f.key_str_idx = kidx;
                            f.val_node = try _.parseAlt();
                        } else if (_.cur.kind == .LParen) {
                            try _.advance();
                            f.key_node = try _.parsePipe();
                            if (_.cur.kind != .RParen) return JqError.InvalidSyntax;
                            try _.advance();
                            if (_.cur.kind != .Colon) return JqError.InvalidSyntax;
                            try _.advance();
                            f.val_node = try _.parseAlt();
                        } else {
                            return JqError.InvalidSyntax;
                        }
                        local_fields.append(_.alloc, f) catch return JqError.OutOfMemory;
                        if (_.cur.kind == .Comma) {
                            try _.advance();
                            continue;
                        }
                        break;
                    }
                }
                if (_.cur.kind != .RBrace) return JqError.InvalidSyntax;
                try _.advance();

                const start_idx: u32 = @as(u32, @intCast(_.obj_fields.items.len));
                const flen: u32 = @as(u32, @intCast(local_fields.items.len));
                _.obj_fields.appendSlice(_.alloc, local_fields.items) catch return JqError.OutOfMemory;
                return _.addNode(.{ .kind = .ObjectConstruct, .field_start = start_idx, .field_len = flen });
            },
            .Ident => {
                const name = _.tokText(_.cur);
                try _.advance();
                if (std.mem.eql(u8, name, "null")) {
                    return _.addNode(.{ .kind = .Literal, .lit_tag = .Null });
                }
                if (std.mem.eql(u8, name, "true")) {
                    return _.addNode(.{ .kind = .Literal, .lit_tag = .Bool, .lit_bool = true });
                }
                if (std.mem.eql(u8, name, "false")) {
                    return _.addNode(.{ .kind = .Literal, .lit_tag = .Bool, .lit_bool = false });
                }
                if (std.mem.eql(u8, name, "not")) {
                    return _.addNode(.{ .kind = .Call, .builtin = .Not });
                }
                if (std.mem.eql(u8, name, "empty")) {
                    return _.addNode(.{ .kind = .Call, .builtin = .Empty });
                }
                if (std.mem.eql(u8, name, "length")) {
                    return _.addNode(.{ .kind = .Call, .builtin = .Length });
                }
                if (std.mem.eql(u8, name, "keys")) {
                    return _.addNode(.{ .kind = .Call, .builtin = .Keys });
                }
                if (std.mem.eql(u8, name, "values")) {
                    return _.addNode(.{ .kind = .Call, .builtin = .Values });
                }
                if (std.mem.eql(u8, name, "type")) {
                    return _.addNode(.{ .kind = .Call, .builtin = .Type });
                }
                if (std.mem.eql(u8, name, "reverse")) {
                    return _.addNode(.{ .kind = .Call, .builtin = .Reverse });
                }
                if (std.mem.eql(u8, name, "unique")) {
                    return _.addNode(.{ .kind = .Call, .builtin = .Unique });
                }
                if (std.mem.eql(u8, name, "sort")) {
                    return _.addNode(.{ .kind = .Call, .builtin = .Sort });
                }
                if (std.mem.eql(u8, name, "select") or std.mem.eql(u8, name, "map") or std.mem.eql(u8, name, "has") or std.mem.eql(u8, name, "contains")) {
                    if (_.cur.kind != .LParen) return JqError.InvalidSyntax;
                    try _.advance();
                    const arg = try _.parsePipe();
                    if (_.cur.kind != .RParen) return JqError.InvalidSyntax;
                    try _.advance();
                    const b: JqBuiltin = if (std.mem.eql(u8, name, "select"))
                        .Select
                    else if (std.mem.eql(u8, name, "map"))
                        .Map
                    else if (std.mem.eql(u8, name, "has"))
                        .Has
                    else
                        .Contains;
                    return _.addNode(.{ .kind = .Call, .builtin = b, .lhs = arg });
                }
                return JqError.InvalidSyntax;
            },
            else => return JqError.InvalidSyntax,
        }
    }
}

pub fn compileJq(alloc: std.mem.Allocator, filter: []const u8): JqError!JqExpr {
    let parser = JqParser{ .alloc = alloc, .src = filter };
    errdefer parser.deinitLists();

    try parser.advance();
    if (parser.cur.kind == .Eof) return JqError.InvalidSyntax;
    const root = try parser.parsePipe();
    if (parser.cur.kind != .Eof) return JqError.InvalidSyntax;

    const nodes_slice = parser.nodes.toOwnedSlice(alloc) catch return JqError.OutOfMemory;
    errdefer alloc.free(nodes_slice);
    const strings_slice = parser.strings.toOwnedSlice(alloc) catch return JqError.OutOfMemory;
    errdefer alloc.free(strings_slice);
    const fields_slice = parser.obj_fields.toOwnedSlice(alloc) catch return JqError.OutOfMemory;

    return .{
        .nodes = nodes_slice,
        .strings = strings_slice,
        .obj_fields = fields_slice,
        .root = root,
    };
}
