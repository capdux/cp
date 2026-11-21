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

    pub fn init(data: []const u8) PbWireReader {
        return .{
            .data = data,
            .pos = 0,
        };
    }

    pub fn isEof(self: *const PbWireReader) bool {
        return self.pos >= self.data.len;
    }

    pub fn remaining(self: *const PbWireReader) usize {
        if (self.pos >= self.data.len) {
            return 0;
        }
        return self.data.len - self.pos;
    }

    pub fn readByte(self: *PbWireReader) !u8 {
        if (self.pos >= self.data.len) {
            return OnnxError.UnexpectedEof;
        }
        const b = self.data[self.pos];
        self.pos += 1;
        return b;
    }

    pub fn readVarint(self: *PbWireReader): !u64 {
        let result: u64 = 0;
        let count: usize = 0;
        while (count < 10) {
            if (self.pos >= self.data.len) {
                return OnnxError.TruncatedVarint;
            }
            const b = self.data[self.pos];
            self.pos += 1;
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

    pub fn readI64(self: *PbWireReader) !i64 {
        const raw = try self.readVarint();
        return @bitCast(raw);
    }

    pub fn readI32(self: *PbWireReader) !i32 {
        const raw = try self.readVarint();
        return @truncate(@as(i64, @bitCast(raw)));
    }

    pub fn readFixed32(self: *PbWireReader) !u32 {
        if (self.remaining() < 4) {
            return OnnxError.UnexpectedEof;
        }
        const slice = self.data[self.pos .. self.pos + 4];
        self.pos += 4;
        return std.mem.readInt(u32, slice[0..4], .little);
    }

    pub fn readFixed64(self: *PbWireReader) !u64 {
        if (self.remaining() < 8) {
            return OnnxError.UnexpectedEof;
        }
        const slice = self.data[self.pos .. self.pos + 8];
        self.pos += 8;
        return std.mem.readInt(u64, slice[0..8], .little);
    }

    pub fn readF32(self: *PbWireReader) !f32 {
        const bits = try self.readFixed32();
        return @bitCast(bits);
    }

    pub fn readF64(self: *PbWireReader) !f64 {
        const bits = try self.readFixed64();
        return @bitCast(bits);
    }

    pub fn readBytes(self: *PbWireReader) ![]const u8 {
        const len_u64 = try self.readVarint();
        if (len_u64 > @as(u64, @intCast(self.remaining()))) {
            return OnnxError.LengthOutOfBounds;
        }
        const len: usize = @intCast(len_u64);
        const slice = self.data[self.pos .. self.pos + len];
        self.pos += len;
        return slice;
    }

    pub fn readTag(self: *PbWireReader) !?PbTag {
        if (self.isEof()) {
            return null;
        }
        const raw = try self.readVarint();
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

    pub fn enterSubmessage(self: *PbWireReader, depth: usize): anyerror!PbWireReader {
        if (depth >= MAX_PROTO_DEPTH) {
            return OnnxError.MaxDepthExceeded;
        }
        const sub_bytes = try self.readBytes();
        return PbWireReader.init(sub_bytes);
    }

    pub fn skipField(self: *PbWireReader, wire_type: PbWireType, depth: usize) !void {
        if (depth >= MAX_PROTO_DEPTH) {
            return OnnxError.MaxDepthExceeded;
        }
        switch (wire_type) {
            .varint => {
                * = try self.readVarint();
            },
            .fixed64 => {
                * = try self.readFixed64();
            },
            .length_delimited => {
                * = try self.readBytes();
            },
            .fixed32 => {
                * = try self.readFixed32();
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

    pub fn init(alloc: std.mem.Allocator) PbWireWriter {
        return .{
            .alloc = alloc,
            .buf = std.ArrayList(u8).empty,
        };
    }

    pub fn deinit(self: *PbWireWriter) void {
        self.buf.deinit(self.alloc);
    }

    pub fn toOwnedSlice(self: *PbWireWriter) ![]u8 {
        return self.buf.toOwnedSlice(self.alloc);
    }

    pub fn writeVarint(self: *PbWireWriter, value: u64) !void {
        var v = value;
        while (true) {
            const byte: u8 = @truncate(v & 0x7f);
            v >>= 7;
            if (v == 0) {
                try self.buf.append(self.alloc, byte);
                return;
            } else {
                try self.buf.append(self.alloc, byte | 0x80);
            }
        }
    }

    pub fn writeI64(self: *PbWireWriter, value: i64) !void {
        try self.writeVarint(@bitCast(value));
    }

    pub fn writeTag(self: *PbWireWriter, field_number: u32, wire_type: PbWireType) !void {
        const combined: u64 = (@as(u64, field_number) << 3) | @as(u64, @intFromEnum(wire_type));
        try self.writeVarint(combined);
    }

    pub fn writeFixed32(self: *PbWireWriter, value: u32) !void {
        var bytes: [4]u8 = undefined;
        std.mem.writeInt(u32, &bytes, value, .little);
        try self.buf.appendSlice(self.alloc, &bytes);
    }

    pub fn writeFixed64(self: *PbWireWriter, value: u64) !void {
        var bytes: [8]u8 = undefined;
        std.mem.writeInt(u64, &bytes, value, .little);
        try self.buf.appendSlice(self.alloc, &bytes);
    }

    pub fn writeF32(self: *PbWireWriter, value: f32) !void {
        try self.writeFixed32(@bitCast(value));
    }

    pub fn writeF64(self: *PbWireWriter, value: f64) !void {
        try self.writeFixed64(@bitCast(value));
    }

    pub fn writeBytes(self: *PbWireWriter, field_number: u32, data: []const u8) !void {
        try self.writeTag(field_number, .length_delimited);
        try self.writeVarint(@intCast(data.len));
        try self.buf.appendSlice(self.alloc, data);
    }

    pub fn writeString(self: *PbWireWriter, field_number: u32, text: []const u8) !void {
        try self.writeBytes(field_number, text);
    }

    pub fn writeVarintField(self: *PbWireWriter, field_number: u32, value: u64) !void {
        try self.writeTag(field_number, .varint);
        try self.writeVarint(value);
    }

    pub fn writeI64Field(self: *PbWireWriter, field_number: u32, value: i64) !void {
        try self.writeTag(field_number, .varint);
        try self.writeI64(value);
    }

    pub fn writeF32Field(self: *PbWireWriter, field_number: u32, value: f32) !void {
        try self.writeTag(field_number, .fixed32);
        try self.writeF32(value);
    }
};
