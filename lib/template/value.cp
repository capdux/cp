// Source: cp/lib/template/value.cp (Template Context Value & Dot-Path Resolver)
// Pure CP Data Model & Mustache Context Stack for cap:template

const std = @import("std");

pub enum TemplateValueTag: u8 {
    Null = 0,
    Bool = 1,
    Int = 2,
    Float = 3,
    Str = 4,
    List = 5,
    Map = 6,
}

pub struct TemplateEntry {
    pub key: []const u8 = "",
    pub value: TemplateValue = TemplateValue{},

    pub static fn init(key: []const u8, value: TemplateValue): TemplateEntry {
        return TemplateEntry{
            key: key,
            value: value,
        };
    }
}

pub struct TemplateValue {
    pub tag: TemplateValueTag = TemplateValueTag.Null,
    pub bool_val: bool = false,
    pub int_val: i64 = 0,
    pub float_val: f64 = 0.0,
    pub str_val: []const u8 = "",
    pub list_val: []const TemplateValue = [],
    pub map_val: []const TemplateEntry = [],

    pub static fn fromNull(): TemplateValue {
        return TemplateValue{ tag: TemplateValueTag.Null };
    }

    pub static fn fromBool(v: bool): TemplateValue {
        return TemplateValue{ tag: TemplateValueTag.Bool, bool_val: v };
    }

    pub static fn fromInt(v: i64): TemplateValue {
        return TemplateValue{ tag: TemplateValueTag.Int, int_val: v };
    }

    pub static fn fromFloat(v: f64): TemplateValue {
        return TemplateValue{ tag: TemplateValueTag.Float, float_val: v };
    }

    pub static fn fromStr(s: []const u8): TemplateValue {
        return TemplateValue{ tag: TemplateValueTag.Str, str_val: s };
    }

    pub static fn fromList(items: []const TemplateValue): TemplateValue {
        return TemplateValue{ tag: TemplateValueTag.List, list_val: items };
    }

    pub static fn fromMap(entries: []const TemplateEntry): TemplateValue {
        return TemplateValue{ tag: TemplateValueTag.Map, map_val: entries };
    }

    pub static fn entry(key: []const u8, val: TemplateValue): TemplateEntry {
        return TemplateEntry{ key: key, value: val };
    }

    pub fn isNull(): bool { return _.tag == TemplateValueTag.Null; }
    pub fn isBool(): bool { return _.tag == TemplateValueTag.Bool; }
    pub fn isInt(): bool { return _.tag == TemplateValueTag.Int; }
    pub fn isFloat(): bool { return _.tag == TemplateValueTag.Float; }
    pub fn isStr(): bool { return _.tag == TemplateValueTag.Str; }
    pub fn isList(): bool { return _.tag == TemplateValueTag.List; }
    pub fn isMap(): bool { return _.tag == TemplateValueTag.Map; }

    pub fn asBool(): bool? {
        if (_.tag == TemplateValueTag.Bool) return _.bool_val;
        return null;
    }

    pub fn asInt(): i64? {
        if (_.tag == TemplateValueTag.Int) return _.int_val;
        return null;
    }

    pub fn asFloat(): f64? {
        if (_.tag == TemplateValueTag.Float) return _.float_val;
        if (_.tag == TemplateValueTag.Int) return _.int_val as f64;
        return null;
    }

    pub fn asStr(): ([]const u8)? {
        if (_.tag == TemplateValueTag.Str) return _.str_val;
        return null;
    }

    pub fn asList(): ([]const TemplateValue)? {
        if (_.tag == TemplateValueTag.List) return _.list_val;
        return null;
    }

    pub fn asMap(): ([]const TemplateEntry)? {
        if (_.tag == TemplateValueTag.Map) return _.map_val;
        return null;
    }

    /// Mustache truthiness:
    /// - Null => false
    /// - Bool => value
    /// - Int => != 0
    /// - Float => != 0.0 and not NaN
    /// - Str => len > 0
    /// - List => len > 0
    /// - Map => true
    pub fn isTruthy(): bool {
        return switch (_.tag) {
            .Null => false,
            .Bool => _.bool_val,
            .Int => _.int_val != 0,
            .Float => _.float_val != 0.0 && !std.math.isNan(_.float_val),
            .Str => _.str_val.len > 0,
            .List => _.list_val.len > 0,
            .Map => true,
        };
    }

    pub fn get(key: []const u8): TemplateValue? {
        if (_.tag != TemplateValueTag.Map) return null;
        let i: usize = _.map_val.len;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.map_val[i].key, key)) {
                return _.map_val[i].value;
            }
        }
        return null;
    }

    pub fn getIndex(idx: usize): TemplateValue? {
        if (_.tag != TemplateValueTag.List || idx >= _.list_val.len) return null;
        return _.list_val[idx];
    }

    pub static fn parseIndex(seg: []const u8): usize? {
        if (seg.len == 0) return null;
        let idx: usize = 0;
        for (c of seg) {
            if (c < '0' || c > '9') return null;
            idx = idx * 10 + ((c - '0') as usize);
        }
        return idx;
    }

    /// Resolves a dot-separated path (e.g. `user.profile.name`, `items.0.title`, or `.`)
    /// directly within this value.
    pub fn getPath(path: []const u8): TemplateValue? {
        if (path.len == 0 || std.mem.eql(u8, path, ".")) return _;
        let cur = _;
        let iter = std.mem.splitScalar(u8, path, '.');
        while (seg := iter.next()) {
            if (seg.len == 0) return null;
            if (cur.tag == TemplateValueTag.Map) {
                if (next_v := cur.get(seg)) {
                    cur = next_v;
                } else {
                    return null;
                }
            } else if (cur.tag == TemplateValueTag.List) {
                const idx = TemplateValue.parseIndex(seg) ?? return null;
                if (next_v := cur.getIndex(idx)) {
                    cur = next_v;
                } else {
                    return null;
                }
            } else {
                return null;
            }
        }
        return cur;
    }

    /// Appends the plain (unescaped) string representation of this value to `out`.
    pub fn appendPlain(allocator: std.mem.Allocator, out: *std.ArrayList(u8)): ! {
        switch (_.tag) {
            .Null => {},
            .Bool => {
                if (_.bool_val) {
                    out.appendSlice(allocator, "true")?;
                } else {
                    out.appendSlice(allocator, "false")?;
                }
            },
            .Int => {
                let buf: [64]u8 = undefined;
                const s = std.fmt.bufPrint(&buf, "{d}", .{ _.int_val }) catch "";
                out.appendSlice(allocator, s)?;
            },
            .Float => {
                let buf: [128]u8 = undefined;
                const s = std.fmt.bufPrint(&buf, "{d}", .{ _.float_val }) catch "";
                out.appendSlice(allocator, s)?;
            },
            .Str => {
                out.appendSlice(allocator, _.str_val)?;
            },
            .List => {},
            .Map => {},
        }
    }
}

pub struct TemplateContext {
    pub data: TemplateValue = TemplateValue{},
    pub parent: (*const TemplateContext)? = null,

    pub static fn init(data: TemplateValue): TemplateContext {
        return TemplateContext{
            data: data,
            parent: null,
        };
    }

    pub static fn initEmpty(): TemplateContext {
        return TemplateContext{
            data: TemplateValue.fromNull(),
            parent: null,
        };
    }

    pub static fn withParent(data: TemplateValue, parent: *const TemplateContext): TemplateContext {
        return TemplateContext{
            data: data,
            parent: parent,
        };
    }

    /// Resolves a variable name or dot-separated path against the context stack.
    /// - `.` returns the current context's `data` directly.
    /// - `a.b.c` searches up the context stack for `a`, and once found, resolves `b.c`
    ///   strictly within that value (standard Mustache scoping semantics).
    pub fn resolve(raw_path: []const u8): TemplateValue? {
        const path = std.mem.trim(u8, raw_path, " \t\r\n");
        if (path.len == 0) return null;
        if (std.mem.eql(u8, path, ".")) {
            return _.data;
        }

        let first = path;
        let rest: []const u8 = "";
        if (dot_idx := std.mem.indexOfScalar(u8, path, '.')) {
            first = path[0..dot_idx];
            rest = path[dot_idx + 1 ..];
            if (first.len == 0 || rest.len == 0) return null;
        }

        let cur: (*const TemplateContext)? = &_;
        while (frame := cur) {
            if (frame.data.tag == TemplateValueTag.Map) {
                if (hit := frame.data.get(first)) {
                    if (rest.len == 0) return hit;
                    return hit.getPath(rest);
                }
            } else if (frame.data.tag == TemplateValueTag.List) {
                if (idx := TemplateValue.parseIndex(first)) {
                    if (hit := frame.data.getIndex(idx)) {
                        if (rest.len == 0) return hit;
                        return hit.getPath(rest);
                    }
                }
            }
            cur = frame.parent;
        }
        return null;
    }

    pub fn has(path: []const u8): bool {
        return _.resolve(path) != null;
    }

    pub fn isTruthy(path: []const u8): bool {
        if (val := _.resolve(path)) {
            return val.isTruthy();
        }
        return false;
    }

    pub fn getStr(path: []const u8): ([]const u8)? {
        if (val := _.resolve(path)) {
            return val.asStr();
        }
        return null;
    }

    pub fn getInt(path: []const u8): i64? {
        if (val := _.resolve(path)) {
            return val.asInt();
        }
        return null;
    }

    pub fn getBool(path: []const u8): bool? {
        if (val := _.resolve(path)) {
            return val.asBool();
        }
        return null;
    }
}
