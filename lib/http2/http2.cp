// Source: cp/lib/http2/http2.cp (RFC 9113 HTTP/2 Binary Framing, Stream State & Flow Control Engine)
// Pure CP zero-C-dependency RFC 9113 HTTP/2 framing, stream state, and flow control library.

const std = @import("std");

/// Client connection preface (24 bytes) per RFC 9113 Section 3.4.
pub const CONNECTION_PREFACE: []const u8 = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

/// Fixed 9-octet HTTP/2 frame header size per RFC 9113 Section 4.1.
pub const FRAME_HEADER_SIZE: usize = 9;

/// Default maximum frame payload size (2^14 = 16,384 octets) per RFC 9113 Section 4.2.
pub const DEFAULT_MAX_FRAME_SIZE: u32 = 16384;

/// Maximum configurable frame payload size (2^24 - 1 = 16,777,215 octets) per RFC 9113 Section 4.2.
pub const MAX_ALLOWED_FRAME_SIZE: u32 = 16777215;

/// Default initial flow-control window size (2^16 - 1 = 65,535 octets) per RFC 9113 Section 6.9.2.
pub const DEFAULT_INITIAL_WINDOW_SIZE: u32 = 65535;

/// Maximum flow-control window size (2^31 - 1 = 2,147,483,647 octets) per RFC 9113 Section 6.9.1.
pub const MAX_WINDOW_SIZE: u32 = 0x7FFFFFFF;

/// HTTP/2 framing, protocol, and flow-control error set.
pub err Http2Error {
    FrameSizeError,
    ProtocolError,
    FlowControlError,
    StreamClosed,
    InvalidSettingsLength,
    InvalidWindowIncrement,
    OutOfMemory,
}

/// HTTP/2 frame types defined in RFC 9113 Section 6.
pub enum FrameType: u8 {
    Data = 0x0,
    Headers = 0x1,
    Priority = 0x2,
    RstStream = 0x3,
    Settings = 0x4,
    PushPromise = 0x5,
    Ping = 0x6,
    Goaway = 0x7,
    WindowUpdate = 0x8,
    Continuation = 0x9,
}

/// Standard HTTP/2 frame flags defined in RFC 9113 Section 6.
pub static class FrameFlags {
    pub static const END_STREAM: u8 = 0x1;
    pub static const ACK: u8 = 0x1;
    pub static const END_HEADERS: u8 = 0x4;
    pub static const PADDED: u8 = 0x8;
    pub static const PRIORITY: u8 = 0x20;
}

/// HTTP/2 error codes defined in RFC 9113 Section 7.
pub enum Http2ErrorCode: u32 {
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
    ConnectError = 0xa,
    EnhanceYourCalm = 0xb,
    InadequateSecurity = 0xc,
    Http11Required = 0xd,
}

/// Defined SETTINGS parameter identifiers per RFC 9113 Section 6.5.2.
pub static class SettingId {
    pub static const HEADER_TABLE_SIZE: u16 = 0x1;
    pub static const ENABLE_PUSH: u16 = 0x2;
    pub static const MAX_CONCURRENT_STREAMS: u16 = 0x3;
    pub static const INITIAL_WINDOW_SIZE: u16 = 0x4;
    pub static const MAX_FRAME_SIZE: u16 = 0x5;
    pub static const MAX_HEADER_LIST_SIZE: u16 = 0x6;
}

/// A single 6-byte SETTINGS parameter entry (`id: u16`, `value: u32`).
pub struct SettingParam {
    id: u16 = 0,
    value: u32 = 0,

    pub static fn init(id: u16, value: u32): SettingParam {
        return SettingParam { id: id, value: value };
    }
}

/// Decoded GOAWAY frame payload.
pub struct GoawayPayload {
    last_stream_id: u31 = 0,
    error_code: u32 = 0,
    debug_data: []const u8 = [],
}

/// HTTP/2 stream states per RFC 9113 Section 5.1.
pub enum StreamState: u8 {
    Idle = 0,
    ReservedLocal = 1,
    ReservedRemote = 2,
    Open = 3,
    HalfClosedLocal = 4,
    HalfClosedRemote = 5,
    Closed = 6,
}

/// HTTP/2 stream state transition helper per RFC 9113 Section 5.1.
pub struct Http2Stream {
    id: u31 = 0,
    state: StreamState = StreamState.Idle,

    pub static fn init(id: u31): Http2Stream {
        return Http2Stream { id: id, state: StreamState.Idle };
    }

    pub fn sendHeaders(*_, end_stream: bool): ! Http2Error {
        switch (_.state) {
            StreamState.Idle => {
                _.state = if (end_stream) StreamState.HalfClosedLocal else StreamState.Open;
            },
            StreamState.ReservedLocal => {
                _.state = if (end_stream) StreamState.Closed else StreamState.HalfClosedRemote;
            },
            StreamState.Open => {
                if (end_stream) {
                    _.state = StreamState.HalfClosedLocal;
                }
            },
            StreamState.HalfClosedRemote => {
                if (end_stream) {
                    _.state = StreamState.Closed;
                }
            },
            else => return Http2Error.StreamClosed,
        }
    }

    pub fn recvHeaders(*_, end_stream: bool): ! Http2Error {
        switch (_.state) {
            StreamState.Idle => {
                _.state = if (end_stream) StreamState.HalfClosedRemote else StreamState.Open;
            },
            StreamState.ReservedRemote => {
                _.state = if (end_stream) StreamState.Closed else StreamState.HalfClosedLocal;
            },
            StreamState.Open => {
                if (end_stream) {
                    _.state = StreamState.HalfClosedRemote;
                }
            },
            StreamState.HalfClosedLocal => {
                if (end_stream) {
                    _.state = StreamState.Closed;
                }
            },
            else => return Http2Error.StreamClosed,
        }
    }

    pub fn reset(*_) {
        _.state = StreamState.Closed;
    }
}

/// Decoded or constructed HTTP/2 frame.
/// When returned by `FrameStreamDecoder.nextFrame()`, `payload` is owned by the caller
/// and must be freed via `frame.deinit(allocator)` (or `allocator.free(frame.payload)`).
pub struct Http2Frame {
    frame_type: FrameType = FrameType.Data,
    flags: u8 = 0,
    stream_id: u31 = 0,
    payload: []const u8 = [],

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.payload.len > 0) {
            allocator.free(_.payload);
            _.payload = [];
        }
    }

    pub fn hasFlag(flag: u8): bool {
        return (_.flags & flag) != 0;
    }
}

fn parseFrameType(raw: u8): FrameType ! Http2Error {
    switch (raw) {
        0x0 => return FrameType.Data,
        0x1 => return FrameType.Headers,
        0x2 => return FrameType.Priority,
        0x3 => return FrameType.RstStream,
        0x4 => return FrameType.Settings,
        0x5 => return FrameType.PushPromise,
        0x6 => return FrameType.Ping,
        0x7 => return FrameType.Goaway,
        0x8 => return FrameType.WindowUpdate,
        0x9 => return FrameType.Continuation,
        else => return Http2Error.ProtocolError,
    }
}

/// Verifies whether `data` starts with the 24-byte HTTP/2 client connection preface.
pub fn hasConnectionPreface(data: []const u8): bool {
    if (data.len < CONNECTION_PREFACE.len) {
        return false;
    }
    return std.mem.eql(u8, data[0..CONNECTION_PREFACE.len], CONNECTION_PREFACE);
}

/// Encodes a raw HTTP/2 frame with a 9-byte big-endian header, clearing reserved bit `R`.
pub fn encodeFrame(
    allocator: std.mem.Allocator,
    frame_type: FrameType,
    flags: u8,
    stream_id: u31,
    payload: []const u8,
): ([]u8) ! Http2Error {
    if (payload.len > (MAX_ALLOWED_FRAME_SIZE as usize)) {
        return Http2Error.FrameSizeError;
    }
    const total = FRAME_HEADER_SIZE + payload.len;
    let out = allocator.alloc(u8, total) catch return Http2Error.OutOfMemory;

    const plen: u32 = payload.len as u32;
    out[0] = ((plen >> 16) & 0xFF) as u8;
    out[1] = ((plen >> 8) & 0xFF) as u8;
    out[2] = (plen & 0xFF) as u8;
    out[3] = frame_type as u8;
    out[4] = flags;
    const masked_sid: u32 = (stream_id as u32) & 0x7FFFFFFF;
    std.mem.writeInt(u32, out[5..9], masked_sid, .big);

    if (payload.len > 0) {
        @memcpy(out[FRAME_HEADER_SIZE..], payload);
    }
    return out;
}

/// Encodes a SETTINGS frame (6 bytes per SettingParam, or empty payload with ACK flag).
pub fn encodeSettings(
    allocator: std.mem.Allocator,
    params: []const SettingParam,
    ack: bool,
): ([]u8) ! Http2Error {
    if (ack) {
        if (params.len > 0) {
            return Http2Error.ProtocolError;
        }
        return encodeFrame(allocator, FrameType.Settings, FrameFlags.ACK, 0, []);
    }

    const payload_len = params.len * 6;
    let payload = allocator.alloc(u8, payload_len) catch return Http2Error.OutOfMemory;
    defer allocator.free(payload);

    let i: usize = 0;
    while (i < params.len) : (i += 1) {
        const off = i * 6;
        std.mem.writeInt(u16, payload[off .. off + 2], params[i].id, .big);
        std.mem.writeInt(u32, payload[off + 2 .. off + 6], params[i].value, .big);
    }
    return encodeFrame(allocator, FrameType.Settings, 0, 0, payload);
}

/// Decodes a SETTINGS frame payload into an owned slice of `SettingParam`.
pub fn decodeSettings(
    allocator: std.mem.Allocator,
    frame: Http2Frame,
): ([]SettingParam) ! Http2Error {
    if (frame.frame_type != FrameType.Settings or frame.stream_id != 0) {
        return Http2Error.ProtocolError;
    }
    if ((frame.flags & FrameFlags.ACK) != 0) {
        if (frame.payload.len != 0) {
            return Http2Error.FrameSizeError;
        }
        return allocator.alloc(SettingParam, 0) catch return Http2Error.OutOfMemory;
    }
    if ((frame.payload.len % 6) != 0) {
        return Http2Error.InvalidSettingsLength;
    }

    const count = frame.payload.len / 6;
    let out = allocator.alloc(SettingParam, count) catch return Http2Error.OutOfMemory;
    let i: usize = 0;
    while (i < count) : (i += 1) {
        const off = i * 6;
        const id = std.mem.readInt(u16, frame.payload[off .. off + 2], .big);
        const value = std.mem.readInt(u32, frame.payload[off + 2 .. off + 6], .big);
        out[i] = SettingParam { id: id, value: value };
    }
    return out;
}

/// Encodes an 8-byte PING frame on stream 0.
pub fn encodePing(
    allocator: std.mem.Allocator,
    opaque_data: [8]u8,
    ack: bool,
): ([]u8) ! Http2Error {
    const flags: u8 = if (ack) FrameFlags.ACK else 0;
    return encodeFrame(allocator, FrameType.Ping, flags, 0, opaque_data[0..]);
}

/// Decodes an 8-byte PING frame payload.
pub fn decodePing(frame: Http2Frame): ([8]u8) ! Http2Error {
    if (frame.frame_type != FrameType.Ping or frame.stream_id != 0) {
        return Http2Error.ProtocolError;
    }
    if (frame.payload.len != 8) {
        return Http2Error.FrameSizeError;
    }
    let out: [8]u8 = undefined;
    @memcpy(out[0..], frame.payload);
    return out;
}

/// Encodes a GOAWAY frame on stream 0.
pub fn encodeGoaway(
    allocator: std.mem.Allocator,
    last_stream_id: u31,
    error_code: Http2ErrorCode,
    debug_data: []const u8,
): ([]u8) ! Http2Error {
    const payload_len = 8 + debug_data.len;
    let payload = allocator.alloc(u8, payload_len) catch return Http2Error.OutOfMemory;
    defer allocator.free(payload);

    std.mem.writeInt(u32, payload[0..4], (last_stream_id as u32) & 0x7FFFFFFF, .big);
    std.mem.writeInt(u32, payload[4..8], error_code as u32, .big);
    if (debug_data.len > 0) {
        @memcpy(payload[8..], debug_data);
    }
    return encodeFrame(allocator, FrameType.Goaway, 0, 0, payload);
}

/// Decodes a GOAWAY frame payload (slice `debug_data` borrows from `frame.payload`).
pub fn decodeGoaway(frame: Http2Frame): GoawayPayload ! Http2Error {
    if (frame.frame_type != FrameType.Goaway or frame.stream_id != 0) {
        return Http2Error.ProtocolError;
    }
    if (frame.payload.len < 8) {
        return Http2Error.FrameSizeError;
    }
    const last_sid = (std.mem.readInt(u32, frame.payload[0..4], .big) & 0x7FFFFFFF) as u31;
    const err_code = std.mem.readInt(u32, frame.payload[4..8], .big);
    return GoawayPayload {
        last_stream_id: last_sid,
        error_code: err_code,
        debug_data: frame.payload[8..],
    };
}

/// Encodes a WINDOW_UPDATE frame, validating increment in `1..0x7FFFFFFF`.
pub fn encodeWindowUpdate(
    allocator: std.mem.Allocator,
    stream_id: u31,
    increment: u32,
): ([]u8) ! Http2Error {
    if (increment == 0 or increment > MAX_WINDOW_SIZE) {
        return Http2Error.InvalidWindowIncrement;
    }
    let payload: [4]u8 = undefined;
    std.mem.writeInt(u32, payload[0..4], increment & 0x7FFFFFFF, .big);
    return encodeFrame(allocator, FrameType.WindowUpdate, 0, stream_id, payload[0..]);
}

/// Decodes a WINDOW_UPDATE frame and validates non-zero 31-bit increment.
pub fn decodeWindowUpdate(frame: Http2Frame): u32 ! Http2Error {
    if (frame.frame_type != FrameType.WindowUpdate) {
        return Http2Error.ProtocolError;
    }
    if (frame.payload.len != 4) {
        return Http2Error.FrameSizeError;
    }
    const inc = std.mem.readInt(u32, frame.payload[0..4], .big) & 0x7FFFFFFF;
    if (inc == 0) {
        return Http2Error.InvalidWindowIncrement;
    }
    return inc;
}

/// Encodes a RST_STREAM frame on a non-zero stream.
pub fn encodeRstStream(
    allocator: std.mem.Allocator,
    stream_id: u31,
    error_code: Http2ErrorCode,
): ([]u8) ! Http2Error {
    if (stream_id == 0) {
        return Http2Error.ProtocolError;
    }
    let payload: [4]u8 = undefined;
    std.mem.writeInt(u32, payload[0..4], error_code as u32, .big);
    return encodeFrame(allocator, FrameType.RstStream, 0, stream_id, payload[0..]);
}

/// Decodes a RST_STREAM frame error code.
pub fn decodeRstStream(frame: Http2Frame): u32 ! Http2Error {
    if (frame.frame_type != FrameType.RstStream or frame.stream_id == 0) {
        return Http2Error.ProtocolError;
    }
    if (frame.payload.len != 4) {
        return Http2Error.FrameSizeError;
    }
    return std.mem.readInt(u32, frame.payload[0..4], .big);
}

/// Encodes a DATA frame on a non-zero stream, supporting optional END_STREAM and PADDED padding.
pub fn encodeDataFrame(
    allocator: std.mem.Allocator,
    stream_id: u31,
    data: []const u8,
    end_stream: bool,
    pad_length: u8,
): ([]u8) ! Http2Error {
    if (stream_id == 0) {
        return Http2Error.ProtocolError;
    }
    let flags: u8 = if (end_stream) FrameFlags.END_STREAM else 0;
    if (pad_length == 0) {
        return encodeFrame(allocator, FrameType.Data, flags, stream_id, data);
    }

    flags |= FrameFlags.PADDED;
    const plen = 1 + data.len + (pad_length as usize);
    let payload = allocator.alloc(u8, plen) catch return Http2Error.OutOfMemory;
    defer allocator.free(payload);

    payload[0] = pad_length;
    if (data.len > 0) {
        @memcpy(payload[1 .. 1 + data.len], data);
    }
    @memset(payload[1 + data.len ..], 0);
    return encodeFrame(allocator, FrameType.Data, flags, stream_id, payload);
}

/// Strips optional PADDED bytes from a DATA frame payload and returns the application data slice.
pub fn decodeDataPayload(frame: Http2Frame): ([]const u8) ! Http2Error {
    if (frame.frame_type != FrameType.Data or frame.stream_id == 0) {
        return Http2Error.ProtocolError;
    }
    if ((frame.flags & FrameFlags.PADDED) == 0) {
        return frame.payload;
    }
    if (frame.payload.len == 0) {
        return Http2Error.ProtocolError;
    }
    const pad_len = frame.payload[0] as usize;
    if (1 + pad_len > frame.payload.len) {
        return Http2Error.ProtocolError;
    }
    return frame.payload[1 .. frame.payload.len - pad_len];
}

/// Incremental streaming HTTP/2 frame decoder handling arbitrary chunk boundaries,
/// partial headers/payloads, and sticky frames.
pub struct FrameStreamDecoder {
    alloc: std.mem.Allocator,
    max_frame_size: u32,
    buf: std.ArrayList(u8),
    read_offset: usize,

    pub static fn init(allocator: std.mem.Allocator, max_frame_size: u32): FrameStreamDecoder {
        const effective = if (max_frame_size == 0) DEFAULT_MAX_FRAME_SIZE else max_frame_size;
        return FrameStreamDecoder {
            alloc: allocator,
            max_frame_size: effective,
            buf: std.ArrayList(u8).empty,
            read_offset: 0,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
        _.read_offset = 0;
    }

    pub fn setMaxFrameSize(*_, max_frame_size: u32): ! Http2Error {
        if (max_frame_size < DEFAULT_MAX_FRAME_SIZE or max_frame_size > MAX_ALLOWED_FRAME_SIZE) {
            return Http2Error.FrameSizeError;
        }
        _.max_frame_size = max_frame_size;
    }

    pub fn feed(*_, chunk: []const u8): ! Http2Error {
        if (chunk.len == 0) {
            return;
        }
        _.buf.appendSlice(_.alloc, chunk) catch return Http2Error.OutOfMemory;
    }

    pub fn bufferedLen(): usize {
        return _.buf.items.len - _.read_offset;
    }

    pub fn nextFrame(*_): (Http2Frame?) ! Http2Error {
        const avail = _.buf.items.len - _.read_offset;
        if (avail < FRAME_HEADER_SIZE) {
            return null;
        }

        const hdr = _.buf.items[_.read_offset .. _.read_offset + FRAME_HEADER_SIZE];
        const plen: u32 = ((hdr[0] as u32) << 16) | ((hdr[1] as u32) << 8) | (hdr[2] as u32);
        if (plen > _.max_frame_size) {
            return Http2Error.FrameSizeError;
        }

        const ftype = try parseFrameType(hdr[3]);
        const flags = hdr[4];
        const stream_id: u31 = (std.mem.readInt(u32, hdr[5..9], .big) & 0x7FFFFFFF) as u31;

        const frame_total = FRAME_HEADER_SIZE + (plen as usize);
        if (avail < frame_total) {
            return null;
        }

        // Validate frame-level structural invariants per RFC 9113 Section 6.
        switch (ftype) {
            FrameType.Settings => {
                if (stream_id != 0) {
                    return Http2Error.ProtocolError;
                }
                if ((flags & FrameFlags.ACK) != 0) {
                    if (plen != 0) {
                        return Http2Error.FrameSizeError;
                    }
                } else if ((plen % 6) != 0) {
                    return Http2Error.InvalidSettingsLength;
                }
            },
            FrameType.Ping => {
                if (stream_id != 0) {
                    return Http2Error.ProtocolError;
                }
                if (plen != 8) {
                    return Http2Error.FrameSizeError;
                }
            },
            FrameType.Goaway => {
                if (stream_id != 0) {
                    return Http2Error.ProtocolError;
                }
                if (plen < 8) {
                    return Http2Error.FrameSizeError;
                }
            },
            FrameType.WindowUpdate => {
                if (plen != 4) {
                    return Http2Error.FrameSizeError;
                }
            },
            FrameType.RstStream => {
                if (stream_id == 0) {
                    return Http2Error.ProtocolError;
                }
                if (plen != 4) {
                    return Http2Error.FrameSizeError;
                }
            },
            else => {},
        }

        const payload_start = _.read_offset + FRAME_HEADER_SIZE;
        const payload_end = payload_start + (plen as usize);
        let payload: []u8 = [];
        if (plen > 0) {
            payload = _.alloc.alloc(u8, plen as usize) catch return Http2Error.OutOfMemory;
            @memcpy(payload, _.buf.items[payload_start..payload_end]);
        }

        _.read_offset += frame_total;
        if (_.read_offset == _.buf.items.len) {
            _.buf.clearRetainingCapacity();
            _.read_offset = 0;
        } else if (_.read_offset >= 4096) {
            const rem = _.buf.items.len - _.read_offset;
            std.mem.copyForwards(u8, _.buf.items[0..rem], _.buf.items[_.read_offset..]);
            _.buf.shrinkRetainingCapacity(rem);
            _.read_offset = 0;
        }

        return Http2Frame {
            frame_type: ftype,
            flags: flags,
            stream_id: stream_id,
            payload: payload,
        };
    }
}

/// HTTP/2 connection or stream flow-control window per RFC 9113 Section 6.9.
pub struct FlowWindow {
    size: i64,

    pub static fn init(initial_size: u32): FlowWindow {
        return FlowWindow { size: initial_size as i64 };
    }

    pub fn available(): i64 {
        return _.size;
    }

    pub fn consume(*_, bytes: u32): ! Http2Error {
        const b = bytes as i64;
        if (b > _.size) {
            return Http2Error.FlowControlError;
        }
        _.size -= b;
    }

    pub fn replenish(*_, increment: u32): ! Http2Error {
        if (increment == 0 or increment > MAX_WINDOW_SIZE) {
            return Http2Error.InvalidWindowIncrement;
        }
        const next_size = _.size + (increment as i64);
        if (next_size > (MAX_WINDOW_SIZE as i64)) {
            return Http2Error.FlowControlError;
        }
        _.size = next_size;
    }

    pub fn updateInitialSize(*_, new_initial: u32, old_initial: u32): ! Http2Error {
        if (new_initial > MAX_WINDOW_SIZE) {
            return Http2Error.FlowControlError;
        }
        const delta: i64 = (new_initial as i64) - (old_initial as i64);
        const next_size = _.size + delta;
        if (next_size > (MAX_WINDOW_SIZE as i64)) {
            return Http2Error.FlowControlError;
        }
        _.size = next_size;
    }
}
