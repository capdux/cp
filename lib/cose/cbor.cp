// Source: cp/lib/cose/cbor.cp (Deterministic RFC 8949 CBOR encoder/decoder for COSE)
const std = @import("std");
import { CoseError } from "./types.cp";

pub const CborWriter = struct {
    buf: std.ArrayList(u8),

    pub static fn init(): CborWriter {
        return CborWriter{ .buf = std.ArrayList(u8).empty };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.buf.deinit(alloc);
    }

    pub fn toOwnedSlice(*_, alloc: std.mem.Allocator): CoseError![]u8 {
        return _.buf.toOwnedSlice(alloc) catch return CoseError.OutOfMemory;
    }

    pub fn writeTypeAndArg(*_, alloc: std.mem.Allocator, major: u3, arg: u64): CoseError!void {
        const m: u8 = @as(u8, major) << 5;
        if (arg < 24) {
            _.buf.append(alloc, m | @as(u8, @intCast(arg))) catch return CoseError.OutOfMemory;
        } else if (arg <= 0xff) {
            _.buf.append(alloc, m | 24) catch return CoseError.OutOfMemory;
            _.buf.append(alloc, @as(u8, @intCast(arg))) catch return CoseError.OutOfMemory;
        } else if (arg <= 0xffff) {
            _.buf.append(alloc, m | 25) catch return CoseError.OutOfMemory;
            let b: [2]u8 = undefined;
            std.mem.writeInt(u16, &b, @as(u16, @intCast(arg)), .big);
            _.buf.appendSlice(alloc, &b) catch return CoseError.OutOfMemory;
        } else if (arg <= 0xffffffff) {
            _.buf.append(alloc, m | 26) catch return CoseError.OutOfMemory;
            let b: [4]u8 = undefined;
            std.mem.writeInt(u32, &b, @as(u32, @intCast(arg)), .big);
            _.buf.appendSlice(alloc, &b) catch return CoseError.OutOfMemory;
        } else {
            _.buf.append(alloc, m | 27) catch return CoseError.OutOfMemory;
            let b: [8]u8 = undefined;
            std.mem.writeInt(u64, &b, arg, .big);
            _.buf.appendSlice(alloc, &b) catch return CoseError.OutOfMemory;
        }
    }

    pub fn writeUint(*_, alloc: std.mem.Allocator, val: u64): CoseError!void {
        try _.writeTypeAndArg(alloc, 0, val);
    }

    pub fn writeInt(*_, alloc: std.mem.Allocator, val: i64): CoseError!void {
        if (val >= 0) {
            try _.writeTypeAndArg(alloc, 0, @as(u64, @intCast(val)));
        } else {
            const neg: u64 = @as(u64, @intCast(-(val + 1)));
            try _.writeTypeAndArg(alloc, 1, neg);
        }
    }

    pub fn writeBstr(*_, alloc: std.mem.Allocator, data: []const u8): CoseError!void {
        try _.writeTypeAndArg(alloc, 2, @as(u64, data.len));
        _.buf.appendSlice(alloc, data) catch return CoseError.OutOfMemory;
    }

    pub fn writeTstr(*_, alloc: std.mem.Allocator, text: []const u8): CoseError!void {
        try _.writeTypeAndArg(alloc, 3, @as(u64, text.len));
        _.buf.appendSlice(alloc, text) catch return CoseError.OutOfMemory;
    }

    pub fn writeArrayHeader(*_, alloc: std.mem.Allocator, len: usize): CoseError!void {
        try _.writeTypeAndArg(alloc, 4, @as(u64, len));
    }

    pub fn writeMapHeader(*_, alloc: std.mem.Allocator, len: usize): CoseError!void {
        try _.writeTypeAndArg(alloc, 5, @as(u64, len));
    }

    pub fn writeTag(*_, alloc: std.mem.Allocator, tag: u64): CoseError!void {
        try _.writeTypeAndArg(alloc, 6, tag);
    }
};

pub const CborReader = struct {
    data: []const u8,
    pos: usize = 0,

    pub static fn init(data: []const u8): CborReader {
        return CborReader{ .data = data, .pos = 0 };
    }

    pub fn remaining(): usize {
        return _.data.len - _.pos;
    }

    pub fn peekMajor(): CoseError!u3 {
        if (_.pos >= _.data.len) {
            return CoseError.InvalidCbor;
        }
        return @as(u3, @intCast(_.data[_.pos] >> 5));
    }

    fn readHeader(*_): CoseError!struct { major: u3, arg: u64 } {
        if (_.pos >= _.data.len) {
            return CoseError.InvalidCbor;
        }
        const initial = _.data[_.pos];
        _.pos += 1;
        const major: u3 = @as(u3, @intCast(initial >> 5));
        const info: u5 = @as(u5, @intCast(initial & 0x1f));

        if (info < 24) {
            return .{ .major = major, .arg = @as(u64, info) };
        } else if (info == 24) {
            if (_.pos + 1 > _.data.len) return CoseError.InvalidCbor;
            const v = _.data[_.pos];
            _.pos += 1;
            return .{ .major = major, .arg = @as(u64, v) };
        } else if (info == 25) {
            if (_.pos + 2 > _.data.len) return CoseError.InvalidCbor;
            const v = std.mem.readInt(u16, _.data[_.pos..][0..2], .big);
            _.pos += 2;
            return .{ .major = major, .arg = @as(u64, v) };
        } else if (info == 26) {
            if (_.pos + 4 > _.data.len) return CoseError.InvalidCbor;
            const v = std.mem.readInt(u32, _.data[_.pos..][0..4], .big);
            _.pos += 4;
            return .{ .major = major, .arg = @as(u64, v) };
        } else if (info == 27) {
            if (_.pos + 8 > _.data.len) return CoseError.InvalidCbor;
            const v = std.mem.readInt(u64, _.data[_.pos..][0..8], .big);
            _.pos += 8;
            return .{ .major = major, .arg = v };
        } else {
            return CoseError.InvalidCbor;
        }
    }

    pub fn readTag(*_): CoseError!u64? {
        const m = try _.peekMajor();
        if (m != 6) return null;
        const h = try _.readHeader();
        return h.arg;
    }

    pub fn readUint(*_): CoseError!u64 {
        const h = try _.readHeader();
        if (h.major != 0) return CoseError.UnexpectedType;
        return h.arg;
    }

    pub fn readInt(*_): CoseError!i64 {
        const h = try _.readHeader();
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

    pub fn readBstr(*_): CoseError![]const u8 {
        const h = try _.readHeader();
        if (h.major != 2) return CoseError.UnexpectedType;
        if (h.arg > _.remaining()) return CoseError.InvalidCbor;
        const len: usize = @intCast(h.arg);
        const slice = _.data[_.pos .. _.pos + len];
        _.pos += len;
        return slice;
    }

    pub fn readTstr(*_): CoseError![]const u8 {
        const h = try _.readHeader();
        if (h.major != 3) return CoseError.UnexpectedType;
        if (h.arg > _.remaining()) return CoseError.InvalidCbor;
        const len: usize = @intCast(h.arg);
        const slice = _.data[_.pos .. _.pos + len];
        _.pos += len;
        return slice;
    }

    pub fn readArrayHeader(*_): CoseError!usize {
        const h = try _.readHeader();
        if (h.major != 4) return CoseError.UnexpectedType;
        if (h.arg > std.math.maxInt(usize)) return CoseError.InvalidCbor;
        return @as(usize, @intCast(h.arg));
    }

    pub fn readMapHeader(*_): CoseError!usize {
        const h = try _.readHeader();
        if (h.major != 5) return CoseError.UnexpectedType;
        if (h.arg > std.math.maxInt(usize)) return CoseError.InvalidCbor;
        return @as(usize, @intCast(h.arg));
    }

    pub fn isNull(*_): CoseError!bool {
        if (_.pos >= _.data.len) return CoseError.InvalidCbor;
        if (_.data[_.pos] == 0xf6) {
            _.pos += 1;
            return true;
        }
        return false;
    }

    pub fn skipValue(*_): CoseError!void {
        const h = try _.readHeader();
        switch (h.major) {
            0, 1, 7 => { return; },
            2, 3 => {
                if (h.arg > _.remaining()) return CoseError.InvalidCbor;
                _.pos += @as(usize, @intCast(h.arg));
            },
            4 => {
                let i: u64 = 0;
                while (i < h.arg) : (i += 1) {
                    try _.skipValue();
                }
            },
            5 => {
                let i: u64 = 0;
                while (i < h.arg) : (i += 1) {
                    try _.skipValue();
                    try _.skipValue();
                }
            },
            6 => {
                try _.skipValue();
            },
        }
    }
};
