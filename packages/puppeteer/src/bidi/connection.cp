// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/bidi/Connection.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/bidi/connection.cp — WebDriver BiDi Connection Engine
// JSON-RPC command encoding, 16-lane @Vector(16, u8) SIMD BiDi frame scanner,
// pending callback correlation, multi-subscriber event dispatch, and
// CdpFrameScanner / CdpNativeWsTransport integration from cap:cdp.
// ============================================================================

const std = @import("std");
import {
    CdpFrameScanner,
    CdpNativeWsTransport,
} from "cap:cdp";
import {
    BidiError,
    BidiMessageType,
    BidiCommand,
    BidiSuccessResponse,
    BidiErrorResponse,
    BidiEvent,
    BidiMessage,
} from "./protocol";

type BidiVec16 = @Vector(16, u8);

pub type BidiSendTransportFn = fn (ctx: usize, payload: []const u8): bool;
pub type BidiEventCallback = fn (ctx: usize, ev: BidiEvent): void;
pub type BidiPipeEmitterFn = fn (ctx: usize, ev: BidiEvent): void;

pub enum BidiConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Closed,
}

pub struct BidiPendingCall {
    id: i64 = 0,
    method: []const u8 = "",
    completed: bool = false,
    is_error: bool = false,
    result_json: []const u8 = "{}",
    error_code: []const u8 = "",
    error_message: []const u8 = "",
    stacktrace: []const u8 = "",
}

pub struct BidiEventListener {
    method_filter: []const u8 = "",
    callback: BidiEventCallback = undefined,
    ctx: usize = 0,
    once: bool = false,
    active: bool = true,
}

pub struct BidiPipeTarget {
    callback: BidiPipeEmitterFn = undefined,
    ctx: usize = 0,
}

// ============================================================================
// 16-Lane @Vector(16, u8) SIMD WebDriver BiDi Frame Scanner
// ============================================================================

pub struct BidiFrameScanner {
    /// 16-lane SIMD scan locating the first byte matching `needle` in a BiDi frame.
    pub static fn indexOfByte(input: []const u8, needle: u8): usize? {
        let i: usize = 0;
        const n = input.len;
        const v_needle: BidiVec16 = @splat(needle);

        while (i + 16 <= n) {
            const chunk: BidiVec16 = input[i..][0..16].*;
            const mask: u16 = @bitCast(chunk == v_needle);
            if (mask != 0) {
                return i + (@ctz(mask) as usize);
            }
            i += 16;
        }

        while (i < n) {
            if (input[i] == needle) return i;
            i += 1;
        }
        return null;
    }

    /// 16-lane SIMD scan locating JSON key/value boundaries ('"', ':', '{', '}') for fast BiDi routing.
    pub static fn indexOfBidiDelimiter(input: []const u8): usize? {
        let i: usize = 0;
        const n = input.len;
        const v_quote: BidiVec16 = @splat('"');
        const v_colon: BidiVec16 = @splat(':');
        const v_lbrace: BidiVec16 = @splat('{');
        const v_rbrace: BidiVec16 = @splat('}');

        while (i + 16 <= n) {
            const chunk: BidiVec16 = input[i..][0..16].*;
            const m1: u16 = @bitCast(chunk == v_quote);
            const m2: u16 = @bitCast(chunk == v_colon);
            const m3: u16 = @bitCast(chunk == v_lbrace);
            const m4: u16 = @bitCast(chunk == v_rbrace);
            const mask: u16 = m1 | m2 | m3 | m4;
            if (mask != 0) {
                return i + (@ctz(mask) as usize);
            }
            i += 16;
        }

        while (i < n) {
            const c = input[i];
            if (c == '"' || c == ':' || c == '{' || c == '}') return i;
            i += 1;
        }
        return null;
    }

    /// Zero-heap top-level JSON field extraction delegated to cap:cdp CdpFrameScanner.
    pub static fn extractField(json_obj: []const u8, key: []const u8, unquote: bool): ([]const u8)? {
        return CdpFrameScanner.extractTopLevelField(json_obj, key, unquote);
    }

    /// Parse signed 64-bit integer using CdpFrameScanner fast ASCII path.
    pub static fn parseI64(raw: []const u8): i64? {
        return CdpFrameScanner.parseFastI64(raw);
    }

    /// Decode a raw W3C WebDriver BiDi JSON frame into BidiMessage without heap allocation.
    /// Supports:
    /// - {"type":"success","id":1,"result":{...}}
    /// - {"type":"error","id":1|null,"error":"...","message":"...","stacktrace":"..."}
    /// - {"type":"event","method":"...","params":{...}}
    /// - {"id":1,"method":"...","params":{...}} (outgoing command)
    pub static fn decodeMessage(raw: []const u8): BidiMessage!BidiError {
        const trimmed = CdpFrameScanner.trimWs(raw);
        if (trimmed.len < 2 || trimmed[0] != '{') {
            return BidiError.InvalidFrame;
        }

        const type_slice = CdpFrameScanner.extractTopLevelField(trimmed, "type", true);
        const id_slice = CdpFrameScanner.extractTopLevelField(trimmed, "id", false);
        const method_slice = CdpFrameScanner.extractTopLevelField(trimmed, "method", true);

        let parsed_id: i64 = 0;
        let has_id = false;
        if (id_raw := id_slice) {
            const id_trim = CdpFrameScanner.trimWs(id_raw);
            if (!std.mem.eql(u8, id_trim, "null")) {
                parsed_id = CdpFrameScanner.parseFastI64(id_trim) orelse return BidiError.InvalidJson;
                has_id = true;
            }
        }

        if (t := type_slice) {
            if (std.mem.eql(u8, t, "success")) {
                if (!has_id) return BidiError.MissingId;
                const result_raw = CdpFrameScanner.extractTopLevelField(trimmed, "result", false) orelse "{}";
                return BidiMessage{
                    msg_type: BidiMessageType.Success,
                    id: parsed_id,
                    has_id: true,
                    result_json: result_raw,
                };
            }
            if (std.mem.eql(u8, t, "error")) {
                const err_code = CdpFrameScanner.extractTopLevelField(trimmed, "error", true) orelse "unknown error";
                const err_msg = CdpFrameScanner.extractTopLevelField(trimmed, "message", true) orelse "";
                const stack = CdpFrameScanner.extractTopLevelField(trimmed, "stacktrace", true) orelse "";
                return BidiMessage{
                    msg_type: BidiMessageType.Error,
                    id: parsed_id,
                    has_id: has_id,
                    error_code: err_code,
                    error_message: err_msg,
                    stacktrace: stack,
                };
            }
            if (std.mem.eql(u8, t, "event")) {
                const ev_method = method_slice orelse return BidiError.MissingMethod;
                const ev_params = CdpFrameScanner.extractTopLevelField(trimmed, "params", false) orelse "{}";
                return BidiMessage{
                    msg_type: BidiMessageType.Event,
                    method: ev_method,
                    params_json: ev_params,
                };
            }
        }

        // Outgoing command: {"id":1,"method":"...","params":{...}}
        if (has_id) {
            if (m := method_slice) {
                const params_raw = CdpFrameScanner.extractTopLevelField(trimmed, "params", false) orelse "{}";
                return BidiMessage{
                    msg_type: BidiMessageType.Command,
                    id: parsed_id,
                    has_id: true,
                    method: m,
                    params_json: params_raw,
                };
            }
            // Non-BiDi response carrying an id (e.g. accidental CDP endpoint)
            return BidiMessage{
                msg_type: BidiMessageType.Error,
                id: parsed_id,
                has_id: true,
                error_code: "protocol error",
                error_message: "Message is not in WebDriver BiDi protocol format",
            };
        }

        return BidiError.InvalidFrame;
    }

    /// Encode a BidiCommand into a caller-provided stack/arena buffer (zero-heap).
    pub static fn encodeCommand(cmd: BidiCommand, buf: []u8): usize!BidiError {
        if (cmd.method.len == 0) return BidiError.MissingMethod;
        const params = if (cmd.params_json.len > 0) cmd.params_json else "{}";
        const s = std.fmt.bufPrint(
            buf,
            "{{\"id\":{d},\"method\":\"{s}\",\"params\":{s}}}",
            .{ cmd.id, cmd.method, params },
        ) catch return BidiError.BufferTooSmall;
        return s.len;
    }

    /// Encode a BidiCommand into a heap-allocated byte slice.
    pub static fn encodeCommandAlloc(alloc: std.mem.Allocator, cmd: BidiCommand): ([]u8)!BidiError {
        if (cmd.method.len == 0) return BidiError.MissingMethod;
        const params = if (cmd.params_json.len > 0) cmd.params_json else "{}";
        return std.fmt.allocPrint(
            alloc,
            "{{\"id\":{d},\"method\":\"{s}\",\"params\":{s}}}",
            .{ cmd.id, cmd.method, params },
        ) catch return BidiError.OutOfMemory;
    }
}

// ============================================================================
// BidiConnection — Command Correlation, Event Dispatch & Transport Binding
// ============================================================================

pub struct BidiConnection {
    allocator: std.mem.Allocator = undefined,
    pub url: []const u8 = "",
    pub state: BidiConnectionState = BidiConnectionState.Disconnected,
    pub next_id: i64 = 1,
    pub timeout_ms: u32 = 180000,
    pub delay_ms: u32 = 0,
    pending: std.AutoHashMapUnmanaged(i64, BidiPendingCall) = .empty,
    listeners: std.ArrayList(BidiEventListener) = .empty,
    pipes: std.ArrayList(BidiPipeTarget) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,
    transport_fn: BidiSendTransportFn? = null,
    transport_ctx: usize = 0,
    native_ws: (*CdpNativeWsTransport)? = null,
    pub total_sent: u64 = 0,
    pub total_received: u64 = 0,
    pub total_events: u64 = 0,

    pub static fn init(alloc: std.mem.Allocator, ws_url: []const u8): BidiConnection {
        return BidiConnection{
            allocator: alloc,
            url: ws_url,
            state: BidiConnectionState.Disconnected,
            next_id: 1,
            timeout_ms: 180000,
            delay_ms: 0,
            pending: .empty,
            listeners: .empty,
            pipes: .empty,
            owned_strings: .empty,
            transport_fn: null,
            transport_ctx: 0,
            native_ws: null,
            total_sent: 0,
            total_received: 0,
            total_events: 0,
        };
    }

    pub fn deinit(*_) {
        _.pending.deinit(_.allocator);
        _.pending = .empty;
        _.listeners.deinit(_.allocator);
        _.listeners = .empty;
        _.pipes.deinit(_.allocator);
        _.pipes = .empty;
        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;
    }

    ~init() {
        _.deinit();
    }

    fn intern(*_, s: []const u8): ([]const u8)!BidiError {
        if (s.len == 0) return "";
        const duped = _.allocator.dupe(u8, s) catch return BidiError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return BidiError.OutOfMemory;
        };
        return duped;
    }

    pub fn closed(): bool {
        return _.state == BidiConnectionState.Closed;
    }

    pub fn setTransport(*_, cb: BidiSendTransportFn, ctx: usize) {
        _.transport_fn = cb;
        _.transport_ctx = ctx;
        _.state = BidiConnectionState.Connected;
    }

    pub fn attachNativeWsTransport(*_, ws: *CdpNativeWsTransport) {
        _.native_ws = ws;
        _.state = BidiConnectionState.Connected;
    }

    pub fn markConnected(*_) {
        _.state = BidiConnectionState.Connected;
    }

    pub fn unbind(*_) {
        if (_.state == BidiConnectionState.Closed) return;
        _.state = BidiConnectionState.Closed;
        _.transport_fn = null;
        _.transport_ctx = 0;
        _.native_ws = null;
    }

    pub fn close(*_) {
        if (_.state == BidiConnectionState.Closed) return;
        _.unbind();
        const closed_ev = BidiEvent{ method: "closed", params_json: "{}" };
        _.dispatchEvent(closed_ev);
    }

    pub fn allocateId(*_): i64 {
        const id = _.next_id;
        _.next_id += 1;
        return id;
    }

    /// Subscribe a listener to a BiDi event method (supports exact match, "*" wildcard, or "domain.*" prefix).
    pub fn on(*_, method_filter: []const u8, callback: BidiEventCallback, ctx: usize): void!BidiError {
        const filter_copy = _.intern(method_filter)?;
        _.listeners.append(_.allocator, BidiEventListener{
            method_filter: filter_copy,
            callback: callback,
            ctx: ctx,
            once: false,
            active: true,
        }) catch return BidiError.OutOfMemory;
    }

    /// Subscribe a one-shot listener to a BiDi event method.
    pub fn once(*_, method_filter: []const u8, callback: BidiEventCallback, ctx: usize): void!BidiError {
        const filter_copy = _.intern(method_filter)?;
        _.listeners.append(_.allocator, BidiEventListener{
            method_filter: filter_copy,
            callback: callback,
            ctx: ctx,
            once: true,
            active: true,
        }) catch return BidiError.OutOfMemory;
    }

    /// Pipe all emitted BiDi events to another subscriber/emitter.
    pub fn pipeTo(*_, callback: BidiPipeEmitterFn, ctx: usize): void!BidiError {
        _.pipes.append(_.allocator, BidiPipeTarget{
            callback: callback,
            ctx: ctx,
        }) catch return BidiError.OutOfMemory;
    }

    /// Encode & send a WebDriver BiDi command, registering a pending callback slot.
    pub fn sendCommand(*_, method: []const u8, params_json: []const u8, out_buf: []u8): i64!BidiError {
        if (_.state == BidiConnectionState.Closed) return BidiError.ConnectionClosed;
        if (method.len == 0) return BidiError.MissingMethod;

        const id = _.allocateId();
        const method_copy = _.intern(method)?;
        const cmd = BidiCommand{
            id: id,
            method: method,
            params_json: if (params_json.len > 0) params_json else "{}",
        };
        const written = BidiFrameScanner.encodeCommand(cmd, out_buf)?;

        _.pending.put(_.allocator, id, BidiPendingCall{
            id: id,
            method: method_copy,
            completed: false,
            is_error: false,
            result_json: "{}",
            error_code: "",
            error_message: "",
            stacktrace: "",
        }) catch return BidiError.OutOfMemory;

        _.total_sent += 1;

        if (cb := _.transport_fn) {
            if (!cb(_.transport_ctx, out_buf[0..written])) {
                return BidiError.ConnectionClosed;
            }
        }
        return id;
    }

    /// Send a command and immediately consume its correlated response (for synchronous/mock transports).
    pub fn sendAndTake(*_, method: []const u8, params_json: []const u8, out_buf: []u8): BidiSuccessResponse!BidiError {
        const req_id = _.sendCommand(method, params_json, out_buf)?;
        if (entry := _.takePending(req_id)) {
            if (!entry.completed) return BidiError.Timeout;
            if (entry.is_error) return BidiError.ProtocolError;
            return BidiSuccessResponse{
                id: entry.id,
                result_json: entry.result_json,
            };
        }
        return BidiError.Timeout;
    }

    /// Feed an incoming raw JSON WebDriver BiDi frame from transport into the connection state machine.
    pub fn onMessage(*_, raw_frame: []const u8): BidiMessage!BidiError {
        const msg = BidiFrameScanner.decodeMessage(raw_frame)?;
        _.total_received += 1;

        if (msg.msg_type == BidiMessageType.Success) {
            if (ptr := _.pending.getPtr(msg.id)) {
                const res_copy = _.intern(msg.result_json)?;
                ptr.completed = true;
                ptr.is_error = false;
                ptr.result_json = res_copy;
            }
            return msg;
        }

        if (msg.msg_type == BidiMessageType.Error) {
            if (msg.has_id) {
                if (ptr := _.pending.getPtr(msg.id)) {
                    const code_copy = _.intern(msg.error_code)?;
                    const msg_copy = _.intern(msg.error_message)?;
                    const stack_copy = _.intern(msg.stacktrace)?;
                    ptr.completed = true;
                    ptr.is_error = true;
                    ptr.error_code = code_copy;
                    ptr.error_message = msg_copy;
                    ptr.stacktrace = stack_copy;
                }
            }
            return msg;
        }

        if (msg.msg_type == BidiMessageType.Event) {
            _.total_events += 1;
            _.dispatchEvent(msg.toEvent());
            return msg;
        }

        return msg;
    }

    /// Retrieve and remove a completed pending call by ID.
    pub fn takePending(*_, id: i64): BidiPendingCall? {
        if (kv := _.pending.fetchRemove(id)) {
            return kv.value;
        }
        return null;
    }

    /// Count currently unresolved pending commands.
    pub fn pendingCount(): usize {
        let count: usize = 0;
        let iter = _.pending.iterator();
        while (entry := iter.next()) {
            if (!entry.value_ptr.completed) {
                count += 1;
            }
        }
        return count;
    }

    static fn matchesFilter(filter: []const u8, method: []const u8): bool {
        if (filter.len == 0 || (filter.len == 1 && filter[0] == '*')) return true;
        if (filter.len >= 2 && filter[filter.len - 1] == '*' && filter[filter.len - 2] == '.') {
            return std.mem.startsWith(u8, method, filter[0 .. filter.len - 1]);
        }
        return std.mem.eql(u8, filter, method);
    }

    fn dispatchEvent(*_, ev: BidiEvent) {
        let p: usize = 0;
        while (p < _.pipes.items.len) {
            const target = _.pipes.items[p];
            target.callback(target.ctx, ev);
            p += 1;
        }

        let i: usize = 0;
        while (i < _.listeners.items.len) {
            if (_.listeners.items[i].active) {
                if (BidiConnection.matchesFilter(_.listeners.items[i].method_filter, ev.method)) {
                    const cb = _.listeners.items[i].callback;
                    const ctx = _.listeners.items[i].ctx;
                    if (_.listeners.items[i].once) {
                        _.listeners.items[i].active = false;
                    }
                    cb(ctx, ev);
                }
            }
            i += 1;
        }
    }
}
