// Reference-hardened against mongodb/bson-rust (MIT)
// Source: cp/lib/bson/bson.cp (MongoDB BSON 1.1 Binary Document & 12-Byte ObjectId Codec)
// Pure CP implementation of MongoDB BSON 1.1 little-endian binary serialization,
// 12-byte ObjectId codec, strict UTF-8 / CString / Binary-0x02 / size / terminator validation,
// zero-heap document validator & cap:simd / 16-lane SIMD acceleration.

const std = @import("std");
import { Simd } from "cap:simd";

type Vec16u8 = @Vector(16, u8);

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
pub const BSON_TYPE_REGEX: u8 = 0x0B;
pub const BSON_TYPE_INT32: u8 = 0x10;
pub const BSON_TYPE_TIMESTAMP: u8 = 0x11;
pub const BSON_TYPE_INT64: u8 = 0x12;
pub const BSON_TYPE_DECIMAL128: u8 = 0x13;
pub const BSON_TYPE_MAX_KEY: u8 = 0x7F;
pub const BSON_TYPE_MIN_KEY: u8 = 0xFF;

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

pub struct BsonRegex {
    pattern: []const u8 = "",
    options: []const u8 = "",
}

pub struct BsonTimestamp {
    increment: u32 = 0,
    timestamp: u32 = 0,
}

pub struct BsonFieldSlice {
    elem_type: u8 = 0,
    value_bytes: []const u8 = "",
}

/// 标量参考：查找 CString NUL 终止符
pub fn findCStringNulScalar(slice: []const u8): usize? {
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        if (slice[i] == 0x00) return i;
    }
    return null;
}

/// cap:simd 加速：查找 CString NUL 终止符
pub fn findCStringNulSimd(slice: []const u8): usize? {
    return Simd.indexOfByte(slice, 0x00);
}

/// 标量参考：判断切片是否为纯 7-bit ASCII
pub fn isBsonAsciiScalar(slice: []const u8): bool {
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        if (slice[i] >= 0x80) return false;
    }
    return true;
}

/// cap:simd 加速：判断切片是否为纯 7-bit ASCII（UTF-8 快路径）
pub fn isBsonAsciiSimd(slice: []const u8): bool {
    return Simd.isAscii(slice);
}

/// 标量参考：字节拷贝
pub fn copyBsonBytesScalar(dst: []u8, src: []const u8) {
    let n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// 16-lane SIMD 加速：BSON String / Binary 大块字节拷贝
pub fn copyBsonBytesSimd(dst: []u8, src: []const u8) {
    let n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        let v: Vec16u8 = src[i .. i + 16][0..16].*;
        dst[i .. i + 16][0..16].* = v;
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

fn isValidBsonUtf8(s: []const u8): bool {
    if (isBsonAsciiSimd(s)) return true;
    return std.unicode.utf8ValidateSlice(s);
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
    regex: BsonRegex,
    int32: i32,
    timestamp: BsonTimestamp,
    int64: i64,
    decimal128: [16]u8,
    min_key,
    max_key,

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
            .regex r => {
                if (r.pattern.len > 0) alloc.free(r.pattern);
                if (r.options.len > 0) alloc.free(r.options);
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
            .regex a => switch (other) {
                .regex b => std.mem.eql(u8, a.pattern, b.pattern) && std.mem.eql(u8, a.options, b.options),
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
            .decimal128 a => switch (other) {
                .decimal128 b => std.mem.eql(u8, &a, &b),
                else => false,
            },
            .min_key => switch (other) {
                .min_key => true,
                else => false,
            },
            .max_key => switch (other) {
                .max_key => true,
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
    if (findCStringNulSimd(key) != null) {
        return err.InvalidCString;
    }
    if (!isValidBsonUtf8(key)) {
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
            if (!isValidBsonUtf8(s)) {
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
        .regex r => {
            out.append(alloc, BSON_TYPE_REGEX) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
            appendCString(alloc, out, r.pattern)?;
            appendCString(alloc, out, r.options)?;
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
        .decimal128 d128 => {
            out.append(alloc, BSON_TYPE_DECIMAL128) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
            out.appendSlice(alloc, &d128) catch return err.OutOfMemory;
        },
        .min_key => {
            out.append(alloc, BSON_TYPE_MIN_KEY) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
        },
        .max_key => {
            out.append(alloc, BSON_TYPE_MAX_KEY) catch return err.OutOfMemory;
            appendCString(alloc, out, key)?;
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

fn readCStringSlice(src: []const u8, pos: *usize, end_limit: usize): ([]const u8)!BsonError {
    const nul_rel = findCStringNulSimd(src[pos.* .. end_limit]) orelse return err.UnexpectedEof;
    const s = src[pos.* .. pos.* + nul_rel];
    pos.* += nul_rel + 1;
    if (!isValidBsonUtf8(s)) {
        return err.InvalidUtf8;
    }
    return s;
}

fn decodeValue(
    alloc: std.mem.Allocator,
    src: []const u8,
    pos: *usize,
    end_limit: usize,
    elem_type: u8,
    depth: usize,
    max_depth: usize,
): BsonValue!BsonError {
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
            if (!isValidBsonUtf8(raw)) {
                return err.InvalidUtf8;
            }
            if (raw.len == 0) {
                return BsonValue{ utf8: "" };
            }
            const copy = alloc.alloc(u8, raw.len) catch return err.OutOfMemory;
            copyBsonBytesSimd(copy, raw);
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
            if (ulen > end_limit - pos.*) return err.UnexpectedEof;
            if (subtype == 0x02) {
                if (ulen < 4) return err.InvalidDocumentSize;
                const inner_len = readI32Le(src[pos.* .. pos.* + 4]);
                if (inner_len < 0 || (inner_len as usize) + 4 != ulen) {
                    return err.InvalidDocumentSize;
                }
            }
            const raw = src[pos.* .. pos.* + ulen];
            pos.* += ulen;
            let copy: []const u8 = "";
            if (raw.len > 0) {
                const buf = alloc.alloc(u8, raw.len) catch return err.OutOfMemory;
                copyBsonBytesSimd(buf, raw);
                copy = buf;
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
        BSON_TYPE_REGEX => {
            const pat_raw = readCStringSlice(src, pos, end_limit)?;
            const opt_raw = readCStringSlice(src, pos, end_limit)?;
            let pat_copy: []const u8 = "";
            if (pat_raw.len > 0) {
                pat_copy = alloc.dupe(u8, pat_raw) catch return err.OutOfMemory;
            }
            errdefer if (pat_copy.len > 0) alloc.free(pat_copy);
            let opt_copy: []const u8 = "";
            if (opt_raw.len > 0) {
                opt_copy = alloc.dupe(u8, opt_raw) catch return err.OutOfMemory;
            }
            return BsonValue{ regex: BsonRegex{ pattern: pat_copy, options: opt_copy } };
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
        BSON_TYPE_DECIMAL128 => {
            if (pos.* + 16 > end_limit) return err.UnexpectedEof;
            let d128: [16]u8 = [0; 16];
            copyBsonBytesSimd(&d128, src[pos.* .. pos.* + 16]);
            pos.* += 16;
            return BsonValue{ decimal128: d128 };
        },
        BSON_TYPE_MIN_KEY => {
            return .min_key;
        },
        BSON_TYPE_MAX_KEY => {
            return .max_key;
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
): ([]BsonField)!BsonError {
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
): ([]BsonValue)!BsonError {
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
): ([]BsonField)!BsonError {
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

pub fn decodeDocument(alloc: std.mem.Allocator, bytes: []const u8): ([]BsonField)!BsonError {
    return decodeDocumentWithMaxDepth(alloc, bytes, BSON_MAX_DEPTH);
}

// ============================================================================
// Zero-Heap BSON 1.1 Validator & Field Lookup (mongodb/bson-rust parity)
// ============================================================================

fn validateElementZeroHeap(
    src: []const u8,
    pos: *usize,
    end_limit: usize,
    elem_type: u8,
    depth: usize,
    max_depth: usize,
): void!BsonError {
    switch (elem_type) {
        BSON_TYPE_DOUBLE | BSON_TYPE_UTC_DATETIME | BSON_TYPE_TIMESTAMP | BSON_TYPE_INT64 => {
            if (pos.* + 8 > end_limit) return err.UnexpectedEof;
            pos.* += 8;
        },
        BSON_TYPE_UTF8 => {
            if (pos.* + 4 > end_limit) return err.UnexpectedEof;
            const str_len = readI32Le(src[pos.* .. pos.* + 4]);
            pos.* += 4;
            if (str_len < 1) return err.InvalidDocumentSize;
            const ulen = str_len as usize;
            if (ulen > end_limit - pos.*) return err.UnexpectedEof;
            if (src[pos.* + ulen - 1] != 0x00) return err.MissingTerminator;
            if (!isValidBsonUtf8(src[pos.* .. pos.* + ulen - 1])) return err.InvalidUtf8;
            pos.* += ulen;
        },
        BSON_TYPE_DOCUMENT | BSON_TYPE_ARRAY => {
            validateDocumentSliceZeroHeap(src, pos, end_limit, depth + 1, max_depth)?;
        },
        BSON_TYPE_BINARY => {
            if (pos.* + 5 > end_limit) return err.UnexpectedEof;
            const bin_len = readI32Le(src[pos.* .. pos.* + 4]);
            pos.* += 4;
            if (bin_len < 0) return err.InvalidDocumentSize;
            const subtype = src[pos.*];
            pos.* += 1;
            const ulen = bin_len as usize;
            if (ulen > end_limit - pos.*) return err.UnexpectedEof;
            if (subtype == 0x02) {
                if (ulen < 4) return err.InvalidDocumentSize;
                const inner_len = readI32Le(src[pos.* .. pos.* + 4]);
                if (inner_len < 0 || (inner_len as usize) + 4 != ulen) return err.InvalidDocumentSize;
            }
            pos.* += ulen;
        },
        BSON_TYPE_OBJECT_ID => {
            if (pos.* + 12 > end_limit) return err.UnexpectedEof;
            pos.* += 12;
        },
        BSON_TYPE_BOOLEAN => {
            if (pos.* + 1 > end_limit) return err.UnexpectedEof;
            const b = src[pos.*];
            pos.* += 1;
            if (b != 0x00 && b != 0x01) return err.InvalidBoolean;
        },
        BSON_TYPE_NULL | BSON_TYPE_MIN_KEY | BSON_TYPE_MAX_KEY => {},
        BSON_TYPE_REGEX => {
            * = readCStringSlice(src, pos, end_limit)?;
            * = readCStringSlice(src, pos, end_limit)?;
        },
        BSON_TYPE_INT32 => {
            if (pos.* + 4 > end_limit) return err.UnexpectedEof;
            pos.* += 4;
        },
        BSON_TYPE_DECIMAL128 => {
            if (pos.* + 16 > end_limit) return err.UnexpectedEof;
            pos.* += 16;
        },
        else => return err.InvalidElementType,
    }
}

fn validateDocumentSliceZeroHeap(
    src: []const u8,
    pos: *usize,
    parent_limit: usize,
    depth: usize,
    max_depth: usize,
): void!BsonError {
    if (depth > max_depth) return err.MaxDepthExceeded;
    if (pos.* + 4 > parent_limit) return err.UnexpectedEof;
    const doc_start = pos.*;
    const doc_size = readI32Le(src[doc_start .. doc_start + 4]);
    pos.* += 4;
    if (doc_size < 5) return err.InvalidDocumentSize;
    const total_len = doc_size as usize;
    if (total_len > parent_limit - doc_start) return err.InvalidDocumentSize;
    const doc_end = doc_start + total_len;
    if (src[doc_end - 1] != 0x00) return err.MissingTerminator;
    const elem_limit = doc_end - 1;

    while (pos.* < elem_limit) {
        const elem_type = src[pos.*];
        pos.* += 1;
        if (elem_type == 0x00) return err.InvalidDocumentSize;
        * = readCStringSlice(src, pos, elem_limit)?;
        validateElementZeroHeap(src, pos, elem_limit, elem_type, depth, max_depth)?;
    }
    if (pos.* != elem_limit) return err.InvalidDocumentSize;
    pos.* = doc_end;
}

/// 零堆校验完整 BSON 1.1 文档（可配置最大嵌套深度）
pub fn validateBsonDocumentWithMaxDepth(bytes: []const u8, max_depth: usize): void!BsonError {
    if (bytes.len < 4) return err.UnexpectedEof;
    const declared = readI32Le(bytes[0..4]);
    if (declared < 5) return err.InvalidDocumentSize;
    const total_len = declared as usize;
    if (total_len > bytes.len) return err.InvalidDocumentSize;
    let pos: usize = 0;
    validateDocumentSliceZeroHeap(bytes, &pos, total_len, 0, max_depth)?;
    if (total_len < bytes.len) return err.TrailingBytes;
}

/// 零堆校验完整 BSON 1.1 文档（默认 `BSON_MAX_DEPTH = 64`）
pub fn validateBsonDocument(bytes: []const u8): void!BsonError {
    return validateBsonDocumentWithMaxDepth(bytes, BSON_MAX_DEPTH);
}

/// 零堆按字段名查找顶层 BSON 元素并返回原始类型与值切片
pub fn findBsonField(bytes: []const u8, key: []const u8): BsonFieldSlice? {
    if (bytes.len < 5) return null;
    const declared = readI32Le(bytes[0..4]);
    if (declared < 5) return null;
    const total_len = declared as usize;
    if (total_len > bytes.len || bytes[total_len - 1] != 0x00) return null;
    const elem_limit = total_len - 1;
    let pos: usize = 4;
    while (pos < elem_limit) {
        const elem_type = bytes[pos];
        pos += 1;
        if (elem_type == 0x00) return null;
        const k = readCStringSlice(bytes, &pos, elem_limit) catch return null;
        const val_start = pos;
        validateElementZeroHeap(bytes, &pos, elem_limit, elem_type, 0, BSON_MAX_DEPTH) catch return null;
        if (std.mem.eql(u8, k, key)) {
            return BsonFieldSlice{
                elem_type: elem_type,
                value_bytes: bytes[val_start..pos],
            };
        }
    }
    return null;
}
