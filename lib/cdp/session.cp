// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/CDPSession.ts | Synced: 2026-11-28
// ============================================================================
// lib/cdp/session.cp — CdpEndpoint & CdpSession Multi-Target Routing
// Handles connection state machine, auto-increment next_id, sessionId
// multiplexing, request-response correlation, and event callback dispatch.
// ============================================================================

const std = @import("std");
import {
    CdpError,
    CdpMessageKind,
    CdpRequest,
    CdpResponse,
    CdpEvent,
    CdpMessage,
    CdpFrameScanner,
} from "./protocol";

pub enum CdpConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Closing,
    Closed,
}

pub type CdpEventCallback = fn(ctx: usize, ev: CdpEvent): void;
pub type CdpSendTransportFn = fn(ctx: usize, payload: []const u8): bool;

pub struct CdpSessionInfo {
    session_id: []const u8 = "",
    target_type: []const u8 = "page",
    target_id: []const u8 = "",
    parent_session_id: []const u8 = "",
    detached: bool = false,
}

pub struct CdpPendingCall {
    id: i64 = 0,
    method: []const u8 = "",
    session_id: []const u8 = "",
    completed: bool = false,
    response: CdpResponse = .{},
}

pub struct CdpEventListener {
    method_filter: []const u8 = "",
    session_filter: []const u8 = "",
    callback: CdpEventCallback = undefined,
    ctx: usize = 0,
}

pub struct CdpSession {
    pub session_id: []const u8 = "";
    pub target_type: []const u8 = "page";
    pub target_id: []const u8 = "";
    pub parent_session_id: []const u8 = "";
    pub detached: bool = false;

    pub static fn init(
        session_id: []const u8,
        target_type: []const u8,
        target_id: []const u8,
        parent_session_id: []const u8,
    ): CdpSession {
        return CdpSession{
            session_id: session_id,
            target_type: target_type,
            target_id: target_id,
            parent_session_id: parent_session_id,
            detached: false,
        };
    }

    pub fn isRoot(): bool {
        return _.session_id.len == 0;
    }

    pub fn markDetached(*_) {
        _.detached = true;
    }

    pub fn encodeCommand(id: i64, method: []const u8, params_json: []const u8, out_buf: []u8): usize!CdpError {
        if (_.detached) return CdpError.SessionClosed;
        const req = CdpRequest{
            id: id,
            method: method,
            params_json: params_json,
            session_id: _.session_id,
        };
        return CdpFrameScanner.encodeRequest(req, out_buf);
    }
}

pub struct CdpEndpoint {
    allocator: std.mem.Allocator = undefined,
    pub state: CdpConnectionState = CdpConnectionState.Disconnected,
    pub next_id: i64 = 1,
    sessions: std.StringHashMapUnmanaged(CdpSessionInfo) = .empty,
    pending: std.AutoHashMapUnmanaged(i64, CdpPendingCall) = .empty,
    listeners: std.ArrayList(CdpEventListener) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,
    transport_fn: CdpSendTransportFn? = null,
    transport_ctx: usize = 0,
    pub total_sent: u64 = 0,
    pub total_received: u64 = 0,

    pub static fn init(alloc: std.mem.Allocator): CdpEndpoint {
        return CdpEndpoint{
            allocator: alloc,
            state: CdpConnectionState.Disconnected,
            next_id: 1,
            sessions: .empty,
            pending: .empty,
            listeners: .empty,
            owned_strings: .empty,
            transport_fn: null,
            transport_ctx: 0,
            total_sent: 0,
            total_received: 0,
        };
    }

    pub fn deinit(*_) {
        _.sessions.deinit(_.allocator);
        _.sessions = .empty;
        _.pending.deinit(_.allocator);
        _.pending = .empty;
        _.listeners.deinit(_.allocator);
        _.listeners = .empty;
        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;
    }

    ~init() {
        _.deinit();
    }

    fn intern(*_, s: []const u8): ([]const u8)!CdpError {
        const duped = _.allocator.dupe(u8, s) catch return CdpError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return CdpError.OutOfMemory;
        };
        return duped;
    }

    pub fn setTransport(*_, cb: CdpSendTransportFn, ctx: usize) {
        _.transport_fn = cb;
        _.transport_ctx = ctx;
    }

    pub fn beginConnect(*_) {
        _.state = CdpConnectionState.Connecting;
    }

    pub fn markConnected(*_) {
        _.state = CdpConnectionState.Connected;
    }

    pub fn close(*_) {
        if (_.state == CdpConnectionState.Closed) return;
        _.state = CdpConnectionState.Closed;
        let iter = _.sessions.iterator();
        while (entry := iter.next()) {
            entry.value_ptr.detached = true;
        }
    }

    pub fn allocateId(*_): i64 {
        const id = _.next_id;
        _.next_id += 1;
        return id;
    }

    pub fn createSession(
        *_,
        session_id: []const u8,
        target_type: []const u8,
        target_id: []const u8,
        parent_session_id: []const u8,
    ): void!CdpError {
        if (session_id.len == 0) return CdpError.SessionNotFound;
        const sid = _.intern(session_id)?;
        const ttype = _.intern(target_type)?;
        const tid = _.intern(target_id)?;
        const psid = _.intern(parent_session_id)?;

        _.sessions.put(_.allocator, sid, CdpSessionInfo{
            session_id: sid,
            target_type: ttype,
            target_id: tid,
            parent_session_id: psid,
            detached: false,
        }) catch return CdpError.OutOfMemory;
    }

    pub fn detachSession(*_, session_id: []const u8): bool {
        if (info := _.sessions.getPtr(session_id)) {
            info.detached = true;
            return true;
        }
        return false;
    }

    pub fn hasActiveSession(session_id: []const u8): bool {
        if (session_id.len == 0) {
            return _.state == CdpConnectionState.Connected;
        }
        if (info := _.sessions.get(session_id)) {
            return !info.detached;
        }
        return false;
    }

    pub fn sessionCount(): usize {
        return _.sessions.count();
    }

    pub fn pendingCount(): usize {
        return _.pending.count();
    }

    pub fn onEvent(
        *_,
        method_filter: []const u8,
        session_filter: []const u8,
        callback: CdpEventCallback,
        ctx: usize,
    ): void!CdpError {
        const mf = _.intern(method_filter)?;
        const sf = _.intern(session_filter)?;
        _.listeners.append(_.allocator, CdpEventListener{
            method_filter: mf,
            session_filter: sf,
            callback: callback,
            ctx: ctx,
        }) catch return CdpError.OutOfMemory;
    }

    /// Encode a CDP command, register a pending call for correlation, and send via transport if configured.
    pub fn sendCommand(
        *_,
        session_id: []const u8,
        method: []const u8,
        params_json: []const u8,
        out_buf: []u8,
    ): i64!CdpError {
        if (_.state != CdpConnectionState.Connected) {
            return CdpError.NotConnected;
        }
        if (session_id.len > 0) {
            if (info := _.sessions.get(session_id)) {
                if (info.detached) return CdpError.SessionClosed;
            } else {
                return CdpError.SessionNotFound;
            }
        }

        const id = _.allocateId();
        const req = CdpRequest{
            id: id,
            method: method,
            params_json: params_json,
            session_id: session_id,
        };
        const n = CdpFrameScanner.encodeRequest(req, out_buf)?;

        const m_interned = _.intern(method)?;
        const s_interned = _.intern(session_id)?;
        _.pending.put(_.allocator, id, CdpPendingCall{
            id: id,
            method: m_interned,
            session_id: s_interned,
            completed: false,
            response: .{},
        }) catch return CdpError.OutOfMemory;

        if (send_fn := _.transport_fn) {
            const ok = send_fn(_.transport_ctx, out_buf[0..n]);
            if (!ok) {
                return CdpError.ConnectionClosed;
            }
        }
        _.total_sent += 1;
        return id;
    }

    /// Decode an incoming raw JSON-RPC frame, correlate pending responses, and dispatch events.
    pub fn handleIncomingFrame(*_, raw: []const u8): CdpMessage!CdpError {
        const msg = CdpFrameScanner.decodeMessage(raw)?;
        _.total_received += 1;

        if (msg.isResponse()) {
            if (call := _.pending.getPtr(msg.id)) {
                const res_json = _.intern(msg.result_json)?;
                const err_msg = _.intern(msg.error_message)?;
                const sess_id = _.intern(msg.session_id)?;
                call.completed = true;
                call.response = CdpResponse{
                    id: msg.id,
                    result_json: res_json,
                    error_code: msg.error_code,
                    error_message: err_msg,
                    session_id: sess_id,
                    is_error: msg.isError(),
                };
            }
            return msg;
        }

        if (msg.isEvent()) {
            const ev = msg.toEvent();
            // Built-in Target lifecycle routing
            if (std.mem.eql(u8, ev.method, "Target.detachedFromTarget")) {
                if (sid := CdpFrameScanner.extractTopLevelField(ev.params_json, "sessionId", true)) {
                    * = _.detachSession(sid);
                }
            }
            // Dispatch to matching listeners
            for (listener of _.listeners.items) {
                const method_match = listener.method_filter.len == 0 ||
                    std.mem.eql(u8, listener.method_filter, "*") ||
                    std.mem.eql(u8, listener.method_filter, ev.method);
                const session_match = listener.session_filter.len == 0 ||
                    std.mem.eql(u8, listener.session_filter, "*") ||
                    std.mem.eql(u8, listener.session_filter, ev.session_id);
                if (method_match && session_match) {
                    listener.callback(listener.ctx, ev);
                }
            }
        }
        return msg;
    }

    /// Retrieve and remove a completed response for `id`. Returns null if still pending or unknown.
    pub fn takeResponse(*_, id: i64): CdpResponse? {
        if (call := _.pending.get(id)) {
            if (!call.completed) return null;
            const resp = call.response;
            * = _.pending.remove(id);
            return resp;
        }
        return null;
    }
}
