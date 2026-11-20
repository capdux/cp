// Source: /data/dev/bun/src/http_jsc/websocket_client/
// ============================================================================
// Absorbed from Bun: bun/src/ws/opcode.cp
// Upstream Sync: Refer to bun/src/ws/opcode.zig for future upstream updates.
// ============================================================================
// lib/ws/opcode.cp
// RFC 6455 WebSocket Opcodes & Close Status Codes in CP

pub enum Opcode: u4 {
    Continue = 0x0,
    Text = 0x1,
    Binary = 0x2,
    Close = 0x8,
    Ping = 0x9,
    Pong = 0xA,

    pub fn isControl(): bool {
        return (@intFromEnum(_) & 0x8) != 0;
    }

    pub fn isData(): bool {
        return _ == .Text || _ == .Binary || _ == .Continue;
    }

    /// Safely converts a raw 4-bit opcode into `Opcode`, returning `null` for reserved opcodes.
    pub static fn fromU4(raw: u4): Opcode? {
        return switch (raw) {
            0x0 => Opcode.Continue,
            0x1 => Opcode.Text,
            0x2 => Opcode.Binary,
            0x8 => Opcode.Close,
            0x9 => Opcode.Ping,
            0xA => Opcode.Pong,
            else => null,
        };
    }
}

pub enum CloseCode: u16 {
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

    /// Validates whether `code` is legal to transmit inside a WebSocket Close frame (RFC 6455 Section 7.4.1).
    pub static fn isValidWireCode(code: u16): bool {
        if (code >= 1000 && code <= 1003) return true;
        if (code >= 1007 && code <= 1011) return true;
        if (code >= 3000 && code <= 4999) return true;
        return false;
    }

    pub static fn fromU16(code: u16): CloseCode? {
        return switch (code) {
            1000 => CloseCode.NormalClosure,
            1001 => CloseCode.GoingAway,
            1002 => CloseCode.ProtocolError,
            1003 => CloseCode.UnsupportedData,
            1005 => CloseCode.NoStatusReceived,
            1006 => CloseCode.AbnormalClosure,
            1007 => CloseCode.InvalidFramePayload,
            1008 => CloseCode.PolicyViolation,
            1009 => CloseCode.MessageTooBig,
            1010 => CloseCode.MissingExtension,
            1011 => CloseCode.InternalError,
            else => null,
        };
    }
}
