// Source: Absorbed from /data/dev/bun/src/http/H2FrameParser.cp | Synced: 2026-11-21T12:38:18Z
//! HTTP/2 (RFC 9113 / RFC 7540) wire-format frame header, settings, priority,
//! error codes, and client connection preface codec.

export const CLIENT_PREFACE: []const u8 = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
export const FRAME_HEADER_SIZE: usize = 9;
export const SETTINGS_UNIT_SIZE: usize = 6;
export const STREAM_PRIORITY_SIZE: usize = 5;

export const MAX_WINDOW_SIZE: i32 = 0x7FFFFFFF;
export const MAX_HEADER_TABLE_SIZE: u32 = 0xFFFFFFFF;
export const MAX_STREAM_ID: u32 = 0x7FFFFFFF;
export const MAX_FRAME_SIZE: u24 = 0xFFFFFF;
export const DEFAULT_WINDOW_SIZE: u32 = 65535;
export const DEFAULT_MAX_FRAME_SIZE: u24 = 16384;

export err H2FrameError {
    ShortBuffer,
    BufferTooSmall,
    InvalidPreface,
    PayloadTooLarge,
}

export enum FrameType: u8 {
    DATA = 0x00,
    HEADERS = 0x01,
    PRIORITY = 0x02,
    RST_STREAM = 0x03,
    SETTINGS = 0x04,
    PUSH_PROMISE = 0x05,
    PING = 0x06,
    GOAWAY = 0x07,
    WINDOW_UPDATE = 0x08,
    CONTINUATION = 0x09,
    ALTSVC = 0x0A,
    ORIGIN = 0x0C,
    _,

    pub fn toU8(): u8 {
        return @intFromEnum(_);
    }

    pub fn isKnown(): bool {
        return switch (_) {
            .DATA, .HEADERS, .PRIORITY, .RST_STREAM, .SETTINGS, .PUSH_PROMISE,
            .PING, .GOAWAY, .WINDOW_UPDATE, .CONTINUATION, .ALTSVC, .ORIGIN => true,
            else => false,
        };
    }

    pub fn isConnectionLevel(): bool {
        return switch (_) {
            .SETTINGS, .PING, .GOAWAY => true,
            else => false,
        };
    }
}

export const FLAG_ACK: u8 = 0x01;
export const FLAG_END_STREAM: u8 = 0x01;
export const FLAG_END_HEADERS: u8 = 0x04;
export const FLAG_PADDED: u8 = 0x08;
export const FLAG_PRIORITY: u8 = 0x20;

export enum ErrorCode: u32 {
    NO_ERROR = 0x0,
    PROTOCOL_ERROR = 0x1,
    INTERNAL_ERROR = 0x2,
    FLOW_CONTROL_ERROR = 0x3,
    SETTINGS_TIMEOUT = 0x4,
    STREAM_CLOSED = 0x5,
    FRAME_SIZE_ERROR = 0x6,
    REFUSED_STREAM = 0x7,
    CANCEL = 0x8,
    COMPRESSION_ERROR = 0x9,
    CONNECT_ERROR = 0xA,
    ENHANCE_YOUR_CALM = 0xB,
    INADEQUATE_SECURITY = 0xC,
    HTTP_1_1_REQUIRED = 0xD,
    _,

    pub fn toU32(): u32 {
        return @intFromEnum(_);
    }

    pub fn isOk(): bool {
        return _ == .NO_ERROR;
    }
}

export enum SettingsType: u16 {
    HEADER_TABLE_SIZE = 0x1,
    ENABLE_PUSH = 0x2,
    MAX_CONCURRENT_STREAMS = 0x3,
    INITIAL_WINDOW_SIZE = 0x4,
    MAX_FRAME_SIZE = 0x5,
    MAX_HEADER_LIST_SIZE = 0x6,
    ENABLE_CONNECT_PROTOCOL = 0x8,
    NO_RFC7540_PRIORITIES = 0x9,
    _,

    pub fn toU16(): u16 {
        return @intFromEnum(_);
    }
}

/// 31-bit unsigned integer with a leading reserved bit (stream IDs, window increments).
export struct UInt31WithReserved {
    reserved: bool = false,
    value: u31 = 0,

    pub static fn fromU32(raw: u32): UInt31WithReserved {
        return UInt31WithReserved{
            reserved: (raw & 0x80000000) != 0,
            value: @truncate(raw & 0x7FFFFFFF),
        };
    }

    pub fn toU32(): u32 {
        const hi: u32 = if (_.reserved) 0x80000000 else 0;
        return hi | @as(u32, _.value);
    }

    pub static fn decodeBe(src: []const u8): UInt31WithReserved! {
        if (src.len < 4) return err.ShortBuffer;
        const raw: u32 = (@as(u32, src[0]) << 24)
            | (@as(u32, src[1]) << 16)
            | (@as(u32, src[2]) << 8)
            | @as(u32, src[3]);
        return UInt31WithReserved.fromU32(raw);
    }
}

/// 9-byte HTTP/2 Frame Header (big-endian on wire).
export struct FrameHeader {
    length: u24 = 0,
    frame_type: FrameType = .SETTINGS,
    flags: u8 = 0,
    stream_id: u31 = 0,
    reserved: bool = false,

    pub fn hasFlag(flag: u8): bool {
        return (_.flags & flag) != 0;
    }

    pub fn isEndStream(): bool {
        return _.hasFlag(FLAG_END_STREAM);
    }

    pub fn isEndHeaders(): bool {
        return _.hasFlag(FLAG_END_HEADERS);
    }

    pub fn isAck(): bool {
        return _.hasFlag(FLAG_ACK);
    }

    pub fn encode(dst: []u8): usize! {
        if (dst.len < FRAME_HEADER_SIZE) return err.BufferTooSmall;
        dst[0] = @truncate(_.length >> 16);
        dst[1] = @truncate(_.length >> 8);
        dst[2] = @truncate(_.length);
        dst[3] = @intFromEnum(_.frame_type);
        dst[4] = _.flags;
        const sid: u32 = (if (_.reserved) @as(u32, 0x80000000) else @as(u32, 0)) | @as(u32, _.stream_id);
        dst[5] = @truncate(sid >> 24);
        dst[6] = @truncate(sid >> 16);
        dst[7] = @truncate(sid >> 8);
        dst[8] = @truncate(sid);
        return FRAME_HEADER_SIZE;
    }

    pub static fn decode(src: []const u8): FrameHeader! {
        if (src.len < FRAME_HEADER_SIZE) return err.ShortBuffer;
        const len: u24 = (@as(u24, src[0]) << 16)
            | (@as(u24, src[1]) << 8)
            | @as(u24, src[2]);
        const ft: FrameType = @enumFromInt(src[3]);
        const fl: u8 = src[4];
        const sid = UInt31WithReserved.decodeBe(src[5..9])?;
        return FrameHeader{
            length: len,
            frame_type: ft,
            flags: fl,
            stream_id: sid.value,
            reserved: sid.reserved,
        };
    }
}

/// 5-byte HTTP/2 PRIORITY payload.
export struct StreamPriority {
    exclusive: bool = false,
    stream_dependency: u31 = 0,
    weight: u8 = 0,

    pub fn encode(dst: []u8): usize! {
        if (dst.len < STREAM_PRIORITY_SIZE) return err.BufferTooSmall;
        const dep: u32 = (if (_.exclusive) @as(u32, 0x80000000) else @as(u32, 0)) | @as(u32, _.stream_dependency);
        dst[0] = @truncate(dep >> 24);
        dst[1] = @truncate(dep >> 16);
        dst[2] = @truncate(dep >> 8);
        dst[3] = @truncate(dep);
        dst[4] = _.weight;
        return STREAM_PRIORITY_SIZE;
    }

    pub static fn decode(src: []const u8): StreamPriority! {
        if (src.len < STREAM_PRIORITY_SIZE) return err.ShortBuffer;
        const dep = UInt31WithReserved.decodeBe(src[0..4])?;
        return StreamPriority{
            exclusive: dep.reserved,
            stream_dependency: dep.value,
            weight: src[4],
        };
    }
}

/// 6-byte HTTP/2 SETTINGS parameter unit.
export struct SettingsUnit {
    setting: SettingsType = .HEADER_TABLE_SIZE,
    value: u32 = 0,

    pub fn encode(dst: []u8): usize! {
        if (dst.len < SETTINGS_UNIT_SIZE) return err.BufferTooSmall;
        const id: u16 = @intFromEnum(_.setting);
        dst[0] = @truncate(id >> 8);
        dst[1] = @truncate(id);
        dst[2] = @truncate(_.value >> 24);
        dst[3] = @truncate(_.value >> 16);
        dst[4] = @truncate(_.value >> 8);
        dst[5] = @truncate(_.value);
        return SETTINGS_UNIT_SIZE;
    }

    pub static fn decode(src: []const u8): SettingsUnit! {
        if (src.len < SETTINGS_UNIT_SIZE) return err.ShortBuffer;
        const id: u16 = (@as(u16, src[0]) << 8) | @as(u16, src[1]);
        const val: u32 = (@as(u32, src[2]) << 24)
            | (@as(u32, src[3]) << 16)
            | (@as(u32, src[4]) << 8)
            | @as(u32, src[5]);
        return SettingsUnit{
            setting: @enumFromInt(id),
            value: val,
        };
    }
}

/// Check if `bytes` starts with the standard 24-byte HTTP/2 client connection preface.
export fn hasClientPreface(bytes: []const u8): bool {
    if (bytes.len < CLIENT_PREFACE.len) return false;
    let i: usize = 0;
    while (i < CLIENT_PREFACE.len) : (i += 1) {
        if (bytes[i] != CLIENT_PREFACE[i]) return false;
    }
    return true;
}
