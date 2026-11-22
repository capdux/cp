// Source: Absorbed from /data/dev/bun/src/http_jsc/websocket_client/ | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// Absorbed from Bun: bun/src/ws/frame.cp
// Upstream Sync: Refer to bun/src/ws/frame.zig for future upstream updates.
// ============================================================================
// lib/ws/frame.cp
// RFC 6455 WebSocket Frame Header, Close Payload & Builder in CP

const std = @import("std");
import { Opcode, CloseCode } from "./opcode";
import { Mask } from "./mask";
import { Utf8 } from "cap:string";

pub struct ClosePayload {
    code: u16 = 1000,
    reason: []const u8 = "",

    /// Parses and validates an RFC 6455 Close frame payload (optional 2-byte big-endian code + UTF-8 reason).
    pub static fn parse(payload: []const u8): ClosePayload! {
        if (payload.len > 125) {
            return err.ControlFrameTooLarge;
        }
        if (payload.len == 0) {
            return ClosePayload{ code: 1005, reason: "" };
        }
        if (payload.len == 1) {
            return err.InvalidClosePayload;
        }
        const code = std.mem.readInt(u16, payload[0..2], .big);
        if (!CloseCode.isValidWireCode(code)) {
            return err.InvalidClosePayload;
        }
        const reason = payload[2..];
        if (reason.len > 0 && !Utf8.isValid(reason)) {
            return err.InvalidUtf8;
        }
        return ClosePayload{ code: code, reason: reason };
    }

    /// Encodes a 2-byte big-endian status code and optional UTF-8 reason into `out`.
    pub static fn encode(out: []u8, code: u16, reason: []const u8): usize! {
        if (!CloseCode.isValidWireCode(code)) {
            return err.InvalidClosePayload;
        }
        if (reason.len > 123) {
            return err.ControlFrameTooLarge;
        }
        if (reason.len > 0 && !Utf8.isValid(reason)) {
            return err.InvalidUtf8;
        }
        const total = 2 + reason.len;
        if (out.len < total) {
            return err.BufferTooSmall;
        }
        std.mem.writeInt(u16, out[0..2], code, .big);
        if (reason.len > 0) {
            @memcpy(out[2..total], reason);
        }
        return total;
    }
}

pub struct FrameHeader {
    fin: bool = true,
    rsv1: bool = false,
    rsv2: bool = false,
    rsv3: bool = false,
    opcode: Opcode = .Text,
    masked: bool = false,
    payload_len: usize = 0,
    mask_key: [4]u8 = [0, 0, 0, 0],
    header_len: usize = 2,

    pub fn encodedSize(): usize {
        let needed_len: usize = 2;
        if (_.payload_len >= 126 && _.payload_len <= 0xFFFF) {
            needed_len += 2;
        } else if (_.payload_len > 0xFFFF) {
            needed_len += 8;
        }
        if (_.masked) {
            needed_len += 4;
        }
        return needed_len;
    }

    pub fn encode(out: []u8): usize! {
        if (_.opcode.isControl() && (!_.fin || _.payload_len > 125)) {
            return err.InvalidControlFrame;
        }

        const needed_len = _.encodedSize();
        if (out.len < needed_len) return err.BufferTooSmall;

        // Byte 0: FIN + RSV1-3 + Opcode
        let b0: u8 = @intFromEnum(_.opcode);
        if (_.fin) b0 |= 0x80;
        if (_.rsv1) b0 |= 0x40;
        if (_.rsv2) b0 |= 0x20;
        if (_.rsv3) b0 |= 0x10;
        out[0] = b0;

        // Byte 1: Mask bit + 7-bit length
        let b1: u8 = 0;
        if (_.masked) b1 |= 0x80;

        let offset: usize = 2;
        if (_.payload_len <= 125) {
            b1 |= @as(u8, @truncate(_.payload_len));
            out[1] = b1;
        } else if (_.payload_len <= 0xFFFF) {
            b1 |= 126;
            out[1] = b1;
            std.mem.writeInt(u16, out[offset .. offset + 2][0..2], @as(u16, @truncate(_.payload_len)), .big);
            offset += 2;
        } else {
            b1 |= 127;
            out[1] = b1;
            std.mem.writeInt(u64, out[offset .. offset + 8][0..8], @as(u64, _.payload_len), .big);
            offset += 8;
        }

        // Masking Key
        if (_.masked) {
            @memcpy(out[offset .. offset + 4], &_.mask_key);
            offset += 4;
        }

        return offset;
    }

    pub static fn decode(buf: []const u8): FrameHeader! {
        if (buf.len < 2) return err.IncompleteHeader;

        const b0 = buf[0];
        const b1 = buf[1];

        const fin = (b0 & 0x80) != 0;
        const rsv1 = (b0 & 0x40) != 0;
        const rsv2 = (b0 & 0x20) != 0;
        const rsv3 = (b0 & 0x10) != 0;
        const opcode_val: u4 = @truncate(b0 & 0x0F);
        const opcode = Opcode.fromU4(opcode_val) ?? return err.InvalidOpcode;

        const masked = (b1 & 0x80) != 0;
        const len7 = b1 & 0x7F;

        let offset: usize = 2;
        let payload_len: usize = 0;

        if (len7 <= 125) {
            payload_len = len7;
        } else if (len7 == 126) {
            if (buf.len < offset + 2) return err.IncompleteHeader;
            payload_len = std.mem.readInt(u16, buf[offset .. offset + 2][0..2], .big);
            offset += 2;
        } else {
            if (buf.len < offset + 8) return err.IncompleteHeader;
            const raw_len = std.mem.readInt(u64, buf[offset .. offset + 8][0..8], .big);
            // RFC 6455 Section 5.2: most significant bit MUST be 0
            if ((raw_len & (@as(u64, 1) << 63)) != 0) {
                return err.InvalidPayloadLength;
            }
            payload_len = @as(usize, @truncate(raw_len));
            offset += 8;
        }

        if (opcode.isControl()) {
            if (!fin) return err.FragmentedControlFrame;
            if (payload_len > 125) return err.ControlFrameTooLarge;
        }

        let mask_key: [4]u8 = [0, 0, 0, 0];
        if (masked) {
            if (buf.len < offset + 4) return err.IncompleteHeader;
            @memcpy(&mask_key, buf[offset .. offset + 4]);
            offset += 4;
        }

        return FrameHeader{
            .fin = fin,
            .rsv1 = rsv1,
            .rsv2 = rsv2,
            .rsv3 = rsv3,
            .opcode = opcode,
            .masked = masked,
            .payload_len = payload_len,
            .mask_key = mask_key,
            .header_len = offset,
        };
    }
}

pub struct FrameBuilder {
    pub static fn buildFragment(out: []u8, fin: bool, opcode: Opcode, payload: []const u8, mask: ([4]u8)?): usize! {
        const header = FrameHeader{
            .fin = fin,
            .opcode = opcode,
            .masked = mask != null,
            .payload_len = payload.len,
            .mask_key = mask ?? [0, 0, 0, 0],
        };

        const header_len = header.encode(out)?;
        if (out.len < header_len + payload.len) return err.BufferTooSmall;

        if (payload.len > 0) {
            @memcpy(out[header_len .. header_len + payload.len], payload);
            if (k := mask) {
                Mask.apply(out[header_len .. header_len + payload.len], k);
            }
        }

        return header_len + payload.len;
    }

    pub static fn build(out: []u8, opcode: Opcode, payload: []const u8, mask: ([4]u8)?): usize! {
        return FrameBuilder.buildFragment(out, true, opcode, payload, mask);
    }

    pub static fn buildText(out: []u8, text: []const u8, mask: ([4]u8)?): usize! {
        return FrameBuilder.build(out, .Text, text, mask);
    }

    pub static fn buildBinary(out: []u8, data: []const u8, mask: ([4]u8)?): usize! {
        return FrameBuilder.build(out, .Binary, data, mask);
    }

    pub static fn buildContinue(out: []u8, fin: bool, data: []const u8, mask: ([4]u8)?): usize! {
        return FrameBuilder.buildFragment(out, fin, .Continue, data, mask);
    }

    pub static fn buildPing(out: []u8, payload: []const u8, mask: ([4]u8)?): usize! {
        return FrameBuilder.build(out, .Ping, payload, mask);
    }

    pub static fn buildPong(out: []u8, payload: []const u8, mask: ([4]u8)?): usize! {
        return FrameBuilder.build(out, .Pong, payload, mask);
    }

    /// Builds a Pong reply frame echoing a received Ping payload (RFC 6455 Section 5.5.3).
    pub static fn buildPongReply(out: []u8, ping_payload: []const u8, mask: ([4]u8)?): usize! {
        if (ping_payload.len > 125) return err.ControlFrameTooLarge;
        return FrameBuilder.build(out, .Pong, ping_payload, mask);
    }

    pub static fn buildClose(out: []u8, code: CloseCode, reason: []const u8, mask: ([4]u8)?): usize! {
        return FrameBuilder.buildCloseCode(out, @intFromEnum(code), reason, mask);
    }

    /// Builds a Close frame using a numeric RFC 6455 status code (`1000..4999`) and UTF-8 reason.
    pub static fn buildCloseCode(out: []u8, code: u16, reason: []const u8, mask: ([4]u8)?): usize! {
        let payload_buf: [125]u8 = undefined;
        const total_len = ClosePayload.encode(&payload_buf, code, reason)?;
        return FrameBuilder.build(out, .Close, payload_buf[0..total_len], mask);
    }
}
