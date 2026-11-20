// Source: cp/lib/yaml/value.cp (YAML 1.2 Value Representation, Path Query & Document)
// Pure CP YAML 1.2 DOM types with recursive RAII cleanup, dot-path queries, and safe type accessors.

const std = @import("std");

pub err YamlError {
    UnexpectedEof,
    InvalidIndentation,
    TabIndentationForbidden,
    DuplicateKey,
    UnterminatedString,
    UnterminatedFlow,
    InvalidAlias,
    MaxDepthExceeded,
    OutOfMemory,
}

pub enum YamlKind {
    Null,
    Bool,
    Int,
    Float,
    String,
    Sequence,
    Mapping,
}

pub struct YamlEntry {
    key: []const u8 = "",
    value: YamlValue = .{},
}

pub type YamlMapping = []YamlEntry;
pub type YamlSequence = []YamlValue;

const MAX_CLONE_DEPTH: usize = 64;

pub struct YamlValue {
    kind: YamlKind = .Null,
    bool_val: bool = false,
    int_val: i64 = 0,
    float_val: f64 = 0.0,
    str_val: []const u8 = "",
    seq_val: []YamlValue = [],
    map_val: []YamlEntry = [],

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

    pub fn isString(): bool {
        return _.kind == .String;
    }

    pub fn isSeq(): bool {
        return _.kind == .Sequence;
    }

    pub fn isMap(): bool {
        return _.kind == .Mapping;
    }

    pub fn asBool(): bool? {
        if (_.kind == .Bool) return _.bool_val;
        return null;
    }

    pub fn asInt(): i64? {
        if (_.kind == .Int) return _.int_val;
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

    pub fn asSeq(): ([]YamlValue)? {
        if (_.kind == .Sequence) return _.seq_val;
        return null;
    }

    pub fn asMap(): ([]YamlEntry)? {
        if (_.kind == .Mapping) return _.map_val;
        return null;
    }

    pub fn len(): usize {
        return switch (_.kind) {
            .Sequence => _.seq_val.len,
            .Mapping => _.map_val.len,
            .String => _.str_val.len,
            else => 0,
        };
    }

    pub fn get(key: []const u8): YamlValue? {
        if (_.kind != .Mapping) return null;
        for (entry of _.map_val) {
            if (std.mem.eql(u8, entry.key, key)) return entry.value;
        }
        return null;
    }

    pub fn getIndex(idx: usize): YamlValue? {
        if (_.kind != .Sequence || idx >= _.seq_val.len) return null;
        return _.seq_val[idx];
    }

    static fn parseSegmentIndex(seg: []const u8): usize? {
        if (seg.len == 0) return null;
        let acc: usize = 0;
        for (c of seg) {
            if (c < '0' || c > '9') return null;
            acc = acc * 10 + ((c - '0') as usize);
        }
        return acc;
    }

    /// Traverses a dot-separated path such as `"jobs.build.runs-on"` or `"steps.0.name"`.
    pub fn getPath(path: []const u8): YamlValue? {
        let cur = YamlValue{
            kind: _.kind,
            bool_val: _.bool_val,
            int_val: _.int_val,
            float_val: _.float_val,
            str_val: _.str_val,
            seq_val: _.seq_val,
            map_val: _.map_val,
        };
        if (path.len == 0) return cur;

        let pos: usize = 0;
        while (pos <= path.len) {
            let end = pos;
            while (end < path.len && path[end] != '.') {
                end += 1;
            }
            const seg = path[pos..end];
            if (cur.kind == .Mapping) {
                if (next_val := cur.get(seg)) {
                    cur = next_val;
                } else {
                    return null;
                }
            } else if (cur.kind == .Sequence) {
                if (idx := YamlValue.parseSegmentIndex(seg)) {
                    if (next_val := cur.getIndex(idx)) {
                        cur = next_val;
                    } else {
                        return null;
                    }
                } else {
                    return null;
                }
            } else {
                return null;
            }
            if (end >= path.len) break;
            pos = end + 1;
        }
        return cur;
    }

    pub fn clone(alloc: std.mem.Allocator): YamlValue! {
        return _.cloneWithDepth(alloc, 0);
    }

    fn cloneWithDepth(alloc: std.mem.Allocator, depth: usize): YamlValue! {
        if (depth >= MAX_CLONE_DEPTH) return YamlError.MaxDepthExceeded;
        switch (_.kind) {
            .Null => return YamlValue{ kind: .Null },
            .Bool => return YamlValue{ kind: .Bool, bool_val: _.bool_val },
            .Int => return YamlValue{ kind: .Int, int_val: _.int_val },
            .Float => return YamlValue{ kind: .Float, float_val: _.float_val },
            .String => {
                if (_.str_val.len == 0) {
                    return YamlValue{ kind: .String, str_val: "" };
                }
                const dup = alloc.dupe(u8, _.str_val) catch return YamlError.OutOfMemory;
                return YamlValue{ kind: .String, str_val: dup };
            },
            .Sequence => {
                if (_.seq_val.len == 0) {
                    return YamlValue{ kind: .Sequence, seq_val: [] };
                }
                let items = alloc.alloc(YamlValue, _.seq_val.len) catch return YamlError.OutOfMemory;
                let filled: usize = 0;
                errdefer {
                    let i: usize = 0;
                    while (i < filled) {
                        items[i].deinit(alloc);
                        i += 1;
                    }
                    alloc.free(items);
                }
                for (item of _.seq_val) {
                    items[filled] = item.cloneWithDepth(alloc, depth + 1)?;
                    filled += 1;
                }
                return YamlValue{ kind: .Sequence, seq_val: items };
            },
            .Mapping => {
                if (_.map_val.len == 0) {
                    return YamlValue{ kind: .Mapping, map_val: [] };
                }
                let entries = alloc.alloc(YamlEntry, _.map_val.len) catch return YamlError.OutOfMemory;
                let filled: usize = 0;
                errdefer {
                    let i: usize = 0;
                    while (i < filled) {
                        if (entries[i].key.len > 0) alloc.free(entries[i].key);
                        entries[i].value.deinit(alloc);
                        i += 1;
                    }
                    alloc.free(entries);
                }
                for (entry of _.map_val) {
                    let key_dup: []const u8 = "";
                    if (entry.key.len > 0) {
                        key_dup = alloc.dupe(u8, entry.key) catch return YamlError.OutOfMemory;
                    }
                    errdefer {
                        if (key_dup.len > 0) alloc.free(key_dup);
                    }
                    const val_dup = entry.value.cloneWithDepth(alloc, depth + 1)?;
                    entries[filled] = YamlEntry{ key: key_dup, value: val_dup };
                    filled += 1;
                }
                return YamlValue{ kind: .Mapping, map_val: entries };
            },
        }
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.kind) {
            .String => {
                if (_.str_val.len > 0) {
                    alloc.free(_.str_val);
                    _.str_val = "";
                }
            },
            .Sequence => {
                for (*item of _.seq_val) {
                    item.deinit(alloc);
                }
                if (_.seq_val.len > 0) {
                    alloc.free(_.seq_val);
                    _.seq_val = [];
                }
            },
            .Mapping => {
                for (*entry of _.map_val) {
                    if (entry.key.len > 0) {
                        alloc.free(entry.key);
                    }
                    entry.value.deinit(alloc);
                }
                if (_.map_val.len > 0) {
                    alloc.free(_.map_val);
                    _.map_val = [];
                }
            },
            else => {},
        }
        _.kind = .Null;
    }
}

pub struct YamlDoc {
    alloc: std.mem.Allocator,
    root: YamlValue = .{},
    documents: []YamlValue = [],

    pub static fn init(alloc: std.mem.Allocator): YamlDoc {
        return YamlDoc{
            alloc: alloc,
            root: YamlValue{ kind: .Null },
            documents: [],
        };
    }

    pub fn deinit(*_) {
        if (_.documents.len > 0) {
            for (*d of _.documents) {
                d.deinit(_.alloc);
            }
            _.alloc.free(_.documents);
            _.documents = [];
            _.root = YamlValue{ kind: .Null };
        } else {
            _.root.deinit(_.alloc);
        }
    }

    pub fn get(key: []const u8): YamlValue? {
        return _.root.get(key);
    }

    pub fn getIndex(idx: usize): YamlValue? {
        return _.root.getIndex(idx);
    }

    pub fn getPath(path: []const u8): YamlValue? {
        return _.root.getPath(path);
    }
}
