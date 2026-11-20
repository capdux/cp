// Source: cp/lib/kafka_wire/wire.cp (Kafka Wire Protocol binary primitives)
const std = @import("std");

pub const KafkaError = error{
    UnexpectedEof,
    InvalidVarint,
    InvalidLength,
    InvalidMagic,
    CrcMismatch,
    FrameTooLarge,
    UnsupportedVersion,
    OutOfMemory,
};

pub struct KafkaReader {
    buf: []const u8,
    pos: usize,

    pub fn init(buf: []const u8): KafkaReader {
        return KafkaReader{
            .buf = buf,
            .pos = 0,
        };
    }

    pub fn remaining(): usize {
        if (_.pos >= _.buf.len) {
            return 0;
        }
        return _.buf.len - _.pos;
    }

    pub fn readBytes(*_, len: usize): KafkaError![]const u8 {
        if (len > _.remaining()) {
            return KafkaError.UnexpectedEof;
        }
        const slice = _.buf[_.pos .. _.pos + len];
        _.pos += len;
        return slice;
    }

    pub fn readU8(*_): KafkaError!u8 {
        const s = _.readBytes(1)?;
        return s[0];
    }

    pub fn readI8(*_): KafkaError!i8 {
        const b = _.readU8()?;
        return @as(i8, @bitCast(b));
    }

    pub fn readU16(*_): KafkaError!u16 {
        const s = _.readBytes(2)?;
        return std.mem.readInt(u16, s[0..2], .big);
    }

    pub fn readI16(*_): KafkaError!i16 {
        const s = _.readBytes(2)?;
        return std.mem.readInt(i16, s[0..2], .big);
    }

    pub fn readU32(*_): KafkaError!u32 {
        const s = _.readBytes(4)?;
        return std.mem.readInt(u32, s[0..4], .big);
    }

    pub fn readI32(*_): KafkaError!i32 {
        const s = _.readBytes(4)?;
        return std.mem.readInt(i32, s[0..4], .big);
    }

    pub fn readU64(*_): KafkaError!u64 {
        const s = _.readBytes(8)?;
        return std.mem.readInt(u64, s[0..8], .big);
    }

    pub fn readI64(*_): KafkaError!i64 {
        const s = _.readBytes(8)?;
        return std.mem.readInt(i64, s[0..8], .big);
    }

    pub fn readUVarint(*_): KafkaError!u32 {
        let value: u32 = 0;
        let shift: u5 = 0;
        let i: usize = 0;
        while (i < 5) {
            const b = _.readU8()?;
            const low: u32 = @as(u32, b & 0x7f);
            if (i == 4 and (b & 0xf0) != 0) {
                return KafkaError.InvalidVarint;
            }
            value |= (low << shift);
            if ((b & 0x80) == 0) {
                return value;
            }
            if (i < 4) {
                shift += 7;
            }
            i += 1;
        }
        return KafkaError.InvalidVarint;
    }

    pub fn readUVarint64(*_): KafkaError!u64 {
        let value: u64 = 0;
        let shift: u6 = 0;
        let i: usize = 0;
        while (i < 10) {
            const b = _.readU8()?;
            const low: u64 = @as(u64, b & 0x7f);
            if (i == 9 and (b & 0xfe) != 0) {
                return KafkaError.InvalidVarint;
            }
            value |= (low << shift);
            if ((b & 0x80) == 0) {
                return value;
            }
            if (i < 9) {
                shift += 7;
            }
            i += 1;
        }
        return KafkaError.InvalidVarint;
    }

    pub fn readVarint(*_): KafkaError!i32 {
        const u = _.readUVarint()?;
        const shifted: u32 = u >> 1;
        const mask: u32 = @as(u32, 0) -% (u & 1);
        return @as(i32, @bitCast(shifted ^ mask));
    }

    pub fn readVarlong(*_): KafkaError!i64 {
        const u = _.readUVarint64()?;
        const shifted: u64 = u >> 1;
        const mask: u64 = @as(u64, 0) -% (u & 1);
        return @as(i64, @bitCast(shifted ^ mask));
    }

    pub fn readNullableString(*_): KafkaError!([]const u8)? {
        const len = _.readI16()?;
        if (len == -1) {
            return null;
        }
        if (len < -1) {
            return KafkaError.InvalidLength;
        }
        return _.readBytes(@as(usize, @intCast(len)))?;
    }

    pub fn readString(*_): KafkaError![]const u8 {
        const len = _.readI16()?;
        if (len < 0) {
            return KafkaError.InvalidLength;
        }
        return _.readBytes(@as(usize, @intCast(len)))?;
    }

    pub fn readCompactString(*_): KafkaError!([]const u8)? {
        const n = _.readUVarint()?;
        if (n == 0) {
            return null;
        }
        return _.readBytes(@as(usize, n - 1))?;
    }

    pub fn readCompactBytes(*_): KafkaError!([]const u8)? {
        const n = _.readUVarint()?;
        if (n == 0) {
            return null;
        }
        return _.readBytes(@as(usize, n - 1))?;
    }

    pub fn readCompactArrayLen(*_): KafkaError!(usize)? {
        const n = _.readUVarint()?;
        if (n == 0) {
            return null;
        }
        return @as(usize, n - 1);
    }

    pub fn readVarintBytes(*_): KafkaError!([]const u8)? {
        const len = _.readVarint()?;
        if (len == -1) {
            return null;
        }
        if (len < -1) {
            return KafkaError.InvalidLength;
        }
        return _.readBytes(@as(usize, @intCast(len)))?;
    }

    pub fn skipTaggedFields(*_): KafkaError!void {
        const num_tags = _.readUVarint()?;
        let i: u32 = 0;
        while (i < num_tags) {
            * = _.readUVarint()?;
            const size = _.readUVarint()?;
            * = _.readBytes(@as(usize, size))?;
            i += 1;
        }
    }
}

pub struct KafkaWriter {
    alloc: std.mem.Allocator,
    buf: std.ArrayList(u8),

    pub fn init(alloc: std.mem.Allocator): KafkaWriter {
        return KafkaWriter{
            .alloc = alloc,
            .buf = std.ArrayList(u8).empty,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    pub fn len(): usize {
        return _.buf.items.len;
    }

    pub fn items(): []const u8 {
        return _.buf.items;
    }

    pub fn toOwnedSlice(*_): KafkaError![]u8 {
        return _.buf.toOwnedSlice(_.alloc) catch return KafkaError.OutOfMemory;
    }

    pub fn writeU8(*_, v: u8): KafkaError!void {
        _.buf.append(_.alloc, v) catch return KafkaError.OutOfMemory;
    }

    pub fn writeI8(*_, v: i8): KafkaError!void {
        _.writeU8(@as(u8, @bitCast(v)))?;
    }

    pub fn writeBytes(*_, data: []const u8): KafkaError!void {
        _.buf.appendSlice(_.alloc, data) catch return KafkaError.OutOfMemory;
    }

    pub fn writeU16(*_, v: u16): KafkaError!void {
        let tmp: [2]u8 = undefined;
        std.mem.writeInt(u16, &tmp, v, .big);
        _.writeBytes(&tmp)?;
    }

    pub fn writeI16(*_, v: i16): KafkaError!void {
        let tmp: [2]u8 = undefined;
        std.mem.writeInt(i16, &tmp, v, .big);
        _.writeBytes(&tmp)?;
    }

    pub fn writeU32(*_, v: u32): KafkaError!void {
        let tmp: [4]u8 = undefined;
        std.mem.writeInt(u32, &tmp, v, .big);
        _.writeBytes(&tmp)?;
    }

    pub fn writeI32(*_, v: i32): KafkaError!void {
        let tmp: [4]u8 = undefined;
        std.mem.writeInt(i32, &tmp, v, .big);
        _.writeBytes(&tmp)?;
    }

    pub fn writeU64(*_, v: u64): KafkaError!void {
        let tmp: [8]u8 = undefined;
        std.mem.writeInt(u64, &tmp, v, .big);
        _.writeBytes(&tmp)?;
    }

    pub fn writeI64(*_, v: i64): KafkaError!void {
        let tmp: [8]u8 = undefined;
        std.mem.writeInt(i64, &tmp, v, .big);
        _.writeBytes(&tmp)?;
    }

    pub fn writeUVarint(*_, val: u32): KafkaError!void {
        let v = val;
        while ((v & ~@as(u32, 0x7f)) != 0) {
            _.writeU8(@as(u8, @intCast((v & 0x7f) | 0x80)))?;
            v >>= 7;
        }
        _.writeU8(@as(u8, @intCast(v & 0x7f)))?;
    }

    pub fn writeUVarint64(*_, val: u64): KafkaError!void {
        let v = val;
        while ((v & ~@as(u64, 0x7f)) != 0) {
            _.writeU8(@as(u8, @intCast((v & 0x7f) | 0x80)))?;
            v >>= 7;
        }
        _.writeU8(@as(u8, @intCast(v & 0x7f)))?;
    }

    pub fn writeVarint(*_, val: i32): KafkaError!void {
        const u: u32 = @as(u32, @bitCast((val << 1) ^ (val >> 31)));
        _.writeUVarint(u)?;
    }

    pub fn writeVarlong(*_, val: i64): KafkaError!void {
        const u: u64 = @as(u64, @bitCast((val << 1) ^ (val >> 63)));
        _.writeUVarint64(u)?;
    }

    pub fn writeNullableString(*_, s: ([]const u8)?): KafkaError!void {
        if (str := s) {
            if (str.len > 32767) {
                return KafkaError.InvalidLength;
            }
            _.writeI16(@as(i16, @intCast(str.len)))?;
            _.writeBytes(str)?;
        } else {
            _.writeI16(-1)?;
        }
    }

    pub fn writeString(*_, s: []const u8): KafkaError!void {
        if (s.len > 32767) {
            return KafkaError.InvalidLength;
        }
        _.writeI16(@as(i16, @intCast(s.len)))?;
        _.writeBytes(s)?;
    }

    pub fn writeCompactString(*_, s: ([]const u8)?): KafkaError!void {
        if (str := s) {
            _.writeUVarint(@as(u32, @intCast(str.len + 1)))?;
            _.writeBytes(str)?;
        } else {
            _.writeUVarint(0)?;
        }
    }

    pub fn writeCompactBytes(*_, b: ([]const u8)?): KafkaError!void {
        if (bytes := b) {
            _.writeUVarint(@as(u32, @intCast(bytes.len + 1)))?;
            _.writeBytes(bytes)?;
        } else {
            _.writeUVarint(0)?;
        }
    }

    pub fn writeCompactArrayLen(*_, n: (usize)?): KafkaError!void {
        if (count := n) {
            _.writeUVarint(@as(u32, @intCast(count + 1)))?;
        } else {
            _.writeUVarint(0)?;
        }
    }

    pub fn writeVarintBytes(*_, b: ([]const u8)?): KafkaError!void {
        if (bytes := b) {
            _.writeVarint(@as(i32, @intCast(bytes.len)))?;
            _.writeBytes(bytes)?;
        } else {
            _.writeVarint(-1)?;
        }
    }

    pub fn writeEmptyTaggedFields(*_): KafkaError!void {
        _.writeUVarint(0)?;
    }
}
