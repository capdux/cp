// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/CDPSession.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/cdp_session.cp — CDPSession Wrapper over cap:cdp
// Provides multi-target session multiplexing, command send/correlate & detach.
// ============================================================================

const std = @import("std");
import {
    CdpError,
    CdpResponse,
    CdpEvent,
    CdpEventCallback,
    CdpSession,
    CdpEndpoint,
    CdpFrameScanner,
} from "cap:cdp";
import { PuppeteerError, mapCdpError } from "./types";

pub struct CDPSession {
    endpoint: *CdpEndpoint = undefined,
    inner: CdpSession = .{},

    pub static fn init(
        endpoint: *CdpEndpoint,
        session_id: []const u8,
        target_type: []const u8,
        target_id: []const u8,
        parent_session_id: []const u8,
    ): CDPSession {
        return CDPSession{
            endpoint: endpoint,
            inner: CdpSession.init(session_id, target_type, target_id, parent_session_id),
        };
    }

    pub fn id(): []const u8 {
        return _.inner.session_id;
    }

    pub fn targetType(): []const u8 {
        return _.inner.target_type;
    }

    pub fn targetId(): []const u8 {
        return _.inner.target_id;
    }

    pub fn parentSessionId(): []const u8 {
        return _.inner.parent_session_id;
    }

    pub fn detached(): bool {
        if (_.inner.detached) return true;
        return !_.endpoint.hasActiveSession(_.inner.session_id);
    }

    pub fn send(*_, method: []const u8, params_json: []const u8, out_buf: []u8): i64!PuppeteerError {
        if (_.inner.detached) return PuppeteerError.SessionClosed;
        const p = if (params_json.len > 0) params_json else "{}";
        const req_id = _.endpoint.sendCommand(_.inner.session_id, method, p, out_buf) catch |e| {
            return mapCdpError(e);
        };
        return req_id;
    }

    pub fn sendAndTakeResponse(
        *_,
        method: []const u8,
        params_json: []const u8,
        out_buf: []u8,
    ): CdpResponse!PuppeteerError {
        const p = if (params_json.len > 0) params_json else "{}";
        const req_id = _.endpoint.sendCommand(_.inner.session_id, method, p, out_buf) catch |e| { return mapCdpError(e); };
        if (resp := _.endpoint.takeResponse(req_id)) {
            if (resp.is_error) {
                return PuppeteerError.ProtocolError;
            }
            return resp;
        }
        return PuppeteerError.Timeout;
    }

    pub fn on(
        *_,
        method_filter: []const u8,
        callback: CdpEventCallback,
        ctx: usize,
    ): void!PuppeteerError {
        _.endpoint.onEvent(method_filter, _.inner.session_id, callback, ctx) catch |e| {
            return mapCdpError(e);
        };
    }

    pub fn detach(*_): void!PuppeteerError {
        if (_.inner.detached) return;
        _.inner.markDetached();
        if (_.inner.session_id.len > 0) {
            const _ign_1 = _.endpoint.detachSession(_.inner.session_id);
            * = _ign_1;
        }
    }
}
