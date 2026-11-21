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

    pub fn deinit(self: *JmesNode, alloc: std.mem.Allocator) {
        switch (self.*) {
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
                if (opt) |inner| {
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

    pub fn deinit(self: *JmesExpr, alloc: std.mem.Allocator) {
        self.root.deinit(alloc);
        alloc.destroy(self.root);
    }
};

const ExprParser = struct {
    input: []const u8,
    pos: usize,
    alloc: std.mem.Allocator,

    fn allocNode(self: *ExprParser, node: JmesNode): JmesError!*JmesNode {
        let ptr = self.alloc.create(JmesNode) catch return JmesError.OutOfMemory;
        ptr.* = node;
        return ptr;
    }

    fn destroyNode(self: *ExprParser, node: *JmesNode) {
        node.deinit(self.alloc);
        self.alloc.destroy(node);
    }

    fn skipWs(self: *ExprParser) {
        while (self.pos < self.input.len) {
            const c = self.input[self.pos];
            if (c == ' ' or c == '\t' or c == '\n' or c == '\r') {
                self.pos += 1;
            } else {
                break;
            }
        }
    }

    fn peek(self: *ExprParser): u8? {
        self.skipWs();
        if (self.pos >= self.input.len) return null;
        return self.input[self.pos];
    }

    fn isIdentStart(c: u8): bool {
        return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or c == '_';
    }

    fn isIdentChar(c: u8): bool {
        return isIdentStart(c) or (c >= '0' and c <= '9');
    }

    fn parseUnquotedIdent(self: *ExprParser): JmesError![]u8 {
        const start = self.pos;
        if (self.pos >= self.input.len or !isIdentStart(self.input[self.pos])) {
            return JmesError.UnexpectedToken;
        }
        while (self.pos < self.input.len and isIdentChar(self.input[self.pos])) {
            self.pos += 1;
        }
        return self.alloc.dupe(u8, self.input[start..self.pos]) catch return JmesError.OutOfMemory;
    }

    fn parseQuotedIdent(self: *ExprParser): JmesError![]u8 {
        if (self.pos >= self.input.len or self.input[self.pos] != '"') return JmesError.InvalidSyntax;
        self.pos += 1;
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(self.alloc);
        while (self.pos < self.input.len) {
            const c = self.input[self.pos];
            if (c == '"') {
                self.pos += 1;
                return out.toOwnedSlice(self.alloc) catch return JmesError.OutOfMemory;
            }
            if (c == '\\') {
                self.pos += 1;
                if (self.pos >= self.input.len) return JmesError.InvalidSyntax;
                const esc = self.input[self.pos];
                self.pos += 1;
                switch (esc) {
                    '"' => out.append(self.alloc, '"') catch return JmesError.OutOfMemory,
                    '\\' => out.append(self.alloc, '\\') catch return JmesError.OutOfMemory,
                    'n' => out.append(self.alloc, '\n') catch return JmesError.OutOfMemory,
                    'r' => out.append(self.alloc, '\r') catch return JmesError.OutOfMemory,
                    't' => out.append(self.alloc, '\t') catch return JmesError.OutOfMemory,
                    else => out.append(self.alloc, esc) catch return JmesError.OutOfMemory,
                }
            } else {
                out.append(self.alloc, c) catch return JmesError.OutOfMemory;
                self.pos += 1;
            }
        }
        return JmesError.InvalidSyntax;
    }

    fn parseRawString(self: *ExprParser): JmesError![]u8 {
        if (self.pos >= self.input.len or self.input[self.pos] != '\'') return JmesError.InvalidSyntax;
        self.pos += 1;
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(self.alloc);
        while (self.pos < self.input.len) {
            const c = self.input[self.pos];
            if (c == '\'') {
                self.pos += 1;
                return out.toOwnedSlice(self.alloc) catch return JmesError.OutOfMemory;
            }
            if (c == '\\' and self.pos + 1 < self.input.len and (self.input[self.pos + 1] == '\'' or self.input[self.pos + 1] == '\\')) {
                self.pos += 1;
                out.append(self.alloc, self.input[self.pos]) catch return JmesError.OutOfMemory;
                self.pos += 1;
            } else {
                out.append(self.alloc, c) catch return JmesError.OutOfMemory;
                self.pos += 1;
            }
        }
        return JmesError.InvalidSyntax;
    }

    fn parseLiteral(self: *ExprParser): JmesError!JmesValue {
        if (self.pos >= self.input.len or self.input[self.pos] != '`') return JmesError.InvalidSyntax;
        self.pos += 1;
        let raw = std.ArrayList(u8).empty;
        defer raw.deinit(self.alloc);
        while (self.pos < self.input.len) {
            const c = self.input[self.pos];
            if (c == '`') {
                self.pos += 1;
                return parseJson(self.alloc, raw.items) catch return JmesError.InvalidSyntax;
            }
            if (c == '\\' and self.pos + 1 < self.input.len and self.input[self.pos + 1] == '`') {
                self.pos += 1;
                raw.append(self.alloc, '`') catch return JmesError.OutOfMemory;
                self.pos += 1;
            } else {
                raw.append(self.alloc, c) catch return JmesError.OutOfMemory;
                self.pos += 1;
            }
        }
        return JmesError.InvalidSyntax;
    }

    fn parseExpr(self: *ExprParser, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        return self.parsePipe(depth);
    }

    fn parsePipe(self: *ExprParser, depth: usize): JmesError!*JmesNode {
        let left = self.parseOr(depth)?;
        errdefer self.destroyNode(left);
        while (true) {
            self.skipWs();
            if (self.pos < self.input.len and self.input[self.pos] == '|' and
                (self.pos + 1 >= self.input.len or self.input[self.pos + 1] != '|'))
            {
                self.pos += 1;
                const right = self.parseOr(depth)?;
                left = self.allocNode(JmesNode{ .Pipe = BinaryNode{ .left = left, .right = right } }) catch |ce| {
                    self.destroyNode(right);
                    return ce;
                };
            } else {
                break;
            }
        }
        return left;
    }

    fn parseOr(self: *ExprParser, depth: usize): JmesError!*JmesNode {
        let left = self.parseAnd(depth)?;
        errdefer self.destroyNode(left);
        while (true) {
            self.skipWs();
            if (self.pos + 1 < self.input.len and self.input[self.pos] == '|' and self.input[self.pos + 1] == '|') {
                self.pos += 2;
                const right = self.parseAnd(depth)?;
                left = self.allocNode(JmesNode{ .Or = BinaryNode{ .left = left, .right = right } }) catch |ce| {
                    self.destroyNode(right);
                    return ce;
                };
            } else {
                break;
            }
        }
        return left;
    }

    fn parseAnd(self: *ExprParser, depth: usize): JmesError!*JmesNode {
        let left = self.parseCompare(depth)?;
        errdefer self.destroyNode(left);
        while (true) {
            self.skipWs();
            if (self.pos + 1 < self.input.len and self.input[self.pos] == '&' and self.input[self.pos + 1] == '&') {
                self.pos += 2;
                const right = self.parseCompare(depth)?;
                left = self.allocNode(JmesNode{ .And = BinaryNode{ .left = left, .right = right } }) catch |ce| {
                    self.destroyNode(right);
                    return ce;
                };
            } else {
                break;
            }
        }
        return left;
    }

    fn parseCompare(self: *ExprParser, depth: usize): JmesError!*JmesNode {
        let left = self.parseUnary(depth)?;
        errdefer self.destroyNode(left);
        self.skipWs();
        if (self.pos + 1 < self.input.len) {
            const two = self.input[self.pos .. self.pos + 2];
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
            } else if (self.input[self.pos] == '<') {
                op_opt = .Lt;
                adv = 1;
            } else if (self.input[self.pos] == '>') {
                op_opt = .Gt;
                adv = 1;
            }
            if (op_opt) |op| {
                self.pos += adv;
                const right = self.parseUnary(depth)?;
                left = self.allocNode(JmesNode{ .Compare = CompareNode{ .op = op, .left = left, .right = right } }) catch |ce| {
                    self.destroyNode(right);
                    return ce;
                };
            }
        } else if (self.pos < self.input.len) {
            if (self.input[self.pos] == '<') {
                self.pos += 1;
                const right = self.parseUnary(depth)?;
                left = self.allocNode(JmesNode{ .Compare = CompareNode{ .op = .Lt, .left = left, .right = right } }) catch |ce| {
                    self.destroyNode(right);
                    return ce;
                };
            } else if (self.input[self.pos] == '>') {
                self.pos += 1;
                const right = self.parseUnary(depth)?;
                left = self.allocNode(JmesNode{ .Compare = CompareNode{ .op = .Gt, .left = left, .right = right } }) catch |ce| {
                    self.destroyNode(right);
                    return ce;
                };
            }
        }
        return left;
    }

    fn parseUnary(self: *ExprParser, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        self.skipWs();
        if (self.pos < self.input.len and self.input[self.pos] == '!' and
            (self.pos + 1 >= self.input.len or self.input[self.pos + 1] != '='))
        {
            self.pos += 1;
            const inner = self.parseUnary(depth + 1)?;
            return self.allocNode(JmesNode{ .Not = inner }) catch |ce| {
                self.destroyNode(inner);
                return ce;
            };
        }
        return self.parsePostfix(depth);
    }

    fn parsePostfix(self: *ExprParser, depth: usize): JmesError!*JmesNode {
        let left = self.parsePrimary(depth)?;
        errdefer self.destroyNode(left);
        while (true) {
            self.skipWs();
            if (self.pos >= self.input.len) break;
            const c = self.input[self.pos];
            if (c == '.') {
                self.pos += 1;
                const rhs = self.parseAfterDot(depth + 1)?;
                left = self.allocNode(JmesNode{ .SubExpr = BinaryNode{ .left = left, .right = rhs } }) catch |ce| {
                    self.destroyNode(rhs);
                    return ce;
                };
            } else if (c == '[') {
                const rhs = self.parseBracketPostfix(depth + 1)?;
                left = self.allocNode(JmesNode{ .IndexExpr = BinaryNode{ .left = left, .right = rhs } }) catch |ce| {
                    self.destroyNode(rhs);
                    return ce;
                };
            } else {
                break;
            }
        }
        return left;
    }

    fn parseAfterDot(self: *ExprParser, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        self.skipWs();
        if (self.pos >= self.input.len) return JmesError.InvalidSyntax;
        const c = self.input[self.pos];
        if (c == '*') {
            self.pos += 1;
            return self.allocNode(.ObjectWildcard);
        }
        if (c == '"') {
            const name = self.parseQuotedIdent()?;
            return self.allocNode(JmesNode{ .Field = name }) catch |ce| {
                self.alloc.free(name);
                return ce;
            };
        }
        if (c == '[') {
            return self.parseMultiSelectList(depth);
        }
        if (c == '{') {
            return self.parseMultiSelectHash(depth);
        }
        if (isIdentStart(c)) {
            const name = self.parseUnquotedIdent()?;
            self.skipWs();
            if (self.pos < self.input.len and self.input[self.pos] == '(') {
                return self.parseFunctionCall(name, depth);
            }
            return self.allocNode(JmesNode{ .Field = name }) catch |ce| {
                self.alloc.free(name);
                return ce;
            };
        }
        return JmesError.UnexpectedToken;
    }

    fn parsePrimary(self: *ExprParser, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        self.skipWs();
        if (self.pos >= self.input.len) return JmesError.InvalidSyntax;
        const c = self.input[self.pos];
        if (c == '@') {
            self.pos += 1;
            return self.allocNode(.Current);
        }
        if (c == '*') {
            self.pos += 1;
            return self.allocNode(.ObjectWildcard);
        }
        if (c == '"') {
            const name = self.parseQuotedIdent()?;
            return self.allocNode(JmesNode{ .Field = name }) catch |ce| {
                self.alloc.free(name);
                return ce;
            };
        }
        if (c == '\'') {
            const s = self.parseRawString()?;
            return self.allocNode(JmesNode{ .RawString = s }) catch |ce| {
                self.alloc.free(s);
                return ce;
            };
        }
        if (c == '`') {
            let lit = self.parseLiteral()?;
            return self.allocNode(JmesNode{ .Literal = lit }) catch |ce| {
                lit.deinit(self.alloc);
                return ce;
            };
        }
        if (c == '(') {
            self.pos += 1;
            const inner = self.parseExpr(depth + 1)?;
            errdefer self.destroyNode(inner);
            self.skipWs();
            if (self.pos >= self.input.len or self.input[self.pos] != ')') {
                return JmesError.InvalidSyntax;
            }
            self.pos += 1;
            return inner;
        }
        if (c == '[') {
            return self.parseBracketPrimary(depth + 1);
        }
        if (c == '{') {
            return self.parseMultiSelectHash(depth + 1);
        }
        if (isIdentStart(c)) {
            const name = self.parseUnquotedIdent()?;
            self.skipWs();
            if (self.pos < self.input.len and self.input[self.pos] == '(') {
                return self.parseFunctionCall(name, depth + 1);
            }
            return self.allocNode(JmesNode{ .Field = name }) catch |ce| {
                self.alloc.free(name);
                return ce;
            };
        }
        return JmesError.UnexpectedToken;
    }

    fn parseFunctionCall(self: *ExprParser, name: []u8, depth: usize): JmesError!*JmesNode {
        errdefer self.alloc.free(name);
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        self.pos += 1; // '('
        let args = std.ArrayList(*JmesNode).empty;
        errdefer {
            for (a of args.items) {
                self.destroyNode(a);
            }
            args.deinit(self.alloc);
        }
        self.skipWs();
        if (self.pos < self.input.len and self.input[self.pos] == ')') {
            self.pos += 1;
        } else {
            while (true) {
                const arg = self.parseExpr(depth + 1)?;
                args.append(self.alloc, arg) catch |ce| {
                    self.destroyNode(arg);
                    * = ce;
                    return JmesError.OutOfMemory;
                };
                self.skipWs();
                if (self.pos >= self.input.len) return JmesError.InvalidSyntax;
                if (self.input[self.pos] == ')') {
                    self.pos += 1;
                    break;
                }
                if (self.input[self.pos] != ',') return JmesError.InvalidSyntax;
                self.pos += 1;
            }
        }
        const slice = args.toOwnedSlice(self.alloc) catch return JmesError.OutOfMemory;
        return self.allocNode(JmesNode{
            .FunctionCall = FunctionCallNode{
                .name = name,
                .args = slice,
            },
        }) catch |ce| {
            for (a of slice) {
                self.destroyNode(a);
            }
            self.alloc.free(slice);
            return ce;
        };
    }

    fn parseOptionalInt(self: *ExprParser): JmesError!i64? {
        self.skipWs();
        if (self.pos >= self.input.len) return null;
        const start = self.pos;
        if (self.input[self.pos] == '-') {
            self.pos += 1;
        }
        if (self.pos >= self.input.len or self.input[self.pos] < '0' or self.input[self.pos] > '9') {
            self.pos = start;
            return null;
        }
        while (self.pos < self.input.len and self.input[self.pos] >= '0' and self.input[self.pos] <= '9') {
            self.pos += 1;
        }
        const v = std.fmt.parseInt(i64, self.input[start..self.pos], 10) catch return JmesError.InvalidSyntax;
        return v;
    }

    fn parseBracketPostfix(self: *ExprParser, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        self.pos += 1; // '['
        self.skipWs();
        if (self.pos >= self.input.len) return JmesError.InvalidSyntax;

        if (self.input[self.pos] == ']') {
            self.pos += 1;
            return self.allocNode(JmesNode{ .Flatten = null });
        }
        if (self.input[self.pos] == '*') {
            self.pos += 1;
            self.skipWs();
            if (self.pos >= self.input.len or self.input[self.pos] != ']') return JmesError.InvalidSyntax;
            self.pos += 1;
            return self.allocNode(.ListWildcard);
        }
        if (self.input[self.pos] == '?') {
            self.pos += 1;
            const cond = self.parseExpr(depth + 1)?;
            errdefer self.destroyNode(cond);
            self.skipWs();
            if (self.pos >= self.input.len or self.input[self.pos] != ']') return JmesError.InvalidSyntax;
            self.pos += 1;
            return self.allocNode(JmesNode{ .Filter = cond }) catch |ce| {
                self.destroyNode(cond);
                return ce;
            };
        }

        // Index or slice
        const first = self.parseOptionalInt()?;
        self.skipWs();
        if (self.pos < self.input.len and self.input[self.pos] == ']') {
            self.pos += 1;
            if (first) |idx| {
                return self.allocNode(JmesNode{ .Index = idx });
            }
            return JmesError.InvalidSyntax;
        }
        if (self.pos < self.input.len and self.input[self.pos] == ':') {
            self.pos += 1;
            const stop = self.parseOptionalInt()?;
            self.skipWs();
            let step: i64? = null;
            if (self.pos < self.input.len and self.input[self.pos] == ':') {
                self.pos += 1;
                step = self.parseOptionalInt()?;
                self.skipWs();
            }
            if (self.pos >= self.input.len or self.input[self.pos] != ']') return JmesError.InvalidSyntax;
            self.pos += 1;
            if (step) |st| {
                if (st == 0) return JmesError.InvalidSliceStep;
            }
            return self.allocNode(JmesNode{ .Slice = SliceSpec{ .start = first, .stop = stop, .step = step } });
        }
        return JmesError.InvalidSyntax;
    }

    fn parseBracketPrimary(self: *ExprParser, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        const saved = self.pos;
        // Peek for [], [*], [?...], [int], [start:stop:step]
        self.pos += 1; // '['
        self.skipWs();
        if (self.pos >= self.input.len) return JmesError.InvalidSyntax;
        const c = self.input[self.pos];
        if (c == ']' or c == '*' or c == '?' or c == ':' or c == '-' or (c >= '0' and c <= '9')) {
            self.pos = saved;
            return self.parseBracketPostfix(depth);
        }
        self.pos = saved;
        return self.parseMultiSelectList(depth);
    }

    fn parseMultiSelectList(self: *ExprParser, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        self.pos += 1; // '['
        let items = std.ArrayList(*JmesNode).empty;
        errdefer {
            for (it of items.items) {
                self.destroyNode(it);
            }
            items.deinit(self.alloc);
        }
        self.skipWs();
        if (self.pos < self.input.len and self.input[self.pos] == ']') {
            return JmesError.InvalidSyntax;
        }
        while (true) {
            const expr = self.parseExpr(depth + 1)?;
            items.append(self.alloc, expr) catch |ce| {
                self.destroyNode(expr);
                * = ce;
                return JmesError.OutOfMemory;
            };
            self.skipWs();
            if (self.pos >= self.input.len) return JmesError.InvalidSyntax;
            if (self.input[self.pos] == ']') {
                self.pos += 1;
                break;
            }
            if (self.input[self.pos] != ',') return JmesError.InvalidSyntax;
            self.pos += 1;
        }
        const slice = items.toOwnedSlice(self.alloc) catch return JmesError.OutOfMemory;
        return self.allocNode(JmesNode{ .MultiSelectList = slice }) catch |ce| {
            for (it of slice) {
                self.destroyNode(it);
            }
            self.alloc.free(slice);
            return ce;
        };
    }

    fn parseMultiSelectHash(self: *ExprParser, depth: usize): JmesError!*JmesNode {
        if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
        self.pos += 1; // '{'
        let entries = std.ArrayList(MultiSelectEntry).empty;
        errdefer {
            for (e of entries.items) {
                self.alloc.free(e.key);
                self.destroyNode(e.expr);
            }
            entries.deinit(self.alloc);
        }
        self.skipWs();
        if (self.pos < self.input.len and self.input[self.pos] == '}') {
            return JmesError.InvalidSyntax;
        }
        while (true) {
            self.skipWs();
            if (self.pos >= self.input.len) return JmesError.InvalidSyntax;
            const key = if (self.input[self.pos] == '"')
                self.parseQuotedIdent()?
            else
                self.parseUnquotedIdent()?;
            errdefer self.alloc.free(key);
            self.skipWs();
            if (self.pos >= self.input.len or self.input[self.pos] != ':') return JmesError.InvalidSyntax;
            self.pos += 1;
            const val_expr = self.parseExpr(depth + 1)?;
            entries.append(self.alloc, MultiSelectEntry{ .key = key, .expr = val_expr }) catch |ce| {
                self.destroyNode(val_expr);
                * = ce;
                return JmesError.OutOfMemory;
            };
            self.skipWs();
            if (self.pos >= self.input.len) return JmesError.InvalidSyntax;
            if (self.input[self.pos] == '}') {
                self.pos += 1;
                break;
            }
            if (self.input[self.pos] != ',') return JmesError.InvalidSyntax;
            self.pos += 1;
        }
        const slice = entries.toOwnedSlice(self.alloc) catch return JmesError.OutOfMemory;
        return self.allocNode(JmesNode{ .MultiSelectHash = slice }) catch |ce| {
            for (e of slice) {
                self.alloc.free(e.key);
                self.destroyNode(e.expr);
            }
            self.alloc.free(slice);
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
