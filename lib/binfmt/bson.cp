// Source: cp/lib/bson/bson.cp (MongoDB BSON 1.1 Binary Document & 12-Byte ObjectId Codec)
// Pure CP implementation of MongoDB BSON 1.1 little-endian binary serialization,
// 12-byte ObjectId codec, strict UTF-8 / CString / size / terminator validation,
// and 64-level recursion depth guard.

const std = @import("std");

pub const BSON_MAX_DEPTH: usize = 64;

pub const BSON_TYPE_DOUBLE: u8 = 0x01;
pub const BSON_TYPE_UTF8: u8 = 0x02;
pub const BSON_TYPE_DOCUMENT: u8 = 0x03;
pub const BSON_TYPE_ARRAY: u8 = 0x04;
pub const BSON_TYPE_BINARY: u8 = 0x05;
pub const BSON_TYPE_OBJECT_ID: u8 = 0x07;
pub const BSON_TYPE_BOOLEAN: u8 = 0x08;
pub const BSON_TYPE_UTC_DATETIME: u8 = 0x09;
pub const BSON_TYPE_NULL: u8 = 0x0A;
pub const BSON_TYPE_INT32: u8 = 0x10;
pub const BSON_TYPE_TIMESTAMP: u8 = 0x11;
pub const BSON_TYPE_INT64: u8 = 0x12;

pub err BsonError {
    UnexpectedEof,
    InvalidDocumentSize,
    MissingTerminator,
    InvalidElementType,
    InvalidUtf8,
    InvalidBoolean,
    InvalidCString,
    InvalidObjectId,
    MaxDepthExceeded,
    TrailingBytes,
    OutOfMemory,
}

fn hexNibble(c: u8): u8! {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return err.InvalidObjectId;
}

pub struct ObjectId {
    bytes: [12]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],

    pub static fn fromBytes(bytes: [12]u8): ObjectId {
        return ObjectId{ bytes: bytes };
    }

    pub static fn fromHex(hex: []const u8): ObjectId! {
        if (hex.len != 24) {
            return err.InvalidObjectId;
        }
        let out: [12]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let i: usize = 0;
        while (i < 12) : (i += 1) {
            const hi = hexNibble(hex[i * 2])?;
            const lo = hexNibble(hex[i * 2 + 1])?;
            out[i] = (hi << 4) | lo;
        }
        return ObjectId{ bytes: out };
    }

    pub fn toHex(alloc: std.mem.Allocator): ([]u8)! {
        const hex_chars = "0123456789abcdef";
        let out = alloc.alloc(u8, 24) catch return err.OutOfMemory;
        let i: usize = 0;
        while (i < 12) : (i += 1) {
            const b = _.bytes[i];
            out[i * 2] = hex_chars[(b >> 4) & 0x0F];
            out[i * 2 + 1] = hex_chars[b & 0x0F];
        }
        return out;
    }

    pub static fn generate(timestamp_sec: u32, machine_rand: [5]u8, counter: u32): ObjectId {
        let out: [12]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        out[0] = ((timestamp_sec >> 24) & 0xFF) as u8;
        out[1] = ((timestamp_sec >> 16) & 0xFF) as u8;
        out[2] = ((timestamp_sec >> 8) & 0xFF) as u8;
        out[3] = (timestamp_sec & 0xFF) as u8;
        out[4] = machine_rand[0];
        out[5] = machine_rand[1];
        out[6] = machine_rand[2];
        out[7] = machine_rand[3];
        out[8] = machine_rand[4];
        out[9] = ((counter >> 16) & 0xFF) as u8;
        out[10] = ((counter >> 8) & 0xFF) as u8;
        out[11] = (counter & 0xFF) as u8;
        return ObjectId{ bytes: out };
    }

    pub fn timestampSec(): u32 {
        return ((_.bytes[0] as u32) << 24) |
            ((_.bytes[1] as u32) << 16) |
            ((_.bytes[2] as u32) << 8) |
            (_.bytes[3] as u32);
    }

    pub fn eql(other: ObjectId): bool {
        return std.mem.eql(u8, &_.bytes, &other.bytes);
    }
}

pub struct BsonBinary {
    subtype: u8 = 0,
    data: []const u8 = "",
}

pub struct BsonTimestamp {
    increment: u32 = 0,
    timestamp: u32 = 0,
}

pub struct BsonField {
    key: []const u8 = "",
    value: BsonValue = .null_val,

    pub static fn init(key: []const u8, value: BsonValue): BsonField {
        return BsonField{ key: key, value: value };
    }
}

pub fn deinitDocument(alloc: std.mem.Allocator, fields: []BsonField) {
    let i: usize = 0;
    while (i < fields.len) : (i += 1) {
        if (fields[i].key.len > 0) {
            alloc.free(fields[i].key);
        }
        fields[i].value.deinit(alloc);
    }
    if (fields.len > 0) {
        alloc.free(fields);
    }
}

pub fn getField(fields: []const BsonField, key: []const u8): BsonValue? {
    let i: usize = 0;
    while (i < fields.len) : (i += 1) {
        if (std.mem.eql(u8, fields[i].key, key)) {
            return fields[i].value;
        }
    }
    return null;
}

pub fn eqlFields(a: []const BsonField, b: []const BsonField): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (!std.mem.eql(u8, a[i].key, b[i].key)) return false;
        if (!a[i].value.eql(b[i].value)) return false;
    }
    return true;
}

pub enum BsonValue {
    double: f64,
    utf8: []const u8,
    document: []BsonField,
    array: []BsonValue,
    binary: BsonBinary,
    object_id: ObjectId,
    boolean: bool,
    utc_datetime: i64,
    null_val,
    int32: i32,
    timestamp: BsonTimestamp,
    int64: i64,

    pub fn deinit(alloc: std.mem.Allocator) {
        switch (_) {
            .utf8 s => {
                if (s.len > 0) {
                    alloc.free(s);
                }
            },
            .document fields => {
                deinitDocument(alloc, fields);
            },
            .array items => {
                let i: usize = 0;
                while (i < items.len) : (i += 1) {
                    items[i].deinit(alloc);
                }
                if (items.len > 0) {
                    alloc.free(items);
                }
            },
            .binary b => {
                if (b.data.len > 0) {
                    alloc.free(b.data);
                }
            },
            else => {},
        }
    }

    pub fn get(key: []const u8): BsonValue? {
        return switch (_) {
            .document fields => getField(fields, key),
            else => null,
        };
    }

    pub fn eql(other: BsonValue): bool {
        return switch (_) {
            .double a => switch (other) {
                .double b => {
                    if (std.math.isNan(a) && std.math.isNan(b)) return true;
                    return a == b;
                },
                else => false,
            },
            .utf8 a => switch (other) {
                .utf8 b => std.mem.eql(u8, a, b),
                else => false,
            },
            .document a => switch (other) {
                .document b => eqlFields(a, b),
                else => false,
            },
            .array a => switch (other) {
                .array b => {
                    if (a.len != b.len) return false;
                    let i: usize = 0;
                    while (i < a.len) : (i += 1) {
                        if (!a[i].eql(b[i])) return false;
                    }
                    return true;
                },
                else => false,
            },
            .binary a => switch (other) {
                .binary b => a.subtype == b.subtype && std.mem.eql(u8, a.data, b.data),
                else => false,
            },
            .object_id a => switch (other) {
                .object_id b => a.eql(b),
                else => false,
            },
            .boolean a => switch (other) {
                .boolean b => a == b,
                else => false,
            },
            .utc_datetime a => switch (other) {
                .utc_datetime b => a == b,
                else => false,
            },
            .null_val => switch (other) {
                .null_val => true,
                else => false,
            },
            .int32 a => switch (other) {
                .int32 b => a == b,
                else => false,
            },
            .timestamp a => switch (other) {
                .timestamp b => a.increment == b.increment && a.timestamp == b.timestamp,
                else => false,
            },
            .int64 a => switch (other) {
                .int64 b => a == b,
                else => false,
            },
        };
    }
}

// ============================================================================
// Little-Endian Primitives
// ============================================================================

fn appendU32Le(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u32): void! {
    let buf: [4]u8 = [
        (v & 0xFF) as u8,
        ((v >> 8) & 0xFF) as u8,
        ((v >> 16) & 0xFF) as u8,
        ((v >> 24) & 0xFF) as u8,
    ];
    out.appendSlice(alloc, &buf) catch return err.OutOfMemory;
}

fn appendI32Le(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: i32): void! {
    const u: u32 = @bitCast(v);
    appendU32Le(alloc, out, u)?;
}

fn appendU64Le(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u64): void! {
    let buf: [8]u8 = [
        (v & 0xFF) as u8,
        ((v >> 8) & 0xFF) as u8,
        ((v >> 16) & 0xFF) as u8,
        ((v >> 24) & 0xFF) as u8,
        ((v >> 32) & 0xFF) as u8,
        ((v >> 40) & 0xFF) as u8,
        ((v >> 48) & 0xFF) as u8,
        ((v >> 56) & 0xFF) as u8,
    ];
    out.appendSlice(alloc, &buf) catch return err.OutOfMemory;
}

fn appendI64Le(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: i64): void! {
    const u: u64 = @bitCast(v);
    appendU64Le(alloc, out, u)?;
}

fn writeI32LeAt(slice: []u8, offset: usize, v: i32) {
    const u: u32 = @bitCast(v);
    slice[offset] = (u & 0xFF) as u8;
    slice[offset + 1] = ((u >> 8) & 0xFF) as u8;
    slice[offset + 2] = ((u >> 16) & 0xFF) as u8;
    slice[offset + 3] = ((u >> 24) & 0xFF) as u8;
}

fn readU32Le(s: []const u8): u32 {
    return (s[0] as u32) |
        ((s[1] as u32) << 8) |
        ((s[2] as u32) << 16) |
        ((s[3] as u32) << 24);
}

fn readI32Le(s: []const u8): i32 {
    const u = readU32Le(s);
    return @bitCast(u);
}

fn readU64Le(s: []const u8): u64 {
    return (s[0] as u64) |
        ((s[1] as u64) << 8) |
        ((s[2] as u64) << 16) |
        ((s[3] as u64) << 24) |
        ((s[4] as u64) << 32) |
        ((s[5] as u64) << 40) |
        ((s[6] as u64) << 48) |
        ((s[7] as u64) << 56);
}

fn readI64Le(s: []const u8): i64 {
    const u = readU64Le(s);
    return @bitCast(u);
}

// ============================================================================
// BSON 1.1 Encoder
// ============================================================================

fn appendCString(alloc: std.mem.Allocator, out: *std.ArrayList(u8), key: []const u8): void! {
    if (std.mem.indexOfScalar(u8, key, 0) != null) {
        return err.InvalidCString;
    }
    if (!std.unicode.utf8ValidateSlice(key)) {
        return err.InvalidUtf8;
    }
    out.appendSlice(alloc, key) catch return err.OutOfMemory;
    out.append(alloc, 0x00) catch return err.OutOfMemory;
}

fn encodeElement(alloc: std.mem.Allocator, out: *std.ArrayList(u8), key: []const u8, val: BsonValue): void! {
    switch (val) {
        .double d => {
            out.append(alloc, BSON_TYPE_DOUBLE) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
            const bits: u64 = @bitCast(d);
            appendU64Le(alloc, out, bits)?;
        },
        .utf8 s => {
            if (!std.unicode.utf8ValidateSlice(s)) {
                return err.InvalidUtf8;
            }
            if (s.len + 1 > 2147483647) {
                return err.InvalidDocumentSize;
            }
            out.append(alloc, BSON_TYPE_UTF8) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
            appendI32Le(alloc, out, (s.len + 1) as i32)?;
            out.appendSlice(alloc, s) catch return err.OutOfMemory;
            out.append(alloc, 0x00) catch return err.OutOfMemory;
        },
        .document fields => {
            out.append(alloc, BSON_TYPE_DOCUMENT) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
            encodeDocumentInto(alloc, out, fields)?;
        },
        .array items => {
            out.append(alloc, BSON_TYPE_ARRAY) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
            encodeArrayInto(alloc, out, items)?;
        },
        .binary b => {
            if (b.data.len > 2147483647) {
                return err.InvalidDocumentSize;
            }
            out.append(alloc, BSON_TYPE_BINARY) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
            appendI32Le(alloc, out, b.data.len as i32)?;
            out.append(alloc, b.subtype) catch return err.OutOfMemory;
            out.appendSlice(alloc, b.data) catch return err.OutOfMemory;
        },
        .object_id oid => {
            out.append(alloc, BSON_TYPE_OBJECT_ID) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
            out.appendSlice(alloc, &oid.bytes) catch return err.OutOfMemory;
        },
        .boolean b => {
            out.append(alloc, BSON_TYPE_BOOLEAN) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
            out.append(alloc, if (b) 0x01 else 0x00) catch return err.OutOfMemory;
        },
        .utc_datetime dt => {
            out.append(alloc, BSON_TYPE_UTC_DATETIME) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
            appendI64Le(alloc, out, dt)?;
        },
        .null_val => {
            out.append(alloc, BSON_TYPE_NULL) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
        },
        .int32 n => {
            out.append(alloc, BSON_TYPE_INT32) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
            appendI32Le(alloc, out, n)?;
        },
        .timestamp ts => {
            out.append(alloc, BSON_TYPE_TIMESTAMP) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
            appendU32Le(alloc, out, ts.increment)?;
            appendU32Le(alloc, out, ts.timestamp)?;
        },
        .int64 n => {
            out.append(alloc, BSON_TYPE_INT64) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
            appendI64Le(alloc, out, n)?;
        },
    }
}

fn encodeDocumentInto(alloc: std.mem.Allocator, out: *std.ArrayList(u8), fields: []const BsonField): void! {
    const start_pos = out.items.len;
    appendI32Le(alloc, out, 0)?;
    let i: usize = 0;
    while (i < fields.len) : (i += 1) {
        encodeElement(alloc, out, fields[i].key, fields[i].value)?;
    }
    out.append(alloc, 0x00) catch return err.OutOfMemory;
    const total_len = out.items.len - start_pos;
    if (total_len > 2147483647) {
        return err.InvalidDocumentSize;
    }
    writeI32LeAt(out.items, start_pos, total_len as i32);
}

fn encodeArrayInto(alloc: std.mem.Allocator, out: *std.ArrayList(u8), items: []const BsonValue): void! {
    const start_pos = out.items.len;
    appendI32Le(alloc, out, 0)?;
    let idx_buf: [24]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let i: usize = 0;
    while (i < items.len) : (i += 1) {
        const key_slice = std.fmt.bufPrint(&idx_buf, "{d}", .{i}) catch return err.InvalidDocumentSize;
        encodeElement(alloc, out, key_slice, items[i])?;
    }
    out.append(alloc, 0x00) catch return err.OutOfMemory;
    const total_len = out.items.len - start_pos;
    if (total_len > 2147483647) {
        return err.InvalidDocumentSize;
    }
    writeI32LeAt(out.items, start_pos, total_len as i32);
}

pub fn encodeDocument(alloc: std.mem.Allocator, fields: []const BsonField): ([]u8)! {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);
    encodeDocumentInto(alloc, &out, fields)?;
    return out.toOwnedSlice(alloc) catch return err.OutOfMemory;
}

// ============================================================================
// BSON 1.1 Decoder
// ============================================================================

fn readCStringSlice(src: []const u8, pos: *usize, end_limit: usize): ([]const u8)! {
    let cur = pos.*;
    while (cur < end_limit) : (cur += 1) {
        if (src[cur] == 0x00) {
            const s = src[pos.* .. cur];
            pos.* = cur + 1;
            if (!std.unicode.utf8ValidateSlice(s)) {
                return err.InvalidUtf8;
            }
            return s;
        }
    }
    return err.UnexpectedEof;
}

fn decodeValue(
    alloc: std.mem.Allocator,
    src: []const u8,
    pos: *usize,
    end_limit: usize,
    elem_type: u8,
    depth: usize,
    max_depth: usize,
): BsonValue! {
    switch (elem_type) {
        BSON_TYPE_DOUBLE => {
            if (pos.* + 8 > end_limit) return err.UnexpectedEof;
            const bits = readU64Le(src[pos.* .. pos.* + 8]);
            pos.* += 8;
            const d: f64 = @bitCast(bits);
            return BsonValue{ double: d };
        },
        BSON_TYPE_UTF8 => {
            if (pos.* + 4 > end_limit) return err.UnexpectedEof;
            const str_len = readI32Le(src[pos.* .. pos.* + 4]);
            pos.* += 4;
            if (str_len < 1) return err.InvalidDocumentSize;
            const ulen = str_len as usize;
            if (pos.* + ulen > end_limit) return err.UnexpectedEof;
            if (src[pos.* + ulen - 1] != 0x00) return err.MissingTerminator;
            const raw = src[pos.* .. pos.* + ulen - 1];
            pos.* += ulen;
            if (!std.unicode.utf8ValidateSlice(raw)) {
                return err.InvalidUtf8;
            }
            if (raw.len == 0) {
                return BsonValue{ utf8: "" };
            }
            const copy = alloc.dupe(u8, raw) catch return err.OutOfMemory;
            return BsonValue{ utf8: copy };
        },
        BSON_TYPE_DOCUMENT => {
            const sub_fields = decodeDocumentSlice(alloc, src, pos, end_limit, depth + 1, max_depth)?;
            return BsonValue{ document: sub_fields };
        },
        BSON_TYPE_ARRAY => {
            const items = decodeArraySlice(alloc, src, pos, end_limit, depth + 1, max_depth)?;
            return BsonValue{ array: items };
        },
        BSON_TYPE_BINARY => {
            if (pos.* + 5 > end_limit) return err.UnexpectedEof;
            const bin_len = readI32Le(src[pos.* .. pos.* + 4]);
            pos.* += 4;
            if (bin_len < 0) return err.InvalidDocumentSize;
            const subtype = src[pos.*];
            pos.* += 1;
            const ulen = bin_len as usize;
            if (pos.* + ulen > end_limit) return err.UnexpectedEof;
            const raw = src[pos.* .. pos.* + ulen];
            pos.* += ulen;
            let copy: []const u8 = "";
            if (raw.len > 0) {
                copy = alloc.dupe(u8, raw) catch return err.OutOfMemory;
            }
            return BsonValue{ binary: BsonBinary{ subtype: subtype, data: copy } };
        },
        BSON_TYPE_OBJECT_ID => {
            if (pos.* + 12 > end_limit) return err.UnexpectedEof;
            let oid_bytes: [12]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
            let i: usize = 0;
            while (i < 12) : (i += 1) {
                oid_bytes[i] = src[pos.* + i];
            }
            pos.* += 12;
            return BsonValue{ object_id: ObjectId{ bytes: oid_bytes } };
        },
        BSON_TYPE_BOOLEAN => {
            if (pos.* + 1 > end_limit) return err.UnexpectedEof;
            const b = src[pos.*];
            pos.* += 1;
            if (b == 0x00) return BsonValue{ boolean: false };
            if (b == 0x01) return BsonValue{ boolean: true };
            return err.InvalidBoolean;
        },
        BSON_TYPE_UTC_DATETIME => {
            if (pos.* + 8 > end_limit) return err.UnexpectedEof;
            const dt = readI64Le(src[pos.* .. pos.* + 8]);
            pos.* += 8;
            return BsonValue{ utc_datetime: dt };
        },
        BSON_TYPE_NULL => {
            return .null_val;
        },
        BSON_TYPE_INT32 => {
            if (pos.* + 4 > end_limit) return err.UnexpectedEof;
            const n = readI32Le(src[pos.* .. pos.* + 4]);
            pos.* += 4;
            return BsonValue{ int32: n };
        },
        BSON_TYPE_TIMESTAMP => {
            if (pos.* + 8 > end_limit) return err.UnexpectedEof;
            const inc = readU32Le(src[pos.* .. pos.* + 4]);
            const ts = readU32Le(src[pos.* + 4 .. pos.* + 8]);
            pos.* += 8;
            return BsonValue{ timestamp: BsonTimestamp{ increment: inc, timestamp: ts } };
        },
        BSON_TYPE_INT64 => {
            if (pos.* + 8 > end_limit) return err.UnexpectedEof;
            const n = readI64Le(src[pos.* .. pos.* + 8]);
            pos.* += 8;
            return BsonValue{ int64: n };
        },
        else => return err.InvalidElementType,
    }
}

fn freeFieldList(alloc: std.mem.Allocator, list: *std.ArrayList(BsonField)) {
    let i: usize = 0;
    while (i < list.items.len) : (i += 1) {
        if (list.items[i].key.len > 0) {
            alloc.free(list.items[i].key);
        }
        list.items[i].value.deinit(alloc);
    }
    list.deinit(alloc);
}

fn freeValueList(alloc: std.mem.Allocator, list: *std.ArrayList(BsonValue)) {
    let i: usize = 0;
    while (i < list.items.len) : (i += 1) {
        list.items[i].deinit(alloc);
    }
    list.deinit(alloc);
}

fn decodeDocumentSlice(
    alloc: std.mem.Allocator,
    src: []const u8,
    pos: *usize,
    parent_limit: usize,
    depth: usize,
    max_depth: usize,
): ([]BsonField)! {
    if (depth > max_depth) {
        return err.MaxDepthExceeded;
    }
    if (pos.* + 4 > parent_limit) {
        return err.UnexpectedEof;
    }
    const doc_start = pos.*;
    const doc_size = readI32Le(src[doc_start .. doc_start + 4]);
    pos.* += 4;
    if (doc_size < 5) {
        return err.InvalidDocumentSize;
    }
    const total_len = doc_size as usize;
    if (doc_start + total_len > parent_limit) {
        return err.InvalidDocumentSize;
    }
    const doc_end = doc_start + total_len;
    if (src[doc_end - 1] != 0x00) {
        return err.MissingTerminator;
    }
    const elem_limit = doc_end - 1;

    let fields: std.ArrayList(BsonField) = .empty;
    errdefer freeFieldList(alloc, &fields);

    while (pos.* < elem_limit) {
        const elem_type = src[pos.*];
        pos.* += 1;
        if (elem_type == 0x00) {
            return err.InvalidDocumentSize;
        }
        const key_raw = readCStringSlice(src, pos, elem_limit)?;
        let val = decodeValue(alloc, src, pos, elem_limit, elem_type, depth, max_depth)?;
        errdefer val.deinit(alloc);

        let key_copy: []const u8 = "";
        if (key_raw.len > 0) {
            key_copy = alloc.dupe(u8, key_raw) catch return err.OutOfMemory;
        }
        errdefer if (key_copy.len > 0) alloc.free(key_copy);

        fields.append(alloc, BsonField{ key: key_copy, value: val }) catch return err.OutOfMemory;
    }

    if (pos.* != elem_limit) {
        return err.InvalidDocumentSize;
    }
    pos.* = doc_end;
    return fields.toOwnedSlice(alloc) catch return err.OutOfMemory;
}

fn decodeArraySlice(
    alloc: std.mem.Allocator,
    src: []const u8,
    pos: *usize,
    parent_limit: usize,
    depth: usize,
    max_depth: usize,
): ([]BsonValue)! {
    if (depth > max_depth) {
        return err.MaxDepthExceeded;
    }
    if (pos.* + 4 > parent_limit) {
        return err.UnexpectedEof;
    }
    const doc_start = pos.*;
    const doc_size = readI32Le(src[doc_start .. doc_start + 4]);
    pos.* += 4;
    if (doc_size < 5) {
        return err.InvalidDocumentSize;
    }
    const total_len = doc_size as usize;
    if (doc_start + total_len > parent_limit) {
        return err.InvalidDocumentSize;
    }
    const doc_end = doc_start + total_len;
    if (src[doc_end - 1] != 0x00) {
        return err.MissingTerminator;
    }
    const elem_limit = doc_end - 1;

    let items: std.ArrayList(BsonValue) = .empty;
    errdefer freeValueList(alloc, &items);

    while (pos.* < elem_limit) {
        const elem_type = src[pos.*];
        pos.* += 1;
        if (elem_type == 0x00) {
            return err.InvalidDocumentSize;
        }
        * = readCStringSlice(src, pos, elem_limit)?;
        let val = decodeValue(alloc, src, pos, elem_limit, elem_type, depth, max_depth)?;
        errdefer val.deinit(alloc);

        items.append(alloc, val) catch return err.OutOfMemory;
    }

    if (pos.* != elem_limit) {
        return err.InvalidDocumentSize;
    }
    pos.* = doc_end;
    return items.toOwnedSlice(alloc) catch return err.OutOfMemory;
}

pub fn decodeDocumentWithMaxDepth(
    alloc: std.mem.Allocator,
    bytes: []const u8,
    max_depth: usize,
): ([]BsonField)! {
    if (bytes.len < 4) {
        return err.UnexpectedEof;
    }
    const declared_size = readI32Le(bytes[0..4]);
    if (declared_size < 5) {
        return err.InvalidDocumentSize;
    }
    const total_len = declared_size as usize;
    if (total_len > bytes.len) {
        return err.InvalidDocumentSize;
    }
    let pos: usize = 0;
    const fields = decodeDocumentSlice(alloc, bytes, &pos, total_len, 0, max_depth)?;
    if (total_len < bytes.len) {
        deinitDocument(alloc, fields);
        return err.TrailingBytes;
    }
    return fields;
}

pub fn decodeDocument(alloc: std.mem.Allocator, bytes: []const u8): ([]BsonField)! {
    return decodeDocumentWithMaxDepth(alloc, bytes, BSON_MAX_DEPTH);
}
