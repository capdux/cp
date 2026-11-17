// Source: Custom implementation by Antigravity AI
/**
 * CP HTTP/2 Frame & Wire Protocol (cap:http)
 * RFC 7540 HTTP/2 binary framing types and constants
 * Ported from Bun.js http/H2FrameParser.cp
 */


pub const CLIENT_PREFACE: []const u8 = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

pub const MAX_WINDOW_SIZE: i32 = 0x7FFFFFFF;
pub const MAX_HEADER_TABLE_SIZE: u32 = 0xFFFFFFFF;
pub const MAX_STREAM_ID: i32 = 0x7FFFFFFF;
pub const MAX_FRAME_SIZE: u32 = 16777215; // 2^24 - 1
pub const DEFAULT_WINDOW_SIZE: u16 = 65535;
pub const DEFAULT_MAX_FRAME_SIZE: u24 = 16384;

pub const FrameType = enum(u8) {
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
};

pub const PingFrameFlags = enum(u8) {
    Ack = 0x1,
};

pub const DataFrameFlags = enum(u8) {
    EndStream = 0x1,
    Padded = 0x8,
};

pub const HeadersFrameFlags = enum(u8) {
    EndStream = 0x1,
    EndHeaders = 0x4,
    Padded = 0x8,
    Priority = 0x20,
};

pub const SettingsFlags = enum(u8) {
    Ack = 0x1,
};

pub const ErrorCode = enum(u32) {
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
};

pub const SettingsType = enum(u16) {
    HeaderTableSize = 0x1,
    EnablePush = 0x2,
    MaxConcurrentStreams = 0x3,
    InitialWindowSize = 0x4,
    MaxFrameSize = 0x5,
    MaxHeaderListSize = 0x6,
    EnableConnectProtocol = 0x8,
    NoRfc7540Priorities = 0x9,
};

pub const FrameHeader = struct {
    length: u24 = 0,
    type: FrameType = FrameType.Settings,
    flags: u8 = 0,
    stream_id: u32 = 0,

    pub const byte_size: usize = 9;

    pub fn encode(*_, out: *[9]u8) {
        out[0] = @as(u8, @intCast((_.length >> 16) & 0xFF));
        out[1] = @as(u8, @intCast((_.length >> 8) & 0xFF));
        out[2] = @as(u8, @intCast(_.length & 0xFF));
        out[3] = @intFromEnum(_.type);
        out[4] = _.flags;
        std.mem.writeInt(u32, out[5..9], _.stream_id & 0x7FFFFFFF, .big);
    }

    pub fn decode(src: []const u8): FrameHeader? {
        if (src.len < 9) return null;
        const len: u24 = (@as(u24, src[0]) << 16) | (@as(u24, src[1]) << 8) | @as(u24, src[2]);
        const raw_type = src[3];
        const flags = src[4];
        const stream_id = std.mem.readInt(u32, src[5..9], .big) & 0x7FFFFFFF;
        let header = FrameHeader{};
        header.length = len;
        header.type = @enumFromInt(raw_type);
        header.flags = flags;
        header.stream_id = stream_id;
        return header;
    }
};

pub const SettingsPayloadUnit = struct {
    type: SettingsType = SettingsType.HeaderTableSize,
    value: u32 = 0,

    pub const byte_size: usize = 6;

    pub fn encode(*_, out: *[6]u8) {
        std.mem.writeInt(u16, out[0..2], @intFromEnum(_.type), .big);
        std.mem.writeInt(u32, out[2..6], _.value, .big);
    }
};
