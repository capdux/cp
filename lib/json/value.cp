// Source: Absorbed from /data/dev/bun/src/interchange/json.cp | Synced: 2026-11-22T05:55:50Z
// Zero-copy JSON value representation, RFC 6901 JSON Pointer & Deep Equality in CP
// String keys and string values borrow directly from source slice when unescaped

const std = @import("std");

pub enum JsonTag {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object,
}

pub struct JsonField {
    key: []const u8 = "",
    value: JsonValue = .{},
}

pub struct JsonValue {
    tag: JsonTag = .Null,
    bool_val: bool = false,
    num_val: f64 = 0,
    str_val: []const u8 = "",
    arr_val: []JsonValue = [],
    obj_val: []JsonField = [],

    pub fn isNull(): bool {
        return _.tag == .Null;
    }

    pub fn isBool(): bool {
        return _.tag == .Bool;
    }

    pub fn isNumber(): bool {
        return _.tag == .Number;
    }

    pub fn isString(): bool {
        return _.tag == .String;
    }

    pub fn isArray(): bool {
        return _.tag == .Array;
    }

    pub fn isObject(): bool {
        return _.tag == .Object;
    }

    pub fn asBool(): bool? {
        if (_.tag == .Bool) return _.bool_val;
        return null;
    }

    pub fn asNumber(): f64? {
        if (_.tag == .Number) return _.num_val;
        return null;
    }

    pub fn asString(): ([]const u8)? {
        if (_.tag == .String) return _.str_val;
        return null;
    }

    pub fn asArray(): ([]JsonValue)? {
        if (_.tag == .Array) return _.arr_val;
        return null;
    }

    pub fn asObject(): ([]JsonField)? {
        if (_.tag == .Object) return _.obj_val;
        return null;
    }

    pub fn len(): usize {
        return switch (_.tag) {
            .Array => _.arr_val.len,
            .Object => _.obj_val.len,
            .String => _.str_val.len,
            else => 0,
        };
    }

    pub fn get(key: []const u8): JsonValue? {
        if (_.tag != .Object) return null;
        for (field of _.obj_val) {
            if (std.mem.eql(u8, field.key, key)) return field.value;
        }
        return null;
    }

    pub fn getIndex(idx: usize): JsonValue? {
        if (_.tag != .Array || idx >= _.arr_val.len) return null;
        return _.arr_val[idx];
    }

    static fn equalsPointerToken(key: []const u8, escaped_token: []const u8): bool {
        let ki: usize = 0;
        let ti: usize = 0;
        while (ti < escaped_token.len) {
            if (ki >= key.len) return false;
            let expected = escaped_token[ti];
            if (expected == '~') {
                if (ti + 1 >= escaped_token.len) return false;
                const esc = escaped_token[ti + 1];
                if (esc == '0') {
                    expected = '~';
                } else if (esc == '1') {
                    expected = '/';
                } else {
                    return false;
                }
                ti += 2;
            } else {
                ti += 1;
            }
            if (key[ki] != expected) return false;
            ki += 1;
        }
        return ki == key.len;
    }

    static fn isValidPointerToken(escaped_token: []const u8): bool {
        let i: usize = 0;
        while (i < escaped_token.len) {
            if (escaped_token[i] == '~') {
                if (i + 1 >= escaped_token.len) return false;
                const esc = escaped_token[i + 1];
                if (esc != '0' && esc != '1') return false;
                i += 2;
            } else {
                i += 1;
            }
        }
        return true;
    }

    static fn parsePointerArrayIndex(token: []const u8): usize? {
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

    /// Evaluates an RFC 6901 JSON Pointer (e.g. `""`, `"/users/0/name"`, `"/a~1b/c~0d"`).
    /// Returns `null` if the pointer syntax is invalid or the target path does not exist.
    pub fn pointer(ptr: []const u8): JsonValue? {
        let cur = JsonValue{
            tag: _.tag,
            bool_val: _.bool_val,
            num_val: _.num_val,
            str_val: _.str_val,
            arr_val: _.arr_val,
            obj_val: _.obj_val,
        };
        if (ptr.len == 0) return cur;
        if (ptr[0] != '/') return null;

        let pos: usize = 1;
        while (true) {
            let next_slash = pos;
            while (next_slash < ptr.len && ptr[next_slash] != '/') {
                next_slash += 1;
            }
            const raw_tok = ptr[pos..next_slash];
            if (!JsonValue.isValidPointerToken(raw_tok)) return null;

            if (cur.tag == .Object) {
                let found: JsonValue? = null;
                for (field of cur.obj_val) {
                    if (JsonValue.equalsPointerToken(field.key, raw_tok)) {
                        found = field.value;
                        break;
                    }
                }
                if (next_val := found) {
                    cur = next_val;
                } else {
                    return null;
                }
            } else if (cur.tag == .Array) {
                const idx = JsonValue.parsePointerArrayIndex(raw_tok) ?? return null;
                if (idx >= cur.arr_val.len) return null;
                cur = cur.arr_val[idx];
            } else {
                return null;
            }

            if (next_slash >= ptr.len) break;
            pos = next_slash + 1;
        }
        return cur;
    }

    /// Deep structural equality check across two `JsonValue` trees (object key-order independent).
    pub fn equals(other: JsonValue): bool {
        if (_.tag != other.tag) return false;
        return switch (_.tag) {
            .Null => true,
            .Bool => _.bool_val == other.bool_val,
            .Number => _.num_val == other.num_val,
            .String => std.mem.eql(u8, _.str_val, other.str_val),
            .Array => {
                if (_.arr_val.len != other.arr_val.len) return false;
                let i: usize = 0;
                while (i < _.arr_val.len) : (i += 1) {
                    if (!_.arr_val[i].equals(other.arr_val[i])) return false;
                }
                return true;
            },
            .Object => {
                if (_.obj_val.len != other.obj_val.len) return false;
                for (f of _.obj_val) {
                    const rhs_val = other.get(f.key) ?? return false;
                    if (!f.value.equals(rhs_val)) return false;
                }
                for (f of other.obj_val) {
                    if (_.get(f.key) == null) return false;
                }
                return true;
            },
        };
    }
}
