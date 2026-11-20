// Source: cp/lib/jsonpath/ast.cp (RFC 9535 JSONPath AST & Compiler)

const std = @import("std");

pub enum JsonPathError {
    EmptyQuery,
    MissingRootIdentifier,
    InvalidSyntax,
    UnterminatedBracket,
    UnterminatedString,
    InvalidEscape,
    InvalidSlice,
    ZeroSliceStep,
    InvalidFilter,
    UnknownFunction,
    OutOfMemory,
}

pub enum SegmentKind {
    Child,
    Descendant,
}

pub enum SelectorKind {
    Name,
    Wildcard,
    Index,
    Slice,
    Filter,
}

pub struct SliceSpec {
    pub start: i64?,
    pub end: i64?,
    pub step: i64,
}

pub enum FilterExprKind {
    Or,
    And,
    Not,
    Cmp,
    TestQuery,
    BoolLit,
}

pub enum CmpOp {
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
}

pub enum FilterValKind {
    Null,
    Bool,
    Number,
    String,
    Query,
    FuncLength,
    FuncCount,
    FuncValue,
}

pub struct FilterQuery {
    pub is_root: bool,
    pub segments: []Segment,
}

pub struct FilterVal {
    pub kind: FilterValKind,
    pub bool_val: bool,
    pub num_val: f64,
    pub str_val: []const u8,
    pub query: (FilterQuery)?,
}

pub struct FilterExpr {
    pub kind: FilterExprKind,
    pub bool_val: bool,
    pub op: CmpOp,
    pub left_expr: (*FilterExpr)?,
    pub right_expr: (*FilterExpr)?,
    pub left_val: (FilterVal)?,
    pub right_val: (FilterVal)?,
    pub test_query: (FilterQuery)?,
}

pub struct Selector {
    pub kind: SelectorKind,
    pub name: []const u8,
    pub index: i64,
    pub slice: SliceSpec,
    pub filter: (*FilterExpr)?,
}

pub struct Segment {
    pub kind: SegmentKind,
    pub selectors: []Selector,
}

pub struct JsonPathQuery {
    pub segments: []Segment,
}

fn deinitFilterVal(v: *FilterVal, alloc: std.mem.Allocator) {
    if (v.kind == FilterValKind.String and v.str_val.len > 0) {
        alloc.free(v.str_val);
    }
    if (v.query != null) {
        let q = v.query.?;
        deinitSegments(q.segments, alloc);
    }
}

fn deinitFilterExpr(expr: *FilterExpr, alloc: std.mem.Allocator) {
    if (expr.left_expr != null) {
        deinitFilterExpr(expr.left_expr.?, alloc);
    }
    if (expr.right_expr != null) {
        deinitFilterExpr(expr.right_expr.?, alloc);
    }
    if (expr.left_val != null) {
        let lv = expr.left_val.?;
        deinitFilterVal(&lv, alloc);
    }
    if (expr.right_val != null) {
        let rv = expr.right_val.?;
        deinitFilterVal(&rv, alloc);
    }
    if (expr.test_query != null) {
        let tq = expr.test_query.?;
        deinitSegments(tq.segments, alloc);
    }
    alloc.destroy(expr);
}

fn deinitSegments(segs: []Segment, alloc: std.mem.Allocator) {
    for (seg of segs) {
        for (sel of seg.selectors) {
            if (sel.kind == SelectorKind.Name and sel.name.len > 0) {
                alloc.free(sel.name);
            }
            if (sel.filter != null) {
                deinitFilterExpr(sel.filter.?, alloc);
            }
        }
        if (seg.selectors.len > 0) {
            alloc.free(seg.selectors);
        }
    }
    if (segs.len > 0) {
        alloc.free(segs);
    }
}

pub fn deinitJsonPathQuery(q: *JsonPathQuery, alloc: std.mem.Allocator) {
    deinitSegments(q.segments, alloc);
}

struct Parser {
    src: []const u8,
    pos: usize,
    alloc: std.mem.Allocator,
}

fn skipWs(p: *Parser) {
    while (p.pos < p.src.len) {
        const c = p.src[p.pos];
        if (c == ' ' or c == '\t' or c == '\n' or c == '\r') {
            p.pos += 1;
        } else {
            break;
        }
    }
}

fn isNameFirst(c: u8): bool {
    return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or c == '_' or c >= 0x80;
}

fn isNameChar(c: u8): bool {
    return isNameFirst(c) or (c >= '0' and c <= '9');
}

fn parseMemberNameShorthand(p: *Parser): JsonPathError![]const u8 {
    if (p.pos >= p.src.len or !isNameFirst(p.src[p.pos])) {
        return JsonPathError.InvalidSyntax;
    }
    const start = p.pos;
    p.pos += 1;
    while (p.pos < p.src.len and isNameChar(p.src[p.pos])) {
        p.pos += 1;
    }
    const copy = try p.alloc.alloc(u8, p.pos - start);
    @memcpy(copy, p.src[start..p.pos]);
    return copy;
}

fn parseHex4(s: []const u8): u21? {
    if (s.len < 4) {
        return null;
    }
    let val: u21 = 0;
    let i: usize = 0;
    while (i < 4) {
        const c = s[i];
        let d: u21 = 0;
        if (c >= '0' and c <= '9') {
            d = @as(u21, c - '0');
        } else if (c >= 'a' and c <= 'f') {
            d = @as(u21, c - 'a' + 10);
        } else if (c >= 'A' and c <= 'F') {
            d = @as(u21, c - 'A' + 10);
        } else {
            return null;
        }
        val = (val << 4) | d;
        i += 1;
    }
    return val;
}

fn parseQuotedString(p: *Parser): JsonPathError![]const u8 {
    if (p.pos >= p.src.len) {
        return JsonPathError.UnterminatedString;
    }
    const quote = p.src[p.pos];
    if (quote != '\'' and quote != '"') {
        return JsonPathError.InvalidSyntax;
    }
    p.pos += 1;
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(p.alloc);

    while (p.pos < p.src.len) {
        const c = p.src[p.pos];
        if (c == quote) {
            p.pos += 1;
            return try buf.toOwnedSlice(p.alloc);
        }
        if (c == '\\') {
            p.pos += 1;
            if (p.pos >= p.src.len) {
                return JsonPathError.UnterminatedString;
            }
            const esc = p.src[p.pos];
            p.pos += 1;
            switch (esc) {
                '\'' => try buf.append(p.alloc, '\''),
                '"' => try buf.append(p.alloc, '"'),
                '\\' => try buf.append(p.alloc, '\\'),
                '/' => try buf.append(p.alloc, '/'),
                'b' => try buf.append(p.alloc, 0x08),
                'f' => try buf.append(p.alloc, 0x0c),
                'n' => try buf.append(p.alloc, '\n'),
                'r' => try buf.append(p.alloc, '\r'),
                't' => try buf.append(p.alloc, '\t'),
                'u' => {
                    if (p.pos + 4 > p.src.len) {
                        return JsonPathError.InvalidEscape;
                    }
                    const cp_opt = parseHex4(p.src[p.pos .. p.pos + 4]);
                    if (cp_opt == null) {
                        return JsonPathError.InvalidEscape;
                    }
                    let cp = cp_opt.?;
                    p.pos += 4;
                    if (cp >= 0xD800 and cp <= 0xDBFF) {
                        if (p.pos + 6 <= p.src.len and p.src[p.pos] == '\\' and p.src[p.pos + 1] == 'u') {
                            const low_opt = parseHex4(p.src[p.pos + 2 .. p.pos + 6]);
                            if (low_opt != null and low_opt.? >= 0xDC00 and low_opt.? <= 0xDFFF) {
                                cp = 0x10000 + (((cp - 0xD800) << 10) | (low_opt.? - 0xDC00));
                                p.pos += 6;
                            }
                        }
                    }
                    let utf8_buf: [4]u8 = undefined;
                    const len = std.unicode.utf8Encode(cp, &utf8_buf) catch return JsonPathError.InvalidEscape;
                    try buf.appendSlice(p.alloc, utf8_buf[0..len]);
                },
                else => return JsonPathError.InvalidEscape,
            }
        } else {
            try buf.append(p.alloc, c);
            p.pos += 1;
        }
    }
    return JsonPathError.UnterminatedString;
}

fn parseInt64(p: *Parser): JsonPathError!i64 {
    const start = p.pos;
    if (p.pos < p.src.len and p.src[p.pos] == '-') {
        p.pos += 1;
    }
    if (p.pos >= p.src.len or p.src[p.pos] < '0' or p.src[p.pos] > '9') {
        return JsonPathError.InvalidSyntax;
    }
    if (p.src[p.pos] == '0') {
        p.pos += 1;
        if (p.pos < p.src.len and p.src[p.pos] >= '0' and p.src[p.pos] <= '9') {
            return JsonPathError.InvalidSyntax;
        }
    } else {
        while (p.pos < p.src.len and p.src[p.pos] >= '0' and p.src[p.pos] <= '9') {
            p.pos += 1;
        }
    }
    return std.fmt.parseInt(i64, p.src[start..p.pos], 10) catch return JsonPathError.InvalidSyntax;
}

fn parseNumberLit(p: *Parser): JsonPathError!f64 {
    const start = p.pos;
    if (p.pos < p.src.len and p.src[p.pos] == '-') {
        p.pos += 1;
    }
    if (p.pos >= p.src.len or p.src[p.pos] < '0' or p.src[p.pos] > '9') {
        return JsonPathError.InvalidFilter;
    }
    while (p.pos < p.src.len and p.src[p.pos] >= '0' and p.src[p.pos] <= '9') {
        p.pos += 1;
    }
    if (p.pos < p.src.len and p.src[p.pos] == '.') {
        p.pos += 1;
        if (p.pos >= p.src.len or p.src[p.pos] < '0' or p.src[p.pos] > '9') {
            return JsonPathError.InvalidFilter;
        }
        while (p.pos < p.src.len and p.src[p.pos] >= '0' and p.src[p.pos] <= '9') {
            p.pos += 1;
        }
    }
    if (p.pos < p.src.len and (p.src[p.pos] == 'e' or p.src[p.pos] == 'E')) {
        p.pos += 1;
        if (p.pos < p.src.len and (p.src[p.pos] == '+' or p.src[p.pos] == '-')) {
            p.pos += 1;
        }
        if (p.pos >= p.src.len or p.src[p.pos] < '0' or p.src[p.pos] > '9') {
            return JsonPathError.InvalidFilter;
        }
        while (p.pos < p.src.len and p.src[p.pos] >= '0' and p.src[p.pos] <= '9') {
            p.pos += 1;
        }
    }
    return std.fmt.parseFloat(f64, p.src[start..p.pos]) catch return JsonPathError.InvalidFilter;
}

fn parseFilterQuery(p: *Parser): JsonPathError!FilterQuery {
    if (p.pos >= p.src.len) {
        return JsonPathError.InvalidFilter;
    }
    let is_root = false;
    if (p.src[p.pos] == '$') {
        is_root = true;
    } else if (p.src[p.pos] == '@') {
        is_root = false;
    } else {
        return JsonPathError.InvalidFilter;
    }
    p.pos += 1;
    const segs = try parseSegments(p, true);
    return FilterQuery{
        .is_root = is_root,
        .segments = segs,
    };
}

fn parseFilterVal(p: *Parser): JsonPathError!FilterVal {
    skipWs(p);
    if (p.pos >= p.src.len) {
        return JsonPathError.InvalidFilter;
    }
    const c = p.src[p.pos];
    if (c == '\'' or c == '"') {
        const s = try parseQuotedString(p);
        return FilterVal{
            .kind = FilterValKind.String,
            .bool_val = false,
            .num_val = 0.0,
            .str_val = s,
            .query = null,
        };
    }
    if (c == '-' or (c >= '0' and c <= '9')) {
        const num = try parseNumberLit(p);
        return FilterVal{
            .kind = FilterValKind.Number,
            .bool_val = false,
            .num_val = num,
            .str_val = "",
            .query = null,
        };
    }
    if (c == '$' or c == '@') {
        const q = try parseFilterQuery(p);
        return FilterVal{
            .kind = FilterValKind.Query,
            .bool_val = false,
            .num_val = 0.0,
            .str_val = "",
            .query = q,
        };
    }
    if (p.pos + 4 <= p.src.len and std.mem.eql(u8, p.src[p.pos .. p.pos + 4], "true") and (p.pos + 4 == p.src.len or !isNameChar(p.src[p.pos + 4]))) {
        p.pos += 4;
        return FilterVal{
            .kind = FilterValKind.Bool,
            .bool_val = true,
            .num_val = 0.0,
            .str_val = "",
            .query = null,
        };
    }
    if (p.pos + 5 <= p.src.len and std.mem.eql(u8, p.src[p.pos .. p.pos + 5], "false") and (p.pos + 5 == p.src.len or !isNameChar(p.src[p.pos + 5]))) {
        p.pos += 5;
        return FilterVal{
            .kind = FilterValKind.Bool,
            .bool_val = false,
            .num_val = 0.0,
            .str_val = "",
            .query = null,
        };
    }
    if (p.pos + 4 <= p.src.len and std.mem.eql(u8, p.src[p.pos .. p.pos + 4], "null") and (p.pos + 4 == p.src.len or !isNameChar(p.src[p.pos + 4]))) {
        p.pos += 4;
        return FilterVal{
            .kind = FilterValKind.Null,
            .bool_val = false,
            .num_val = 0.0,
            .str_val = "",
            .query = null,
        };
    }
    if (isNameFirst(c)) {
        const fn_start = p.pos;
        while (p.pos < p.src.len and isNameChar(p.src[p.pos])) {
            p.pos += 1;
        }
        const fn_name = p.src[fn_start..p.pos];
        skipWs(p);
        if (p.pos >= p.src.len or p.src[p.pos] != '(') {
            return JsonPathError.InvalidFilter;
        }
        p.pos += 1;
        skipWs(p);
        let func_kind = FilterValKind.FuncLength;
        if (std.mem.eql(u8, fn_name, "length")) {
            func_kind = FilterValKind.FuncLength;
        } else if (std.mem.eql(u8, fn_name, "count")) {
            func_kind = FilterValKind.FuncCount;
        } else if (std.mem.eql(u8, fn_name, "value")) {
            func_kind = FilterValKind.FuncValue;
        } else {
            return JsonPathError.UnknownFunction;
        }
        const arg_q = try parseFilterQuery(p);
        errdefer {
            let tmp = arg_q;
            deinitSegments(tmp.segments, p.alloc);
        }
        skipWs(p);
        if (p.pos >= p.src.len or p.src[p.pos] != ')') {
            return JsonPathError.InvalidFilter;
        }
        p.pos += 1;
        return FilterVal{
            .kind = func_kind,
            .bool_val = false,
            .num_val = 0.0,
            .str_val = "",
            .query = arg_q,
        };
    }
    return JsonPathError.InvalidFilter;
}

fn allocFilterExpr(p: *Parser, val: FilterExpr): JsonPathError!*FilterExpr {
    let node = try p.alloc.create(FilterExpr);
    node.* = val;
    return node;
}

fn parseCmpOp(p: *Parser): CmpOp? {
    skipWs(p);
    if (p.pos + 2 <= p.src.len) {
        const two = p.src[p.pos .. p.pos + 2];
        if (std.mem.eql(u8, two, "==")) {
            p.pos += 2;
            return CmpOp.Eq;
        }
        if (std.mem.eql(u8, two, "!=")) {
            p.pos += 2;
            return CmpOp.Ne;
        }
        if (std.mem.eql(u8, two, "<=")) {
            p.pos += 2;
            return CmpOp.Le;
        }
        if (std.mem.eql(u8, two, ">=")) {
            p.pos += 2;
            return CmpOp.Ge;
        }
    }
    if (p.pos < p.src.len) {
        if (p.src[p.pos] == '<') {
            p.pos += 1;
            return CmpOp.Lt;
        }
        if (p.src[p.pos] == '>') {
            p.pos += 1;
            return CmpOp.Gt;
        }
    }
    return null;
}

fn parseFilterPrimary(p: *Parser): JsonPathError!*FilterExpr {
    skipWs(p);
    if (p.pos >= p.src.len) {
        return JsonPathError.InvalidFilter;
    }
    if (p.src[p.pos] == '!') {
        p.pos += 1;
        const inner = try parseFilterPrimary(p);
        return allocFilterExpr(p, FilterExpr{
            .kind = FilterExprKind.Not,
            .bool_val = false,
            .op = CmpOp.Eq,
            .left_expr = inner,
            .right_expr = null,
            .left_val = null,
            .right_val = null,
            .test_query = null,
        });
    }
    if (p.src[p.pos] == '(') {
        p.pos += 1;
        const inner = try parseFilterOr(p);
        errdefer deinitFilterExpr(inner, p.alloc);
        skipWs(p);
        if (p.pos >= p.src.len or p.src[p.pos] != ')') {
            return JsonPathError.InvalidFilter;
        }
        p.pos += 1;
        return inner;
    }

    let lhs = try parseFilterVal(p);
    const op_opt = parseCmpOp(p);
    if (op_opt != null) {
        errdefer deinitFilterVal(&lhs, p.alloc);
        const rhs = try parseFilterVal(p);
        return allocFilterExpr(p, FilterExpr{
            .kind = FilterExprKind.Cmp,
            .bool_val = false,
            .op = op_opt.?,
            .left_expr = null,
            .right_expr = null,
            .left_val = lhs,
            .right_val = rhs,
            .test_query = null,
        });
    }

    if (lhs.kind == FilterValKind.Query) {
        return allocFilterExpr(p, FilterExpr{
            .kind = FilterExprKind.TestQuery,
            .bool_val = false,
            .op = CmpOp.Eq,
            .left_expr = null,
            .right_expr = null,
            .left_val = null,
            .right_val = null,
            .test_query = lhs.query,
        });
    }
    if (lhs.kind == FilterValKind.Bool) {
        return allocFilterExpr(p, FilterExpr{
            .kind = FilterExprKind.BoolLit,
            .bool_val = lhs.bool_val,
            .op = CmpOp.Eq,
            .left_expr = null,
            .right_expr = null,
            .left_val = null,
            .right_val = null,
            .test_query = null,
        });
    }
    deinitFilterVal(&lhs, p.alloc);
    return JsonPathError.InvalidFilter;
}

fn parseFilterAnd(p: *Parser): JsonPathError!*FilterExpr {
    let left = try parseFilterPrimary(p);
    while (true) {
        skipWs(p);
        if (p.pos + 2 <= p.src.len and std.mem.eql(u8, p.src[p.pos .. p.pos + 2], "&&")) {
            p.pos += 2;
            errdefer deinitFilterExpr(left, p.alloc);
            const right = try parseFilterPrimary(p);
            left = try allocFilterExpr(p, FilterExpr{
                .kind = FilterExprKind.And,
                .bool_val = false,
                .op = CmpOp.Eq,
                .left_expr = left,
                .right_expr = right,
                .left_val = null,
                .right_val = null,
                .test_query = null,
            });
        } else {
            break;
        }
    }
    return left;
}

fn parseFilterOr(p: *Parser): JsonPathError!*FilterExpr {
    let left = try parseFilterAnd(p);
    while (true) {
        skipWs(p);
        if (p.pos + 2 <= p.src.len and std.mem.eql(u8, p.src[p.pos .. p.pos + 2], "||")) {
            p.pos += 2;
            errdefer deinitFilterExpr(left, p.alloc);
            const right = try parseFilterAnd(p);
            left = try allocFilterExpr(p, FilterExpr{
                .kind = FilterExprKind.Or,
                .bool_val = false,
                .op = CmpOp.Eq,
                .left_expr = left,
                .right_expr = right,
                .left_val = null,
                .right_val = null,
                .test_query = null,
            });
        } else {
            break;
        }
    }
    return left;
}

fn parseSelector(p: *Parser): JsonPathError!Selector {
    skipWs(p);
    if (p.pos >= p.src.len) {
        return JsonPathError.UnterminatedBracket;
    }
    const c = p.src[p.pos];
    if (c == '*') {
        p.pos += 1;
        return Selector{
            .kind = SelectorKind.Wildcard,
            .name = "",
            .index = 0,
            .slice = SliceSpec{ .start = null, .end = null, .step = 1 },
            .filter = null,
        };
    }
    if (c == '\'' or c == '"') {
        const s = try parseQuotedString(p);
        return Selector{
            .kind = SelectorKind.Name,
            .name = s,
            .index = 0,
            .slice = SliceSpec{ .start = null, .end = null, .step = 1 },
            .filter = null,
        };
    }
    if (c == '?') {
        p.pos += 1;
        const expr = try parseFilterOr(p);
        return Selector{
            .kind = SelectorKind.Filter,
            .name = "",
            .index = 0,
            .slice = SliceSpec{ .start = null, .end = null, .step = 1 },
            .filter = expr,
        };
    }
    if (c == ':' or c == '-' or (c >= '0' and c <= '9')) {
        let start_opt: i64? = null;
        if (c != ':') {
            start_opt = try parseInt64(p);
        }
        skipWs(p);
        if (p.pos >= p.src.len or p.src[p.pos] != ':') {
            if (start_opt == null) {
                return JsonPathError.InvalidSyntax;
            }
            return Selector{
                .kind = SelectorKind.Index,
                .name = "",
                .index = start_opt.?,
                .slice = SliceSpec{ .start = null, .end = null, .step = 1 },
                .filter = null,
            };
        }
        p.pos += 1;
        skipWs(p);
        let end_opt: i64? = null;
        if (p.pos < p.src.len and (p.src[p.pos] == '-' or (p.src[p.pos] >= '0' and p.src[p.pos] <= '9'))) {
            end_opt = try parseInt64(p);
        }
        skipWs(p);
        let step: i64 = 1;
        if (p.pos < p.src.len and p.src[p.pos] == ':') {
            p.pos += 1;
            skipWs(p);
            if (p.pos < p.src.len and (p.src[p.pos] == '-' or (p.src[p.pos] >= '0' and p.src[p.pos] <= '9'))) {
                step = try parseInt64(p);
            }
        }
        if (step == 0) {
            return JsonPathError.ZeroSliceStep;
        }
        return Selector{
            .kind = SelectorKind.Slice,
            .name = "",
            .index = 0,
            .slice = SliceSpec{ .start = start_opt, .end = end_opt, .step = step },
            .filter = null,
        };
    }
    return JsonPathError.InvalidSyntax;
}

fn parseBracketSelectors(p: *Parser): JsonPathError![]Selector {
    if (p.pos >= p.src.len or p.src[p.pos] != '[') {
        return JsonPathError.InvalidSyntax;
    }
    p.pos += 1;
    let sels = std.ArrayList(Selector).empty;
    errdefer {
        for (*sel of sels.items) {
            if (sel.kind == SelectorKind.Name and sel.name.len > 0) {
                p.alloc.free(sel.name);
            }
            if (sel.filter != null) {
                deinitFilterExpr(sel.filter.?, p.alloc);
            }
        }
        sels.deinit(p.alloc);
    }

    while (true) {
        const sel = try parseSelector(p);
        try sels.append(p.alloc, sel);
        skipWs(p);
        if (p.pos >= p.src.len) {
            return JsonPathError.UnterminatedBracket;
        }
        if (p.src[p.pos] == ',') {
            p.pos += 1;
            continue;
        }
        if (p.src[p.pos] == ']') {
            p.pos += 1;
            break;
        }
        return JsonPathError.InvalidSyntax;
    }
    return try sels.toOwnedSlice(p.alloc);
}

fn parseSegments(p: *Parser, in_filter: bool): JsonPathError![]Segment {
    let segs = std.ArrayList(Segment).empty;
    errdefer deinitSegments(segs.items, p.alloc);

    while (p.pos < p.src.len) {
        if (!in_filter) {
            skipWs(p);
            if (p.pos >= p.src.len) {
                break;
            }
        }
        if (p.pos + 1 < p.src.len and p.src[p.pos] == '.' and p.src[p.pos + 1] == '.') {
            p.pos += 2;
            if (p.pos >= p.src.len) {
                return JsonPathError.InvalidSyntax;
            }
            if (p.src[p.pos] == '[') {
                const sels = try parseBracketSelectors(p);
                try segs.append(p.alloc, Segment{ .kind = SegmentKind.Descendant, .selectors = sels });
            } else if (p.src[p.pos] == '*') {
                p.pos += 1;
                let sels = try p.alloc.alloc(Selector, 1);
                sels[0] = Selector{
                    .kind = SelectorKind.Wildcard,
                    .name = "",
                    .index = 0,
                    .slice = SliceSpec{ .start = null, .end = null, .step = 1 },
                    .filter = null,
                };
                try segs.append(p.alloc, Segment{ .kind = SegmentKind.Descendant, .selectors = sels });
            } else {
                const name = try parseMemberNameShorthand(p);
                errdefer p.alloc.free(name);
                let sels = try p.alloc.alloc(Selector, 1);
                sels[0] = Selector{
                    .kind = SelectorKind.Name,
                    .name = name,
                    .index = 0,
                    .slice = SliceSpec{ .start = null, .end = null, .step = 1 },
                    .filter = null,
                };
                try segs.append(p.alloc, Segment{ .kind = SegmentKind.Descendant, .selectors = sels });
            }
        } else if (p.src[p.pos] == '.') {
            p.pos += 1;
            if (p.pos >= p.src.len) {
                return JsonPathError.InvalidSyntax;
            }
            if (p.src[p.pos] == '*') {
                p.pos += 1;
                let sels = try p.alloc.alloc(Selector, 1);
                sels[0] = Selector{
                    .kind = SelectorKind.Wildcard,
                    .name = "",
                    .index = 0,
                    .slice = SliceSpec{ .start = null, .end = null, .step = 1 },
                    .filter = null,
                };
                try segs.append(p.alloc, Segment{ .kind = SegmentKind.Child, .selectors = sels });
            } else {
                const name = try parseMemberNameShorthand(p);
                errdefer p.alloc.free(name);
                let sels = try p.alloc.alloc(Selector, 1);
                sels[0] = Selector{
                    .kind = SelectorKind.Name,
                    .name = name,
                    .index = 0,
                    .slice = SliceSpec{ .start = null, .end = null, .step = 1 },
                    .filter = null,
                };
                try segs.append(p.alloc, Segment{ .kind = SegmentKind.Child, .selectors = sels });
            }
        } else if (p.src[p.pos] == '[') {
            const sels = try parseBracketSelectors(p);
            try segs.append(p.alloc, Segment{ .kind = SegmentKind.Child, .selectors = sels });
        } else {
            if (in_filter) {
                break;
            }
            return JsonPathError.InvalidSyntax;
        }
    }
    return try segs.toOwnedSlice(p.alloc);
}

pub fn compileJsonPath(alloc: std.mem.Allocator, expr: []const u8): JsonPathError!JsonPathQuery {
    if (expr.len == 0) {
        return JsonPathError.EmptyQuery;
    }
    let p = Parser{
        .src = expr,
        .pos = 0,
        .alloc = alloc,
    };
    skipWs(&p);
    if (p.pos >= p.src.len or p.src[p.pos] != '$') {
        return JsonPathError.MissingRootIdentifier;
    }
    p.pos += 1;
    const segs = try parseSegments(&p, false);
    return JsonPathQuery{ .segments = segs };
}
