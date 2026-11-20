// Source: cp/lib/websocket_frame/types.cp (RFC 6455 Opcodes, Close Codes, Errors & Handshake Accept Key)

const std = @import("std");

/// RFC 6455 Section 1.3 WebSocket GUID constant.
pub const WS_GUID: []const u8 = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

/// RFC 6455 Section 5.2 WebSocket Frame Opcodes.
pub enum WsOpcode: u4 {
    Continuation = 0x0,
    Text = 0x1,
    Binary = 0x2,
    Close = 0x8,
    Ping = 0x9,
    Pong = 0xA,

    /// Returns true if this opcode represents a control frame (Close, Ping, Pong).
    pub fn isControl(): bool {
        return (@intFromEnum(_) & 0x8) != 0;
    }

    /// Returns true if this opcode represents a data frame (Continuation, Text, Binary).
    pub fn isData(): bool {
        return _ == .Continuation || _ == .Text || _ == .Binary;
    }

    /// Converts a raw 4-bit opcode into `WsOpcode`, returning `null` for reserved opcodes.
    pub static fn fromU4(raw: u4): WsOpcode? {
        return switch (raw) {
            0x0 => WsOpcode.Continuation,
            0x1 => WsOpcode.Text,
            0x2 => WsOpcode.Binary,
            0x8 => WsOpcode.Close,
            0x9 => WsOpcode.Ping,
            0xA => WsOpcode.Pong,
            else => null,
        };
    }
}

/// RFC 6455 Section 7.4.1 WebSocket Close Status Codes.
pub enum WsCloseCode: u16 {
    NormalClosure = 1000,
    GoingAway = 1001,
    ProtocolError = 1002,
    UnsupportedData = 1003,
    NoStatusReceived = 1005,
    AbnormalClosure = 1006,
    InvalidFramePayload = 1007,
    PolicyViolation = 1008,
    MessageTooBig = 1009,
    MissingExtension = 1010,
    InternalError = 1011,

    /// Validates whether `code` is legal to transmit on the wire in a Close frame (RFC 6455 Section 7.4.1).
    pub static fn isValidWireCode(code: u16): bool {
        if (code >= 1000 && code <= 1003) {
            return true;
        }
        if (code >= 1007 && code <= 1011) {
            return true;
        }
        if (code >= 3000 && code <= 4999) {
            return true;
        }
        return false;
    }

    /// Converts a raw 16-bit status code into `WsCloseCode`, returning `null` if not a named standard code.
    pub static fn fromU16(code: u16): WsCloseCode? {
        return switch (code) {
            1000 => WsCloseCode.NormalClosure,
            1001 => WsCloseCode.GoingAway,
            1002 => WsCloseCode.ProtocolError,
            1003 => WsCloseCode.UnsupportedData,
            1005 => WsCloseCode.NoStatusReceived,
            1006 => WsCloseCode.AbnormalClosure,
            1007 => WsCloseCode.InvalidFramePayload,
            1008 => WsCloseCode.PolicyViolation,
            1009 => WsCloseCode.MessageTooBig,
            1010 => WsCloseCode.MissingExtension,
            1011 => WsCloseCode.InternalError,
            else => null,
        };
    }
}

/// Unified error set for `cap:websocket_frame` encoding, decoding, and stream assembly.
pub err WsError {
    Incomplete,
    ReservedBitsSet,
    InvalidOpcode,
    ControlFrameTooLarge,
    ControlFrameFragmented,
    UnmaskedClientFrame,
    PayloadTooLarge,
    InvalidUtf8,
    UnexpectedContinuation,
    FragmentedInProgress,
    InvalidClosePayload,
    OutOfMemory,
}

/// Computes the RFC 6455 Section 4.2.2 `Sec-WebSocket-Accept` key from `Sec-WebSocket-Key`
/// using SHA-1 + WS_GUID + Base64 encoding with zero heap allocations.
pub fn computeAcceptKey(key_base64: []const u8): [28]u8 {
    let hasher = std.crypto.hash.Sha1.init(.{});
    hasher.update(key_base64);
    hasher.update(WS_GUID);
    let digest: [20]u8 = [0; 20];
    hasher.final(&digest);

    let out: [28]u8 = [0; 28];
    * = std.base64.standard.Encoder.encode(&out, &digest);
    return out;
}
