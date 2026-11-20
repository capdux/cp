// Source: cp/lib/toml/value.cp (TOML v1.0.0 Value Model, Error Set & Document Tree)
// Pure CP TOML v1.0.0 data model with recursive ownership cleanup and path lookup.

const std = @import("std");

pub err TomlError {
    UnexpectedEof,
    UnexpectedChar,
    UnterminatedString,
    InvalidEscape,
    DuplicateKey,
    TypeConflict,
    InvalidNumber,
    InvalidDateTime,
    MaxDepthExceeded,
    OutOfMemory,
}

pub enum TomlKind {
    String,
    Integer,
    Float,
    Boolean,
    DateTime,
    Array,
    Table,
}

pub struct TomlEntry {
    key: []const u8 = "",
    value: TomlValue = .{},
    is_explicit_table: bool = false,
    is_dotted_table: bool = false,
    is_inline_table: bool = false,
    is_array_of_tables: bool = false,
}

pub struct TomlArray {
    allocator: std.mem.Allocator? = null,
    items: std.ArrayList(TomlValue) = .empty,

    static fn initEmpty(): TomlArray {
        return TomlArray{
            allocator: null,
            items: .empty,
        };
    }

    static fn init(alloc: std.mem.Allocator): TomlArray {
        return TomlArray{
            allocator: alloc,
            items: .empty,
        };
    }

    fn len(): usize {
        return _.items.items.len;
    }

    fn get(idx: usize): TomlValue? {
        if (idx >= _.items.items.len) return null;
        return _.items.items[idx];
    }

    fn getPtr(*_, idx: usize): *TomlValue!? {
        if (idx >= _.items.items.len) return null;
        return &_.items.items[idx];
    }

    fn push(*_, val: TomlValue): ! {
        const alloc = _.allocator orelse return TomlError.OutOfMemory;
        _.items.append(alloc, val)?;
    }

    fn pushWith(*_, alloc: std.mem.Allocator, val: TomlValue): ! {
        if (_.allocator == null) {
            _.allocator = alloc;
        }
        _.items.append(alloc, val)?;
    }

    fn deinit(*_) {
        if (alloc := _.allocator) {
            for (*item of _.items.items) {
                item.deinitWith(alloc);
            }
            _.items.deinit(alloc);
            _.allocator = null;
        }
    }

    fn deinitWith(*_, alloc: std.mem.Allocator) {
        for (*item of _.items.items) {
            item.deinitWith(alloc);
        }
        _.items.deinit(alloc);
        _.allocator = null;
    }
}

pub struct TomlTable {
    allocator: std.mem.Allocator? = null,
    entries: std.ArrayList(TomlEntry) = .empty,

    static fn initEmpty(): TomlTable {
        return TomlTable{
            allocator: null,
            entries: .empty,
        };
    }

    static fn init(alloc: std.mem.Allocator): TomlTable {
        return TomlTable{
            allocator: alloc,
            entries: .empty,
        };
    }

    fn len(): usize {
        return _.entries.items.len;
    }

    fn contains(key: []const u8): bool {
        for (entry of _.entries.items) {
            if (std.mem.eql(u8, entry.key, key)) return true;
        }
        return false;
    }

    fn get(key: []const u8): TomlValue? {
        for (entry of _.entries.items) {
            if (std.mem.eql(u8, entry.key, key)) return entry.value;
        }
        return null;
    }

    fn getEntryPtr(*_, key: []const u8): *TomlEntry!? {
        let i: usize = 0;
        while (i < _.entries.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.entries.items[i].key, key)) {
                return &_.entries.items[i];
            }
        }
        return null;
    }

    fn getEntryAtPtr(*_, idx: usize): *TomlEntry! {
        return &_.entries.items[idx];
    }

    fn getPath(path: []const u8): TomlValue? {
        if (path.len == 0) {
            return TomlValue.fromTable(TomlTable{
                allocator: _.allocator,
                entries: _.entries,
            });
        }

        let cur = TomlTable{
            allocator: _.allocator,
            entries: _.entries,
        };
        let pos: usize = 0;

        while (pos < path.len) {
            let end = pos;
            while (end < path.len && path[end] != '.') {
                end += 1;
            }
            const seg = path[pos..end];
            if (seg.len == 0) return null;

            const found = cur.get(seg) orelse return null;
            if (end >= path.len) {
                return found;
            }
            if (found.kind != .Table) return null;
            cur = found.table_val;
            pos = end + 1;
        }
        return null;
    }

    fn put(*_, key: []const u8, val: TomlValue): ! {
        const alloc = _.allocator orelse return TomlError.OutOfMemory;
        _.putWith(alloc, key, val)?;
    }

    fn putWith(*_, alloc: std.mem.Allocator, key: []const u8, val: TomlValue): ! {
        if (_.allocator == null) {
            _.allocator = alloc;
        }
        for (*entry of _.entries.items) {
            if (std.mem.eql(u8, entry.key, key)) {
                entry.value.deinitWith(alloc);
                entry.value = val;
                return;
            }
        }
        const key_copy = alloc.dupe(u8, key)?;
        errdefer alloc.free(key_copy);
        _.entries.append(alloc, TomlEntry{
            key: key_copy,
            value: val,
        })?;
    }

    fn appendEntryWith(*_, alloc: std.mem.Allocator, entry: TomlEntry): ! {
        if (_.allocator == null) {
            _.allocator = alloc;
        }
        _.entries.append(alloc, entry)?;
    }

    fn deinit(*_) {
        if (alloc := _.allocator) {
            _.deinitWith(alloc);
        }
    }

    fn deinitWith(*_, alloc: std.mem.Allocator) {
        for (*entry of _.entries.items) {
            if (entry.key.len > 0) {
                alloc.free(entry.key);
            }
            entry.value.deinitWith(alloc);
        }
        _.entries.deinit(alloc);
        _.allocator = null;
    }
}

pub struct TomlValue {
    kind: TomlKind = .Boolean,
    str_val: []const u8 = "",
    int_val: i64 = 0,
    float_val: f64 = 0.0,
    bool_val: bool = false,
    arr_val: TomlArray = TomlArray.initEmpty(),
    table_val: TomlTable = TomlTable.initEmpty(),

    static fn fromString(alloc: std.mem.Allocator, s: []const u8): TomlValue! {
        const copy = alloc.dupe(u8, s)?;
        return TomlValue{
            kind: .String,
            str_val: copy,
        };
    }

    static fn fromStringOwned(s: []const u8): TomlValue {
        return TomlValue{
            kind: .String,
            str_val: s,
        };
    }

    static fn fromInt(v: i64): TomlValue {
        return TomlValue{
            kind: .Integer,
            int_val: v,
        };
    }

    static fn fromFloat(v: f64): TomlValue {
        return TomlValue{
            kind: .Float,
            float_val: v,
        };
    }

    static fn fromBool(v: bool): TomlValue {
        return TomlValue{
            kind: .Boolean,
            bool_val: v,
        };
    }

    static fn fromDateTime(alloc: std.mem.Allocator, s: []const u8): TomlValue! {
        const copy = alloc.dupe(u8, s)?;
        return TomlValue{
            kind: .DateTime,
            str_val: copy,
        };
    }

    static fn fromDateTimeOwned(s: []const u8): TomlValue {
        return TomlValue{
            kind: .DateTime,
            str_val: s,
        };
    }

    static fn fromArray(arr: TomlArray): TomlValue {
        return TomlValue{
            kind: .Array,
            arr_val: arr,
        };
    }

    static fn fromTable(tbl: TomlTable): TomlValue {
        return TomlValue{
            kind: .Table,
            table_val: tbl,
        };
    }

    fn isString(): bool { return _.kind == .String; }
    fn isInt(): bool { return _.kind == .Integer; }
    fn isFloat(): bool { return _.kind == .Float; }
    fn isBool(): bool { return _.kind == .Boolean; }
    fn isDateTime(): bool { return _.kind == .DateTime; }
    fn isArray(): bool { return _.kind == .Array; }
    fn isTable(): bool { return _.kind == .Table; }

    fn asString(): ([]const u8)? {
        if (_.kind == .String) return _.str_val;
        return null;
    }

    fn asInt(): i64? {
        if (_.kind == .Integer) return _.int_val;
        return null;
    }

    fn asFloat(): f64? {
        if (_.kind == .Float) return _.float_val;
        return null;
    }

    fn asBool(): bool? {
        if (_.kind == .Boolean) return _.bool_val;
        return null;
    }

    fn asDateTime(): ([]const u8)? {
        if (_.kind == .DateTime) return _.str_val;
        return null;
    }

    fn asArray(): TomlArray? {
        if (_.kind == .Array) return _.arr_val;
        return null;
    }

    fn asTable(): TomlTable? {
        if (_.kind == .Table) return _.table_val;
        return null;
    }

    fn get(key: []const u8): TomlValue? {
        if (_.kind != .Table) return null;
        return _.table_val.get(key);
    }

    fn getPath(path: []const u8): TomlValue? {
        if (_.kind != .Table) return null;
        return _.table_val.getPath(path);
    }

    fn asTablePtr(*_): *TomlTable! {
        return &_.table_val;
    }

    fn asArrayPtr(*_): *TomlArray! {
        return &_.arr_val;
    }

    fn deinit(*_, alloc: std.mem.Allocator) {
        _.deinitWith(alloc);
    }

    fn deinitWith(*_, alloc: std.mem.Allocator) {
        switch (_.kind) {
            .String | .DateTime => {
                if (_.str_val.len > 0) {
                    alloc.free(_.str_val);
                    _.str_val = "";
                }
            },
            .Array => {
                _.arr_val.deinitWith(alloc);
            },
            .Table => {
                _.table_val.deinitWith(alloc);
            },
            else => {},
        }
    }
}

pub struct TomlDoc {
    allocator: std.mem.Allocator,
    root: TomlTable,

    static fn init(alloc: std.mem.Allocator): TomlDoc {
        return TomlDoc{
            allocator: alloc,
            root: TomlTable.init(alloc),
        };
    }

    fn asTable(): TomlTable {
        return _.root;
    }

    fn get(key: []const u8): TomlValue? {
        return _.root.get(key);
    }

    fn getPath(path: []const u8): TomlValue? {
        return _.root.getPath(path);
    }

    fn deinit(*_) {
        _.root.deinitWith(_.allocator);
    }
}
