// Reference-hardened against snapview/tungstenite-rs (MIT/Apache-2.0)
// Source: cp/lib/websocket_frame/assembler.cp (RFC 6455 Incremental Stream Frame & Fragment Assembler)

const std = @import("std");
import { WsOpcode, WsError } from "./wsTypes.cp";
import { DecodedFrame, decodeFrame } from "./wsCodec.cp";

/// Assembled WebSocket message (either a complete data message or an interleaved control frame).
/// Caller owns `payload` and must call `deinit(alloc)`.
pub struct WsMessage {
    opcode: WsOpcode = .Text,
    payload: []u8 = [],
    close_code: u16? = null,

    /// Frees the message's payload buffer if non-empty.
    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.payload.len > 0) {
            alloc.free(_.payload);
        }
        _.payload = [];
    }
}

/// Incremental RFC 6455 WebSocket stream decoder and fragment reassembler.
/// Handles arbitrary 1..N byte partial/coalesced chunks, multi-frame Text/Binary fragmentation,
/// and legal RFC 6455 §5.4 interleaved control frames (Ping/Pong/Close) mid-fragment.
pub struct WsStreamAssembler {
    alloc: std.mem.Allocator,
    require_masked: bool,
    max_payload: usize,
    recv_buf: std.ArrayList(u8) = .empty,
    frag_buf: std.ArrayList(u8) = .empty,
    in_fragment: bool = false,
    frag_opcode: WsOpcode = .Text,
    messages: std.ArrayList(WsMessage) = .empty,

    /// Initializes a new incremental stream assembler.
    pub static fn init(alloc: std.mem.Allocator, require_masked: bool, max_payload: usize): WsStreamAssembler {
        return WsStreamAssembler{
            alloc: alloc,
            require_masked: require_masked,
            max_payload: max_payload,
        };
    }

    /// Releases all internal buffers and any unconsumed queued messages.
    pub fn deinit(*_) {
        _.recv_buf.deinit(_.alloc);
        _.frag_buf.deinit(_.alloc);
        for (msg of _.messages.items) {
            let m = msg;
            m.deinit(_.alloc);
        }
        _.messages.deinit(_.alloc);
    }

    /// Resets fragment and receive state while preserving allocated capacity.
    pub fn reset(*_) {
        _.recv_buf.clearRetainingCapacity();
        _.frag_buf.clearRetainingCapacity();
        _.in_fragment = false;
        for (msg of _.messages.items) {
            let m = msg;
            m.deinit(_.alloc);
        }
        _.messages.clearRetainingCapacity();
    }

    /// Returns the number of assembled messages waiting to be popped.
    pub fn pendingCount(): usize {
        return _.messages.items.len;
    }

    /// Pops and returns the next assembled `WsMessage`, or `null` if none is ready.
    /// Caller must call `msg.deinit(alloc)`.
    pub fn nextMessage(*_): WsMessage? {
        if (_.messages.items.len == 0) {
            return null;
        }
        return _.messages.orderedRemove(0);
    }

    fn consumeRecv(*_, n: usize) {
        const total = _.recv_buf.items.len;
        if (n >= total) {
            _.recv_buf.clearRetainingCapacity();
            return;
        }
        const rem = total - n;
        std.mem.copyForwards(u8, _.recv_buf.items[0..rem], _.recv_buf.items[n..total]);
        _.recv_buf.shrinkRetainingCapacity(rem);
    }

    /// Feeds an arbitrary-length incoming byte chunk into the assembler and decodes all complete frames.
    /// Guards receive buffer growth against unbounded OOM when fed oversized incomplete streams.
    pub fn feed(*_, chunk: []const u8): void ! WsError {
        if (chunk.len > 0) {
            const max_recv_cap = if (_.max_payload <= (0xFFFFFFFF - 65536)) (_.max_payload + 65536) else _.max_payload;
            if (_.recv_buf.items.len + chunk.len > max_recv_cap && _.recv_buf.items.len > _.max_payload + 14) {
                return WsError.PayloadTooLarge;
            }
            _.recv_buf.appendSlice(_.alloc, chunk) catch return WsError.OutOfMemory;
        }

        while (_.recv_buf.items.len > 0) {
            const decoded = decodeFrame(_.alloc, _.recv_buf.items, _.require_masked, _.max_payload) catch e => {
                if (e == WsError.Incomplete) {
                    return;
                }
                return e;
            };

            _.consumeRecv(decoded.bytes_consumed);
            let frame = decoded.frame;

            if (frame.opcode.isControl()) {
                let close_code: u16? = null;
                if (frame.opcode == .Close && frame.payload.len >= 2) {
                    close_code = std.mem.readInt(u16, frame.payload[0..2], .big);
                }
                _.messages.append(_.alloc, WsMessage{
                    opcode: frame.opcode,
                    payload: frame.payload,
                    close_code: close_code,
                }) catch {
                    frame.deinit(_.alloc);
                    return WsError.OutOfMemory;
                };
            } else if (frame.opcode == .Continuation) {
                if (!_.in_fragment) {
                    frame.deinit(_.alloc);
                    return WsError.UnexpectedContinuation;
                }
                if (_.frag_buf.items.len + frame.payload.len > _.max_payload) {
                    frame.deinit(_.alloc);
                    _.frag_buf.clearRetainingCapacity();
                    _.in_fragment = false;
                    return WsError.PayloadTooLarge;
                }
                if (frame.payload.len > 0) {
                    _.frag_buf.appendSlice(_.alloc, frame.payload) catch {
                        frame.deinit(_.alloc);
                        return WsError.OutOfMemory;
                    };
                }
                frame.deinit(_.alloc);

                if (frame.fin) {
                    if (_.frag_opcode == .Text && _.frag_buf.items.len > 0) {
                        if (!std.unicode.utf8ValidateSlice(_.frag_buf.items)) {
                            _.frag_buf.clearRetainingCapacity();
                            _.in_fragment = false;
                            return WsError.InvalidUtf8;
                        }
                    }
                    let assembled_payload: []u8 = [];
                    if (_.frag_buf.items.len > 0) {
                        assembled_payload = _.frag_buf.toOwnedSlice(_.alloc) catch return WsError.OutOfMemory;
                    } else {
                        _.frag_buf.clearRetainingCapacity();
                    }
                    const msg_op = _.frag_opcode;
                    _.in_fragment = false;

                    _.messages.append(_.alloc, WsMessage{
                        opcode: msg_op,
                        payload: assembled_payload,
                        close_code: null,
                    }) catch {
                        if (assembled_payload.len > 0) {
                            _.alloc.free(assembled_payload);
                        }
                        return WsError.OutOfMemory;
                    };
                }
            } else {
                // Data frame: Text or Binary
                if (_.in_fragment) {
                    frame.deinit(_.alloc);
                    return WsError.FragmentedInProgress;
                }
                if (frame.fin) {
                    _.messages.append(_.alloc, WsMessage{
                        opcode: frame.opcode,
                        payload: frame.payload,
                        close_code: null,
                    }) catch {
                        frame.deinit(_.alloc);
                        return WsError.OutOfMemory;
                    };
                } else {
                    _.in_fragment = true;
                    _.frag_opcode = frame.opcode;
                    _.frag_buf.clearRetainingCapacity();
                    if (frame.payload.len > 0) {
                        _.frag_buf.appendSlice(_.alloc, frame.payload) catch {
                            frame.deinit(_.alloc);
                            return WsError.OutOfMemory;
                        };
                    }
                    frame.deinit(_.alloc);
                }
            }
        }
    }
}
