// Source: cp/lib/jq/eval.cp (Multi-Output Stream Evaluator for cap:jq)
const std = @import("std");
import { JqError, JqTag, JqObjEntry, JqValue, freeValues, parseJson, stringifyJson } from "./value.cp";
import { INVALID_NODE, JqBinOp, JqBuiltin, JqExpr, JqNode, compileJq } from "./ast.cp";

pub const MAX_JQ_OUTPUTS: usize = 65536;
pub const MAX_JQ_EVAL_DEPTH: usize = 128;

fn emitVal(alloc: std.mem.Allocator, out: *std.ArrayList(JqValue), val: JqValue): JqError!void {
    if (out.items.len >= MAX_JQ_OUTPUTS) {
        val.freeContents(alloc);
        return JqError.OutputLimitExceeded;
    }
    out.append(alloc, val) catch {
        val.freeContents(alloc);
        return JqError.OutOfMemory;
    };
}

fn containsValue(haystack: JqValue, needle: JqValue): bool {
    if (haystack.tag == .String and needle.tag == .String) {
        return std.mem.indexOf(u8, haystack.str_val, needle.str_val) != null;
    }
    if (haystack.tag == .Array and needle.tag == .Array) {
        for (n_item of needle.arr_val) {
            let found = false;
            for (h_item of haystack.arr_val) {
                if (containsValue(h_item, n_item)) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }
    if (haystack.tag == .Object and needle.tag == .Object) {
        for (n_entry of needle.obj_val) {
            let found = false;
            for (h_entry of haystack.obj_val) {
                if (std.mem.eql(u8, h_entry.key, n_entry.key)) {
                    if (!containsValue(h_entry.value, n_entry.value)) return false;
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }
    return haystack.equals(needle);
}

fn addValues(alloc: std.mem.Allocator, a: JqValue, b: JqValue): JqError!JqValue {
    if (a.tag == .Null) return b.clone(alloc);
    if (b.tag == .Null) return a.clone(alloc);
    if (a.tag == .Int and b.tag == .Int) {
        return JqValue.intVal(a.int_val +% b.int_val);
    }
    if ((a.tag == .Int or a.tag == .Float) and (b.tag == .Int or b.tag == .Float)) {
        const fa = a.asNumber() orelse return JqError.TypeError;
        const fb = b.asNumber() orelse return JqError.TypeError;
        return JqValue.floatVal(fa + fb);
    }
    if (a.tag == .String and b.tag == .String) {
        let buf = alloc.alloc(u8, a.str_val.len + b.str_val.len) catch return JqError.OutOfMemory;
        @memcpy(buf[0..a.str_val.len], a.str_val);
        @memcpy(buf[a.str_val.len..], b.str_val);
        return .{ .tag = .String, .str_val = buf };
    }
    if (a.tag == .Array and b.tag == .Array) {
        const total = a.arr_val.len + b.arr_val.len;
        let arr = alloc.alloc(JqValue, total) catch return JqError.OutOfMemory;
        let filled: usize = 0;
        errdefer {
            let j: usize = 0;
            while (j < filled) {
                arr[j].deinit(alloc);
                j += 1;
            }
            alloc.free(arr);
        }
        for (item of a.arr_val) {
            arr[filled] = try item.clone(alloc);
            filled += 1;
        }
        for (item of b.arr_val) {
            arr[filled] = try item.clone(alloc);
            filled += 1;
        }
        return JqValue.arrayVal(arr);
    }
    if (a.tag == .Object and b.tag == .Object) {
        let entries = std.ArrayList(JqObjEntry).empty;
        errdefer {
            for (*en of entries.items) {
                alloc.free(en.key);
                en.value.deinit(alloc);
            }
            entries.deinit(alloc);
        }
        for (en of a.obj_val) {
            const k = alloc.dupe(u8, en.key) catch return JqError.OutOfMemory;
            errdefer alloc.free(k);
            const v = try en.value.clone(alloc);
            entries.append(alloc, .{ .key = k, .value = v }) catch {
                let vv = v;
                vv.deinit(alloc);
                return JqError.OutOfMemory;
            };
        }
        for (ben of b.obj_val) {
            let replaced = false;
            for (*ex of entries.items) {
                if (std.mem.eql(u8, ex.key, ben.key)) {
                    ex.value.deinit(alloc);
                    ex.value = try ben.value.clone(alloc);
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                const k = alloc.dupe(u8, ben.key) catch return JqError.OutOfMemory;
                errdefer alloc.free(k);
                const v = try ben.value.clone(alloc);
                entries.append(alloc, .{ .key = k, .value = v }) catch {
                    let vv = v;
                    vv.deinit(alloc);
                    return JqError.OutOfMemory;
                };
            }
        }
        return JqValue.objectVal(entries.toOwnedSlice(alloc) catch return JqError.OutOfMemory);
    }
    return JqError.TypeError;
}

fn subValues(alloc: std.mem.Allocator, a: JqValue, b: JqValue): JqError!JqValue {
    if (a.tag == .Int and b.tag == .Int) {
        return JqValue.intVal(a.int_val -% b.int_val);
    }
    if ((a.tag == .Int or a.tag == .Float) and (b.tag == .Int or b.tag == .Float)) {
        const fa = a.asNumber() orelse return JqError.TypeError;
        const fb = b.asNumber() orelse return JqError.TypeError;
        return JqValue.floatVal(fa - fb);
    }
    if (a.tag == .Array and b.tag == .Array) {
        let res = std.ArrayList(JqValue).empty;
        errdefer {
            for (*it of res.items) it.deinit(alloc);
            res.deinit(alloc);
        }
        for (item of a.arr_val) {
            let in_b = false;
            for (bitem of b.arr_val) {
                if (item.equals(bitem)) {
                    in_b = true;
                    break;
                }
            }
            if (!in_b) {
                const c = try item.clone(alloc);
                res.append(alloc, c) catch {
                    let vc = c;
                    vc.deinit(alloc);
                    return JqError.OutOfMemory;
                };
            }
        }
        return JqValue.arrayVal(res.toOwnedSlice(alloc) catch return JqError.OutOfMemory);
    }
    return JqError.TypeError;
}

fn mulValues(alloc: std.mem.Allocator, a: JqValue, b: JqValue): JqError!JqValue {
    if (a.tag == .Int and b.tag == .Int) {
        return JqValue.intVal(a.int_val *% b.int_val);
    }
    if ((a.tag == .Int or a.tag == .Float) and (b.tag == .Int or b.tag == .Float)) {
        const fa = a.asNumber() orelse return JqError.TypeError;
        const fb = b.asNumber() orelse return JqError.TypeError;
        return JqValue.floatVal(fa * fb);
    }
    if (a.tag == .String and b.tag == .Int) {
        if (b.int_val <= 0) return JqValue.nullVal();
        const count: usize = @as(usize, @intCast(b.int_val));
        if (count > 1024 * 1024) return JqError.OutputLimitExceeded;
        let buf = alloc.alloc(u8, a.str_val.len * count) catch return JqError.OutOfMemory;
        let i: usize = 0;
        while (i < count) {
            @memcpy(buf[i * a.str_val.len .. (i + 1) * a.str_val.len], a.str_val);
            i += 1;
        }
        return .{ .tag = .String, .str_val = buf };
    }
    return JqError.TypeError;
}

fn divValues(a: JqValue, b: JqValue): JqError!JqValue {
    if ((a.tag == .Int or a.tag == .Float) and (b.tag == .Int or b.tag == .Float)) {
        const fa = a.asNumber() orelse return JqError.TypeError;
        const fb = b.asNumber() orelse return JqError.TypeError;
        if (fb == 0.0) return JqError.DivisionByZero;
        return JqValue.floatVal(fa / fb);
    }
    return JqError.TypeError;
}

fn sortValuesSlice(items: []JqValue) {
    // Simple insertion/selection sort (no external closure needed)
    let i: usize = 1;
    while (i < items.len) {
        let j = i;
        while (j > 0 and items[j - 1].compare(items[j]) > 0) {
            const tmp = items[j - 1];
            items[j - 1] = items[j];
            items[j] = tmp;
            j -= 1;
        }
        i += 1;
    }
}

fn sortStringSlice(items: [][]const u8) {
    let i: usize = 1;
    while (i < items.len) {
        let j = i;
        while (j > 0 and std.mem.order(u8, items[j - 1], items[j]) == .gt) {
            const tmp = items[j - 1];
            items[j - 1] = items[j];
            items[j] = tmp;
            j -= 1;
        }
        i += 1;
    }
}

fn evalNode(alloc: std.mem.Allocator, expr: *const JqExpr, node_id: u32, input: JqValue, out: *std.ArrayList(JqValue), depth: usize): JqError!void {
    if (depth >= MAX_JQ_EVAL_DEPTH) return JqError.MaxDepthExceeded;
    const node = expr.nodes[node_id];

    switch (node.kind) {
        .Identity => {
            try emitVal(alloc, out, try input.clone(alloc));
        },
        .Literal => {
            const val: JqValue = switch (node.lit_tag) {
                .Null => JqValue.nullVal(),
                .Bool => JqValue.boolVal(node.lit_bool),
                .Int => JqValue.intVal(node.lit_int),
                .Float => JqValue.floatVal(node.lit_float),
                .String => try JqValue.stringVal(alloc, expr.strings[node.str_idx]),
                else => JqValue.nullVal(),
            };
            try emitVal(alloc, out, val);
        },
        .Field => {
            const key = expr.strings[node.str_idx];
            if (input.tag == .Null) {
                try emitVal(alloc, out, JqValue.nullVal());
            } else if (input.tag == .Object) {
                let found: JqValue? = null;
                for (entry of input.obj_val) {
                    if (std.mem.eql(u8, entry.key, key)) {
                        found = entry.value;
                        break;
                    }
                }
                if (fv := found) {
                    try emitVal(alloc, out, try fv.clone(alloc));
                } else {
                    try emitVal(alloc, out, JqValue.nullVal());
                }
            } else {
                if (node.optional) {
                    try emitVal(alloc, out, JqValue.nullVal());
                } else {
                    return JqError.TypeError;
                }
            }
        },
        .Index => {
            if (input.tag == .Null) {
                try emitVal(alloc, out, JqValue.nullVal());
            } else if (input.tag == .Array) {
                const len_i: i64 = @as(i64, @intCast(input.arr_val.len));
                let idx = node.lit_int;
                if (idx < 0) idx += len_i;
                if (idx < 0 or idx >= len_i) {
                    try emitVal(alloc, out, JqValue.nullVal());
                } else {
                    try emitVal(alloc, out, try input.arr_val[@as(usize, @intCast(idx))].clone(alloc));
                }
            } else {
                if (node.optional) {
                    try emitVal(alloc, out, JqValue.nullVal());
                } else {
                    return JqError.TypeError;
                }
            }
        },
        .Slice => {
            if (input.tag == .Null) {
                try emitVal(alloc, out, JqValue.nullVal());
            } else if (input.tag == .Array) {
                const len_i: i64 = @as(i64, @intCast(input.arr_val.len));
                let s: i64 = if (node.has_start) node.slice_start else 0;
                let e: i64 = if (node.has_end) node.slice_end else len_i;
                if (s < 0) s += len_i;
                if (e < 0) e += len_i;
                if (s < 0) s = 0;
                if (e > len_i) e = len_i;
                if (s >= e) {
                    const empty_arr = alloc.alloc(JqValue, 0) catch return JqError.OutOfMemory;
                    try emitVal(alloc, out, JqValue.arrayVal(empty_arr));
                } else {
                    const su: usize = @as(usize, @intCast(s));
                    const eu: usize = @as(usize, @intCast(e));
                    const sub_len = eu - su;
                    let arr = alloc.alloc(JqValue, sub_len) catch return JqError.OutOfMemory;
                    let filled: usize = 0;
                    errdefer {
                        let j: usize = 0;
                        while (j < filled) {
                            arr[j].deinit(alloc);
                            j += 1;
                        }
                        alloc.free(arr);
                    }
                    while (filled < sub_len) {
                        arr[filled] = try input.arr_val[su + filled].clone(alloc);
                        filled += 1;
                    }
                    try emitVal(alloc, out, JqValue.arrayVal(arr));
                }
            } else if (input.tag == .String) {
                const len_i: i64 = @as(i64, @intCast(input.str_val.len));
                let s: i64 = if (node.has_start) node.slice_start else 0;
                let e: i64 = if (node.has_end) node.slice_end else len_i;
                if (s < 0) s += len_i;
                if (e < 0) e += len_i;
                if (s < 0) s = 0;
                if (e > len_i) e = len_i;
                if (s >= e) {
                    try emitVal(alloc, out, try JqValue.stringVal(alloc, ""));
                } else {
                    const su: usize = @as(usize, @intCast(s));
                    const eu: usize = @as(usize, @intCast(e));
                    try emitVal(alloc, out, try JqValue.stringVal(alloc, input.str_val[su..eu]));
                }
            } else {
                if (node.optional) {
                    try emitVal(alloc, out, JqValue.nullVal());
                } else {
                    return JqError.TypeError;
                }
            }
        },
        .Iterate => {
            if (input.tag == .Array) {
                for (item of input.arr_val) {
                    try emitVal(alloc, out, try item.clone(alloc));
                }
            } else if (input.tag == .Object) {
                for (entry of input.obj_val) {
                    try emitVal(alloc, out, try entry.value.clone(alloc));
                }
            } else {
                if (!node.optional) return JqError.TypeError;
            }
        },
        .Pipe => {
            let mid = std.ArrayList(JqValue).empty;
            defer {
                for (*m of mid.items) m.deinit(alloc);
                mid.deinit(alloc);
            }
            try evalNode(alloc, expr, node.lhs, input, &mid, depth + 1);
            for (mval of mid.items) {
                try evalNode(alloc, expr, node.rhs, mval, out, depth + 1);
            }
        },
        .Comma => {
            try evalNode(alloc, expr, node.lhs, input, out, depth + 1);
            try evalNode(alloc, expr, node.rhs, input, out, depth + 1);
        },
        .ArrayConstruct => {
            if (node.lhs == INVALID_NODE) {
                const empty_arr = alloc.alloc(JqValue, 0) catch return JqError.OutOfMemory;
                try emitVal(alloc, out, JqValue.arrayVal(empty_arr));
            } else {
                let items = std.ArrayList(JqValue).empty;
                errdefer {
                    for (*it of items.items) it.deinit(alloc);
                    items.deinit(alloc);
                }
                try evalNode(alloc, expr, node.lhs, input, &items, depth + 1);
                const slice = items.toOwnedSlice(alloc) catch return JqError.OutOfMemory;
                try emitVal(alloc, out, JqValue.arrayVal(slice));
            }
        },
        .ObjectConstruct => {
            const fstart: usize = @as(usize, node.field_start);
            const flen: usize = @as(usize, node.field_len);
            if (flen == 0) {
                const empty_obj = alloc.alloc(JqObjEntry, 0) catch return JqError.OutOfMemory;
                try emitVal(alloc, out, JqValue.objectVal(empty_obj));
                return;
            }
            // Evaluate each field (using first output for simplicity, or cartesian? single output per field is standard for {k:v} when deterministic)
            let entries = std.ArrayList(JqObjEntry).empty;
            errdefer {
                for (*en of entries.items) {
                    alloc.free(en.key);
                    en.value.deinit(alloc);
                }
                entries.deinit(alloc);
            }
            let fi: usize = 0;
            while (fi < flen) {
                const f = expr.obj_fields[fstart + fi];
                let key_str: []const u8 = "";
                let free_key = false;
                if (f.key_node != INVALID_NODE) {
                    let kout = std.ArrayList(JqValue).empty;
                    defer {
                        for (*kv of kout.items) kv.deinit(alloc);
                        kout.deinit(alloc);
                    }
                    try evalNode(alloc, expr, f.key_node, input, &kout, depth + 1);
                    if (kout.items.len == 0 or kout.items[0].tag != .String) return JqError.TypeError;
                    key_str = alloc.dupe(u8, kout.items[0].str_val) catch return JqError.OutOfMemory;
                    free_key = true;
                } else {
                    key_str = alloc.dupe(u8, expr.strings[f.key_str_idx]) catch return JqError.OutOfMemory;
                    free_key = true;
                }
                errdefer if (free_key) alloc.free(key_str);

                let vout = std.ArrayList(JqValue).empty;
                defer {
                    for (*vv of vout.items) vv.deinit(alloc);
                    vout.deinit(alloc);
                }
                try evalNode(alloc, expr, f.val_node, input, &vout, depth + 1);
                if (vout.items.len > 0) {
                    const vclone = try vout.items[0].clone(alloc);
                    entries.append(alloc, .{ .key = key_str, .value = vclone }) catch {
                        let vc = vclone;
                        vc.deinit(alloc);
                        return JqError.OutOfMemory;
                    };
                } else {
                    if (free_key) alloc.free(key_str);
                }
                fi += 1;
            }
            const slice = entries.toOwnedSlice(alloc) catch return JqError.OutOfMemory;
            try emitVal(alloc, out, JqValue.objectVal(slice));
        },
        .Neg => {
            let sub = std.ArrayList(JqValue).empty;
            defer {
                for (*sv of sub.items) sv.deinit(alloc);
                sub.deinit(alloc);
            }
            try evalNode(alloc, expr, node.lhs, input, &sub, depth + 1);
            for (v of sub.items) {
                if (v.tag == .Int) {
                    try emitVal(alloc, out, JqValue.intVal(0 -% v.int_val));
                } else if (v.tag == .Float) {
                    try emitVal(alloc, out, JqValue.floatVal(-v.float_val));
                } else {
                    return JqError.TypeError;
                }
            }
        },
        .BinOp => {
            if (node.bin_op == .Alt) {
                let lout = std.ArrayList(JqValue).empty;
                defer {
                    for (*lv of lout.items) lv.deinit(alloc);
                    lout.deinit(alloc);
                }
                try evalNode(alloc, expr, node.lhs, input, &lout, depth + 1);
                let emitted_any = false;
                for (lv of lout.items) {
                    if (lv.isTruthy()) {
                        try emitVal(alloc, out, try lv.clone(alloc));
                        emitted_any = true;
                    }
                }
                if (!emitted_any) {
                    try evalNode(alloc, expr, node.rhs, input, out, depth + 1);
                }
                return;
            }

            if (node.bin_op == .And or node.bin_op == .Or) {
                let lout = std.ArrayList(JqValue).empty;
                defer {
                    for (*lv of lout.items) lv.deinit(alloc);
                    lout.deinit(alloc);
                }
                try evalNode(alloc, expr, node.lhs, input, &lout, depth + 1);
                for (lv of lout.items) {
                    if (node.bin_op == .And) {
                        if (!lv.isTruthy()) {
                            try emitVal(alloc, out, JqValue.boolVal(false));
                        } else {
                            let rout = std.ArrayList(JqValue).empty;
                            defer {
                                for (*rv of rout.items) rv.deinit(alloc);
                                rout.deinit(alloc);
                            }
                            try evalNode(alloc, expr, node.rhs, input, &rout, depth + 1);
                            for (rv of rout.items) {
                                try emitVal(alloc, out, JqValue.boolVal(rv.isTruthy()));
                            }
                        }
                    } else {
                        if (lv.isTruthy()) {
                            try emitVal(alloc, out, JqValue.boolVal(true));
                        } else {
                            let rout = std.ArrayList(JqValue).empty;
                            defer {
                                for (*rv of rout.items) rv.deinit(alloc);
                                rout.deinit(alloc);
                            }
                            try evalNode(alloc, expr, node.rhs, input, &rout, depth + 1);
                            for (rv of rout.items) {
                                try emitVal(alloc, out, JqValue.boolVal(rv.isTruthy()));
                            }
                        }
                    }
                }
                return;
            }

            let lout = std.ArrayList(JqValue).empty;
            defer {
                for (*lv of lout.items) lv.deinit(alloc);
                lout.deinit(alloc);
            }
            let rout = std.ArrayList(JqValue).empty;
            defer {
                for (*rv of rout.items) rv.deinit(alloc);
                rout.deinit(alloc);
            }
            try evalNode(alloc, expr, node.lhs, input, &lout, depth + 1);
            try evalNode(alloc, expr, node.rhs, input, &rout, depth + 1);

            for (lv of lout.items) {
                for (rv of rout.items) {
                    const res: JqValue = switch (node.bin_op) {
                        .Eq => JqValue.boolVal(lv.equals(rv)),
                        .Neq => JqValue.boolVal(!lv.equals(rv)),
                        .Lt => JqValue.boolVal(lv.compare(rv) < 0),
                        .Le => JqValue.boolVal(lv.compare(rv) <= 0),
                        .Gt => JqValue.boolVal(lv.compare(rv) > 0),
                        .Ge => JqValue.boolVal(lv.compare(rv) >= 0),
                        .Add => try addValues(alloc, lv, rv),
                        .Sub => try subValues(alloc, lv, rv),
                        .Mul => try mulValues(alloc, lv, rv),
                        .Div => try divValues(lv, rv),
                        else => JqValue.nullVal(),
                    };
                    try emitVal(alloc, out, res);
                }
            }
        },
        .Call => {
            switch (node.builtin) {
                .Empty => {},
                .Not => {
                    try emitVal(alloc, out, JqValue.boolVal(!input.isTruthy()));
                },
                .Type => {
                    try emitVal(alloc, out, try JqValue.stringVal(alloc, input.typeName()));
                },
                .Values => {
                    if (input.tag != .Null) {
                        try emitVal(alloc, out, try input.clone(alloc));
                    }
                },
                .Length => {
                    const n: i64 = switch (input.tag) {
                        .Null => 0,
                        .String => @as(i64, @intCast(input.str_val.len)),
                        .Array => @as(i64, @intCast(input.arr_val.len)),
                        .Object => @as(i64, @intCast(input.obj_val.len)),
                        .Int => if (input.int_val < 0) -input.int_val else input.int_val,
                        else => return JqError.TypeError,
                    };
                    try emitVal(alloc, out, JqValue.intVal(n));
                },
                .Keys => {
                    if (input.tag == .Object) {
                        let keys_tmp = alloc.alloc(string, input.obj_val.len) catch return JqError.OutOfMemory;
                        defer alloc.free(keys_tmp);
                        let ki: usize = 0;
                        for (en of input.obj_val) {
                            keys_tmp[ki] = en.key;
                            ki += 1;
                        }
                        sortStringSlice(keys_tmp);

                        let arr = alloc.alloc(JqValue, keys_tmp.len) catch return JqError.OutOfMemory;
                        let filled: usize = 0;
                        errdefer {
                            let j: usize = 0;
                            while (j < filled) {
                                arr[j].deinit(alloc);
                                j += 1;
                            }
                            alloc.free(arr);
                        }
                        while (filled < keys_tmp.len) {
                            arr[filled] = try JqValue.stringVal(alloc, keys_tmp[filled]);
                            filled += 1;
                        }
                        try emitVal(alloc, out, JqValue.arrayVal(arr));
                    } else if (input.tag == .Array) {
                        let arr = alloc.alloc(JqValue, input.arr_val.len) catch return JqError.OutOfMemory;
                        let i: usize = 0;
                        while (i < input.arr_val.len) {
                            arr[i] = JqValue.intVal(@as(i64, @intCast(i)));
                            i += 1;
                        }
                        try emitVal(alloc, out, JqValue.arrayVal(arr));
                    } else {
                        return JqError.TypeError;
                    }
                },
                .Reverse => {
                    if (input.tag == .Array) {
                        const n = input.arr_val.len;
                        let arr = alloc.alloc(JqValue, n) catch return JqError.OutOfMemory;
                        let filled: usize = 0;
                        errdefer {
                            let j: usize = 0;
                            while (j < filled) {
                                arr[j].deinit(alloc);
                                j += 1;
                            }
                            alloc.free(arr);
                        }
                        while (filled < n) {
                            arr[filled] = try input.arr_val[n - 1 - filled].clone(alloc);
                            filled += 1;
                        }
                        try emitVal(alloc, out, JqValue.arrayVal(arr));
                    } else if (input.tag == .String) {
                        const n = input.str_val.len;
                        let buf = alloc.alloc(u8, n) catch return JqError.OutOfMemory;
                        let i: usize = 0;
                        while (i < n) {
                            buf[i] = input.str_val[n - 1 - i];
                            i += 1;
                        }
                        try emitVal(alloc, out, .{ .tag = .String, .str_val = buf });
                    } else {
                        return JqError.TypeError;
                    }
                },
                .Sort => {
                    if (input.tag != .Array) return JqError.TypeError;
                    let cloned = try input.clone(alloc);
                    errdefer cloned.deinit(alloc);
                    sortValuesSlice(cloned.arr_val);
                    try emitVal(alloc, out, cloned);
                },
                .Unique => {
                    if (input.tag != .Array) return JqError.TypeError;
                    let cloned = try input.clone(alloc);
                    defer cloned.deinit(alloc);
                    sortValuesSlice(cloned.arr_val);

                    let uniq = std.ArrayList(JqValue).empty;
                    errdefer {
                        for (*uv of uniq.items) uv.deinit(alloc);
                        uniq.deinit(alloc);
                    }
                    for (item of cloned.arr_val) {
                        if (uniq.items.len == 0 or !uniq.items[uniq.items.len - 1].equals(item)) {
                            const c = try item.clone(alloc);
                            uniq.append(alloc, c) catch {
                                let vc = c;
                                vc.deinit(alloc);
                                return JqError.OutOfMemory;
                            };
                        }
                    }
                    try emitVal(alloc, out, JqValue.arrayVal(uniq.toOwnedSlice(alloc) catch return JqError.OutOfMemory));
                },
                .Select => {
                    let cond_out = std.ArrayList(JqValue).empty;
                    defer {
                        for (*cv of cond_out.items) cv.deinit(alloc);
                        cond_out.deinit(alloc);
                    }
                    try evalNode(alloc, expr, node.lhs, input, &cond_out, depth + 1);
                    for (cv of cond_out.items) {
                        if (cv.isTruthy()) {
                            try emitVal(alloc, out, try input.clone(alloc));
                        }
                    }
                },
                .Map => {
                    if (input.tag != .Array) return JqError.TypeError;
                    let mapped = std.ArrayList(JqValue).empty;
                    errdefer {
                        for (*mv of mapped.items) mv.deinit(alloc);
                        mapped.deinit(alloc);
                    }
                    for (item of input.arr_val) {
                        try evalNode(alloc, expr, node.lhs, item, &mapped, depth + 1);
                    }
                    try emitVal(alloc, out, JqValue.arrayVal(mapped.toOwnedSlice(alloc) catch return JqError.OutOfMemory));
                },
                .Has => {
                    let arg_out = std.ArrayList(JqValue).empty;
                    defer {
                        for (*av of arg_out.items) av.deinit(alloc);
                        arg_out.deinit(alloc);
                    }
                    try evalNode(alloc, expr, node.lhs, input, &arg_out, depth + 1);
                    for (av of arg_out.items) {
                        if (input.tag == .Object) {
                            if (av.tag != .String) return JqError.TypeError;
                            let found = false;
                            for (en of input.obj_val) {
                                if (std.mem.eql(u8, en.key, av.str_val)) {
                                    found = true;
                                    break;
                                }
                            }
                            try emitVal(alloc, out, JqValue.boolVal(found));
                        } else if (input.tag == .Array) {
                            if (av.tag != .Int) return JqError.TypeError;
                            const ok = av.int_val >= 0 and @as(usize, @intCast(av.int_val)) < input.arr_val.len;
                            try emitVal(alloc, out, JqValue.boolVal(ok));
                        } else if (input.tag == .Null) {
                            try emitVal(alloc, out, JqValue.boolVal(false));
                        } else {
                            return JqError.TypeError;
                        }
                    }
                },
                .Contains => {
                    let arg_out = std.ArrayList(JqValue).empty;
                    defer {
                        for (*av of arg_out.items) av.deinit(alloc);
                        arg_out.deinit(alloc);
                    }
                    try evalNode(alloc, expr, node.lhs, input, &arg_out, depth + 1);
                    for (av of arg_out.items) {
                        try emitVal(alloc, out, JqValue.boolVal(containsValue(input, av)));
                    }
                },
            }
        },
    }
}

pub fn runJq(alloc: std.mem.Allocator, expr: *const JqExpr, input: JqValue): JqError![]JqValue {
    let out = std.ArrayList(JqValue).empty;
    errdefer {
        for (*ov of out.items) ov.deinit(alloc);
        out.deinit(alloc);
    }
    try evalNode(alloc, expr, expr.root, input, &out, 0);
    return out.toOwnedSlice(alloc) catch return JqError.OutOfMemory;
}

pub fn runJqJson(alloc: std.mem.Allocator, filter: []const u8, json_input: []const u8): JqError![]u8 {
    let val = try parseJson(alloc, json_input);
    defer val.deinit(alloc);

    let expr = try compileJq(alloc, filter);
    defer expr.deinit(alloc);

    let results = try runJq(alloc, &expr, val);
    defer freeValues(alloc, &results);

    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);

    let i: usize = 0;
    for (rv of results) {
        if (i > 0) buf.append(alloc, '\n') catch return JqError.OutOfMemory;
        const s = try stringifyJson(alloc, rv);
        defer alloc.free(s);
        buf.appendSlice(alloc, s) catch return JqError.OutOfMemory;
        i += 1;
    }
    return buf.toOwnedSlice(alloc) catch return JqError.OutOfMemory;
}
