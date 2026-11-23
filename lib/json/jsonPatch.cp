// Source: native-cp-stdlib (RFC 6902 JSON Patch & RFC 6901 Pointer)
import std from "std";

type StrSlice = []const u8;

export enum PatchOpKind {
    Add,
    Remove,
    Replace,
    Move,
    Copy,
    Test,
}

export struct JsonPatchOp {
    op: PatchOpKind,
    path: []const u8,
    from: StrSlice? = null,
    value: std.json.Value? = null,
}

export error JsonPatchError {
    InvalidPointer,
    PointerNotFound,
    IndexOutOfBounds,
    InvalidArrayIndex,
    MissingFrom,
    MissingValue,
    TestFailed,
    CannotMoveToChild,
    InvalidPatch,
    InvalidJson,
    TypeMismatch,
}

fn makeJsonNull(): std.json.Value {
    return @unionInit(std.json.Value, "null", {});
}

fn isJsonNull(val: std.json.Value): bool {
    switch (val) {
        .bool, .integer, .float, .number_string, .string, .array, .object => return false,
        else => return true,
    }
}

export fn freeValue(alloc: std.mem.Allocator, val: *std.json.Value) {
    switch (val.*) {
        .bool, .integer, .float => {},
        .string => |s| {
            alloc.free(s);
        },
        .number_string => |s| {
            alloc.free(s);
        },
        .array => |*arr| {
            let i: usize = 0;
            while (i < arr.items.len) {
                freeValue(alloc, &arr.items[i]);
                i = i + 1;
            }
            arr.deinit();
        },
        .object => |*obj| {
            let it = obj.iterator();
            while (entry := it.next()) {
                alloc.free(entry.key_ptr.*);
                freeValue(alloc, entry.value_ptr);
            }
            obj.deinit();
        },
        else => {},
    }
    val.* = makeJsonNull();
}

export fn cloneValue(alloc: std.mem.Allocator, val: std.json.Value): !std.json.Value {
    switch (val) {
        .bool => |b| return .{ .bool = b },
        .integer => |i| return .{ .integer = i },
        .float => |f| return .{ .float = f },
        .string => |s| {
            const copy = try alloc.dupe(u8, s);
            return .{ .string = copy };
        },
        .number_string => |s| {
            const copy = try alloc.dupe(u8, s);
            return .{ .number_string = copy };
        },
        .array => |arr| {
            let new_arr = std.json.Array.init(alloc);
            errdefer {
                let i: usize = 0;
                while (i < new_arr.items.len) {
                    freeValue(alloc, &new_arr.items[i]);
                    i = i + 1;
                }
                new_arr.deinit();
            }
            for (item of arr.items) {
                const cloned = try cloneValue(alloc, item);
                try new_arr.append(cloned);
            }
            return .{ .array = new_arr };
        },
        .object => |obj| {
            let new_obj = std.json.ObjectMap.init(alloc);
            errdefer {
                let it_err = new_obj.iterator();
                while (entry := it_err.next()) {
                    alloc.free(entry.key_ptr.*);
                    freeValue(alloc, entry.value_ptr);
                }
                new_obj.deinit();
            }
            let it = obj.iterator();
            while (entry := it.next()) {
                const k = try alloc.dupe(u8, entry.key_ptr.*);
                errdefer alloc.free(k);
                let v = try cloneValue(alloc, entry.value_ptr.*);
                errdefer freeValue(alloc, &v);
                try new_obj.put(k, v);
            }
            return .{ .object = new_obj };
        },
        else => return makeJsonNull(),
    }
}

export fn valuesEqual(a: std.json.Value, b: std.json.Value): bool {
    switch (a) {
        .bool => |ba| {
            switch (b) {
                .bool => |bb| return ba == bb,
                else => return false,
            }
        },
        .integer => |ia| {
            switch (b) {
                .integer => |ib| return ia == ib,
                .float => |fb| return @as(f64, @floatFromInt(ia)) == fb,
                else => return false,
            }
        },
        .float => |fa| {
            switch (b) {
                .float => |fb| return fa == fb,
                .integer => |ib| return fa == @as(f64, @floatFromInt(ib)),
                else => return false,
            }
        },
        .number_string => |sa| {
            switch (b) {
                .number_string => |sb| return std.mem.eql(u8, sa, sb),
                else => return false,
            }
        },
        .string => |sa| {
            switch (b) {
                .string => |sb| return std.mem.eql(u8, sa, sb),
                else => return false,
            }
        },
        .array => |aa| {
            switch (b) {
                .array => |ab| {
                    if (aa.items.len != ab.items.len) return false;
                    let i: usize = 0;
                    while (i < aa.items.len) {
                        if (!valuesEqual(aa.items[i], ab.items[i])) return false;
                        i = i +% 1;
                    }
                    return true;
                },
                else => return false,
            }
        },
        .object => |oa| {
            switch (b) {
                .object => |ob| {
                    if (oa.count() != ob.count()) return false;
                    let it = oa.iterator();
                    while (entry := it.next()) {
                        const other_val = ob.get(entry.key_ptr.*) orelse return false;
                        if (!valuesEqual(entry.value_ptr.*, other_val)) return false;
                    }
                    return true;
                },
                else => return false,
            }
        },
        else => return isJsonNull(b),
    }
}

fn unescapePointerToken(alloc: std.mem.Allocator, token: []const u8): ![]const u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);
    let i: usize = 0;
    while (i < token.len) {
        const ch = token[i];
        if (ch == '~') {
            if (i + 1 >= token.len) {
                return error.InvalidPointer;
            }
            const next_ch = token[i + 1];
            if (next_ch == '0') {
                try buf.append(alloc, '~');
            } else if (next_ch == '1') {
                try buf.append(alloc, '/');
            } else {
                return error.InvalidPointer;
            }
            i = i + 2;
        } else {
            try buf.append(alloc, ch);
            i = i + 1;
        }
    }
    return try buf.toOwnedSlice(alloc);
}

fn escapePointerToken(alloc: std.mem.Allocator, token: []const u8): ![]const u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);
    for (ch of token) {
        if (ch == '~') {
            try buf.appendSlice(alloc, "~0");
        } else if (ch == '/') {
            try buf.appendSlice(alloc, "~1");
        } else {
            try buf.append(alloc, ch);
        }
    }
    return try buf.toOwnedSlice(alloc);
}

fn parsePointerTokens(alloc: std.mem.Allocator, pointer: []const u8): ![]StrSlice {
    if (pointer.len == 0) {
        return try alloc.alloc(StrSlice, 0);
    }
    if (pointer[0] != '/') {
        return error.InvalidPointer;
    }
    let tokens = std.ArrayList(StrSlice).empty;
    errdefer {
        for (t of tokens.items) {
            alloc.free(t);
        }
        tokens.deinit(alloc);
    }
    let start: usize = 1;
    let i: usize = 1;
    while (i <= pointer.len) {
        if (i == pointer.len || pointer[i] == '/') {
            const raw = pointer[start..i];
            const decoded = try unescapePointerToken(alloc, raw);
            errdefer alloc.free(decoded);
            try tokens.append(alloc, decoded);
            start = i + 1;
        }
        i = i + 1;
    }
    return try tokens.toOwnedSlice(alloc);
}

fn freePointerTokens(alloc: std.mem.Allocator, tokens: []StrSlice) {
    for (t of tokens) {
        alloc.free(t);
    }
    alloc.free(tokens);
}

fn parseArrayIndex(token: []const u8): !usize {
    if (token.len == 0) return error.InvalidArrayIndex;
    if (token.len > 1 && token[0] == '0') return error.InvalidArrayIndex;
    let val: usize = 0;
    for (ch of token) {
        if (ch < '0' || ch > '9') return error.InvalidArrayIndex;
        const digit: usize = @intCast(ch - '0');
        val = val * 10 + digit;
    }
    return val;
}

export fn resolvePointer(root: std.json.Value, pointer: []const u8): !std.json.Value {
    if (pointer.len == 0) return root;
    if (pointer[0] != '/') return error.InvalidPointer;

    let current = root;
    let start: usize = 1;
    let i: usize = 1;
    while (i <= pointer.len) {
        if (i == pointer.len || pointer[i] == '/') {
            const raw = pointer[start..i];
            switch (current) {
                .object => |obj| {
                    if (std.mem.indexOfScalar(u8, raw, '~') == null) {
                        if (next_v := obj.get(raw)) {
                            current = next_v;
                        } else {
                            return error.PointerNotFound;
                        }
                    } else {
                        let stack_buf: [512]u8 = undefined;
                        if (raw.len <= 512) {
                            let len: usize = 0;
                            let j: usize = 0;
                            while (j < raw.len) {
                                if (raw[j] == '~') {
                                    if (j + 1 >= raw.len) return error.InvalidPointer;
                                    if (raw[j + 1] == '0') {
                                        stack_buf[len] = '~';
                                    } else if (raw[j + 1] == '1') {
                                        stack_buf[len] = '/';
                                    } else {
                                        return error.InvalidPointer;
                                    }
                                    len = len + 1;
                                    j = j + 2;
                                } else {
                                    stack_buf[len] = raw[j];
                                    len = len + 1;
                                    j = j + 1;
                                }
                            }
                            if (next_v := obj.get(stack_buf[0..len])) {
                                current = next_v;
                            } else {
                                return error.PointerNotFound;
                            }
                        } else {
                            return error.InvalidPointer;
                        }
                    }
                },
                .array => |arr| {
                    if (std.mem.eql(u8, raw, "-")) return error.IndexOutOfBounds;
                    const idx = try parseArrayIndex(raw);
                    if (idx >= arr.items.len) return error.IndexOutOfBounds;
                    current = arr.items[idx];
                },
                else => return error.PointerNotFound,
            }
            start = i + 1;
        }
        i = i + 1;
    }
    return current;
}

fn resolveParentMut(
    alloc: std.mem.Allocator,
    root: *std.json.Value,
    pointer: []const u8,
    last_token_out: *[]const u8
): !*std.json.Value {
    const tokens = try parsePointerTokens(alloc, pointer);
    defer freePointerTokens(alloc, tokens);
    if (tokens.len == 0) return error.InvalidPointer;

    let curr: *std.json.Value = root;
    let i: usize = 0;
    while (i + 1 < tokens.len) {
        const tok = tokens[i];
        switch (curr.*) {
            .object => |*obj| {
                if (next_ptr := obj.getPtr(tok)) {
                    curr = next_ptr;
                } else {
                    return error.PointerNotFound;
                }
            },
            .array => |*arr| {
                if (std.mem.eql(u8, tok, "-")) return error.IndexOutOfBounds;
                const idx = try parseArrayIndex(tok);
                if (idx >= arr.items.len) return error.IndexOutOfBounds;
                curr = &arr.items[idx];
            },
            else => return error.PointerNotFound,
        }
        i = i + 1;
    }
    last_token_out.* = try alloc.dupe(u8, tokens[tokens.len - 1]);
    return curr;
}

fn applyAddOwned(
    alloc: std.mem.Allocator,
    root: *std.json.Value,
    path: []const u8,
    owned_val_in: std.json.Value
): !void {
    let owned_val = owned_val_in;
    if (path.len == 0) {
        freeValue(alloc, root);
        root.* = owned_val;
        return;
    }
    let last_tok: []const u8 = "";
    const parent = resolveParentMut(alloc, root, path, &last_tok) catch |e| {
        freeValue(alloc, &owned_val);
        return e;
    };
    defer alloc.free(last_tok);

    switch (parent.*) {
        .object => |*obj| {
            if (old_val := obj.get(last_tok)) {
                let mv = old_val;
                freeValue(alloc, &mv);
                obj.put(last_tok, owned_val) catch |e| {
                    freeValue(alloc, &owned_val);
                    return e;
                };
            } else {
                const key_copy = alloc.dupe(u8, last_tok) catch |e| {
                    freeValue(alloc, &owned_val);
                    return e;
                };
                obj.put(key_copy, owned_val) catch |e| {
                    alloc.free(key_copy);
                    freeValue(alloc, &owned_val);
                    return e;
                };
            }
        },
        .array => |*arr| {
            if (std.mem.eql(u8, last_tok, "-")) {
                arr.append(owned_val) catch |e| {
                    freeValue(alloc, &owned_val);
                    return e;
                };
            } else {
                const idx = parseArrayIndex(last_tok) catch |e| {
                    freeValue(alloc, &owned_val);
                    return e;
                };
                if (idx > arr.items.len) {
                    freeValue(alloc, &owned_val);
                    return error.IndexOutOfBounds;
                }
                arr.insert(idx, owned_val) catch |e| {
                    freeValue(alloc, &owned_val);
                    return e;
                };
            }
        },
        else => {
            freeValue(alloc, &owned_val);
            return error.PointerNotFound;
        },
    }
}

fn applyRemoveTake(
    alloc: std.mem.Allocator,
    root: *std.json.Value,
    path: []const u8
): !std.json.Value {
    if (path.len == 0) return error.InvalidPointer;
    let last_tok: []const u8 = "";
    const parent = try resolveParentMut(alloc, root, path, &last_tok);
    defer alloc.free(last_tok);

    switch (parent.*) {
        .object => |*obj| {
            const removed = obj.fetchOrderedRemove(last_tok) orelse return error.PointerNotFound;
            alloc.free(removed.key);
            return removed.value;
        },
        .array => |*arr| {
            if (std.mem.eql(u8, last_tok, "-")) return error.IndexOutOfBounds;
            const idx = try parseArrayIndex(last_tok);
            if (idx >= arr.items.len) return error.IndexOutOfBounds;
            return arr.orderedRemove(idx);
        },
        else => return error.PointerNotFound,
    }
}

fn applyReplaceOwned(
    alloc: std.mem.Allocator,
    root: *std.json.Value,
    path: []const u8,
    owned_val_in: std.json.Value
): !void {
    let owned_val = owned_val_in;
    if (path.len == 0) {
        freeValue(alloc, root);
        root.* = owned_val;
        return;
    }
    let last_tok: []const u8 = "";
    const parent = resolveParentMut(alloc, root, path, &last_tok) catch |e| {
        freeValue(alloc, &owned_val);
        return e;
    };
    defer alloc.free(last_tok);

    switch (parent.*) {
        .object => |*obj| {
            if (old_val := obj.get(last_tok)) {
                let mv = old_val;
                freeValue(alloc, &mv);
                obj.put(last_tok, owned_val) catch |e| {
                    freeValue(alloc, &owned_val);
                    return e;
                };
            } else {
                freeValue(alloc, &owned_val);
                return error.PointerNotFound;
            }
        },
        .array => |*arr| {
            if (std.mem.eql(u8, last_tok, "-")) {
                freeValue(alloc, &owned_val);
                return error.IndexOutOfBounds;
            }
            const idx = parseArrayIndex(last_tok) catch |e| {
                freeValue(alloc, &owned_val);
                return e;
            };
            if (idx >= arr.items.len) {
                freeValue(alloc, &owned_val);
                return error.IndexOutOfBounds;
            }
            freeValue(alloc, &arr.items[idx]);
            arr.items[idx] = owned_val;
        },
        else => {
            freeValue(alloc, &owned_val);
            return error.PointerNotFound;
        },
    }
}

fn isProperPrefixPath(prefix: []const u8, target: []const u8): bool {
    if (prefix.len == 0) {
        return target.len > 0;
    }
    if (target.len <= prefix.len) return false;
    return std.mem.startsWith(u8, target, prefix) && target[prefix.len] == '/';
}

export fn applyPatch(
    alloc: std.mem.Allocator,
    doc: std.json.Value,
    ops: []const JsonPatchOp
): !std.json.Value {
    let working = try cloneValue(alloc, doc);
    errdefer freeValue(alloc, &working);

    for (op of ops) {
        switch (op.op) {
            .Add => {
                const v = op.value orelse return error.MissingValue;
                const cloned = try cloneValue(alloc, v);
                try applyAddOwned(alloc, &working, op.path, cloned);
            },
            .Remove => {
                let removed = try applyRemoveTake(alloc, &working, op.path);
                freeValue(alloc, &removed);
            },
            .Replace => {
                const v = op.value orelse return error.MissingValue;
                const cloned = try cloneValue(alloc, v);
                try applyReplaceOwned(alloc, &working, op.path, cloned);
            },
            .Move => {
                const from_ptr = op.from orelse return error.MissingFrom;
                if (isProperPrefixPath(from_ptr, op.path)) {
                    return error.CannotMoveToChild;
                }
                if (std.mem.eql(u8, from_ptr, op.path)) {
                    * = try resolvePointer(working, from_ptr);
                } else {
                    const taken = try applyRemoveTake(alloc, &working, from_ptr);
                    try applyAddOwned(alloc, &working, op.path, taken);
                }
            },
            .Copy => {
                const from_ptr = op.from orelse return error.MissingFrom;
                const found = try resolvePointer(working, from_ptr);
                const cloned = try cloneValue(alloc, found);
                try applyAddOwned(alloc, &working, op.path, cloned);
            },
            .Test => {
                const expected = op.value orelse return error.MissingValue;
                const actual = try resolvePointer(working, op.path);
                if (!valuesEqual(actual, expected)) {
                    return error.TestFailed;
                }
            },
        }
    }

    return working;
}

export fn mergePatch(
    alloc: std.mem.Allocator,
    target: std.json.Value,
    patch: std.json.Value
): !std.json.Value {
    switch (patch) {
        .object => |patch_obj| {
            let res_obj: std.json.ObjectMap = undefined;
            switch (target) {
                .object => {
                    const cloned_target = try cloneValue(alloc, target);
                    res_obj = cloned_target.object;
                },
                else => {
                    res_obj = std.json.ObjectMap.init(alloc);
                },
            }
            let res_val: std.json.Value = .{ .object = res_obj };
            errdefer freeValue(alloc, &res_val);

            let it = patch_obj.iterator();
            while (entry := it.next()) {
                const key = entry.key_ptr.*;
                const patch_v = entry.value_ptr.*;
                if (isJsonNull(patch_v)) {
                    if (removed := res_val.object.fetchOrderedRemove(key)) {
                        alloc.free(removed.key);
                        let rv = removed.value;
                        freeValue(alloc, &rv);
                    }
                } else {
                    if (existing_val := res_val.object.get(key)) {
                        const merged = try mergePatch(alloc, existing_val, patch_v);
                        let mv = existing_val;
                        freeValue(alloc, &mv);
                        try res_val.object.put(key, merged);
                    } else {
                        const merged = try mergePatch(alloc, makeJsonNull(), patch_v);
                        errdefer {
                            let mv = merged;
                            freeValue(alloc, &mv);
                        }
                        const key_copy = try alloc.dupe(u8, key);
                        errdefer alloc.free(key_copy);
                        try res_val.object.put(key_copy, merged);
                    }
                }
            }
            return res_val;
        },
        else => return try cloneValue(alloc, patch),
    }
}

fn appendEscapedJsonString(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), s: []const u8): !void {
    try buf.append(alloc, '"');
    for (ch of s) {
        if (ch == '"') {
            try buf.appendSlice(alloc, "\\\"");
        } else if (ch == '\\') {
            try buf.appendSlice(alloc, "\\\\");
        } else if (ch == '\n') {
            try buf.appendSlice(alloc, "\\n");
        } else if (ch == '\r') {
            try buf.appendSlice(alloc, "\\r");
        } else if (ch == '\t') {
            try buf.appendSlice(alloc, "\\t");
        } else if (ch == 0x08) {
            try buf.appendSlice(alloc, "\\b");
        } else if (ch == 0x0c) {
            try buf.appendSlice(alloc, "\\f");
        } else if (ch < 0x20) {
            const hex = "0123456789abcdef";
            try buf.appendSlice(alloc, "\\u00");
            const hi: usize = @intCast((ch >> 4) & 0x0f);
            const lo: usize = @intCast(ch & 0x0f);
            try buf.append(alloc, hex[hi]);
            try buf.append(alloc, hex[lo]);
        } else {
            try buf.append(alloc, ch);
        }
    }
    try buf.append(alloc, '"');
}

fn stringifyInto(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), val: std.json.Value): !void {
    switch (val) {
        .bool => |b| {
            if (b) {
                try buf.appendSlice(alloc, "true");
            } else {
                try buf.appendSlice(alloc, "false");
            }
        },
        .integer => |i| {
            const formatted = try std.fmt.allocPrint(alloc, "{d}", .{i});
            defer alloc.free(formatted);
            try buf.appendSlice(alloc, formatted);
        },
        .float => |f| {
            const formatted = try std.fmt.allocPrint(alloc, "{d}", .{f});
            defer alloc.free(formatted);
            try buf.appendSlice(alloc, formatted);
        },
        .number_string => |s| try buf.appendSlice(alloc, s),
        .string => |s| try appendEscapedJsonString(alloc, buf, s),
        .array => |arr| {
            try buf.append(alloc, '[');
            let i: usize = 0;
            while (i < arr.items.len) {
                if (i > 0) try buf.append(alloc, ',');
                try stringifyInto(alloc, buf, arr.items[i]);
                i = i + 1;
            }
            try buf.append(alloc, ']');
        },
        .object => |obj| {
            try buf.append(alloc, '{');
            let first = true;
            let it = obj.iterator();
            while (entry := it.next()) {
                if (!first) try buf.append(alloc, ',');
                first = false;
                try appendEscapedJsonString(alloc, buf, entry.key_ptr.*);
                try buf.append(alloc, ':');
                try stringifyInto(alloc, buf, entry.value_ptr.*);
            }
            try buf.append(alloc, '}');
        },
        else => try buf.appendSlice(alloc, "null"),
    }
}

export fn stringifyValue(alloc: std.mem.Allocator, val: std.json.Value): ![]const u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);
    try stringifyInto(alloc, &buf, val);
    return try buf.toOwnedSlice(alloc);
}

export fn freeOps(alloc: std.mem.Allocator, ops: []JsonPatchOp) {
    for (op of ops) {
        alloc.free(op.path);
        if (f := op.from) {
            alloc.free(f);
        }
        if (v := op.value) {
            let mv = v;
            freeValue(alloc, &mv);
        }
    }
    alloc.free(ops);
}

fn parsePatchOpsFromValue(alloc: std.mem.Allocator, patch_val: std.json.Value): ![]JsonPatchOp {
    const arr = switch (patch_val) {
        .array => |a| a,
        else => return error.InvalidPatch,
    };
    let ops = std.ArrayList(JsonPatchOp).empty;
    errdefer {
        for (op of ops.items) {
            alloc.free(op.path);
            if (f := op.from) alloc.free(f);
            if (v := op.value) {
                let mv = v;
                freeValue(alloc, &mv);
            }
        }
        ops.deinit(alloc);
    }

    for (item of arr.items) {
        const obj = switch (item) {
            .object => |o| o,
            else => return error.InvalidPatch,
        };
        const op_val = obj.get("op") orelse return error.InvalidPatch;
        const op_str = switch (op_val) {
            .string => |s| s,
            else => return error.InvalidPatch,
        };
        const path_val = obj.get("path") orelse return error.InvalidPatch;
        const path_str = switch (path_val) {
            .string => |s| s,
            else => return error.InvalidPatch,
        };

        let kind: PatchOpKind = .Add;
        if (std.mem.eql(u8, op_str, "add")) {
            kind = .Add;
        } else if (std.mem.eql(u8, op_str, "remove")) {
            kind = .Remove;
        } else if (std.mem.eql(u8, op_str, "replace")) {
            kind = .Replace;
        } else if (std.mem.eql(u8, op_str, "move")) {
            kind = .Move;
        } else if (std.mem.eql(u8, op_str, "copy")) {
            kind = .Copy;
        } else if (std.mem.eql(u8, op_str, "test")) {
            kind = .Test;
        } else {
            return error.InvalidPatch;
        }

        const path_copy = try alloc.dupe(u8, path_str);
        errdefer alloc.free(path_copy);

        let from_copy: StrSlice? = null;
        if (fv := obj.get("from")) {
            const fs = switch (fv) {
                .string => |s| s,
                else => return error.InvalidPatch,
            };
            from_copy = try alloc.dupe(u8, fs);
        }
        errdefer {
            if (fc := from_copy) alloc.free(fc);
        }

        let val_copy: std.json.Value? = null;
        if (vv := obj.get("value")) {
            val_copy = try cloneValue(alloc, vv);
        }
        errdefer {
            if (vc := val_copy) {
                let mvc = vc;
                freeValue(alloc, &mvc);
            }
        }

        try ops.append(alloc, .{
            .op = kind,
            .path = path_copy,
            .from = from_copy,
            .value = val_copy,
        });
    }

    return try ops.toOwnedSlice(alloc);
}

export fn applyPatchJson(
    alloc: std.mem.Allocator,
    doc_json: []const u8,
    patch_json: []const u8
): ![]const u8 {
    let doc_parsed = std.json.parseFromSlice(std.json.Value, alloc, doc_json, .{}) catch {
        return error.InvalidJson;
    };
    defer doc_parsed.deinit();

    let patch_parsed = std.json.parseFromSlice(std.json.Value, alloc, patch_json, .{}) catch {
        return error.InvalidJson;
    };
    defer patch_parsed.deinit();

    const ops = try parsePatchOpsFromValue(alloc, patch_parsed.value);
    defer freeOps(alloc, ops);

    let patched = try applyPatch(alloc, doc_parsed.value, ops);
    defer freeValue(alloc, &patched);

    return try stringifyValue(alloc, patched);
}

fn diffRecursive(
    alloc: std.mem.Allocator,
    prefix: []const u8,
    source: std.json.Value,
    target: std.json.Value,
    ops: *std.ArrayList(JsonPatchOp)
): !void {
    if (valuesEqual(source, target)) return;

    switch (source) {
        .object => |src_obj| {
            switch (target) {
                .object => |dst_obj| {
                    let it_src = src_obj.iterator();
                    while (entry := it_src.next()) {
                        const k = entry.key_ptr.*;
                        if (dst_obj.get(k) == null) {
                            const esc = try escapePointerToken(alloc, k);
                            defer alloc.free(esc);
                            const child_path = try std.fmt.allocPrint(alloc, "{s}/{s}", .{ prefix, esc });
                            errdefer alloc.free(child_path);
                            try ops.append(alloc, .{
                                .op = .Remove,
                                .path = child_path,
                                .from = null,
                                .value = null,
                            });
                        }
                    }

                    let it_dst = dst_obj.iterator();
                    while (entry := it_dst.next()) {
                        const k = entry.key_ptr.*;
                        const dst_v = entry.value_ptr.*;
                        const esc = try escapePointerToken(alloc, k);
                        defer alloc.free(esc);
                        const child_path = try std.fmt.allocPrint(alloc, "{s}/{s}", .{ prefix, esc });
                        if (src_v := src_obj.get(k)) {
                            defer alloc.free(child_path);
                            try diffRecursive(alloc, child_path, src_v, dst_v, ops);
                        } else {
                            errdefer alloc.free(child_path);
                            let cloned = try cloneValue(alloc, dst_v);
                            errdefer freeValue(alloc, &cloned);
                            try ops.append(alloc, .{
                                .op = .Add,
                                .path = child_path,
                                .from = null,
                                .value = cloned,
                            });
                        }
                    }
                    return;
                },
                else => {},
            }
        },
        .array => |src_arr| {
            switch (target) {
                .array => |dst_arr| {
                    const min_len = @min(src_arr.items.len, dst_arr.items.len);
                    let i: usize = 0;
                    while (i < min_len) {
                        const child_path = try std.fmt.allocPrint(alloc, "{s}/{d}", .{ prefix, i });
                        defer alloc.free(child_path);
                        try diffRecursive(alloc, child_path, src_arr.items[i], dst_arr.items[i], ops);
                        i = i + 1;
                    }
                    if (dst_arr.items.len > src_arr.items.len) {
                        let j: usize = src_arr.items.len;
                        while (j < dst_arr.items.len) {
                            const child_path = try std.fmt.allocPrint(alloc, "{s}/-", .{prefix});
                            errdefer alloc.free(child_path);
                            let cloned = try cloneValue(alloc, dst_arr.items[j]);
                            errdefer freeValue(alloc, &cloned);
                            try ops.append(alloc, .{
                                .op = .Add,
                                .path = child_path,
                                .from = null,
                                .value = cloned,
                            });
                            j = j + 1;
                        }
                    } else if (src_arr.items.len > dst_arr.items.len) {
                        let j: usize = src_arr.items.len;
                        while (j > dst_arr.items.len) {
                            j = j - 1;
                            const child_path = try std.fmt.allocPrint(alloc, "{s}/{d}", .{ prefix, j });
                            errdefer alloc.free(child_path);
                            try ops.append(alloc, .{
                                .op = .Remove,
                                .path = child_path,
                                .from = null,
                                .value = null,
                            });
                        }
                    }
                    return;
                },
                else => {},
            }
        },
        else => {},
    }

    const path_copy = try alloc.dupe(u8, prefix);
    errdefer alloc.free(path_copy);
    let cloned_target = try cloneValue(alloc, target);
    errdefer freeValue(alloc, &cloned_target);
    try ops.append(alloc, .{
        .op = .Replace,
        .path = path_copy,
        .from = null,
        .value = cloned_target,
    });
}

export fn diff(
    alloc: std.mem.Allocator,
    source: std.json.Value,
    target: std.json.Value
): ![]JsonPatchOp {
    let ops = std.ArrayList(JsonPatchOp).empty;
    errdefer {
        for (op of ops.items) {
            alloc.free(op.path);
            if (f := op.from) alloc.free(f);
            if (v := op.value) {
                let mv = v;
                freeValue(alloc, &mv);
            }
        }
        ops.deinit(alloc);
    }
    try diffRecursive(alloc, "", source, target, &ops);
    return try ops.toOwnedSlice(alloc);
}
