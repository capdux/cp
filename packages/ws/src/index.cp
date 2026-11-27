// @cap/ws-frame — Zero-heap RFC 6455 WebSocket binary frame codec
// Clean-room MIT implementation for CP (compatible with npm ws & Rust tungstenite).
// Accelerated with 16-lane @Vector(16, u8) SIMD 4-byte XOR masking/unmasking and @memcpy.

type WsVec16 = @Vector(16, u8);

pub const OP_CONTINUATION: u8 = 0x0;
pub const OP_TEXT: u8 = 0x1;
pub const OP_BINARY: u8 = 0x2;
pub const OP_CLOSE: u8 = 0x8;
pub const OP_PING: u8 = 0x9;
pub const OP_PONG: u8 = 0xA;

pub const CLOSE_NORMAL: u16 = 1000;
pub const CLOSE_GOING_AWAY: u16 = 1001;
pub const CLOSE_PROTOCOL_ERROR: u16 = 1002;
pub const CLOSE_UNSUPPORTED_DATA: u16 = 1003;
pub const CLOSE_INVALID_PAYLOAD: u16 = 1007;
pub const CLOSE_POLICY_VIOLATION: u16 = 1008;
pub const CLOSE_MESSAGE_TOO_BIG: u16 = 1009;
pub const CLOSE_INTERNAL_ERROR: u16 = 1011;

pub const WS_OK: i32 = 0;
pub const WS_INCOMPLETE: i32 = 1;
pub const WS_INVALID: i32 = -1;

pub struct FrameHeader {
    pub fin: bool,
    pub rsv: u8,
    pub opcode: u8,
    pub masked: bool,
    pub mask_key: [4]u8,
    pub payload_len: u64,
    pub header_len: usize,
}

pub fn isControlOpcode(opcode: u8): bool {
    return (opcode & 0x08) != 0;
}

pub fn isValidOpcode(opcode: u8): bool {
    return opcode == OP_CONTINUATION ||
        opcode == OP_TEXT ||
        opcode == OP_BINARY ||
        opcode == OP_CLOSE ||
        opcode == OP_PING ||
        opcode == OP_PONG;
}

/// Apply RFC 6455 4-byte XOR masking key in-place (symmetric for masking and unmasking).
pub fn applyMask(payload: []u8, mask_key: [4]u8) {
    let i: usize = 0;
    if (payload.len >= 16) {
        const mk16: [16]u8 = [
            mask_key[0], mask_key[1], mask_key[2], mask_key[3],
            mask_key[0], mask_key[1], mask_key[2], mask_key[3],
            mask_key[0], mask_key[1], mask_key[2], mask_key[3],
            mask_key[0], mask_key[1], mask_key[2], mask_key[3],
        ];
        const vmask: WsVec16 = mk16;
        while (i + 16 <= payload.len) {
            const vp: WsVec16 = payload[i .. i + 16][0..16].*;
            const out_arr: [16]u8 = vp ^ vmask;
            @memcpy(payload[i .. i + 16], out_arr[0..16]);
            i += 16;
        }
    }
    while (i < payload.len) {
        payload[i] = payload[i] ^ mask_key[i & 3];
        i += 1;
    }
}

/// Decode an RFC 6455 WebSocket frame header from `data`.
/// Returns `WS_OK` on success, `WS_INCOMPLETE` if more header bytes are needed, or `WS_INVALID` on protocol violation.
pub fn decodeHeader(data: []const u8, out: *FrameHeader): i32 {
    if (data.len < 2) {
        return WS_INCOMPLETE;
    }
    let b0: u8 = data[0];
    let b1: u8 = data[1];

    let fin: bool = (b0 & 0x80) != 0;
    let rsv: u8 = (b0 >> 4) & 0x07;
    let opcode: u8 = b0 & 0x0f;
    let masked: bool = (b1 & 0x80) != 0;
    let len7: u8 = b1 & 0x7f;

    if (!isValidOpcode(opcode)) {
        return WS_INVALID;
    }
    // Control frames MUST NOT be fragmented and MUST have payload <= 125 bytes
    if (isControlOpcode(opcode)) {
        if (!fin || len7 > 125) {
            return WS_INVALID;
        }
    }

    let pos: usize = 2;
    let payload_len: u64 = 0;
    if (len7 < 126) {
        payload_len = len7 as u64;
    } else if (len7 == 126) {
        if (data.len < pos + 2) {
            return WS_INCOMPLETE;
        }
        payload_len = (((data[pos] as u64) << 8) | (data[pos + 1] as u64));
        pos += 2;
    } else {
        if (data.len < pos + 8) {
            return WS_INCOMPLETE;
        }
        // RFC 6455: MSB of 64-bit length MUST be 0
        if ((data[pos] & 0x80) != 0) {
            return WS_INVALID;
        }
        payload_len =
            ((data[pos] as u64) << 56) |
            ((data[pos + 1] as u64) << 48) |
            ((data[pos + 2] as u64) << 40) |
            ((data[pos + 3] as u64) << 32) |
            ((data[pos + 4] as u64) << 24) |
            ((data[pos + 5] as u64) << 16) |
            ((data[pos + 6] as u64) << 8) |
            (data[pos + 7] as u64);
        pos += 8;
    }

    let mask_key: [4]u8 = undefined;
    mask_key[0] = 0;
    mask_key[1] = 0;
    mask_key[2] = 0;
    mask_key[3] = 0;
    if (masked) {
        if (data.len < pos + 4) {
            return WS_INCOMPLETE;
        }
        mask_key[0] = data[pos];
        mask_key[1] = data[pos + 1];
        mask_key[2] = data[pos + 2];
        mask_key[3] = data[pos + 3];
        pos += 4;
    }

    out.* = FrameHeader {
        fin: fin,
        rsv: rsv,
        opcode: opcode,
        masked: masked,
        mask_key: mask_key,
        payload_len: payload_len,
        header_len: pos,
    };
    return WS_OK;
}

/// Encode an RFC 6455 WebSocket frame header into `out`.
/// Returns header bytes written (2..14), or 0 on invalid opcode / insufficient buffer.
pub fn encodeHeader(
    fin: bool,
    opcode: u8,
    masked: bool,
    mask_key: [4]u8,
    payload_len: u64,
    out: []u8,
): usize {
    if (!isValidOpcode(opcode)) { return 0; }
    if (isControlOpcode(opcode) && (!fin || payload_len > 125)) { return 0; }

    let ext_len: usize = 0;
    if (payload_len >= 126 && payload_len <= 65535) {
        ext_len = 2;
    } else if (payload_len > 65535) {
        ext_len = 8;
    }
    let mask_len: usize = 0;
    if (masked) { mask_len = 4; }
    let total: usize = (2 as usize) + ext_len + mask_len;
    if (out.len < total) {
        return 0;
    }

    let b0: u8 = opcode & 0x0f;
    if (fin) { b0 = b0 | 0x80; }
    out[0] = b0;

    let mask_bit: u8 = 0;
    if (masked) { mask_bit = 0x80; }

    let pos: usize = 2;
    if (payload_len < 126) {
        out[1] = mask_bit | (payload_len as u8);
    } else if (payload_len <= 65535) {
        out[1] = mask_bit | 126;
        out[2] = ((payload_len >> 8) & 0xff) as u8;
        out[3] = (payload_len & 0xff) as u8;
        pos = 4;
    } else {
        out[1] = mask_bit | 127;
        out[2] = ((payload_len >> 56) & 0xff) as u8;
        out[3] = ((payload_len >> 48) & 0xff) as u8;
        out[4] = ((payload_len >> 40) & 0xff) as u8;
        out[5] = ((payload_len >> 32) & 0xff) as u8;
        out[6] = ((payload_len >> 24) & 0xff) as u8;
        out[7] = ((payload_len >> 16) & 0xff) as u8;
        out[8] = ((payload_len >> 8) & 0xff) as u8;
        out[9] = (payload_len & 0xff) as u8;
        pos = 10;
    }

    if (masked) {
        out[pos] = mask_key[0];
        out[pos + 1] = mask_key[1];
        out[pos + 2] = mask_key[2];
        out[pos + 3] = mask_key[3];
        pos += 4;
    }
    return pos;
}

/// Encode a complete RFC 6455 WebSocket frame (header + optional masked payload) into `out`.
/// Returns total bytes written, or 0 on error / insufficient buffer.
pub fn encodeFrame(
    fin: bool,
    opcode: u8,
    masked: bool,
    mask_key: [4]u8,
    payload: []const u8,
    out: []u8,
): usize {
    let hlen: usize = encodeHeader(fin, opcode, masked, mask_key, payload.len as u64, out);
    if (hlen == 0 || hlen + payload.len > out.len) {
        return 0;
    }
    if (!masked) {
        if (payload.len > 0) {
            @memcpy(out[hlen .. hlen + payload.len], payload);
        }
        return hlen + payload.len;
    }
    let i: usize = 0;
    if (payload.len >= 16) {
        const mk16: [16]u8 = [
            mask_key[0], mask_key[1], mask_key[2], mask_key[3],
            mask_key[0], mask_key[1], mask_key[2], mask_key[3],
            mask_key[0], mask_key[1], mask_key[2], mask_key[3],
            mask_key[0], mask_key[1], mask_key[2], mask_key[3],
        ];
        const vmask: WsVec16 = mk16;
        while (i + 16 <= payload.len) {
            const vp: WsVec16 = payload[i .. i + 16][0..16].*;
            const out_arr: [16]u8 = vp ^ vmask;
            @memcpy(out[hlen + i .. hlen + i + 16], out_arr[0..16]);
            i += 16;
        }
    }
    while (i < payload.len) {
        out[hlen + i] = payload[i] ^ mask_key[i & 3];
        i += 1;
    }
    return hlen + payload.len;
}

/// Encode a Close frame payload (`2-byte big-endian status code + optional UTF-8 reason`) into `out`.
pub fn encodeClosePayload(code: u16, reason: string, out: []u8): usize {
    let total: usize = (2 as usize) + reason.len;
    if (total > 125 || total > out.len) {
        return 0;
    }
    out[0] = ((code >> 8) & 0xff) as u8;
    out[1] = (code & 0xff) as u8;
    if (reason.len > 0) {
        @memcpy(out[2 .. (2 as usize) + reason.len], reason);
    }
    return total;
}

/// Validate RFC 6455 Close status code (1000..4999 excluding reserved 1004, 1005, 1006, 1015).
pub fn isValidCloseCode(code: u16): bool {
    if (code < 1000 || code >= 5000) { return false; }
    if (code >= 1004 && code <= 1006) { return false; }
    if (code >= 1015 && code <= 2999) { return false; }
    return true;
}
