// Source: cp/lib/jmespath/parser.cp (JMESPath expression AST & compiler)
const std = @import("std");
import { MAX_DEPTH, JmesError, JmesValue, parseJson } from "./value.cp";

pub const CompareOp = enum {
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
};

pub const MultiSelectEntry = struct {
    key: []const u8,
    expr: *JmesNode,
};

pub const SliceSpec = struct {
    start: i64?,
    stop: i64?,
    step: i64?,
};

pub const BinaryNode = struct {
    left: *JmesNode,
    right: *JmesNode,
};

pub const CompareNode = struct {
    op: CompareOp,
    left: *JmesNode,
    right: *JmesNode,
};

pub const FunctionCallNode = struct {
    name: []const u8,
    args: []*JmesNode,
};

pub const JmesNode = union(enum) {
    Current: void,
    Field: []const u8,
    Index: i64,
    Slice: SliceSpec,
    SubExpr: BinaryNode,
    IndexExpr: BinaryNode,
    Flatten: *JmesNode?,
    ListWildcard: void,
    ObjectWildcard: void,
    Filter: *JmesNode,
    Pipe: BinaryNode,
    MultiSelectList: []*JmesNode,
    MultiSelectHash: []MultiSelectEntry,
    Literal: JmesValue,
    RawString: []const u8,
    Not: *JmesNode,
    And: BinaryNode,
    Or: BinaryNode,
    Compare: CompareNode,
    FunctionCall: FunctionCallNode,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.*) {
            .Current, .Index, .Slice, .ListWildcard, .ObjectWildcard => {},
            .Field => |s| alloc.free(s),
            .RawString => |s| alloc.free(s),
            .Literal => |*v| v.deinit(alloc),
            .SubExpr => |b| {
                b.left.deinit(alloc);
                alloc.destroy(b.left);
                b.right.deinit(alloc);
                alloc.destroy(b.right);
            },
            .IndexExpr => |b| {
                b.left.deinit(alloc);
                alloc.destroy(b.left);
                b.right.deinit(alloc);
                alloc.destroy(b.right);
            },
            .Pipe => |b| {
                b.left.deinit(alloc);
                alloc.destroy(b.left);
                b.right.deinit(alloc);
                alloc.destroy(b.right);
            },
            .And => |b| {
                b.left.deinit(alloc);
                alloc.destroy(b.left);
                b.right.deinit(alloc);
                alloc.destroy(b.right);
            },
            .Or => |b| {
                b.left.deinit(alloc);
                alloc.destroy(b.left);
                b.right.deinit(alloc);
                alloc.destroy(b.right);
            },
            .Compare => |c| {
                c.left.deinit(alloc);
                alloc.destroy(c.left);
                c.right.deinit(alloc);
                alloc.destroy(c.right);
            },
            .Flatten => |opt| {
                if (inner := opt) {
                    inner.deinit(alloc);
                    alloc.destroy(inner);
                }
            },
            .Filter => |inner| {
                inner.deinit(alloc);
                alloc.destroy(inner);
            },
            .Not => |inner| {
                inner.deinit(alloc);
                alloc.destroy(inner);
            },
            .MultiSelectList => |list| {
                for (item of list) {
                    item.deinit(alloc);
                    alloc.destroy(item);
                }
                alloc.free(list);
            },
            .MultiSelectHash => |entries| {
                for (entry of entries) {
                    alloc.free(entry.key);
                    entry.expr.deinit(alloc);
                    alloc.destroy(entry.expr);
                }
                alloc.free(entries);
            },
            .FunctionCall => |fc| {
                alloc.free(fc.name);
                for (arg of fc.args) {
                    arg.deinit(alloc);
                    alloc.destroy(arg);
                }
                alloc.free(fc.args);
            },
        }
    }
};

pub const JmesExpr = struct {
    root: *JmesNode,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.root.deinit(alloc);
        alloc.destroy(_.root);
    }
};

const ExprParser = struct {
    input: []const u8,
    pos: usize,
    alloc: std.mem.Allocator,

    fn allocNode(*_, node: JmesNode): JmesError!*JmesNode {
        let ptr = _.alloc.create(JmesNode) catch return JmesError.OutOfMemory;
        ptr.* = node;
        return ptr;
    }

    fn destroyNode(*_, node: *JmesNode) {
        node.deinit(_.alloc);
        _.alloc.destroy(node);
    }

    fn skipWs(*_) {
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (c == ' ' or c == '\t' or c == '\n' or c == '\r') {
                _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn peek(*_): u8? {
        _.skipWs();
        if (_.pos >= _.input.len) return null;
        return _.input[_.pos];
    }

    static fn isIdentStart(c: u8): bool {
        return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or c == '_';
    }

    static fn isIdentChar(c: u8): bool {
        return isIdentStart(c) or (c >= '0' and c <= '9');
    }

    fn parseUnquotedIdent(*_): JmesError![]u8 {
        const start = _.pos;
        if (_.pos >= _.input.len or !isIdentStart(_.input[_.pos])) {
            return JmesError.UnexpectedToken;
        }
        while (_.pos < _.input.len and isIdentChar(_.input[_.pos])) {
            _.pos += 1;
        }
        return _.alloc.dupe(u8, _.input[start.._.pos]) catch return JmesError.OutOfMemory;
    }

    fn parseQuotedIdent(*_): JmesError![]u8 {
        if (_.pos >= _.input.len or _.input[_.pos] != '"') return JmesError.InvalidSyntax;
        _.pos += 1;
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(_.alloc);
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (c == '"') {
                _.pos += 1;
                return out.toOwnedSlice(_.alloc) catch return JmesError.OutOfMemory;
            }
            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= _.input.len) return JmesError.InvalidSyntax;
                const esc = _.input[_.pos];
                _.pos += 1;
                switch (esc) {
                    '"' => out.append(_.alloc, '"') catch return JmesError.OutOfMemory,
                    '\\' => out.append(_.alloc, '\\') catch return JmesError.OutOfMemory,
                    'n' => out.append(_.alloc, '\n') catch return JmesError.OutOfMemory,
                    'r' => out.append(_.alloc, '\r') catch return JmesError.OutOfMemory,
                    't' => out.append(_.alloc, '\t') catch return JmesError.OutOfMemory,
                    else => out.append(_.alloc, esc) catch return JmesError.OutOfMemory,
                }
            } else {
                out.append(_.alloc, c) catch return JmesError.OutOfMemory;
                _.pos += 1;
            }
        }
        return JmesError.InvalidSyntax;
    }

    fn parseRawString(*_): JmesError![]u8 {
        if (_.pos >= _.input.len or _.input[_.pos] != '\'') return JmesError.InvalidSyntax;
        _.pos += 1;
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(_.alloc);
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (c == '\'') {
                _.pos += 1;
                return out.toOwnedSlice(_.alloc) catch return JmesError.OutOfMemory;
            }
            if (c == '\\' and _.pos + 1 < _.input.len and (_.input[_.pos + 1] == '\'' or _.input[_.pos + 1] == '\\')) {
                _.pos += 1;
                out.append(_.alloc, _.input[_.pos]) catch return JmesError.OutOfMemory;
                _.pos += 1;
            } else {
                out.append(_.alloc, c) catch return JmesError.OutOfMemory;
                _.pos += 1;
            }
        }
        return JmesError.InvalidSyntax;
    }

    fn parseLiteral(*_): JmesError!JmesValue {
        if (_.pos >= _.input.len or _.input[_.pos] != '`') return JmesError.InvalidSyntax;
        _.pos += 1;
        let raw = std.ArrayList(u8).empty;
        defer raw.deinit(_.alloc);
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (c == '`') {
                _.pos += 1;
                return parseJson(_.alloc, raw.items) catch return JmesError.InvalidSyntax;
            }
            if (c == '\\' and _.pos + 1 < _.input.len and _.input[_.pos + 1] == '`') {
                _.pos += 1;
                raw.append(_.alloc, '`') catch return JmesError.OutOfMemory;
                _.pos += 1;
            } else {
                raw.append(_.alloc, c) catch return JmesError.OutOfMemory;
                _.pos += 1;
            }
        }
        return JmesError.InvalidSyntax;
    }

    fn parseExpr(*_, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        return _.parsePipe(depth);
    }

    fn parsePipe(*_, depth: usize): JmesError!*JmesNode {
        let left = _.parseOr(depth)?;
        errdefer _.destroyNode(left);
        while (true) {
            _.skipWs();
            if (_.pos < _.input.len and _.input[_.pos] == '|' and
                (_.pos + 1 >= _.input.len or _.input[_.pos + 1] != '|'))
            {
                _.pos += 1;
                const right = _.parseOr(depth)?;
                left = _.allocNode(JmesNode{ .Pipe = BinaryNode{ .left = left, .right = right } }) catch |ce| {
                    _.destroyNode(right);
                    return ce;
                };
            } else {
                break;
            }
        }
        return left;
    }

    fn parseOr(*_, depth: usize): JmesError!*JmesNode {
        let left = _.parseAnd(depth)?;
        errdefer _.destroyNode(left);
        while (true) {
            _.skipWs();
            if (_.pos + 1 < _.input.len and _.input[_.pos] == '|' and _.input[_.pos + 1] == '|') {
                _.pos += 2;
                const right = _.parseAnd(depth)?;
                left = _.allocNode(JmesNode{ .Or = BinaryNode{ .left = left, .right = right } }) catch |ce| {
                    _.destroyNode(right);
                    return ce;
                };
            } else {
                break;
            }
        }
        return left;
    }

    fn parseAnd(*_, depth: usize): JmesError!*JmesNode {
        let left = _.parseCompare(depth)?;
        errdefer _.destroyNode(left);
        while (true) {
            _.skipWs();
            if (_.pos + 1 < _.input.len and _.input[_.pos] == '&' and _.input[_.pos + 1] == '&') {
                _.pos += 2;
                const right = _.parseCompare(depth)?;
                left = _.allocNode(JmesNode{ .And = BinaryNode{ .left = left, .right = right } }) catch |ce| {
                    _.destroyNode(right);
                    return ce;
                };
            } else {
                break;
            }
        }
        return left;
    }

    fn parseCompare(*_, depth: usize): JmesError!*JmesNode {
        let left = _.parseUnary(depth)?;
        errdefer _.destroyNode(left);
        _.skipWs();
        if (_.pos + 1 < _.input.len) {
            const two = _.input[_.pos .. _.pos + 2];
            let op_opt: CompareOp? = null;
            let adv: usize = 0;
            if (std.mem.eql(u8, two, "==")) {
                op_opt = .Eq;
                adv = 2;
            } else if (std.mem.eql(u8, two, "!=")) {
                op_opt = .Ne;
                adv = 2;
            } else if (std.mem.eql(u8, two, "<=")) {
                op_opt = .Le;
                adv = 2;
            } else if (std.mem.eql(u8, two, ">=")) {
                op_opt = .Ge;
                adv = 2;
            } else if (_.input[_.pos] == '<') {
                op_opt = .Lt;
                adv = 1;
            } else if (_.input[_.pos] == '>') {
                op_opt = .Gt;
                adv = 1;
            }
            if (op := op_opt) {
                _.pos += adv;
                const right = _.parseUnary(depth)?;
                left = _.allocNode(JmesNode{ .Compare = CompareNode{ .op = op, .left = left, .right = right } }) catch |ce| {
                    _.destroyNode(right);
                    return ce;
                };
            }
        } else if (_.pos < _.input.len) {
            if (_.input[_.pos] == '<') {
                _.pos += 1;
                const right = _.parseUnary(depth)?;
                left = _.allocNode(JmesNode{ .Compare = CompareNode{ .op = .Lt, .left = left, .right = right } }) catch |ce| {
                    _.destroyNode(right);
                    return ce;
                };
            } else if (_.input[_.pos] == '>') {
                _.pos += 1;
                const right = _.parseUnary(depth)?;
                left = _.allocNode(JmesNode{ .Compare = CompareNode{ .op = .Gt, .left = left, .right = right } }) catch |ce| {
                    _.destroyNode(right);
                    return ce;
                };
            }
        }
        return left;
    }

    fn parseUnary(*_, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        _.skipWs();
        if (_.pos < _.input.len and _.input[_.pos] == '!' and
            (_.pos + 1 >= _.input.len or _.input[_.pos + 1] != '='))
        {
            _.pos += 1;
            const inner = _.parseUnary(depth + 1)?;
            return _.allocNode(JmesNode{ .Not = inner }) catch |ce| {
                _.destroyNode(inner);
                return ce;
            };
        }
        return _.parsePostfix(depth);
    }

    fn parsePostfix(*_, depth: usize): JmesError!*JmesNode {
        let left = _.parsePrimary(depth)?;
        errdefer _.destroyNode(left);
        while (true) {
            _.skipWs();
            if (_.pos >= _.input.len) break;
            const c = _.input[_.pos];
            if (c == '.') {
                _.pos += 1;
                const rhs = _.parseAfterDot(depth + 1)?;
                left = _.allocNode(JmesNode{ .SubExpr = BinaryNode{ .left = left, .right = rhs } }) catch |ce| {
                    _.destroyNode(rhs);
                    return ce;
                };
            } else if (c == '[') {
                const rhs = _.parseBracketPostfix(depth + 1)?;
                left = _.allocNode(JmesNode{ .IndexExpr = BinaryNode{ .left = left, .right = rhs } }) catch |ce| {
                    _.destroyNode(rhs);
                    return ce;
                };
            } else {
                break;
            }
        }
        return left;
    }

    fn parseAfterDot(*_, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        _.skipWs();
        if (_.pos >= _.input.len) return JmesError.InvalidSyntax;
        const c = _.input[_.pos];
        if (c == '*') {
            _.pos += 1;
            return _.allocNode(.ObjectWildcard);
        }
        if (c == '"') {
            const name = _.parseQuotedIdent()?;
            return _.allocNode(JmesNode{ .Field = name }) catch |ce| {
                _.alloc.free(name);
                return ce;
            };
        }
        if (c == '[') {
            return _.parseMultiSelectList(depth);
        }
        if (c == '{') {
            return _.parseMultiSelectHash(depth);
        }
        if (isIdentStart(c)) {
            const name = _.parseUnquotedIdent()?;
            _.skipWs();
            if (_.pos < _.input.len and _.input[_.pos] == '(') {
                return _.parseFunctionCall(name, depth);
            }
            return _.allocNode(JmesNode{ .Field = name }) catch |ce| {
                _.alloc.free(name);
                return ce;
            };
        }
        return JmesError.UnexpectedToken;
    }

    fn parsePrimary(*_, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        _.skipWs();
        if (_.pos >= _.input.len) return JmesError.InvalidSyntax;
        const c = _.input[_.pos];
        if (c == '@') {
            _.pos += 1;
            return _.allocNode(.Current);
        }
        if (c == '*') {
            _.pos += 1;
            return _.allocNode(.ObjectWildcard);
        }
        if (c == '"') {
            const name = _.parseQuotedIdent()?;
            return _.allocNode(JmesNode{ .Field = name }) catch |ce| {
                _.alloc.free(name);
                return ce;
            };
        }
        if (c == '\'') {
            const s = _.parseRawString()?;
            return _.allocNode(JmesNode{ .RawString = s }) catch |ce| {
                _.alloc.free(s);
                return ce;
            };
        }
        if (c == '`') {
            let lit = _.parseLiteral()?;
            return _.allocNode(JmesNode{ .Literal = lit }) catch |ce| {
                lit.deinit(_.alloc);
                return ce;
            };
        }
        if (c == '(') {
            _.pos += 1;
            const inner = _.parseExpr(depth + 1)?;
            errdefer _.destroyNode(inner);
            _.skipWs();
            if (_.pos >= _.input.len or _.input[_.pos] != ')') {
                return JmesError.InvalidSyntax;
            }
            _.pos += 1;
            return inner;
        }
        if (c == '[') {
            return _.parseBracketPrimary(depth + 1);
        }
        if (c == '{') {
            return _.parseMultiSelectHash(depth + 1);
        }
        if (isIdentStart(c)) {
            const name = _.parseUnquotedIdent()?;
            _.skipWs();
            if (_.pos < _.input.len and _.input[_.pos] == '(') {
                return _.parseFunctionCall(name, depth + 1);
            }
            return _.allocNode(JmesNode{ .Field = name }) catch |ce| {
                _.alloc.free(name);
                return ce;
            };
        }
        return JmesError.UnexpectedToken;
    }

    fn parseFunctionCall(*_, name: []u8, depth: usize): JmesError!*JmesNode {
        errdefer _.alloc.free(name);
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        _.pos += 1; // '('
        let args = std.ArrayList(*JmesNode).empty;
        errdefer {
            for (a of args.items) {
                _.destroyNode(a);
            }
            args.deinit(_.alloc);
        }
        _.skipWs();
        if (_.pos < _.input.len and _.input[_.pos] == ')') {
            _.pos += 1;
        } else {
            while (true) {
                const arg = _.parseExpr(depth + 1)?;
                args.append(_.alloc, arg) catch |ce| {
                    _.destroyNode(arg);
                    * = ce;
                    return JmesError.OutOfMemory;
                };
                _.skipWs();
                if (_.pos >= _.input.len) return JmesError.InvalidSyntax;
                if (_.input[_.pos] == ')') {
                    _.pos += 1;
                    break;
                }
                if (_.input[_.pos] != ',') return JmesError.InvalidSyntax;
                _.pos += 1;
            }
        }
        const slice = args.toOwnedSlice(_.alloc) catch return JmesError.OutOfMemory;
        return _.allocNode(JmesNode{
            .FunctionCall = FunctionCallNode{
                .name = name,
                .args = slice,
            },
        }) catch |ce| {
            for (a of slice) {
                _.destroyNode(a);
            }
            _.alloc.free(slice);
            return ce;
        };
    }

    fn parseOptionalInt(*_): JmesError!i64? {
        _.skipWs();
        if (_.pos >= _.input.len) return null;
        const start = _.pos;
        if (_.input[_.pos] == '-') {
            _.pos += 1;
        }
        if (_.pos >= _.input.len or _.input[_.pos] < '0' or _.input[_.pos] > '9') {
            _.pos = start;
            return null;
        }
        while (_.pos < _.input.len and _.input[_.pos] >= '0' and _.input[_.pos] <= '9') {
            _.pos += 1;
        }
        const v = std.fmt.parseInt(i64, _.input[start.._.pos], 10) catch return JmesError.InvalidSyntax;
        return v;
    }

    fn parseBracketPostfix(*_, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        _.pos += 1; // '['
        _.skipWs();
        if (_.pos >= _.input.len) return JmesError.InvalidSyntax;

        if (_.input[_.pos] == ']') {
            _.pos += 1;
            return _.allocNode(JmesNode{ .Flatten = null });
        }
        if (_.input[_.pos] == '*') {
            _.pos += 1;
            _.skipWs();
            if (_.pos >= _.input.len or _.input[_.pos] != ']') return JmesError.InvalidSyntax;
            _.pos += 1;
            return _.allocNode(.ListWildcard);
        }
        if (_.input[_.pos] == '?') {
            _.pos += 1;
            const cond = _.parseExpr(depth + 1)?;
            errdefer _.destroyNode(cond);
            _.skipWs();
            if (_.pos >= _.input.len or _.input[_.pos] != ']') return JmesError.InvalidSyntax;
            _.pos += 1;
            return _.allocNode(JmesNode{ .Filter = cond }) catch |ce| {
                _.destroyNode(cond);
                return ce;
            };
        }

        // Index or slice
        const first = _.parseOptionalInt()?;
        _.skipWs();
        if (_.pos < _.input.len and _.input[_.pos] == ']') {
            _.pos += 1;
            if (idx := first) {
                return _.allocNode(JmesNode{ .Index = idx });
            }
            return JmesError.InvalidSyntax;
        }
        if (_.pos < _.input.len and _.input[_.pos] == ':') {
            _.pos += 1;
            const stop = _.parseOptionalInt()?;
            _.skipWs();
            let step: i64? = null;
            if (_.pos < _.input.len and _.input[_.pos] == ':') {
                _.pos += 1;
                step = _.parseOptionalInt()?;
                _.skipWs();
            }
            if (_.pos >= _.input.len or _.input[_.pos] != ']') return JmesError.InvalidSyntax;
            _.pos += 1;
            if (st := step) {
                if (st == 0) return JmesError.InvalidSliceStep;
            }
            return _.allocNode(JmesNode{ .Slice = SliceSpec{ .start = first, .stop = stop, .step = step } });
        }
        return JmesError.InvalidSyntax;
    }

    fn parseBracketPrimary(*_, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        const saved = _.pos;
        // Peek for [], [*], [?...], [int], [start:stop:step]
        _.pos += 1; // '['
        _.skipWs();
        if (_.pos >= _.input.len) return JmesError.InvalidSyntax;
        const c = _.input[_.pos];
        if (c == ']' or c == '*' or c == '?' or c == ':' or c == '-' or (c >= '0' and c <= '9')) {
            _.pos = saved;
            return _.parseBracketPostfix(depth);
        }
        _.pos = saved;
        return _.parseMultiSelectList(depth);
    }

    fn parseMultiSelectList(*_, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        _.pos += 1; // '['
        let items = std.ArrayList(*JmesNode).empty;
        errdefer {
            for (it of items.items) {
                _.destroyNode(it);
            }
            items.deinit(_.alloc);
        }
        _.skipWs();
        if (_.pos < _.input.len and _.input[_.pos] == ']') {
            return JmesError.InvalidSyntax;
        }
        while (true) {
            const expr = _.parseExpr(depth + 1)?;
            items.append(_.alloc, expr) catch |ce| {
                _.destroyNode(expr);
                * = ce;
                return JmesError.OutOfMemory;
            };
            _.skipWs();
            if (_.pos >= _.input.len) return JmesError.InvalidSyntax;
            if (_.input[_.pos] == ']') {
                _.pos += 1;
                break;
            }
            if (_.input[_.pos] != ',') return JmesError.InvalidSyntax;
            _.pos += 1;
        }
        const slice = items.toOwnedSlice(_.alloc) catch return JmesError.OutOfMemory;
        return _.allocNode(JmesNode{ .MultiSelectList = slice }) catch |ce| {
            for (it of slice) {
                _.destroyNode(it);
            }
            _.alloc.free(slice);
            return ce;
        };
    }

    fn parseMultiSelectHash(*_, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        _.pos += 1; // '{'
        let entries = std.ArrayList(MultiSelectEntry).empty;
        errdefer {
            for (e of entries.items) {
                _.alloc.free(e.key);
                _.destroyNode(e.expr);
            }
            entries.deinit(_.alloc);
        }
        _.skipWs();
        if (_.pos < _.input.len and _.input[_.pos] == '}') {
            return JmesError.InvalidSyntax;
        }
        while (true) {
            _.skipWs();
            if (_.pos >= _.input.len) return JmesError.InvalidSyntax;
            const key = if (_.input[_.pos] == '"')
                _.parseQuotedIdent()?
            else
                _.parseUnquotedIdent()?;
            errdefer _.alloc.free(key);
            _.skipWs();
            if (_.pos >= _.input.len or _.input[_.pos] != ':') return JmesError.InvalidSyntax;
            _.pos += 1;
            const val_expr = _.parseExpr(depth + 1)?;
            entries.append(_.alloc, MultiSelectEntry{ .key = key, .expr = val_expr }) catch |ce| {
                _.destroyNode(val_expr);
                * = ce;
                return JmesError.OutOfMemory;
            };
            _.skipWs();
            if (_.pos >= _.input.len) return JmesError.InvalidSyntax;
            if (_.input[_.pos] == '}') {
                _.pos += 1;
                break;
            }
            if (_.input[_.pos] != ',') return JmesError.InvalidSyntax;
            _.pos += 1;
        }
        const slice = entries.toOwnedSlice(_.alloc) catch return JmesError.OutOfMemory;
        return _.allocNode(JmesNode{ .MultiSelectHash = slice }) catch |ce| {
            for (e of slice) {
                _.alloc.free(e.key);
                _.destroyNode(e.expr);
            }
            _.alloc.free(slice);
            return ce;
        };
    }
};

pub fn compileJmesPath(alloc: std.mem.Allocator, expr: []const u8): JmesError!JmesExpr {
    let parser = ExprParser{
        .input = expr,
        .pos = 0,
        .alloc = alloc,
    };
    const root = parser.parseExpr(0)?;
    parser.skipWs();
    if (parser.pos != parser.input.len) {
        root.deinit(alloc);
        alloc.destroy(root);
        return JmesError.InvalidSyntax;
    }
    return JmesExpr{ .root = root };
}
