// Source: cp/lib/ws/session.cp (RFC 6455 WebSocket Session State Machine)
// lib/ws/session.cp
// RFC 6455 Full WebSocket Session State Machine (Framing, Interleaving, Auto-Pong, Close Handshake & Bounds)

const std = @import("std");
import { Utf8 } from "cap:string";
import { Opcode } from "./opcode";
import { Mask } from "./mask";
import { FrameHeader, FrameBuilder } from "./frame";
import { WebSocketMessageKind, WebSocketMessage, WebSocketMessageAssembler } from "./assembler";
import { CloseCodeValidator } from "./subprotocol";
import { PerMessageDeflate } from "./permessage_deflate";

pub enum WsSessionState: u8 {
    Connecting = 0,
    Open = 1,
    Closing = 2,
    Closed = 3,
}

pub struct WsSessionConfig {
    is_server: bool = true,
    max_message_bytes: usize = 16 * 1024 * 1024,
    max_frame_bytes: usize = 1024 * 1024,
    auto_pong: bool = true,
    heartbeat_interval_ms: u64 = 30000,
    permessage_deflate: bool = false,
    require_mask_from_client: bool = true,
    forbid_mask_from_server: bool = true,
}

pub struct WsSessionResult {
    bytes_consumed: usize = 0,
    message: WebSocketMessage? = null,
    state: WsSessionState = .Open,
    needs_pong: bool = false,
    needs_close_reply: bool = false,
    compressed: bool = false,
}

pub struct WsSession {
    allocator: std.mem.Allocator = undefined,
    config: WsSessionConfig = WsSessionConfig{},
    state: WsSessionState = .Connecting,
    assembler: WebSocketMessageAssembler = undefined,
    sent_close: bool = false,
    received_close: bool = false,
    has_pending_close_reply: bool = false,
    close_code: u16 = 0,
    close_reason_buf: [125]u8 = undefined,
    close_reason_len: usize = 0,
    compressed_in_progress: bool = false,
    compressed_kind: WebSocketMessageKind = .Binary,
    last_decompressed: ([]u8)? = null,
    last_activity_ms: u64 = 0,
    last_ping_sent_ms: u64 = 0,
    awaiting_pong: bool = false,

    pub static fn init(allocator: std.mem.Allocator, config: WsSessionConfig): WsSession {
        return WsSession{
            allocator: allocator,
            config: config,
            state: .Connecting,
            assembler: WebSocketMessageAssembler.init(allocator, config.max_message_bytes),
            sent_close: false,
            received_close: false,
            has_pending_close_reply: false,
            close_code: 0,
            close_reason_buf: undefined,
            close_reason_len: 0,
            compressed_in_progress: false,
            compressed_kind: .Binary,
            last_decompressed: null,
            last_activity_ms: 0,
            last_ping_sent_ms: 0,
            awaiting_pong: false,
        };
    }

    pub static fn initOpen(allocator: std.mem.Allocator, config: WsSessionConfig): WsSession {
        let s = WsSession.init(allocator, config);
        s.state = .Open;
        return s;
    }

    pub fn deinit(*_) {
        _.assembler.deinit();
    }

    pub fn markOpen(*_): ! {
        if (_.state != .Connecting) {
            return err.InvalidSessionState;
        }
        _.state = .Open;
    }

    pub fn getState(): WsSessionState {
        return _.state;
    }

    pub fn isOpen(): bool {
        return _.state == .Open;
    }

    pub fn isClosed(): bool {
        return _.state == .Closed;
    }

    pub fn hasPendingPong(): bool {
        return _.config.auto_pong && _.assembler.hasPendingPong();
    }

    pub fn hasPendingCloseReply(): bool {
        return _.has_pending_close_reply;
    }

    pub fn getCloseCode(): u16 {
        return _.close_code;
    }

    pub fn getCloseReason(*_): []const u8 {
        return _.close_reason_buf[0.._.close_reason_len];
    }

    /// Generates an automatic Pong reply frame if a Ping was received and `auto_pong` is enabled.
    pub fn takePendingPongFrame(*_, out: []u8, mask: ([4]u8)?): usize! {
        if (!_.config.auto_pong) return 0;
        return _.assembler.takePongFrame(out, mask);
    }

    /// Generates the queued Close reply frame after receiving a peer Close frame, completing the Close handshake.
    pub fn takePendingCloseFrame(*_, out: []u8, mask: ([4]u8)?): usize! {
        if (!_.has_pending_close_reply) return 0;
        const reply_code: u16 = if (CloseCodeValidator.isValidWireCloseCode(_.close_code)) _.close_code else 1000;
        const n = FrameBuilder.buildCloseCode(out, reply_code, "", mask)?;
        _.has_pending_close_reply = false;
        _.sent_close = true;
        if (_.received_close) {
            _.state = .Closed;
        }
        return n;
    }

    /// Initiates an active WebSocket Close handshake from this endpoint.
    pub fn initiateClose(*_, out: []u8, code: u16, reason: []const u8, mask: ([4]u8)?): usize! {
        if (_.state == .Connecting || _.state == .Closed) {
            return err.InvalidSessionState;
        }
        if (!CloseCodeValidator.isValidWireCloseCode(code)) {
            return err.InvalidClosePayload;
        }
        if (reason.len > 123) {
            return err.ControlFrameTooLarge;
        }
        if (reason.len > 0 && !CloseCodeValidator.isValidUtf8(reason)) {
            return err.InvalidUtf8;
        }

        const n = FrameBuilder.buildCloseCode(out, code, reason, mask)?;
        _.sent_close = true;
        _.has_pending_close_reply = false;
        if (_.received_close) {
            _.state = .Closed;
        } else {
            _.state = .Closing;
        }
        return n;
    }

    /// Marks an externally transmitted Close frame as sent.
    pub fn markCloseSent(*_) {
        _.sent_close = true;
        _.has_pending_close_reply = false;
        if (_.received_close) {
            _.state = .Closed;
        } else if (_.state == .Open) {
            _.state = .Closing;
        }
    }

    pub fn recordActivity(*_, now_ms: u64) {
        _.last_activity_ms = now_ms;
    }

    pub fn markHeartbeatSent(*_, now_ms: u64) {
        _.last_ping_sent_ms = now_ms;
        _.awaiting_pong = true;
    }

    pub fn shouldSendHeartbeat(now_ms: u64): bool {
        if (_.state != .Open || _.config.heartbeat_interval_ms == 0) {
            return false;
        }
        if (now_ms <= _.last_activity_ms) return false;
        return (now_ms - _.last_activity_ms) >= _.config.heartbeat_interval_ms;
    }

    pub fn buildTextFrame(*_, out: []u8, text: []const u8, mask: ([4]u8)?): usize! {
        if (_.state != .Open) return err.InvalidSessionState;
        if (!_.config.is_server && _.config.require_mask_from_client && mask == null) {
            return err.ClientFrameRequiresMask;
        }
        return FrameBuilder.buildText(out, text, mask);
    }

    pub fn buildBinaryFrame(*_, out: []u8, data: []const u8, mask: ([4]u8)?): usize! {
        if (_.state != .Open) return err.InvalidSessionState;
        if (!_.config.is_server && _.config.require_mask_from_client && mask == null) {
            return err.ClientFrameRequiresMask;
        }
        return FrameBuilder.buildBinary(out, data, mask);
    }

    pub fn buildPingFrame(*_, out: []u8, payload: []const u8, mask: ([4]u8)?): usize! {
        if (_.state != .Open) return err.InvalidSessionState;
        return FrameBuilder.buildPing(out, payload, mask);
    }

    fn validateHeader(*_, hdr: FrameHeader): ! {
        if (_.state == .Connecting) return err.InvalidSessionState;
        if (_.state == .Closed || _.received_close) return err.SessionClosed;

        // Masking enforcement per RFC 6455 §5.1
        if (_.config.is_server) {
            if (_.config.require_mask_from_client && !hdr.masked) {
                return err.UnmaskedClientFrame;
            }
        } else {
            if (_.config.forbid_mask_from_server && hdr.masked) {
                return err.MaskedServerFrame;
            }
        }

        // RSV bits validation per RFC 6455 §5.2 & RFC 7692 §6.1
        if (hdr.rsv2 || hdr.rsv3) {
            return err.InvalidReservedBits;
        }
        if (hdr.rsv1) {
            if (!_.config.permessage_deflate || hdr.opcode.isControl() || hdr.opcode == .Continue) {
                return err.InvalidReservedBits;
            }
        }

        // Frame & message bounds before payload buffering
        if (hdr.payload_len > _.config.max_frame_bytes) {
            return err.FrameTooLarge;
        }
        if (!hdr.opcode.isControl() && hdr.payload_len > _.config.max_message_bytes) {
            return err.MessageTooBig;
        }
    }

    fn handleCloseFrame(*_, hdr: FrameHeader, payload: []const u8): WebSocketMessage! {
        let unmasked_buf: [125]u8 = undefined;
        if (payload.len > 0) {
            @memcpy(unmasked_buf[0..payload.len], payload);
            if (hdr.masked) {
                Mask.apply(unmasked_buf[0..payload.len], hdr.mask_key);
            }
        }
        const raw_close = unmasked_buf[0..payload.len];
        const parsed = CloseCodeValidator.parseClosePayload(raw_close)?;

        _.received_close = true;
        _.close_code = parsed.code;
        _.close_reason_len = parsed.reason.len;
        if (parsed.reason.len > 0) {
            @memcpy(_.close_reason_buf[0..parsed.reason.len], parsed.reason);
        }

        if (_.sent_close) {
            _.has_pending_close_reply = false;
            _.state = .Closed;
        } else {
            _.has_pending_close_reply = true;
            _.state = .Closing;
        }

        return WebSocketMessage{
            kind: .Close,
            data: _.close_reason_buf[0.._.close_reason_len],
            close_code: parsed.code,
            close_reason: _.close_reason_buf[0.._.close_reason_len],
        };
    }

    /// Feeds one decoded frame header and its exact payload into the session state machine.
    pub fn feedFrame(*_, hdr: FrameHeader, payload: []const u8): (WebSocketMessage?)! {
        if (payload.len != hdr.payload_len) {
            return err.IncompleteFrame;
        }
        _.validateHeader(hdr)?;

        if (hdr.opcode == .Close) {
            return _.handleCloseFrame(hdr, payload)?;
        }

        if (hdr.opcode.isControl()) {
            const msg = _.assembler.feedFrame(hdr, payload)?;
            if (m := msg) {
                if (m.kind == .Pong) {
                    _.awaiting_pong = false;
                }
            }
            return msg;
        }

        // Data frame: if RFC 7692 RSV1 compressed message, defer UTF-8 validation until after inflate
        if (hdr.opcode != .Continue) {
            if (!_.assembler.in_fragment) {
                _.compressed_in_progress = hdr.rsv1;
                _.compressed_kind = if (hdr.opcode == .Text) .Text else .Binary;
            }
        }

        if (_.compressed_in_progress) {
            // Reassemble raw compressed bytes as Binary in assembler, then decompress & validate UTF-8
            const bin_hdr = FrameHeader{
                .fin = hdr.fin,
                .rsv1 = hdr.rsv1,
                .rsv2 = hdr.rsv2,
                .rsv3 = hdr.rsv3,
                .opcode = if (hdr.opcode == .Text) .Binary else hdr.opcode,
                .masked = hdr.masked,
                .payload_len = hdr.payload_len,
                .mask_key = hdr.mask_key,
                .header_len = hdr.header_len,
            };
            const orig_kind: WebSocketMessageKind = _.compressed_kind;
            const raw_msg_opt = _.assembler.feedFrame(bin_hdr, payload)?;
            if (raw_msg := raw_msg_opt) {
                _.compressed_in_progress = false;
                const decompressed = PerMessageDeflate.decompressPayload(raw_msg.data, _.config.max_message_bytes)?;
                if (orig_kind == .Text && !Utf8.isValid(decompressed)) {
                    return err.InvalidUtf8;
                }
                _.last_decompressed = decompressed;
                return WebSocketMessage{
                    kind: orig_kind,
                    data: decompressed,
                };
            }
            return null;
        }

        return _.assembler.feedFrame(hdr, payload);
    }

    /// Parses and processes the next complete wire frame from `wire`.
    /// Validates `max_frame_bytes` / `max_message_bytes` and masking rules right after header decode.
    pub fn feedBytes(*_, wire: []const u8): WsSessionResult! {
        if (_.state == .Connecting) return err.InvalidSessionState;
        if (_.state == .Closed || _.received_close) return err.SessionClosed;

        if (wire.len < 2) {
            return WsSessionResult{
                bytes_consumed: 0,
                message: null,
                state: _.state,
                needs_pong: _.hasPendingPong(),
                needs_close_reply: _.hasPendingCloseReply(),
                compressed: false,
            };
        }

        const hdr = FrameHeader.decode(wire) catch e => {
            if (e == @as(anyerror, error.IncompleteHeader)) {
                return WsSessionResult{
                    bytes_consumed: 0,
                    message: null,
                    state: _.state,
                    needs_pong: _.hasPendingPong(),
                    needs_close_reply: _.hasPendingCloseReply(),
                    compressed: false,
                };
            }
            return e;
        };

        _.validateHeader(hdr)?;

        const total_len = hdr.header_len + hdr.payload_len;
        if (wire.len < total_len) {
            return WsSessionResult{
                bytes_consumed: 0,
                message: null,
                state: _.state,
                needs_pong: _.hasPendingPong(),
                needs_close_reply: _.hasPendingCloseReply(),
                compressed: false,
            };
        }

        const was_compressed = (!hdr.opcode.isControl()) && (hdr.rsv1 || _.compressed_in_progress);
        const msg = _.feedFrame(hdr, wire[hdr.header_len..total_len])?;
        return WsSessionResult{
            bytes_consumed: total_len,
            message: msg,
            state: _.state,
            needs_pong: _.hasPendingPong(),
            needs_close_reply: _.hasPendingCloseReply(),
            compressed: was_compressed && msg != null,
        };
    }
}
