// Source: Absorbed from /data/dev/bun/src/http_jsc/websocket_client/ | Synced: 2026-11-20T05:42:23Z
// lib/ws/assembler.cp
// RFC 6455 Fragmented Message Reassembly, UTF-8 Validation, Close Parsing & Auto-Pong

const std = @import("std");
import { Opcode } from "./opcode";
import { Mask } from "./mask";
import { FrameHeader, FrameBuilder, ClosePayload } from "./frame";
import { Utf8 } from "cap:string";

pub enum WebSocketMessageKind: u8 {
    Text = 0,
    Binary = 1,
    Close = 2,
    Ping = 3,
    Pong = 4,
}

pub struct WebSocketMessage {
    kind: WebSocketMessageKind = .Text,
    data: []const u8 = "",
    close_code: u16 = 0,
    close_reason: []const u8 = "",
}

pub struct AssembleResult {
    bytes_consumed: usize = 0,
    message: WebSocketMessage? = null,
}

pub struct WebSocketMessageAssembler {
    allocator: std.mem.Allocator = undefined,
    max_message_size: usize = 16 * 1024 * 1024,
    frag_buf: std.ArrayList(u8) = .empty,
    msg_buf: std.ArrayList(u8) = .empty,
    last_ping_buf: [125]u8 = undefined,
    last_ping_len: usize = 0,
    has_pending_pong: bool = false,
    in_fragment: bool = false,
    fragment_kind: WebSocketMessageKind = .Text,

    pub static fn init(allocator: std.mem.Allocator, max_message_size: usize): WebSocketMessageAssembler {
        return WebSocketMessageAssembler{
            allocator: allocator,
            max_message_size: max_message_size,
            frag_buf: .empty,
            msg_buf: .empty,
            last_ping_buf: undefined,
            last_ping_len: 0,
            has_pending_pong: false,
            in_fragment: false,
            fragment_kind: .Text,
        };
    }

    pub fn deinit(*_) {
        _.frag_buf.deinit(_.allocator);
        _.msg_buf.deinit(_.allocator);
    }

    pub fn reset(*_) {
        _.frag_buf.clearRetainingCapacity();
        _.msg_buf.clearRetainingCapacity();
        _.last_ping_len = 0;
        _.has_pending_pong = false;
        _.in_fragment = false;
        _.fragment_kind = .Text;
    }

    pub fn hasPendingPong(): bool {
        return _.has_pending_pong;
    }

    /// Generates an automatic RFC 6455 Pong reply frame matching the most recently received Ping payload.
    pub fn takePongFrame(*_, out: []u8, mask: ([4]u8)?): usize! {
        if (!_.has_pending_pong) return 0;
        const n = FrameBuilder.buildPongReply(out, _.last_ping_buf[0.._.last_ping_len], mask)?;
        _.has_pending_pong = false;
        _.last_ping_len = 0;
        return n;
    }

    fn copyAndUnmask(*_, target: *std.ArrayList(u8), payload: []const u8, masked: bool, mask_key: [4]u8): ! {
        if (payload.len == 0) return;
        const start = target.items.len;
        target.appendSlice(_.allocator, payload)?;
        if (masked) {
            Mask.apply(target.items[start .. start + payload.len], mask_key);
        }
    }

    /// Feeds one decoded frame header and its exact `payload` slice into the reassembly state machine.
    /// Returns a completed `WebSocketMessage` if the frame completes a data message or is a control frame,
    /// or `null` if awaiting additional Continuation frames.
    pub fn feedFrame(*_, hdr: FrameHeader, payload: []const u8): (WebSocketMessage?)! {
        if (payload.len != hdr.payload_len) {
            return err.IncompleteFrame;
        }

        // Control frames may be interleaved between fragments of a data message (RFC 6455 Section 5.4)
        if (hdr.opcode.isControl()) {
            if (!hdr.fin) return err.FragmentedControlFrame;
            if (payload.len > 125) return err.ControlFrameTooLarge;

            _.msg_buf.clearRetainingCapacity();
            _.copyAndUnmask(&_.msg_buf, payload, hdr.masked, hdr.mask_key)?;
            const ctrl_data = _.msg_buf.items;

            if (hdr.opcode == .Ping) {
                if (ctrl_data.len > 0) {
                    @memcpy(_.last_ping_buf[0..ctrl_data.len], ctrl_data);
                }
                _.last_ping_len = ctrl_data.len;
                _.has_pending_pong = true;
                return WebSocketMessage{
                    kind: .Ping,
                    data: ctrl_data,
                };
            }

            if (hdr.opcode == .Pong) {
                return WebSocketMessage{
                    kind: .Pong,
                    data: ctrl_data,
                };
            }

            // Opcode.Close
            const parsed_close = ClosePayload.parse(ctrl_data)?;
            return WebSocketMessage{
                kind: .Close,
                data: ctrl_data,
                close_code: parsed_close.code,
                close_reason: parsed_close.reason,
            };
        }

        // Data frames: Continue / Text / Binary
        if (hdr.opcode == .Continue) {
            if (!_.in_fragment) {
                return err.UnexpectedContinue;
            }
            if (_.frag_buf.items.len + payload.len > _.max_message_size) {
                return err.MessageTooBig;
            }
            _.copyAndUnmask(&_.frag_buf, payload, hdr.masked, hdr.mask_key)?;
            if (!hdr.fin) {
                return null;
            }
            _.in_fragment = false;
            const full = _.frag_buf.items;
            if (_.fragment_kind == .Text && !Utf8.isValid(full)) {
                return err.InvalidUtf8;
            }
            return WebSocketMessage{
                kind: _.fragment_kind,
                data: full,
            };
        }

        // Start of a new Text or Binary message
        if (_.in_fragment) {
            return err.UnexpectedOpcode;
        }
        if (payload.len > _.max_message_size) {
            return err.MessageTooBig;
        }
        const msg_kind: WebSocketMessageKind = if (hdr.opcode == .Text) .Text else .Binary;

        if (hdr.fin) {
            _.msg_buf.clearRetainingCapacity();
            _.copyAndUnmask(&_.msg_buf, payload, hdr.masked, hdr.mask_key)?;
            const full = _.msg_buf.items;
            if (msg_kind == .Text && !Utf8.isValid(full)) {
                return err.InvalidUtf8;
            }
            return WebSocketMessage{
                kind: msg_kind,
                data: full,
            };
        }

        // First fragment of a multi-frame message
        _.frag_buf.clearRetainingCapacity();
        _.in_fragment = true;
        _.fragment_kind = msg_kind;
        _.copyAndUnmask(&_.frag_buf, payload, hdr.masked, hdr.mask_key)?;
        return null;
    }

    /// Parses and feeds the next complete wire frame from `wire`.
    /// Returns `bytes_consumed == 0` when `wire` does not yet contain a complete frame.
    pub fn feedBytes(*_, wire: []const u8): AssembleResult! {
        if (wire.len < 2) {
            return AssembleResult{ bytes_consumed: 0, message: null };
        }
        const hdr = FrameHeader.decode(wire) catch e => {
            if (e == error.IncompleteHeader) {
                return AssembleResult{ bytes_consumed: 0, message: null };
            }
            return e;
        };
        const total_len = hdr.header_len + hdr.payload_len;
        if (wire.len < total_len) {
            return AssembleResult{ bytes_consumed: 0, message: null };
        }
        const msg = _.feedFrame(hdr, wire[hdr.header_len..total_len])?;
        return AssembleResult{
            bytes_consumed: total_len,
            message: msg,
        };
    }
}
