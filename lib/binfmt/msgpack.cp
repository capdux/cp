// Reference-hardened against 3Hren/msgpack-rust (MIT)
// Source: cp/lib/msgpack/msgpack.cp (MessagePack Binary Serialization & Deserialization Engine)
// Pure CP MessagePack binary serialization & deserialization engine supporting:
//   - All 36 markers: positive/negative fixint, uint8/16/32/64, int8/16/32/64, float32/64,
//     nil, bool, fixstr/str8/16/32, bin8/16/32, fixarray/array16/32, fixmap/map16/32,
//     fixext1/2/4/8/16 & ext8/16/32
//   - MessagePack Timestamp extension (ext type = -1: 32-bit, 64-bit, 96-bit, strict nsec <= 999_999_999)
//   - Zero-heap streaming skipValue / validateMsgpack / MsgPackSliceWriter & max-depth protection
//   - 16-lane @Vector(16, u8) SIMD payload copy + cap:simd ASCII fast-path UTF-8 validation

const std = @import("std");
import { Simd } from "cap:simd";

pub const DEFAULT_MAX_DEPTH: usize = 64;
pub const MSGPACK_EXT_TIMESTAMP: i8 = -1;
pub const MSGPACK_MAX_NSEC: u32 = 999999999;

/// 16-lane SIMD 字节向量（用于大 bin/str 载荷高速拷贝）
type Vec16u8 = @Vector(16, u8);

pub err MsgPackError {
    UnexpectedEof,
    InvalidFormatTag,
    NeverUsedMarker,
    MaxDepthExceeded,
    TrailingBytes,
    IntegerOverflow,
    InvalidTimestamp,
    InvalidUtf8,
    BufferTooSmall,
    OutOfMemory,
}

/// 16-lane SIMD 加速大载荷拷贝（与逐字节标量拷贝 0 差异）
pub fn copyPayloadSimd(dst: []u8, src: []const u8) {
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

/// UTF-8 合法性校验：先走 cap:simd Simd.isAscii 全 ASCII 向量快路径，非 ASCII 时回落 RFC 3629 全量校验
pub fn isValidUtf8Simd(bytes: []const u8): bool {
    if (Simd.isAscii(bytes)) {
        return true;
    }
    return std.unicode.utf8ValidateSlice(bytes);
}

/// MessagePack Timestamp 扩展类型（ext type = -1，支持 32/64/96-bit 编解码与纳秒范围校验）
pub struct MsgTimestamp {
    pub sec: i64 = 0,
    pub nsec: u32 = 0,

    pub static fn init(sec: i64, nsec: u32): MsgTimestamp! {
        if (nsec > MSGPACK_MAX_NSEC) {
            return err.InvalidTimestamp;
        }
        return MsgTimestamp{ sec: sec, nsec: nsec };
    }

    pub fn eql(other: MsgTimestamp): bool {
        return _.sec == other.sec && _.nsec == other.nsec;
    }

    /// 编码为最紧凑的 MessagePack Timestamp 原始载荷（4 / 8 / 12 字节写入调用方缓冲区）
    pub fn encodePayload(out: *[12]u8): usize! {
        if (_.nsec > MSGPACK_MAX_NSEC) {
            return err.InvalidTimestamp;
        }
        // Timestamp 32: nsec == 0 && 0 <= sec <= 2^32 - 1 (fixext 4)
        if (_.nsec == 0 && _.sec >= 0 && _.sec <= 4294967295) {
            const s32: u32 = _.sec as u32;
            out[0] = ((s32 >> 24) & 0xFF) as u8;
            out[1] = ((s32 >> 16) & 0xFF) as u8;
            out[2] = ((s32 >> 8) & 0xFF) as u8;
            out[3] = (s32 & 0xFF) as u8;
            return 4;
        }
        // Timestamp 64: 0 <= sec <= 2^34 - 1 (fixext 8): (nsec << 34) | sec
        if (_.sec >= 0 && _.sec <= 17179869183) {
            const data64: u64 = ((_.nsec as u64) << 34) | (_.sec as u64);
            out[0] = ((data64 >> 56) & 0xFF) as u8;
            out[1] = ((data64 >> 48) & 0xFF) as u8;
            out[2] = ((data64 >> 40) & 0xFF) as u8;
            out[3] = ((data64 >> 32) & 0xFF) as u8;
            out[4] = ((data64 >> 24) & 0xFF) as u8;
            out[5] = ((data64 >> 16) & 0xFF) as u8;
            out[6] = ((data64 >> 8) & 0xFF) as u8;
            out[7] = (data64 & 0xFF) as u8;
            return 8;
        }
        // Timestamp 96: 任意 i64 秒（含负纪元）(ext 8, len=12): u32be(nsec) + i64be(sec)
        const n32: u32 = _.nsec;
        const s64: u64 = @bitCast(_.sec);
        out[0] = ((n32 >> 24) & 0xFF) as u8;
        out[1] = ((n32 >> 16) & 0xFF) as u8;
        out[2] = ((n32 >> 8) & 0xFF) as u8;
        out[3] = (n32 & 0xFF) as u8;
        out[4] = ((s64 >> 56) & 0xFF) as u8;
        out[5] = ((s64 >> 48) & 0xFF) as u8;
        out[6] = ((s64 >> 40) & 0xFF) as u8;
        out[7] = ((s64 >> 32) & 0xFF) as u8;
        out[8] = ((s64 >> 24) & 0xFF) as u8;
        out[9] = ((s64 >> 16) & 0xFF) as u8;
        out[10] = ((s64 >> 8) & 0xFF) as u8;
        out[11] = (s64 & 0xFF) as u8;
        return 12;
    }

    /// 从 MsgExt（ext_type == -1）严格解码 32/64/96-bit Timestamp
    pub static fn decodeFromExt(ext: MsgExt): MsgTimestamp! {
        if (ext.ext_type != MSGPACK_EXT_TIMESTAMP) {
            return err.InvalidTimestamp;
        }
        const d = ext.data;
        if (d.len == 4) {
            const s32: u32 = ((d[0] as u32) << 24) | ((d[1] as u32) << 16) | ((d[2] as u32) << 8) | (d[3] as u32);
            return MsgTimestamp{ sec: s32 as i64, nsec: 0 };
        }
        if (d.len == 8) {
            const v: u64 = ((d[0] as u64) << 56) | ((d[1] as u64) << 48) | ((d[2] as u64) << 40) | ((d[3] as u64) << 32) |
                ((d[4] as u64) << 24) | ((d[5] as u64) << 16) | ((d[6] as u64) << 8) | (d[7] as u64);
            const nsec: u32 = (v >> 34) as u32;
            const sec: i64 = (v & 0x3FFFFFFFF) as i64;
            if (nsec > MSGPACK_MAX_NSEC) {
                return err.InvalidTimestamp;
            }
            return MsgTimestamp{ sec: sec, nsec: nsec };
        }
        if (d.len == 12) {
            const nsec: u32 = ((d[0] as u32) << 24) | ((d[1] as u32) << 16) | ((d[2] as u32) << 8) | (d[3] as u32);
            if (nsec > MSGPACK_MAX_NSEC) {
                return err.InvalidTimestamp;
            }
            const s_bits: u64 = ((d[4] as u64) << 56) | ((d[5] as u64) << 48) | ((d[6] as u64) << 40) | ((d[7] as u64) << 32) |
                ((d[8] as u64) << 24) | ((d[9] as u64) << 16) | ((d[10] as u64) << 8) | (d[11] as u64);
            const sec: i64 = @bitCast(s_bits);
            return MsgTimestamp{ sec: sec, nsec: nsec };
        }
        return err.InvalidTimestamp;
    }
}

pub struct MsgExt {
    ext_type: i8 = 0,
    data: []const u8 = "",

    pub static fn init(ext_type: i8, data: []const u8): MsgExt {
        return MsgExt{ ext_type: ext_type, data: data };
    }
}

pub struct MsgMapEntry {
    key: MsgValue = .nil,
    value: MsgValue = .nil,

    pub static fn init(key: MsgValue, value: MsgValue): MsgMapEntry {
        return MsgMapEntry{ key: key, value: value };
    }
}

pub enum MsgValue {
    nil,
    boolean: bool,
    int: i64,
    uint: u64,
    float: f64,
    str: []const u8,
    bin: []const u8,
    array: []MsgValue,
    map: []MsgMapEntry,
    ext: MsgExt,

    pub static fn makeNil(): MsgValue {
        return .nil;
    }

    pub static fn makeBool(v: bool): MsgValue {
        return MsgValue{ .boolean = v };
    }

    pub static fn makeInt(v: i64): MsgValue {
        return MsgValue{ .int = v };
    }

    pub static fn makeUint(v: u64): MsgValue {
        return MsgValue{ .uint = v };
    }

    pub static fn makeFloat(v: f64): MsgValue {
        return MsgValue{ .float = v };
    }

    pub static fn makeStr(v: []const u8): MsgValue {
        return MsgValue{ .str = v };
    }

    pub static fn makeBin(v: []const u8): MsgValue {
        return MsgValue{ .bin = v };
    }

    pub static fn makeArray(items: []MsgValue): MsgValue {
        return MsgValue{ .array = items };
    }

    pub static fn makeMap(entries: []MsgMapEntry): MsgValue {
        return MsgValue{ .map = entries };
    }

    pub static fn makeExt(ext_type: i8, data: []const u8): MsgValue {
        return MsgValue{ .ext = MsgExt.init(ext_type, data) };
    }

    pub fn isNil(): bool {
        return switch (_) {
            .nil => true,
            else => false,
        };
    }

    pub fn asBool(): bool? {
        return switch (_) {
            .boolean b => b,
            else => null,
        };
    }

    pub fn asInt(): i64? {
        return switch (_) {
            .int i => i,
            .uint u => if (u <= 9223372036854775807) (u as i64) else null,
            else => null,
        };
    }

    pub fn asUint(): u64? {
        return switch (_) {
            .uint u => u,
            .int i => if (i >= 0) (i as u64) else null,
            else => null,
        };
    }

    pub fn asFloat(): f64? {
        return switch (_) {
            .float f => f,
            .int i => @as(f64, @floatFromInt(i)),
            .uint u => @as(f64, @floatFromInt(u)),
            else => null,
        };
    }

    pub fn asStr(): ([]const u8)? {
        return switch (_) {
            .str s => s,
            else => null,
        };
    }

    pub fn asBin(): ([]const u8)? {
        return switch (_) {
            .bin b => b,
            else => null,
        };
    }

    pub fn asArray(): ([]MsgValue)? {
        return switch (_) {
            .array a => a,
            else => null,
        };
    }

    pub fn asMap(): ([]MsgMapEntry)? {
        return switch (_) {
            .map m => m,
            else => null,
        };
    }

    pub fn asExt(): MsgExt? {
        return switch (_) {
            .ext e => e,
            else => null,
        };
    }

    pub fn get(key_str: []const u8): MsgValue? {
        const entries = _.asMap() ?? return null;
        for (entry of entries) {
            const k = entry.key.asStr() ?? continue;
            if (std.mem.eql(u8, k, key_str)) {
                return entry.value;
            }
        }
        return null;
    }

    pub fn eql(other: MsgValue): bool {
        return switch (_) {
            .nil => other.isNil(),
            .boolean b => {
                const ob = other.asBool() ?? return false;
                return b == ob;
            },
            .int i => {
                const oi = other.asInt() ?? return false;
                return i == oi;
            },
            .uint u => {
                const ou = other.asUint() ?? return false;
                return u == ou;
            },
            .float f => {
                return switch (other) {
                    .float o_float => {
                        if (std.math.isNan(f) && std.math.isNan(o_float)) return true;
                        return f == o_float;
                    },
                    else => false,
                };
            },
            .str s => {
                const os = other.asStr() ?? return false;
                return std.mem.eql(u8, s, os);
            },
            .bin b => {
                const ob = other.asBin() ?? return false;
                return std.mem.eql(u8, b, ob);
            },
            .array arr => {
                const oarr = other.asArray() ?? return false;
                if (arr.len != oarr.len) return false;
                let i: usize = 0;
                while (i < arr.len) : (i += 1) {
                    if (!arr[i].eql(oarr[i])) return false;
                }
                return true;
            },
            .map entries => {
                const oentries = other.asMap() ?? return false;
                if (entries.len != oentries.len) return false;
                let i: usize = 0;
                while (i < entries.len) : (i += 1) {
                    if (!entries[i].key.eql(oentries[i].key)) return false;
                    if (!entries[i].value.eql(oentries[i].value)) return false;
                }
                return true;
            },
            .ext e => {
                const oe = other.asExt() ?? return false;
                return e.ext_type == oe.ext_type && std.mem.eql(u8, e.data, oe.data);
            },
        };
    }

    /// Recursively frees heap-allocated `array` and `map` containers.
    /// Zero-copy `str`, `bin`, and `ext.data` slices borrow from the input buffer and are not freed.
    pub fn deinit(allocator: std.mem.Allocator) {
        switch (_) {
            .array arr => {
                for (item of arr) {
                    item.deinit(allocator);
                }
                allocator.free(arr);
            },
            .map entries => {
                for (entry of entries) {
                    entry.key.deinit(allocator);
                    entry.value.deinit(allocator);
                }
                allocator.free(entries);
            },
            else => {},
        }
    }
}

pub struct MsgPackWriter {
    alloc: std.mem.Allocator = undefined,
    buf: std.ArrayListUnmanaged(u8) = std.ArrayListUnmanaged(u8).empty,

    pub static fn init(allocator: std.mem.Allocator): MsgPackWriter {
        return MsgPackWriter{
            alloc: allocator,
            buf: std.ArrayListUnmanaged(u8).empty,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    pub fn bytes(): []const u8 {
        return _.buf.items;
    }

    pub fn toOwnedSlice(*_): ([]u8)! {
        return _.buf.toOwnedSlice(_.alloc) catch return err.OutOfMemory;
    }

    fn writeByte(*_, b: u8): void! {
        _.buf.append(_.alloc, b) catch return err.OutOfMemory;
    }

    fn writeSlice(*_, s: []const u8): void! {
        if (s.len == 0) return;
        const old_len = _.buf.items.len;
        _.buf.resize(_.alloc, old_len + s.len) catch return err.OutOfMemory;
        copyPayloadSimd(_.buf.items[old_len .. old_len + s.len], s);
    }

    fn writeU16(*_, v: u16): void! {
        const tmp: [2]u8 = [
            ((v >> 8) & 0xFF) as u8,
            (v & 0xFF) as u8,
        ];
        _.writeSlice(&tmp)?;
    }

    fn writeU32(*_, v: u32): void! {
        const tmp: [4]u8 = [
            ((v >> 24) & 0xFF) as u8,
            ((v >> 16) & 0xFF) as u8,
            ((v >> 8) & 0xFF) as u8,
            (v & 0xFF) as u8,
        ];
        _.writeSlice(&tmp)?;
    }

    fn writeU64(*_, v: u64): void! {
        const tmp: [8]u8 = [
            ((v >> 56) & 0xFF) as u8,
            ((v >> 48) & 0xFF) as u8,
            ((v >> 40) & 0xFF) as u8,
            ((v >> 32) & 0xFF) as u8,
            ((v >> 24) & 0xFF) as u8,
            ((v >> 16) & 0xFF) as u8,
            ((v >> 8) & 0xFF) as u8,
            (v & 0xFF) as u8,
        ];
        _.writeSlice(&tmp)?;
    }

    pub fn writeNil(*_): void! {
        _.writeByte(0xC0)?;
    }

    pub fn writeBool(*_, v: bool): void! {
        _.writeByte(if (v) 0xC3 else 0xC2)?;
    }

    pub fn writeUint(*_, v: u64): void! {
        if (v <= 0x7F) {
            _.writeByte(v as u8)?;
        } else if (v <= 0xFF) {
            _.writeByte(0xCC)?;
            _.writeByte(v as u8)?;
        } else if (v <= 0xFFFF) {
            _.writeByte(0xCD)?;
            _.writeU16(v as u16)?;
        } else if (v <= 0xFFFFFFFF) {
            _.writeByte(0xCE)?;
            _.writeU32(v as u32)?;
        } else {
            _.writeByte(0xCF)?;
            _.writeU64(v)?;
        }
    }

    pub fn writeInt(*_, v: i64): void! {
        if (v >= 0) {
            _.writeUint(v as u64)?;
            return;
        }
        if (v >= -32) {
            const b: u8 = @bitCast(@as(i8, @intCast(v)));
            _.writeByte(b)?;
        } else if (v >= -128) {
            _.writeByte(0xD0)?;
            const b: u8 = @bitCast(@as(i8, @intCast(v)));
            _.writeByte(b)?;
        } else if (v >= -32768) {
            _.writeByte(0xD1)?;
            const bits: u16 = @bitCast(@as(i16, @intCast(v)));
            _.writeU16(bits)?;
        } else if (v >= -2147483648) {
            _.writeByte(0xD2)?;
            const bits: u32 = @bitCast(@as(i32, @intCast(v)));
            _.writeU32(bits)?;
        } else {
            _.writeByte(0xD3)?;
            const bits: u64 = @bitCast(v);
            _.writeU64(bits)?;
        }
    }

    pub fn writeFloat(*_, v: f64): void! {
        _.writeByte(0xCB)?;
        const bits: u64 = @bitCast(v);
        _.writeU64(bits)?;
    }

    pub fn writeFloat32(*_, v: f32): void! {
        _.writeByte(0xCA)?;
        const bits: u32 = @bitCast(v);
        _.writeU32(bits)?;
    }

    pub fn writeStr(*_, s: []const u8): void! {
        if (s.len <= 31) {
            _.writeByte((0xA0 | s.len) as u8)?;
        } else if (s.len <= 0xFF) {
            _.writeByte(0xD9)?;
            _.writeByte(s.len as u8)?;
        } else if (s.len <= 0xFFFF) {
            _.writeByte(0xDA)?;
            _.writeU16(s.len as u16)?;
        } else if (s.len <= 0xFFFFFFFF) {
            _.writeByte(0xDB)?;
            _.writeU32(s.len as u32)?;
        } else {
            return err.IntegerOverflow;
        }
        _.writeSlice(s)?;
    }

    pub fn writeBin(*_, b: []const u8): void! {
        if (b.len <= 0xFF) {
            _.writeByte(0xC4)?;
            _.writeByte(b.len as u8)?;
        } else if (b.len <= 0xFFFF) {
            _.writeByte(0xC5)?;
            _.writeU16(b.len as u16)?;
        } else if (b.len <= 0xFFFFFFFF) {
            _.writeByte(0xC6)?;
            _.writeU32(b.len as u32)?;
        } else {
            return err.IntegerOverflow;
        }
        _.writeSlice(b)?;
    }

    pub fn writeArrayHeader(*_, len: usize): void! {
        if (len <= 15) {
            _.writeByte((0x90 | len) as u8)?;
        } else if (len <= 0xFFFF) {
            _.writeByte(0xDC)?;
            _.writeU16(len as u16)?;
        } else if (len <= 0xFFFFFFFF) {
            _.writeByte(0xDD)?;
            _.writeU32(len as u32)?;
        } else {
            return err.IntegerOverflow;
        }
    }

    pub fn writeMapHeader(*_, len: usize): void! {
        if (len <= 15) {
            _.writeByte((0x80 | len) as u8)?;
        } else if (len <= 0xFFFF) {
            _.writeByte(0xDE)?;
            _.writeU16(len as u16)?;
        } else if (len <= 0xFFFFFFFF) {
            _.writeByte(0xDF)?;
            _.writeU32(len as u32)?;
        } else {
            return err.IntegerOverflow;
        }
    }

    pub fn writeExt(*_, ext_type: i8, data: []const u8): void! {
        const t_byte: u8 = @bitCast(ext_type);
        if (data.len == 1) {
            _.writeByte(0xD4)?;
            _.writeByte(t_byte)?;
        } else if (data.len == 2) {
            _.writeByte(0xD5)?;
            _.writeByte(t_byte)?;
        } else if (data.len == 4) {
            _.writeByte(0xD6)?;
            _.writeByte(t_byte)?;
        } else if (data.len == 8) {
            _.writeByte(0xD7)?;
            _.writeByte(t_byte)?;
        } else if (data.len == 16) {
            _.writeByte(0xD8)?;
            _.writeByte(t_byte)?;
        } else if (data.len <= 0xFF) {
            _.writeByte(0xC7)?;
            _.writeByte(data.len as u8)?;
            _.writeByte(t_byte)?;
        } else if (data.len <= 0xFFFF) {
            _.writeByte(0xC8)?;
            _.writeU16(data.len as u16)?;
            _.writeByte(t_byte)?;
        } else if (data.len <= 0xFFFFFFFF) {
            _.writeByte(0xC9)?;
            _.writeU32(data.len as u32)?;
            _.writeByte(t_byte)?;
        } else {
            return err.IntegerOverflow;
        }
        _.writeSlice(data)?;
    }

    /// 写入 MessagePack Timestamp 扩展类型（自动按规范选择 32/64/96-bit 最紧凑编码）
    pub fn writeTimestamp(*_, ts: MsgTimestamp): void! {
        let payload: [12]u8 = [0; 12];
        const n = ts.encodePayload(&payload)?;
        _.writeExt(MSGPACK_EXT_TIMESTAMP, payload[0..n])?;
    }

    pub fn encodeValue(*_, value: MsgValue): void! {
        switch (value) {
            .nil => _.writeNil()?,
            .boolean b => _.writeBool(b)?,
            .int i => _.writeInt(i)?,
            .uint u => _.writeUint(u)?,
            .float f => _.writeFloat(f)?,
            .str s => _.writeStr(s)?,
            .bin b => _.writeBin(b)?,
            .array items => {
                _.writeArrayHeader(items.len)?;
                for (item of items) {
                    _.encodeValue(item)?;
                }
            },
            .map entries => {
                _.writeMapHeader(entries.len)?;
                for (entry of entries) {
                    _.encodeValue(entry.key)?;
                    _.encodeValue(entry.value)?;
                }
            },
            .ext e => _.writeExt(e.ext_type, e.data)?,
        }
    }
}

pub struct MsgPackReader {
    alloc: std.mem.Allocator = undefined,
    src: []const u8 = "",
    pos: usize = 0,
    max_depth: usize = DEFAULT_MAX_DEPTH,

    pub static fn init(allocator: std.mem.Allocator, data: []const u8): MsgPackReader {
        return MsgPackReader{
            alloc: allocator,
            src: data,
            pos: 0,
            max_depth: DEFAULT_MAX_DEPTH,
        };
    }

    pub static fn initWithMaxDepth(allocator: std.mem.Allocator, data: []const u8, max_depth: usize): MsgPackReader {
        return MsgPackReader{
            alloc: allocator,
            src: data,
            pos: 0,
            max_depth: max_depth,
        };
    }

    pub fn remaining(): usize {
        if (_.pos >= _.src.len) return 0;
        return _.src.len - _.pos;
    }

    pub fn isAtEnd(): bool {
        return _.pos >= _.src.len;
    }

    fn readByte(*_): u8! {
        if (_.pos >= _.src.len) {
            return err.UnexpectedEof;
        }
        const b = _.src[_.pos];
        _.pos += 1;
        return b;
    }

    fn readSlice(*_, n: usize): ([]const u8)! {
        if (n > _.remaining()) {
            return err.UnexpectedEof;
        }
        const s = _.src[_.pos .. _.pos + n];
        _.pos += n;
        return s;
    }

    fn readU16(*_): u16! {
        const s = _.readSlice(2)?;
        return ((s[0] as u16) << 8) | (s[1] as u16);
    }

    fn readU32(*_): u32! {
        const s = _.readSlice(4)?;
        return ((s[0] as u32) << 24) |
            ((s[1] as u32) << 16) |
            ((s[2] as u32) << 8) |
            (s[3] as u32);
    }

    fn readU64(*_): u64! {
        const s = _.readSlice(8)?;
        return ((s[0] as u64) << 56) |
            ((s[1] as u64) << 48) |
            ((s[2] as u64) << 40) |
            ((s[3] as u64) << 32) |
            ((s[4] as u64) << 24) |
            ((s[5] as u64) << 16) |
            ((s[6] as u64) << 8) |
            (s[7] as u64);
    }

    fn readExtWithLen(*_, len: usize): MsgValue! {
        const t_byte = _.readByte()?;
        const ext_type: i8 = @bitCast(t_byte);
        const data = _.readSlice(len)?;
        return MsgValue.makeExt(ext_type, data);
    }

    fn readArrayWithLen(*_, count: usize, depth: usize): MsgValue! {
        if (depth >= _.max_depth) {
            return err.MaxDepthExceeded;
        }
        if (count > _.remaining()) {
            return err.UnexpectedEof;
        }
        let items = _.alloc.alloc(MsgValue, count) catch return err.OutOfMemory;
        let initialized: usize = 0;
        errdefer {
            let j: usize = 0;
            while (j < initialized) : (j += 1) {
                items[j].deinit(_.alloc);
            }
            _.alloc.free(items);
        }
        while (initialized < count) : (initialized += 1) {
            items[initialized] = _.readValueAtDepth(depth + 1)?;
        }
        return MsgValue.makeArray(items);
    }

    fn readMapWithLen(*_, count: usize, depth: usize): MsgValue! {
        if (depth >= _.max_depth) {
            return err.MaxDepthExceeded;
        }
        if (count > _.remaining() / 2 && count > 0) {
            return err.UnexpectedEof;
        }
        let entries = _.alloc.alloc(MsgMapEntry, count) catch return err.OutOfMemory;
        let initialized: usize = 0;
        errdefer {
            let j: usize = 0;
            while (j < initialized) : (j += 1) {
                entries[j].key.deinit(_.alloc);
                entries[j].value.deinit(_.alloc);
            }
            _.alloc.free(entries);
        }
        while (initialized < count) : (initialized += 1) {
            const k = _.readValueAtDepth(depth + 1)?;
            errdefer k.deinit(_.alloc);
            const v = _.readValueAtDepth(depth + 1)?;
            entries[initialized] = MsgMapEntry.init(k, v);
        }
        return MsgValue.makeMap(entries);
    }

    pub fn readValue(*_): MsgValue! {
        return _.readValueAtDepth(0);
    }

    /// 读取并解码一个 MessagePack Timestamp（ext_type = -1，校验 32/64/96-bit 与 nsec <= 999_999_999）
    pub fn readTimestamp(*_): MsgTimestamp! {
        const val = _.readValue()?;
        const ext = val.asExt() ?? return err.InvalidTimestamp;
        return MsgTimestamp.decodeFromExt(ext);
    }

    /// 读取并严格校验 UTF-8 字符串（走 cap:simd ASCII 快通路）
    pub fn readStrStrictUtf8(*_): ([]const u8)! {
        const tag = _.readByte()?;
        let len: usize = 0;
        if (tag >= 0xA0 && tag <= 0xBF) {
            len = (tag & 0x1F) as usize;
        } else if (tag == 0xD9) {
            len = (_.readByte()?) as usize;
        } else if (tag == 0xDA) {
            len = (_.readU16()?) as usize;
        } else if (tag == 0xDB) {
            len = (_.readU32()?) as usize;
        } else {
            return err.InvalidFormatTag;
        }
        const s = _.readSlice(len)?;
        if (!isValidUtf8Simd(s)) {
            return err.InvalidUtf8;
        }
        return s;
    }

    /// 零堆流式跳读任意嵌套 MessagePack 值（严格校验 0xC1 NeverUsedMarker、边界溢出与深度上限）
    pub fn skipValue(*_): void! {
        _.skipValueAtDepth(0)?;
    }

    fn skipBytes(*_, n: usize): void! {
        if (n > _.remaining()) {
            return err.UnexpectedEof;
        }
        _.pos += n;
    }

    fn skipValueAtDepth(*_, depth: usize): void! {
        const tag = _.readByte()?;

        if (tag <= 0x7F || tag >= 0xE0) {
            return;
        }
        if (tag >= 0x80 && tag <= 0x8F) {
            const count: usize = (tag & 0x0F) as usize;
            _.skipMapEntries(count, depth)?;
            return;
        }
        if (tag >= 0x90 && tag <= 0x9F) {
            const count: usize = (tag & 0x0F) as usize;
            _.skipArrayItems(count, depth)?;
            return;
        }
        if (tag >= 0xA0 && tag <= 0xBF) {
            const len: usize = (tag & 0x1F) as usize;
            _.skipBytes(len)?;
            return;
        }

        switch (tag) {
            0xC0 => {},
            0xC1 => return err.NeverUsedMarker,
            0xC2 => {},
            0xC3 => {},
            0xC4 => {
                const len: usize = (_.readByte()?) as usize;
                _.skipBytes(len)?;
            },
            0xC5 => {
                const len: usize = (_.readU16()?) as usize;
                _.skipBytes(len)?;
            },
            0xC6 => {
                const len: usize = (_.readU32()?) as usize;
                _.skipBytes(len)?;
            },
            0xC7 => {
                const len: usize = (_.readByte()?) as usize;
                _.skipBytes(1)?;
                _.skipBytes(len)?;
            },
            0xC8 => {
                const len: usize = (_.readU16()?) as usize;
                _.skipBytes(1)?;
                _.skipBytes(len)?;
            },
            0xC9 => {
                const len: usize = (_.readU32()?) as usize;
                _.skipBytes(1)?;
                _.skipBytes(len)?;
            },
            0xCA => _.skipBytes(4)?,
            0xCB => _.skipBytes(8)?,
            0xCC => _.skipBytes(1)?,
            0xCD => _.skipBytes(2)?,
            0xCE => _.skipBytes(4)?,
            0xCF => _.skipBytes(8)?,
            0xD0 => _.skipBytes(1)?,
            0xD1 => _.skipBytes(2)?,
            0xD2 => _.skipBytes(4)?,
            0xD3 => _.skipBytes(8)?,
            0xD4 => _.skipBytes(2)?,
            0xD5 => _.skipBytes(3)?,
            0xD6 => _.skipBytes(5)?,
            0xD7 => _.skipBytes(9)?,
            0xD8 => _.skipBytes(17)?,
            0xD9 => {
                const len: usize = (_.readByte()?) as usize;
                _.skipBytes(len)?;
            },
            0xDA => {
                const len: usize = (_.readU16()?) as usize;
                _.skipBytes(len)?;
            },
            0xDB => {
                const len: usize = (_.readU32()?) as usize;
                _.skipBytes(len)?;
            },
            0xDC => {
                const count: usize = (_.readU16()?) as usize;
                _.skipArrayItems(count, depth)?;
            },
            0xDD => {
                const count: usize = (_.readU32()?) as usize;
                _.skipArrayItems(count, depth)?;
            },
            0xDE => {
                const count: usize = (_.readU16()?) as usize;
                _.skipMapEntries(count, depth)?;
            },
            0xDF => {
                const count: usize = (_.readU32()?) as usize;
                _.skipMapEntries(count, depth)?;
            },
            else => return err.InvalidFormatTag,
        }
    }

    fn skipArrayItems(*_, count: usize, depth: usize): void! {
        if (depth >= _.max_depth) {
            return err.MaxDepthExceeded;
        }
        if (count > _.remaining()) {
            return err.UnexpectedEof;
        }
        let i: usize = 0;
        while (i < count) : (i += 1) {
            _.skipValueAtDepth(depth + 1)?;
        }
    }

    fn skipMapEntries(*_, count: usize, depth: usize): void! {
        if (depth >= _.max_depth) {
            return err.MaxDepthExceeded;
        }
        if (count > 0 && count > _.remaining() / 2) {
            return err.UnexpectedEof;
        }
        let i: usize = 0;
        while (i < count) : (i += 1) {
            _.skipValueAtDepth(depth + 1)?;
            _.skipValueAtDepth(depth + 1)?;
        }
    }

    pub fn readValueAtDepth(*_, depth: usize): MsgValue! {
        const tag = _.readByte()?;

        // Positive fixint (0x00 .. 0x7F)
        if (tag <= 0x7F) {
            return MsgValue.makeUint(tag as u64);
        }

        // Fixmap (0x80 .. 0x8F)
        if (tag >= 0x80 && tag <= 0x8F) {
            const count: usize = (tag & 0x0F) as usize;
            return _.readMapWithLen(count, depth);
        }

        // Fixarray (0x90 .. 0x9F)
        if (tag >= 0x90 && tag <= 0x9F) {
            const count: usize = (tag & 0x0F) as usize;
            return _.readArrayWithLen(count, depth);
        }

        // Fixstr (0xA0 .. 0xBF)
        if (tag >= 0xA0 && tag <= 0xBF) {
            const len: usize = (tag & 0x1F) as usize;
            const s = _.readSlice(len)?;
            return MsgValue.makeStr(s);
        }

        // Negative fixint (0xE0 .. 0xFF)
        if (tag >= 0xE0) {
            const signed_val: i8 = @bitCast(tag);
            return MsgValue.makeInt(signed_val as i64);
        }

        return switch (tag) {
            0xC0 => MsgValue.nil,
            0xC1 => return err.InvalidFormatTag,
            0xC2 => MsgValue.makeBool(false),
            0xC3 => MsgValue.makeBool(true),
            0xC4 => {
                const len: usize = (_.readByte()?) as usize;
                const b = _.readSlice(len)?;
                return MsgValue.makeBin(b);
            },
            0xC5 => {
                const len: usize = (_.readU16()?) as usize;
                const b = _.readSlice(len)?;
                return MsgValue.makeBin(b);
            },
            0xC6 => {
                const len: usize = (_.readU32()?) as usize;
                const b = _.readSlice(len)?;
                return MsgValue.makeBin(b);
            },
            0xC7 => {
                const len: usize = (_.readByte()?) as usize;
                return _.readExtWithLen(len);
            },
            0xC8 => {
                const len: usize = (_.readU16()?) as usize;
                return _.readExtWithLen(len);
            },
            0xC9 => {
                const len: usize = (_.readU32()?) as usize;
                return _.readExtWithLen(len);
            },
            0xCA => {
                const bits = _.readU32()?;
                const f: f32 = @bitCast(bits);
                return MsgValue.makeFloat(f as f64);
            },
            0xCB => {
                const bits = _.readU64()?;
                const f: f64 = @bitCast(bits);
                return MsgValue.makeFloat(f);
            },
            0xCC => {
                const v = _.readByte()?;
                return MsgValue.makeUint(v as u64);
            },
            0xCD => {
                const v = _.readU16()?;
                return MsgValue.makeUint(v as u64);
            },
            0xCE => {
                const v = _.readU32()?;
                return MsgValue.makeUint(v as u64);
            },
            0xCF => {
                const v = _.readU64()?;
                return MsgValue.makeUint(v);
            },
            0xD0 => {
                const raw = _.readByte()?;
                const v: i8 = @bitCast(raw);
                return MsgValue.makeInt(v as i64);
            },
            0xD1 => {
                const raw = _.readU16()?;
                const v: i16 = @bitCast(raw);
                return MsgValue.makeInt(v as i64);
            },
            0xD2 => {
                const raw = _.readU32()?;
                const v: i32 = @bitCast(raw);
                return MsgValue.makeInt(v as i64);
            },
            0xD3 => {
                const raw = _.readU64()?;
                const v: i64 = @bitCast(raw);
                return MsgValue.makeInt(v);
            },
            0xD4 => _.readExtWithLen(1),
            0xD5 => _.readExtWithLen(2),
            0xD6 => _.readExtWithLen(4),
            0xD7 => _.readExtWithLen(8),
            0xD8 => _.readExtWithLen(16),
            0xD9 => {
                const len: usize = (_.readByte()?) as usize;
                const s = _.readSlice(len)?;
                return MsgValue.makeStr(s);
            },
            0xDA => {
                const len: usize = (_.readU16()?) as usize;
                const s = _.readSlice(len)?;
                return MsgValue.makeStr(s);
            },
            0xDB => {
                const len: usize = (_.readU32()?) as usize;
                const s = _.readSlice(len)?;
                return MsgValue.makeStr(s);
            },
            0xDC => {
                const count: usize = (_.readU16()?) as usize;
                return _.readArrayWithLen(count, depth);
            },
            0xDD => {
                const count: usize = (_.readU32()?) as usize;
                return _.readArrayWithLen(count, depth);
            },
            0xDE => {
                const count: usize = (_.readU16()?) as usize;
                return _.readMapWithLen(count, depth);
            },
            0xDF => {
                const count: usize = (_.readU32()?) as usize;
                return _.readMapWithLen(count, depth);
            },
            else => return err.InvalidFormatTag,
        };
    }
}

pub fn encode(allocator: std.mem.Allocator, value: MsgValue): ([]u8)! {
    let writer = MsgPackWriter.init(allocator);
    errdefer writer.deinit();
    writer.encodeValue(value)?;
    return writer.toOwnedSlice();
}

pub fn decode(allocator: std.mem.Allocator, bytes: []const u8): MsgValue! {
    return decodeWithMaxDepth(allocator, bytes, DEFAULT_MAX_DEPTH);
}

pub fn decodeWithMaxDepth(allocator: std.mem.Allocator, bytes: []const u8, max_depth: usize): MsgValue! {
    let reader = MsgPackReader.initWithMaxDepth(allocator, bytes, max_depth);
    let value = reader.readValue()?;
    if (!reader.isAtEnd()) {
        value.deinit(allocator);
        return err.TrailingBytes;
    }
    return value;
}

/// 零堆流式校验完整 MessagePack 缓冲区，返回总字节数（拒绝 0xC1 NeverUsedMarker、截断、越界、尾随字节与超深嵌套）
pub fn validateMsgpack(bytes: []const u8): usize! {
    return validateMsgpackWithMaxDepth(bytes, DEFAULT_MAX_DEPTH);
}

pub fn validateMsgpackWithMaxDepth(bytes: []const u8, max_depth: usize): usize! {
    let reader = MsgPackReader.initWithMaxDepth(std.heap.page_allocator, bytes, max_depth);
    reader.skipValue()?;
    if (!reader.isAtEnd()) {
        return err.TrailingBytes;
    }
    return reader.pos;
}

/// 零堆外部切片 MessagePack 编码器（支持全 36 种 Marker、Timestamp 及 16-lane SIMD 载荷拷贝）
pub struct MsgPackSliceWriter {
    dst: []u8 = [],
    pos: usize = 0,

    pub static fn init(dst: []u8): MsgPackSliceWriter {
        return MsgPackSliceWriter{ dst: dst, pos: 0 };
    }

    pub fn written(): []const u8 {
        return _.dst[0.._.pos];
    }

    fn writeByte(*_, b: u8): void! {
        if (_.pos >= _.dst.len) return err.BufferTooSmall;
        _.dst[_.pos] = b;
        _.pos += 1;
    }

    fn writeSlice(*_, s: []const u8): void! {
        if (s.len > _.dst.len - _.pos) return err.BufferTooSmall;
        copyPayloadSimd(_.dst[_.pos .. _.pos + s.len], s);
        _.pos += s.len;
    }

    fn writeU16(*_, v: u16): void! {
        const tmp: [2]u8 = [((v >> 8) & 0xFF) as u8, (v & 0xFF) as u8];
        _.writeSlice(tmp[0..])?;
    }

    fn writeU32(*_, v: u32): void! {
        const tmp: [4]u8 = [
            ((v >> 24) & 0xFF) as u8,
            ((v >> 16) & 0xFF) as u8,
            ((v >> 8) & 0xFF) as u8,
            (v & 0xFF) as u8,
        ];
        _.writeSlice(tmp[0..])?;
    }

    fn writeU64(*_, v: u64): void! {
        const tmp: [8]u8 = [
            ((v >> 56) & 0xFF) as u8,
            ((v >> 48) & 0xFF) as u8,
            ((v >> 40) & 0xFF) as u8,
            ((v >> 32) & 0xFF) as u8,
            ((v >> 24) & 0xFF) as u8,
            ((v >> 16) & 0xFF) as u8,
            ((v >> 8) & 0xFF) as u8,
            (v & 0xFF) as u8,
        ];
        _.writeSlice(tmp[0..])?;
    }

    pub fn writeNil(*_): void! { _.writeByte(0xC0)?; }
    pub fn writeBool(*_, v: bool): void! { _.writeByte(if (v) 0xC3 else 0xC2)?; }

    pub fn writeUint(*_, v: u64): void! {
        if (v <= 0x7F) {
            _.writeByte(v as u8)?;
        } else if (v <= 0xFF) {
            _.writeByte(0xCC)?;
            _.writeByte(v as u8)?;
        } else if (v <= 0xFFFF) {
            _.writeByte(0xCD)?;
            _.writeU16(v as u16)?;
        } else if (v <= 0xFFFFFFFF) {
            _.writeByte(0xCE)?;
            _.writeU32(v as u32)?;
        } else {
            _.writeByte(0xCF)?;
            _.writeU64(v)?;
        }
    }

    pub fn writeInt(*_, v: i64): void! {
        if (v >= 0) {
            _.writeUint(v as u64)?;
            return;
        }
        if (v >= -32) {
            const b: u8 = @bitCast(@as(i8, @intCast(v)));
            _.writeByte(b)?;
        } else if (v >= -128) {
            _.writeByte(0xD0)?;
            const b: u8 = @bitCast(@as(i8, @intCast(v)));
            _.writeByte(b)?;
        } else if (v >= -32768) {
            _.writeByte(0xD1)?;
            const bits: u16 = @bitCast(@as(i16, @intCast(v)));
            _.writeU16(bits)?;
        } else if (v >= -2147483648) {
            _.writeByte(0xD2)?;
            const bits: u32 = @bitCast(@as(i32, @intCast(v)));
            _.writeU32(bits)?;
        } else {
            _.writeByte(0xD3)?;
            const bits: u64 = @bitCast(v);
            _.writeU64(bits)?;
        }
    }

    pub fn writeFloat32(*_, v: f32): void! {
        _.writeByte(0xCA)?;
        const bits: u32 = @bitCast(v);
        _.writeU32(bits)?;
    }

    pub fn writeFloat(*_, v: f64): void! {
        _.writeByte(0xCB)?;
        const bits: u64 = @bitCast(v);
        _.writeU64(bits)?;
    }

    pub fn writeStr(*_, s: []const u8): void! {
        if (s.len <= 31) {
            _.writeByte((0xA0 | s.len) as u8)?;
        } else if (s.len <= 0xFF) {
            _.writeByte(0xD9)?;
            _.writeByte(s.len as u8)?;
        } else if (s.len <= 0xFFFF) {
            _.writeByte(0xDA)?;
            _.writeU16(s.len as u16)?;
        } else if (s.len <= 0xFFFFFFFF) {
            _.writeByte(0xDB)?;
            _.writeU32(s.len as u32)?;
        } else {
            return err.IntegerOverflow;
        }
        _.writeSlice(s)?;
    }

    pub fn writeBin(*_, b: []const u8): void! {
        if (b.len <= 0xFF) {
            _.writeByte(0xC4)?;
            _.writeByte(b.len as u8)?;
        } else if (b.len <= 0xFFFF) {
            _.writeByte(0xC5)?;
            _.writeU16(b.len as u16)?;
        } else if (b.len <= 0xFFFFFFFF) {
            _.writeByte(0xC6)?;
            _.writeU32(b.len as u32)?;
        } else {
            return err.IntegerOverflow;
        }
        _.writeSlice(b)?;
    }

    pub fn writeArrayHeader(*_, len: usize): void! {
        if (len <= 15) {
            _.writeByte((0x90 | len) as u8)?;
        } else if (len <= 0xFFFF) {
            _.writeByte(0xDC)?;
            _.writeU16(len as u16)?;
        } else if (len <= 0xFFFFFFFF) {
            _.writeByte(0xDD)?;
            _.writeU32(len as u32)?;
        } else {
            return err.IntegerOverflow;
        }
    }

    pub fn writeMapHeader(*_, len: usize): void! {
        if (len <= 15) {
            _.writeByte((0x80 | len) as u8)?;
        } else if (len <= 0xFFFF) {
            _.writeByte(0xDE)?;
            _.writeU16(len as u16)?;
        } else if (len <= 0xFFFFFFFF) {
            _.writeByte(0xDF)?;
            _.writeU32(len as u32)?;
        } else {
            return err.IntegerOverflow;
        }
    }

    pub fn writeExt(*_, ext_type: i8, data: []const u8): void! {
        const t_byte: u8 = @bitCast(ext_type);
        if (data.len == 1) {
            _.writeByte(0xD4)?;
            _.writeByte(t_byte)?;
        } else if (data.len == 2) {
            _.writeByte(0xD5)?;
            _.writeByte(t_byte)?;
        } else if (data.len == 4) {
            _.writeByte(0xD6)?;
            _.writeByte(t_byte)?;
        } else if (data.len == 8) {
            _.writeByte(0xD7)?;
            _.writeByte(t_byte)?;
        } else if (data.len == 16) {
            _.writeByte(0xD8)?;
            _.writeByte(t_byte)?;
        } else if (data.len <= 0xFF) {
            _.writeByte(0xC7)?;
            _.writeByte(data.len as u8)?;
            _.writeByte(t_byte)?;
        } else if (data.len <= 0xFFFF) {
            _.writeByte(0xC8)?;
            _.writeU16(data.len as u16)?;
            _.writeByte(t_byte)?;
        } else if (data.len <= 0xFFFFFFFF) {
            _.writeByte(0xC9)?;
            _.writeU32(data.len as u32)?;
            _.writeByte(t_byte)?;
        } else {
            return err.IntegerOverflow;
        }
        _.writeSlice(data)?;
    }

    pub fn writeTimestamp(*_, ts: MsgTimestamp): void! {
        let payload: [12]u8 = [0; 12];
        const n = ts.encodePayload(&payload)?;
        _.writeExt(MSGPACK_EXT_TIMESTAMP, payload[0..n])?;
    }
}
