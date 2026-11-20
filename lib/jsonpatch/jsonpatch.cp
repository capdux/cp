// Source: cp/lib/jsonpatch/jsonpatch.cp (RFC 6901 JSON Pointer & RFC 6902 JSON Patch Atomic Engine)
// Pure CP RFC 6901 JSON Pointer & RFC 6902 JSON Patch Atomic Engine built on std.json.

const std = @import("std");

pub err JsonPatchError {
    InvalidJson,
    InvalidPointer,
    InvalidPatch,
    PathNotFound,
    IndexOutOfBounds,
    TestFailed,
    CycleMove,
    OutOfMemory,
}

pub enum PatchOpKind {
    Add,
    Remove,
    Replace,
    Move,
    Copy,
    Test,

    pub fn fromStr(s: []const u8): PatchOpKind? {
        if (std.mem.eql(u8, s, "add")) return PatchOpKind.Add;
        if (std.mem.eql(u8, s, "remove")) return PatchOpKind.Remove;
        if (std.mem.eql(u8, s, "replace")) return PatchOpKind.Replace;
        if (std.mem.eql(u8, s, "move")) return PatchOpKind.Move;
        if (std.mem.eql(u8, s, "copy")) return PatchOpKind.Copy;
        if (std.mem.eql(u8, s, "test")) return PatchOpKind.Test;
        return null;
    }

    pub fn toStr(): []const u8 {
        return switch (_) {
            PatchOpKind.Add => "add",
            PatchOpKind.Remove => "remove",
            PatchOpKind.Replace => "replace",
            PatchOpKind.Move => "move",
            PatchOpKind.Copy => "copy",
            PatchOpKind.Test => "test",
        };
    }
}

/// Escapes a single RFC 6901 reference token (`~` -> `~0`, `/` -> `~1`).
pub fn escapeSegment(allocator: std.mem.Allocator, segment: []const u8): ([]u8) ! JsonPatchError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);
    for (c of segment) {
        if (c == '~') {
            out.appendSlice(allocator, "~0") catch return JsonPatchError.OutOfMemory;
        } else if (c == '/') {
            out.appendSlice(allocator, "~1") catch return JsonPatchError.OutOfMemory;
        } else {
            out.append(allocator, c) catch return JsonPatchError.OutOfMemory;
        }
    }
    return out.toOwnedSlice(allocator) catch return JsonPatchError.OutOfMemory;
}

/// Unescapes a single RFC 6901 reference token (`~1` -> `/`, `~0` -> `~` in one pass).
pub fn unescapeSegment(allocator: std.mem.Allocator, segment: []const u8): ([]u8) ! JsonPatchError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);
    let i: usize = 0;
    while (i < segment.len) {
        const c = segment[i];
        if (c == '~') {
            if (i + 1 >= segment.len) return JsonPatchError.InvalidPointer;
            const nxt = segment[i + 1];
            if (nxt == '0') {
                out.append(allocator, '~') catch return JsonPatchError.OutOfMemory;
            } else if (nxt == '1') {
                out.append(allocator, '/') catch return JsonPatchError.OutOfMemory;
            } else {
                return JsonPatchError.InvalidPointer;
            }
            i += 2;
        } else {
            out.append(allocator, c) catch return JsonPatchError.OutOfMemory;
            i += 1;
        }
    }
    return out.toOwnedSlice(allocator) catch return JsonPatchError.OutOfMemory;
}

/// Validates an escaped RFC 6901 segment without allocating.
fn validateEscapedSegment(segment: []const u8): bool {
    let i: usize = 0;
    while (i < segment.len) {
        if (segment[i] == '~') {
            if (i + 1 >= segment.len) return false;
            const nxt = segment[i + 1];
            if (nxt != '0' && nxt != '1') return false;
            i += 2;
        } else {
            i += 1;
        }
    }
    return true;
}

/// Compares an escaped RFC 6901 segment against an unescaped key without allocating.
fn escapedSegmentEqualsKey(escaped: []const u8, key: []const u8): bool {
    let ei: usize = 0;
    let ki: usize = 0;
    while (ei < escaped.len) {
        if (ki >= key.len) return false;
        if (escaped[ei] == '~') {
            if (ei + 1 >= escaped.len) return false;
            const nxt = escaped[ei + 1];
            let decoded: u8 = 0;
            if (nxt == '0') {
                decoded = '~';
            } else if (nxt == '1') {
                decoded = '/';
            } else {
                return false;
            }
            if (key[ki] != decoded) return false;
            ei += 2;
            ki += 1;
        } else {
            if (key[ki] != escaped[ei]) return false;
            ei += 1;
            ki += 1;
        }
    }
    return ki == key.len;
}

/// Parses a strict RFC 6901 array index (`0` or `[1-9][0-9]*`). Rejects leading zeros such as `"01"`.
fn parseArrayIndex(token: []const u8): usize? {
    if (token.len == 0) return null;
    if (token.len == 1 && token[0] == '0') return 0;
    if (token[0] < '1' || token[0] > '9') return null;
    let idx: usize = 0;
    for (c of token) {
        if (c < '0' || c > '9') return null;
        idx = std.math.mul(usize, idx, 10) catch return null;
        idx = std.math.add(usize, idx, (c - '0') as usize) catch return null;
    }
    return idx;
}

/// Resolves an RFC 6901 JSON Pointer against a `std.json.Value` without heap allocation.
pub fn resolvePointer(doc_val: std.json.Value, pointer: []const u8): std.json.Value ! JsonPatchError {
    if (pointer.len == 0) return doc_val;
    if (pointer[0] != '/') return JsonPatchError.InvalidPointer;

    let cur = doc_val;
    let pos: usize = 1;
    while (true) {
        let next_slash = pos;
        while (next_slash < pointer.len && pointer[next_slash] != '/') {
            next_slash += 1;
        }
        const raw_seg = pointer[pos..next_slash];
        if (!validateEscapedSegment(raw_seg)) return JsonPatchError.InvalidPointer;

        switch (cur) {
            .object => |obj| {
                let found: std.json.Value? = null;
                if (std.mem.indexOfScalar(u8, raw_seg, '~') == null) {
                    found = obj.get(raw_seg);
                } else {
                    let it = obj.iterator();
                    while (entry := it.next()) {
                        if (escapedSegmentEqualsKey(raw_seg, entry.key_ptr.*)) {
                            found = entry.value_ptr.*;
                            break;
                        }
                    }
                }
                if (next_val := found) {
                    cur = next_val;
                } else {
                    return JsonPatchError.PathNotFound;
                }
            },
            .array => |arr| {
                if (std.mem.eql(u8, raw_seg, "-")) {
                    return JsonPatchError.IndexOutOfBounds;
                }
                if (idx := parseArrayIndex(raw_seg)) {
                    if (idx >= arr.items.len) return JsonPatchError.IndexOutOfBounds;
                    cur = arr.items[idx];
                } else {
                    return JsonPatchError.InvalidPointer;
                }
            },
            else => return JsonPatchError.PathNotFound,
        }

        if (next_slash >= pointer.len) break;
        pos = next_slash + 1;
    }
    return cur;
}

/// Parses an RFC 6901 JSON Pointer into unescaped reference tokens.
fn parsePointerTokens(allocator: std.mem.Allocator, pointer: []const u8): ([][]const u8) ! JsonPatchError {
    if (pointer.len == 0) {
        let empty_tokens: std.ArrayList(string) = .empty;
        return empty_tokens.toOwnedSlice(allocator) catch return JsonPatchError.OutOfMemory;
    }
    if (pointer[0] != '/') return JsonPatchError.InvalidPointer;

    let tokens: std.ArrayList(string) = .empty;
    errdefer tokens.deinit(allocator);

    let pos: usize = 1;
    while (true) {
        let next_slash = pos;
        while (next_slash < pointer.len && pointer[next_slash] != '/') {
            next_slash += 1;
        }
        const raw_seg = pointer[pos..next_slash];
        const decoded = unescapeSegment(allocator, raw_seg)?;
        tokens.append(allocator, decoded) catch return JsonPatchError.OutOfMemory;
        if (next_slash >= pointer.len) break;
        pos = next_slash + 1;
    }
    return tokens.toOwnedSlice(allocator) catch return JsonPatchError.OutOfMemory;
}

/// Deep clones a `std.json.Value` onto the given arena/allocator.
fn cloneValue(allocator: std.mem.Allocator, val: std.json.Value): std.json.Value ! JsonPatchError {
    switch (val) {
        .null => return std.json.Value{ .null = {} },
        .bool => |b| return std.json.Value{ .bool = b },
        .integer => |i| return std.json.Value{ .integer = i },
        .float => |f| return std.json.Value{ .float = f },
        .number_string => |s| {
            const dup = allocator.dupe(u8, s) catch return JsonPatchError.OutOfMemory;
            return std.json.Value{ .number_string = dup };
        },
        .string => |s| {
            const dup = allocator.dupe(u8, s) catch return JsonPatchError.OutOfMemory;
            return std.json.Value{ .string = dup };
        },
        .array => |arr| {
            let new_arr = std.json.Array.init(allocator);
            new_arr.ensureTotalCapacity(arr.items.len) catch return JsonPatchError.OutOfMemory;
            for (item of arr.items) {
                const cloned_item = cloneValue(allocator, item)?;
                new_arr.append(cloned_item) catch return JsonPatchError.OutOfMemory;
            }
            return std.json.Value{ .array = new_arr };
        },
        .object => |obj| {
            let new_obj = std.json.ObjectMap.init(allocator);
            new_obj.ensureTotalCapacity(obj.count() as u32) catch return JsonPatchError.OutOfMemory;
            let it = obj.iterator();
            while (entry := it.next()) {
                const k_dup = allocator.dupe(u8, entry.key_ptr.*) catch return JsonPatchError.OutOfMemory;
                const v_dup = cloneValue(allocator, entry.value_ptr.*)?;
                new_obj.put(k_dup, v_dup) catch return JsonPatchError.OutOfMemory;
            }
            return std.json.Value{ .object = new_obj };
        },
    }
}

/// Checks deep structural equality of two `std.json.Value` trees per RFC 6902 Section 4.6.
pub fn jsonValuesEqual(a: std.json.Value, b: std.json.Value): bool {
    switch (a) {
        .null => return switch (b) {
            .null => true,
            else => false,
        },
        .bool => |ab| return switch (b) {
            .bool => |bb| ab == bb,
            else => false,
        },
        .integer => |ai| return switch (b) {
            .integer => |bi| ai == bi,
            .float => |bf| (ai as f64) == bf,
            .number_string => |bs| {
                const bf = std.fmt.parseFloat(f64, bs) catch return false;
                return (ai as f64) == bf;
            },
            else => false,
        },
        .float => |af| return switch (b) {
            .float => |bf| af == bf,
            .integer => |bi| af == (bi as f64),
            .number_string => |bs| {
                const bf = std.fmt.parseFloat(f64, bs) catch return false;
                return af == bf;
            },
            else => false,
        },
        .number_string => |as_str| return switch (b) {
            .number_string => |bs_str| {
                if (std.mem.eql(u8, as_str, bs_str)) return true;
                const af = std.fmt.parseFloat(f64, as_str) catch return false;
                const bf = std.fmt.parseFloat(f64, bs_str) catch return false;
                return af == bf;
            },
            .integer => |bi| {
                const af = std.fmt.parseFloat(f64, as_str) catch return false;
                return af == (bi as f64);
            },
            .float => |bf| {
                const af = std.fmt.parseFloat(f64, as_str) catch return false;
                return af == bf;
            },
            else => false,
        },
        .string => |as_str| return switch (b) {
            .string => |bs_str| std.mem.eql(u8, as_str, bs_str),
            else => false,
        },
        .array => |aa| return switch (b) {
            .array => |ba| {
                if (aa.items.len != ba.items.len) return false;
                let i: usize = 0;
                while (i < aa.items.len) : (i += 1) {
                    if (!jsonValuesEqual(aa.items[i], ba.items[i])) return false;
                }
                return true;
            },
            else => false,
        },
        .object => |ao| return switch (b) {
            .object => |bo| {
                if (ao.count() != bo.count()) return false;
                let it = ao.iterator();
                while (entry := it.next()) {
                    if (b_val := bo.get(entry.key_ptr.*)) {
                        if (!jsonValuesEqual(entry.value_ptr.*, b_val)) return false;
                    } else {
                        return false;
                    }
                }
                return true;
            },
            else => false,
        },
    }
}

/// Navigates to the mutable parent container of `tokens` (i.e. `tokens[0..tokens.len - 1]`).
fn resolveParentPtr(root: *std.json.Value, tokens: []const []const u8): (*std.json.Value) ! JsonPatchError {
    let cur = root;
    let i: usize = 0;
    while (i + 1 < tokens.len) : (i += 1) {
        const tok = tokens[i];
        switch (cur.*) {
            .object => |*obj| {
                if (child_ptr := obj.getPtr(tok)) {
                    cur = child_ptr;
                } else {
                    return JsonPatchError.PathNotFound;
                }
            },
            .array => |*arr| {
                if (std.mem.eql(u8, tok, "-")) return JsonPatchError.IndexOutOfBounds;
                if (idx := parseArrayIndex(tok)) {
                    if (idx >= arr.items.len) return JsonPatchError.IndexOutOfBounds;
                    cur = &arr.items[idx];
                } else {
                    return JsonPatchError.InvalidPointer;
                }
            },
            else => return JsonPatchError.PathNotFound,
        }
    }
    return cur;
}

/// Resolves a value pointer by token list.
fn resolveValueByTokens(root: std.json.Value, tokens: []const []const u8): std.json.Value ! JsonPatchError {
    let cur = root;
    for (tok of tokens) {
        switch (cur) {
            .object => |obj| {
                if (next_v := obj.get(tok)) {
                    cur = next_v;
                } else {
                    return JsonPatchError.PathNotFound;
                }
            },
            .array => |arr| {
                if (std.mem.eql(u8, tok, "-")) return JsonPatchError.IndexOutOfBounds;
                if (idx := parseArrayIndex(tok)) {
                    if (idx >= arr.items.len) return JsonPatchError.IndexOutOfBounds;
                    cur = arr.items[idx];
                } else {
                    return JsonPatchError.InvalidPointer;
                }
            },
            else => return JsonPatchError.PathNotFound,
        }
    }
    return cur;
}

/// Checks if `prefix` is a strict prefix of `full` (used for CycleMove detection).
fn isStrictPrefixTokens(prefix: []const []const u8, full: []const []const u8): bool {
    if (prefix.len >= full.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (!std.mem.eql(u8, prefix[i], full[i])) return false;
    }
    return true;
}

/// Checks if two token slices are identical.
fn tokensEqual(a: []const []const u8, b: []const []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (!std.mem.eql(u8, a[i], b[i])) return false;
    }
    return true;
}

/// Applies an `add` operation in-place on `root`.
fn execAdd(allocator: std.mem.Allocator, root: *std.json.Value, tokens: []const []const u8, val: std.json.Value): void ! JsonPatchError {
    const cloned = cloneValue(allocator, val)?;
    if (tokens.len == 0) {
        let r = root;
        r.* = cloned;
        return;
    }
    const parent = resolveParentPtr(root, tokens)?;
    const last = tokens[tokens.len - 1];
    switch (parent.*) {
        .object => |*obj| {
            const k_dup = allocator.dupe(u8, last) catch return JsonPatchError.OutOfMemory;
            obj.put(k_dup, cloned) catch return JsonPatchError.OutOfMemory;
        },
        .array => |*arr| {
            if (std.mem.eql(u8, last, "-")) {
                arr.append(cloned) catch return JsonPatchError.OutOfMemory;
            } else {
                if (idx := parseArrayIndex(last)) {
                    if (idx > arr.items.len) return JsonPatchError.IndexOutOfBounds;
                    arr.insert(idx, cloned) catch return JsonPatchError.OutOfMemory;
                } else {
                    return JsonPatchError.InvalidPointer;
                }
            }
        },
        else => return JsonPatchError.PathNotFound,
    }
}

/// Applies a `remove` operation in-place on `root` and returns the removed value.
fn execRemove(root: *std.json.Value, tokens: []const []const u8): std.json.Value ! JsonPatchError {
    if (tokens.len == 0) return JsonPatchError.InvalidPatch;
    const parent = resolveParentPtr(root, tokens)?;
    const last = tokens[tokens.len - 1];
    switch (parent.*) {
        .object => |*obj| {
            if (removed := obj.fetchOrderedRemove(last)) {
                return removed.value;
            } else {
                return JsonPatchError.PathNotFound;
            }
        },
        .array => |*arr| {
            if (std.mem.eql(u8, last, "-")) return JsonPatchError.IndexOutOfBounds;
            if (idx := parseArrayIndex(last)) {
                if (idx >= arr.items.len) return JsonPatchError.IndexOutOfBounds;
                return arr.orderedRemove(idx);
            } else {
                return JsonPatchError.InvalidPointer;
            }
        },
        else => return JsonPatchError.PathNotFound,
    }
}

/// Applies a `replace` operation in-place on `root`.
fn execReplace(allocator: std.mem.Allocator, root: *std.json.Value, tokens: []const []const u8, val: std.json.Value): void ! JsonPatchError {
    const cloned = cloneValue(allocator, val)?;
    if (tokens.len == 0) {
        let r = root;
        r.* = cloned;
        return;
    }
    const parent = resolveParentPtr(root, tokens)?;
    const last = tokens[tokens.len - 1];
    switch (parent.*) {
        .object => |*obj| {
            if (entry_ptr := obj.getPtr(last)) {
                let mut_ptr = entry_ptr;
                mut_ptr.* = cloned;
            } else {
                return JsonPatchError.PathNotFound;
            }
        },
        .array => |*arr| {
            if (std.mem.eql(u8, last, "-")) return JsonPatchError.IndexOutOfBounds;
            if (idx := parseArrayIndex(last)) {
                if (idx >= arr.items.len) return JsonPatchError.IndexOutOfBounds;
                arr.items[idx] = cloned;
            } else {
                return JsonPatchError.InvalidPointer;
            }
        },
        else => return JsonPatchError.PathNotFound,
    }
}

/// Atomically applies an RFC 6902 JSON Patch document to `doc_json`.
/// Non-destructive: operates on an isolated deep-cloned arena tree; any failure rolls back cleanly.
pub fn applyPatch(allocator: std.mem.Allocator, doc_json: []const u8, patch_json: []const u8): ([]u8) ! JsonPatchError {
    let mem_arena = std.heap.ArenaAllocator.init(allocator);
    defer mem_arena.deinit();
    const aa = mem_arena.allocator();

    const parsed_doc = std.json.parseFromSliceLeaky(std.json.Value, aa, doc_json, .{}) catch e => {
        return if (e == error.OutOfMemory) JsonPatchError.OutOfMemory else JsonPatchError.InvalidJson;
    };
    const parsed_patch = std.json.parseFromSliceLeaky(std.json.Value, aa, patch_json, .{}) catch e => {
        return if (e == error.OutOfMemory) JsonPatchError.OutOfMemory else JsonPatchError.InvalidJson;
    };

    const ops_arr = switch (parsed_patch) {
        .array => |arr| arr,
        else => return JsonPatchError.InvalidPatch,
    };

    let work_doc = cloneValue(aa, parsed_doc)?;

    for (op_val of ops_arr.items) {
        const op_obj = switch (op_val) {
            .object => |obj| obj,
            else => return JsonPatchError.InvalidPatch,
        };

        const op_field_opt = op_obj.get("op");
        if (op_field_opt == null) return JsonPatchError.InvalidPatch;
        const op_field = op_field_opt.?;
        const op_str = switch (op_field) {
            .string => |s| s,
            else => return JsonPatchError.InvalidPatch,
        };
        const kind_opt = PatchOpKind.fromStr(op_str);
        if (kind_opt == null) return JsonPatchError.InvalidPatch;
        const kind = kind_opt.?;

        const path_field_opt = op_obj.get("path");
        if (path_field_opt == null) return JsonPatchError.InvalidPatch;
        const path_field = path_field_opt.?;
        const path_str = switch (path_field) {
            .string => |s| s,
            else => return JsonPatchError.InvalidPointer,
        };
        const path_tokens = parsePointerTokens(aa, path_str)?;

        switch (kind) {
            PatchOpKind.Add => {
                const val_opt = op_obj.get("value");
                if (val_opt == null) return JsonPatchError.InvalidPatch;
                execAdd(aa, &work_doc, path_tokens, val_opt.?)?;
            },
            PatchOpKind.Remove => {
                * = execRemove(&work_doc, path_tokens)?;
            },
            PatchOpKind.Replace => {
                const val_opt = op_obj.get("value");
                if (val_opt == null) return JsonPatchError.InvalidPatch;
                execReplace(aa, &work_doc, path_tokens, val_opt.?)?;
            },
            PatchOpKind.Move => {
                const from_field_opt = op_obj.get("from");
                if (from_field_opt == null) return JsonPatchError.InvalidPatch;
                const from_field = from_field_opt.?;
                const from_str = switch (from_field) {
                    .string => |s| s,
                    else => return JsonPatchError.InvalidPointer,
                };
                const from_tokens = parsePointerTokens(aa, from_str)?;
                if (isStrictPrefixTokens(from_tokens, path_tokens)) {
                    return JsonPatchError.CycleMove;
                }
                if (tokensEqual(from_tokens, path_tokens)) {
                    * = resolveValueByTokens(work_doc, from_tokens)?;
                } else {
                    const moved_val = execRemove(&work_doc, from_tokens)?;
                    execAdd(aa, &work_doc, path_tokens, moved_val)?;
                }
            },
            PatchOpKind.Copy => {
                const from_field_opt = op_obj.get("from");
                if (from_field_opt == null) return JsonPatchError.InvalidPatch;
                const from_field = from_field_opt.?;
                const from_str = switch (from_field) {
                    .string => |s| s,
                    else => return JsonPatchError.InvalidPointer,
                };
                const from_tokens = parsePointerTokens(aa, from_str)?;
                const src_val = resolveValueByTokens(work_doc, from_tokens)?;
                execAdd(aa, &work_doc, path_tokens, src_val)?;
            },
            PatchOpKind.Test => {
                const expected_opt = op_obj.get("value");
                if (expected_opt == null) return JsonPatchError.InvalidPatch;
                const actual = resolveValueByTokens(work_doc, path_tokens)?;
                if (!jsonValuesEqual(actual, expected_opt.?)) {
                    return JsonPatchError.TestFailed;
                }
            },
        }
    }

    return std.json.Stringify.valueAlloc(allocator, work_doc, .{}) catch return JsonPatchError.OutOfMemory;
}

/// Appends a single JSON Patch op object into `ops_list`.
fn appendPatchOp(
    allocator: std.mem.Allocator,
    ops_list: *std.json.Array,
    op_name: []const u8,
    path: []const u8,
    val: std.json.Value?,
): void ! JsonPatchError {
    let obj = std.json.ObjectMap.init(allocator);
    obj.put("op", std.json.Value{ .string = op_name }) catch return JsonPatchError.OutOfMemory;
    obj.put("path", std.json.Value{ .string = path }) catch return JsonPatchError.OutOfMemory;
    if (v := val) {
        obj.put("value", v) catch return JsonPatchError.OutOfMemory;
    }
    ops_list.append(std.json.Value{ .object = obj }) catch return JsonPatchError.OutOfMemory;
}

/// Formats `prefix + "/" + escapeSegment(seg)`.
fn joinPointerSegment(allocator: std.mem.Allocator, prefix: []const u8, seg: []const u8): ([]const u8) ! JsonPatchError {
    const esc = escapeSegment(allocator, seg)?;
    return std.fmt.allocPrint(allocator, "{s}/{s}", .{ prefix, esc }) catch return JsonPatchError.OutOfMemory;
}

/// Formats `prefix + "/" + idx`.
fn joinPointerIndex(allocator: std.mem.Allocator, prefix: []const u8, idx: usize): ([]const u8) ! JsonPatchError {
    return std.fmt.allocPrint(allocator, "{s}/{d}", .{ prefix, idx }) catch return JsonPatchError.OutOfMemory;
}

/// Recursively computes minimal RFC 6902 patch operations from `from_val` to `to_val`.
fn diffValues(
    allocator: std.mem.Allocator,
    prefix: []const u8,
    from_val: std.json.Value,
    to_val: std.json.Value,
    ops_list: *std.json.Array,
): void ! JsonPatchError {
    if (jsonValuesEqual(from_val, to_val)) return;

    switch (from_val) {
        .object => |fo| {
            switch (to_val) {
                .object => |to_obj| {
                    // 1. Removed or modified keys
                    let fit = fo.iterator();
                    while (entry := fit.next()) {
                        const k = entry.key_ptr.*;
                        const child_path = joinPointerSegment(allocator, prefix, k)?;
                        if (tv := to_obj.get(k)) {
                            diffValues(allocator, child_path, entry.value_ptr.*, tv, ops_list)?;
                        } else {
                            appendPatchOp(allocator, ops_list, "remove", child_path, null)?;
                        }
                    }
                    // 2. Added keys
                    let tit = to_obj.iterator();
                    while (entry := tit.next()) {
                        const k = entry.key_ptr.*;
                        if (!fo.contains(k)) {
                            const child_path = joinPointerSegment(allocator, prefix, k)?;
                            appendPatchOp(allocator, ops_list, "add", child_path, entry.value_ptr.*)?;
                        }
                    }
                    return;
                },
                else => {},
            }
        },
        .array => |fa| {
            switch (to_val) {
                .array => |ta| {
                    const min_len = @min(fa.items.len, ta.items.len);
                    let i: usize = 0;
                    while (i < min_len) : (i += 1) {
                        const child_path = joinPointerIndex(allocator, prefix, i)?;
                        diffValues(allocator, child_path, fa.items[i], ta.items[i], ops_list)?;
                    }
                    if (fa.items.len > ta.items.len) {
                        // Remove trailing elements in reverse order so indices stay valid.
                        let idx = fa.items.len;
                        while (idx > ta.items.len) {
                            idx -= 1;
                            const child_path = joinPointerIndex(allocator, prefix, idx)?;
                            appendPatchOp(allocator, ops_list, "remove", child_path, null)?;
                        }
                    } else if (ta.items.len > fa.items.len) {
                        let idx = fa.items.len;
                        while (idx < ta.items.len) : (idx += 1) {
                            const append_path = std.fmt.allocPrint(allocator, "{s}/-", .{prefix}) catch return JsonPatchError.OutOfMemory;
                            appendPatchOp(allocator, ops_list, "add", append_path, ta.items[idx])?;
                        }
                    }
                    return;
                },
                else => {},
            }
        },
        else => {},
    }

    appendPatchOp(allocator, ops_list, "replace", prefix, to_val)?;
}

/// Generates a minimal RFC 6902 JSON Patch document transforming `from_json` into `to_json`.
pub fn createDiffPatch(allocator: std.mem.Allocator, from_json: []const u8, to_json: []const u8): ([]u8) ! JsonPatchError {
    let mem_arena = std.heap.ArenaAllocator.init(allocator);
    defer mem_arena.deinit();
    const aa = mem_arena.allocator();

    const from_val = std.json.parseFromSliceLeaky(std.json.Value, aa, from_json, .{}) catch e => {
        return if (e == error.OutOfMemory) JsonPatchError.OutOfMemory else JsonPatchError.InvalidJson;
    };
    const to_val = std.json.parseFromSliceLeaky(std.json.Value, aa, to_json, .{}) catch e => {
        return if (e == error.OutOfMemory) JsonPatchError.OutOfMemory else JsonPatchError.InvalidJson;
    };

    let ops_list = std.json.Array.init(aa);
    diffValues(aa, "", from_val, to_val, &ops_list)?;

    const patch_val = std.json.Value{ .array = ops_list };
    return std.json.Stringify.valueAlloc(allocator, patch_val, .{}) catch return JsonPatchError.OutOfMemory;
}
