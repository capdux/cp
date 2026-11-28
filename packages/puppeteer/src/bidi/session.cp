// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/bidi/core/Session.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/bidi/session.cp — BidiSession, BidiUserContext & BidiBrowser
// Implements session.new/status/subscribe/unsubscribe/end and
// browser.createUserContext/removeUserContext/getUserContexts/close/setDownloadBehavior.
// ============================================================================

const std = @import("std");
import {
    BidiError,
    BidiSuccessResponse,
    SessionCapabilitiesRequest,
    SessionCapabilities,
    SessionStatusResult,
    CreateUserContextParams,
    DownloadBehaviorPolicy,
    DownloadBehaviorParams,
    CreateContextType,
    CreateContextParams,
    createContextTypeName,
    proxyTypeName,
    bidiWriteStr,
    bidiWriteJsonEscaped,
} from "./protocol";
import { BidiConnection, BidiFrameScanner } from "./connection";

// ============================================================================
// BidiSession — W3C WebDriver BiDi Session Lifecycle
// ============================================================================

pub struct BidiSession {
    conn: *BidiConnection = undefined,
    session_id_buf: [128]u8 = undefined,
    session_id_len: usize = 0,
    browser_name_buf: [64]u8 = undefined,
    browser_name_len: usize = 0,
    browser_version_buf: [64]u8 = undefined,
    browser_version_len: usize = 0,
    user_agent_buf: [256]u8 = undefined,
    user_agent_len: usize = 0,
    pub accept_insecure_certs: bool = false,
    pub ended: bool = false,

    pub static fn init(conn: *BidiConnection): BidiSession {
        return BidiSession{
            conn: conn,
            session_id_len: 0,
            browser_name_len: 0,
            browser_version_len: 0,
            user_agent_len: 0,
            accept_insecure_certs: false,
            ended: false,
        };
    }

    pub fn id(): []const u8 {
        return _.session_id_buf[0.._.session_id_len];
    }

    pub fn browserName(): []const u8 {
        return _.browser_name_buf[0.._.browser_name_len];
    }

    pub fn browserVersion(): []const u8 {
        return _.browser_version_buf[0.._.browser_version_len];
    }

    pub fn userAgent(): []const u8 {
        return _.user_agent_buf[0.._.user_agent_len];
    }

    pub fn capabilities(): SessionCapabilities {
        return SessionCapabilities{
            accept_insecure_certs: _.accept_insecure_certs,
            browser_name: _.browserName(),
            browser_version: _.browserVersion(),
            user_agent: _.userAgent(),
        };
    }

    /// Execute `session.new` to establish a WebDriver BiDi session and parse capabilities.
    pub fn start(*_, req: SessionCapabilitiesRequest): void!BidiError {
        if (_.ended) return BidiError.SessionClosed;

        let pbuf: [1024]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"capabilities\":{\"alwaysMatch\":{\"acceptInsecureCerts\":")?;
        pos = bidiWriteStr(&pbuf, pos, if (req.accept_insecure_certs) "true" else "false")?;
        if (req.browser_name.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"browserName\":\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, req.browser_name)?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
        }
        if (req.unhandled_prompt_behavior.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"unhandledPromptBehavior\":\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, req.unhandled_prompt_behavior)?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
        }
        pos = bidiWriteStr(&pbuf, pos, "}}}")?;

        let frame_buf: [2048]u8 = undefined;
        const resp = _.conn.sendAndTake("session.new", pbuf[0..pos], &frame_buf)?;

        if (sid := BidiFrameScanner.extractField(resp.result_json, "sessionId", true)) {
            const n = if (sid.len <= 128) sid.len else 128;
            if (n > 0) @memcpy(_.session_id_buf[0..n], sid[0..n]);
            _.session_id_len = n;
        }

        if (caps := BidiFrameScanner.extractField(resp.result_json, "capabilities", false)) {
            if (bn := BidiFrameScanner.extractField(caps, "browserName", true)) {
                const n = if (bn.len <= 64) bn.len else 64;
                if (n > 0) @memcpy(_.browser_name_buf[0..n], bn[0..n]);
                _.browser_name_len = n;
            }
            if (bv := BidiFrameScanner.extractField(caps, "browserVersion", true)) {
                const n = if (bv.len <= 64) bv.len else 64;
                if (n > 0) @memcpy(_.browser_version_buf[0..n], bv[0..n]);
                _.browser_version_len = n;
            }
            if (ua := BidiFrameScanner.extractField(caps, "userAgent", true)) {
                const n = if (ua.len <= 256) ua.len else 256;
                if (n > 0) @memcpy(_.user_agent_buf[0..n], ua[0..n]);
                _.user_agent_len = n;
            }
            if (aic := BidiFrameScanner.extractField(caps, "acceptInsecureCerts", false)) {
                _.accept_insecure_certs = std.mem.eql(u8, aic, "true");
            }
        }
    }

    /// Execute `session.status` to query endpoint readiness.
    pub fn status(*_): SessionStatusResult!BidiError {
        let frame_buf: [512]u8 = undefined;
        const resp = _.conn.sendAndTake("session.status", "{}", &frame_buf)?;
        let ready = false;
        if (r := BidiFrameScanner.extractField(resp.result_json, "ready", false)) {
            ready = std.mem.eql(u8, r, "true");
        }
        const msg = BidiFrameScanner.extractField(resp.result_json, "message", true) orelse "";
        return SessionStatusResult{
            ready: ready,
            message: msg,
        };
    }

    /// Execute `session.subscribe` for one or more BiDi event names (and optional contexts).
    pub fn subscribe(*_, events: []const []const u8, contexts: []const []const u8): void!BidiError {
        if (_.ended) return BidiError.SessionClosed;
        let pbuf: [2048]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"events\":[")?;
        let i: usize = 0;
        while (i < events.len) {
            if (i > 0) pos = bidiWriteStr(&pbuf, pos, ",")?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, events[i])?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
            i += 1;
        }
        pos = bidiWriteStr(&pbuf, pos, "]")?;
        if (contexts.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"contexts\":[")?;
            let j: usize = 0;
            while (j < contexts.len) {
                if (j > 0) pos = bidiWriteStr(&pbuf, pos, ",")?;
                pos = bidiWriteStr(&pbuf, pos, "\"")?;
                pos = bidiWriteJsonEscaped(&pbuf, pos, contexts[j])?;
                pos = bidiWriteStr(&pbuf, pos, "\"")?;
                j += 1;
            }
            pos = bidiWriteStr(&pbuf, pos, "]")?;
        }
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [2560]u8 = undefined;
        * = _.conn.sendAndTake("session.subscribe", pbuf[0..pos], &frame_buf)?;
    }

    /// Execute `session.unsubscribe` for one or more BiDi event names.
    pub fn unsubscribe(*_, events: []const []const u8): void!BidiError {
        if (_.ended) return BidiError.SessionClosed;
        let pbuf: [1024]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"events\":[")?;
        let i: usize = 0;
        while (i < events.len) {
            if (i > 0) pos = bidiWriteStr(&pbuf, pos, ",")?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, events[i])?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
            i += 1;
        }
        pos = bidiWriteStr(&pbuf, pos, "]}")?;

        let frame_buf: [1536]u8 = undefined;
        * = _.conn.sendAndTake("session.unsubscribe", pbuf[0..pos], &frame_buf)?;
    }

    /// Send an arbitrary BiDi command through this session.
    pub fn sendCommand(*_, method: []const u8, params_json: []const u8, out_buf: []u8): BidiSuccessResponse!BidiError {
        if (_.ended) return BidiError.SessionClosed;
        return _.conn.sendAndTake(method, params_json, out_buf);
    }

    /// Execute `session.end` and mark the session ended.
    pub fn end(*_): void!BidiError {
        if (_.ended) return;
        let frame_buf: [512]u8 = undefined;
        * = _.conn.sendAndTake("session.end", "{}", &frame_buf)?;
        _.ended = true;
    }
}

// ============================================================================
// BidiUserContext — Isolated Browser Context (Default / Incognito)
// ============================================================================

pub struct BidiUserContext {
    session: *BidiSession = undefined,
    id_buf: [128]u8 = undefined,
    id_len: usize = 0,
    pub closed: bool = false,

    pub static fn init(session: *BidiSession, user_context_id: []const u8): BidiUserContext {
        let uc = BidiUserContext{
            session: session,
            id_len: 0,
            closed: false,
        };
        const n = if (user_context_id.len <= 128) user_context_id.len else 128;
        if (n > 0) @memcpy(uc.id_buf[0..n], user_context_id[0..n]);
        uc.id_len = n;
        return uc;
    }

    pub fn id(): []const u8 {
        return _.id_buf[0.._.id_len];
    }

    pub fn isDefault(): bool {
        return std.mem.eql(u8, _.id(), "default");
    }

    /// Create a new top-level browsing context (tab or window) inside this user context.
    /// Returns the created context ID slice written into `id_out_buf`.
    pub fn createBrowsingContext(
        *_,
        ctx_type: CreateContextType,
        background: bool,
        id_out_buf: []u8,
    ): ([]const u8)!BidiError {
        if (_.closed) return BidiError.ContextClosed;
        let pbuf: [512]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"type\":\"")?;
        pos = bidiWriteStr(&pbuf, pos, createContextTypeName(ctx_type))?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"userContext\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"background\":")?;
        pos = bidiWriteStr(&pbuf, pos, if (background) "true" else "false")?;
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [1024]u8 = undefined;
        const resp = _.session.sendCommand("browsingContext.create", pbuf[0..pos], &frame_buf)?;
        const ctx_id = BidiFrameScanner.extractField(resp.result_json, "context", true) orelse return BidiError.InvalidJson;
        if (ctx_id.len > id_out_buf.len) return BidiError.BufferTooSmall;
        @memcpy(id_out_buf[0..ctx_id.len], ctx_id);
        return id_out_buf[0..ctx_id.len];
    }

    /// Remove this user context via `browser.removeUserContext`.
    pub fn remove(*_): void!BidiError {
        if (_.closed) return;
        let pbuf: [256]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"userContext\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;

        let frame_buf: [512]u8 = undefined;
        * = _.session.sendCommand("browser.removeUserContext", pbuf[0..pos], &frame_buf)?;
        _.closed = true;
    }

    /// Query cookies partitioned to this user context via `storage.getCookies`.
    pub fn getCookiesRaw(*_): ([]const u8)!BidiError {
        if (_.closed) return BidiError.ContextClosed;
        let pbuf: [256]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"partition\":{\"type\":\"storageKey\",\"userContext\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\"}}")?;

        let frame_buf: [512]u8 = undefined;
        const resp = _.session.sendCommand("storage.getCookies", pbuf[0..pos], &frame_buf)?;
        return resp.result_json;
    }

    /// Grant or deny a permission for this user context via `permissions.setPermission`.
    pub fn setPermission(*_, name: []const u8, state: []const u8, origin: []const u8): void!BidiError {
        if (_.closed) return BidiError.ContextClosed;
        let pbuf: [512]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"descriptor\":{\"name\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, name)?;
        pos = bidiWriteStr(&pbuf, pos, "\"},\"state\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, state)?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"origin\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, origin)?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"userContext\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;

        let frame_buf: [1024]u8 = undefined;
        * = _.session.sendCommand("permissions.setPermission", pbuf[0..pos], &frame_buf)?;
    }
}

// ============================================================================
// BidiBrowser — Browser-Level UserContext Registry & Lifecycle
// ============================================================================

pub struct BidiBrowser {
    allocator: std.mem.Allocator = undefined,
    session: *BidiSession = undefined,
    user_contexts: std.ArrayList([]u8) = .empty,
    pub closed: bool = false,

    pub static fn init(alloc: std.mem.Allocator, session: *BidiSession): BidiBrowser {
        return BidiBrowser{
            allocator: alloc,
            session: session,
            user_contexts: .empty,
            closed: false,
        };
    }

    pub fn deinit(*_) {
        for (uc of _.user_contexts.items) {
            _.allocator.free(uc);
        }
        _.user_contexts.deinit(_.allocator);
        _.user_contexts = .empty;
    }

    ~init() {
        _.deinit();
    }

    fn recordUserContext(*_, uc_id: []const u8): void!BidiError {
        for (existing of _.user_contexts.items) {
            if (std.mem.eql(u8, existing, uc_id)) return;
        }
        const duped = _.allocator.dupe(u8, uc_id) catch return BidiError.OutOfMemory;
        _.user_contexts.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return BidiError.OutOfMemory;
        };
    }

    fn forgetUserContext(*_, uc_id: []const u8) {
        let i: usize = 0;
        while (i < _.user_contexts.items.len) {
            if (std.mem.eql(u8, _.user_contexts.items[i], uc_id)) {
                _.allocator.free(_.user_contexts.items[i]);
                * = _.user_contexts.orderedRemove(i);
                return;
            }
            i += 1;
        }
    }

    pub fn userContextCount(): usize {
        return _.user_contexts.items.len;
    }

    pub fn hasUserContext(uc_id: []const u8): bool {
        for (existing of _.user_contexts.items) {
            if (std.mem.eql(u8, existing, uc_id)) return true;
        }
        return false;
    }

    /// Query `browser.getUserContexts` and synchronize local registry.
    pub fn getUserContexts(*_): usize!BidiError {
        if (_.closed) return BidiError.SessionClosed;
        let frame_buf: [512]u8 = undefined;
        const resp = _.session.sendCommand("browser.getUserContexts", "{}", &frame_buf)?;

        const arr = BidiFrameScanner.extractField(resp.result_json, "userContexts", false) orelse return 0;
        let count: usize = 0;
        let scan_pos: usize = 0;
        while (scan_pos < arr.len) {
            const key_pat = "\"userContext\"";
            if (rel := std.mem.indexOf(u8, arr[scan_pos..], key_pat)) {
                const after_key = scan_pos + rel + key_pat.len;
                // Find opening quote of value
                if (q1_rel := BidiFrameScanner.indexOfByte(arr[after_key..], '"')) {
                    const q1 = after_key + q1_rel;
                    if (q2_rel := BidiFrameScanner.indexOfByte(arr[q1 + 1 ..], '"')) {
                        const uc_id = arr[q1 + 1 .. q1 + 1 + q2_rel];
                        _.recordUserContext(uc_id)?;
                        count += 1;
                        scan_pos = q1 + 1 + q2_rel + 1;
                        continue;
                    }
                }
                break;
            } else {
                break;
            }
        }
        return count;
    }

    /// Create an isolated user context via `browser.createUserContext`.
    pub fn createUserContext(*_, params: CreateUserContextParams): BidiUserContext!BidiError {
        if (_.closed) return BidiError.SessionClosed;

        let pbuf: [1024]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"acceptInsecureCerts\":")?;
        pos = bidiWriteStr(&pbuf, pos, if (params.accept_insecure_certs) "true" else "false")?;
        if (params.has_proxy) {
            pos = bidiWriteStr(&pbuf, pos, ",\"proxy\":{\"proxyType\":\"")?;
            pos = bidiWriteStr(&pbuf, pos, proxyTypeName(params.proxy.proxy_type))?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
            if (params.proxy.http_proxy.len > 0) {
                pos = bidiWriteStr(&pbuf, pos, ",\"httpProxy\":\"")?;
                pos = bidiWriteJsonEscaped(&pbuf, pos, params.proxy.http_proxy)?;
                pos = bidiWriteStr(&pbuf, pos, "\"")?;
            }
            if (params.proxy.ssl_proxy.len > 0) {
                pos = bidiWriteStr(&pbuf, pos, ",\"sslProxy\":\"")?;
                pos = bidiWriteJsonEscaped(&pbuf, pos, params.proxy.ssl_proxy)?;
                pos = bidiWriteStr(&pbuf, pos, "\"")?;
            }
            pos = bidiWriteStr(&pbuf, pos, "}")?;
        }
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [1536]u8 = undefined;
        const resp = _.session.sendCommand("browser.createUserContext", pbuf[0..pos], &frame_buf)?;
        const uc_id = BidiFrameScanner.extractField(resp.result_json, "userContext", true) orelse return BidiError.InvalidJson;
        _.recordUserContext(uc_id)?;
        return BidiUserContext.init(_.session, uc_id);
    }

    /// Remove a user context by ID via `browser.removeUserContext`.
    pub fn removeUserContext(*_, uc_id: []const u8): void!BidiError {
        if (_.closed) return BidiError.SessionClosed;
        let pbuf: [256]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"userContext\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, uc_id)?;
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;

        let frame_buf: [512]u8 = undefined;
        * = _.session.sendCommand("browser.removeUserContext", pbuf[0..pos], &frame_buf)?;
        _.forgetUserContext(uc_id);
    }

    /// Configure browser download behavior via `browser.setDownloadBehavior`.
    pub fn setDownloadBehavior(*_, params: DownloadBehaviorParams): void!BidiError {
        if (_.closed) return BidiError.SessionClosed;
        let pbuf: [1024]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"downloadBehavior\":")?;
        switch (params.policy) {
            DownloadBehaviorPolicy.Allowed => {
                pos = bidiWriteStr(&pbuf, pos, "{\"type\":\"allowed\",\"destinationFolder\":\"")?;
                pos = bidiWriteJsonEscaped(&pbuf, pos, params.destination_folder)?;
                pos = bidiWriteStr(&pbuf, pos, "\"}")?;
            },
            DownloadBehaviorPolicy.Denied => {
                pos = bidiWriteStr(&pbuf, pos, "{\"type\":\"denied\"}")?;
            },
            DownloadBehaviorPolicy.Default => {
                pos = bidiWriteStr(&pbuf, pos, "null")?;
            },
        }
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [1536]u8 = undefined;
        * = _.session.sendCommand("browser.setDownloadBehavior", pbuf[0..pos], &frame_buf)?;
    }

    /// Close the browser process via `browser.close`.
    pub fn close(*_): void!BidiError {
        if (_.closed) return;
        let frame_buf: [512]u8 = undefined;
        * = _.session.sendCommand("browser.close", "{}", &frame_buf)?;
        _.closed = true;
    }
}
