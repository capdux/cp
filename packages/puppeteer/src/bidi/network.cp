// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/bidi/core/Request.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/bidi/network.cp — BidiNetworkIntercept, BidiRequest
// & BidiResponse
// Implements network.addIntercept/removeIntercept/continueRequest/
// provideResponse/failRequest/continueResponse/continueWithAuth/getData.
// ============================================================================

const std = @import("std");
import {
    BidiError,
    InterceptPhase,
    UrlPatternType,
    UrlPattern,
    BidiHeader,
    ContinueRequestParams,
    ProvideResponseParams,
    ContinueResponseParams,
    AuthAction,
    ContinueWithAuthParams,
    interceptPhaseName,
    bidiWriteStr,
    bidiWriteJsonEscaped,
} from "./protocol";
import { BidiFrameScanner } from "./connection";
import { BidiSession } from "./session";

// ============================================================================
// Helper: Encode BiDi Header List [{"name":...,"value":{"type":"string","value":...}}]
// ============================================================================

fn writeBidiHeadersJson(buf: []u8, pos: usize, headers: []const BidiHeader): usize!BidiError {
    let p = pos;
    p = bidiWriteStr(buf, p, "[")?;
    let i: usize = 0;
    while (i < headers.len) {
        if (i > 0) p = bidiWriteStr(buf, p, ",")?;
        p = bidiWriteStr(buf, p, "{\"name\":\"")?;
        p = bidiWriteJsonEscaped(buf, p, headers[i].name)?;
        p = bidiWriteStr(buf, p, "\",\"value\":{\"type\":\"string\",\"value\":\"")?;
        p = bidiWriteJsonEscaped(buf, p, headers[i].value)?;
        p = bidiWriteStr(buf, p, "\"}}")?;
        i += 1;
    }
    p = bidiWriteStr(buf, p, "]")?;
    return p;
}

// ============================================================================
// BidiNetworkIntercept — Intercept Rule Manager (addIntercept / removeIntercept)
// ============================================================================

pub struct BidiNetworkIntercept {
    allocator: std.mem.Allocator = undefined,
    session: *BidiSession = undefined,
    active_ids: std.ArrayList([]u8) = .empty,

    pub static fn init(alloc: std.mem.Allocator, session: *BidiSession): BidiNetworkIntercept {
        return BidiNetworkIntercept{
            allocator: alloc,
            session: session,
            active_ids: .empty,
        };
    }

    pub fn deinit(*_) {
        for (id of _.active_ids.items) {
            _.allocator.free(id);
        }
        _.active_ids.deinit(_.allocator);
        _.active_ids = .empty;
    }

    ~init() {
        _.deinit();
    }

    pub fn activeCount(): usize {
        return _.active_ids.items.len;
    }

    pub fn hasIntercept(intercept_id: []const u8): bool {
        for (id of _.active_ids.items) {
            if (std.mem.eql(u8, id, intercept_id)) return true;
        }
        return false;
    }

    /// Register a network intercept rule via `network.addIntercept`.
    /// Returns the allocated intercept ID owned by this manager.
    pub fn addIntercept(
        *_,
        phases: []const InterceptPhase,
        contexts: []const []const u8,
        url_patterns: []const UrlPattern,
    ): ([]const u8)!BidiError {
        if (phases.len == 0) return BidiError.InvalidArgument;

        let pbuf: [2048]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"phases\":[")?;
        let i: usize = 0;
        while (i < phases.len) {
            if (i > 0) pos = bidiWriteStr(&pbuf, pos, ",")?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
            pos = bidiWriteStr(&pbuf, pos, interceptPhaseName(phases[i]))?;
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

        if (url_patterns.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"urlPatterns\":[")?;
            let k: usize = 0;
            while (k < url_patterns.len) {
                if (k > 0) pos = bidiWriteStr(&pbuf, pos, ",")?;
                const pat = url_patterns[k];
                if (pat.pattern_type == UrlPatternType.String) {
                    pos = bidiWriteStr(&pbuf, pos, "{\"type\":\"string\",\"pattern\":\"")?;
                    pos = bidiWriteJsonEscaped(&pbuf, pos, pat.pattern)?;
                    pos = bidiWriteStr(&pbuf, pos, "\"}")?;
                } else {
                    pos = bidiWriteStr(&pbuf, pos, "{\"type\":\"pattern\"")?;
                    if (pat.protocol.len > 0) {
                        pos = bidiWriteStr(&pbuf, pos, ",\"protocol\":\"")?;
                        pos = bidiWriteJsonEscaped(&pbuf, pos, pat.protocol)?;
                        pos = bidiWriteStr(&pbuf, pos, "\"")?;
                    }
                    if (pat.hostname.len > 0) {
                        pos = bidiWriteStr(&pbuf, pos, ",\"hostname\":\"")?;
                        pos = bidiWriteJsonEscaped(&pbuf, pos, pat.hostname)?;
                        pos = bidiWriteStr(&pbuf, pos, "\"")?;
                    }
                    if (pat.pathname.len > 0) {
                        pos = bidiWriteStr(&pbuf, pos, ",\"pathname\":\"")?;
                        pos = bidiWriteJsonEscaped(&pbuf, pos, pat.pathname)?;
                        pos = bidiWriteStr(&pbuf, pos, "\"")?;
                    }
                    pos = bidiWriteStr(&pbuf, pos, "}")?;
                }
                k += 1;
            }
            pos = bidiWriteStr(&pbuf, pos, "]")?;
        }
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [2560]u8 = undefined;
        const resp = _.session.sendCommand("network.addIntercept", pbuf[0..pos], &frame_buf)?;
        const intercept_id = BidiFrameScanner.extractField(resp.result_json, "intercept", true) orelse return BidiError.InvalidJson;

        const duped = _.allocator.dupe(u8, intercept_id) catch return BidiError.OutOfMemory;
        _.active_ids.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return BidiError.OutOfMemory;
        };
        return duped;
    }

    /// Remove a previously registered intercept rule via `network.removeIntercept`.
    pub fn removeIntercept(*_, intercept_id: []const u8): void!BidiError {
        let pbuf: [256]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"intercept\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, intercept_id)?;
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;

        let frame_buf: [512]u8 = undefined;
        * = _.session.sendCommand("network.removeIntercept", pbuf[0..pos], &frame_buf)?;

        let i: usize = 0;
        while (i < _.active_ids.items.len) {
            if (std.mem.eql(u8, _.active_ids.items[i], intercept_id)) {
                _.allocator.free(_.active_ids.items[i]);
                * = _.active_ids.orderedRemove(i);
                return;
            }
            i += 1;
        }
    }
}

// ============================================================================
// BidiRequest — Intercepted Network Request & Fulfillment Controls
// ============================================================================

pub struct BidiRequest {
    session: *BidiSession = undefined,
    request_id_buf: [128]u8 = undefined,
    request_id_len: usize = 0,
    context_id_buf: [128]u8 = undefined,
    context_id_len: usize = 0,
    navigation_id_buf: [128]u8 = undefined,
    navigation_id_len: usize = 0,
    url_buf: [512]u8 = undefined,
    url_len: usize = 0,
    method_buf: [32]u8 = undefined,
    method_len: usize = 0,
    pub is_blocked: bool = false,
    pub redirect_count: u32 = 0,
    pub handled: bool = false,

    pub static fn init(
        session: *BidiSession,
        request_id: []const u8,
        context_id: []const u8,
        req_url: []const u8,
        req_method: []const u8,
        is_blocked: bool,
    ): BidiRequest {
        let r = BidiRequest{
            session: session,
            request_id_len: 0,
            context_id_len: 0,
            navigation_id_len: 0,
            url_len: 0,
            method_len: 0,
            is_blocked: is_blocked,
            redirect_count: 0,
            handled: false,
        };
        const rn = if (request_id.len <= 128) request_id.len else 128;
        if (rn > 0) @memcpy(r.request_id_buf[0..rn], request_id[0..rn]);
        r.request_id_len = rn;
        const cn = if (context_id.len <= 128) context_id.len else 128;
        if (cn > 0) @memcpy(r.context_id_buf[0..cn], context_id[0..cn]);
        r.context_id_len = cn;
        const un = if (req_url.len <= 512) req_url.len else 512;
        if (un > 0) @memcpy(r.url_buf[0..un], req_url[0..un]);
        r.url_len = un;
        const mn = if (req_method.len <= 32) req_method.len else 32;
        if (mn > 0) @memcpy(r.method_buf[0..mn], req_method[0..mn]);
        r.method_len = mn;
        return r;
    }

    /// Construct a BidiRequest from a `network.beforeRequestSent` event payload JSON.
    pub static fn fromEventJson(session: *BidiSession, params_json: []const u8): BidiRequest!BidiError {
        const ctx_id = BidiFrameScanner.extractField(params_json, "context", true) orelse "";
        const blocked_raw = BidiFrameScanner.extractField(params_json, "isBlocked", false) orelse "false";
        const is_blocked = std.mem.eql(u8, blocked_raw, "true");
        const req_obj = BidiFrameScanner.extractField(params_json, "request", false) orelse return BidiError.InvalidJson;
        const req_id = BidiFrameScanner.extractField(req_obj, "request", true) orelse return BidiError.InvalidJson;
        const req_url = BidiFrameScanner.extractField(req_obj, "url", true) orelse "";
        const req_method = BidiFrameScanner.extractField(req_obj, "method", true) orelse "GET";

        let req = BidiRequest.init(session, req_id, ctx_id, req_url, req_method, is_blocked);
        if (nav := BidiFrameScanner.extractField(params_json, "navigation", true)) {
            const nn = if (nav.len <= 128) nav.len else 128;
            if (nn > 0) @memcpy(req.navigation_id_buf[0..nn], nav[0..nn]);
            req.navigation_id_len = nn;
        }
        if (rc_raw := BidiFrameScanner.extractField(params_json, "redirectCount", false)) {
            if (rc := BidiFrameScanner.parseI64(rc_raw)) {
                if (rc >= 0) req.redirect_count = rc as u32;
            }
        }
        return req;
    }

    pub fn id(): []const u8 {
        return _.request_id_buf[0.._.request_id_len];
    }

    pub fn contextId(): []const u8 {
        return _.context_id_buf[0.._.context_id_len];
    }

    pub fn url(): []const u8 {
        return _.url_buf[0.._.url_len];
    }

    pub fn method(): []const u8 {
        return _.method_buf[0.._.method_len];
    }

    /// Continue an intercepted request (optionally overriding URL, method, body, or headers) via `network.continueRequest`.
    pub fn continueRequest(*_, params: ContinueRequestParams): void!BidiError {
        let pbuf: [2048]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"request\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\"")?;
        if (params.url.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"url\":\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, params.url)?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
        }
        if (params.method.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"method\":\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, params.method)?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
        }
        if (params.body.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"body\":{\"type\":\"string\",\"value\":\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, params.body)?;
            pos = bidiWriteStr(&pbuf, pos, "\"}")?;
        }
        if (params.headers.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"headers\":")?;
            pos = writeBidiHeadersJson(&pbuf, pos, params.headers)?;
        }
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [2560]u8 = undefined;
        * = _.session.sendCommand("network.continueRequest", pbuf[0..pos], &frame_buf)?;
        _.handled = true;
    }

    /// Fulfill an intercepted request with a synthetic response via `network.provideResponse`.
    pub fn provideResponse(*_, params: ProvideResponseParams): void!BidiError {
        let pbuf: [2048]u8 = undefined;
        let pos: usize = 0;
        let sbuf: [64]u8 = undefined;
        const status_str = std.fmt.bufPrint(&sbuf, "{d}", .{params.status_code}) catch return BidiError.BufferTooSmall;

        pos = bidiWriteStr(&pbuf, pos, "{\"request\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\",\"statusCode\":")?;
        pos = bidiWriteStr(&pbuf, pos, status_str)?;
        if (params.reason_phrase.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"reasonPhrase\":\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, params.reason_phrase)?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
        }
        if (params.headers.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"headers\":")?;
            pos = writeBidiHeadersJson(&pbuf, pos, params.headers)?;
        }
        if (params.body.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"body\":{\"type\":\"")?;
            pos = bidiWriteStr(&pbuf, pos, if (params.body_base64) "base64" else "string")?;
            pos = bidiWriteStr(&pbuf, pos, "\",\"value\":\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, params.body)?;
            pos = bidiWriteStr(&pbuf, pos, "\"}")?;
        }
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [2560]u8 = undefined;
        * = _.session.sendCommand("network.provideResponse", pbuf[0..pos], &frame_buf)?;
        _.handled = true;
    }

    /// Abort an intercepted request via `network.failRequest`.
    pub fn failRequest(*_): void!BidiError {
        let pbuf: [256]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"request\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        pos = bidiWriteStr(&pbuf, pos, "\"}")?;

        let frame_buf: [512]u8 = undefined;
        * = _.session.sendCommand("network.failRequest", pbuf[0..pos], &frame_buf)?;
        _.handled = true;
    }

    /// Respond to an auth challenge via `network.continueWithAuth`.
    pub fn continueWithAuth(*_, params: ContinueWithAuthParams): void!BidiError {
        let pbuf: [1024]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"request\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.id())?;
        switch (params.action) {
            AuthAction.ProvideCredentials => {
                pos = bidiWriteStr(&pbuf, pos, "\",\"action\":\"provideCredentials\",\"credentials\":{\"type\":\"password\",\"username\":\"")?;
                pos = bidiWriteJsonEscaped(&pbuf, pos, params.username)?;
                pos = bidiWriteStr(&pbuf, pos, "\",\"password\":\"")?;
                pos = bidiWriteJsonEscaped(&pbuf, pos, params.password)?;
                pos = bidiWriteStr(&pbuf, pos, "\"}}")?;
            },
            AuthAction.Cancel => {
                pos = bidiWriteStr(&pbuf, pos, "\",\"action\":\"cancel\"}")?;
            },
            AuthAction.Default => {
                pos = bidiWriteStr(&pbuf, pos, "\",\"action\":\"default\"}")?;
            },
        }
        let frame_buf: [1536]u8 = undefined;
        * = _.session.sendCommand("network.continueWithAuth", pbuf[0..pos], &frame_buf)?;
        _.handled = true;
    }
}

// ============================================================================
// BidiResponse — Network Response Metadata & continueResponse Control
// ============================================================================

pub struct BidiResponse {
    session: *BidiSession = undefined,
    request_id_buf: [128]u8 = undefined,
    request_id_len: usize = 0,
    url_buf: [512]u8 = undefined,
    url_len: usize = 0,
    status_text_buf: [64]u8 = undefined,
    status_text_len: usize = 0,
    protocol_buf: [32]u8 = undefined,
    protocol_len: usize = 0,
    mime_type_buf: [128]u8 = undefined,
    mime_type_len: usize = 0,
    pub status: u16 = 200,
    pub from_cache: bool = false,
    pub bytes_received: u64 = 0,

    pub static fn init(
        session: *BidiSession,
        request_id: []const u8,
        resp_url: []const u8,
        status: u16,
        status_text: []const u8,
    ): BidiResponse {
        let r = BidiResponse{
            session: session,
            request_id_len: 0,
            url_len: 0,
            status_text_len: 0,
            protocol_len: 0,
            mime_type_len: 0,
            status: status,
            from_cache: false,
            bytes_received: 0,
        };
        const rn = if (request_id.len <= 128) request_id.len else 128;
        if (rn > 0) @memcpy(r.request_id_buf[0..rn], request_id[0..rn]);
        r.request_id_len = rn;
        const un = if (resp_url.len <= 512) resp_url.len else 512;
        if (un > 0) @memcpy(r.url_buf[0..un], resp_url[0..un]);
        r.url_len = un;
        const sn = if (status_text.len <= 64) status_text.len else 64;
        if (sn > 0) @memcpy(r.status_text_buf[0..sn], status_text[0..sn]);
        r.status_text_len = sn;
        return r;
    }

    /// Construct a BidiResponse from a `network.responseStarted` / `network.responseCompleted` event JSON.
    pub static fn fromEventJson(session: *BidiSession, params_json: []const u8): BidiResponse!BidiError {
        const req_obj = BidiFrameScanner.extractField(params_json, "request", false) orelse return BidiError.InvalidJson;
        const req_id = BidiFrameScanner.extractField(req_obj, "request", true) orelse return BidiError.InvalidJson;
        const resp_obj = BidiFrameScanner.extractField(params_json, "response", false) orelse return BidiError.InvalidJson;
        const resp_url = BidiFrameScanner.extractField(resp_obj, "url", true) orelse "";
        const status_text = BidiFrameScanner.extractField(resp_obj, "statusText", true) orelse "";

        let status: u16 = 200;
        if (st_raw := BidiFrameScanner.extractField(resp_obj, "status", false)) {
            if (st := BidiFrameScanner.parseI64(st_raw)) {
                if (st >= 0 && st <= 65535) status = st as u16;
            }
        }

        let resp = BidiResponse.init(session, req_id, resp_url, status, status_text);
        if (fc := BidiFrameScanner.extractField(resp_obj, "fromCache", false)) {
            resp.from_cache = std.mem.eql(u8, fc, "true");
        }
        if (br := BidiFrameScanner.extractField(resp_obj, "bytesReceived", false)) {
            if (v := BidiFrameScanner.parseI64(br)) {
                if (v >= 0) resp.bytes_received = v as u64;
            }
        }
        if (proto := BidiFrameScanner.extractField(resp_obj, "protocol", true)) {
            const pn = if (proto.len <= 32) proto.len else 32;
            if (pn > 0) @memcpy(resp.protocol_buf[0..pn], proto[0..pn]);
            resp.protocol_len = pn;
        }
        if (mime := BidiFrameScanner.extractField(resp_obj, "mimeType", true)) {
            const mn = if (mime.len <= 128) mime.len else 128;
            if (mn > 0) @memcpy(resp.mime_type_buf[0..mn], mime[0..mn]);
            resp.mime_type_len = mn;
        }
        return resp;
    }

    pub fn requestId(): []const u8 {
        return _.request_id_buf[0.._.request_id_len];
    }

    pub fn url(): []const u8 {
        return _.url_buf[0.._.url_len];
    }

    pub fn statusText(): []const u8 {
        return _.status_text_buf[0.._.status_text_len];
    }

    pub fn ok(): bool {
        return _.status >= 200 && _.status < 300;
    }

    /// Continue a blocked response (optionally overriding status/headers) via `network.continueResponse`.
    pub fn continueResponse(*_, params: ContinueResponseParams): void!BidiError {
        let pbuf: [2048]u8 = undefined;
        let pos: usize = 0;
        pos = bidiWriteStr(&pbuf, pos, "{\"request\":\"")?;
        pos = bidiWriteJsonEscaped(&pbuf, pos, _.requestId())?;
        pos = bidiWriteStr(&pbuf, pos, "\"")?;
        if (params.status_code > 0) {
            let sbuf: [64]u8 = undefined;
            const ss = std.fmt.bufPrint(&sbuf, ",\"statusCode\":{d}", .{params.status_code}) catch return BidiError.BufferTooSmall;
            pos = bidiWriteStr(&pbuf, pos, ss)?;
        }
        if (params.reason_phrase.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"reasonPhrase\":\"")?;
            pos = bidiWriteJsonEscaped(&pbuf, pos, params.reason_phrase)?;
            pos = bidiWriteStr(&pbuf, pos, "\"")?;
        }
        if (params.headers.len > 0) {
            pos = bidiWriteStr(&pbuf, pos, ",\"headers\":")?;
            pos = writeBidiHeadersJson(&pbuf, pos, params.headers)?;
        }
        pos = bidiWriteStr(&pbuf, pos, "}")?;

        let frame_buf: [2560]u8 = undefined;
        * = _.session.sendCommand("network.continueResponse", pbuf[0..pos], &frame_buf)?;
    }
}
