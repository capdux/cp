// Source: cp/lib/cose/cbor.cp (Deterministic RFC 8949 CBOR encoder/decoder for COSE)
const std = @import("std");
import { CoseError } from "./types.cp";

pub const CborWriter = struct {
    buf: std.ArrayList(u8),

    pub fn init() CborWriter {
        return CborWriter{ .buf = std.ArrayList(u8).empty };
    }

    pub fn deinit(self: *CborWriter, alloc: std.mem.Allocator) void {
        self.buf.deinit(alloc);
    }

    pub fn toOwnedSlice(self: *CborWriter, alloc: std.mem.Allocator) CoseError![]u8 {
        return self.buf.toOwnedSlice(alloc) catch return CoseError.OutOfMemory;
    }

    pub fn writeTypeAndArg(self: *CborWriter, alloc: std.mem.Allocator, major: u3, arg: u64) CoseError!void {
        const m: u8 = @as(u8, major) << 5;
        if (arg < 24) {
            self.buf.append(alloc, m | @as(u8, @intCast(arg))) catch return CoseError.OutOfMemory;
        } else if (arg <= 0xff) {
            self.buf.append(alloc, m | 24) catch return CoseError.OutOfMemory;
            self.buf.append(alloc, @as(u8, @intCast(arg))) catch return CoseError.OutOfMemory;
        } else if (arg <= 0xffff) {
            self.buf.append(alloc, m | 25) catch return CoseError.OutOfMemory;
            var b: [2]u8 = undefined;
            std.mem.writeInt(u16, &b, @as(u16, @intCast(arg)), .big);
            self.buf.appendSlice(alloc, &b) catch return CoseError.OutOfMemory;
        } else if (arg <= 0xffffffff) {
            self.buf.append(alloc, m | 26) catch return CoseError.OutOfMemory;
            var b: [4]u8 = undefined;
            std.mem.writeInt(u32, &b, @as(u32, @intCast(arg)), .big);
            self.buf.appendSlice(alloc, &b) catch return CoseError.OutOfMemory;
        } else {
            self.buf.append(alloc, m | 27) catch return CoseError.OutOfMemory;
            var b: [8]u8 = undefined;
            std.mem.writeInt(u64, &b, arg, .big);
            self.buf.appendSlice(alloc, &b) catch return CoseError.OutOfMemory;
        }
    }

    pub fn writeUint(self: *CborWriter, alloc: std.mem.Allocator, val: u64) CoseError!void {
        try self.writeTypeAndArg(alloc, 0, val);
    }

    pub fn writeInt(self: *CborWriter, alloc: std.mem.Allocator, val: i64) CoseError!void {
        if (val >= 0) {
            try self.writeTypeAndArg(alloc, 0, @as(u64, @intCast(val)));
        } else {
            const neg: u64 = @as(u64, @intCast(-(val + 1)));
            try self.writeTypeAndArg(alloc, 1, neg);
        }
    }

    pub fn writeBstr(self: *CborWriter, alloc: std.mem.Allocator, data: []const u8) CoseError!void {
        try self.writeTypeAndArg(alloc, 2, @as(u64, data.len));
        self.buf.appendSlice(alloc, data) catch return CoseError.OutOfMemory;
    }

    pub fn writeTstr(self: *CborWriter, alloc: std.mem.Allocator, text: []const u8) CoseError!void {
        try self.writeTypeAndArg(alloc, 3, @as(u64, text.len));
        self.buf.appendSlice(alloc, text) catch return CoseError.OutOfMemory;
    }

    pub fn writeArrayHeader(self: *CborWriter, alloc: std.mem.Allocator, len: usize) CoseError!void {
        try self.writeTypeAndArg(alloc, 4, @as(u64, len));
    }

    pub fn writeMapHeader(self: *CborWriter, alloc: std.mem.Allocator, len: usize) CoseError!void {
        try self.writeTypeAndArg(alloc, 5, @as(u64, len));
    }

    pub fn writeTag(self: *CborWriter, alloc: std.mem.Allocator, tag: u64) CoseError!void {
        try self.writeTypeAndArg(alloc, 6, tag);
    }
};

pub const CborReader = struct {
    data: []const u8,
    pos: usize = 0,

    pub fn init(data: []const u8) CborReader {
        return CborReader{ .data = data, .pos = 0 };
    }

    pub fn remaining(self: *const CborReader) usize {
        return self.data.len - self.pos;
    }

    pub fn peekMajor(self: *const CborReader) CoseError!u3 {
        if (self.pos >= self.data.len) {
            return CoseError.InvalidCbor;
        }
        return @as(u3, @intCast(self.data[self.pos] >> 5));
    }

    fn readHeader(self: *CborReader) CoseError!struct { major: u3, arg: u64 } {
        if (self.pos >= self.data.len) {
            return CoseError.InvalidCbor;
        }
        const initial = self.data[self.pos];
        self.pos += 1;
        const major: u3 = @as(u3, @intCast(initial >> 5));
        const info: u5 = @as(u5, @intCast(initial & 0x1f));

        if (info < 24) {
            return .{ .major = major, .arg = @as(u64, info) };
        } else if (info == 24) {
            if (self.pos + 1 > self.data.len) return CoseError.InvalidCbor;
            const v = self.data[self.pos];
            self.pos += 1;
            return .{ .major = major, .arg = @as(u64, v) };
        } else if (info == 25) {
            if (self.pos + 2 > self.data.len) return CoseError.InvalidCbor;
            const v = std.mem.readInt(u16, self.data[self.pos..][0..2], .big);
            self.pos += 2;
            return .{ .major = major, .arg = @as(u64, v) };
        } else if (info == 26) {
            if (self.pos + 4 > self.data.len) return CoseError.InvalidCbor;
            const v = std.mem.readInt(u32, self.data[self.pos..][0..4], .big);
            self.pos += 4;
            return .{ .major = major, .arg = @as(u64, v) };
        } else if (info == 27) {
            if (self.pos + 8 > self.data.len) return CoseError.InvalidCbor;
            const v = std.mem.readInt(u64, self.data[self.pos..][0..8], .big);
            self.pos += 8;
            return .{ .major = major, .arg = v };
        } else {
            return CoseError.InvalidCbor;
        }
    }

    pub fn readTag(self: *CborReader) CoseError!?u64 {
        const m = try self.peekMajor();
        if (m != 6) return null;
        const h = try self.readHeader();
        return h.arg;
    }

    pub fn readUint(self: *CborReader) CoseError!u64 {
        const h = try self.readHeader();
        if (h.major != 0) return CoseError.UnexpectedType;
        return h.arg;
    }

    pub fn readInt(self: *CborReader) CoseError!i64 {
        const h = try self.readHeader();
        if (h.major == 0) {
            if (h.arg > @as(u64, std.math.maxInt(i64))) return CoseError.InvalidCbor;
            return @as(i64, @intCast(h.arg));
        } else if (h.major == 1) {
            if (h.arg > @as(u64, std.math.maxInt(i64))) return CoseError.InvalidCbor;
            return -1 - @as(i64, @intCast(h.arg));
        } else {
            return CoseError.UnexpectedType;
        }
    }

    pub fn readBstr(self: *CborReader) CoseError![]const u8 {
        const h = try self.readHeader();
        if (h.major != 2) return CoseError.UnexpectedType;
        if (h.arg > self.remaining()) return CoseError.InvalidCbor;
        const len: usize = @intCast(h.arg);
        const slice = self.data[self.pos .. self.pos + len];
        self.pos += len;
        return slice;
    }

    pub fn readTstr(self: *CborReader) CoseError![]const u8 {
        const h = try self.readHeader();
        if (h.major != 3) return CoseError.UnexpectedType;
        if (h.arg > self.remaining()) return CoseError.InvalidCbor;
        const len: usize = @intCast(h.arg);
        const slice = self.data[self.pos .. self.pos + len];
        self.pos += len;
        return slice;
    }

    pub fn readArrayHeader(self: *CborReader) CoseError!usize {
        const h = try self.readHeader();
        if (h.major != 4) return CoseError.UnexpectedType;
        if (h.arg > std.math.maxInt(usize)) return CoseError.InvalidCbor;
        return @as(usize, @intCast(h.arg));
    }

    pub fn readMapHeader(self: *CborReader) CoseError!usize {
        const h = try self.readHeader();
        if (h.major != 5) return CoseError.UnexpectedType;
        if (h.arg > std.math.maxInt(usize)) return CoseError.InvalidCbor;
        return @as(usize, @intCast(h.arg));
    }

    pub fn isNull(self: *CborReader) CoseError!bool {
        if (self.pos >= self.data.len) return CoseError.InvalidCbor;
        if (self.data[self.pos] == 0xf6) {
            self.pos += 1;
            return true;
        }
        return false;
    }

    pub fn skipValue(self: *CborReader) CoseError!void {
        const h = try self.readHeader();
        switch (h.major) {
            0, 1, 7 => return,
            2, 3 => {
                if (h.arg > self.remaining()) return CoseError.InvalidCbor;
                self.pos += @as(usize, @intCast(h.arg));
            },
            4 => {
                var i: u64 = 0;
                while (i < h.arg) : (i += 1) {
                    try self.skipValue();
                }
            },
            5 => {
                var i: u64 = 0;
                while (i < h.arg) : (i += 1) {
                    try self.skipValue();
                    try self.skipValue();
                }
            },
            6 => {
                try self.skipValue();
            },
        }
    }
};
