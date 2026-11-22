// Source: Absorbed from /data/dev/bun/src/http/H2FrameParser.cp | Synced: 2026-11-22T11:12:13Z
// lib/http/h2/frame.cp
// CP HTTP/2 Frame & Wire Protocol (cap:http)
// RFC 7540 HTTP/2 binary framing types, payload codecs, and constants

const std = @import("std");

pub const CLIENT_PREFACE: []const u8 = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

pub const MAX_WINDOW_SIZE: i32 = 0x7FFFFFFF;
pub const MAX_HEADER_TABLE_SIZE: u32 = 0xFFFFFFFF;
pub const MAX_STREAM_ID: i32 = 0x7FFFFFFF;
pub const MAX_FRAME_SIZE: u32 = 16777215; // 2^24 - 1
pub const DEFAULT_WINDOW_SIZE: u16 = 65535;
pub const DEFAULT_MAX_FRAME_SIZE: u24 = 16384;

pub enum FrameType: u8 {
    Data = 0x00,
    Headers = 0x01,
    Priority = 0x02,
    RstStream = 0x03,
    Settings = 0x04,
    PushPromise = 0x05,
    Ping = 0x06,
    Goaway = 0x07,
    WindowUpdate = 0x08,
    Continuation = 0x09,
    AltSvc = 0x0A,
    Origin = 0x0C,
}

pub enum PingFrameFlags: u8 {
    Ack = 0x1,
}

pub enum DataFrameFlags: u8 {
    EndStream = 0x1,
    Padded = 0x8,
}

pub enum HeadersFrameFlags: u8 {
    EndStream = 0x1,
    EndHeaders = 0x4,
    Padded = 0x8,
    Priority = 0x20,
}

pub enum SettingsFlags: u8 {
    Ack = 0x1,
}

pub enum ErrorCode: u32 {
    NoError = 0x0,
    ProtocolError = 0x1,
    InternalError = 0x2,
    FlowControlError = 0x3,
    SettingsTimeout = 0x4,
    StreamClosed = 0x5,
    FrameSizeError = 0x6,
    RefusedStream = 0x7,
    Cancel = 0x8,
    CompressionError = 0x9,
    ConnectError = 0x0A,
    EnhanceYourCalm = 0x0B,
    InadequateSecurity = 0x0C,
    Http1_1Required = 0x0D,
}

pub enum SettingsType: u16 {
    HeaderTableSize = 0x1,
    EnablePush = 0x2,
    MaxConcurrentStreams = 0x3,
    InitialWindowSize = 0x4,
    MaxFrameSize = 0x5,
    MaxHeaderListSize = 0x6,
    EnableConnectProtocol = 0x8,
    NoRfc7540Priorities = 0x9,
}

pub struct UInt31WithReserved {
    pub reserved: bool = false;
    pub value: u32 = 0;

    pub static fn decode(src: []const u8): UInt31WithReserved? {
        if (src.len < 4) return null;
        const raw = std.mem.readInt(u32, src[0..4], .big);
        return UInt31WithReserved{
            reserved: (raw & 0x80000000) != 0,
            value: raw & 0x7FFFFFFF,
        };
    }

    pub fn encode(out: *[4]u8) {
        const flag: u32 = if (_.reserved) 0x80000000 else 0;
        std.mem.writeInt(u32, out, (_.value & 0x7FFFFFFF) | flag, .big);
    }
}

pub struct StreamPriority {
    pub exclusive: bool = false;
    pub dependency: u32 = 0;
    pub weight: u8 = 15;

    pub const byte_size: usize = 5;

    pub static fn decode(src: []const u8): StreamPriority? {
        if (src.len < 5) return null;
        const dep = UInt31WithReserved.decode(src[0..4]) ?? return null;
        return StreamPriority{
            exclusive: dep.reserved,
            dependency: dep.value,
            weight: src[4],
        };
    }

    pub fn encode(out: *[5]u8) {
        let tmp: [4]u8 = undefined;
        const u = UInt31WithReserved{ reserved: _.exclusive, value: _.dependency };
        u.encode(&tmp);
        @memcpy(out[0..4], &tmp);
        out[4] = _.weight;
    }
}

pub struct FrameHeader {
    pub length: u24 = 0;
    pub type: FrameType = FrameType.Settings;
    pub flags: u8 = 0;
    pub stream_id: u32 = 0;

    pub const byte_size: usize = 9;

    pub fn encode(out: *[9]u8) {
        out[0] = ((_.length >> 16) & 0xFF) as u8;
        out[1] = ((_.length >> 8) & 0xFF) as u8;
        out[2] = (_.length & 0xFF) as u8;
        out[3] = @intFromEnum(_.type);
        out[4] = _.flags;
        std.mem.writeInt(u32, out[5..9], _.stream_id & 0x7FFFFFFF, .big);
    }

    pub fn encodeSlice(out: []u8): usize {
        if (out.len < 9) return 0;
        let tmp: [9]u8 = undefined;
        _.encode(&tmp);
        @memcpy(out[0..9], &tmp);
        return 9;
    }

    pub static fn decode(src: []const u8): FrameHeader? {
        if (src.len < 9) return null;
        const len: u24 = ((src[0] as u24) << 16) | ((src[1] as u24) << 8) | (src[2] as u24);
        const raw_type = src[3];
        const flags = src[4];
        const stream_id = std.mem.readInt(u32, src[5..9], .big) & 0x7FFFFFFF;
        return FrameHeader{
            length: len,
            type: @enumFromInt(raw_type),
            flags: flags,
            stream_id: stream_id,
        };
    }
}

pub struct SettingsPayloadUnit {
    pub type: SettingsType = SettingsType.HeaderTableSize;
    pub value: u32 = 0;

    pub const byte_size: usize = 6;

    pub fn encode(out: *[6]u8) {
        std.mem.writeInt(u16, out[0..2], @intFromEnum(_.type), .big);
        std.mem.writeInt(u32, out[2..6], _.value, .big);
    }

    pub static fn decode(src: []const u8): SettingsPayloadUnit? {
        if (src.len < 6) return null;
        const raw_id = std.mem.readInt(u16, src[0..2], .big);
        const val = std.mem.readInt(u32, src[2..6], .big);
        return SettingsPayloadUnit{
            type: @enumFromInt(raw_id),
            value: val,
        };
    }
}

pub struct RstStreamFrame {
    pub error_code: ErrorCode = ErrorCode.NoError;
    pub const byte_size: usize = 4;

    pub fn encode(out: *[4]u8) {
        std.mem.writeInt(u32, out, @intFromEnum(_.error_code), .big);
    }

    pub static fn decode(src: []const u8): RstStreamFrame? {
        if (src.len < 4) return null;
        const raw = std.mem.readInt(u32, src[0..4], .big);
        return RstStreamFrame{ error_code: @enumFromInt(raw) };
    }
}

pub struct WindowUpdateFrame {
    pub increment: u32 = 0;
    pub const byte_size: usize = 4;

    pub fn encode(out: *[4]u8) {
        std.mem.writeInt(u32, out, _.increment & 0x7FFFFFFF, .big);
    }

    pub static fn decode(src: []const u8): WindowUpdateFrame? {
        if (src.len < 4) return null;
        const val = std.mem.readInt(u32, src[0..4], .big) & 0x7FFFFFFF;
        return WindowUpdateFrame{ increment: val };
    }
}

pub struct PingFrame {
    pub opaque_data: [8]u8 = [0; 8];
    pub const byte_size: usize = 8;

    pub fn encode(out: *[8]u8) {
        @memcpy(out, &_.opaque_data);
    }

    pub static fn decode(src: []const u8): PingFrame? {
        if (src.len < 8) return null;
        let pf = PingFrame{};
        @memcpy(&pf.opaque_data, src[0..8]);
        return pf;
    }
}

pub struct GoawayFrame {
    pub last_stream_id: u32 = 0;
    pub error_code: ErrorCode = ErrorCode.NoError;
    pub debug_data: []const u8 = "";

    pub fn encode(out: []u8): usize {
        const needed = 8 + _.debug_data.len;
        if (out.len < needed) return 0;
        let id_buf: [4]u8 = undefined;
        std.mem.writeInt(u32, &id_buf, _.last_stream_id & 0x7FFFFFFF, .big);
        @memcpy(out[0..4], &id_buf);
        let code_buf: [4]u8 = undefined;
        std.mem.writeInt(u32, &code_buf, @intFromEnum(_.error_code), .big);
        @memcpy(out[4..8], &code_buf);
        if (_.debug_data.len > 0) {
            @memcpy(out[8..needed], _.debug_data);
        }
        return needed;
    }

    pub static fn decode(src: []const u8): GoawayFrame? {
        if (src.len < 8) return null;
        const last_sid = std.mem.readInt(u32, src[0..4], .big) & 0x7FFFFFFF;
        const raw_code = std.mem.readInt(u32, src[4..8], .big);
        return GoawayFrame{
            last_stream_id: last_sid,
            error_code: @enumFromInt(raw_code),
            debug_data: src[8..],
        };
    }
}
