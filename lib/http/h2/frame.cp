// Source: /data/dev/bun/src/http/H2FrameParser.cp
// lib/http/h2/frame.cp
// CP HTTP/2 Frame & Wire Protocol (cap:http)
// RFC 7540 HTTP/2 binary framing types and constants

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
}
