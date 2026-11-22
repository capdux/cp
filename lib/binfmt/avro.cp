// Source: cp/lib/avro/avro.cp (Apache Avro Schema-Driven Binary Codec & OCF Container)
// Pure CP implementation of Apache Avro 1.11.x schema parser, ZigZag + Varint
// binary datum codec, and Object Container File (OCF "Obj\x01") reader/writer.
// Zero external C/Bun/POSIX dependencies.

const std = @import("std");

pub const DEFAULT_MAX_DEPTH: usize = 64;
pub const OCF_MAGIC: [4]u8 = ['O', 'b', 'j', 1];

pub err AvroError {
    InvalidSchema,
    UnexpectedEof,
    VarintOverflow,
    SchemaMismatch,
    InvalidUnionIndex,
    InvalidSyncMarker,
    InvalidContainerHeader,
    MaxDepthExceeded,
    OutOfMemory,
}

// ---------------------------------------------------------------------------
// Schema AST
// ---------------------------------------------------------------------------

pub struct AvroFieldSchema {
    name: []const u8 = "",
    schema: AvroSchemaNode = .null_type,
}

pub struct AvroEnumSchema {
    name: []const u8 = "",
    symbols: []const []const u8,
}

pub struct AvroFixedSchema {
    name: []const u8 = "",
    size: usize = 0,
}

pub struct AvroRecordSchema {
    name: []const u8 = "",
    fields: []const AvroFieldSchema,
}

pub enum AvroSchemaNode {
    null_type,
    boolean_type,
    int_type,
    long_type,
    float_type,
    double_type,
    bytes_type,
    string_type,
    array_type: *const AvroSchemaNode,
    map_type: *const AvroSchemaNode,
    union_type: []const AvroSchemaNode,
    enum_type: AvroEnumSchema,
    fixed_type: AvroFixedSchema,
    record_type: AvroRecordSchema,
}

pub struct AvroSchema {
    root: AvroSchemaNode = .null_type,
    owned_arena: (*std.heap.ArenaAllocator)? = null,

    pub static fn parseSchema(allocator: std.mem.Allocator, schema_json: []const u8): AvroError!AvroSchema {
        let arena_ptr = allocator.create(std.heap.ArenaAllocator) catch return AvroError.OutOfMemory;
        arena_ptr.* = std.heap.ArenaAllocator.init(allocator);
        errdefer {
            arena_ptr.deinit();
            allocator.destroy(arena_ptr);
        }

        let parsed = std.json.parseFromSlice(std.json.Value, allocator, schema_json, .{}) catch e => {
            if (e == error.OutOfMemory) return AvroError.OutOfMemory;
            return AvroError.InvalidSchema;
        };
        defer parsed.deinit();

        const arena_alloc = arena_ptr.allocator();
        let named_types = std.StringHashMap(AvroSchemaNode).init(arena_alloc);
        const root_node = parseSchemaValue(arena_alloc, &named_types, parsed.value, "", 0)?;

        return AvroSchema{
            root: root_node,
            owned_arena: arena_ptr,
        };
    }

    pub fn deinit(*_) {
        if (arena_ptr := _.owned_arena) {
            const child_alloc = arena_ptr.child_allocator;
            arena_ptr.deinit();
            child_alloc.destroy(arena_ptr);
            _.owned_arena = null;
        }
    }
}

fn getObjField(obj: std.json.ObjectMap, key: []const u8): AvroError!std.json.Value {
    if (v := obj.get(key)) {
        return v;
    }
    return AvroError.InvalidSchema;
}

fn dupeStr(alloc: std.mem.Allocator, s: []const u8): AvroError![]const u8 {
    return alloc.dupe(u8, s) catch return AvroError.OutOfMemory;
}

fn resolveFullname(alloc: std.mem.Allocator, name: []const u8, namespace: []const u8): AvroError![]const u8 {
    if (std.mem.indexOfScalar(u8, name, '.') != null || namespace.len == 0) {
        return dupeStr(alloc, name);
    }
    let buf = alloc.alloc(u8, namespace.len + 1 + name.len) catch return AvroError.OutOfMemory;
    std.mem.copyForwards(u8, buf[0..namespace.len], namespace);
    buf[namespace.len] = '.';
    std.mem.copyForwards(u8, buf[namespace.len + 1 ..], name);
    return buf;
}

fn parsePrimitiveName(name: []const u8): AvroSchemaNode? {
    if (std.mem.eql(u8, name, "null")) return .null_type;
    if (std.mem.eql(u8, name, "boolean")) return .boolean_type;
    if (std.mem.eql(u8, name, "int")) return .int_type;
    if (std.mem.eql(u8, name, "long")) return .long_type;
    if (std.mem.eql(u8, name, "float")) return .float_type;
    if (std.mem.eql(u8, name, "double")) return .double_type;
    if (std.mem.eql(u8, name, "bytes")) return .bytes_type;
    if (std.mem.eql(u8, name, "string")) return .string_type;
    return null;
}

fn parseSchemaValue(
    alloc: std.mem.Allocator,
    named_types: *std.StringHashMap(AvroSchemaNode),
    val: std.json.Value,
    enclosing_ns: []const u8,
    depth: usize,
): AvroError!AvroSchemaNode {
    if (depth > DEFAULT_MAX_DEPTH) {
        return AvroError.MaxDepthExceeded;
    }

    switch (val) {
        .string => |s| {
            if (prim := parsePrimitiveName(s)) {
                return prim;
            }
            if (found := named_types.get(s)) {
                return found;
            }
            if (enclosing_ns.len > 0 && std.mem.indexOfScalar(u8, s, '.') == null) {
                const full = resolveFullname(alloc, s, enclosing_ns)?;
                if (found_ns := named_types.get(full)) {
                    return found_ns;
                }
            }
            return AvroError.InvalidSchema;
        },
        .array => |arr| {
            let branches = alloc.alloc(AvroSchemaNode, arr.items.len) catch return AvroError.OutOfMemory;
            let i: usize = 0;
            while (i < arr.items.len) : (i += 1) {
                const branch = parseSchemaValue(alloc, named_types, arr.items[i], enclosing_ns, depth + 1)?;
                switch (branch) {
                    .union_type => return AvroError.InvalidSchema,
                    else => {},
                }
                branches[i] = branch;
            }
            return AvroSchemaNode{ union_type: branches };
        },
        .object => |obj| {
            const type_val = getObjField(obj, "type")?;
            const type_str = switch (type_val) {
                .string => |s| s,
                else => return AvroError.InvalidSchema,
            };

            if (prim := parsePrimitiveName(type_str)) {
                return prim;
            }

            if (std.mem.eql(u8, type_str, "array")) {
                const items_val = getObjField(obj, "items")?;
                let item_ptr = alloc.create(AvroSchemaNode) catch return AvroError.OutOfMemory;
                item_ptr.* = parseSchemaValue(alloc, named_types, items_val, enclosing_ns, depth + 1)?;
                return AvroSchemaNode{ array_type: item_ptr };
            }

            if (std.mem.eql(u8, type_str, "map")) {
                const values_val = getObjField(obj, "values")?;
                let val_ptr = alloc.create(AvroSchemaNode) catch return AvroError.OutOfMemory;
                val_ptr.* = parseSchemaValue(alloc, named_types, values_val, enclosing_ns, depth + 1)?;
                return AvroSchemaNode{ map_type: val_ptr };
            }

            if (std.mem.eql(u8, type_str, "enum")) {
                const name_val = getObjField(obj, "name")?;
                const raw_name = switch (name_val) {
                    .string => |s| s,
                    else => return AvroError.InvalidSchema,
                };
                if (raw_name.len == 0) return AvroError.InvalidSchema;

                let ns = enclosing_ns;
                if (ns_val := obj.get("namespace")) {
                    ns = switch (ns_val) {
                        .string => |s| s,
                        else => return AvroError.InvalidSchema,
                    };
                }
                const fullname = resolveFullname(alloc, raw_name, ns)?;

                const syms_val = getObjField(obj, "symbols")?;
                const syms_arr = switch (syms_val) {
                    .array => |a| a,
                    else => return AvroError.InvalidSchema,
                };
                if (syms_arr.items.len == 0) return AvroError.InvalidSchema;

                let symbols = alloc.alloc([]u8, syms_arr.items.len) catch return AvroError.OutOfMemory;
                let i: usize = 0;
                while (i < syms_arr.items.len) : (i += 1) {
                    const sym_s = switch (syms_arr.items[i]) {
                        .string => |s| s,
                        else => return AvroError.InvalidSchema,
                    };
                    if (sym_s.len == 0) return AvroError.InvalidSchema;
                    let j: usize = 0;
                    while (j < i) : (j += 1) {
                        if (std.mem.eql(u8, symbols[j], sym_s)) {
                            return AvroError.InvalidSchema;
                        }
                    }
                    symbols[i] = alloc.dupe(u8, sym_s) catch return AvroError.OutOfMemory;
                }

                const node = AvroSchemaNode{
                    enum_type: AvroEnumSchema{
                        name: dupeStr(alloc, raw_name)?,
                        symbols: symbols,
                    },
                };
                named_types.put(fullname, node) catch return AvroError.OutOfMemory;
                return node;
            }

            if (std.mem.eql(u8, type_str, "fixed")) {
                const name_val = getObjField(obj, "name")?;
                const raw_name = switch (name_val) {
                    .string => |s| s,
                    else => return AvroError.InvalidSchema,
                };
                if (raw_name.len == 0) return AvroError.InvalidSchema;

                let ns = enclosing_ns;
                if (ns_val := obj.get("namespace")) {
                    ns = switch (ns_val) {
                        .string => |s| s,
                        else => return AvroError.InvalidSchema,
                    };
                }
                const fullname = resolveFullname(alloc, raw_name, ns)?;

                const size_val = getObjField(obj, "size")?;
                const size_int = switch (size_val) {
                    .integer => |n| n,
                    else => return AvroError.InvalidSchema,
                };
                if (size_int < 0) return AvroError.InvalidSchema;

                const node = AvroSchemaNode{
                    fixed_type: AvroFixedSchema{
                        name: dupeStr(alloc, raw_name)?,
                        size: size_int as usize,
                    },
                };
                named_types.put(fullname, node) catch return AvroError.OutOfMemory;
                return node;
            }

            if (std.mem.eql(u8, type_str, "record")) {
                const name_val = getObjField(obj, "name")?;
                const raw_name = switch (name_val) {
                    .string => |s| s,
                    else => return AvroError.InvalidSchema,
                };
                if (raw_name.len == 0) return AvroError.InvalidSchema;

                let ns = enclosing_ns;
                if (ns_val := obj.get("namespace")) {
                    ns = switch (ns_val) {
                        .string => |s| s,
                        else => return AvroError.InvalidSchema,
                    };
                }
                const fullname = resolveFullname(alloc, raw_name, ns)?;

                const fields_val = getObjField(obj, "fields")?;
                const fields_arr = switch (fields_val) {
                    .array => |a| a,
                    else => return AvroError.InvalidSchema,
                };

                let fields = alloc.alloc(AvroFieldSchema, fields_arr.items.len) catch return AvroError.OutOfMemory;
                let i: usize = 0;
                while (i < fields_arr.items.len) : (i += 1) {
                    const f_obj = switch (fields_arr.items[i]) {
                        .object => |fo| fo,
                        else => return AvroError.InvalidSchema,
                    };
                    const f_name_val = getObjField(f_obj, "name")?;
                    const f_name = switch (f_name_val) {
                        .string => |s| s,
                        else => return AvroError.InvalidSchema,
                    };
                    if (f_name.len == 0) return AvroError.InvalidSchema;

                    const f_type_val = getObjField(f_obj, "type")?;
                    const f_schema = parseSchemaValue(alloc, named_types, f_type_val, ns, depth + 1)?;
                    fields[i] = AvroFieldSchema{
                        name: dupeStr(alloc, f_name)?,
                        schema: f_schema,
                    };
                }

                const node = AvroSchemaNode{
                    record_type: AvroRecordSchema{
                        name: dupeStr(alloc, raw_name)?,
                        fields: fields,
                    },
                };
                named_types.put(fullname, node) catch return AvroError.OutOfMemory;
                return node;
            }

            return AvroError.InvalidSchema;
        },
        else => return AvroError.InvalidSchema,
    }
}

// ---------------------------------------------------------------------------
// Dynamic Value Tree
// ---------------------------------------------------------------------------

pub struct AvroMapEntry {
    key: []const u8 = "",
    value: AvroValue = .null_val,

    pub static fn init(key: []const u8, value: AvroValue): AvroMapEntry {
        return AvroMapEntry{ key: key, value: value };
    }
}

pub struct AvroRecordField {
    name: []const u8 = "",
    value: AvroValue = .null_val,

    pub static fn init(name: []const u8, value: AvroValue): AvroRecordField {
        return AvroRecordField{ name: name, value: value };
    }
}

pub struct AvroEnumValue {
    index: i32 = 0,
    symbol: []const u8 = "",

    pub static fn init(index: i32, symbol: []const u8): AvroEnumValue {
        return AvroEnumValue{ index: index, symbol: symbol };
    }
}

pub struct AvroUnionValue {
    index: usize = 0,
    value: *AvroValue,

    pub static fn init(index: usize, value: *AvroValue): AvroUnionValue {
        return AvroUnionValue{ index: index, value: value };
    }
}

pub enum AvroValue {
    null_val,
    boolean: bool,
    int: i32,
    long: i64,
    float: f32,
    double: f64,
    bytes: []const u8,
    string: []const u8,
    array: []AvroValue,
    map: []AvroMapEntry,
    union_val: AvroUnionValue,
    enum_val: AvroEnumValue,
    fixed: []const u8,
    record: []AvroRecordField,

    pub static fn makeNull(): AvroValue {
        return .null_val;
    }

    pub static fn makeBoolean(v: bool): AvroValue {
        return AvroValue{ boolean: v };
    }

    pub static fn makeInt(v: i32): AvroValue {
        return AvroValue{ int: v };
    }

    pub static fn makeLong(v: i64): AvroValue {
        return AvroValue{ long: v };
    }

    pub static fn makeFloat(v: f32): AvroValue {
        return AvroValue{ float: v };
    }

    pub static fn makeDouble(v: f64): AvroValue {
        return AvroValue{ double: v };
    }

    pub static fn makeBytes(v: []const u8): AvroValue {
        return AvroValue{ bytes: v };
    }

    pub static fn makeString(v: []const u8): AvroValue {
        return AvroValue{ string: v };
    }

    pub static fn makeArray(items: []AvroValue): AvroValue {
        return AvroValue{ array: items };
    }

    pub static fn makeMap(entries: []AvroMapEntry): AvroValue {
        return AvroValue{ map: entries };
    }

    pub static fn makeUnion(index: usize, value: *AvroValue): AvroValue {
        return AvroValue{ union_val: AvroUnionValue.init(index, value) };
    }

    pub static fn makeEnum(index: i32, symbol: []const u8): AvroValue {
        return AvroValue{ enum_val: AvroEnumValue.init(index, symbol) };
    }

    pub static fn makeFixed(data: []const u8): AvroValue {
        return AvroValue{ fixed: data };
    }

    pub static fn makeRecord(fields: []AvroRecordField): AvroValue {
        return AvroValue{ record: fields };
    }

    pub fn isNull(): bool {
        return switch (_) {
            .null_val => true,
            else => false,
        };
    }

    pub fn asBoolean(): bool? {
        return switch (_) {
            .boolean b => b,
            else => null,
        };
    }

    pub fn asInt(): i32? {
        return switch (_) {
            .int v => v,
            else => null,
        };
    }

    pub fn asLong(): i64? {
        return switch (_) {
            .long v => v,
            else => null,
        };
    }

    pub fn asFloat(): f32? {
        return switch (_) {
            .float v => v,
            else => null,
        };
    }

    pub fn asDouble(): f64? {
        return switch (_) {
            .double v => v,
            else => null,
        };
    }

    pub fn asBytes(): ([]const u8)? {
        return switch (_) {
            .bytes v => v,
            else => null,
        };
    }

    pub fn asString(): ([]const u8)? {
        return switch (_) {
            .string v => v,
            else => null,
        };
    }

    pub fn asArray(): ([]AvroValue)? {
        return switch (_) {
            .array v => v,
            else => null,
        };
    }

    pub fn asMap(): ([]AvroMapEntry)? {
        return switch (_) {
            .map v => v,
            else => null,
        };
    }

    pub fn asUnion(): AvroUnionValue? {
        return switch (_) {
            .union_val v => v,
            else => null,
        };
    }

    pub fn asEnum(): AvroEnumValue? {
        return switch (_) {
            .enum_val v => v,
            else => null,
        };
    }

    pub fn asFixed(): ([]const u8)? {
        return switch (_) {
            .fixed v => v,
            else => null,
        };
    }

    pub fn asRecord(): ([]AvroRecordField)? {
        return switch (_) {
            .record v => v,
            else => null,
        };
    }

    pub fn getField(field_name: []const u8): AvroValue? {
        return switch (_) {
            .record fields => {
                let i: usize = 0;
                while (i < fields.len) : (i += 1) {
                    if (std.mem.eql(u8, fields[i].name, field_name)) {
                        return fields[i].value;
                    }
                }
                return null;
            },
            else => null,
        };
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        switch (_.*) {
            .bytes => |b| allocator.free(b),
            .string => |s| allocator.free(s),
            .fixed => |f| allocator.free(f),
            .enum_val => |e| allocator.free(e.symbol),
            .array => |items| {
                let i: usize = 0;
                while (i < items.len) : (i += 1) {
                    items[i].deinit(allocator);
                }
                allocator.free(items);
            },
            .map => |entries| {
                let i: usize = 0;
                while (i < entries.len) : (i += 1) {
                    allocator.free(entries[i].key);
                    entries[i].value.deinit(allocator);
                }
                allocator.free(entries);
            },
            .union_val => |u| {
                u.value.deinit(allocator);
                allocator.destroy(u.value);
            },
            .record => |fields| {
                let i: usize = 0;
                while (i < fields.len) : (i += 1) {
                    allocator.free(fields[i].name);
                    fields[i].value.deinit(allocator);
                }
                allocator.free(fields);
            },
            else => {},
        }
    }

    pub fn eql(other: AvroValue): bool {
        switch (_) {
            .null_val => return switch (other) { .null_val => true, else => false },
            .boolean => |a| return switch (other) { .boolean => |b| a == b, else => false },
            .int => |a| return switch (other) { .int => |b| a == b, else => false },
            .long => |a| return switch (other) { .long => |b| a == b, else => false },
            .float => |a| return switch (other) {
                .float => |b| (std.math.isNan(a) && std.math.isNan(b)) || a == b,
                else => false,
            },
            .double => |a| return switch (other) {
                .double => |b| (std.math.isNan(a) && std.math.isNan(b)) || a == b,
                else => false,
            },
            .bytes => |a| return switch (other) { .bytes => |b| std.mem.eql(u8, a, b), else => false },
            .string => |a| return switch (other) { .string => |b| std.mem.eql(u8, a, b), else => false },
            .fixed => |a| return switch (other) { .fixed => |b| std.mem.eql(u8, a, b), else => false },
            .enum_val => |a| return switch (other) {
                .enum_val => |b| a.index == b.index && std.mem.eql(u8, a.symbol, b.symbol),
                else => false,
            },
            .union_val => |a| return switch (other) {
                .union_val => |b| a.index == b.index && a.value.eql(b.value.*),
                else => false,
            },
            .array => |a| return switch (other) {
                .array => |b| {
                    if (a.len != b.len) return false;
                    let i: usize = 0;
                    while (i < a.len) : (i += 1) {
                        if (!a[i].eql(b[i])) return false;
                    }
                    return true;
                },
                else => false,
            },
            .map => |a| return switch (other) {
                .map => |b| {
                    if (a.len != b.len) return false;
                    let i: usize = 0;
                    while (i < a.len) : (i += 1) {
                        if (!std.mem.eql(u8, a[i].key, b[i].key)) return false;
                        if (!a[i].value.eql(b[i].value)) return false;
                    }
                    return true;
                },
                else => false,
            },
            .record => |a| return switch (other) {
                .record => |b| {
                    if (a.len != b.len) return false;
                    let i: usize = 0;
                    while (i < a.len) : (i += 1) {
                        if (!std.mem.eql(u8, a[i].name, b[i].name)) return false;
                        if (!a[i].value.eql(b[i].value)) return false;
                    }
                    return true;
                },
                else => false,
            },
        }
    }
}

// ---------------------------------------------------------------------------
// ZigZag + Varint & Primitive Wire Helpers
// ---------------------------------------------------------------------------

fn appendByte(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, b: u8): AvroError!void {
    buf.append(alloc, b) catch return AvroError.OutOfMemory;
}

fn appendSlice(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): AvroError!void {
    buf.appendSlice(alloc, s) catch return AvroError.OutOfMemory;
}

fn writeVarintU64(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u64): AvroError!void {
    let v = val;
    while (true) {
        let byte: u8 = (v & 0x7F) as u8;
        v >>= 7;
        if (v != 0) {
            byte |= 0x80;
            appendByte(buf, alloc, byte)?;
        } else {
            appendByte(buf, alloc, byte)?;
            break;
        }
    }
}

fn writeZigZagI32(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: i32): AvroError!void {
    const u: u32 = @bitCast(val);
    const mask: u32 = if (val < 0) 0xFFFFFFFF else 0;
    const zz: u32 = (u << 1) ^ mask;
    writeVarintU64(buf, alloc, zz as u64)?;
}

fn writeZigZagI64(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: i64): AvroError!void {
    const u: u64 = @bitCast(val);
    const mask: u64 = if (val < 0) 0xFFFFFFFFFFFFFFFF else 0;
    const zz: u64 = (u << 1) ^ mask;
    writeVarintU64(buf, alloc, zz)?;
}

fn readByte(data: []const u8, pos: *usize): AvroError!u8 {
    if (pos.* >= data.len) {
        return AvroError.UnexpectedEof;
    }
    const b = data[pos.*];
    pos.* += 1;
    return b;
}

fn readVarintU32(data: []const u8, pos: *usize): AvroError!u32 {
    let result: u32 = 0;
    let shift: u5 = 0;
    let count: usize = 0;
    while (true) {
        const b = readByte(data, pos)?;
        if (count == 4) {
            if ((b & 0xF0) != 0) {
                return AvroError.VarintOverflow;
            }
            result |= ((b & 0x0F) as u32) << 28;
            return result;
        }
        result |= ((b & 0x7F) as u32) << shift;
        if ((b & 0x80) == 0) {
            return result;
        }
        shift += 7;
        count += 1;
    }
}

fn readVarintU64(data: []const u8, pos: *usize): AvroError!u64 {
    let result: u64 = 0;
    let shift: u6 = 0;
    let count: usize = 0;
    while (true) {
        const b = readByte(data, pos)?;
        if (count == 9) {
            if ((b & 0xFE) != 0) {
                return AvroError.VarintOverflow;
            }
            result |= ((b & 0x01) as u64) << 63;
            return result;
        }
        result |= ((b & 0x7F) as u64) << shift;
        if ((b & 0x80) == 0) {
            return result;
        }
        shift += 7;
        count += 1;
    }
}

fn readZigZagI32(data: []const u8, pos: *usize): AvroError!i32 {
    const zz = readVarintU32(data, pos)?;
    const raw: u32 = (zz >> 1) ^ (0 -% (zz & 1));
    return @bitCast(raw);
}

fn readZigZagI64(data: []const u8, pos: *usize): AvroError!i64 {
    const zz = readVarintU64(data, pos)?;
    const raw: u64 = (zz >> 1) ^ (0 -% (zz & 1));
    return @bitCast(raw);
}

// ---------------------------------------------------------------------------
// Datum Encoder & Decoder
// ---------------------------------------------------------------------------

pub fn encodeDatum(allocator: std.mem.Allocator, schema: AvroSchema, value: AvroValue): AvroError![]u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(allocator);

    encodeNode(&buf, allocator, schema.root, value, 0)?;
    return buf.toOwnedSlice(allocator) catch return AvroError.OutOfMemory;
}

fn encodeNode(
    buf: *std.ArrayList(u8),
    alloc: std.mem.Allocator,
    node: AvroSchemaNode,
    value: AvroValue,
    depth: usize,
): AvroError!void {
    if (depth > DEFAULT_MAX_DEPTH) {
        return AvroError.MaxDepthExceeded;
    }

    switch (node) {
        .null_type => {
            switch (value) {
                .null_val => { return; },
                else => return AvroError.SchemaMismatch,
            }
        },
        .boolean_type => {
            switch (value) {
                .boolean => |b| appendByte(buf, alloc, if (b) 1 else 0)?,
                else => return AvroError.SchemaMismatch,
            }
        },
        .int_type => {
            switch (value) {
                .int => |v| writeZigZagI32(buf, alloc, v)?,
                else => return AvroError.SchemaMismatch,
            }
        },
        .long_type => {
            switch (value) {
                .long => |v| writeZigZagI64(buf, alloc, v)?,
                else => return AvroError.SchemaMismatch,
            }
        },
        .float_type => {
            switch (value) {
                .float => |v| {
                    const bits: u32 = @bitCast(v);
                    let raw: [4]u8 = [0, 0, 0, 0];
                    std.mem.writeInt(u32, &raw, bits, .little);
                    appendSlice(buf, alloc, &raw)?;
                },
                else => return AvroError.SchemaMismatch,
            }
        },
        .double_type => {
            switch (value) {
                .double => |v| {
                    const bits: u64 = @bitCast(v);
                    let raw: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
                    std.mem.writeInt(u64, &raw, bits, .little);
                    appendSlice(buf, alloc, &raw)?;
                },
                else => return AvroError.SchemaMismatch,
            }
        },
        .bytes_type => {
            switch (value) {
                .bytes => |b| {
                    writeZigZagI64(buf, alloc, b.len as i64)?;
                    appendSlice(buf, alloc, b)?;
                },
                else => return AvroError.SchemaMismatch,
            }
        },
        .string_type => {
            switch (value) {
                .string => |s| {
                    writeZigZagI64(buf, alloc, s.len as i64)?;
                    appendSlice(buf, alloc, s)?;
                },
                else => return AvroError.SchemaMismatch,
            }
        },
        .fixed_type => |fx| {
            switch (value) {
                .fixed => |b| {
                    if (b.len != fx.size) return AvroError.SchemaMismatch;
                    appendSlice(buf, alloc, b)?;
                },
                else => return AvroError.SchemaMismatch,
            }
        },
        .enum_type => |en| {
            switch (value) {
                .enum_val => |ev| {
                    if (ev.index < 0 || (ev.index as usize) >= en.symbols.len) {
                        return AvroError.SchemaMismatch;
                    }
                    if (ev.symbol.len > 0 && !std.mem.eql(u8, ev.symbol, en.symbols[ev.index as usize])) {
                        return AvroError.SchemaMismatch;
                    }
                    writeZigZagI32(buf, alloc, ev.index)?;
                },
                else => return AvroError.SchemaMismatch,
            }
        },
        .array_type => |item_schema| {
            switch (value) {
                .array => |items| {
                    if (items.len > 0) {
                        writeZigZagI64(buf, alloc, items.len as i64)?;
                        let i: usize = 0;
                        while (i < items.len) : (i += 1) {
                            encodeNode(buf, alloc, item_schema.*, items[i], depth + 1)?;
                        }
                    }
                    writeZigZagI64(buf, alloc, 0)?;
                },
                else => return AvroError.SchemaMismatch,
            }
        },
        .map_type => |val_schema| {
            switch (value) {
                .map => |entries| {
                    if (entries.len > 0) {
                        writeZigZagI64(buf, alloc, entries.len as i64)?;
                        let i: usize = 0;
                        while (i < entries.len) : (i += 1) {
                            writeZigZagI64(buf, alloc, entries[i].key.len as i64)?;
                            appendSlice(buf, alloc, entries[i].key)?;
                            encodeNode(buf, alloc, val_schema.*, entries[i].value, depth + 1)?;
                        }
                    }
                    writeZigZagI64(buf, alloc, 0)?;
                },
                else => return AvroError.SchemaMismatch,
            }
        },
        .union_type => |branches| {
            switch (value) {
                .union_val => |uv| {
                    if (uv.index >= branches.len) {
                        return AvroError.InvalidUnionIndex;
                    }
                    writeZigZagI64(buf, alloc, uv.index as i64)?;
                    encodeNode(buf, alloc, branches[uv.index], uv.value.*, depth + 1)?;
                },
                else => return AvroError.SchemaMismatch,
            }
        },
        .record_type => |rec| {
            switch (value) {
                .record => |val_fields| {
                    if (val_fields.len != rec.fields.len) {
                        return AvroError.SchemaMismatch;
                    }
                    let i: usize = 0;
                    while (i < rec.fields.len) : (i += 1) {
                        const schema_field = rec.fields[i];
                        if (std.mem.eql(u8, val_fields[i].name, schema_field.name)) {
                            encodeNode(buf, alloc, schema_field.schema, val_fields[i].value, depth + 1)?;
                        } else {
                            let found_val: AvroValue? = null;
                            let j: usize = 0;
                            while (j < val_fields.len) : (j += 1) {
                                if (std.mem.eql(u8, val_fields[j].name, schema_field.name)) {
                                    found_val = val_fields[j].value;
                                    break;
                                }
                            }
                            if (fv := found_val) {
                                encodeNode(buf, alloc, schema_field.schema, fv, depth + 1)?;
                            } else {
                                return AvroError.SchemaMismatch;
                            }
                        }
                    }
                },
                else => return AvroError.SchemaMismatch,
            }
        },
    }
}

pub fn decodeDatum(allocator: std.mem.Allocator, schema: AvroSchema, bytes: []const u8): AvroError!AvroValue {
    let pos: usize = 0;
    return decodeNode(allocator, schema.root, bytes, &pos, 0);
}

fn readLenPrefixedBytes(alloc: std.mem.Allocator, data: []const u8, pos: *usize): AvroError![]const u8 {
    const raw_len = readZigZagI64(data, pos)?;
    if (raw_len < 0) {
        return AvroError.SchemaMismatch;
    }
    const len = raw_len as usize;
    if (pos.* + len > data.len) {
        return AvroError.UnexpectedEof;
    }
    const slice = data[pos.* .. pos.* + len];
    pos.* += len;
    return dupeStr(alloc, slice);
}

fn decodeNode(
    alloc: std.mem.Allocator,
    node: AvroSchemaNode,
    data: []const u8,
    pos: *usize,
    depth: usize,
): AvroError!AvroValue {
    if (depth > DEFAULT_MAX_DEPTH) {
        return AvroError.MaxDepthExceeded;
    }

    switch (node) {
        .null_type => return AvroValue.makeNull(),
        .boolean_type => {
            const b = readByte(data, pos)?;
            if (b == 0) return AvroValue.makeBoolean(false);
            if (b == 1) return AvroValue.makeBoolean(true);
            return AvroError.SchemaMismatch;
        },
        .int_type => {
            const v = readZigZagI32(data, pos)?;
            return AvroValue.makeInt(v);
        },
        .long_type => {
            const v = readZigZagI64(data, pos)?;
            return AvroValue.makeLong(v);
        },
        .float_type => {
            if (pos.* + 4 > data.len) return AvroError.UnexpectedEof;
            const bits = std.mem.readInt(u32, data[pos.* .. pos.* + 4][0..4], .little);
            pos.* += 4;
            const f: f32 = @bitCast(bits);
            return AvroValue.makeFloat(f);
        },
        .double_type => {
            if (pos.* + 8 > data.len) return AvroError.UnexpectedEof;
            const bits = std.mem.readInt(u64, data[pos.* .. pos.* + 8][0..8], .little);
            pos.* += 8;
            const d: f64 = @bitCast(bits);
            return AvroValue.makeDouble(d);
        },
        .bytes_type => {
            const b = readLenPrefixedBytes(alloc, data, pos)?;
            return AvroValue.makeBytes(b);
        },
        .string_type => {
            const s = readLenPrefixedBytes(alloc, data, pos)?;
            return AvroValue.makeString(s);
        },
        .fixed_type => |fx| {
            if (pos.* + fx.size > data.len) return AvroError.UnexpectedEof;
            const slice = data[pos.* .. pos.* + fx.size];
            pos.* += fx.size;
            return AvroValue.makeFixed(dupeStr(alloc, slice)?);
        },
        .enum_type => |en| {
            const idx = readZigZagI32(data, pos)?;
            if (idx < 0 || (idx as usize) >= en.symbols.len) {
                return AvroError.SchemaMismatch;
            }
            const sym = dupeStr(alloc, en.symbols[idx as usize])?;
            return AvroValue.makeEnum(idx, sym);
        },
        .array_type => |item_schema| {
            let items_list = std.ArrayList(AvroValue).empty;
            errdefer {
                let k: usize = 0;
                while (k < items_list.items.len) : (k += 1) {
                    items_list.items[k].deinit(alloc);
                }
                items_list.deinit(alloc);
            }

            while (true) {
                let count = readZigZagI64(data, pos)?;
                if (count == 0) break;
                if (count < 0) {
                    count = -count;
                    * = readZigZagI64(data, pos)?;
                }
                const n = count as usize;
                let i: usize = 0;
                while (i < n) : (i += 1) {
                    let elem = decodeNode(alloc, item_schema.*, data, pos, depth + 1)?;
                    errdefer elem.deinit(alloc);
                    items_list.append(alloc, elem) catch return AvroError.OutOfMemory;
                }
            }
            const out = items_list.toOwnedSlice(alloc) catch return AvroError.OutOfMemory;
            return AvroValue.makeArray(out);
        },
        .map_type => |val_schema| {
            let entries_list = std.ArrayList(AvroMapEntry).empty;
            errdefer {
                let k: usize = 0;
                while (k < entries_list.items.len) : (k += 1) {
                    alloc.free(entries_list.items[k].key);
                    entries_list.items[k].value.deinit(alloc);
                }
                entries_list.deinit(alloc);
            }

            while (true) {
                let count = readZigZagI64(data, pos)?;
                if (count == 0) break;
                if (count < 0) {
                    count = -count;
                    * = readZigZagI64(data, pos)?;
                }
                const n = count as usize;
                let i: usize = 0;
                while (i < n) : (i += 1) {
                    const key = readLenPrefixedBytes(alloc, data, pos)?;
                    errdefer alloc.free(key);
                    let val = decodeNode(alloc, val_schema.*, data, pos, depth + 1)?;
                    errdefer val.deinit(alloc);
                    entries_list.append(alloc, AvroMapEntry.init(key, val)) catch return AvroError.OutOfMemory;
                }
            }
            const out = entries_list.toOwnedSlice(alloc) catch return AvroError.OutOfMemory;
            return AvroValue.makeMap(out);
        },
        .union_type => |branches| {
            const raw_idx = readZigZagI64(data, pos)?;
            if (raw_idx < 0 || (raw_idx as usize) >= branches.len) {
                return AvroError.InvalidUnionIndex;
            }
            const idx = raw_idx as usize;
            let inner = decodeNode(alloc, branches[idx], data, pos, depth + 1)?;
            errdefer inner.deinit(alloc);
            let inner_ptr = alloc.create(AvroValue) catch return AvroError.OutOfMemory;
            inner_ptr.* = inner;
            return AvroValue.makeUnion(idx, inner_ptr);
        },
        .record_type => |rec| {
            let fields_out = alloc.alloc(AvroRecordField, rec.fields.len) catch return AvroError.OutOfMemory;
            let initialized: usize = 0;
            errdefer {
                let j: usize = 0;
                while (j < initialized) : (j += 1) {
                    alloc.free(fields_out[j].name);
                    fields_out[j].value.deinit(alloc);
                }
                alloc.free(fields_out);
            }

            while (initialized < rec.fields.len) : (initialized += 1) {
                const sf = rec.fields[initialized];
                const fname = dupeStr(alloc, sf.name)?;
                errdefer alloc.free(fname);
                const fval = decodeNode(alloc, sf.schema, data, pos, depth + 1)?;
                fields_out[initialized] = AvroRecordField.init(fname, fval);
            }
            return AvroValue.makeRecord(fields_out);
        },
    }
}

// ---------------------------------------------------------------------------
// Object Container File (OCF "Obj\x01") Reader & Writer
// ---------------------------------------------------------------------------

pub struct AvroContainer {
    schema_json: []const u8 = "",
    sync_marker: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    codec: []const u8 = "",
    records: []AvroValue,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        allocator.free(_.schema_json);
        allocator.free(_.codec);
        let i: usize = 0;
        while (i < _.records.len) : (i += 1) {
            _.records[i].deinit(allocator);
        }
        allocator.free(_.records);
    }
}

pub fn writeContainer(
    allocator: std.mem.Allocator,
    schema_json: []const u8,
    sync_marker: [16]u8,
    records: []const AvroValue,
): AvroError![]u8 {
    let schema = AvroSchema.parseSchema(allocator, schema_json)?;
    defer schema.deinit();

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(allocator);

    // 1. Magic "Obj\x01"
    appendSlice(&out, allocator, &OCF_MAGIC)?;

    // 2. Metadata map<bytes>: avro.schema & avro.codec = "null"
    writeZigZagI64(&out, allocator, 2)?;
    writeZigZagI64(&out, allocator, "avro.schema".len as i64)?;
    appendSlice(&out, allocator, "avro.schema")?;
    writeZigZagI64(&out, allocator, schema_json.len as i64)?;
    appendSlice(&out, allocator, schema_json)?;

    writeZigZagI64(&out, allocator, "avro.codec".len as i64)?;
    appendSlice(&out, allocator, "avro.codec")?;
    writeZigZagI64(&out, allocator, "null".len as i64)?;
    appendSlice(&out, allocator, "null")?;
    writeZigZagI64(&out, allocator, 0)?;

    // 3. 16-byte sync marker
    appendSlice(&out, allocator, &sync_marker)?;

    // 4. Data block (if records.len > 0)
    if (records.len > 0) {
        let block_buf = std.ArrayList(u8).empty;
        defer block_buf.deinit(allocator);

        let i: usize = 0;
        while (i < records.len) : (i += 1) {
            encodeNode(&block_buf, allocator, schema.root, records[i], 0)?;
        }

        writeZigZagI64(&out, allocator, records.len as i64)?;
        writeZigZagI64(&out, allocator, block_buf.items.len as i64)?;
        appendSlice(&out, allocator, block_buf.items)?;
        appendSlice(&out, allocator, &sync_marker)?;
    }

    return out.toOwnedSlice(allocator) catch return AvroError.OutOfMemory;
}

pub fn readContainer(allocator: std.mem.Allocator, ocf_bytes: []const u8): AvroError!AvroContainer {
    if (ocf_bytes.len < 4 || !std.mem.eql(u8, ocf_bytes[0..4], &OCF_MAGIC)) {
        return AvroError.InvalidContainerHeader;
    }

    let pos: usize = 4;
    let schema_json_opt: ([]const u8)? = null;
    errdefer {
        if (sj := schema_json_opt) {
            allocator.free(sj);
        }
    }
    let codec_opt: ([]const u8)? = null;
    errdefer {
        if (c := codec_opt) {
            allocator.free(c);
        }
    }

    // Read metadata map<bytes>
    while (true) {
        let count = readZigZagI64(ocf_bytes, &pos)?;
        if (count == 0) break;
        if (count < 0) {
            count = -count;
            * = readZigZagI64(ocf_bytes, &pos)?;
        }
        const n = count as usize;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            const k = readLenPrefixedBytes(allocator, ocf_bytes, &pos)?;
            defer allocator.free(k);
            const v = readLenPrefixedBytes(allocator, ocf_bytes, &pos)?;
            if (std.mem.eql(u8, k, "avro.schema")) {
                if (old := schema_json_opt) {
                    allocator.free(old);
                }
                schema_json_opt = v;
            } else if (std.mem.eql(u8, k, "avro.codec")) {
                if (old := codec_opt) {
                    allocator.free(old);
                }
                codec_opt = v;
            } else {
                allocator.free(v);
            }
        }
    }

    let schema_json: []const u8 = "";
    if (sj := schema_json_opt) {
        schema_json = sj;
    } else {
        return AvroError.InvalidContainerHeader;
    }

    let codec: []const u8 = "";
    if (c := codec_opt) {
        codec = c;
    } else {
        codec = dupeStr(allocator, "null")?;
    }
    if (!std.mem.eql(u8, codec, "null")) {
        return AvroError.InvalidContainerHeader;
    }

    if (pos + 16 > ocf_bytes.len) {
        return AvroError.UnexpectedEof;
    }
    let sync_marker: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    std.mem.copyForwards(u8, &sync_marker, ocf_bytes[pos .. pos + 16]);
    pos += 16;

    let schema = AvroSchema.parseSchema(allocator, schema_json)?;
    defer schema.deinit();

    let records_list = std.ArrayList(AvroValue).empty;
    errdefer {
        let k: usize = 0;
        while (k < records_list.items.len) : (k += 1) {
            records_list.items[k].deinit(allocator);
        }
        records_list.deinit(allocator);
    }

    while (pos < ocf_bytes.len) {
        const block_count = readZigZagI64(ocf_bytes, &pos)?;
        if (block_count <= 0) {
            return AvroError.InvalidContainerHeader;
        }
        const block_size = readZigZagI64(ocf_bytes, &pos)?;
        if (block_size < 0) {
            return AvroError.InvalidContainerHeader;
        }
        const bsize = block_size as usize;
        if (pos + bsize > ocf_bytes.len) {
            return AvroError.UnexpectedEof;
        }

        const block_end = pos + bsize;
        let i: usize = 0;
        const n = block_count as usize;
        while (i < n) : (i += 1) {
            let rec = decodeNode(allocator, schema.root, ocf_bytes[0..block_end], &pos, 0)?;
            errdefer rec.deinit(allocator);
            records_list.append(allocator, rec) catch return AvroError.OutOfMemory;
        }
        pos = block_end;

        if (pos + 16 > ocf_bytes.len) {
            return AvroError.UnexpectedEof;
        }
        if (!std.mem.eql(u8, ocf_bytes[pos .. pos + 16], &sync_marker)) {
            return AvroError.InvalidSyncMarker;
        }
        pos += 16;
    }

    const records = records_list.toOwnedSlice(allocator) catch return AvroError.OutOfMemory;
    return AvroContainer{
        schema_json: schema_json,
        sync_marker: sync_marker,
        codec: codec,
        records: records,
    };
}
