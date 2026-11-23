// Reference-hardened against snapview/tungstenite-rs (MIT/Apache-2.0)
// Source: cp/lib/websocket_frame/codec.cp (RFC 6455 WebSocket Frame Encoder, Decoder & 16-Lane SIMD Masking)

const std = @import("std");
import { WsOpcode, WsCloseCode, WsError } from "./wsTypes.cp";

type WsVec16u8 = @Vector(16, u8);

fn wsLoadVec16(data: []const u8, off: usize): WsVec16u8 {
    const v: WsVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// Decoded WebSocket frame with owned heap-allocated payload.
pub struct WsFrame {
    fin: bool = true,
    rsv1: bool = false,
    rsv2: bool = false,
    rsv3: bool = false,
    opcode: WsOpcode = .Text,
    masked: bool = false,
    mask_key: [4]u8 = [0; 4],
    payload: []u8 = [],

    /// Frees the frame's payload buffer if non-empty.
    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.payload.len > 0) {
            alloc.free(_.payload);
        }
        _.payload = [];
    }
}

/// Result of `decodeFrame`: the decoded frame and total bytes consumed from the input buffer.
pub struct DecodedFrame {
    frame: WsFrame,
    bytes_consumed: usize,
}

/// 16-lane `@Vector(16, u8)` SIMD RFC 6455 cyclic 4-byte XOR masking in-place over `buf`
/// starting at key phase `offset & 3`.
pub fn applyMaskOffsetSimd(buf: []u8, mask_key: [4]u8, offset: usize) {
    const len = buf.len;
    if (len == 0) return;

    const k0 = mask_key[offset & 3];
    const k1 = mask_key[(offset + 1) & 3];
    const k2 = mask_key[(offset + 2) & 3];
    const k3 = mask_key[(offset + 3) & 3];

    const mask_v: WsVec16u8 = [
        k0, k1, k2, k3,
        k0, k1, k2, k3,
        k0, k1, k2, k3,
        k0, k1, k2, k3,
    ];

    let i: usize = 0;
    while (i + 16 <= len) {
        const chunk = wsLoadVec16(buf, i);
        const xored: [16]u8 = chunk ^ mask_v;
        @memcpy(buf[i .. i + 16], xored[0..16]);
        i += 16;
    }
    while (i + 4 <= len) {
        buf[i] ^= k0;
        buf[i + 1] ^= k1;
        buf[i + 2] ^= k2;
        buf[i + 3] ^= k3;
        i += 4;
    }
    while (i < len) : (i += 1) {
        buf[i] ^= mask_key[(offset + i) & 3];
    }
}

/// Scalar reference for `applyMaskOffsetSimd` (differential verification).
pub fn applyMaskOffsetScalar(buf: []u8, mask_key: [4]u8, offset: usize) {
    let i: usize = 0;
    while (i < buf.len) : (i += 1) {
        buf[i] ^= mask_key[(offset + i) & 3];
    }
}

/// 16-lane `@Vector(16, u8)` SIMD RFC 6455 4-byte cyclic XOR masking in-place over `buf`.
pub fn applyMaskSimd(buf: []u8, mask_key: [4]u8) {
    applyMaskOffsetSimd(buf, mask_key, 0);
}

/// Scalar reference for `applyMaskSimd` (differential verification).
pub fn applyMaskScalar(buf: []u8, mask_key: [4]u8) {
    applyMaskOffsetScalar(buf, mask_key, 0);
}

/// Applies RFC 6455 4-byte cyclic XOR masking in-place over `buf` (16-lane SIMD accelerated).
pub fn applyMask(buf: []u8, mask_key: [4]u8) {
    applyMaskSimd(buf, mask_key);
}

/// Encodes a single RFC 6455 WebSocket frame with 7-bit, 16-bit (`126`), or 64-bit (`127`)
/// payload length and optional 4-byte XOR masking. Caller owns the returned slice.
pub fn encodeFrame(
    alloc: std.mem.Allocator,
    opcode: WsOpcode,
    payload: []const u8,
    fin: bool,
    mask_key_opt: ([4]u8)?,
): ([]u8) ! WsError {
    if (opcode.isControl()) {
        if (!fin) {
            return WsError.ControlFrameFragmented;
        }
        if (payload.len > 125) {
            return WsError.ControlFrameTooLarge;
        }
    }

    const plen = payload.len;
    let header_len: usize = 2;
    if (plen >= 126 && plen <= 0xFFFF) {
        header_len += 2;
    } else if (plen > 0xFFFF) {
        header_len += 8;
    }
    if (mask_key_opt != null) {
        header_len += 4;
    }

    const total_len = header_len + plen;
    let out = alloc.alloc(u8, total_len) catch return WsError.OutOfMemory;

    let b0: u8 = @intFromEnum(opcode) as u8;
    if (fin) {
        b0 |= 0x80;
    }
    out[0] = b0;

    let b1: u8 = 0;
    if (mask_key_opt != null) {
        b1 |= 0x80;
    }

    let offset: usize = 2;
    if (plen <= 125) {
        b1 |= plen as u8;
        out[1] = b1;
    } else if (plen <= 0xFFFF) {
        b1 |= 126;
        out[1] = b1;
        std.mem.writeInt(u16, out[2..4], plen as u16, .big);
        offset = 4;
    } else {
        b1 |= 127;
        out[1] = b1;
        std.mem.writeInt(u64, out[2..10], plen as u64, .big);
        offset = 10;
    }

    if (mkey := mask_key_opt) {
        @memcpy(out[offset..(offset + 4)], &mkey);
        offset += 4;
        if (plen > 0) {
            @memcpy(out[offset..(offset + plen)], payload);
            applyMaskSimd(out[offset..(offset + plen)], mkey);
        }
    } else {
        if (plen > 0) {
            @memcpy(out[offset..(offset + plen)], payload);
        }
    }

    return out;
}

/// Encodes an RFC 6455 Close frame with a 2-byte big-endian status code and optional UTF-8 reason.
/// Caller owns the returned slice.
pub fn encodeCloseFrame(
    alloc: std.mem.Allocator,
    code: u16,
    reason: []const u8,
    mask_key_opt: ([4]u8)?,
): ([]u8) ! WsError {
    if (!WsCloseCode.isValidWireCode(code)) {
        return WsError.InvalidClosePayload;
    }
    if (reason.len > 123) {
        return WsError.ControlFrameTooLarge;
    }
    if (reason.len > 0 && !std.unicode.utf8ValidateSlice(reason)) {
        return WsError.InvalidUtf8;
    }

    let buf: [125]u8 = [0; 125];
    std.mem.writeInt(u16, buf[0..2], code, .big);
    if (reason.len > 0) {
        @memcpy(buf[2..(2 + reason.len)], reason);
    }
    return encodeFrame(alloc, .Close, buf[0..(2 + reason.len)], true, mask_key_opt);
}

/// Decodes a single RFC 6455 WebSocket frame from `buf`.
/// Enforces RSV=0, valid opcode, control frame limits (FIN=1, len<=125),
/// minimal length encoding, MSB=0 on 64-bit length, optional client masking requirement,
/// `max_payload` limit, Close payload validity, and UTF-8 validity for unfragmented Text frames.
pub fn decodeFrame(
    alloc: std.mem.Allocator,
    buf: []const u8,
    require_masked: bool,
    max_payload: usize,
): DecodedFrame ! WsError {
    if (buf.len < 2) {
        return WsError.Incomplete;
    }

    const b0 = buf[0];
    const b1 = buf[1];

    const fin = (b0 & 0x80) != 0;
    const rsv1 = (b0 & 0x40) != 0;
    const rsv2 = (b0 & 0x20) != 0;
    const rsv3 = (b0 & 0x10) != 0;

    if (rsv1 || rsv2 || rsv3) {
        return WsError.ReservedBitsSet;
    }

    const raw_opcode = (b0 & 0x0F) as u4;
    const opcode = WsOpcode.fromU4(raw_opcode) orelse return WsError.InvalidOpcode;

    const masked = (b1 & 0x80) != 0;
    const len7 = b1 & 0x7F;

    if (opcode.isControl()) {
        if (!fin) {
            return WsError.ControlFrameFragmented;
        }
        if (len7 > 125) {
            return WsError.ControlFrameTooLarge;
        }
    }

    if (require_masked && !masked) {
        return WsError.UnmaskedClientFrame;
    }

    let header_len: usize = 2;
    let payload_len: usize = 0;

    if (len7 == 126) {
        if (buf.len < 4) {
            return WsError.Incomplete;
        }
        const len16 = std.mem.readInt(u16, buf[2..4], .big);
        // RFC 6455 Section 5.2: minimal length representation required
        if (len16 < 126) {
            return WsError.NonMinimalLength;
        }
        payload_len = len16 as usize;
        header_len = 4;
    } else if (len7 == 127) {
        if (buf.len < 10) {
            return WsError.Incomplete;
        }
        const len64 = std.mem.readInt(u64, buf[2..10], .big);
        if ((len64 >> 63) != 0) {
            return WsError.PayloadTooLarge;
        }
        // RFC 6455 Section 5.2: minimal length representation required
        if (len64 <= 0xFFFF) {
            return WsError.NonMinimalLength;
        }
        if (len64 > (max_payload as u64)) {
            return WsError.PayloadTooLarge;
        }
        payload_len = len64 as usize;
        header_len = 10;
    } else {
        payload_len = len7 as usize;
    }

    if (payload_len > max_payload) {
        return WsError.PayloadTooLarge;
    }

    let mask_key: [4]u8 = [0; 4];
    if (masked) {
        if (buf.len < header_len + 4) {
            return WsError.Incomplete;
        }
        @memcpy(&mask_key, buf[header_len..(header_len + 4)]);
        header_len += 4;
    }

    if (buf.len - header_len < payload_len) {
        return WsError.Incomplete;
    }

    let payload_buf: []u8 = [];
    if (payload_len > 0) {
        payload_buf = alloc.alloc(u8, payload_len) catch return WsError.OutOfMemory;
        @memcpy(payload_buf, buf[header_len..(header_len + payload_len)]);
        if (masked) {
            applyMaskSimd(payload_buf, mask_key);
        }
    }

    if (opcode == .Close) {
        if (payload_len == 1) {
            if (payload_buf.len > 0) {
                alloc.free(payload_buf);
            }
            return WsError.InvalidClosePayload;
        }
        if (payload_len >= 2) {
            const code = std.mem.readInt(u16, payload_buf[0..2], .big);
            if (!WsCloseCode.isValidWireCode(code)) {
                alloc.free(payload_buf);
                return WsError.InvalidClosePayload;
            }
            if (payload_len > 2 && !std.unicode.utf8ValidateSlice(payload_buf[2..payload_len])) {
                alloc.free(payload_buf);
                return WsError.InvalidUtf8;
            }
        }
    } else if (opcode == .Text && fin) {
        if (payload_len > 0 && !std.unicode.utf8ValidateSlice(payload_buf)) {
            alloc.free(payload_buf);
            return WsError.InvalidUtf8;
        }
    }

    return DecodedFrame{
        frame: WsFrame{
            fin: fin,
            rsv1: false,
            rsv2: false,
            rsv3: false,
            opcode: opcode,
            masked: masked,
            mask_key: mask_key,
            payload: payload_buf,
        },
        bytes_consumed: header_len + payload_len,
    };
}
