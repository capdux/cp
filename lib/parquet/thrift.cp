// Source: cp/lib/parquet/thrift.cp (Thrift TCompactProtocol encoder and decoder)
const std = @import("std");
import { ParquetError } from "./types.cp";

pub const MAX_THRIFT_DEPTH: usize = 32;

pub const ThriftType = enum(u8) {
    Stop = 0,
    BoolTrue = 1,
    BoolFalse = 2,
    Byte = 3,
    I16 = 4,
    I32 = 5,
    I64 = 6,
    Double = 7,
    Binary = 8,
    List = 9,
    Set = 10,
    Map = 11,
    Struct = 12,

    pub fn fromU8(v: u8): !ThriftType {
        return switch (v) {
            0 => .Stop,
            1 => .BoolTrue,
            2 => .BoolFalse,
            3 => .Byte,
            4 => .I16,
            5 => .I32,
            6 => .I64,
            7 => .Double,
            8 => .Binary,
            9 => .List,
            10 => .Set,
            11 => .Map,
            12 => .Struct,
            else => ParquetError.InvalidThriftType,
        };
    }
};

pub struct ThriftFieldHeader {
    field_id: i16,
    field_type: ThriftType,
}

pub struct ThriftListHeader {
    elem_type: ThriftType,
    count: usize,
}

pub struct ThriftWriter {
    alloc: std.mem.Allocator,
    buf: std.ArrayList(u8),
    last_field_stack: [32]i16,
    depth: usize,

    pub static fn init(alloc: std.mem.Allocator): ThriftWriter {
        return ThriftWriter{
            .alloc = alloc,
            .buf = std.ArrayList(u8).empty,
            .last_field_stack = std.mem.zeroes([32]i16),
            .depth = 0,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    pub fn bytes(): []const u8 {
        return _.buf.items;
    }

    pub fn toOwnedSlice(*_): ![]u8 {
        return _.buf.toOwnedSlice(_.alloc);
    }

    pub fn writeByte(*_, b: u8): !void {
        try _.buf.append(_.alloc, b);
    }

    pub fn writeVarintU64(*_, value: u64): !void {
        let v = value;
        while (true) {
            if ((v & ~@as(u64, 0x7F)) == 0) {
                try _.writeByte(@as(u8, @intCast(v)));
                return;
            }
            try _.writeByte(@as(u8, @intCast((v & 0x7F) | 0x80)));
            v >>= 7;
        }
    }

    pub fn writeZigZagI32(*_, val: i32): !void {
        const uval: u32 = @bitCast(val);
        const shift: u32 = @bitCast(val >> 31);
        const zz: u32 = (uval << 1) ^ shift;
        try _.writeVarintU64(@as(u64, zz));
    }

    pub fn writeZigZagI64(*_, val: i64): !void {
        const uval: u64 = @bitCast(val);
        const shift: u64 = @bitCast(val >> 63);
        const zz: u64 = (uval << 1) ^ shift;
        try _.writeVarintU64(zz);
    }

    pub fn writeFieldHeader(*_, field_id: i16, field_type: ThriftType): !void {
        const last = _.last_field_stack[_.depth];
        const delta = @as(i32, field_id) - @as(i32, last);
        const t_u8: u8 = @intFromEnum(field_type);
        if (delta > 0 and delta <= 15) {
            const d_u8: u8 = @intCast(delta);
            try _.writeByte((d_u8 << 4) | t_u8);
        } else {
            try _.writeByte(t_u8);
            try _.writeZigZagI32(@as(i32, field_id));
        }
        _.last_field_stack[_.depth] = field_id;
    }

    pub fn writeFieldStop(*_): !void {
        try _.writeByte(0);
    }

    pub fn beginStruct(*_): !void {
        if (_.depth + 1 >= MAX_THRIFT_DEPTH) {
            return ParquetError.DepthLimitExceeded;
        }
        _.depth += 1;
        _.last_field_stack[_.depth] = 0;
    }

    pub fn endStruct(*_): !void {
        try _.writeFieldStop();
        if (_.depth > 0) {
            _.depth -= 1;
        }
    }

    pub fn writeBoolField(*_, field_id: i16, val: bool): !void {
        const t: ThriftType = if (val) .BoolTrue else .BoolFalse;
        try _.writeFieldHeader(field_id, t);
    }

    pub fn writeI32Field(*_, field_id: i16, val: i32): !void {
        try _.writeFieldHeader(field_id, .I32);
        try _.writeZigZagI32(val);
    }

    pub fn writeI64Field(*_, field_id: i16, val: i64): !void {
        try _.writeFieldHeader(field_id, .I64);
        try _.writeZigZagI64(val);
    }

    pub fn writeDoubleField(*_, field_id: i16, val: f64): !void {
        try _.writeFieldHeader(field_id, .Double);
        const bits: u64 = @bitCast(val);
        let buf: [8]u8 = std.mem.zeroes([8]u8);
        std.mem.writeInt(u64, &buf, bits, .little);
        try _.buf.appendSlice(_.alloc, &buf);
    }

    pub fn writeBinary(*_, data: []const u8): !void {
        try _.writeVarintU64(@as(u64, data.len));
        try _.buf.appendSlice(_.alloc, data);
    }

    pub fn writeBinaryField(*_, field_id: i16, data: []const u8): !void {
        try _.writeFieldHeader(field_id, .Binary);
        try _.writeBinary(data);
    }

    pub fn writeListHeader(*_, elem_type: ThriftType, count: usize): !void {
        const t_u8: u8 = @intFromEnum(elem_type);
        if (count < 15) {
            const c_u8: u8 = @intCast(count);
            try _.writeByte((c_u8 << 4) | t_u8);
        } else {
            try _.writeByte(0xF0 | t_u8);
            try _.writeVarintU64(@as(u64, count));
        }
    }
}

pub struct ThriftReader {
    data: []const u8,
    pos: usize,
    last_field_stack: [32]i16,
    depth: usize,

    pub static fn init(data: []const u8): ThriftReader {
        return ThriftReader{
            .data = data,
            .pos = 0,
            .last_field_stack = std.mem.zeroes([32]i16),
            .depth = 0,
        };
    }

    pub fn readByte(*_): !u8 {
        if (_.pos >= _.data.len) {
            return ParquetError.TruncatedData;
        }
        const b = _.data[_.pos];
        _.pos += 1;
        return b;
    }

    pub fn readBytes(*_, len: usize): ![]const u8 {
        if (len > _.data.len - _.pos) {
            return ParquetError.TruncatedData;
        }
        const slice = _.data[_.pos .. _.pos + len];
        _.pos += len;
        return slice;
    }

    pub fn readVarintU64(*_): !u64 {
        let result: u64 = 0;
        let shift: u6 = 0;
        let count: usize = 0;
        while (count < 10) : (count += 1) {
            const b = try _.readByte();
            result |= @as(u64, b & 0x7F) << shift;
            if ((b & 0x80) == 0) {
                return result;
            }
            if (shift >= 63) {
                return ParquetError.InvalidVarint;
            }
            shift += 7;
        }
        return ParquetError.InvalidVarint;
    }

    pub fn readZigZagI32(*_): !i32 {
        const uval64 = try _.readVarintU64();
        if (uval64 > 0xFFFFFFFF) {
            return ParquetError.InvalidVarint;
        }
        const uval: u32 = @intCast(uval64);
        const shifted: i32 = @bitCast(uval >> 1);
        const mask: i32 = -@as(i32, @intCast(uval & 1));
        return shifted ^ mask;
    }

    pub fn readZigZagI64(*_): !i64 {
        const uval = try _.readVarintU64();
        const shifted: i64 = @bitCast(uval >> 1);
        const mask: i64 = -@as(i64, @intCast(uval & 1));
        return shifted ^ mask;
    }

    pub fn readFieldHeader(*_): !ThriftFieldHeader {
        const b = try _.readByte();
        if (b == 0) {
            return ThriftFieldHeader{
                .field_id = 0,
                .field_type = .Stop,
            };
        }
        const t = try ThriftType.fromU8(b & 0x0F);
        const delta = (b >> 4) & 0x0F;
        let field_id: i16 = 0;
        if (delta == 0) {
            const fid_i32 = try _.readZigZagI32();
            field_id = @intCast(fid_i32);
        } else {
            field_id = _.last_field_stack[_.depth] + @as(i16, @intCast(delta));
        }
        _.last_field_stack[_.depth] = field_id;
        return ThriftFieldHeader{
            .field_id = field_id,
            .field_type = t,
        };
    }

    pub fn beginStruct(*_): !void {
        if (_.depth + 1 >= MAX_THRIFT_DEPTH) {
            return ParquetError.DepthLimitExceeded;
        }
        _.depth += 1;
        _.last_field_stack[_.depth] = 0;
    }

    pub fn endStruct(*_) {
        if (_.depth > 0) {
            _.depth -= 1;
        }
    }

    pub fn readListHeader(*_): !ThriftListHeader {
        const b = try _.readByte();
        const elem_type = try ThriftType.fromU8(b & 0x0F);
        const size_nibble = (b >> 4) & 0x0F;
        let count: usize = 0;
        if (size_nibble == 15) {
            const c64 = try _.readVarintU64();
            if (c64 > _.data.len - _.pos) {
                return ParquetError.TruncatedData;
            }
            count = @intCast(c64);
        } else {
            count = @as(usize, size_nibble);
        }
        return ThriftListHeader{
            .elem_type = elem_type,
            .count = count,
        };
    }

    pub fn readBinary(*_): ![]const u8 {
        const len64 = try _.readVarintU64();
        if (len64 > _.data.len - _.pos) {
            return ParquetError.TruncatedData;
        }
        return _.readBytes(@as(usize, @intCast(len64)));
    }

    pub fn readDouble(*_): !f64 {
        const raw = try _.readBytes(8);
        const bits = std.mem.readInt(u64, raw[0..8], .little);
        return @bitCast(bits);
    }

    pub fn skipValue(*_, field_type: ThriftType): !void {
        switch (field_type) {
            .Stop, .BoolTrue, .BoolFalse => {},
            .Byte => {
                * = try _.readByte();
            },
            .I16, .I32, .I64 => {
                * = try _.readVarintU64();
            },
            .Double => {
                * = try _.readBytes(8);
            },
            .Binary => {
                * = try _.readBinary();
            },
            .List, .Set => {
                const lh = try _.readListHeader();
                let i: usize = 0;
                while (i < lh.count) : (i += 1) {
                    try _.skipValue(lh.elem_type);
                }
            },
            .Map => {
                const count64 = try _.readVarintU64();
                if (count64 == 0) {
                    return;
                }
                if (count64 > _.data.len - _.pos) {
                    return ParquetError.TruncatedData;
                }
                const kv_types = try _.readByte();
                const key_type = try ThriftType.fromU8((kv_types >> 4) & 0x0F);
                const val_type = try ThriftType.fromU8(kv_types & 0x0F);
                const count: usize = @intCast(count64);
                let i: usize = 0;
                while (i < count) : (i += 1) {
                    try _.skipValue(key_type);
                    try _.skipValue(val_type);
                }
            },
            .Struct => {
                try _.beginStruct();
                defer _.endStruct();
                while (true) {
                    const fh = try _.readFieldHeader();
                    if (fh.field_type == .Stop) {
                        break;
                    }
                    try _.skipValue(fh.field_type);
                }
            },
        }
    }
}
