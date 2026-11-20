// Source: cp/lib/http2_frame/types.cp (RFC 9113 HTTP/2 Frame Types & Constants)
const std = @import("std");

pub const H2_PREFACE: []const u8 = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
pub const H2_HEADER_LEN: usize = 9;
pub const H2_DEFAULT_MAX_FRAME_SIZE: u32 = 16384;
pub const H2_MAX_FRAME_SIZE_LIMIT: u32 = 16777215;
pub const H2_DEFAULT_INITIAL_WINDOW_SIZE: i64 = 65535;
pub const H2_MAX_WINDOW_SIZE: i64 = 2147483647;

pub const H2_FLAG_ACK: u8 = 0x01;
pub const H2_FLAG_END_STREAM: u8 = 0x01;
pub const H2_FLAG_END_HEADERS: u8 = 0x04;
pub const H2_FLAG_PADDED: u8 = 0x08;
pub const H2_FLAG_PRIORITY: u8 = 0x20;

pub const H2Error = error{
    FrameTooLarge,
    InvalidFrameLength,
    InvalidStreamId,
    InvalidPadding,
    InvalidIncrement,
    FlowControlError,
    InvalidPreface,
    UnknownFrameType,
    InvalidSettings,
    BufferTooSmall,
    InvalidStreamState,
    OutOfMemory,
};

pub enum H2FrameType : u8 {
    data = 0,
    headers = 1,
    priority = 2,
    rst_stream = 3,
    settings = 4,
    push_promise = 5,
    ping = 6,
    goaway = 7,
    window_update = 8,
    continuation = 9,

    pub fn fromU8(raw: u8): H2FrameType? {
        return switch (raw) {
            0 => .data,
            1 => .headers,
            2 => .priority,
            3 => .rst_stream,
            4 => .settings,
            5 => .push_promise,
            6 => .ping,
            7 => .goaway,
            8 => .window_update,
            9 => .continuation,
            else => null,
        };
    }
}

pub enum H2ErrorCode : u32 {
    no_error = 0x0,
    protocol_error = 0x1,
    internal_error = 0x2,
    flow_control_error = 0x3,
    settings_timeout = 0x4,
    stream_closed = 0x5,
    frame_size_error = 0x6,
    refused_stream = 0x7,
    cancel = 0x8,
    compression_error = 0x9,
    connect_error = 0xa,
    enhance_your_calm = 0xb,
    inadequate_security = 0xc,
    http_1_1_required = 0xd,
}

pub struct H2FrameHeader {
    pub length: u32,
    pub frame_type: u8,
    pub flags: u8,
    pub stream_id: u32,

    pub fn parse(bytes: []const u8): H2Error!H2FrameHeader {
        if (bytes.len < H2_HEADER_LEN) {
            return H2Error.BufferTooSmall;
        }
        const length: u32 = (@as(u32, bytes[0]) << 16) | (@as(u32, bytes[1]) << 8) | @as(u32, bytes[2]);
        const frame_type: u8 = bytes[3];
        const flags: u8 = bytes[4];
        const stream_id: u32 = ((@as(u32, bytes[5]) & 0x7f) << 24) |
            (@as(u32, bytes[6]) << 16) |
            (@as(u32, bytes[7]) << 8) |
            @as(u32, bytes[8]);
        return H2FrameHeader{
            .length = length,
            .frame_type = frame_type,
            .flags = flags,
            .stream_id = stream_id,
        };
    }

    pub fn encode(out: []u8): H2Error!void {
        if (out.len < H2_HEADER_LEN) {
            return H2Error.BufferTooSmall;
        }
        if (_.length > H2_MAX_FRAME_SIZE_LIMIT) {
            return H2Error.FrameTooLarge;
        }
        out[0] = @intCast((_.length >> 16) & 0xff);
        out[1] = @intCast((_.length >> 8) & 0xff);
        out[2] = @intCast(_.length & 0xff);
        out[3] = _.frame_type;
        out[4] = _.flags;
        const sid = _.stream_id & 0x7fffffff;
        out[5] = @intCast((sid >> 24) & 0x7f);
        out[6] = @intCast((sid >> 16) & 0xff);
        out[7] = @intCast((sid >> 8) & 0xff);
        out[8] = @intCast(sid & 0xff);
    }

    pub fn hasFlag(flag: u8): bool {
        return (_.flags & flag) != 0;
    }

    pub fn kind(): H2FrameType? {
        return H2FrameType.fromU8(_.frame_type);
    }
}

pub struct H2Setting {
    pub id: u16,
    pub value: u32,
}

pub struct H2PrioritySpec {
    pub exclusive: bool,
    pub stream_dependency: u32,
    pub weight: u8,
}

pub struct H2DataFrame {
    pub stream_id: u32,
    pub end_stream: bool,
    pub pad_len: u8,
    pub payload: []const u8,
}

pub struct H2HeadersFrame {
    pub stream_id: u32,
    pub end_stream: bool,
    pub end_headers: bool,
    pub pad_len: u8,
    pub priority: H2PrioritySpec?,
    pub header_block: []const u8,
}

pub struct H2RstStreamFrame {
    pub stream_id: u32,
    pub error_code: u32,
}

pub struct H2SettingsFrame {
    pub ack: bool,
    pub settings: []const H2Setting,
}

pub struct H2PingFrame {
    pub ack: bool,
    pub opaque_data: [8]u8,
}

pub struct H2GoAwayFrame {
    pub last_stream_id: u32,
    pub error_code: u32,
    pub debug_data: []const u8,
}

pub struct H2WindowUpdateFrame {
    pub stream_id: u32,
    pub increment: u32,
}

pub struct H2RawFrame {
    pub header: H2FrameHeader,
    pub payload: []const u8,
}

pub const H2Frame = union(enum) {
    data: H2DataFrame,
    headers: H2HeadersFrame,
    priority: H2RawFrame,
    rst_stream: H2RstStreamFrame,
    settings: H2SettingsFrame,
    push_promise: H2RawFrame,
    ping: H2PingFrame,
    goaway: H2GoAwayFrame,
    window_update: H2WindowUpdateFrame,
    continuation: H2RawFrame,
    unknown: H2RawFrame,
};
