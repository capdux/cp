// Source: cp/lib/mysql_wire/packet.cp (MySQL Wire Protocol framing & length-encoded primitives)
const std = @import("std");

pub const MAX_DEFAULT_PACKET_BYTES: usize = 16 * 1024 * 1024;

pub enum MysqlError {
    UnexpectedEof,
    InvalidPacketHeader,
    PacketTooLarge,
    InvalidLengthEncodedInt,
    InvalidProtocolVersion,
    InvalidPacketType,
    MalformedPacket,
    OutOfMemory,
}

pub struct LenEncInt {
    value: u64,
    is_null: bool,
}

pub struct MysqlPacket {
    sequence_id: u8,
    payload: []u8,

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.payload);
    }
}

pub fn readLenEncInt(data: []const u8, offset: *usize): !LenEncInt {
    if (offset.* >= data.len) {
        return error.UnexpectedEof;
    }
    const first = data[offset.*];
    offset.* += 1;

    if (first < 0xFB) {
        return LenEncInt{ .value = @as(u64, first), .is_null = false };
    }
    if (first == 0xFB) {
        return LenEncInt{ .value = 0, .is_null = true };
    }
    if (first == 0xFC) {
        if (offset.* + 2 > data.len) {
            return error.UnexpectedEof;
        }
        const v = std.mem.readInt(u16, data[offset.*..][0..2], .little);
        offset.* += 2;
        return LenEncInt{ .value = @as(u64, v), .is_null = false };
    }
    if (first == 0xFD) {
        if (offset.* + 3 > data.len) {
            return error.UnexpectedEof;
        }
        const b0 = @as(u64, data[offset.*]);
        const b1 = @as(u64, data[offset.* + 1]);
        const b2 = @as(u64, data[offset.* + 2]);
        offset.* += 3;
        return LenEncInt{ .value = b0 | (b1 << 8) | (b2 << 16), .is_null = false };
    }
    if (first == 0xFE) {
        if (offset.* + 8 > data.len) {
            return error.UnexpectedEof;
        }
        const v = std.mem.readInt(u64, data[offset.*..][0..8], .little);
        offset.* += 8;
        return LenEncInt{ .value = v, .is_null = false };
    }
    return error.InvalidLengthEncodedInt;
}

pub fn writeLenEncInt(alloc: std.mem.Allocator, out: *std.ArrayList(u8), value: u64): !void {
    if (value < 251) {
        try out.append(alloc, @truncate(value));
        return;
    }
    if (value < 65536) {
        try out.append(alloc, 0xFC);
        let buf: [2]u8 = undefined;
        std.mem.writeInt(u16, &buf, @truncate(value), .little);
        try out.appendSlice(alloc, &buf);
        return;
    }
    if (value < 16777216) {
        try out.append(alloc, 0xFD);
        try out.append(alloc, @truncate(value));
        try out.append(alloc, @truncate(value >> 8));
        try out.append(alloc, @truncate(value >> 16));
        return;
    }
    try out.append(alloc, 0xFE);
    let buf: [8]u8 = undefined;
    std.mem.writeInt(u64, &buf, value, .little);
    try out.appendSlice(alloc, &buf);
}

pub fn writeLenEncNull(alloc: std.mem.Allocator, out: *std.ArrayList(u8)): !void {
    try out.append(alloc, 0xFB);
}

pub fn readLenEncStr(data: []const u8, offset: *usize): !(([]const u8)?) {
    const len_info = try readLenEncInt(data, offset);
    if (len_info.is_null) {
        return null;
    }
    const str_len: usize = @intCast(len_info.value);
    if (offset.* + str_len > data.len) {
        return error.UnexpectedEof;
    }
    const slice = data[offset.* .. offset.* + str_len];
    offset.* += str_len;
    return slice;
}

pub fn writeLenEncStr(alloc: std.mem.Allocator, out: *std.ArrayList(u8), value: []const u8): !void {
    try writeLenEncInt(alloc, out, @as(u64, value.len));
    try out.appendSlice(alloc, value);
}

pub fn readNullTermStr(data: []const u8, offset: *usize): !([]const u8) {
    const start = offset.*;
    while (offset.* < data.len) {
        if (data[offset.*] == 0) {
            const slice = data[start..offset.*];
            offset.* += 1;
            return slice;
        }
        offset.* += 1;
    }
    return error.UnexpectedEof;
}

pub fn encodePacketFrame(alloc: std.mem.Allocator, sequence_id: u8, payload: []const u8): !([]u8) {
    if (payload.len > 0xFFFFFF) {
        return error.PacketTooLarge;
    }
    const total = 4 + payload.len;
    let out = try alloc.alloc(u8, total);
    const plen: u32 = @intCast(payload.len);
    out[0] = @truncate(plen);
    out[1] = @truncate(plen >> 8);
    out[2] = @truncate(plen >> 16);
    out[3] = sequence_id;
    if (payload.len > 0) {
        @memcpy(out[4..total], payload);
    }
    return out;
}

pub struct MysqlStreamFramer {
    buf: std.ArrayList(u8),
    read_pos: usize,
    max_packet_bytes: usize,

    pub fn init(max_packet_bytes: usize): MysqlStreamFramer {
        return MysqlStreamFramer{
            .buf = std.ArrayList(u8).empty,
            .read_pos = 0,
            .max_packet_bytes = max_packet_bytes,
        };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.buf.deinit(alloc);
    }

    pub fn pendingBytes(): usize {
        return _.buf.items.len - _.read_pos;
    }

    pub fn feed(*_, alloc: std.mem.Allocator, chunk: []const u8): !void {
        if (chunk.len == 0) {
            return;
        }
        if (_.read_pos > 0 and _.read_pos == _.buf.items.len) {
            _.buf.clearRetainingCapacity();
            _.read_pos = 0;
        } else if (_.read_pos > 4096) {
            const rem = _.buf.items.len - _.read_pos;
            std.mem.copyForwards(u8, _.buf.items[0..rem], _.buf.items[_.read_pos..]);
            _.buf.items.len = rem;
            _.read_pos = 0;
        }
        try _.buf.appendSlice(alloc, chunk);
    }

    pub fn nextPacket(*_, alloc: std.mem.Allocator): !(MysqlPacket?) {
        const avail = _.buf.items.len - _.read_pos;
        if (avail < 4) {
            return null;
        }
        const hdr = _.buf.items[_.read_pos .. _.read_pos + 4];
        const payload_len: usize = @as(usize, hdr[0]) |
            (@as(usize, hdr[1]) << 8) |
            (@as(usize, hdr[2]) << 16);
        if (payload_len > _.max_packet_bytes) {
            return error.PacketTooLarge;
        }
        const frame_len = 4 + payload_len;
        if (avail < frame_len) {
            return null;
        }
        const seq_id = hdr[3];
        const payload = try alloc.alloc(u8, payload_len);
        if (payload_len > 0) {
            @memcpy(payload, _.buf.items[_.read_pos + 4 .. _.read_pos + frame_len]);
        }
        _.read_pos += frame_len;
        if (_.read_pos == _.buf.items.len) {
            _.buf.clearRetainingCapacity();
            _.read_pos = 0;
        }
        return MysqlPacket{
            .sequence_id = seq_id,
            .payload = payload,
        };
    }
}
