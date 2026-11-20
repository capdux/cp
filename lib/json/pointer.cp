// Source: cp/lib/json/pointer.cp (RFC 6901 JSON Pointer & RFC 6902 JSON Patch)
// Complete RFC 6901 JSON Pointer, RFC 6902 JSON Patch & RFC 7396 JSON Merge Patch

const std = @import("std");
import { JsonValue, JsonField, JsonTag } from "./value";

pub err JsonPatchError {
    InvalidPointer,
    PathNotFound,
    IndexOutOfBounds,
    InvalidTarget,
    TestFailed,
    MissingValue,
    MissingFrom,
    CannotMoveIntoChild,
    InvalidOperation,
}

pub struct JsonPointer {
    /// Escapes a single RFC 6901 reference token (`~` -> `~0`, `/` -> `~1`).
    pub static fn escapeToken(allocator: std.mem.Allocator, token: []const u8): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        for (c of token) {
            if (c == '~') {
                out.appendSlice(allocator, "~0")?;
            } else if (c == '/') {
                out.appendSlice(allocator, "~1")?;
            } else {
                out.append(allocator, c)?;
            }
        }
        return out.toOwnedSlice(allocator);
    }

    /// Unescapes a single RFC 6901 reference token (`~1` -> `/`, `~0` -> `~` in one pass).
    pub static fn unescapeToken(allocator: std.mem.Allocator, escaped: []const u8): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        let i: usize = 0;
        while (i < escaped.len) {
            const c = escaped[i];
            if (c == '~') {
                if (i + 1 >= escaped.len) return err.InvalidPointer;
                const nxt = escaped[i + 1];
                if (nxt == '0') {
                    out.append(allocator, '~')?;
                } else if (nxt == '1') {
                    out.append(allocator, '/')?;
                } else {
                    return err.InvalidPointer;
                }
                i += 2;
            } else {
                out.append(allocator, c)?;
                i += 1;
            }
        }
        return out.toOwnedSlice(allocator);
    }

    /// Parses a strict RFC 6901 array index (`0` or `[1-9][0-9]*`). Rejects leading zeros such as `"01"`.
    pub static fn parseArrayIndex(token: []const u8): usize? {
        if (token.len == 0) return null;
        if (token.len == 1 && token[0] == '0') return 0;
        if (token[0] < '1' || token[0] > '9') return null;
        let idx: usize = 0;
        for (c of token) {
            if (c < '0' || c > '9') return null;
            idx = idx * 10 + ((c - '0') as usize);
        }
        return idx;
    }

    /// Splits and unescapes an RFC 6901 pointer into reference tokens.
    /// Empty string `""` produces an empty slice.
    pub static fn parseTokens(allocator: std.mem.Allocator, ptr: []const u8): ([][]u8)! {
        if (ptr.len == 0) {
            let empty_tokens: std.ArrayList([]u8) = .empty;
            return empty_tokens.toOwnedSlice(allocator);
        }
        if (ptr[0] != '/') {
            return err.InvalidPointer;
        }
        let tokens: std.ArrayList([]u8) = .empty;
        errdefer {
            for (t of tokens.items) allocator.free(t);
            tokens.deinit(allocator);
        }
        let pos: usize = 1;
        while (true) {
            let next_slash = pos;
            while (next_slash < ptr.len && ptr[next_slash] != '/') {
                next_slash += 1;
            }
            const raw_tok = ptr[pos..next_slash];
            const decoded = JsonPointer.unescapeToken(allocator, raw_tok)?;
            tokens.append(allocator, decoded)?;
            if (next_slash >= ptr.len) break;
            pos = next_slash + 1;
        }
        return tokens.toOwnedSlice(allocator);
    }

    /// Frees a token slice allocated by `parseTokens`.
    pub static fn freeTokens(allocator: std.mem.Allocator, tokens: [][]u8) {
        for (t of tokens) {
            allocator.free(t);
        }
        if (tokens.len > 0) {
            allocator.free(tokens);
        }
    }

    /// Formats a list of unescaped reference tokens into an RFC 6901 pointer string.
    pub static fn format(allocator: std.mem.Allocator, tokens: []const []const u8): ([]u8)! {
        if (tokens.len == 0) {
            return allocator.alloc(u8, 0);
        }
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        for (tok of tokens) {
            out.append(allocator, '/')?;
            const esc = JsonPointer.escapeToken(allocator, tok)?;
            defer allocator.free(esc);
            out.appendSlice(allocator, esc)?;
        }
        return out.toOwnedSlice(allocator);
    }

    /// Evaluates an RFC 6901 JSON Pointer against `root`.
    /// Rejects invalid escapes and array indices with leading zeros (e.g. `"01"`).
    pub static fn resolve(root: JsonValue, ptr: []const u8): JsonValue? {
        return root.pointer(ptr);
    }
}

pub enum JsonPatchOpKind {
    Add,
    Remove,
    Replace,
    Move,
    Copy,
    Test,
}

pub struct JsonPatchOp {
    op: JsonPatchOpKind = .Add,
    path: []const u8 = "",
    from_path: ([]const u8)? = null,
    value: JsonValue? = null,

    pub static fn add(path: []const u8, value: JsonValue): JsonPatchOp {
        return JsonPatchOp{ op: .Add, path: path, value: value };
    }

    pub static fn remove(path: []const u8): JsonPatchOp {
        return JsonPatchOp{ op: .Remove, path: path };
    }

    pub static fn replaceOp(path: []const u8, value: JsonValue): JsonPatchOp {
        return JsonPatchOp{ op: .Replace, path: path, value: value };
    }

    pub static fn move(src_path: []const u8, path: []const u8): JsonPatchOp {
        return JsonPatchOp{ op: .Move, from_path: src_path, path: path };
    }

    pub static fn copy(src_path: []const u8, path: []const u8): JsonPatchOp {
        return JsonPatchOp{ op: .Copy, from_path: src_path, path: path };
    }

    pub static fn testValue(path: []const u8, value: JsonValue): JsonPatchOp {
        return JsonPatchOp{ op: .Test, path: path, value: value };
    }
}

pub struct JsonPatch {
    /// Deep structural equality check (RFC 6902 §4.6 `test` semantics).
    pub static fn deepEqual(a: JsonValue, b: JsonValue): bool {
        return a.equals(b);
    }

    /// Deep-clones a `JsonValue` into `allocator` so patched trees do not alias input slices.
    pub static fn cloneValue(allocator: std.mem.Allocator, val: JsonValue): JsonValue! {
        return switch (val.tag) {
            .Null => JsonValue{ tag: .Null },
            .Bool => JsonValue{ tag: .Bool, bool_val: val.bool_val },
            .Number => JsonValue{ tag: .Number, num_val: val.num_val },
            .String => {
                const s = allocator.dupe(u8, val.str_val)?;
                return JsonValue{ tag: .String, str_val: s };
            },
            .Array => {
                let arr = allocator.alloc(JsonValue, val.arr_val.len)?;
                errdefer allocator.free(arr);
                let i: usize = 0;
                while (i < val.arr_val.len) : (i += 1) {
                    arr[i] = JsonPatch.cloneValue(allocator, val.arr_val[i])?;
                }
                return JsonValue{ tag: .Array, arr_val: arr };
            },
            .Object => {
                let fields = allocator.alloc(JsonField, val.obj_val.len)?;
                errdefer allocator.free(fields);
                let i: usize = 0;
                while (i < val.obj_val.len) : (i += 1) {
                    const k = allocator.dupe(u8, val.obj_val[i].key)?;
                    const v = JsonPatch.cloneValue(allocator, val.obj_val[i].value)?;
                    fields[i] = JsonField{ key: k, value: v };
                }
                return JsonValue{ tag: .Object, obj_val: fields };
            },
        };
    }

    static fn navigateParent(root: *JsonValue, tokens: [][]u8): (*JsonValue)! {
        let cur: *JsonValue = root;
        let i: usize = 0;
        const limit = tokens.len - 1;
        while (i < limit) : (i += 1) {
            const tok = tokens[i];
            if (cur.tag == .Object) {
                let found: (*JsonValue)? = null;
                let j: usize = 0;
                while (j < cur.obj_val.len) : (j += 1) {
                    if (std.mem.eql(u8, cur.obj_val[j].key, tok)) {
                        found = &cur.obj_val[j].value;
                        break;
                    }
                }
                if (nxt := found) {
                    cur = nxt;
                } else {
                    return err.PathNotFound;
                }
            } else if (cur.tag == .Array) {
                const idx = JsonPointer.parseArrayIndex(tok) ?? return err.InvalidPointer;
                if (idx >= cur.arr_val.len) return err.IndexOutOfBounds;
                cur = &cur.arr_val[idx];
            } else {
                return err.InvalidTarget;
            }
        }
        return cur;
    }

    static fn applyAdd(allocator: std.mem.Allocator, root: *JsonValue, path: []const u8, val: JsonValue): ! {
        if (path.len == 0) {
            root.* = JsonPatch.cloneValue(allocator, val)?;
            return;
        }
        const tokens = JsonPointer.parseTokens(allocator, path)?;
        defer JsonPointer.freeTokens(allocator, tokens);

        const parent = JsonPatch.navigateParent(root, tokens)?;
        const leaf = tokens[tokens.len - 1];

        if (parent.tag == .Object) {
            const cloned_val = JsonPatch.cloneValue(allocator, val)?;
            let j: usize = 0;
            while (j < parent.obj_val.len) : (j += 1) {
                if (std.mem.eql(u8, parent.obj_val[j].key, leaf)) {
                    parent.obj_val[j].value = cloned_val;
                    return;
                }
            }
            const new_len = parent.obj_val.len + 1;
            let new_fields = allocator.alloc(JsonField, new_len)?;
            if (parent.obj_val.len > 0) {
                @memcpy(new_fields[0..parent.obj_val.len], parent.obj_val);
            }
            const owned_key = allocator.dupe(u8, leaf)?;
            new_fields[parent.obj_val.len] = JsonField{ key: owned_key, value: cloned_val };
            parent.obj_val = new_fields;
            return;
        }

        if (parent.tag == .Array) {
            let insert_idx: usize = 0;
            if (std.mem.eql(u8, leaf, "-")) {
                insert_idx = parent.arr_val.len;
            } else {
                const parsed_idx = JsonPointer.parseArrayIndex(leaf) ?? return err.InvalidPointer;
                insert_idx = parsed_idx;
                if (insert_idx > parent.arr_val.len) return err.IndexOutOfBounds;
            }
            const cloned_val = JsonPatch.cloneValue(allocator, val)?;
            const old_len = parent.arr_val.len;
            let new_arr = allocator.alloc(JsonValue, old_len + 1)?;
            if (insert_idx > 0) {
                @memcpy(new_arr[0..insert_idx], parent.arr_val[0..insert_idx]);
            }
            new_arr[insert_idx] = cloned_val;
            if (insert_idx < old_len) {
                @memcpy(new_arr[insert_idx + 1 .. old_len + 1], parent.arr_val[insert_idx..old_len]);
            }
            parent.arr_val = new_arr;
            return;
        }

        return err.InvalidTarget;
    }

    static fn applyRemove(allocator: std.mem.Allocator, root: *JsonValue, path: []const u8): JsonValue! {
        if (path.len == 0) return err.InvalidTarget;
        const tokens = JsonPointer.parseTokens(allocator, path)?;
        defer JsonPointer.freeTokens(allocator, tokens);

        const parent = JsonPatch.navigateParent(root, tokens)?;
        const leaf = tokens[tokens.len - 1];

        if (parent.tag == .Object) {
            let match_idx: usize? = null;
            let i: usize = 0;
            while (i < parent.obj_val.len) : (i += 1) {
                if (std.mem.eql(u8, parent.obj_val[i].key, leaf)) {
                    match_idx = i;
                    break;
                }
            }
            const idx = match_idx ?? return err.PathNotFound;
            const removed = parent.obj_val[idx].value;
            const old_len = parent.obj_val.len;
            let new_fields = allocator.alloc(JsonField, old_len - 1)?;
            if (idx > 0) {
                @memcpy(new_fields[0..idx], parent.obj_val[0..idx]);
            }
            if (idx + 1 < old_len) {
                @memcpy(new_fields[idx .. old_len - 1], parent.obj_val[idx + 1 .. old_len]);
            }
            parent.obj_val = new_fields;
            return removed;
        }

        if (parent.tag == .Array) {
            const idx = JsonPointer.parseArrayIndex(leaf) ?? return err.InvalidPointer;
            if (idx >= parent.arr_val.len) return err.IndexOutOfBounds;
            const removed = parent.arr_val[idx];
            const old_len = parent.arr_val.len;
            let new_arr = allocator.alloc(JsonValue, old_len - 1)?;
            if (idx > 0) {
                @memcpy(new_arr[0..idx], parent.arr_val[0..idx]);
            }
            if (idx + 1 < old_len) {
                @memcpy(new_arr[idx .. old_len - 1], parent.arr_val[idx + 1 .. old_len]);
            }
            parent.arr_val = new_arr;
            return removed;
        }

        return err.InvalidTarget;
    }

    static fn applyReplace(allocator: std.mem.Allocator, root: *JsonValue, path: []const u8, val: JsonValue): ! {
        if (path.len == 0) {
            root.* = JsonPatch.cloneValue(allocator, val)?;
            return;
        }
        const tokens = JsonPointer.parseTokens(allocator, path)?;
        defer JsonPointer.freeTokens(allocator, tokens);

        const parent = JsonPatch.navigateParent(root, tokens)?;
        const leaf = tokens[tokens.len - 1];

        if (parent.tag == .Object) {
            let j: usize = 0;
            while (j < parent.obj_val.len) : (j += 1) {
                if (std.mem.eql(u8, parent.obj_val[j].key, leaf)) {
                    parent.obj_val[j].value = JsonPatch.cloneValue(allocator, val)?;
                    return;
                }
            }
            return err.PathNotFound;
        }

        if (parent.tag == .Array) {
            const idx = JsonPointer.parseArrayIndex(leaf) ?? return err.InvalidPointer;
            if (idx >= parent.arr_val.len) return err.IndexOutOfBounds;
            parent.arr_val[idx] = JsonPatch.cloneValue(allocator, val)?;
            return;
        }

        return err.InvalidTarget;
    }

    static fn isProperPrefix(src_path: []const u8, path: []const u8): bool {
        if (src_path.len >= path.len) return false;
        if (!std.mem.startsWith(u8, path, src_path)) return false;
        return path[src_path.len] == '/';
    }

    /// Parses an RFC 6902 JSON array of patch operations from a `JsonValue`.
    pub static fn parseOps(allocator: std.mem.Allocator, patch_doc: JsonValue): ([]JsonPatchOp)! {
        if (patch_doc.tag != .Array) return err.InvalidOperation;
        let ops = allocator.alloc(JsonPatchOp, patch_doc.arr_val.len)?;
        errdefer allocator.free(ops);

        let i: usize = 0;
        while (i < patch_doc.arr_val.len) : (i += 1) {
            const item = patch_doc.arr_val[i];
            if (item.tag != .Object) return err.InvalidOperation;
            const op_val = item.get("op") ?? return err.InvalidOperation;
            const op_str = op_val.asString() ?? return err.InvalidOperation;
            const path_val = item.get("path") ?? return err.InvalidOperation;
            const path_str = path_val.asString() ?? return err.InvalidPointer;

            if (std.mem.eql(u8, op_str, "add")) {
                const v = item.get("value") ?? return err.MissingValue;
                ops[i] = JsonPatchOp.add(path_str, v);
            } else if (std.mem.eql(u8, op_str, "remove")) {
                ops[i] = JsonPatchOp.remove(path_str);
            } else if (std.mem.eql(u8, op_str, "replace")) {
                const v = item.get("value") ?? return err.MissingValue;
                ops[i] = JsonPatchOp.replaceOp(path_str, v);
            } else if (std.mem.eql(u8, op_str, "move")) {
                const f_val = item.get("from") ?? return err.MissingFrom;
                const f_str = f_val.asString() ?? return err.InvalidPointer;
                ops[i] = JsonPatchOp.move(f_str, path_str);
            } else if (std.mem.eql(u8, op_str, "copy")) {
                const f_val = item.get("from") ?? return err.MissingFrom;
                const f_str = f_val.asString() ?? return err.InvalidPointer;
                ops[i] = JsonPatchOp.copy(f_str, path_str);
            } else if (std.mem.eql(u8, op_str, "test")) {
                const v = item.get("value") ?? return err.MissingValue;
                ops[i] = JsonPatchOp.testValue(path_str, v);
            } else {
                return err.InvalidOperation;
            }
        }
        return ops;
    }

    /// Atomically applies a sequence of RFC 6902 patch operations to a deep clone of `doc`.
    /// On any failure (e.g. `test` mismatch or missing path), returns an error without mutating `doc`.
    pub static fn apply(
        allocator: std.mem.Allocator,
        doc: JsonValue,
        ops: []const JsonPatchOp,
    ): JsonValue! {
        let cur = JsonPatch.cloneValue(allocator, doc)?;
        for (op of ops) {
            switch (op.op) {
                .Add => {
                    const val = op.value ?? return err.MissingValue;
                    JsonPatch.applyAdd(allocator, &cur, op.path, val)?;
                },
                .Remove => {
                    * = JsonPatch.applyRemove(allocator, &cur, op.path)?;
                },
                .Replace => {
                    const val = op.value ?? return err.MissingValue;
                    JsonPatch.applyReplace(allocator, &cur, op.path, val)?;
                },
                .Move => {
                    const from_ptr = op.from_path ?? return err.MissingFrom;
                    if (JsonPatch.isProperPrefix(from_ptr, op.path)) {
                        return err.CannotMoveIntoChild;
                    }
                    if (std.mem.eql(u8, from_ptr, op.path)) {
                        * = JsonPointer.resolve(cur, from_ptr) ?? return err.PathNotFound;
                        continue;
                    }
                    const moved = JsonPatch.applyRemove(allocator, &cur, from_ptr)?;
                    JsonPatch.applyAdd(allocator, &cur, op.path, moved)?;
                },
                .Copy => {
                    const from_ptr = op.from_path ?? return err.MissingFrom;
                    const src_val = JsonPointer.resolve(cur, from_ptr) ?? return err.PathNotFound;
                    JsonPatch.applyAdd(allocator, &cur, op.path, src_val)?;
                },
                .Test => {
                    const expected = op.value ?? return err.MissingValue;
                    const actual = JsonPointer.resolve(cur, op.path) ?? return err.PathNotFound;
                    if (!JsonPatch.deepEqual(actual, expected)) {
                        return err.TestFailed;
                    }
                },
            }
        }
        return cur;
    }

    /// Convenience helper: parses an RFC 6902 patch `JsonValue` array and applies it atomically.
    pub static fn applyValue(
        allocator: std.mem.Allocator,
        doc: JsonValue,
        patch_doc: JsonValue,
    ): JsonValue! {
        const ops = JsonPatch.parseOps(allocator, patch_doc)?;
        defer allocator.free(ops);
        return JsonPatch.apply(allocator, doc, ops);
    }
}

pub struct JsonMergePatch {
    /// Applies an RFC 7396 JSON Merge Patch recursively.
    /// When `patch` is an Object, `null` values delete target keys and non-null values merge recursively.
    /// Non-object patches replace `target` wholesale. Returns a freshly cloned `JsonValue`.
    pub static fn apply(
        allocator: std.mem.Allocator,
        target: JsonValue,
        patch: JsonValue,
    ): JsonValue! {
        if (patch.tag != .Object) {
            return JsonPatch.cloneValue(allocator, patch);
        }

        let base_fields: std.ArrayList(JsonField) = .empty;
        errdefer base_fields.deinit(allocator);

        if (target.tag == .Object) {
            for (f of target.obj_val) {
                const k = allocator.dupe(u8, f.key)?;
                const v = JsonPatch.cloneValue(allocator, f.value)?;
                base_fields.append(allocator, JsonField{ key: k, value: v })?;
            }
        }

        for (pf of patch.obj_val) {
            let found_idx: usize? = null;
            let i: usize = 0;
            while (i < base_fields.items.len) : (i += 1) {
                if (std.mem.eql(u8, base_fields.items[i].key, pf.key)) {
                    found_idx = i;
                    break;
                }
            }

            if (pf.value.tag == .Null) {
                if (idx := found_idx) {
                    * = base_fields.orderedRemove(idx);
                }
            } else {
                if (idx := found_idx) {
                    const merged = JsonMergePatch.apply(allocator, base_fields.items[idx].value, pf.value)?;
                    base_fields.items[idx].value = merged;
                } else {
                    const empty_target = JsonValue{ tag: .Null };
                    const merged = JsonMergePatch.apply(allocator, empty_target, pf.value)?;
                    const k = allocator.dupe(u8, pf.key)?;
                    base_fields.append(allocator, JsonField{ key: k, value: merged })?;
                }
            }
        }

        return JsonValue{
            tag: .Object,
            obj_val: base_fields.toOwnedSlice(allocator)?,
        };
    }
}
