// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/NewWriter.cp | Synced: 2026-11-24T04:40:49Z
//! MySQL wire-protocol 4-byte PacketHeader framing, `MySqlPacketWriter` (with `~init()` RAII
//! and automatic 24-bit LE payload length backfill), and zero-copy `MySqlPacketReader`.

import std from "std";

export const PACKET_HEADER_SIZE: usize = 4;
export const MAX_PACKET_PAYLOAD: u24 = 0xFFFFFF;

export err MySqlPacketError {
    ShortRead,
    BufferTooSmall,
    PacketTooLarge,
    InvalidEncodedLength,
    MissingNullTerminator,
}

/// 4-byte MySQL wire packet header: 24-bit little-endian payload length + 8-bit sequence ID.
export struct PacketHeader {
    length: u24 = 0,
    sequence_id: u8 = 0,

    pub fn encode(): [4]u8 {
        return [
            @truncate(_.length),
            @truncate(_.length >> 8),
            @truncate(_.length >> 16),
            _.sequence_id,
        ];
    }

    pub static fn decode(src: []const u8): PacketHeader! {
        if (src.len < PACKET_HEADER_SIZE) return err.ShortRead;
        const len: u24 = @as(u24, src[0])
            | (@as(u24, src[1]) << 8)
            | (@as(u24, src[2]) << 16);
        return PacketHeader{
            length: len,
            sequence_id: src[3],
        };
    }
}

export struct DecodedLenInt {
    value: u64,
    bytes_read: usize,
}

/// Decode a MySQL length-encoded integer (1, 3, 4, or 9 bytes).
export fn decodeLenInt(src: []const u8): DecodedLenInt! {
    if (src.len == 0) return err.ShortRead;
    const first = src[0];
    if (first < 0xFB) {
        return DecodedLenInt{ value: @as(u64, first), bytes_read: 1 };
    }
    if (first == 0xFC) {
        if (src.len < 3) return err.ShortRead;
        const v: u64 = @as(u64, src[1]) | (@as(u64, src[2]) << 8);
        return DecodedLenInt{ value: v, bytes_read: 3 };
    }
    if (first == 0xFD) {
        if (src.len < 4) return err.ShortRead;
        const v: u64 = @as(u64, src[1])
            | (@as(u64, src[2]) << 8)
            | (@as(u64, src[3]) << 16);
        return DecodedLenInt{ value: v, bytes_read: 4 };
    }
    if (first == 0xFE) {
        if (src.len < 9) return err.ShortRead;
        const v: u64 = @as(u64, src[1])
            | (@as(u64, src[2]) << 8)
            | (@as(u64, src[3]) << 16)
            | (@as(u64, src[4]) << 24)
            | (@as(u64, src[5]) << 32)
            | (@as(u64, src[6]) << 40)
            | (@as(u64, src[7]) << 48)
            | (@as(u64, src[8]) << 56);
        return DecodedLenInt{ value: v, bytes_read: 9 };
    }
    return err.InvalidEncodedLength;
}

/// Zero-copy MySQL packet payload reader.
export struct MySqlPacketReader {
    data: []const u8,
    pos: usize = 0,

    pub static fn init(data: []const u8): MySqlPacketReader {
        return MySqlPacketReader{ data: data, pos: 0 };
    }

    pub fn remaining(): usize {
        return _.data.len - _.pos;
    }

    pub fn peek(): []const u8 {
        return _.data[_.pos..];
    }

    pub fn skip(*_, count: usize): void! {
        if (count > _.remaining()) return err.ShortRead;
        _.pos += count;
    }

    pub fn readBytes(*_, count: usize): ([]const u8)! {
        if (count > _.remaining()) return err.ShortRead;
        const s = _.data[_.pos .. _.pos + count];
        _.pos += count;
        return s;
    }

    pub fn readHeader(*_): PacketHeader! {
        const s = _.readBytes(PACKET_HEADER_SIZE)?;
        return PacketHeader.decode(s);
    }

    pub fn readU8(*_): u8! {
        const s = _.readBytes(1)?;
        return s[0];
    }

    pub fn readU16Le(*_): u16! {
        const s = _.readBytes(2)?;
        return @as(u16, s[0]) | (@as(u16, s[1]) << 8);
    }

    pub fn readU32Le(*_): u32! {
        const s = _.readBytes(4)?;
        return @as(u32, s[0])
            | (@as(u32, s[1]) << 8)
            | (@as(u32, s[2]) << 16)
            | (@as(u32, s[3]) << 24);
    }

    pub fn readU64Le(*_): u64! {
        const s = _.readBytes(8)?;
        return @as(u64, s[0])
            | (@as(u64, s[1]) << 8)
            | (@as(u64, s[2]) << 16)
            | (@as(u64, s[3]) << 24)
            | (@as(u64, s[4]) << 32)
            | (@as(u64, s[5]) << 40)
            | (@as(u64, s[6]) << 48)
            | (@as(u64, s[7]) << 56);
    }

    pub fn readLenInt(*_): u64! {
        const r = decodeLenInt(_.peek())?;
        _.pos += r.bytes_read;
        return r.value;
    }

    pub fn readLenString(*_): ([]const u8)! {
        const len = _.readLenInt()?;
        const n: usize = @intCast(len);
        return _.readBytes(n);
    }

    pub fn readCString(*_): ([]const u8)! {
        const start = _.pos;
        while (_.pos < _.data.len) : (_.pos += 1) {
            if (_.data[_.pos] == 0) {
                const s = _.data[start.._.pos];
                _.pos += 1;
                return s;
            }
        }
        return err.MissingNullTerminator;
    }
}

export struct PacketHandle {
    offset: usize,
    sequence_id: u8,
}

/// Growable MySQL wire packet writer with deterministic `~init()` RAII cleanup
/// and automatic 24-bit LE payload length header backfill.
export struct MySqlPacketWriter {
    allocator: std.mem.Allocator,
    buf: std.ArrayList(u8),

    pub static fn init(allocator: std.mem.Allocator): MySqlPacketWriter {
        return MySqlPacketWriter{
            allocator: allocator,
            buf: std.ArrayList(u8).empty,
        };
    }

    ~init() {
        _.buf.deinit(_.allocator);
    }

    pub fn bytes(): []const u8 {
        return _.buf.items;
    }

    pub fn offset(): usize {
        return _.buf.items.len;
    }

    pub fn reset(*_) {
        _.buf.shrinkRetainingCapacity(0);
    }

    pub fn write(*_, data: []const u8): void! {
        _.buf.appendSlice(_.allocator, data)?;
    }

    pub fn writeU8(*_, v: u8): void! {
        _.buf.append(_.allocator, v)?;
    }

    pub fn writeU16Le(*_, v: u16): void! {
        const b: [2]u8 = [@truncate(v), @truncate(v >> 8)];
        _.write(b[0..])?;
    }

    pub fn writeU32Le(*_, v: u32): void! {
        const b: [4]u8 = [
            @truncate(v),
            @truncate(v >> 8),
            @truncate(v >> 16),
            @truncate(v >> 24),
        ];
        _.write(b[0..])?;
    }

    pub fn writeU64Le(*_, v: u64): void! {
        const b: [8]u8 = [
            @truncate(v),
            @truncate(v >> 8),
            @truncate(v >> 16),
            @truncate(v >> 24),
            @truncate(v >> 32),
            @truncate(v >> 40),
            @truncate(v >> 48),
            @truncate(v >> 56),
        ];
        _.write(b[0..])?;
    }

    pub fn writeLenInt(*_, v: u64): void! {
        if (v < 0xFB) {
            _.writeU8(@truncate(v))?;
        } else if (v <= 0xFFFF) {
            _.writeU8(0xFC)?;
            _.writeU16Le(@truncate(v))?;
        } else if (v <= 0xFFFFFF) {
            const b: [4]u8 = [0xFD, @truncate(v), @truncate(v >> 8), @truncate(v >> 16)];
            _.write(b[0..])?;
        } else {
            _.writeU8(0xFE)?;
            _.writeU64Le(v)?;
        }
    }

    pub fn writeLenString(*_, s: []const u8): void! {
        _.writeLenInt(@as(u64, s.len))?;
        _.write(s)?;
    }

    pub fn writeCString(*_, s: []const u8): void! {
        _.write(s)?;
        if (s.len == 0 || s[s.len - 1] != 0) {
            _.writeU8(0)?;
        }
    }

    /// Reserve a 4-byte packet header placeholder and return a handle to backfill on `endPacket`.
    pub fn startPacket(*_, sequence_id: u8): PacketHandle! {
        const off = _.buf.items.len;
        const zeros: [4]u8 = [0, 0, 0, 0];
        _.write(zeros[0..])?;
        return PacketHandle{ offset: off, sequence_id: sequence_id };
    }

    /// Compute payload length since `handle.offset` and backfill the 4-byte header.
    pub fn endPacket(*_, handle: PacketHandle): void! {
        const payload_len = _.buf.items.len - handle.offset - PACKET_HEADER_SIZE;
        if (payload_len > @as(usize, MAX_PACKET_PAYLOAD)) return err.PacketTooLarge;
        const hdr = PacketHeader{
            length: @intCast(payload_len),
            sequence_id: handle.sequence_id,
        };
        const enc = hdr.encode();
        @memcpy(_.buf.items[handle.offset .. handle.offset + PACKET_HEADER_SIZE], enc[0..]);
    }
}
