// Source: cp/lib/interchange/value.cp (Unified Interchange Data Model, Format Detection & Error Set)
// Pure CP unified interchange DOM, format enum, error set, and deterministic RAII cleanup.

const std = @import("std");

pub const INTERCHANGE_MAX_DEPTH: usize = 64;

pub enum InterchangeFormat {
    Json,
    Jsonc,
    Json5,
    Toml,
    Yaml,
}

pub err InterchangeError {
    UnsupportedFormat,
    UnexpectedEof,
    UnexpectedChar,
    UnexpectedToken,
    UnterminatedString,
    UnterminatedComment,
    InvalidEscape,
    InvalidNumber,
    DuplicateKey,
    TypeConflict,
    InvalidIndentation,
    TabIndentationForbidden,
    MaxDepthExceeded,
    TrailingData,
    OutOfMemory,
}

pub enum InterchangeKind {
    Null,
    Bool,
    Int,
    Float,
    String,
    Array,
    Object,
}

pub struct InterchangeEntry {
    key: []const u8 = "",
    value: InterchangeValue = .{},
}

pub struct InterchangeValue {
    kind: InterchangeKind = .Null,
    bool_val: bool = false,
    int_val: i64 = 0,
    float_val: f64 = 0.0,
    str_val: []const u8 = "",
    arr_val: []InterchangeValue = [],
    obj_val: []InterchangeEntry = [],

    pub static fn initNull(): InterchangeValue {
        return InterchangeValue{ kind: .Null };
    }

    pub static fn fromBool(v: bool): InterchangeValue {
        return InterchangeValue{ kind: .Bool, bool_val: v };
    }

    pub static fn fromInt(v: i64): InterchangeValue {
        return InterchangeValue{ kind: .Int, int_val: v };
    }

    pub static fn fromFloat(v: f64): InterchangeValue {
        return InterchangeValue{ kind: .Float, float_val: v };
    }

    pub static fn fromString(alloc: std.mem.Allocator, s: []const u8): InterchangeValue! {
        if (s.len == 0) {
            return InterchangeValue{ kind: .String, str_val: "" };
        }
        const dup = alloc.dupe(u8, s) catch return InterchangeError.OutOfMemory;
        return InterchangeValue{ kind: .String, str_val: dup };
    }

    pub static fn fromArray(items: []InterchangeValue): InterchangeValue {
        return InterchangeValue{ kind: .Array, arr_val: items };
    }

    pub static fn fromObject(entries: []InterchangeEntry): InterchangeValue {
        return InterchangeValue{ kind: .Object, obj_val: entries };
    }

    pub fn isNull(): bool {
        return _.kind == .Null;
    }

    pub fn isBool(): bool {
        return _.kind == .Bool;
    }

    pub fn isInt(): bool {
        return _.kind == .Int;
    }

    pub fn isFloat(): bool {
        return _.kind == .Float;
    }

    pub fn isNumber(): bool {
        return _.kind == .Int || _.kind == .Float;
    }

    pub fn isString(): bool {
        return _.kind == .String;
    }

    pub fn isArray(): bool {
        return _.kind == .Array;
    }

    pub fn isObject(): bool {
        return _.kind == .Object;
    }

    pub fn asBool(): bool? {
        if (_.kind == .Bool) return _.bool_val;
        return null;
    }

    pub fn asInt(): i64? {
        if (_.kind == .Int) return _.int_val;
        if (_.kind == .Float) {
            const f = _.float_val;
            if (std.math.isNan(f) || std.math.isInf(f)) return null;
            if (f >= -9223372036854775808.0 && f < 9223372036854775808.0) {
                const truncated = @trunc(f);
                if (truncated == f) {
                    return @intFromFloat(truncated);
                }
            }
        }
        return null;
    }

    pub fn asFloat(): f64? {
        if (_.kind == .Float) return _.float_val;
        if (_.kind == .Int) return @as(f64, @floatFromInt(_.int_val));
        return null;
    }

    pub fn asString(): ([]const u8)? {
        if (_.kind == .String) return _.str_val;
        return null;
    }

    pub fn asArray(): ([]InterchangeValue)? {
        if (_.kind == .Array) return _.arr_val;
        return null;
    }

    pub fn asObject(): ([]InterchangeEntry)? {
        if (_.kind == .Object) return _.obj_val;
        return null;
    }

    pub fn len(): usize {
        return switch (_.kind) {
            .Array => _.arr_val.len,
            .Object => _.obj_val.len,
            .String => _.str_val.len,
            else => 0,
        };
    }

    pub fn get(key: []const u8): InterchangeValue? {
        if (_.kind != .Object) return null;
        for (entry of _.obj_val) {
            if (std.mem.eql(u8, entry.key, key)) return entry.value;
        }
        return null;
    }

    pub fn getIndex(idx: usize): InterchangeValue? {
        if (_.kind != .Array || idx >= _.arr_val.len) return null;
        return _.arr_val[idx];
    }

    pub fn getPath(path: []const u8): InterchangeValue? {
        if (path.len == 0) return _;
        let cur: InterchangeValue = _;
        let pos: usize = 0;
        while (pos <= path.len) {
            let next_dot = pos;
            while (next_dot < path.len && path[next_dot] != '.') {
                next_dot += 1;
            }
            const seg = path[pos..next_dot];
            if (cur.kind == .Object) {
                if (next_v := cur.get(seg)) {
                    cur = next_v;
                } else {
                    return null;
                }
            } else if (cur.kind == .Array) {
                const idx = std.fmt.parseInt(usize, seg, 10) catch return null;
                if (next_v := cur.getIndex(idx)) {
                    cur = next_v;
                } else {
                    return null;
                }
            } else {
                return null;
            }
            if (next_dot >= path.len) break;
            pos = next_dot + 1;
        }
        return cur;
    }

    pub fn clone(alloc: std.mem.Allocator): InterchangeValue! {
        return _.cloneDepth(alloc, 0);
    }

    fn cloneDepth(alloc: std.mem.Allocator, depth: usize): InterchangeValue! {
        if (depth > INTERCHANGE_MAX_DEPTH) return InterchangeError.MaxDepthExceeded;
        switch (_.kind) {
            .Null => return InterchangeValue.initNull(),
            .Bool => return InterchangeValue.fromBool(_.bool_val),
            .Int => return InterchangeValue.fromInt(_.int_val),
            .Float => return InterchangeValue.fromFloat(_.float_val),
            .String => return InterchangeValue.fromString(alloc, _.str_val),
            .Array => {
                if (_.arr_val.len == 0) return InterchangeValue.fromArray([]);
                let out = alloc.alloc(InterchangeValue, _.arr_val.len) catch return InterchangeError.OutOfMemory;
                let filled: usize = 0;
                errdefer {
                    let j: usize = 0;
                    while (j < filled) : (j += 1) {
                        out[j].deinit(alloc);
                    }
                    alloc.free(out);
                }
                while (filled < _.arr_val.len) : (filled += 1) {
                    out[filled] = _.arr_val[filled].cloneDepth(alloc, depth + 1)?;
                }
                return InterchangeValue.fromArray(out);
            },
            .Object => {
                if (_.obj_val.len == 0) return InterchangeValue.fromObject([]);
                let out = alloc.alloc(InterchangeEntry, _.obj_val.len) catch return InterchangeError.OutOfMemory;
                let filled: usize = 0;
                errdefer {
                    let j: usize = 0;
                    while (j < filled) : (j += 1) {
                        if (out[j].key.len > 0) alloc.free(out[j].key);
                        out[j].value.deinit(alloc);
                    }
                    alloc.free(out);
                }
                while (filled < _.obj_val.len) : (filled += 1) {
                    const src_entry = _.obj_val[filled];
                    const dup_key: []const u8 = if (src_entry.key.len > 0)
                        (alloc.dupe(u8, src_entry.key) catch return InterchangeError.OutOfMemory)
                    else
                        "";
                    errdefer {
                        if (dup_key.len > 0) alloc.free(dup_key);
                    }
                    const dup_val = src_entry.value.cloneDepth(alloc, depth + 1)?;
                    out[filled] = InterchangeEntry{ key: dup_key, value: dup_val };
                }
                return InterchangeValue.fromObject(out);
            },
        }
    }

    pub fn equals(other: InterchangeValue): bool {
        if (_.kind != other.kind) {
            if (_.isNumber() && other.isNumber()) {
                const a = _.asFloat() orelse return false;
                const b = other.asFloat() orelse return false;
                if (std.math.isNan(a) && std.math.isNan(b)) return true;
                return a == b;
            }
            return false;
        }
        return switch (_.kind) {
            .Null => true,
            .Bool => _.bool_val == other.bool_val,
            .Int => _.int_val == other.int_val,
            .Float => {
                if (std.math.isNan(_.float_val) && std.math.isNan(other.float_val)) return true;
                return _.float_val == other.float_val;
            },
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
                for (entry of _.obj_val) {
                    if (rhs := other.get(entry.key)) {
                        if (!entry.value.equals(rhs)) return false;
                    } else {
                        return false;
                    }
                }
                return true;
            },
        };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.kind) {
            .String => {
                if (_.str_val.len > 0) {
                    alloc.free(_.str_val);
                    _.str_val = "";
                }
            },
            .Array => {
                for (*item of _.arr_val) {
                    item.deinit(alloc);
                }
                if (_.arr_val.len > 0) {
                    alloc.free(_.arr_val);
                    _.arr_val = [];
                }
            },
            .Object => {
                for (*entry of _.obj_val) {
                    if (entry.key.len > 0) {
                        alloc.free(entry.key);
                    }
                    entry.value.deinit(alloc);
                }
                if (_.obj_val.len > 0) {
                    alloc.free(_.obj_val);
                    _.obj_val = [];
                }
            },
            else => {},
        }
        _.kind = .Null;
    }
}
