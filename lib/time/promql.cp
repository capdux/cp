// Source: native-cp-stdlib (PromQL Lexer & AST Parser)
import std from "std";

export type StrSlice = []const u8;

export error PromqlError {
    UnexpectedToken,
    UnexpectedEof,
    InvalidNumber,
    InvalidDuration,
    InvalidString,
    InvalidMatcher,
    InvalidModifier,
    UnmatchedParen,
    UnmatchedBrace,
    UnmatchedBracket,
    EmptyExpression,
    OutOfMemory,
}

export enum MatchOp {
    Eq,
    Neq,
    Regex,
    NotRegex,
}

export struct LabelMatcher {
    name: []const u8,
    op: MatchOp,
    value: []const u8,
}

export struct VectorSelector {
    name: []const u8,
    matchers: []LabelMatcher,
    offset_ms: i64?,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.matchers.len > 0) {
            alloc.free(_.matchers);
        }
    }
}

export struct MatrixSelector {
    vector: VectorSelector,
    range_ms: i64,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.vector.deinit(alloc);
    }
}

export enum AggregationOp {
    Sum,
    Avg,
    Min,
    Max,
    Count,
    Topk,
    Bottomk,
}

export struct AggregationExpr {
    op: AggregationOp,
    expr: *Expr,
    param: (*Expr)?,
    grouping: []StrSlice,
    without: bool,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.expr.deinit(alloc);
        alloc.destroy(_.expr);
        if (p := _.param) {
            p.deinit(alloc);
            alloc.destroy(p);
        }
        if (_.grouping.len > 0) {
            alloc.free(_.grouping);
        }
    }
}

export enum BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Pow,
    Eq,
    Neq,
    Lt,
    Lte,
    Gt,
    Gte,
    And,
    Or,
    Unless,
}

export enum VectorMatchCardinality {
    OneToOne,
    ManyToOne,
    OneToMany,
}

export struct VectorMatching {
    card: VectorMatchCardinality,
    matching_labels: []StrSlice,
    on_labels: bool,
    include_labels: []StrSlice,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.matching_labels.len > 0) {
            alloc.free(_.matching_labels);
        }
        if (_.include_labels.len > 0) {
            alloc.free(_.include_labels);
        }
    }
}

export struct BinaryExpr {
    op: BinaryOp,
    lhs: *Expr,
    rhs: *Expr,
    matching: VectorMatching?,
    return_bool: bool,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.lhs.deinit(alloc);
        alloc.destroy(_.lhs);
        _.rhs.deinit(alloc);
        alloc.destroy(_.rhs);
        if (m := &_.matching) {
            m.deinit(alloc);
        }
    }
}

export enum UnaryOp {
    Plus,
    Minus,
}

export struct UnaryExpr {
    op: UnaryOp,
    expr: *Expr,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.expr.deinit(alloc);
        alloc.destroy(_.expr);
    }
}

export struct CallExpr {
    func_name: []const u8,
    args: []Expr,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        for (arg of _.args) {
            let mut_arg = arg;
            mut_arg.deinit(alloc);
        }
        if (_.args.len > 0) {
            alloc.free(_.args);
        }
    }
}

export struct ParenExpr {
    expr: *Expr,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.expr.deinit(alloc);
        alloc.destroy(_.expr);
    }
}

export union Expr {
    NumberLiteral: f64,
    StringLiteral: []const u8,
    Vector: VectorSelector,
    Matrix: MatrixSelector,
    Aggregation: AggregationExpr,
    Binary: BinaryExpr,
    Unary: UnaryExpr,
    Call: CallExpr,
    Paren: ParenExpr,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.*) {
            .NumberLiteral => {},
            .StringLiteral => {},
            .Vector => |*v| v.deinit(alloc),
            .Matrix => |*m| m.deinit(alloc),
            .Aggregation => |*a| a.deinit(alloc),
            .Binary => |*b| b.deinit(alloc),
            .Unary => |*u| u.deinit(alloc),
            .Call => |*c| c.deinit(alloc),
            .Paren => |*p| p.deinit(alloc),
        }
    }
}

export fn parseDuration(s: []const u8) -> !i64 {
    if (s.len == 0) {
        return PromqlError.InvalidDuration;
    }
    let i: usize = 0;
    let total_ms: i64 = 0;
    while (i < s.len) {
        if (s[i] < '0' or s[i] > '9') {
            return PromqlError.InvalidDuration;
        }
        let val: i64 = 0;
        while (i < s.len and s[i] >= '0' and s[i] <= '9') {
            val = val * 10 + @as(i64, s[i] - '0');
            i += 1;
        }
        if (i >= s.len) {
            return PromqlError.InvalidDuration;
        }
        let unit_mult: i64 = 0;
        if (i + 1 < s.len and s[i] == 'm' and s[i + 1] == 's') {
            unit_mult = 1;
            i += 2;
        } else {
            const ch = s[i];
            i += 1;
            switch (ch) {
                's' => unit_mult = 1000,
                'm' => unit_mult = 60_000,
                'h' => unit_mult = 3_600_000,
                'd' => unit_mult = 86_400_000,
                'w' => unit_mult = 604_800_000,
                'y' => unit_mult = 31_536_000_000,
                else => return PromqlError.InvalidDuration,
            }
        }
        total_ms = total_ms + val * unit_mult;
    }
    return total_ms;
}

enum TokenKind {
    Eof,
    Identifier,
    Number,
    String,
    Duration,
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Comma,
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Caret,
    Eq,
    EqEq,
    Neq,
    EqTilde,
    NotTilde,
    Lt,
    Lte,
    Gt,
    Gte,
}

struct Token {
    kind: TokenKind,
    lexeme: []const u8,
    pos: usize,
}

fn isIdentStart(c: u8) -> bool {
    return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or c == '_' or c == ':';
}

fn isIdentChar(c: u8) -> bool {
    return isIdentStart(c) or (c >= '0' and c <= '9');
}

fn isDigit(c: u8) -> bool {
    return c >= '0' and c <= '9';
}

fn isDurationUnitStart(c: u8) -> bool {
    return c == 's' or c == 'm' or c == 'h' or c == 'd' or c == 'w' or c == 'y';
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let idx: usize = 0;
    while (idx < a.len) {
        let ca = a[idx];
        let cb = b[idx];
        if (ca >= 'A' and ca <= 'Z') {
            ca = ca + 32;
        }
        if (cb >= 'A' and cb <= 'Z') {
            cb = cb + 32;
        }
        if (ca != cb) {
            return false;
        }
        idx += 1;
    }
    return true;
}

struct Lexer {
    input: []const u8,
    pos: usize,

    pub static fn init(input: []const u8) -> Lexer {
        return Lexer{ input: input, pos: 0 };
    }

    fn skipWhitespaceAndComments(*_) {
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (c == ' ' or c == '\t' or c == '\n' or c == '\r') {
                _.pos += 1;
            } else if (c == '#') {
                while (_.pos < _.input.len and _.input[_.pos] != '\n') {
                    _.pos += 1;
                }
            } else {
                break;
            }
        }
    }

    pub fn nextToken(*_) -> !Token {
        _.skipWhitespaceAndComments();
        if (_.pos >= _.input.len) {
            return Token{ kind: .Eof, lexeme: "", pos: _.pos };
        }
        const start = _.pos;
        const c = _.input[_.pos];

        if (c == '(') {
            _.pos += 1;
            return Token{ kind: .LParen, lexeme: _.input[start.._.pos], pos: start };
        }
        if (c == ')') {
            _.pos += 1;
            return Token{ kind: .RParen, lexeme: _.input[start.._.pos], pos: start };
        }
        if (c == '{') {
            _.pos += 1;
            return Token{ kind: .LBrace, lexeme: _.input[start.._.pos], pos: start };
        }
        if (c == '}') {
            _.pos += 1;
            return Token{ kind: .RBrace, lexeme: _.input[start.._.pos], pos: start };
        }
        if (c == '[') {
            _.pos += 1;
            return Token{ kind: .LBracket, lexeme: _.input[start.._.pos], pos: start };
        }
        if (c == ']') {
            _.pos += 1;
            return Token{ kind: .RBracket, lexeme: _.input[start.._.pos], pos: start };
        }
        if (c == ',') {
            _.pos += 1;
            return Token{ kind: .Comma, lexeme: _.input[start.._.pos], pos: start };
        }
        if (c == '+') {
            _.pos += 1;
            return Token{ kind: .Plus, lexeme: _.input[start.._.pos], pos: start };
        }
        if (c == '-') {
            _.pos += 1;
            return Token{ kind: .Minus, lexeme: _.input[start.._.pos], pos: start };
        }
        if (c == '*') {
            _.pos += 1;
            return Token{ kind: .Star, lexeme: _.input[start.._.pos], pos: start };
        }
        if (c == '/') {
            _.pos += 1;
            return Token{ kind: .Slash, lexeme: _.input[start.._.pos], pos: start };
        }
        if (c == '%') {
            _.pos += 1;
            return Token{ kind: .Percent, lexeme: _.input[start.._.pos], pos: start };
        }
        if (c == '^') {
            _.pos += 1;
            return Token{ kind: .Caret, lexeme: _.input[start.._.pos], pos: start };
        }

        if (c == '=') {
            _.pos += 1;
            if (_.pos < _.input.len) {
                if (_.input[_.pos] == '=') {
                    _.pos += 1;
                    return Token{ kind: .EqEq, lexeme: _.input[start.._.pos], pos: start };
                }
                if (_.input[_.pos] == '~') {
                    _.pos += 1;
                    return Token{ kind: .EqTilde, lexeme: _.input[start.._.pos], pos: start };
                }
            }
            return Token{ kind: .Eq, lexeme: _.input[start.._.pos], pos: start };
        }

        if (c == '!') {
            _.pos += 1;
            if (_.pos < _.input.len) {
                if (_.input[_.pos] == '=') {
                    _.pos += 1;
                    return Token{ kind: .Neq, lexeme: _.input[start.._.pos], pos: start };
                }
                if (_.input[_.pos] == '~') {
                    _.pos += 1;
                    return Token{ kind: .NotTilde, lexeme: _.input[start.._.pos], pos: start };
                }
            }
            return PromqlError.UnexpectedToken;
        }

        if (c == '<') {
            _.pos += 1;
            if (_.pos < _.input.len and _.input[_.pos] == '=') {
                _.pos += 1;
                return Token{ kind: .Lte, lexeme: _.input[start.._.pos], pos: start };
            }
            return Token{ kind: .Lt, lexeme: _.input[start.._.pos], pos: start };
        }

        if (c == '>') {
            _.pos += 1;
            if (_.pos < _.input.len and _.input[_.pos] == '=') {
                _.pos += 1;
                return Token{ kind: .Gte, lexeme: _.input[start.._.pos], pos: start };
            }
            return Token{ kind: .Gt, lexeme: _.input[start.._.pos], pos: start };
        }

        if (c == '"' or c == '\'' or c == '`') {
            const quote = c;
            _.pos += 1;
            const content_start = _.pos;
            while (_.pos < _.input.len) {
                const sc = _.input[_.pos];
                if (sc == '\\' and quote != '`') {
                    _.pos += 2;
                    continue;
                }
                if (sc == quote) {
                    const content_end = _.pos;
                    _.pos += 1;
                    return Token{ kind: .String, lexeme: _.input[content_start..content_end], pos: start };
                }
                _.pos += 1;
            }
            return PromqlError.InvalidString;
        }

        if (isDigit(c) or (c == '.' and _.pos + 1 < _.input.len and isDigit(_.input[_.pos + 1]))) {
            let has_dot = false;
            let has_exp = false;
            if (c == '0' and _.pos + 1 < _.input.len and (_.input[_.pos + 1] == 'x' or _.input[_.pos + 1] == 'X')) {
                _.pos += 2;
                while (_.pos < _.input.len) {
                    const hc = _.input[_.pos];
                    if (isDigit(hc) or (hc >= 'a' and hc <= 'f') or (hc >= 'A' and hc <= 'F')) {
                        _.pos += 1;
                    } else {
                        break;
                    }
                }
                return Token{ kind: .Number, lexeme: _.input[start.._.pos], pos: start };
            }

            while (_.pos < _.input.len and isDigit(_.input[_.pos])) {
                _.pos += 1;
            }
            if (_.pos < _.input.len and isDurationUnitStart(_.input[_.pos])) {
                while (_.pos < _.input.len and (isDigit(_.input[_.pos]) or isDurationUnitStart(_.input[_.pos]))) {
                    _.pos += 1;
                }
                return Token{ kind: .Duration, lexeme: _.input[start.._.pos], pos: start };
            }
            if (_.pos < _.input.len and _.input[_.pos] == '.') {
                has_dot = true;
                _.pos += 1;
                while (_.pos < _.input.len and isDigit(_.input[_.pos])) {
                    _.pos += 1;
                }
            }
            if (_.pos < _.input.len and (_.input[_.pos] == 'e' or _.input[_.pos] == 'E')) {
                has_exp = true;
                _.pos += 1;
                if (_.pos < _.input.len and (_.input[_.pos] == '+' or _.input[_.pos] == '-')) {
                    _.pos += 1;
                }
                if (_.pos >= _.input.len or !isDigit(_.input[_.pos])) {
                    return PromqlError.InvalidNumber;
                }
                while (_.pos < _.input.len and isDigit(_.input[_.pos])) {
                    _.pos += 1;
                }
            }
            * = has_dot;
            * = has_exp;
            return Token{ kind: .Number, lexeme: _.input[start.._.pos], pos: start };
        }

        if (isIdentStart(c)) {
            while (_.pos < _.input.len and isIdentChar(_.input[_.pos])) {
                _.pos += 1;
            }
            return Token{ kind: .Identifier, lexeme: _.input[start.._.pos], pos: start };
        }

        return PromqlError.UnexpectedToken;
    }
}

fn matchAggregationOp(name: []const u8) -> AggregationOp? {
    if (asciiEqlIgnoreCase(name, "sum")) { return .Sum; }
    if (asciiEqlIgnoreCase(name, "avg")) { return .Avg; }
    if (asciiEqlIgnoreCase(name, "min")) { return .Min; }
    if (asciiEqlIgnoreCase(name, "max")) { return .Max; }
    if (asciiEqlIgnoreCase(name, "count")) { return .Count; }
    if (asciiEqlIgnoreCase(name, "topk")) { return .Topk; }
    if (asciiEqlIgnoreCase(name, "bottomk")) { return .Bottomk; }
    return null;
}

fn aggNeedsParam(op: AggregationOp) -> bool {
    return op == .Topk or op == .Bottomk;
}

fn binaryOpFromToken(tok: Token) -> BinaryOp? {
    switch (tok.kind) {
        .Caret => return .Pow,
        .Star => return .Mul,
        .Slash => return .Div,
        .Percent => return .Mod,
        .Plus => return .Add,
        .Minus => return .Sub,
        .EqEq => return .Eq,
        .Neq => return .Neq,
        .Lt => return .Lt,
        .Lte => return .Lte,
        .Gt => return .Gt,
        .Gte => return .Gte,
        .Identifier => {
            if (asciiEqlIgnoreCase(tok.lexeme, "and")) { return .And; }
            if (asciiEqlIgnoreCase(tok.lexeme, "or")) { return .Or; }
            if (asciiEqlIgnoreCase(tok.lexeme, "unless")) { return .Unless; }
            return null;
        },
        else => return null,
    }
}

fn binaryPrecedence(op: BinaryOp) -> u8 {
    switch (op) {
        .Pow => return 6,
        .Mul, .Div, .Mod => return 5,
        .Add, .Sub => return 4,
        .Eq, .Neq, .Lt, .Lte, .Gt, .Gte => return 3,
        .And, .Unless => return 2,
        .Or => return 1,
    }
}

fn isRightAssoc(op: BinaryOp) -> bool {
    return op == .Pow;
}

fn isComparisonOp(op: BinaryOp) -> bool {
    switch (op) {
        .Eq, .Neq, .Lt, .Lte, .Gt, .Gte => return true,
        else => return false,
    }
}

struct Parser {
    lexer: Lexer,
    curr: Token,
    alloc: std.mem.Allocator,

    pub static fn init(alloc: std.mem.Allocator, input: []const u8) -> !Parser {
        let lex = Lexer.init(input);
        const first = lex.nextToken()?;
        return Parser{
            lexer: lex,
            curr: first,
            alloc: alloc,
        };
    }

    fn advance(*_) -> !Token {
        const prev = _.curr;
        _.curr = _.lexer.nextToken()?;
        return prev;
    }

    fn expect(*_, kind: TokenKind) -> !Token {
        if (_.curr.kind != kind) {
            switch (kind) {
                .RParen => return PromqlError.UnmatchedParen,
                .RBrace => return PromqlError.UnmatchedBrace,
                .RBracket => return PromqlError.UnmatchedBracket,
                else => return PromqlError.UnexpectedToken,
            }
        }
        return _.advance();
    }

    fn allocExpr(*_, val: Expr) -> !*Expr {
        let ptr = _.alloc.create(Expr) catch return PromqlError.OutOfMemory;
        ptr.* = val;
        return ptr;
    }

    pub fn parseRoot(*_) -> !Expr {
        if (_.curr.kind == .Eof) {
            return PromqlError.EmptyExpression;
        }
        let expr = _.parseExpr(1)?;
        errdefer expr.deinit(_.alloc);
        if (_.curr.kind != .Eof) {
            return PromqlError.UnexpectedToken;
        }
        return expr;
    }

    fn parseExpr(*_, min_prec: u8) -> !Expr {
        let lhs = _.parseUnary()?;
        errdefer lhs.deinit(_.alloc);

        while (true) {
            const op_opt = binaryOpFromToken(_.curr);
            if (op_opt == null) {
                break;
            }
            const op = op_opt.?;
            const prec = binaryPrecedence(op);
            if (prec < min_prec) {
                break;
            }

            * = _.advance()?;

            let return_bool = false;
            if (isComparisonOp(op) and _.curr.kind == .Identifier and asciiEqlIgnoreCase(_.curr.lexeme, "bool")) {
                return_bool = true;
                * = _.advance()?;
            }

            let matching = _.parseVectorMatching()?;
            errdefer {
                if (m := &matching) {
                    m.deinit(_.alloc);
                }
            }

            const next_min = if (isRightAssoc(op)) prec else prec + 1;
            let rhs = _.parseExpr(next_min)?;
            errdefer rhs.deinit(_.alloc);

            const lhs_ptr = _.allocExpr(lhs) catch {
                rhs.deinit(_.alloc);
                return PromqlError.OutOfMemory;
            };
            const rhs_ptr = _.allocExpr(rhs) catch {
                lhs_ptr.deinit(_.alloc);
                _.alloc.destroy(lhs_ptr);
                rhs.deinit(_.alloc);
                return PromqlError.OutOfMemory;
            };

            lhs = Expr{
                .Binary = BinaryExpr{
                    .op = op,
                    .lhs = lhs_ptr,
                    .rhs = rhs_ptr,
                    .matching = matching,
                    .return_bool = return_bool,
                },
            };
        }

        return lhs;
    }

    fn parseLabelList(*_) -> ![]StrSlice {
        * = _.expect(.LParen)?;
        let list = std.ArrayList(StrSlice).empty;
        errdefer list.deinit(_.alloc);

        if (_.curr.kind != .RParen) {
            while (true) {
                const tok = _.expect(.Identifier)?;
                list.append(_.alloc, tok.lexeme) catch return PromqlError.OutOfMemory;
                if (_.curr.kind == .Comma) {
                    * = _.advance()?;
                    if (_.curr.kind == .RParen) {
                        break;
                    }
                } else {
                    break;
                }
            }
        }
        * = _.expect(.RParen)?;
        return list.toOwnedSlice(_.alloc) catch return PromqlError.OutOfMemory;
    }

    fn parseVectorMatching(*_) -> !VectorMatching? {
        let has_matching = false;
        let on_labels = false;
        let matching_labels: []StrSlice = [];
        errdefer {
            if (matching_labels.len > 0) {
                _.alloc.free(matching_labels);
            }
        }

        if (_.curr.kind == .Identifier) {
            if (asciiEqlIgnoreCase(_.curr.lexeme, "on")) {
                has_matching = true;
                on_labels = true;
                * = _.advance()?;
                matching_labels = _.parseLabelList()?;
            } else if (asciiEqlIgnoreCase(_.curr.lexeme, "ignoring")) {
                has_matching = true;
                on_labels = false;
                * = _.advance()?;
                matching_labels = _.parseLabelList()?;
            }
        }

        let card = VectorMatchCardinality.OneToOne;
        let include_labels: []StrSlice = [];
        errdefer {
            if (include_labels.len > 0) {
                _.alloc.free(include_labels);
            }
        }

        if (_.curr.kind == .Identifier) {
            if (asciiEqlIgnoreCase(_.curr.lexeme, "group_left")) {
                has_matching = true;
                card = .ManyToOne;
                * = _.advance()?;
                if (_.curr.kind == .LParen) {
                    include_labels = _.parseLabelList()?;
                }
            } else if (asciiEqlIgnoreCase(_.curr.lexeme, "group_right")) {
                has_matching = true;
                card = .OneToMany;
                * = _.advance()?;
                if (_.curr.kind == .LParen) {
                    include_labels = _.parseLabelList()?;
                }
            }
        }

        if (!has_matching) {
            return null;
        }

        return VectorMatching{
            .card = card,
            .matching_labels = matching_labels,
            .on_labels = on_labels,
            .include_labels = include_labels,
        };
    }

    fn parseUnary(*_) -> !Expr {
        if (_.curr.kind == .Plus or _.curr.kind == .Minus) {
            const op = if (_.curr.kind == .Plus) UnaryOp.Plus else UnaryOp.Minus;
            * = _.advance()?;
            let inner = _.parseUnary()?;
            errdefer inner.deinit(_.alloc);
            const ptr = _.allocExpr(inner)?;
            return Expr{
                .Unary = UnaryExpr{
                    .op = op,
                    .expr = ptr,
                },
            };
        }
        return _.parsePrimary();
    }

    fn parsePrimary(*_) -> !Expr {
        switch (_.curr.kind) {
            .Number => {
                const tok = _.advance()?;
                const val = std.fmt.parseFloat(f64, tok.lexeme) catch return PromqlError.InvalidNumber;
                return Expr{ .NumberLiteral = val };
            },
            .String => {
                const tok = _.advance()?;
                return Expr{ .StringLiteral = tok.lexeme };
            },
            .LParen => {
                * = _.advance()?;
                let inner = _.parseExpr(1)?;
                errdefer inner.deinit(_.alloc);
                * = _.expect(.RParen)?;
                const ptr = _.allocExpr(inner)?;
                return Expr{ .Paren = ParenExpr{ .expr = ptr } };
            },
            .LBrace => {
                return _.parseVectorOrMatrix("");
            },
            .Identifier => {
                const ident = _.advance()?;
                if (agg_op := matchAggregationOp(ident.lexeme)) {
                    return _.parseAggregation(agg_op);
                }
                if (_.curr.kind == .LParen) {
                    return _.parseCall(ident.lexeme);
                }
                return _.parseVectorOrMatrix(ident.lexeme);
            },
            .Eof => return PromqlError.UnexpectedEof,
            else => return PromqlError.UnexpectedToken,
        }
    }

    fn parseAggregation(*_, op: AggregationOp) -> !Expr {
        let grouping: []StrSlice = [];
        let without = false;
        let has_pre_grouping = false;
        errdefer {
            if (grouping.len > 0) {
                _.alloc.free(grouping);
            }
        }

        if (_.curr.kind == .Identifier) {
            if (asciiEqlIgnoreCase(_.curr.lexeme, "by")) {
                has_pre_grouping = true;
                without = false;
                * = _.advance()?;
                grouping = _.parseLabelList()?;
            } else if (asciiEqlIgnoreCase(_.curr.lexeme, "without")) {
                has_pre_grouping = true;
                without = true;
                * = _.advance()?;
                grouping = _.parseLabelList()?;
            }
        }

        * = _.expect(.LParen)?;

        let param_ptr: (*Expr)? = null;
        errdefer {
            if (p := param_ptr) {
                p.deinit(_.alloc);
                _.alloc.destroy(p);
            }
        }

        if (aggNeedsParam(op)) {
            let p_expr = _.parseExpr(1)?;
            errdefer p_expr.deinit(_.alloc);
            param_ptr = _.allocExpr(p_expr)?;
            * = _.expect(.Comma)?;
        }

        let target = _.parseExpr(1)?;
        errdefer target.deinit(_.alloc);
        * = _.expect(.RParen)?;

        if (!has_pre_grouping and _.curr.kind == .Identifier) {
            if (asciiEqlIgnoreCase(_.curr.lexeme, "by")) {
                without = false;
                * = _.advance()?;
                grouping = _.parseLabelList()?;
            } else if (asciiEqlIgnoreCase(_.curr.lexeme, "without")) {
                without = true;
                * = _.advance()?;
                grouping = _.parseLabelList()?;
            }
        }

        const target_ptr = _.allocExpr(target)?;
        return Expr{
            .Aggregation = AggregationExpr{
                .op = op,
                .expr = target_ptr,
                .param = param_ptr,
                .grouping = grouping,
                .without = without,
            },
        };
    }

    fn parseCall(*_, func_name: []const u8) -> !Expr {
        * = _.expect(.LParen)?;
        let args = std.ArrayList(Expr).empty;
        errdefer {
            for (arg of args.items) {
                let mut_arg = arg;
                mut_arg.deinit(_.alloc);
            }
            args.deinit(_.alloc);
        }

        if (_.curr.kind != .RParen) {
            while (true) {
                let arg = _.parseExpr(1)?;
                args.append(_.alloc, arg) catch {
                    arg.deinit(_.alloc);
                    return PromqlError.OutOfMemory;
                };
                if (_.curr.kind == .Comma) {
                    * = _.advance()?;
                    if (_.curr.kind == .RParen) {
                        break;
                    }
                } else {
                    break;
                }
            }
        }
        * = _.expect(.RParen)?;
        const slice = args.toOwnedSlice(_.alloc) catch return PromqlError.OutOfMemory;
        return Expr{
            .Call = CallExpr{
                .func_name = func_name,
                .args = slice,
            },
        };
    }

    fn parseVectorOrMatrix(*_, metric_name: []const u8) -> !Expr {
        let matchers = std.ArrayList(LabelMatcher).empty;
        errdefer matchers.deinit(_.alloc);

        if (_.curr.kind == .LBrace) {
            * = _.advance()?;
            if (_.curr.kind != .RBrace) {
                while (true) {
                    const label_tok = _.expect(.Identifier)?;
                    let op = MatchOp.Eq;
                    if (_.curr.kind == .Eq) {
                        op = .Eq;
                    } else if (_.curr.kind == .Neq) {
                        op = .Neq;
                    } else if (_.curr.kind == .EqTilde) {
                        op = .Regex;
                    } else if (_.curr.kind == .NotTilde) {
                        op = .NotRegex;
                    } else {
                        return PromqlError.InvalidMatcher;
                    }
                    * = _.advance()?;
                    const val_tok = _.expect(.String)?;
                    matchers.append(_.alloc, LabelMatcher{
                        .name = label_tok.lexeme,
                        .op = op,
                        .value = val_tok.lexeme,
                    }) catch return PromqlError.OutOfMemory;

                    if (_.curr.kind == .Comma) {
                        * = _.advance()?;
                        if (_.curr.kind == .RBrace) {
                            break;
                        }
                    } else {
                        break;
                    }
                }
            }
            * = _.expect(.RBrace)?;
        }

        const slice = matchers.toOwnedSlice(_.alloc) catch return PromqlError.OutOfMemory;
        errdefer {
            if (slice.len > 0) {
                _.alloc.free(slice);
            }
        }

        if (_.curr.kind == .LBracket) {
            * = _.advance()?;
            if (_.curr.kind != .Duration) {
                return PromqlError.InvalidDuration;
            }
            const dur_tok = _.advance()?;
            const range_ms = parseDuration(dur_tok.lexeme)?;
            * = _.expect(.RBracket)?;

            let offset_ms: i64? = null;
            if (_.curr.kind == .Identifier and asciiEqlIgnoreCase(_.curr.lexeme, "offset")) {
                * = _.advance()?;
                if (_.curr.kind != .Duration) {
                    return PromqlError.InvalidDuration;
                }
                const off_tok = _.advance()?;
                offset_ms = parseDuration(off_tok.lexeme)?;
            }

            return Expr{
                .Matrix = MatrixSelector{
                    .vector = VectorSelector{
                        .name = metric_name,
                        .matchers = slice,
                        .offset_ms = offset_ms,
                    },
                    .range_ms = range_ms,
                },
            };
        }

        let offset_ms: i64? = null;
        if (_.curr.kind == .Identifier and asciiEqlIgnoreCase(_.curr.lexeme, "offset")) {
            * = _.advance()?;
            if (_.curr.kind != .Duration) {
                return PromqlError.InvalidDuration;
            }
            const off_tok = _.advance()?;
            offset_ms = parseDuration(off_tok.lexeme)?;
        }

        return Expr{
            .Vector = VectorSelector{
                .name = metric_name,
                .matchers = slice,
                .offset_ms = offset_ms,
            },
        };
    }
}

export fn parse(alloc: std.mem.Allocator, input: []const u8) -> !Expr {
    let parser = Parser.init(alloc, input)?;
    return parser.parseRoot();
}
