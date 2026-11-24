// Source: cp/lib/jsonpath/parser.cp (RFC 9535 JSONPath Compiler & AST)
// Pure CP zero-C-dependency RFC 9535 JSONPath parser, validator, and AST pool.

const std = @import("std");

pub const MAX_QUERY_DEPTH: usize = 32;

pub err JsonPathError {
    InvalidExpression,
    UnexpectedToken,
    UnclosedBracket,
    InvalidFilter,
    InvalidSliceStep,
    MaxDepthExceeded,
    OutOfMemory,
}

pub enum SelectorKind {
    Name,
    Wildcard,
    Index,
    Slice,
    Filter,
}

pub struct Selector {
    kind: SelectorKind,
    name: []u8 = "",
    index: i64 = 0,
    slice_start: i64? = null,
    slice_end: i64? = null,
    slice_step: i64? = null,
    filter_root: usize = 0,
}

pub struct Segment {
    is_descendant: bool = false,
    selectors: []Selector = [],
}

pub struct QueryPlan {
    is_root: bool = true,
    segments: []Segment = [],
}

pub enum FilterKind {
    Or,
    And,
    Not,
    CmpEq,
    CmpNe,
    CmpLt,
    CmpLe,
    CmpGt,
    CmpGe,
    TestQuery,
    LitNull,
    LitBool,
    LitNumber,
    LitString,
    QueryVal,
}

pub struct FilterNode {
    kind: FilterKind,
    left: usize = 0,
    right: usize = 0,
    query_id: usize = 0,
    bool_val: bool = false,
    num_val: f64 = 0,
    str_val: []u8 = "",
}

pub struct CompiledProgram {
    alloc: std.mem.Allocator,
    queries: std.ArrayList(QueryPlan) = .empty,
    filter_nodes: std.ArrayList(FilterNode) = .empty,

    pub fn deinit(*_) {
        for (q of _.queries.items) {
            for (seg of q.segments) {
                for (sel of seg.selectors) {
                    if (sel.name.len > 0) {
                        _.alloc.free(sel.name);
                    }
                }
                if (seg.selectors.len > 0) {
                    _.alloc.free(seg.selectors);
                }
            }
            if (q.segments.len > 0) {
                _.alloc.free(q.segments);
            }
        }
        _.queries.deinit(_.alloc);

        for (fn_node of _.filter_nodes.items) {
            if (fn_node.str_val.len > 0) {
                _.alloc.free(fn_node.str_val);
            }
        }
        _.filter_nodes.deinit(_.alloc);
    }

    pub fn isSingularQuery(query_id: usize): bool {
        if (query_id >= _.queries.items.len) return false;
        const plan = _.queries.items[query_id];
        for (seg of plan.segments) {
            if (seg.is_descendant) return false;
            if (seg.selectors.len != 1) return false;
            const kind = seg.selectors[0].kind;
            if (kind != .Name && kind != .Index) return false;
        }
        return true;
    }

    pub static fn compile(alloc: std.mem.Allocator, expr: []const u8): CompiledProgram ! JsonPathError {
        let prog = CompiledProgram{ alloc: alloc };
        errdefer prog.deinit();

        let parser = Parser{
            alloc: alloc,
            src: expr,
            pos: 0,
            prog: *prog,
        };

        if (expr.len == 0 || expr[0] != '$') {
            return err.InvalidExpression;
        }
        parser.pos = 1;
        * = parser.parseQuery(true, false, 0)?;

        parser.skipWs();
        if (parser.pos < parser.src.len) {
            return err.UnexpectedToken;
        }
        return prog;
    }
}

struct Parser {
    alloc: std.mem.Allocator,
    src: []const u8,
    pos: usize = 0,
    prog: *CompiledProgram,

    static fn isWs(c: u8): bool {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    fn skipWs(*_) {
        while (_.pos < _.src.len && Parser.isWs(_.src[_.pos])) {
            _.pos += 1;
        }
    }

    static fn isDigit(c: u8): bool {
        return c >= '0' && c <= '9';
    }

    static fn isNameFirst(c: u8): bool {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0x80;
    }

    static fn isNameChar(c: u8): bool {
        return Parser.isNameFirst(c) || Parser.isDigit(c);
    }

    fn matchStr(s: []const u8): bool {
        if (_.pos + s.len > _.src.len) return false;
        return std.mem.eql(u8, _.src[_.pos.._.pos + s.len], s);
    }

    fn addFilterNode(*_, node: FilterNode): usize ! JsonPathError {
        const idx = _.prog.filter_nodes.items.len;
        _.prog.filter_nodes.append(_.alloc, node) catch return err.OutOfMemory;
        return idx;
    }

    fn parseQuery(*_, is_root: bool, is_filter_query: bool, depth: usize): usize ! JsonPathError {
        if (depth > MAX_QUERY_DEPTH) return err.MaxDepthExceeded;
        const qid = _.prog.queries.items.len;
        _.prog.queries.append(_.alloc, QueryPlan{ is_root: is_root, segments: [] }) catch return err.OutOfMemory;

        const segs = _.parseSegments(is_filter_query, depth)?;
        _.prog.queries.items[qid].segments = segs;
        return qid;
    }

    fn parseSegments(*_, is_filter_query: bool, base_depth: usize): ([]Segment) ! JsonPathError {
        let segs: std.ArrayList(Segment) = .empty;
        errdefer {
            for (seg of segs.items) {
                for (sel of seg.selectors) {
                    if (sel.name.len > 0) _.alloc.free(sel.name);
                }
                if (seg.selectors.len > 0) _.alloc.free(seg.selectors);
            }
            segs.deinit(_.alloc);
        }

        while (true) {
            const saved_pos = _.pos;
            _.skipWs();
            if (_.pos >= _.src.len) break;

            const c = _.src[_.pos];
            if (c != '.' && c != '[') {
                if (is_filter_query) {
                    _.pos = saved_pos;
                    break;
                }
                return err.UnexpectedToken;
            }

            const seg_depth = base_depth + segs.items.len + 1;
            if (seg_depth > MAX_QUERY_DEPTH) {
                return err.MaxDepthExceeded;
            }

            if (c == '.') {
                if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '.') {
                    // Descendant segment: ..
                    _.pos += 2;
                    if (_.pos >= _.src.len) return err.InvalidExpression;
                    const nxt = _.src[_.pos];
                    if (nxt == '*') {
                        _.pos += 1;
                        let sels = _.alloc.alloc(Selector, 1) catch return err.OutOfMemory;
                        sels[0] = Selector{ kind: .Wildcard };
                        segs.append(_.alloc, Segment{ is_descendant: true, selectors: sels }) catch {
                            _.alloc.free(sels);
                            return err.OutOfMemory;
                        };
                    } else if (nxt == '[') {
                        const sels = _.parseBracketed(seg_depth)?;
                        segs.append(_.alloc, Segment{ is_descendant: true, selectors: sels }) catch {
                            for (s of sels) {
                                if (s.name.len > 0) _.alloc.free(s.name);
                            }
                            _.alloc.free(sels);
                            return err.OutOfMemory;
                        };
                    } else if (Parser.isNameFirst(nxt)) {
                        const name = _.parseMemberName()?;
                        let sels = _.alloc.alloc(Selector, 1) catch {
                            _.alloc.free(name);
                            return err.OutOfMemory;
                        };
                        sels[0] = Selector{ kind: .Name, name: name };
                        segs.append(_.alloc, Segment{ is_descendant: true, selectors: sels }) catch {
                            _.alloc.free(name);
                            _.alloc.free(sels);
                            return err.OutOfMemory;
                        };
                    } else {
                        return err.UnexpectedToken;
                    }
                } else {
                    // Child dot segment: .name or .*
                    _.pos += 1;
                    if (_.pos >= _.src.len) return err.InvalidExpression;
                    const nxt = _.src[_.pos];
                    if (nxt == '*') {
                        _.pos += 1;
                        let sels = _.alloc.alloc(Selector, 1) catch return err.OutOfMemory;
                        sels[0] = Selector{ kind: .Wildcard };
                        segs.append(_.alloc, Segment{ is_descendant: false, selectors: sels }) catch {
                            _.alloc.free(sels);
                            return err.OutOfMemory;
                        };
                    } else if (Parser.isNameFirst(nxt)) {
                        const name = _.parseMemberName()?;
                        let sels = _.alloc.alloc(Selector, 1) catch {
                            _.alloc.free(name);
                            return err.OutOfMemory;
                        };
                        sels[0] = Selector{ kind: .Name, name: name };
                        segs.append(_.alloc, Segment{ is_descendant: false, selectors: sels }) catch {
                            _.alloc.free(name);
                            _.alloc.free(sels);
                            return err.OutOfMemory;
                        };
                    } else {
                        return err.UnexpectedToken;
                    }
                }
            } else {
                // Bracketed child segment: [...]
                const sels = _.parseBracketed(seg_depth)?;
                segs.append(_.alloc, Segment{ is_descendant: false, selectors: sels }) catch {
                    for (s of sels) {
                        if (s.name.len > 0) _.alloc.free(s.name);
                    }
                    _.alloc.free(sels);
                    return err.OutOfMemory;
                };
            }
        }

        return segs.toOwnedSlice(_.alloc) catch return err.OutOfMemory;
    }

    fn parseMemberName(*_): ([]u8) ! JsonPathError {
        const start = _.pos;
        if (_.pos >= _.src.len || !Parser.isNameFirst(_.src[_.pos])) {
            return err.UnexpectedToken;
        }
        _.pos += 1;
        while (_.pos < _.src.len && Parser.isNameChar(_.src[_.pos])) {
            _.pos += 1;
        }
        const slice = _.src[start.._.pos];
        const copy = _.alloc.alloc(u8, slice.len) catch return err.OutOfMemory;
        @memcpy(copy, slice);
        return copy;
    }

    fn parseBracketed(*_, depth: usize): ([]Selector) ! JsonPathError {
        if (_.pos >= _.src.len || _.src[_.pos] != '[') return err.UnexpectedToken;
        _.pos += 1;
        _.skipWs();
        if (_.pos >= _.src.len) return err.UnclosedBracket;
        if (_.src[_.pos] == ']') return err.InvalidExpression;

        let sels: std.ArrayList(Selector) = .empty;
        errdefer {
            for (s of sels.items) {
                if (s.name.len > 0) _.alloc.free(s.name);
            }
            sels.deinit(_.alloc);
        }

        while (true) {
            _.skipWs();
            if (_.pos >= _.src.len) return err.UnclosedBracket;

            const c = _.src[_.pos];
            if (c == '\'' || c == '"') {
                const name = _.parseQuotedString()?;
                sels.append(_.alloc, Selector{ kind: .Name, name: name }) catch {
                    _.alloc.free(name);
                    return err.OutOfMemory;
                };
            } else if (c == '*') {
                _.pos += 1;
                sels.append(_.alloc, Selector{ kind: .Wildcard }) catch return err.OutOfMemory;
            } else if (c == '?') {
                _.pos += 1;
                _.skipWs();
                if (_.pos >= _.src.len) return err.UnclosedBracket;
                const froot = _.parseFilterOr(depth + 1)?;
                sels.append(_.alloc, Selector{ kind: .Filter, filter_root: froot }) catch return err.OutOfMemory;
            } else if (c == ':' || c == '-' || Parser.isDigit(c)) {
                const sel = _.parseIndexOrSlice()?;
                sels.append(_.alloc, sel) catch return err.OutOfMemory;
            } else {
                return err.UnexpectedToken;
            }

            _.skipWs();
            if (_.pos >= _.src.len) return err.UnclosedBracket;
            const sep = _.src[_.pos];
            if (sep == ',') {
                _.pos += 1;
                _.skipWs();
                if (_.pos >= _.src.len) return err.UnclosedBracket;
                if (_.src[_.pos] == ']') return err.UnexpectedToken;
            } else if (sep == ']') {
                _.pos += 1;
                break;
            } else {
                return err.UnexpectedToken;
            }
        }

        return sels.toOwnedSlice(_.alloc) catch return err.OutOfMemory;
    }

    fn parseIntLiteral(*_): i64 ! JsonPathError {
        const start = _.pos;
        if (_.pos < _.src.len && _.src[_.pos] == '-') {
            _.pos += 1;
            if (_.pos >= _.src.len || !Parser.isDigit(_.src[_.pos])) {
                return err.UnexpectedToken;
            }
            if (_.src[_.pos] == '0') {
                // RFC 9535 forbids -0 as integer index
                return err.InvalidExpression;
            }
        }
        if (_.pos >= _.src.len || !Parser.isDigit(_.src[_.pos])) {
            return err.UnexpectedToken;
        }
        if (_.src[_.pos] == '0') {
            _.pos += 1;
            if (_.pos < _.src.len && Parser.isDigit(_.src[_.pos])) {
                return err.InvalidExpression;
            }
            return 0;
        }
        while (_.pos < _.src.len && Parser.isDigit(_.src[_.pos])) {
            _.pos += 1;
        }
        return std.fmt.parseInt(i64, _.src[start.._.pos], 10) catch return err.InvalidExpression;
    }

    fn parseIndexOrSlice(*_): Selector ! JsonPathError {
        let start_val: i64? = null;
        if (_.pos < _.src.len && (_.src[_.pos] == '-' || Parser.isDigit(_.src[_.pos]))) {
            start_val = _.parseIntLiteral()?;
        }

        _.skipWs();
        if (_.pos >= _.src.len || _.src[_.pos] != ':') {
            if (idx := start_val) {
                return Selector{ kind: .Index, index: idx };
            }
            return err.UnexpectedToken;
        }

        // Slice: start : [end] [: [step]]
        _.pos += 1;
        _.skipWs();

        let end_val: i64? = null;
        if (_.pos < _.src.len && (_.src[_.pos] == '-' || Parser.isDigit(_.src[_.pos]))) {
            end_val = _.parseIntLiteral()?;
        }

        _.skipWs();
        let step_val: i64? = null;
        if (_.pos < _.src.len && _.src[_.pos] == ':') {
            _.pos += 1;
            _.skipWs();
            if (_.pos < _.src.len && (_.src[_.pos] == '-' || Parser.isDigit(_.src[_.pos]))) {
                const sv = _.parseIntLiteral()?;
                if (sv == 0) {
                    return err.InvalidSliceStep;
                }
                step_val = sv;
            }
        }

        return Selector{
            kind: .Slice,
            slice_start: start_val,
            slice_end: end_val,
            slice_step: step_val,
        };
    }

    fn parseQuotedString(*_): ([]u8) ! JsonPathError {
        if (_.pos >= _.src.len) return err.UnexpectedToken;
        const quote = _.src[_.pos];
        if (quote != '\'' && quote != '"') return err.UnexpectedToken;
        _.pos += 1;

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.alloc);

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == quote) {
                _.pos += 1;
                return out.toOwnedSlice(_.alloc) catch return err.OutOfMemory;
            }
            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= _.src.len) return err.UnclosedBracket;
                const esc = _.src[_.pos];
                _.pos += 1;
                switch (esc) {
                    '"' => out.append(_.alloc, '"') catch return err.OutOfMemory,
                    '\'' => out.append(_.alloc, '\'') catch return err.OutOfMemory,
                    '\\' => out.append(_.alloc, '\\') catch return err.OutOfMemory,
                    '/' => out.append(_.alloc, '/') catch return err.OutOfMemory,
                    'b' => out.append(_.alloc, 0x08) catch return err.OutOfMemory,
                    'f' => out.append(_.alloc, 0x0C) catch return err.OutOfMemory,
                    'n' => out.append(_.alloc, '\n') catch return err.OutOfMemory,
                    'r' => out.append(_.alloc, '\r') catch return err.OutOfMemory,
                    't' => out.append(_.alloc, '\t') catch return err.OutOfMemory,
                    else => return err.InvalidExpression,
                }
            } else if (c < 0x20) {
                return err.InvalidExpression;
            } else {
                out.append(_.alloc, c) catch return err.OutOfMemory;
                _.pos += 1;
            }
        }
        return err.UnclosedBracket;
    }

    fn parseFilterOr(*_, depth: usize): usize ! JsonPathError {
        if (depth > MAX_QUERY_DEPTH) return err.MaxDepthExceeded;
        let lhs = _.parseFilterAnd(depth)?;
        while (true) {
            _.skipWs();
            if (_.matchStr("||")) {
                _.pos += 2;
                _.skipWs();
                const rhs = _.parseFilterAnd(depth)?;
                lhs = _.addFilterNode(FilterNode{ kind: .Or, left: lhs, right: rhs })?;
            } else {
                break;
            }
        }
        return lhs;
    }

    fn parseFilterAnd(*_, depth: usize): usize ! JsonPathError {
        let lhs = _.parseFilterBasic(depth)?;
        while (true) {
            _.skipWs();
            if (_.matchStr("&&")) {
                _.pos += 2;
                _.skipWs();
                const rhs = _.parseFilterBasic(depth)?;
                lhs = _.addFilterNode(FilterNode{ kind: .And, left: lhs, right: rhs })?;
            } else {
                break;
            }
        }
        return lhs;
    }

    fn parseFilterBasic(*_, depth: usize): usize ! JsonPathError {
        if (depth > MAX_QUERY_DEPTH) return err.MaxDepthExceeded;
        _.skipWs();
        if (_.pos >= _.src.len) return err.InvalidFilter;

        let negated = false;
        if (_.src[_.pos] == '!') {
            negated = true;
            _.pos += 1;
            _.skipWs();
        }

        if (_.pos >= _.src.len) return err.InvalidFilter;

        if (_.src[_.pos] == '(') {
            _.pos += 1;
            _.skipWs();
            if (_.pos < _.src.len && _.src[_.pos] == ')') {
                return err.InvalidFilter;
            }
            const inner = _.parseFilterOr(depth + 1)?;
            _.skipWs();
            if (_.pos >= _.src.len) return err.UnclosedBracket;
            if (_.src[_.pos] != ')') return err.InvalidFilter;
            _.pos += 1;
            if (negated) {
                return _.addFilterNode(FilterNode{ kind: .Not, left: inner });
            }
            return inner;
        }

        if (negated) {
            const c = _.src[_.pos];
            if (c != '@' && c != '$') return err.InvalidFilter;
            const is_root = (c == '$');
            _.pos += 1;
            const qid = _.parseQuery(is_root, true, depth + 1)?;
            const tnode = _.addFilterNode(FilterNode{ kind: .TestQuery, query_id: qid })?;
            return _.addFilterNode(FilterNode{ kind: .Not, left: tnode });
        }

        const c = _.src[_.pos];
        if (c == '@' || c == '$') {
            const is_root = (c == '$');
            _.pos += 1;
            const qid = _.parseQuery(is_root, true, depth + 1)?;
            _.skipWs();
            if (op := _.tryParseCmpOp()) {
                if (!_.prog.isSingularQuery(qid)) {
                    return err.InvalidFilter;
                }
                _.skipWs();
                const rhs = _.parseComparable(depth + 1)?;
                const lnode = _.addFilterNode(FilterNode{ kind: .QueryVal, query_id: qid })?;
                return _.addFilterNode(FilterNode{ kind: op, left: lnode, right: rhs });
            }
            return _.addFilterNode(FilterNode{ kind: .TestQuery, query_id: qid });
        }

        // Must be literal <op> comparable
        const lnode = _.parseLiteral()?;
        _.skipWs();
        let op: FilterKind = .CmpEq;
        if (found_op := _.tryParseCmpOp()) {
            op = found_op;
        } else {
            return err.InvalidFilter;
        }
        _.skipWs();
        const rnode = _.parseComparable(depth + 1)?;
        return _.addFilterNode(FilterNode{ kind: op, left: lnode, right: rnode });
    }

    fn tryParseCmpOp(*_): FilterKind? {
        if (_.matchStr("==")) {
            _.pos += 2;
            return .CmpEq;
        }
        if (_.matchStr("!=")) {
            _.pos += 2;
            return .CmpNe;
        }
        if (_.matchStr("<=")) {
            _.pos += 2;
            return .CmpLe;
        }
        if (_.matchStr(">=")) {
            _.pos += 2;
            return .CmpGe;
        }
        if (_.pos < _.src.len && _.src[_.pos] == '<') {
            _.pos += 1;
            return .CmpLt;
        }
        if (_.pos < _.src.len && _.src[_.pos] == '>') {
            _.pos += 1;
            return .CmpGt;
        }
        return null;
    }

    fn parseComparable(*_, depth: usize): usize ! JsonPathError {
        if (_.pos >= _.src.len) return err.InvalidFilter;
        const c = _.src[_.pos];
        if (c == '@' || c == '$') {
            const is_root = (c == '$');
            _.pos += 1;
            const qid = _.parseQuery(is_root, true, depth)?;
            if (!_.prog.isSingularQuery(qid)) {
                return err.InvalidFilter;
            }
            return _.addFilterNode(FilterNode{ kind: .QueryVal, query_id: qid });
        }
        return _.parseLiteral();
    }

    fn parseLiteral(*_): usize ! JsonPathError {
        if (_.pos >= _.src.len) return err.InvalidFilter;
        const c = _.src[_.pos];
        if (c == '\'' || c == '"') {
            const s = _.parseQuotedString() catch return err.InvalidFilter;
            return _.addFilterNode(FilterNode{ kind: .LitString, str_val: s }) catch {
                _.alloc.free(s);
                return err.OutOfMemory;
            };
        }
        if (_.matchStr("true") && (_.pos + 4 >= _.src.len || !Parser.isNameChar(_.src[_.pos + 4]))) {
            _.pos += 4;
            return _.addFilterNode(FilterNode{ kind: .LitBool, bool_val: true });
        }
        if (_.matchStr("false") && (_.pos + 5 >= _.src.len || !Parser.isNameChar(_.src[_.pos + 5]))) {
            _.pos += 5;
            return _.addFilterNode(FilterNode{ kind: .LitBool, bool_val: false });
        }
        if (_.matchStr("null") && (_.pos + 4 >= _.src.len || !Parser.isNameChar(_.src[_.pos + 4]))) {
            _.pos += 4;
            return _.addFilterNode(FilterNode{ kind: .LitNull });
        }
        if (c == '-' || Parser.isDigit(c)) {
            const start = _.pos;
            if (c == '-') _.pos += 1;
            if (_.pos >= _.src.len || !Parser.isDigit(_.src[_.pos])) return err.InvalidFilter;
            while (_.pos < _.src.len && Parser.isDigit(_.src[_.pos])) {
                _.pos += 1;
            }
            if (_.pos < _.src.len && _.src[_.pos] == '.') {
                _.pos += 1;
                if (_.pos >= _.src.len || !Parser.isDigit(_.src[_.pos])) return err.InvalidFilter;
                while (_.pos < _.src.len && Parser.isDigit(_.src[_.pos])) {
                    _.pos += 1;
                }
            }
            if (_.pos < _.src.len && (_.src[_.pos] == 'e' || _.src[_.pos] == 'E')) {
                _.pos += 1;
                if (_.pos < _.src.len && (_.src[_.pos] == '+' || _.src[_.pos] == '-')) _.pos += 1;
                if (_.pos >= _.src.len || !Parser.isDigit(_.src[_.pos])) return err.InvalidFilter;
                while (_.pos < _.src.len && Parser.isDigit(_.src[_.pos])) {
                    _.pos += 1;
                }
            }
            const num = std.fmt.parseFloat(f64, _.src[start.._.pos]) catch return err.InvalidFilter;
            return _.addFilterNode(FilterNode{ kind: .LitNumber, num_val: num });
        }
        return err.InvalidFilter;
    }
}
