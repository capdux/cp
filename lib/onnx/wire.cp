// Source: cp/lib/onnx/wire.cp (Deterministic Protobuf wire reader/writer for ONNX)
const std = @import("std");

pub const MAX_PROTO_DEPTH: usize = 32;

pub const OnnxError = error{
    TruncatedVarint,
    VarintOverflow,
    UnexpectedEof,
    InvalidWireType,
    InvalidFieldNumber,
    MaxDepthExceeded,
    LengthOutOfBounds,
    InvalidUtf8,
    InvalidDataType,
    MissingGraph,
    CyclicGraph,
    DuplicateOutput,
    OutOfMemory,
};

pub const PbWireType = enum(u3) {
    varint = 0,
    fixed64 = 1,
    length_delimited = 2,
    start_group = 3,
    end_group = 4,
    fixed32 = 5,
};

pub const PbTag = struct {
    field_number: u32,
    wire_type: PbWireType,
};

pub const PbWireReader = struct {
    data: []const u8,
    pos: usize,

    pub static fn init(data: []const u8): PbWireReader {
        return .{
            .data = data,
            .pos = 0,
        };
    }

    pub fn isEof(): bool {
        return _.pos >= _.data.len;
    }

    pub fn remaining(): usize {
        if (_.pos >= _.data.len) {
            return 0;
        }
        return _.data.len - _.pos;
    }

    pub fn readByte(*_): !u8 {
        if (_.pos >= _.data.len) {
            return OnnxError.UnexpectedEof;
        }
        const b = _.data[_.pos];
        _.pos += 1;
        return b;
    }

    pub fn readVarint(*_): !u64 {
        let result: u64 = 0;
        let count: usize = 0;
        while (count < 10) {
            if (_.pos >= _.data.len) {
                return OnnxError.TruncatedVarint;
            }
            const b = _.data[_.pos];
            _.pos += 1;
            const payload: u64 = @intCast(b & 0x7f);
            if (count == 9 and (b & 0xfe) != 0) {
                return OnnxError.VarintOverflow;
            }
            const shift: u6 = @intCast(count * 7);
            result |= (payload << shift);
            if ((b & 0x80) == 0) {
                return result;
            }
            count += 1;
        }
        return OnnxError.VarintOverflow;
    }

    pub fn readI64(*_): !i64 {
        const raw = try _.readVarint();
        return @bitCast(raw);
    }

    pub fn readI32(*_): !i32 {
        const raw = try _.readVarint();
        return @truncate(@as(i64, @bitCast(raw)));
    }

    pub fn readFixed32(*_): !u32 {
        if (_.remaining() < 4) {
            return OnnxError.UnexpectedEof;
        }
        const slice = _.data[_.pos .. _.pos + 4];
        _.pos += 4;
        return std.mem.readInt(u32, slice[0..4], .little);
    }

    pub fn readFixed64(*_): !u64 {
        if (_.remaining() < 8) {
            return OnnxError.UnexpectedEof;
        }
        const slice = _.data[_.pos .. _.pos + 8];
        _.pos += 8;
        return std.mem.readInt(u64, slice[0..8], .little);
    }

    pub fn readF32(*_): !f32 {
        const bits = try _.readFixed32();
        return @bitCast(bits);
    }

    pub fn readF64(*_): !f64 {
        const bits = try _.readFixed64();
        return @bitCast(bits);
    }

    pub fn readBytes(*_): ![]const u8 {
        const len_u64 = try _.readVarint();
        if (len_u64 > @as(u64, @intCast(_.remaining()))) {
            return OnnxError.LengthOutOfBounds;
        }
        const len: usize = @intCast(len_u64);
        const slice = _.data[_.pos .. _.pos + len];
        _.pos += len;
        return slice;
    }

    pub fn readTag(*_): !PbTag? {
        if (_.isEof()) {
            return null;
        }
        const raw = try _.readVarint();
        const wire_raw: u3 = @truncate(raw & 0x07);
        const field_u64 = raw >> 3;
        if (field_u64 == 0 or field_u64 > std.math.maxInt(u32)) {
            return OnnxError.InvalidFieldNumber;
        }
        const wire_type: PbWireType = switch (wire_raw) {
            0 => .varint,
            1 => .fixed64,
            2 => .length_delimited,
            3 => .start_group,
            4 => .end_group,
            5 => .fixed32,
            else => return OnnxError.InvalidWireType,
        };
        return PbTag{
            .field_number = @intCast(field_u64),
            .wire_type = wire_type,
        };
    }

    pub fn enterSubmessage(*_, depth: usize): anyerror!PbWireReader {
        if (depth >= MAX_PROTO_DEPTH) {
            return OnnxError.MaxDepthExceeded;
        }
        const sub_bytes = try _.readBytes();
        return PbWireReader.init(sub_bytes);
    }

    pub fn skipField(*_, wire_type: PbWireType, depth: usize): !void {
        if (depth >= MAX_PROTO_DEPTH) {
            return OnnxError.MaxDepthExceeded;
        }
        switch (wire_type) {
            .varint => {
                * = try _.readVarint();
            },
            .fixed64 => {
                * = try _.readFixed64();
            },
            .length_delimited => {
                * = try _.readBytes();
            },
            .fixed32 => {
                * = try _.readFixed32();
            },
            .start_group, .end_group => {
                return OnnxError.InvalidWireType;
            },
        }
    }
};

pub const PbWireWriter = struct {
    alloc: std.mem.Allocator,
    buf: std.ArrayList(u8),

    pub static fn init(alloc: std.mem.Allocator): PbWireWriter {
        return .{
            .alloc = alloc,
            .buf = std.ArrayList(u8).empty,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    pub fn toOwnedSlice(*_): ![]u8 {
        return _.buf.toOwnedSlice(_.alloc);
    }

    pub fn writeVarint(*_, value: u64): !void {
        let v = value;
        while (true) {
            const byte: u8 = @truncate(v & 0x7f);
            v >>= 7;
            if (v == 0) {
                try _.buf.append(_.alloc, byte);
                return;
            } else {
                try _.buf.append(_.alloc, byte | 0x80);
            }
        }
    }

    pub fn writeI64(*_, value: i64): !void {
        try _.writeVarint(@bitCast(value));
    }

    pub fn writeTag(*_, field_number: u32, wire_type: PbWireType): !void {
        const combined: u64 = (@as(u64, field_number) << 3) | @as(u64, @intFromEnum(wire_type));
        try _.writeVarint(combined);
    }

    pub fn writeFixed32(*_, value: u32): !void {
        let bytes: [4]u8 = undefined;
        std.mem.writeInt(u32, &bytes, value, .little);
        try _.buf.appendSlice(_.alloc, &bytes);
    }

    pub fn writeFixed64(*_, value: u64): !void {
        let bytes: [8]u8 = undefined;
        std.mem.writeInt(u64, &bytes, value, .little);
        try _.buf.appendSlice(_.alloc, &bytes);
    }

    pub fn writeF32(*_, value: f32): !void {
        try _.writeFixed32(@bitCast(value));
    }

    pub fn writeF64(*_, value: f64): !void {
        try _.writeFixed64(@bitCast(value));
    }

    pub fn writeBytes(*_, field_number: u32, data: []const u8): !void {
        try _.writeTag(field_number, .length_delimited);
        try _.writeVarint(@intCast(data.len));
        try _.buf.appendSlice(_.alloc, data);
    }

    pub fn writeString(*_, field_number: u32, text: []const u8): !void {
        try _.writeBytes(field_number, text);
    }

    pub fn writeVarintField(*_, field_number: u32, value: u64): !void {
        try _.writeTag(field_number, .varint);
        try _.writeVarint(value);
    }

    pub fn writeI64Field(*_, field_number: u32, value: i64): !void {
        try _.writeTag(field_number, .varint);
        try _.writeI64(value);
    }

    pub fn writeF32Field(*_, field_number: u32, value: f32): !void {
        try _.writeTag(field_number, .fixed32);
        try _.writeF32(value);
    }
};
