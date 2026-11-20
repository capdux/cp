// Source: cp/lib/jmespath/eval.cp (JMESPath evaluator & built-in functions)
const std = @import("std");
import { MAX_DEPTH, JmesError, JmesEntry, JmesValue, parseJson, stringifyJson } from "./value.cp";
import { JmesNode, JmesExpr, CompareOp, SliceSpec, compileJmesPath } from "./parser.cp";

fn isProjectionNode(node: *const JmesNode): bool {
    switch (node.*) {
        .ListWildcard, .ObjectWildcard, .Slice, .Filter, .Flatten => return true,
        .IndexExpr => |b| return isProjectionNode(b.left) or isProjectionNode(b.right),
        .SubExpr => |b| return isProjectionNode(b.left) or isProjectionNode(b.right),
        else => return false,
    }
}

fn evalSlice(alloc: std.mem.Allocator, arr: []JmesValue, spec: SliceSpec): JmesError!JmesValue {
    const len_i: i64 = @as(i64, @intCast(arr.len));
    const step: i64 = spec.step orelse 1;
    if (step == 0) return JmesError.InvalidSliceStep;

    let start: i64 = spec.start orelse (if (step > 0) 0 else len_i - 1);
    let stop: i64 = spec.stop orelse (if (step > 0) len_i else -len_i - 1);

    if (spec.start != null) {
        if (start < 0) {
            start += len_i;
            if (start < 0) start = if (step > 0) 0 else -1;
        } else if (start >= len_i) {
            start = if (step > 0) len_i else len_i - 1;
        }
    }
    if (spec.stop != null) {
        if (stop < 0) {
            stop += len_i;
            if (stop < 0) stop = if (step > 0) 0 else -1;
        } else if (stop >= len_i) {
            stop = if (step > 0) len_i else len_i - 1;
        }
    }

    let out = std.ArrayList(JmesValue).empty;
    errdefer {
        for (*it of out.items) {
            it.deinit(alloc);
        }
        out.deinit(alloc);
    }

    let idx = start;
    if (step > 0) {
        while (idx < stop) : (idx += step) {
            if (idx >= 0 and idx < len_i) {
                const c = arr[@as(usize, @intCast(idx))].clone(alloc)?;
                out.append(alloc, c) catch return JmesError.OutOfMemory;
            }
        }
    } else {
        while (idx > stop) : (idx += step) {
            if (idx >= 0 and idx < len_i) {
                const c = arr[@as(usize, @intCast(idx))].clone(alloc)?;
                out.append(alloc, c) catch return JmesError.OutOfMemory;
            }
        }
    }
    const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
    return JmesValue{ .Array = slice };
}

fn evalFlatten(alloc: std.mem.Allocator, val: JmesValue): JmesError!JmesValue {
    switch (val) {
        .Array => |arr| {
            let out = std.ArrayList(JmesValue).empty;
            errdefer {
                for (*it of out.items) {
                    it.deinit(alloc);
                }
                out.deinit(alloc);
            }
            for (item of arr) {
                switch (item) {
                    .Array => |sub| {
                        for (sub_item of sub) {
                            const c = sub_item.clone(alloc)?;
                            out.append(alloc, c) catch return JmesError.OutOfMemory;
                        }
                    },
                    else => {
                        const c = item.clone(alloc)?;
                        out.append(alloc, c) catch return JmesError.OutOfMemory;
                    },
                }
            }
            const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
            return JmesValue{ .Array = slice };
        },
        else => return .Null,
    }
}

fn mapProjection(alloc: std.mem.Allocator, list_val: JmesValue, rhs: *const JmesNode, depth: usize): JmesError!JmesValue {
    switch (list_val) {
        .Array => |arr| {
            let out = std.ArrayList(JmesValue).empty;
            errdefer {
                for (*it of out.items) {
                    it.deinit(alloc);
                }
                out.deinit(alloc);
            }
            for (item of arr) {
                let mapped = evalNode(alloc, rhs, item, depth + 1)?;
                if (mapped == .Null) {
                    mapped.deinit(alloc);
                } else {
                    out.append(alloc, mapped) catch |ce| {
                        mapped.deinit(alloc);
                        * = ce;
                        return JmesError.OutOfMemory;
                    };
                }
            }
            const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
            return JmesValue{ .Array = slice };
        },
        else => return .Null,
    }
}

fn evalIndexOrSub(alloc: std.mem.Allocator, left_node: *const JmesNode, right_node: *const JmesNode, data: JmesValue, depth: usize): JmesError!JmesValue {
    let left_val = evalNode(alloc, left_node, data, depth + 1)?;
    defer left_val.deinit(alloc);
    if (isProjectionNode(left_node)) {
        return mapProjection(alloc, left_val, right_node, depth);
    }
    return evalNode(alloc, right_node, left_val, depth + 1);
}

fn compareValues(op: CompareOp, left: JmesValue, right: JmesValue): bool {
    switch (op) {
        .Eq => return left.equals(right),
        .Ne => return !left.equals(right),
        .Lt, .Le, .Gt, .Ge => {
            const ln = left.asNumber() orelse return false;
            const rn = right.asNumber() orelse return false;
            switch (op) {
                .Lt => return ln < rn,
                .Le => return ln <= rn,
                .Gt => return ln > rn,
                .Ge => return ln >= rn,
                else => return false,
            }
        },
    }
}

fn evalFunction(alloc: std.mem.Allocator, name: []const u8, arg_nodes: []*JmesNode, data: JmesValue, depth: usize): JmesError!JmesValue {
    let args = std.ArrayList(JmesValue).empty;
    defer {
        for (*a of args.items) {
            a.deinit(alloc);
        }
        args.deinit(alloc);
    }
    for (an of arg_nodes) {
        const av = evalNode(alloc, an, data, depth + 1)?;
        args.append(alloc, av) catch |ce| {
            let mav = av;
            mav.deinit(alloc);
            * = ce;
            return JmesError.OutOfMemory;
        };
    }

    if (std.mem.eql(u8, name, "length")) {
        if (args.items.len != 1) return JmesError.InvalidArity;
        switch (args.items[0]) {
            .String => |s| return JmesValue{ .Int = @as(i64, @intCast(s.len)) },
            .Array => |a| return JmesValue{ .Int = @as(i64, @intCast(a.len)) },
            .Object => |o| return JmesValue{ .Int = @as(i64, @intCast(o.len)) },
            else => return JmesError.InvalidArgumentType,
        }
    }

    if (std.mem.eql(u8, name, "keys")) {
        if (args.items.len != 1) return JmesError.InvalidArity;
        switch (args.items[0]) {
            .Object => |entries| {
                let out = std.ArrayList(JmesValue).empty;
                errdefer {
                    for (*it of out.items) it.deinit(alloc);
                    out.deinit(alloc);
                }
                for (e of entries) {
                    const k = alloc.dupe(u8, e.key) catch return JmesError.OutOfMemory;
                    out.append(alloc, JmesValue{ .String = k }) catch |ce| {
                        alloc.free(k);
                        * = ce;
                        return JmesError.OutOfMemory;
                    };
                }
                const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
                return JmesValue{ .Array = slice };
            },
            else => return JmesError.InvalidArgumentType,
        }
    }

    if (std.mem.eql(u8, name, "values")) {
        if (args.items.len != 1) return JmesError.InvalidArity;
        switch (args.items[0]) {
            .Object => |entries| {
                let out = std.ArrayList(JmesValue).empty;
                errdefer {
                    for (*it of out.items) it.deinit(alloc);
                    out.deinit(alloc);
                }
                for (e of entries) {
                    const v = e.value.clone(alloc)?;
                    out.append(alloc, v) catch return JmesError.OutOfMemory;
                }
                const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
                return JmesValue{ .Array = slice };
            },
            else => return JmesError.InvalidArgumentType,
        }
    }

    if (std.mem.eql(u8, name, "type")) {
        if (args.items.len != 1) return JmesError.InvalidArity;
        const tn = alloc.dupe(u8, args.items[0].typeName()) catch return JmesError.OutOfMemory;
        return JmesValue{ .String = tn };
    }

    if (std.mem.eql(u8, name, "not_null")) {
        if (args.items.len == 0) return JmesError.InvalidArity;
        for (a of args.items) {
            if (a != .Null) {
                return a.clone(alloc);
            }
        }
        return .Null;
    }

    if (std.mem.eql(u8, name, "contains")) {
        if (args.items.len != 2) return JmesError.InvalidArity;
        switch (args.items[0]) {
            .Array => |arr| {
                for (item of arr) {
                    if (item.equals(args.items[1])) return JmesValue{ .Bool = true };
                }
                return JmesValue{ .Bool = false };
            },
            .String => |s| {
                switch (args.items[1]) {
                    .String => |sub| return JmesValue{ .Bool = std.mem.indexOf(u8, s, sub) != null },
                    else => return JmesValue{ .Bool = false },
                }
            },
            else => return JmesError.InvalidArgumentType,
        }
    }

    if (std.mem.eql(u8, name, "starts_with")) {
        if (args.items.len != 2) return JmesError.InvalidArity;
        switch (args.items[0]) {
            .String => |s| {
                switch (args.items[1]) {
                    .String => |prefix| return JmesValue{ .Bool = std.mem.startsWith(u8, s, prefix) },
                    else => return JmesError.InvalidArgumentType,
                }
            },
            else => return JmesError.InvalidArgumentType,
        }
    }

    if (std.mem.eql(u8, name, "ends_with")) {
        if (args.items.len != 2) return JmesError.InvalidArity;
        switch (args.items[0]) {
            .String => |s| {
                switch (args.items[1]) {
                    .String => |suffix| return JmesValue{ .Bool = std.mem.endsWith(u8, s, suffix) },
                    else => return JmesError.InvalidArgumentType,
                }
            },
            else => return JmesError.InvalidArgumentType,
        }
    }

    if (std.mem.eql(u8, name, "join")) {
        if (args.items.len != 2) return JmesError.InvalidArity;
        const glue = switch (args.items[0]) {
            .String => |s| s,
            else => return JmesError.InvalidArgumentType,
        };
        const arr = switch (args.items[1]) {
            .Array => |a| a,
            else => return JmesError.InvalidArgumentType,
        };
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);
        let i: usize = 0;
        while (i < arr.len) : (i += 1) {
            const s = switch (arr[i]) {
                .String => |str_val| str_val,
                else => return JmesError.InvalidArgumentType,
            };
            if (i > 0) out.appendSlice(alloc, glue) catch return JmesError.OutOfMemory;
            out.appendSlice(alloc, s) catch return JmesError.OutOfMemory;
        }
        const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
        return JmesValue{ .String = slice };
    }

    if (std.mem.eql(u8, name, "reverse")) {
        if (args.items.len != 1) return JmesError.InvalidArity;
        switch (args.items[0]) {
            .Array => |arr| {
                let out = std.ArrayList(JmesValue).empty;
                errdefer {
                    for (*it of out.items) it.deinit(alloc);
                    out.deinit(alloc);
                }
                let i: usize = arr.len;
                while (i > 0) {
                    i -= 1;
                    const c = arr[i].clone(alloc)?;
                    out.append(alloc, c) catch return JmesError.OutOfMemory;
                }
                const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
                return JmesValue{ .Array = slice };
            },
            .String => |s| {
                let out = alloc.alloc(u8, s.len) catch return JmesError.OutOfMemory;
                let i: usize = 0;
                while (i < s.len) : (i += 1) {
                    out[i] = s[s.len - 1 - i];
                }
                return JmesValue{ .String = out };
            },
            else => return JmesError.InvalidArgumentType,
        }
    }

    if (std.mem.eql(u8, name, "sort")) {
        if (args.items.len != 1) return JmesError.InvalidArity;
        const arr = switch (args.items[0]) {
            .Array => |a| a,
            else => return JmesError.InvalidArgumentType,
        };
        let out = std.ArrayList(JmesValue).empty;
        errdefer {
            for (*it of out.items) it.deinit(alloc);
            out.deinit(alloc);
        }
        if (arr.len == 0) {
            const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
            return JmesValue{ .Array = slice };
        }
        const is_num = arr[0].asNumber() != null;
        const is_str = arr[0] == .String;
        if (!is_num and !is_str) return JmesError.InvalidArgumentType;

        for (item of arr) {
            if (is_num and item.asNumber() == null) return JmesError.InvalidArgumentType;
            if (is_str and item != .String) return JmesError.InvalidArgumentType;
            const c = item.clone(alloc)?;
            out.append(alloc, c) catch return JmesError.OutOfMemory;
        }

        // Insertion sort (stable and simple for JMESPath arrays)
        let i: usize = 1;
        while (i < out.items.len) : (i += 1) {
            const key = out.items[i];
            let j: usize = i;
            while (j > 0) {
                let should_swap = false;
                if (is_num) {
                    should_swap = out.items[j - 1].asNumber().? > key.asNumber().?;
                } else {
                    should_swap = std.mem.order(u8, out.items[j - 1].String, key.String) == .gt;
                }
                if (!should_swap) break;
                out.items[j] = out.items[j - 1];
                j -= 1;
            }
            out.items[j] = key;
        }
        const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
        return JmesValue{ .Array = slice };
    }

    if (std.mem.eql(u8, name, "max") or std.mem.eql(u8, name, "min")) {
        if (args.items.len != 1) return JmesError.InvalidArity;
        const arr = switch (args.items[0]) {
            .Array => |a| a,
            else => return JmesError.InvalidArgumentType,
        };
        if (arr.len == 0) return .Null;
        const is_max = std.mem.eql(u8, name, "max");
        const is_num = arr[0].asNumber() != null;
        const is_str = arr[0] == .String;
        if (!is_num and !is_str) return JmesError.InvalidArgumentType;

        let best_idx: usize = 0;
        let i: usize = 1;
        while (i < arr.len) : (i += 1) {
            if (is_num) {
                const cur = arr[i].asNumber() orelse return JmesError.InvalidArgumentType;
                const best = arr[best_idx].asNumber().?;
                if ((is_max and cur > best) or (!is_max and cur < best)) {
                    best_idx = i;
                }
            } else {
                if (arr[i] != .String) return JmesError.InvalidArgumentType;
                const ord = std.mem.order(u8, arr[i].String, arr[best_idx].String);
                if ((is_max and ord == .gt) or (!is_max and ord == .lt)) {
                    best_idx = i;
                }
            }
        }
        return arr[best_idx].clone(alloc);
    }

    return JmesError.InvalidFunction;
}

fn evalNode(alloc: std.mem.Allocator, node: *const JmesNode, data: JmesValue, depth: usize): JmesError!JmesValue {
    if (depth > MAX_DEPTH) return JmesError.MaxDepthExceeded;
    switch (node.*) {
        .Current => return data.clone(alloc),
        .Field => |key| {
            const found = data.getField(key) orelse return .Null;
            return found.clone(alloc);
        },
        .Index => |idx| {
            switch (data) {
                .Array => |arr| {
                    const len_i: i64 = @as(i64, @intCast(arr.len));
                    const actual: i64 = if (idx < 0) len_i + idx else idx;
                    if (actual < 0 or actual >= len_i) return .Null;
                    return arr[@as(usize, @intCast(actual))].clone(alloc);
                },
                else => return .Null,
            }
        },
        .Slice => |spec| {
            switch (data) {
                .Array => |arr| return evalSlice(alloc, arr, spec),
                else => return .Null,
            }
        },
        .Flatten => {
            return evalFlatten(alloc, data);
        },
        .ListWildcard => {
            switch (data) {
                .Array => |arr| {
                    let out = std.ArrayList(JmesValue).empty;
                    errdefer {
                        for (*it of out.items) it.deinit(alloc);
                        out.deinit(alloc);
                    }
                    for (item of arr) {
                        const c = item.clone(alloc)?;
                        out.append(alloc, c) catch return JmesError.OutOfMemory;
                    }
                    const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
                    return JmesValue{ .Array = slice };
                },
                else => return .Null,
            }
        },
        .ObjectWildcard => {
            switch (data) {
                .Object => |entries| {
                    let out = std.ArrayList(JmesValue).empty;
                    errdefer {
                        for (*it of out.items) it.deinit(alloc);
                        out.deinit(alloc);
                    }
                    for (e of entries) {
                        const c = e.value.clone(alloc)?;
                        out.append(alloc, c) catch return JmesError.OutOfMemory;
                    }
                    const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
                    return JmesValue{ .Array = slice };
                },
                else => return .Null,
            }
        },
        .Filter => |cond| {
            switch (data) {
                .Array => |arr| {
                    let out = std.ArrayList(JmesValue).empty;
                    errdefer {
                        for (*it of out.items) it.deinit(alloc);
                        out.deinit(alloc);
                    }
                    for (item of arr) {
                        let cv = evalNode(alloc, cond, item, depth + 1)?;
                        const ok = cv.isTruthy();
                        cv.deinit(alloc);
                        if (ok) {
                            const c = item.clone(alloc)?;
                            out.append(alloc, c) catch return JmesError.OutOfMemory;
                        }
                    }
                    const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
                    return JmesValue{ .Array = slice };
                },
                else => return .Null,
            }
        },
        .SubExpr => |b| return evalIndexOrSub(alloc, b.left, b.right, data, depth),
        .IndexExpr => |b| return evalIndexOrSub(alloc, b.left, b.right, data, depth),
        .Pipe => |b| {
            let lv = evalNode(alloc, b.left, data, depth + 1)?;
            defer lv.deinit(alloc);
            return evalNode(alloc, b.right, lv, depth + 1);
        },
        .MultiSelectList => |items| {
            if (data == .Null) return .Null;
            let out = std.ArrayList(JmesValue).empty;
            errdefer {
                for (*it of out.items) it.deinit(alloc);
                out.deinit(alloc);
            }
            for (expr of items) {
                const v = evalNode(alloc, expr, data, depth + 1)?;
                out.append(alloc, v) catch |ce| {
                    let mv = v;
                    mv.deinit(alloc);
                    * = ce;
                    return JmesError.OutOfMemory;
                };
            }
            const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
            return JmesValue{ .Array = slice };
        },
        .MultiSelectHash => |entries| {
            if (data == .Null) return .Null;
            let out = std.ArrayList(JmesEntry).empty;
            errdefer {
                for (*e of out.items) {
                    alloc.free(e.key);
                    e.value.deinit(alloc);
                }
                out.deinit(alloc);
            }
            for (entry of entries) {
                const k = alloc.dupe(u8, entry.key) catch return JmesError.OutOfMemory;
                const v = evalNode(alloc, entry.expr, data, depth + 1) catch |ce| {
                    alloc.free(k);
                    return ce;
                };
                out.append(alloc, JmesEntry{ .key = k, .value = v }) catch |ce| {
                    alloc.free(k);
                    let mv = v;
                    mv.deinit(alloc);
                    * = ce;
                    return JmesError.OutOfMemory;
                };
            }
            const slice = out.toOwnedSlice(alloc) catch return JmesError.OutOfMemory;
            return JmesValue{ .Object = slice };
        },
        .Literal => |v| return v.clone(alloc),
        .RawString => |s| {
            const copy = alloc.dupe(u8, s) catch return JmesError.OutOfMemory;
            return JmesValue{ .String = copy };
        },
        .Not => |inner| {
            let v = evalNode(alloc, inner, data, depth + 1)?;
            const truthy = v.isTruthy();
            v.deinit(alloc);
            return JmesValue{ .Bool = !truthy };
        },
        .And => |b| {
            let lv = evalNode(alloc, b.left, data, depth + 1)?;
            if (!lv.isTruthy()) return lv;
            lv.deinit(alloc);
            return evalNode(alloc, b.right, data, depth + 1);
        },
        .Or => |b| {
            let lv = evalNode(alloc, b.left, data, depth + 1)?;
            if (lv.isTruthy()) return lv;
            lv.deinit(alloc);
            return evalNode(alloc, b.right, data, depth + 1);
        },
        .Compare => |c| {
            let lv = evalNode(alloc, c.left, data, depth + 1)?;
            defer lv.deinit(alloc);
            let rv = evalNode(alloc, c.right, data, depth + 1)?;
            defer rv.deinit(alloc);
            return JmesValue{ .Bool = compareValues(c.op, lv, rv) };
        },
        .FunctionCall => |fc| return evalFunction(alloc, fc.name, fc.args, data, depth),
    }
}

pub fn searchJmesPath(alloc: std.mem.Allocator, expr: *const JmesExpr, data: JmesValue): JmesError!JmesValue {
    return evalNode(alloc, expr.root, data, 0);
}

pub fn searchJmesPathJson(alloc: std.mem.Allocator, expr_str: []const u8, json_str: []const u8): JmesError![]u8 {
    let expr = compileJmesPath(alloc, expr_str)?;
    defer expr.deinit(alloc);
    let data = parseJson(alloc, json_str)?;
    defer data.deinit(alloc);
    let res = searchJmesPath(alloc, &expr, data)?;
    defer res.deinit(alloc);
    return stringifyJson(alloc, res);
}
