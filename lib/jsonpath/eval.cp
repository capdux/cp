// Source: cp/lib/jsonpath/eval.cp (RFC 9535 Evaluation Engine & Normalized Path)

const std = @import("std");
const jv = @import("./json_val.cp");
const ast = @import("./ast.cp");

const JsonKind = jv.JsonKind;
const FilterValKind = ast.FilterValKind;
const CmpOp = ast.CmpOp;
const FilterExprKind = ast.FilterExprKind;
const SelectorKind = ast.SelectorKind;

pub enum QueryError {
    MaxDepthExceeded,
    MaxNodesExceeded,
    OutOfMemory,
}

pub struct QueryLimits {
    pub max_depth: usize,
    pub max_nodes: usize,
}

pub fn defaultQueryLimits(): QueryLimits {
    return QueryLimits{
        .max_depth = 64,
        .max_nodes = 4096,
    };
}

pub struct NodeMatch {
    pub value: *const jv.JsonVal,
    pub path: []const u8,
}

pub fn deinitMatches(matches: []NodeMatch, alloc: std.mem.Allocator) {
    for (m of matches) {
        alloc.free(m.path);
    }
    if (matches.len > 0) {
        alloc.free(matches);
    }
}

enum PathStepKind {
    Key,
    Index,
}

struct PathStep {
    kind: PathStepKind,
    key: []const u8,
    index: usize,
}

struct NodeRef {
    val: *const jv.JsonVal,
    steps: []PathStep,
}

fn deinitNodeRef(nr: *NodeRef, alloc: std.mem.Allocator) {
    if (nr.steps.len > 0) {
        alloc.free(nr.steps);
    }
}

fn appendStep(alloc: std.mem.Allocator, base: []const PathStep, step: PathStep): anyerror![]PathStep {
    let next = try alloc.alloc(PathStep, base.len + 1);
    if (base.len > 0) {
        @memcpy(next[0..base.len], base);
    }
    next[base.len] = step;
    return next;
}

pub fn formatNormalizedPath(alloc: std.mem.Allocator, steps: []const PathStep): anyerror![]const u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);
    try buf.append(alloc, '$');
    for (st of steps) {
        switch (st.kind) {
            PathStepKind.Key => {
                try buf.appendSlice(alloc, "['");
                for (c of st.key) {
                    switch (c) {
                        '\'' => try buf.appendSlice(alloc, "\\'"),
                        '\\' => try buf.appendSlice(alloc, "\\\\"),
                        '\n' => try buf.appendSlice(alloc, "\\n"),
                        '\r' => try buf.appendSlice(alloc, "\\r"),
                        '\t' => try buf.appendSlice(alloc, "\\t"),
                        0x08 => try buf.appendSlice(alloc, "\\b"),
                        0x0c => try buf.appendSlice(alloc, "\\f"),
                        else => {
                            if (c < 0x20) {
                                let hex_buf: [6]u8 = undefined;
                                const formatted = try std.fmt.bufPrint(&hex_buf, "\\u{x:0>4}", .{c});
                                try buf.appendSlice(alloc, formatted);
                            } else {
                                try buf.append(alloc, c);
                            }
                        },
                    }
                }
                try buf.appendSlice(alloc, "']");
            },
            PathStepKind.Index => {
                let idx_buf: [32]u8 = undefined;
                const formatted = try std.fmt.bufPrint(&idx_buf, "[{d}]", .{st.index});
                try buf.appendSlice(alloc, formatted);
            },
        }
    }
    return try buf.toOwnedSlice(alloc);
}

struct EvalCtx {
    alloc: std.mem.Allocator,
    root: *const jv.JsonVal,
    limits: QueryLimits,
}

fn collectDescendants(
    ctx: *EvalCtx,
    node: *const jv.JsonVal,
    steps: []const PathStep,
    depth: usize,
    out: *std.ArrayList(NodeRef),
): QueryError!void {
    if (depth > ctx.limits.max_depth) {
        return QueryError.MaxDepthExceeded;
    }
    if (out.items.len >= ctx.limits.max_nodes) {
        return QueryError.MaxNodesExceeded;
    }
    let copy_steps = ctx.alloc.alloc(PathStep, steps.len) catch return QueryError.OutOfMemory;
    if (steps.len > 0) {
        @memcpy(copy_steps, steps);
    }
    out.append(ctx.alloc, NodeRef{ .val = node, .steps = copy_steps }) catch return QueryError.OutOfMemory;

    switch (node.kind) {
        JsonKind.Array => {
            let i: usize = 0;
            while (i < node.arr_val.len) {
                const next_steps = appendStep(ctx.alloc, steps, PathStep{
                    .kind = PathStepKind.Index,
                    .key = "",
                    .index = i,
                }) catch return QueryError.OutOfMemory;
                defer ctx.alloc.free(next_steps);
                try collectDescendants(ctx, &node.arr_val[i], next_steps, depth + 1, out);
                i += 1;
            }
        },
        JsonKind.Object => {
            let i: usize = 0;
            while (i < node.obj_val.len) {
                const next_steps = appendStep(ctx.alloc, steps, PathStep{
                    .kind = PathStepKind.Key,
                    .key = node.obj_val[i].key,
                    .index = 0,
                }) catch return QueryError.OutOfMemory;
                defer ctx.alloc.free(next_steps);
                try collectDescendants(ctx, &node.obj_val[i].value, next_steps, depth + 1, out);
                i += 1;
            }
        },
        else => {},
    }
}

struct SliceBounds {
    start: i64,
    end: i64,
    step: i64,
}

fn evalSliceBounds(len: usize, spec: ast.SliceSpec): SliceBounds {
    const n: i64 = @as(i64, @intCast(len));
    const step = spec.step;
    let start: i64 = 0;
    let end: i64 = n;
    if (step >= 0) {
        start = if (spec.start != null) spec.start.? else 0;
        end = if (spec.end != null) spec.end.? else n;
        if (start < 0) {
            start += n;
        }
        if (end < 0) {
            end += n;
        }
        if (start < 0) {
            start = 0;
        }
        if (start > n) {
            start = n;
        }
        if (end < 0) {
            end = 0;
        }
        if (end > n) {
            end = n;
        }
    } else {
        start = if (spec.start != null) spec.start.? else n - 1;
        end = if (spec.end != null) spec.end.? else -n - 1;
        if (start < 0) {
            start += n;
        }
        if (end < 0) {
            end += n;
        }
        if (start < -1) {
            start = -1;
        }
        if (start >= n) {
            start = n - 1;
        }
        if (end < -1) {
            end = -1;
        }
        if (end >= n) {
            end = n - 1;
        }
    }
    return SliceBounds{ .start = start, .end = end, .step = step };
}

enum ResolvedKind {
    Nothing,
    Bool,
    Number,
    String,
    Null,
    JsonRef,
}

struct ResolvedVal {
    kind: ResolvedKind,
    bool_val: bool,
    num_val: f64,
    str_val: []const u8,
    json_ref: (*const jv.JsonVal)?,
}

fn fromJsonVal(v: *const jv.JsonVal): ResolvedVal {
    switch (v.kind) {
        JsonKind.Null => return ResolvedVal{ .kind = ResolvedKind.Null, .bool_val = false, .num_val = 0.0, .str_val = "", .json_ref = v },
        JsonKind.Bool => return ResolvedVal{ .kind = ResolvedKind.Bool, .bool_val = v.bool_val, .num_val = 0.0, .str_val = "", .json_ref = v },
        JsonKind.Number => return ResolvedVal{ .kind = ResolvedKind.Number, .bool_val = false, .num_val = v.num_val, .str_val = "", .json_ref = v },
        JsonKind.String => return ResolvedVal{ .kind = ResolvedKind.String, .bool_val = false, .num_val = 0.0, .str_val = v.str_val, .json_ref = v },
        else => return ResolvedVal{ .kind = ResolvedKind.JsonRef, .bool_val = false, .num_val = 0.0, .str_val = "", .json_ref = v },
    }
}

fn evalSegmentsOnNode(
    ctx: *EvalCtx,
    start_node: *const jv.JsonVal,
    segments: []const ast.Segment,
    out: *std.ArrayList(NodeRef),
): QueryError!void {
    let current = std.ArrayList(NodeRef).empty;
    defer {
        for (*nr of current.items) {
            deinitNodeRef(nr, ctx.alloc);
        }
        current.deinit(ctx.alloc);
    }
    const empty_steps = ctx.alloc.alloc(PathStep, 0) catch return QueryError.OutOfMemory;
    current.append(ctx.alloc, NodeRef{ .val = start_node, .steps = empty_steps }) catch return QueryError.OutOfMemory;

    for (seg of segments) {
        let next = std.ArrayList(NodeRef).empty;
        errdefer {
            for (*nr of next.items) {
                deinitNodeRef(nr, ctx.alloc);
            }
            next.deinit(ctx.alloc);
        }

        for (nr of current.items) {
            let targets = std.ArrayList(NodeRef).empty;
            defer {
                for (*t of targets.items) {
                    deinitNodeRef(t, ctx.alloc);
                }
                targets.deinit(ctx.alloc);
            }

            if (seg.kind == ast.SegmentKind.Descendant) {
                try collectDescendants(ctx, nr.val, nr.steps, 0, &targets);
            } else {
                let copy_steps = ctx.alloc.alloc(PathStep, nr.steps.len) catch return QueryError.OutOfMemory;
                if (nr.steps.len > 0) {
                    @memcpy(copy_steps, nr.steps);
                }
                targets.append(ctx.alloc, NodeRef{ .val = nr.val, .steps = copy_steps }) catch return QueryError.OutOfMemory;
            }

            for (t of targets.items) {
                for (sel of seg.selectors) {
                    if (next.items.len >= ctx.limits.max_nodes) {
                        return QueryError.MaxNodesExceeded;
                    }
                    try applySelector(ctx, t.val, t.steps, sel, &next);
                }
            }
        }

        for (*nr of current.items) {
            deinitNodeRef(nr, ctx.alloc);
        }
        current.deinit(ctx.alloc);
        current = next;
    }

    for (nr of current.items) {
        if (out.items.len >= ctx.limits.max_nodes) {
            return QueryError.MaxNodesExceeded;
        }
        let copy_steps = ctx.alloc.alloc(PathStep, nr.steps.len) catch return QueryError.OutOfMemory;
        if (nr.steps.len > 0) {
            @memcpy(copy_steps, nr.steps);
        }
        out.append(ctx.alloc, NodeRef{ .val = nr.val, .steps = copy_steps }) catch return QueryError.OutOfMemory;
    }
}

fn evalFilterVal(ctx: *EvalCtx, cur: *const jv.JsonVal, fv: ast.FilterVal): QueryError!ResolvedVal {
    switch (fv.kind) {
        FilterValKind.Null => return ResolvedVal{ .kind = ResolvedKind.Null, .bool_val = false, .num_val = 0.0, .str_val = "", .json_ref = null },
        FilterValKind.Bool => return ResolvedVal{ .kind = ResolvedKind.Bool, .bool_val = fv.bool_val, .num_val = 0.0, .str_val = "", .json_ref = null },
        FilterValKind.Number => return ResolvedVal{ .kind = ResolvedKind.Number, .bool_val = false, .num_val = fv.num_val, .str_val = "", .json_ref = null },
        FilterValKind.String => return ResolvedVal{ .kind = ResolvedKind.String, .bool_val = false, .num_val = 0.0, .str_val = fv.str_val, .json_ref = null },
        FilterValKind.Query => {
            const fq = fv.query.?;
            const start = if (fq.is_root) ctx.root else cur;
            let res = std.ArrayList(NodeRef).empty;
            defer {
                for (*nr of res.items) {
                    deinitNodeRef(nr, ctx.alloc);
                }
                res.deinit(ctx.alloc);
            }
            try evalSegmentsOnNode(ctx, start, fq.segments, &res);
            if (res.items.len == 1) {
                return fromJsonVal(res.items[0].val);
            }
            return ResolvedVal{ .kind = ResolvedKind.Nothing, .bool_val = false, .num_val = 0.0, .str_val = "", .json_ref = null };
        },
        FilterValKind.FuncValue => {
            const fq = fv.query.?;
            const start = if (fq.is_root) ctx.root else cur;
            let res = std.ArrayList(NodeRef).empty;
            defer {
                for (*nr of res.items) {
                    deinitNodeRef(nr, ctx.alloc);
                }
                res.deinit(ctx.alloc);
            }
            try evalSegmentsOnNode(ctx, start, fq.segments, &res);
            if (res.items.len == 1) {
                return fromJsonVal(res.items[0].val);
            }
            return ResolvedVal{ .kind = ResolvedKind.Nothing, .bool_val = false, .num_val = 0.0, .str_val = "", .json_ref = null };
        },
        FilterValKind.FuncCount => {
            const fq = fv.query.?;
            const start = if (fq.is_root) ctx.root else cur;
            let res = std.ArrayList(NodeRef).empty;
            defer {
                for (*nr of res.items) {
                    deinitNodeRef(nr, ctx.alloc);
                }
                res.deinit(ctx.alloc);
            }
            try evalSegmentsOnNode(ctx, start, fq.segments, &res);
            return ResolvedVal{
                .kind = ResolvedKind.Number,
                .bool_val = false,
                .num_val = @as(f64, @floatFromInt(res.items.len)),
                .str_val = "",
                .json_ref = null,
            };
        },
        FilterValKind.FuncLength => {
            const fq = fv.query.?;
            const start = if (fq.is_root) ctx.root else cur;
            let res = std.ArrayList(NodeRef).empty;
            defer {
                for (*nr of res.items) {
                    deinitNodeRef(nr, ctx.alloc);
                }
                res.deinit(ctx.alloc);
            }
            try evalSegmentsOnNode(ctx, start, fq.segments, &res);
            if (res.items.len != 1) {
                return ResolvedVal{ .kind = ResolvedKind.Nothing, .bool_val = false, .num_val = 0.0, .str_val = "", .json_ref = null };
            }
            const target = res.items[0].val;
            switch (target.kind) {
                JsonKind.String => {
                    const count = std.unicode.utf8CountCodepoints(target.str_val) catch target.str_val.len;
                    return ResolvedVal{ .kind = ResolvedKind.Number, .bool_val = false, .num_val = @as(f64, @floatFromInt(count)), .str_val = "", .json_ref = null };
                },
                JsonKind.Array => return ResolvedVal{ .kind = ResolvedKind.Number, .bool_val = false, .num_val = @as(f64, @floatFromInt(target.arr_val.len)), .str_val = "", .json_ref = null },
                JsonKind.Object => return ResolvedVal{ .kind = ResolvedKind.Number, .bool_val = false, .num_val = @as(f64, @floatFromInt(target.obj_val.len)), .str_val = "", .json_ref = null },
                else => return ResolvedVal{ .kind = ResolvedKind.Nothing, .bool_val = false, .num_val = 0.0, .str_val = "", .json_ref = null },
            }
        },
    }
}

fn resolvedEqual(a: ResolvedVal, b: ResolvedVal): bool {
    if (a.kind == ResolvedKind.Nothing and b.kind == ResolvedKind.Nothing) {
        return true;
    }
    if (a.kind != b.kind) {
        return false;
    }
    switch (a.kind) {
        ResolvedKind.Nothing => return true,
        ResolvedKind.Null => return true,
        ResolvedKind.Bool => return a.bool_val == b.bool_val,
        ResolvedKind.Number => return a.num_val == b.num_val,
        ResolvedKind.String => return std.mem.eql(u8, a.str_val, b.str_val),
        ResolvedKind.JsonRef => {
            if (a.json_ref != null and b.json_ref != null) {
                return jv.jsonEqual(a.json_ref.?, b.json_ref.?);
            }
            return false;
        },
    }
}

fn resolvedLess(a: ResolvedVal, b: ResolvedVal): bool {
    if (a.kind == ResolvedKind.Number and b.kind == ResolvedKind.Number) {
        return a.num_val < b.num_val;
    }
    if (a.kind == ResolvedKind.String and b.kind == ResolvedKind.String) {
        return std.mem.order(u8, a.str_val, b.str_val) == std.math.Order.lt;
    }
    return false;
}

fn evalCmp(op: CmpOp, a: ResolvedVal, b: ResolvedVal): bool {
    switch (op) {
        CmpOp.Eq => return resolvedEqual(a, b),
        CmpOp.Ne => return !resolvedEqual(a, b),
        CmpOp.Lt => return resolvedLess(a, b),
        CmpOp.Le => return resolvedLess(a, b) or resolvedEqual(a, b),
        CmpOp.Gt => return resolvedLess(b, a),
        CmpOp.Ge => return resolvedLess(b, a) or resolvedEqual(a, b),
    }
}

fn evalFilterExpr(ctx: *EvalCtx, cur: *const jv.JsonVal, expr: *const ast.FilterExpr): QueryError!bool {
    switch (expr.kind) {
        FilterExprKind.BoolLit => return expr.bool_val,
        FilterExprKind.Not => {
            const v = try evalFilterExpr(ctx, cur, expr.left_expr.?);
            return !v;
        },
        FilterExprKind.And => {
            const l = try evalFilterExpr(ctx, cur, expr.left_expr.?);
            if (!l) {
                return false;
            }
            return try evalFilterExpr(ctx, cur, expr.right_expr.?);
        },
        FilterExprKind.Or => {
            const l = try evalFilterExpr(ctx, cur, expr.left_expr.?);
            if (l) {
                return true;
            }
            return try evalFilterExpr(ctx, cur, expr.right_expr.?);
        },
        FilterExprKind.TestQuery => {
            const fq = expr.test_query.?;
            const start = if (fq.is_root) ctx.root else cur;
            let res = std.ArrayList(NodeRef).empty;
            defer {
                for (*nr of res.items) {
                    deinitNodeRef(nr, ctx.alloc);
                }
                res.deinit(ctx.alloc);
            }
            try evalSegmentsOnNode(ctx, start, fq.segments, &res);
            return res.items.len > 0;
        },
        FilterExprKind.Cmp => {
            const lv = try evalFilterVal(ctx, cur, expr.left_val.?);
            const rv = try evalFilterVal(ctx, cur, expr.right_val.?);
            return evalCmp(expr.op, lv, rv);
        },
    }
}

fn applySelector(
    ctx: *EvalCtx,
    node: *const jv.JsonVal,
    steps: []const PathStep,
    sel: ast.Selector,
    out: *std.ArrayList(NodeRef),
): QueryError!void {
    switch (sel.kind) {
        SelectorKind.Name => {
            if (node.kind == JsonKind.Object) {
                let i: usize = 0;
                while (i < node.obj_val.len) {
                    if (std.mem.eql(u8, node.obj_val[i].key, sel.name)) {
                        const next_steps = appendStep(ctx.alloc, steps, PathStep{
                            .kind = PathStepKind.Key,
                            .key = node.obj_val[i].key,
                            .index = 0,
                        }) catch return QueryError.OutOfMemory;
                        out.append(ctx.alloc, NodeRef{ .val = &node.obj_val[i].value, .steps = next_steps }) catch return QueryError.OutOfMemory;
                        break;
                    }
                    i += 1;
                }
            }
        },
        SelectorKind.Wildcard => {
            if (node.kind == JsonKind.Object) {
                let i: usize = 0;
                while (i < node.obj_val.len) {
                    if (out.items.len >= ctx.limits.max_nodes) {
                        return QueryError.MaxNodesExceeded;
                    }
                    const next_steps = appendStep(ctx.alloc, steps, PathStep{
                        .kind = PathStepKind.Key,
                        .key = node.obj_val[i].key,
                        .index = 0,
                    }) catch return QueryError.OutOfMemory;
                    out.append(ctx.alloc, NodeRef{ .val = &node.obj_val[i].value, .steps = next_steps }) catch return QueryError.OutOfMemory;
                    i += 1;
                }
            } else if (node.kind == JsonKind.Array) {
                let i: usize = 0;
                while (i < node.arr_val.len) {
                    if (out.items.len >= ctx.limits.max_nodes) {
                        return QueryError.MaxNodesExceeded;
                    }
                    const next_steps = appendStep(ctx.alloc, steps, PathStep{
                        .kind = PathStepKind.Index,
                        .key = "",
                        .index = i,
                    }) catch return QueryError.OutOfMemory;
                    out.append(ctx.alloc, NodeRef{ .val = &node.arr_val[i], .steps = next_steps }) catch return QueryError.OutOfMemory;
                    i += 1;
                }
            }
        },
        SelectorKind.Index => {
            if (node.kind == JsonKind.Array) {
                const n: i64 = @as(i64, @intCast(node.arr_val.len));
                const idx: i64 = if (sel.index < 0) sel.index + n else sel.index;
                if (idx >= 0 and idx < n) {
                    const u_idx: usize = @as(usize, @intCast(idx));
                    const next_steps = appendStep(ctx.alloc, steps, PathStep{
                        .kind = PathStepKind.Index,
                        .key = "",
                        .index = u_idx,
                    }) catch return QueryError.OutOfMemory;
                    out.append(ctx.alloc, NodeRef{ .val = &node.arr_val[u_idx], .steps = next_steps }) catch return QueryError.OutOfMemory;
                }
            }
        },
        SelectorKind.Slice => {
            if (node.kind == JsonKind.Array) {
                const b = evalSliceBounds(node.arr_val.len, sel.slice);
                let i: i64 = b.start;
                if (b.step > 0) {
                    while (i < b.end) {
                        if (out.items.len >= ctx.limits.max_nodes) {
                            return QueryError.MaxNodesExceeded;
                        }
                        const u_idx: usize = @as(usize, @intCast(i));
                        const next_steps = appendStep(ctx.alloc, steps, PathStep{
                            .kind = PathStepKind.Index,
                            .key = "",
                            .index = u_idx,
                        }) catch return QueryError.OutOfMemory;
                        out.append(ctx.alloc, NodeRef{ .val = &node.arr_val[u_idx], .steps = next_steps }) catch return QueryError.OutOfMemory;
                        i += b.step;
                    }
                } else {
                    while (i > b.end) {
                        if (out.items.len >= ctx.limits.max_nodes) {
                            return QueryError.MaxNodesExceeded;
                        }
                        const u_idx: usize = @as(usize, @intCast(i));
                        const next_steps = appendStep(ctx.alloc, steps, PathStep{
                            .kind = PathStepKind.Index,
                            .key = "",
                            .index = u_idx,
                        }) catch return QueryError.OutOfMemory;
                        out.append(ctx.alloc, NodeRef{ .val = &node.arr_val[u_idx], .steps = next_steps }) catch return QueryError.OutOfMemory;
                        i += b.step;
                    }
                }
            }
        },
        SelectorKind.Filter => {
            const expr = sel.filter.?;
            if (node.kind == JsonKind.Array) {
                let i: usize = 0;
                while (i < node.arr_val.len) {
                    if (try evalFilterExpr(ctx, &node.arr_val[i], expr)) {
                        if (out.items.len >= ctx.limits.max_nodes) {
                            return QueryError.MaxNodesExceeded;
                        }
                        const next_steps = appendStep(ctx.alloc, steps, PathStep{
                            .kind = PathStepKind.Index,
                            .key = "",
                            .index = i,
                        }) catch return QueryError.OutOfMemory;
                        out.append(ctx.alloc, NodeRef{ .val = &node.arr_val[i], .steps = next_steps }) catch return QueryError.OutOfMemory;
                    }
                    i += 1;
                }
            } else if (node.kind == JsonKind.Object) {
                let i: usize = 0;
                while (i < node.obj_val.len) {
                    if (try evalFilterExpr(ctx, &node.obj_val[i].value, expr)) {
                        if (out.items.len >= ctx.limits.max_nodes) {
                            return QueryError.MaxNodesExceeded;
                        }
                        const next_steps = appendStep(ctx.alloc, steps, PathStep{
                            .kind = PathStepKind.Key,
                            .key = node.obj_val[i].key,
                            .index = 0,
                        }) catch return QueryError.OutOfMemory;
                        out.append(ctx.alloc, NodeRef{ .val = &node.obj_val[i].value, .steps = next_steps }) catch return QueryError.OutOfMemory;
                    }
                    i += 1;
                }
            }
        },
    }
}

pub fn queryJsonPathWithLimits(
    alloc: std.mem.Allocator,
    root: *const jv.JsonVal,
    query: *const ast.JsonPathQuery,
    limits: QueryLimits,
): QueryError![]NodeMatch {
    let ctx = EvalCtx{
        .alloc = alloc,
        .root = root,
        .limits = limits,
    };
    let raw = std.ArrayList(NodeRef).empty;
    defer {
        for (*nr of raw.items) {
            deinitNodeRef(nr, alloc);
        }
        raw.deinit(alloc);
    }
    try evalSegmentsOnNode(&ctx, root, query.segments, &raw);

    let matches = std.ArrayList(NodeMatch).empty;
    errdefer deinitMatches(matches.items, alloc);
    for (nr of raw.items) {
        const path = formatNormalizedPath(alloc, nr.steps) catch return QueryError.OutOfMemory;
        matches.append(alloc, NodeMatch{ .value = nr.val, .path = path }) catch return QueryError.OutOfMemory;
    }
    return matches.toOwnedSlice(alloc) catch return QueryError.OutOfMemory;
}

pub fn queryJsonPath(
    alloc: std.mem.Allocator,
    root: *const jv.JsonVal,
    query: *const ast.JsonPathQuery,
): QueryError![]NodeMatch {
    return queryJsonPathWithLimits(alloc, root, query, defaultQueryLimits());
}

pub fn queryJsonPathValues(
    alloc: std.mem.Allocator,
    root: *const jv.JsonVal,
    query: *const ast.JsonPathQuery,
): QueryError![]*const jv.JsonVal {
    const matches = try queryJsonPath(alloc, root, query);
    defer deinitMatches(matches, alloc);
    let vals = alloc.alloc(*const jv.JsonVal, matches.len) catch return QueryError.OutOfMemory;
    let i: usize = 0;
    while (i < matches.len) {
        vals[i] = matches[i].value;
        i += 1;
    }
    return vals;
}

pub fn queryJsonPathStrings(
    alloc: std.mem.Allocator,
    root: *const jv.JsonVal,
    query: *const ast.JsonPathQuery,
): anyerror![]string {
    const matches = try queryJsonPath(alloc, root, query);
    defer deinitMatches(matches, alloc);
    let out = alloc.alloc(string, matches.len) catch return QueryError.OutOfMemory;
    errdefer alloc.free(out);
    let filled: usize = 0;
    errdefer {
        for (s of out[0..filled]) {
            alloc.free(s);
        }
    }
    let i: usize = 0;
    while (i < matches.len) {
        out[i] = try jv.stringifyJson(alloc, matches[i].value);
        filled += 1;
        i += 1;
    }
    return out;
}

pub fn deinitStrings(strs: []string, alloc: std.mem.Allocator) {
    for (s of strs) {
        alloc.free(s);
    }
    if (strs.len > 0) {
        alloc.free(strs);
    }
}
