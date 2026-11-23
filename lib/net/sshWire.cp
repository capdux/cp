// Source: cp/lib/ssh_wire/wire.cp (RFC 4251 SSH-2 Wire Codec)
const std = @import("std");
type NameSlice = []const u8;

pub err SshError {
    UnexpectedEof,
    InvalidFormat,
    InvalidKeyType,
    KeyTypeMismatch,
    UnsupportedCipher,
    UnsupportedKdf,
    CheckintMismatch,
    InvalidMagic,
    InvalidBase64,
    InvalidSignature,
    InvalidNamespace,
    InvalidHashAlgorithm,
    BufferTooSmall,
    OutOfMemory,
}

pub struct SshReader {
    data: []const u8;
    pos: usize = 0;

    static fn init(data: []const u8): SshReader {
        return SshReader{
            data: data,
            pos: 0,
        };
    }

    fn remaining(): usize {
        if (_.pos >= _.data.len) {
            return 0;
        }
        return _.data.len - _.pos;
    }

    fn readByte(*_): SshError!u8 {
        if (_.pos >= _.data.len) {
            return SshError.UnexpectedEof;
        }
        const b = _.data[_.pos];
        _.pos += 1;
        return b;
    }

    fn readBytes(*_, len: usize): SshError![]const u8 {
        if (_.remaining() < len) {
            return SshError.UnexpectedEof;
        }
        const slice = _.data[_.pos .. _.pos + len];
        _.pos += len;
        return slice;
    }

    fn readU32(*_): SshError!u32 {
        const raw = try _.readBytes(4);
        return std.mem.readInt(u32, raw[0..4], .big);
    }

    fn readString(*_): SshError![]const u8 {
        const len = try _.readU32();
        return try _.readBytes(@as(usize, len));
    }

    fn readMpint(*_): SshError![]const u8 {
        const bytes = try _.readString();
        if (bytes.len == 0) {
            return bytes;
        }
        if (bytes.len == 1 and bytes[0] == 0x00) {
            return SshError.InvalidFormat;
        }
        if (bytes.len >= 2) {
            if (bytes[0] == 0x00 and (bytes[1] & 0x80) == 0) {
                return SshError.InvalidFormat;
            }
            if (bytes[0] == 0xff and (bytes[1] & 0x80) != 0) {
                return SshError.InvalidFormat;
            }
        }
        return bytes;
    }

    fn readNameList(*_, alloc: std.mem.Allocator): SshError![][]const u8 {
        const raw = try _.readString();
        if (raw.len == 0) {
            let empty_list = std.ArrayList(NameSlice).empty;
            return empty_list.toOwnedSlice(alloc) catch return SshError.OutOfMemory;
        }
        let list = std.ArrayList(NameSlice).empty;
        errdefer list.deinit(alloc);

        let it = std.mem.splitScalar(u8, raw, ',');
        while (item := it.next()) {
            if (item.len == 0) {
                return SshError.InvalidFormat;
            }
            list.append(alloc, item) catch return SshError.OutOfMemory;
        }
        return list.toOwnedSlice(alloc) catch return SshError.OutOfMemory;
    }
}

pub struct SshWriter {
    buf: std.ArrayList(u8) = .empty;
    alloc: std.mem.Allocator;

    static fn init(alloc: std.mem.Allocator): SshWriter {
        return SshWriter{
            buf: std.ArrayList(u8).empty,
            alloc: alloc,
        };
    }

    fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    fn items(): []const u8 {
        return _.buf.items;
    }

    fn toOwnedSlice(*_): SshError![]u8 {
        return _.buf.toOwnedSlice(_.alloc) catch return SshError.OutOfMemory;
    }

    fn writeByte(*_, b: u8): SshError!void {
        _.buf.append(_.alloc, b) catch return SshError.OutOfMemory;
    }

    fn writeRaw(*_, data: []const u8): SshError!void {
        _.buf.appendSlice(_.alloc, data) catch return SshError.OutOfMemory;
    }

    fn writeU32(*_, val: u32): SshError!void {
        let tmp = std.mem.zeroes([4]u8);
        std.mem.writeInt(u32, &tmp, val, .big);
        try _.writeRaw(&tmp);
    }

    fn writeString(*_, data: []const u8): SshError!void {
        if (data.len > std.math.maxInt(u32)) {
            return SshError.InvalidFormat;
        }
        try _.writeU32(@as(u32, @intCast(data.len)));
        try _.writeRaw(data);
    }

    /// Writes an unsigned big-endian integer in RFC 4251 mpint wire format.
    fn writeMpint(*_, bytes: []const u8): SshError!void {
        let start: usize = 0;
        while (start < bytes.len and bytes[start] == 0) {
            start += 1;
        }
        const trimmed = bytes[start..];
        if (trimmed.len == 0) {
            try _.writeU32(0);
            return;
        }
        if ((trimmed[0] & 0x80) != 0) {
            try _.writeU32(@as(u32, @intCast(trimmed.len + 1)));
            try _.writeByte(0x00);
            try _.writeRaw(trimmed);
        } else {
            try _.writeString(trimmed);
        }
    }

    /// Writes a signed two's complement big-endian integer in RFC 4251 mpint wire format.
    fn writeSignedMpint(*_, bytes: []const u8): SshError!void {
        if (bytes.len == 0) {
            try _.writeU32(0);
            return;
        }
        const is_negative = (bytes[0] & 0x80) != 0;
        if (!is_negative) {
            try _.writeMpint(bytes);
            return;
        }
        let start: usize = 0;
        while (start + 1 < bytes.len and bytes[start] == 0xff and (bytes[start + 1] & 0x80) != 0) {
            start += 1;
        }
        try _.writeString(bytes[start..]);
    }

    fn writeNameList(*_, names: []const []const u8): SshError!void {
        if (names.len == 0) {
            try _.writeU32(0);
            return;
        }
        let total_len: usize = names.len - 1;
        for (name of names) {
            total_len += name.len;
        }
        try _.writeU32(@as(u32, @intCast(total_len)));
        let idx: usize = 0;
        while (idx < names.len) {
            if (idx > 0) {
                try _.writeByte(',');
            }
            try _.writeRaw(names[idx]);
            idx += 1;
        }
    }
}
