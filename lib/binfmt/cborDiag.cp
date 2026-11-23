// Reference-hardened against enarx/ciborium (Apache-2.0)
// Source: native-cp-stdlib (RFC 8949 CBOR & Diagnostic Notation)
import std from "std";
import { Simd } from "cap:simd";

pub const MAX_DEPTH: usize = 64;
pub const CBOR_MAX_DEPTH: usize = 64;

pub error CborError {
    UnexpectedEof,
    InvalidAdditionalInfo,
    IndefiniteLengthNotAllowed,
    UnexpectedBreak,
    NestingTooDeep,
    InvalidUtf8,
    DuplicateMapKey,
    NonDeterministicEncoding,
    OutOfMemory,
    TrailingBytes,
}

/// 16-lane SIMD `@Vector(16, u8)` payload copy for hot-path CBOR ByteString/TextString.
pub fn cborCopyPayloadSimd(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i + 16 <= src.len) : (i += 16) {
        const chunk: @Vector(16, u8) = src[i..][0..16].*;
        dst[i..][0..16].* = chunk;
    }
    while (i < src.len) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Scalar reference payload copy for 0-diff parity testing against `cborCopyPayloadSimd`.
pub fn cborCopyPayloadScalar(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Strict UTF-8 validation with `cap:simd` `Simd.isAscii` fast-path.
pub fn cborIsValidUtf8Simd(s: []const u8): bool {
    if (Simd.isAscii(s)) return true;
    return std.unicode.utf8ValidateSlice(s);
}

/// Scalar reference UTF-8 validation for 0-diff parity testing against `cborIsValidUtf8Simd`.
pub fn cborIsValidUtf8Scalar(s: []const u8): bool {
    return std.unicode.utf8ValidateSlice(s);
}

/// Decodes an IEEE-754 half-precision Float16 (`0xF9` + big-endian u16) into `f64`.
pub fn decodeFloat16(bits: u16): f64 {
    const f16_val: f16 = @bitCast(bits);
    return @as(f64, f16_val);
}

pub struct CborEntry {
    key: CborValue,
    value: CborValue,
}

pub struct CborTag {
    tag: u64,
    value: CborValue,
}

pub union(enum) CborValue {
    Unsigned: u64,
    Negative: i64,
    Bytes: []const u8,
    Text: []const u8,
    Array: []CborValue,
    Map: []CborEntry,
    Tag: *CborTag,
    Bool: bool,
    Null: void,
    Undefined: void,
    Simple: u8,
    Float: f64,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.*) {
            .Bytes => |b| alloc.free(b),
            .Text => |t| alloc.free(t),
            .Array => |arr| {
                let i: usize = 0;
                while (i < arr.len) {
                    arr[i].deinit(alloc);
                    i += 1;
                }
                alloc.free(arr);
            },
            .Map => |entries| {
                let i: usize = 0;
                while (i < entries.len) {
                    entries[i].key.deinit(alloc);
                    entries[i].value.deinit(alloc);
                    i += 1;
                }
                alloc.free(entries);
            },
            .Tag => |tag_ptr| {
                tag_ptr.value.deinit(alloc);
                alloc.destroy(tag_ptr);
            },
            else => {},
        }
    }

    pub fn eql(other: CborValue): bool {
        const tag_a = std.meta.activeTag(_);
        const tag_b = std.meta.activeTag(other);
        if (tag_a != tag_b) {
            return false;
        }
        switch (_) {
            .Unsigned => |a| return a == other.Unsigned,
            .Negative => |a| return a == other.Negative,
            .Bytes => |a| return std.mem.eql(u8, a, other.Bytes),
            .Text => |a| return std.mem.eql(u8, a, other.Text),
            .Array => |a| {
                const b = other.Array;
                if (a.len != b.len) {
                    return false;
                }
                let i: usize = 0;
                while (i < a.len) {
                    if (!a[i].eql(b[i])) {
                        return false;
                    }
                    i += 1;
                }
                return true;
            },
            .Map => |a| {
                const b = other.Map;
                if (a.len != b.len) {
                    return false;
                }
                let i: usize = 0;
                while (i < a.len) {
                    if (!a[i].key.eql(b[i].key) or !a[i].value.eql(b[i].value)) {
                        return false;
                    }
                    i += 1;
                }
                return true;
            },
            .Tag => |a| {
                const b = other.Tag;
                return a.tag == b.tag and a.value.eql(b.value);
            },
            .Bool => |a| return a == other.Bool,
            .Null => return true,
            .Undefined => return true,
            .Simple => |a| return a == other.Simple,
            .Float => |a| {
                const b = other.Float;
                if (std.math.isNan(a) and std.math.isNan(b)) {
                    return true;
                }
                return a == b;
            },
        }
    }
}

struct CborReader {
    data: []const u8,
    pos: usize,
    alloc: std.mem.Allocator,
    depth: usize,
    max_depth: usize = CBOR_MAX_DEPTH,

    fn readByte(*_): !u8 {
        if (_.pos >= _.data.len) {
            return error.UnexpectedEof;
        }
        const b = _.data[_.pos];
        _.pos += 1;
        return b;
    }

    fn peekByte(): !u8 {
        if (_.pos >= _.data.len) {
            return error.UnexpectedEof;
        }
        return _.data[_.pos];
    }

    fn readSlice(*_, len: usize): ![]const u8 {
        if (len > _.data.len - _.pos) {
            return error.UnexpectedEof;
        }
        const slice = _.data[_.pos .. _.pos + len];
        _.pos += len;
        return slice;
    }

    fn readU16(*_): !u16 {
        const s = try _.readSlice(2);
        return std.mem.readInt(u16, s[0..2], .big);
    }

    fn readU32(*_): !u32 {
        const s = try _.readSlice(4);
        return std.mem.readInt(u32, s[0..4], .big);
    }

    fn readU64(*_): !u64 {
        const s = try _.readSlice(8);
        return std.mem.readInt(u64, s[0..8], .big);
    }

    fn readArg(*_, ai: u8): !u64 {
        if (ai < 24) {
            return @as(u64, ai);
        } else if (ai == 24) {
            return @as(u64, try _.readByte());
        } else if (ai == 25) {
            return @as(u64, try _.readU16());
        } else if (ai == 26) {
            return @as(u64, try _.readU32());
        } else if (ai == 27) {
            return try _.readU64();
        } else {
            return error.InvalidAdditionalInfo;
        }
    }

    fn readValue(*_): anyerror!CborValue {
        if (_.depth >= _.max_depth) {
            return error.NestingTooDeep;
        }
        const ib = try _.readByte();
        const major: u8 = ib >> 5;
        const ai: u8 = ib & 0x1f;

        if (major == 0) {
            if (ai == 31) {
                return error.IndefiniteLengthNotAllowed;
            }
            const val = try _.readArg(ai);
            return CborValue{ .Unsigned = val };
        } else if (major == 1) {
            if (ai == 31) {
                return error.IndefiniteLengthNotAllowed;
            }
            const val = try _.readArg(ai);
            if (val > 9223372036854775807) {
                return error.InvalidAdditionalInfo;
            }
            const signed_val: i64 = @intCast(val);
            return CborValue{ .Negative = -1 - signed_val };
        } else if (major == 2) {
            if (ai == 31) {
                let buf = std.ArrayList(u8).empty;
                errdefer buf.deinit(_.alloc);
                while (true) {
                    const next = try _.peekByte();
                    if (next == 0xff) {
                        _.pos += 1;
                        break;
                    }
                    const chunk_ib = try _.readByte();
                    const chunk_major: u8 = chunk_ib >> 5;
                    const chunk_ai: u8 = chunk_ib & 0x1f;
                    if (chunk_major != 2 or chunk_ai == 31) {
                        return error.InvalidAdditionalInfo;
                    }
                    const chunk_len_u64 = try _.readArg(chunk_ai);
                    if (chunk_len_u64 > _.data.len - _.pos) {
                        return error.UnexpectedEof;
                    }
                    const chunk_len: usize = @intCast(chunk_len_u64);
                    const chunk = try _.readSlice(chunk_len);
                    try buf.appendSlice(_.alloc, chunk);
                }
                const owned = try buf.toOwnedSlice(_.alloc);
                return CborValue{ .Bytes = owned };
            } else {
                const len_u64 = try _.readArg(ai);
                if (len_u64 > _.data.len - _.pos) {
                    return error.UnexpectedEof;
                }
                const len: usize = @intCast(len_u64);
                const slice = try _.readSlice(len);
                if (len >= 16) {
                    const duped = try _.alloc.alloc(u8, len);
                    cborCopyPayloadSimd(duped, slice);
                    return CborValue{ .Bytes = duped };
                }
                const duped = try _.alloc.dupe(u8, slice);
                return CborValue{ .Bytes = duped };
            }
        } else if (major == 3) {
            if (ai == 31) {
                let buf = std.ArrayList(u8).empty;
                errdefer buf.deinit(_.alloc);
                while (true) {
                    const next = try _.peekByte();
                    if (next == 0xff) {
                        _.pos += 1;
                        break;
                    }
                    const chunk_ib = try _.readByte();
                    const chunk_major: u8 = chunk_ib >> 5;
                    const chunk_ai: u8 = chunk_ib & 0x1f;
                    if (chunk_major != 3 or chunk_ai == 31) {
                        return error.InvalidAdditionalInfo;
                    }
                    const chunk_len_u64 = try _.readArg(chunk_ai);
                    if (chunk_len_u64 > _.data.len - _.pos) {
                        return error.UnexpectedEof;
                    }
                    const chunk_len: usize = @intCast(chunk_len_u64);
                    const chunk = try _.readSlice(chunk_len);
                    if (!cborIsValidUtf8Simd(chunk)) {
                        return error.InvalidUtf8;
                    }
                    try buf.appendSlice(_.alloc, chunk);
                }
                const owned = try buf.toOwnedSlice(_.alloc);
                return CborValue{ .Text = owned };
            } else {
                const len_u64 = try _.readArg(ai);
                if (len_u64 > _.data.len - _.pos) {
                    return error.UnexpectedEof;
                }
                const len: usize = @intCast(len_u64);
                const slice = try _.readSlice(len);
                if (!cborIsValidUtf8Simd(slice)) {
                    return error.InvalidUtf8;
                }
                if (len >= 16) {
                    const duped = try _.alloc.alloc(u8, len);
                    cborCopyPayloadSimd(duped, slice);
                    return CborValue{ .Text = duped };
                }
                const duped = try _.alloc.dupe(u8, slice);
                return CborValue{ .Text = duped };
            }
        } else if (major == 4) {
            _.depth += 1;
            defer {
                _.depth -= 1;
            }
            let items = std.ArrayList(CborValue).empty;
            errdefer {
                let k: usize = 0;
                while (k < items.items.len) {
                    items.items[k].deinit(_.alloc);
                    k += 1;
                }
                items.deinit(_.alloc);
            }
            if (ai == 31) {
                while (true) {
                    const next = try _.peekByte();
                    if (next == 0xff) {
                        _.pos += 1;
                        break;
                    }
                    let item = try _.readValue();
                    errdefer item.deinit(_.alloc);
                    try items.append(_.alloc, item);
                }
            } else {
                const count_u64 = try _.readArg(ai);
                if (count_u64 > _.data.len - _.pos) {
                    return error.UnexpectedEof;
                }
                const count: usize = @intCast(count_u64);
                let i: usize = 0;
                while (i < count) {
                    let item = try _.readValue();
                    errdefer item.deinit(_.alloc);
                    try items.append(_.alloc, item);
                    i += 1;
                }
            }
            const owned = try items.toOwnedSlice(_.alloc);
            return CborValue{ .Array = owned };
        } else if (major == 5) {
            _.depth += 1;
            defer {
                _.depth -= 1;
            }
            let entries = std.ArrayList(CborEntry).empty;
            errdefer {
                let k: usize = 0;
                while (k < entries.items.len) {
                    entries.items[k].key.deinit(_.alloc);
                    entries.items[k].value.deinit(_.alloc);
                    k += 1;
                }
                entries.deinit(_.alloc);
            }
            if (ai == 31) {
                while (true) {
                    const next = try _.peekByte();
                    if (next == 0xff) {
                        _.pos += 1;
                        break;
                    }
                    let key_val = try _.readValue();
                    errdefer key_val.deinit(_.alloc);
                    let val_val = try _.readValue();
                    errdefer val_val.deinit(_.alloc);
                    try entries.append(_.alloc, CborEntry{ .key = key_val, .value = val_val });
                }
            } else {
                const count_u64 = try _.readArg(ai);
                if (count_u64 > (_.data.len - _.pos) / 2) {
                    return error.UnexpectedEof;
                }
                const count: usize = @intCast(count_u64);
                let i: usize = 0;
                while (i < count) {
                    let key_val = try _.readValue();
                    errdefer key_val.deinit(_.alloc);
                    let val_val = try _.readValue();
                    errdefer val_val.deinit(_.alloc);
                    try entries.append(_.alloc, CborEntry{ .key = key_val, .value = val_val });
                    i += 1;
                }
            }
            const owned = try entries.toOwnedSlice(_.alloc);
            return CborValue{ .Map = owned };
        } else if (major == 6) {
            if (ai == 31) {
                return error.IndefiniteLengthNotAllowed;
            }
            _.depth += 1;
            defer {
                _.depth -= 1;
            }
            const tag_num = try _.readArg(ai);
            let inner = try _.readValue();
            errdefer inner.deinit(_.alloc);
            let tag_ptr = try _.alloc.create(CborTag);
            tag_ptr.* = CborTag{ .tag = tag_num, .value = inner };
            return CborValue{ .Tag = tag_ptr };
        } else {
            if (ai < 20) {
                return CborValue{ .Simple = ai };
            } else if (ai == 20) {
                return CborValue{ .Bool = false };
            } else if (ai == 21) {
                return CborValue{ .Bool = true };
            } else if (ai == 22) {
                return CborValue.Null;
            } else if (ai == 23) {
                return CborValue.Undefined;
            } else if (ai == 24) {
                const s = try _.readByte();
                if (s < 32) {
                    return error.InvalidAdditionalInfo;
                }
                return CborValue{ .Simple = s };
            } else if (ai == 25) {
                const bits = try _.readU16();
                const f16_val: f16 = @bitCast(bits);
                return CborValue{ .Float = @as(f64, f16_val) };
            } else if (ai == 26) {
                const bits = try _.readU32();
                const f32_val: f32 = @bitCast(bits);
                return CborValue{ .Float = @as(f64, f32_val) };
            } else if (ai == 27) {
                const bits = try _.readU64();
                const f64_val: f64 = @bitCast(bits);
                return CborValue{ .Float = f64_val };
            } else if (ai == 31) {
                return error.UnexpectedBreak;
            } else {
                return error.InvalidAdditionalInfo;
            }
        }
    }
}

pub fn decode(alloc: std.mem.Allocator, bytes: []const u8): !CborValue {
    return decodeWithMaxDepth(alloc, bytes, CBOR_MAX_DEPTH);
}

pub fn decodeWithMaxDepth(alloc: std.mem.Allocator, bytes: []const u8, max_depth: usize): !CborValue {
    let reader = CborReader{
        .data = bytes,
        .pos = 0,
        .alloc = alloc,
        .depth = 0,
        .max_depth = max_depth,
    };
    let val = try reader.readValue();
    errdefer val.deinit(alloc);
    if (reader.pos != bytes.len) {
        return error.TrailingBytes;
    }
    return val;
}

fn validateDeterministicArg(ai: u8, arg: u64): !void {
    if (ai == 24 and arg < 24) return error.NonDeterministicEncoding;
    if (ai == 25 and arg <= 0xff) return error.NonDeterministicEncoding;
    if (ai == 26 and arg <= 0xffff) return error.NonDeterministicEncoding;
    if (ai == 27 and arg <= 0xffffffff) return error.NonDeterministicEncoding;
}

fn validateCborNode(reader: *CborReader, deterministic: bool): anyerror!void {
    if (reader.depth >= reader.max_depth) {
        return error.NestingTooDeep;
    }
    const ib = try reader.readByte();
    const major: u8 = ib >> 5;
    const ai: u8 = ib & 0x1f;

    if (major == 0 or major == 1) {
        if (ai == 31) return error.IndefiniteLengthNotAllowed;
        const val = try reader.readArg(ai);
        if (deterministic) try validateDeterministicArg(ai, val);
        if (major == 1 and val > 9223372036854775807) {
            return error.InvalidAdditionalInfo;
        }
    } else if (major == 2 or major == 3) {
        if (ai == 31) {
            if (deterministic) return error.NonDeterministicEncoding;
            while (true) {
                const next = try reader.peekByte();
                if (next == 0xff) {
                    reader.pos += 1;
                    break;
                }
                const cib = try reader.readByte();
                const cmajor: u8 = cib >> 5;
                const cai: u8 = cib & 0x1f;
                if (cmajor != major or cai == 31) return error.InvalidAdditionalInfo;
                const clen_u64 = try reader.readArg(cai);
                if (clen_u64 > reader.data.len - reader.pos) return error.UnexpectedEof;
                const cslice = try reader.readSlice(@intCast(clen_u64));
                if (major == 3 and !cborIsValidUtf8Simd(cslice)) return error.InvalidUtf8;
            }
        } else {
            const len_u64 = try reader.readArg(ai);
            if (deterministic) try validateDeterministicArg(ai, len_u64);
            if (len_u64 > reader.data.len - reader.pos) return error.UnexpectedEof;
            const slice = try reader.readSlice(@intCast(len_u64));
            if (major == 3 and !cborIsValidUtf8Simd(slice)) return error.InvalidUtf8;
        }
    } else if (major == 4) {
        reader.depth += 1;
        defer { reader.depth -= 1; }
        if (ai == 31) {
            if (deterministic) return error.NonDeterministicEncoding;
            while (true) {
                const next = try reader.peekByte();
                if (next == 0xff) {
                    reader.pos += 1;
                    break;
                }
                try validateCborNode(reader, deterministic);
            }
        } else {
            const count_u64 = try reader.readArg(ai);
            if (deterministic) try validateDeterministicArg(ai, count_u64);
            if (count_u64 > reader.data.len - reader.pos) return error.UnexpectedEof;
            const count: usize = @intCast(count_u64);
            let i: usize = 0;
            while (i < count) : (i += 1) {
                try validateCborNode(reader, deterministic);
            }
        }
    } else if (major == 5) {
        reader.depth += 1;
        defer { reader.depth -= 1; }
        if (ai == 31) {
            if (deterministic) return error.NonDeterministicEncoding;
            while (true) {
                const next = try reader.peekByte();
                if (next == 0xff) {
                    reader.pos += 1;
                    break;
                }
                try validateCborNode(reader, deterministic);
                try validateCborNode(reader, deterministic);
            }
        } else {
            const count_u64 = try reader.readArg(ai);
            if (deterministic) try validateDeterministicArg(ai, count_u64);
            if (count_u64 > (reader.data.len - reader.pos) / 2) return error.UnexpectedEof;
            const count: usize = @intCast(count_u64);
            let prev_key: []const u8 = "";
            let has_prev = false;
            let i: usize = 0;
            while (i < count) : (i += 1) {
                const key_start = reader.pos;
                try validateCborNode(reader, deterministic);
                const key_slice = reader.data[key_start..reader.pos];
                if (deterministic) {
                    if (has_prev) {
                        if (key_slice.len < prev_key.len) return error.NonDeterministicEncoding;
                        if (key_slice.len == prev_key.len) {
                            const ord = std.mem.order(u8, prev_key, key_slice);
                            if (ord == .eq) return error.DuplicateMapKey;
                            if (ord == .gt) return error.NonDeterministicEncoding;
                        }
                    }
                    prev_key = key_slice;
                    has_prev = true;
                }
                try validateCborNode(reader, deterministic);
            }
        }
    } else if (major == 6) {
        if (ai == 31) return error.IndefiniteLengthNotAllowed;
        reader.depth += 1;
        defer { reader.depth -= 1; }
        const tag_num = try reader.readArg(ai);
        if (deterministic) try validateDeterministicArg(ai, tag_num);
        try validateCborNode(reader, deterministic);
    } else {
        if (ai < 24) {
            return;
        } else if (ai == 24) {
            const s = try reader.readByte();
            if (s < 32) return error.InvalidAdditionalInfo;
        } else if (ai == 25) {
            const bits = try reader.readU16();
            if (deterministic) {
                const f16_v: f16 = @bitCast(bits);
                if (std.math.isNan(f16_v) and bits != 0x7e00) return error.NonDeterministicEncoding;
            }
        } else if (ai == 26) {
            const bits = try reader.readU32();
            if (deterministic) {
                const f32_v: f32 = @bitCast(bits);
                if (std.math.isNan(f32_v)) return error.NonDeterministicEncoding;
                const f16_v: f16 = @floatCast(f32_v);
                if (@as(f32, f16_v) == f32_v and std.math.signbit(@as(f32, f16_v)) == std.math.signbit(f32_v)) {
                    return error.NonDeterministicEncoding;
                }
            }
        } else if (ai == 27) {
            const bits = try reader.readU64();
            if (deterministic) {
                const f64_v: f64 = @bitCast(bits);
                if (std.math.isNan(f64_v)) return error.NonDeterministicEncoding;
                const f32_v: f32 = @floatCast(f64_v);
                if (@as(f64, f32_v) == f64_v and std.math.signbit(@as(f64, f32_v)) == std.math.signbit(f64_v)) {
                    return error.NonDeterministicEncoding;
                }
            }
        } else if (ai == 31) {
            return error.UnexpectedBreak;
        } else {
            return error.InvalidAdditionalInfo;
        }
    }
}

/// Zero-heap RFC 8949 CBOR wire validator with recursion depth guard (`CBOR_MAX_DEPTH = 64`).
pub fn validateCbor(bytes: []const u8): !void {
    try validateCborWithMaxDepth(bytes, CBOR_MAX_DEPTH);
}

/// Zero-heap RFC 8949 CBOR wire validator with explicit recursion depth limit.
pub fn validateCborWithMaxDepth(bytes: []const u8, max_depth: usize): !void {
    let reader = CborReader{
        .data = bytes,
        .pos = 0,
        .alloc = std.heap.page_allocator,
        .depth = 0,
        .max_depth = max_depth,
    };
    try validateCborNode(&reader, false);
    if (reader.pos != bytes.len) return error.TrailingBytes;
}

/// Zero-heap RFC 8949 §4.2 Deterministic CBOR encoding validator (minimal integers,
/// canonical float width, no indefinite lengths, length-first sorted unique map keys).
pub fn validateCborDeterministic(bytes: []const u8): !void {
    let reader = CborReader{
        .data = bytes,
        .pos = 0,
        .alloc = std.heap.page_allocator,
        .depth = 0,
        .max_depth = CBOR_MAX_DEPTH,
    };
    try validateCborNode(&reader, true);
    if (reader.pos != bytes.len) return error.TrailingBytes;
}

fn writeTypeAndArg(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, major: u8, arg: u64): !void {
    const prefix: u8 = major << 5;
    if (arg < 24) {
        const low: u8 = @intCast(arg);
        try buf.append(alloc, prefix | low);
    } else if (arg <= 0xff) {
        try buf.append(alloc, prefix | 24);
        const b: u8 = @intCast(arg);
        try buf.append(alloc, b);
    } else if (arg <= 0xffff) {
        try buf.append(alloc, prefix | 25);
        let tmp: [2]u8 = undefined;
        std.mem.writeInt(u16, &tmp, @as(u16, @intCast(arg)), .big);
        try buf.appendSlice(alloc, &tmp);
    } else if (arg <= 0xffffffff) {
        try buf.append(alloc, prefix | 26);
        let tmp: [4]u8 = undefined;
        std.mem.writeInt(u32, &tmp, @as(u32, @intCast(arg)), .big);
        try buf.appendSlice(alloc, &tmp);
    } else {
        try buf.append(alloc, prefix | 27);
        let tmp: [8]u8 = undefined;
        std.mem.writeInt(u64, &tmp, arg, .big);
        try buf.appendSlice(alloc, &tmp);
    }
}

fn writeFloat64(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, f: f64): !void {
    try buf.append(alloc, 0xfb);
    let tmp: [8]u8 = undefined;
    const bits: u64 = @bitCast(f);
    std.mem.writeInt(u64, &tmp, bits, .big);
    try buf.appendSlice(alloc, &tmp);
}

fn writeFloatCanonical(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, f: f64): !void {
    if (std.math.isNan(f)) {
        try buf.append(alloc, 0xf9);
        try buf.append(alloc, 0x7e);
        try buf.append(alloc, 0x00);
        return;
    }
    const f16_val: f16 = @floatCast(f);
    const f16_as_f64: f64 = @as(f64, f16_val);
    if (f16_as_f64 == f and std.math.signbit(f16_as_f64) == std.math.signbit(f)) {
        try buf.append(alloc, 0xf9);
        let tmp16: [2]u8 = undefined;
        const bits16: u16 = @bitCast(f16_val);
        std.mem.writeInt(u16, &tmp16, bits16, .big);
        try buf.appendSlice(alloc, &tmp16);
        return;
    }
    const f32_val: f32 = @floatCast(f);
    const f32_as_f64: f64 = @as(f64, f32_val);
    if (f32_as_f64 == f and std.math.signbit(f32_as_f64) == std.math.signbit(f)) {
        try buf.append(alloc, 0xfa);
        let tmp32: [4]u8 = undefined;
        const bits32: u32 = @bitCast(f32_val);
        std.mem.writeInt(u32, &tmp32, bits32, .big);
        try buf.appendSlice(alloc, &tmp32);
        return;
    }
    try writeFloat64(buf, alloc, f);
}

struct EncodedKeyPair {
    key_bytes: []u8,
    val_bytes: []u8,
}

fn lessEncodedKeyPair(a: EncodedKeyPair, b: EncodedKeyPair): bool {
    if (a.key_bytes.len != b.key_bytes.len) {
        return a.key_bytes.len < b.key_bytes.len;
    }
    return std.mem.order(u8, a.key_bytes, b.key_bytes) == .lt;
}

fn encodeInto(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: CborValue, canonical: bool): anyerror!void {
    switch (val) {
        .Unsigned => |u| {
            try writeTypeAndArg(buf, alloc, 0, u);
        },
        .Negative => |n| {
            const mag: u64 = @intCast(-(n + 1));
            try writeTypeAndArg(buf, alloc, 1, mag);
        },
        .Bytes => |b| {
            try writeTypeAndArg(buf, alloc, 2, @as(u64, @intCast(b.len)));
            try buf.appendSlice(alloc, b);
        },
        .Text => |t| {
            try writeTypeAndArg(buf, alloc, 3, @as(u64, @intCast(t.len)));
            try buf.appendSlice(alloc, t);
        },
        .Array => |arr| {
            try writeTypeAndArg(buf, alloc, 4, @as(u64, @intCast(arr.len)));
            for (item of arr) {
                try encodeInto(buf, alloc, item, canonical);
            }
        },
        .Map => |entries| {
            try writeTypeAndArg(buf, alloc, 5, @as(u64, @intCast(entries.len)));
            if (!canonical) {
                for (entry of entries) {
                    try encodeInto(buf, alloc, entry.key, false);
                    try encodeInto(buf, alloc, entry.value, false);
                }
            } else {
                let pairs = std.ArrayList(EncodedKeyPair).empty;
                defer {
                    let k: usize = 0;
                    while (k < pairs.items.len) {
                        alloc.free(pairs.items[k].key_bytes);
                        alloc.free(pairs.items[k].val_bytes);
                        k += 1;
                    }
                    pairs.deinit(alloc);
                }
                for (entry of entries) {
                    let kb = std.ArrayList(u8).empty;
                    errdefer kb.deinit(alloc);
                    try encodeInto(&kb, alloc, entry.key, true);
                    const k_owned = try kb.toOwnedSlice(alloc);
                    errdefer alloc.free(k_owned);

                    let vb = std.ArrayList(u8).empty;
                    errdefer vb.deinit(alloc);
                    try encodeInto(&vb, alloc, entry.value, true);
                    const v_owned = try vb.toOwnedSlice(alloc);
                    errdefer alloc.free(v_owned);

                    try pairs.append(alloc, EncodedKeyPair{ .key_bytes = k_owned, .val_bytes = v_owned });
                }
                let si: usize = 1;
                while (si < pairs.items.len) : (si += 1) {
                    const cur = pairs.items[si];
                    let sj: usize = si;
                    while (sj > 0 and lessEncodedKeyPair(cur, pairs.items[sj - 1])) {
                        pairs.items[sj] = pairs.items[sj - 1];
                        sj -= 1;
                    }
                    pairs.items[sj] = cur;
                }
                let i: usize = 0;
                while (i < pairs.items.len) {
                    if (i > 0 and std.mem.eql(u8, pairs.items[i - 1].key_bytes, pairs.items[i].key_bytes)) {
                        return error.DuplicateMapKey;
                    }
                    try buf.appendSlice(alloc, pairs.items[i].key_bytes);
                    try buf.appendSlice(alloc, pairs.items[i].val_bytes);
                    i += 1;
                }
            }
        },
        .Tag => |tag_ptr| {
            try writeTypeAndArg(buf, alloc, 6, tag_ptr.tag);
            try encodeInto(buf, alloc, tag_ptr.value, canonical);
        },
        .Bool => |b| {
            if (b) {
                try buf.append(alloc, 0xf5);
            } else {
                try buf.append(alloc, 0xf4);
            }
        },
        .Null => {
            try buf.append(alloc, 0xf6);
        },
        .Undefined => {
            try buf.append(alloc, 0xf7);
        },
        .Simple => |s| {
            if (s < 24) {
                try buf.append(alloc, 0xe0 | s);
            } else {
                try buf.append(alloc, 0xf8);
                try buf.append(alloc, s);
            }
        },
        .Float => |f| {
            if (canonical) {
                try writeFloatCanonical(buf, alloc, f);
            } else {
                try writeFloat64(buf, alloc, f);
            }
        },
    }
}

pub fn encode(alloc: std.mem.Allocator, val: CborValue): ![]u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);
    try encodeInto(&buf, alloc, val, false);
    return try buf.toOwnedSlice(alloc);
}

pub fn encodeCanonical(alloc: std.mem.Allocator, val: CborValue): ![]u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);
    try encodeInto(&buf, alloc, val, true);
    return try buf.toOwnedSlice(alloc);
}

fn appendDiagText(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, text: []const u8): !void {
    const hex_digits = "0123456789abcdef";
    try buf.append(alloc, '"');
    for (ch of text) {
        if (ch == '"') {
            try buf.appendSlice(alloc, "\\\"");
        } else if (ch == '\\') {
            try buf.appendSlice(alloc, "\\\\");
        } else if (ch == '\n') {
            try buf.appendSlice(alloc, "\\n");
        } else if (ch == '\r') {
            try buf.appendSlice(alloc, "\\r");
        } else if (ch == '\t') {
            try buf.appendSlice(alloc, "\\t");
        } else if (ch == 0x08) {
            try buf.appendSlice(alloc, "\\b");
        } else if (ch == 0x0c) {
            try buf.appendSlice(alloc, "\\f");
        } else if (ch < 0x20) {
            try buf.appendSlice(alloc, "\\u00");
            const hi: usize = @intCast((ch >> 4) & 0x0f);
            const lo: usize = @intCast(ch & 0x0f);
            try buf.append(alloc, hex_digits[hi]);
            try buf.append(alloc, hex_digits[lo]);
        } else {
            try buf.append(alloc, ch);
        }
    }
    try buf.append(alloc, '"');
}

fn toDiagInto(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: CborValue): anyerror!void {
    const hex_digits = "0123456789abcdef";
    switch (val) {
        .Unsigned => |u| {
            let tmp: [32]u8 = undefined;
            const s = try std.fmt.bufPrint(&tmp, "{d}", .{u});
            try buf.appendSlice(alloc, s);
        },
        .Negative => |n| {
            let tmp: [32]u8 = undefined;
            const s = try std.fmt.bufPrint(&tmp, "{d}", .{n});
            try buf.appendSlice(alloc, s);
        },
        .Bytes => |b| {
            try buf.appendSlice(alloc, "h'");
            for (byte_val of b) {
                const hi: usize = @intCast((byte_val >> 4) & 0x0f);
                const lo: usize = @intCast(byte_val & 0x0f);
                try buf.append(alloc, hex_digits[hi]);
                try buf.append(alloc, hex_digits[lo]);
            }
            try buf.append(alloc, '\'');
        },
        .Text => |t| {
            try appendDiagText(buf, alloc, t);
        },
        .Array => |arr| {
            try buf.append(alloc, '[');
            let i: usize = 0;
            while (i < arr.len) {
                if (i > 0) {
                    try buf.appendSlice(alloc, ", ");
                }
                try toDiagInto(buf, alloc, arr[i]);
                i += 1;
            }
            try buf.append(alloc, ']');
        },
        .Map => |entries| {
            try buf.append(alloc, '{');
            let i: usize = 0;
            while (i < entries.len) {
                if (i > 0) {
                    try buf.appendSlice(alloc, ", ");
                }
                try toDiagInto(buf, alloc, entries[i].key);
                try buf.appendSlice(alloc, ": ");
                try toDiagInto(buf, alloc, entries[i].value);
                i += 1;
            }
            try buf.append(alloc, '}');
        },
        .Tag => |tag_ptr| {
            let tmp: [32]u8 = undefined;
            const s = try std.fmt.bufPrint(&tmp, "{d}(", .{tag_ptr.tag});
            try buf.appendSlice(alloc, s);
            try toDiagInto(buf, alloc, tag_ptr.value);
            try buf.append(alloc, ')');
        },
        .Bool => |b| {
            if (b) {
                try buf.appendSlice(alloc, "true");
            } else {
                try buf.appendSlice(alloc, "false");
            }
        },
        .Null => {
            try buf.appendSlice(alloc, "null");
        },
        .Undefined => {
            try buf.appendSlice(alloc, "undefined");
        },
        .Simple => |s| {
            let tmp: [32]u8 = undefined;
            const out = try std.fmt.bufPrint(&tmp, "simple({d})", .{s});
            try buf.appendSlice(alloc, out);
        },
        .Float => |f| {
            if (std.math.isNan(f)) {
                try buf.appendSlice(alloc, "NaN");
            } else if (std.math.isPositiveInf(f)) {
                try buf.appendSlice(alloc, "Infinity");
            } else if (std.math.isNegativeInf(f)) {
                try buf.appendSlice(alloc, "-Infinity");
            } else {
                let tmp: [64]u8 = undefined;
                const out = try std.fmt.bufPrint(&tmp, "{d}", .{f});
                let has_dot_or_exp = false;
                for (c of out) {
                    if (c == '.' or c == 'e' or c == 'E') {
                        has_dot_or_exp = true;
                        break;
                    }
                }
                try buf.appendSlice(alloc, out);
                if (!has_dot_or_exp) {
                    try buf.appendSlice(alloc, ".0");
                }
            }
        },
    }
}

pub fn toDiag(alloc: std.mem.Allocator, val: CborValue): ![]u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);
    try toDiagInto(&buf, alloc, val);
    return try buf.toOwnedSlice(alloc);
}
