// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/chromium/crNetworkManager.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/cr_network_manager.cp — Chromium CDP Network Manager
// Handles Network.requestWillBeSent, Fetch.requestPaused, Fetch.authRequired,
// Network.responseReceived, Network.loadingFinished, Network.loadingFailed.
// Supports UrlMatcher route table matching, Fetch.fulfillRequest (Base64 body),
// Fetch.continueRequest (header/method/URL overrides), redirect chains, and
// Fetch.continueWithAuth automatic challenge response.
// ============================================================================

const std = @import("std");
import {
    CdpEndpoint,
    CdpConnectionState,
    CdpFrameScanner,
} from "cap:cdp";
import {
    PlaywrightError,
    Route,
    RouteAction,
    RouteEntry,
    RouteHandlerFn,
    Request,
    Response,
    ResourceTiming,
    UrlMatcher,
    NameValue,
    parseHeadersJsonArray,
} from "./network";
import { HTTPCredentials } from "./context";

type CrNetVec16 = @Vector(16, u8);

/// 16-lane SIMD scanner: returns index of first JSON-escapable byte or null.
pub fn simdScanNetEscape(data: []const u8): usize? {
    let i: usize = 0;
    const v_quote: CrNetVec16 = @splat('"');
    const v_bslash: CrNetVec16 = @splat('\\');
    const v_ctrl: CrNetVec16 = @splat(0x20);
    while (i + 16 <= data.len) : (i += 16) {
        const chunk: CrNetVec16 = data[i..][0..16].*;
        const mask = (chunk == v_quote) | (chunk == v_bslash) | (chunk < v_ctrl);
        if (@reduce(.Or, mask)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                const b = data[i + k];
                if (b == '"' || b == '\\' || b < 0x20) return i + k;
            }
        }
    }
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b == '"' || b == '\\' || b < 0x20) return i;
    }
    return null;
}

/// Encode raw bytes to standard Base64 using caller-provided allocator.
pub fn encodeBase64Alloc(alloc: std.mem.Allocator, raw: []const u8): ([]u8)!PlaywrightError {
    const out_len = std.base64.standard.Encoder.calcSize(raw.len);
    const buf = alloc.alloc(u8, out_len) catch return PlaywrightError.OutOfMemory;
    const encoded = std.base64.standard.Encoder.encode(buf, raw);
    if (encoded.len == 0 && out_len > 0) return buf;
    return buf;
}

/// Map Playwright abort error code string to Chromium CDP Network.ErrorReason.
pub fn mapErrorReasonToCdp(code: []const u8): []const u8 {
    if (std.ascii.eqlIgnoreCase(code, "aborted")) return "Aborted";
    if (std.ascii.eqlIgnoreCase(code, "accessdenied")) return "AccessDenied";
    if (std.ascii.eqlIgnoreCase(code, "addressunreachable")) return "AddressUnreachable";
    if (std.ascii.eqlIgnoreCase(code, "blockedbyclient")) return "BlockedByClient";
    if (std.ascii.eqlIgnoreCase(code, "blockedbyresponse")) return "BlockedByResponse";
    if (std.ascii.eqlIgnoreCase(code, "connectionaborted")) return "ConnectionAborted";
    if (std.ascii.eqlIgnoreCase(code, "connectionclosed")) return "ConnectionClosed";
    if (std.ascii.eqlIgnoreCase(code, "connectionfailed")) return "ConnectionFailed";
    if (std.ascii.eqlIgnoreCase(code, "connectionrefused")) return "ConnectionRefused";
    if (std.ascii.eqlIgnoreCase(code, "connectionreset")) return "ConnectionReset";
    if (std.ascii.eqlIgnoreCase(code, "internetdisconnected")) return "InternetDisconnected";
    if (std.ascii.eqlIgnoreCase(code, "namenotresolved")) return "NameNotResolved";
    if (std.ascii.eqlIgnoreCase(code, "timedout")) return "TimedOut";
    return "Failed";
}

pub enum CrNetworkRequestState {
    Pending,
    Paused,
    Continued,
    Fulfilled,
    Aborted,
    ResponseReceived,
    Finished,
    Failed,
}

pub enum CrAuthResponseAction {
    Default,
    CancelAuth,
    ProvideCredentials,

    pub fn asCdpString(): []const u8 {
        return switch (_) {
            .Default => "Default",
            .CancelAuth => "CancelAuth",
            .ProvideCredentials => "ProvideCredentials",
        };
    }
}

pub struct CrAuthChallenge {
    pub source: []const u8 = "Server",
    pub origin: []const u8 = "",
    pub scheme: []const u8 = "Basic",
    pub realm: []const u8 = "",
}

pub struct CrManagedRequest {
    pub request_id: []const u8 = "",
    pub interception_id: []const u8 = "",
    pub frame_id: []const u8 = "",
    pub url: []const u8 = "",
    pub method: []const u8 = "GET",
    pub resource_type: []const u8 = "Other",
    pub headers_json: []const u8 = "{}",
    pub post_data: []const u8 = "",
    pub status: i32 = 0,
    pub status_text: []const u8 = "",
    pub mime_type: []const u8 = "",
    pub response_headers_json: []const u8 = "{}",
    pub encoded_data_length: i64 = 0,
    pub error_text: []const u8 = "",
    pub state: CrNetworkRequestState = CrNetworkRequestState.Pending,
    pub redirected_from_id: []const u8 = "",
    pub redirected_to_id: []const u8 = "",
    pub redirect_count: u32 = 0,
    pub timing: ResourceTiming = .{},
}

pub struct CrNetworkManager {
    arena: std.heap.ArenaAllocator = undefined,
    allocator: std.mem.Allocator = undefined,
    deinitialized: bool = false,

    pub endpoint: (*CdpEndpoint)? = null,
    pub session_id: []const u8 = "",
    pub routes: std.ArrayList(RouteEntry) = .empty,
    pub requests: std.ArrayList(CrManagedRequest) = .empty,
    pub credentials: HTTPCredentials? = null,
    pub interception_enabled: bool = false,

    pub intercepted_count: usize = 0,
    pub fulfilled_count: usize = 0,
    pub continued_count: usize = 0,
    pub aborted_count: usize = 0,
    pub auth_challenges_handled: usize = 0,
    pub last_dispatched_method: []const u8 = "",
    pub last_dispatched_params: []const u8 = "",

    pub static fn init(alloc: std.mem.Allocator): CrNetworkManager {
        return CrNetworkManager{
            arena: std.heap.ArenaAllocator.init(alloc),
            allocator: alloc,
            deinitialized: false,
            endpoint: null,
            session_id: "",
            routes: .empty,
            requests: .empty,
            credentials: null,
            interception_enabled: false,
            intercepted_count: 0,
            fulfilled_count: 0,
            continued_count: 0,
            aborted_count: 0,
            auth_challenges_handled: 0,
            last_dispatched_method: "",
            last_dispatched_params: "",
        };
    }

    pub static fn initWithEndpoint(
        alloc: std.mem.Allocator,
        endpoint: *CdpEndpoint,
        session_id: []const u8,
    ): CrNetworkManager {
        let mgr = CrNetworkManager.init(alloc);
        mgr.endpoint = endpoint;
        mgr.session_id = session_id;
        return mgr;
    }

    pub fn deinit(*_) {
        if (_.deinitialized) return;
        _.deinitialized = true;
        _.routes.deinit(_.allocator);
        _.routes = .empty;
        _.requests.deinit(_.allocator);
        _.requests = .empty;
        _.arena.deinit();
    }

    ~init() {
        _.deinit();
    }

    fn intern(*_, s: []const u8): ([]const u8)!PlaywrightError {
        if (s.len == 0) return "";
        const aa = _.arena.allocator();
        return aa.dupe(u8, s) catch return PlaywrightError.OutOfMemory;
    }

    fn dispatchCdp(*_, method: []const u8, params_json: []const u8): void!PlaywrightError {
        _.last_dispatched_method = _.intern(method)?;
        _.last_dispatched_params = _.intern(params_json)?;
        if (ep := _.endpoint) {
            if (ep.state == CdpConnectionState.Connected) {
                let frame_buf: [8192]u8 = undefined;
                if (params_json.len + 256 <= frame_buf.len) {
                    const cmd_id = ep.sendCommand(_.session_id, method, params_json, &frame_buf) catch return PlaywrightError.ProtocolError;
                    if (ep.takeResponse(cmd_id) != null) {}
                } else {
                    const heap_buf = _.allocator.alloc(u8, params_json.len + 512) catch return PlaywrightError.OutOfMemory;
                    defer _.allocator.free(heap_buf);
                    const cmd_id = ep.sendCommand(_.session_id, method, params_json, heap_buf) catch return PlaywrightError.ProtocolError;
                    if (ep.takeResponse(cmd_id) != null) {}
                }
            }
        }
    }

    pub fn enableInterception(*_): void!PlaywrightError {
        _.interception_enabled = true;
        _.dispatchCdp("Network.enable", "{}")?;
        _.dispatchCdp("Fetch.enable", "{\"patterns\":[{\"urlPattern\":\"*\"}],\"handleAuthRequests\":true}")?;
    }

    pub fn addRoute(*_, pattern: []const u8, handler: RouteHandlerFn, ctx: usize): void!PlaywrightError {
        const pat = _.intern(pattern)?;
        _.routes.append(_.allocator, RouteEntry{
            pattern: pat,
            handler: handler,
            ctx: ctx,
            times: -1,
        }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn removeRoute(*_, pattern: []const u8) {
        let i: usize = 0;
        while (i < _.routes.items.len) {
            if (std.mem.eql(u8, _.routes.items[i].pattern, pattern)) {
                const removed = _.routes.orderedRemove(i);
                if (removed.pattern.len == 0) {}
            } else {
                i += 1;
            }
        }
    }

    pub fn setHttpCredentials(*_, creds: HTTPCredentials?): void!PlaywrightError {
        if (c := creds) {
            _.credentials = HTTPCredentials{
                username: _.intern(c.username)?,
                password: _.intern(c.password)?,
                origin: _.intern(c.origin)?,
            };
        } else {
            _.credentials = null;
        }
    }

    fn findLatestRequestIndex(request_id: []const u8): usize? {
        let i: usize = _.requests.items.len;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.requests.items[i].request_id, request_id)) {
                return i;
            }
        }
        return null;
    }

    pub fn findRequest(*_, request_id: []const u8): (*CrManagedRequest)? {
        const idx = _.findLatestRequestIndex(request_id) orelse return null;
        return &_.requests.items[idx];
    }

    pub fn requestCount(): usize {
        return _.requests.items.len;
    }

    /// Handle `Network.requestWillBeSent` (including redirect chain linking).
    pub fn onRequestWillBeSent(
        *_,
        request_id: []const u8,
        frame_id: []const u8,
        url: []const u8,
        method: []const u8,
        resource_type: []const u8,
        headers_json: []const u8,
        post_data: []const u8,
        redirect_status: i32,
    ): (*CrManagedRequest)!PlaywrightError {
        let redirected_from_id: []const u8 = "";
        let redirect_count: u32 = 0;

        if (prev_idx := _.findLatestRequestIndex(request_id)) {
            // Existing request with same requestId -> Chromium redirect hop!
            let hop_buf: [160]u8 = undefined;
            const prev_hop_id = std.fmt.bufPrint(
                &hop_buf,
                "{s}#r{d}",
                .{ request_id, _.requests.items[prev_idx].redirect_count },
            ) catch return PlaywrightError.BufferTooSmall;
            const interned_prev_id = _.intern(prev_hop_id)?;

            _.requests.items[prev_idx].request_id = interned_prev_id;
            _.requests.items[prev_idx].status = if (redirect_status > 0) redirect_status else 302;
            _.requests.items[prev_idx].state = CrNetworkRequestState.Finished;
            _.requests.items[prev_idx].redirected_to_id = _.intern(request_id)?;
            redirected_from_id = interned_prev_id;
            redirect_count = _.requests.items[prev_idx].redirect_count + 1;
        }

        const rid = _.intern(request_id)?;
        const fid = _.intern(frame_id)?;
        const u = _.intern(url)?;
        const m = _.intern(if (method.len > 0) method else "GET")?;
        const rt = _.intern(if (resource_type.len > 0) resource_type else "Other")?;
        const hj = _.intern(if (headers_json.len > 0) headers_json else "{}")?;
        const pd = _.intern(post_data)?;

        _.requests.append(_.allocator, CrManagedRequest{
            request_id: rid,
            interception_id: "",
            frame_id: fid,
            url: u,
            method: m,
            resource_type: rt,
            headers_json: hj,
            post_data: pd,
            status: 0,
            status_text: "",
            mime_type: "",
            response_headers_json: "{}",
            encoded_data_length: 0,
            error_text: "",
            state: CrNetworkRequestState.Pending,
            redirected_from_id: redirected_from_id,
            redirected_to_id: "",
            redirect_count: redirect_count,
            timing: .{},
        }) catch return PlaywrightError.OutOfMemory;
        return &_.requests.items[_.requests.items.len - 1];
    }

    /// Append JSON-escaped string into ArrayList using 16-lane SIMD fast-path.
    static fn appendJsonEscaped(out: *std.ArrayList(u8), alloc: std.mem.Allocator, raw: []const u8): void!PlaywrightError {
        let pos: usize = 0;
        while (pos < raw.len) {
            const rest = raw[pos..];
            const hit = simdScanNetEscape(rest);
            if (hit == null) {
                out.appendSlice(alloc, rest) catch return PlaywrightError.OutOfMemory;
                return;
            }
            const idx = hit.?;
            if (idx > 0) {
                out.appendSlice(alloc, rest[0..idx]) catch return PlaywrightError.OutOfMemory;
            }
            const b = rest[idx];
            if (b == '"') {
                out.appendSlice(alloc, "\\\"") catch return PlaywrightError.OutOfMemory;
            } else if (b == '\\') {
                out.appendSlice(alloc, "\\\\") catch return PlaywrightError.OutOfMemory;
            } else if (b == '\n') {
                out.appendSlice(alloc, "\\n") catch return PlaywrightError.OutOfMemory;
            } else if (b == '\r') {
                out.appendSlice(alloc, "\\r") catch return PlaywrightError.OutOfMemory;
            } else if (b == '\t') {
                out.appendSlice(alloc, "\\t") catch return PlaywrightError.OutOfMemory;
            } else {
                let hex_buf: [6]u8 = undefined;
                const hex = std.fmt.bufPrint(&hex_buf, "\\u{x:0>4}", .{b}) catch return PlaywrightError.BufferTooSmall;
                out.appendSlice(alloc, hex) catch return PlaywrightError.OutOfMemory;
            }
            pos += idx + 1;
        }
    }

    static fn appendCdpHeadersArray(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        headers_json: []const u8,
        extra_content_type: []const u8,
    ): void!PlaywrightError {
        out.append(alloc, '[') catch return PlaywrightError.OutOfMemory;
        let first = true;
        let has_ct = false;

        if (headers_json.len > 2) {
            const pairs = parseHeadersJsonArray(alloc, headers_json)?;
            defer alloc.free(pairs);
            for (p of pairs) {
                if (std.ascii.eqlIgnoreCase(p.name, "content-type")) has_ct = true;
                if (!first) {
                    out.append(alloc, ',') catch return PlaywrightError.OutOfMemory;
                }
                first = false;
                out.appendSlice(alloc, "{\"name\":\"") catch return PlaywrightError.OutOfMemory;
                CrNetworkManager.appendJsonEscaped(out, alloc, p.name)?;
                out.appendSlice(alloc, "\",\"value\":\"") catch return PlaywrightError.OutOfMemory;
                CrNetworkManager.appendJsonEscaped(out, alloc, p.value)?;
                out.appendSlice(alloc, "\"}") catch return PlaywrightError.OutOfMemory;
            }
        }
        if (extra_content_type.len > 0 && !has_ct) {
            if (!first) {
                out.append(alloc, ',') catch return PlaywrightError.OutOfMemory;
            }
            out.appendSlice(alloc, "{\"name\":\"Content-Type\",\"value\":\"") catch return PlaywrightError.OutOfMemory;
            CrNetworkManager.appendJsonEscaped(out, alloc, extra_content_type)?;
            out.appendSlice(alloc, "\"}") catch return PlaywrightError.OutOfMemory;
        }
        out.append(alloc, ']') catch return PlaywrightError.OutOfMemory;
    }

    /// Handle `Fetch.requestPaused`: match UrlMatcher route table (LIFO), invoke
    /// handler, and dispatch `Fetch.fulfillRequest` (Base64 body),
    /// `Fetch.continueRequest` (overrides), or `Fetch.failRequest`.
    pub fn onFetchRequestPaused(
        *_,
        interception_id: []const u8,
        request_id: []const u8,
        url: []const u8,
        method: []const u8,
        resource_type: []const u8,
        headers_json: []const u8,
        post_data: []const u8,
    ): RouteAction!PlaywrightError {
        _.intercepted_count += 1;

        const req_idx = _.findLatestRequestIndex(request_id);
        if (idx := req_idx) {
            _.requests.items[idx].interception_id = _.intern(interception_id)?;
            _.requests.items[idx].state = CrNetworkRequestState.Paused;
        }

        let route = Route{
            request: Request{
                url: url,
                method: if (method.len > 0) method else "GET",
                resource_type: if (resource_type.len > 0) resource_type else "Other",
                post_data: post_data,
                headers_json: if (headers_json.len > 0) headers_json else "{}",
                is_navigation_request: std.ascii.eqlIgnoreCase(resource_type, "Document"),
                frame_id: "",
            },
            action: RouteAction.Pending,
        };

        // Match route table LIFO (most recently registered route wins, fallback cascades)
        let ri: usize = _.routes.items.len;
        while (ri > 0) {
            ri -= 1;
            const entry = _.routes.items[ri];
            if (UrlMatcher.matches(entry.pattern, url)) {
                route.action = RouteAction.Pending;
                entry.handler(entry.ctx, &route);
                if (route.action != RouteAction.Fallback && route.action != RouteAction.Pending) {
                    break;
                }
            }
        }

        let tmp_arena = std.heap.ArenaAllocator.init(_.allocator);
        defer tmp_arena.deinit();
        const ta = tmp_arena.allocator();

        let params: std.ArrayList(u8) = .empty;
        if (route.action == RouteAction.Fulfilled) {
            _.fulfilled_count += 1;
            const b64_body = encodeBase64Alloc(ta, route.fulfill_body)?;

            params.appendSlice(ta, "{\"requestId\":\"") catch return PlaywrightError.OutOfMemory;
            CrNetworkManager.appendJsonEscaped(&params, ta, interception_id)?;
            let code_buf: [128]u8 = undefined;
            const code_str = std.fmt.bufPrint(&code_buf, "\",\"responseCode\":{d},\"responseHeaders\":", .{route.fulfill_status}) catch return PlaywrightError.BufferTooSmall;
            params.appendSlice(ta, code_str) catch return PlaywrightError.OutOfMemory;
            CrNetworkManager.appendCdpHeadersArray(&params, ta, route.fulfill_headers_json, route.fulfill_content_type)?;
            params.appendSlice(ta, ",\"body\":\"") catch return PlaywrightError.OutOfMemory;
            params.appendSlice(ta, b64_body) catch return PlaywrightError.OutOfMemory;
            params.appendSlice(ta, "\"}") catch return PlaywrightError.OutOfMemory;

            _.dispatchCdp("Fetch.fulfillRequest", params.items)?;
            if (idx := req_idx) {
                _.requests.items[idx].status = route.fulfill_status;
                _.requests.items[idx].state = CrNetworkRequestState.Fulfilled;
            }
            return RouteAction.Fulfilled;
        }

        if (route.action == RouteAction.Aborted) {
            _.aborted_count += 1;
            const reason = mapErrorReasonToCdp(route.abort_error_code);
            let fail_buf: [512]u8 = undefined;
            const fail_params = std.fmt.bufPrint(
                &fail_buf,
                "{{\"requestId\":\"{s}\",\"errorReason\":\"{s}\"}}",
                .{ interception_id, reason },
            ) catch return PlaywrightError.BufferTooSmall;
            _.dispatchCdp("Fetch.failRequest", fail_params)?;
            if (idx := req_idx) {
                _.requests.items[idx].error_text = _.intern(reason)?;
                _.requests.items[idx].state = CrNetworkRequestState.Aborted;
            }
            return RouteAction.Aborted;
        }

        // Continued (with optional URL/method/headers/postData overrides) or unmatched default continue
        _.continued_count += 1;
        params.appendSlice(ta, "{\"requestId\":\"") catch return PlaywrightError.OutOfMemory;
        CrNetworkManager.appendJsonEscaped(&params, ta, interception_id)?;
        params.append(ta, '"') catch return PlaywrightError.OutOfMemory;

        if (route.override_url.len > 0) {
            params.appendSlice(ta, ",\"url\":\"") catch return PlaywrightError.OutOfMemory;
            CrNetworkManager.appendJsonEscaped(&params, ta, route.override_url)?;
            params.append(ta, '"') catch return PlaywrightError.OutOfMemory;
        }
        if (route.override_method.len > 0) {
            params.appendSlice(ta, ",\"method\":\"") catch return PlaywrightError.OutOfMemory;
            CrNetworkManager.appendJsonEscaped(&params, ta, route.override_method)?;
            params.append(ta, '"') catch return PlaywrightError.OutOfMemory;
        }
        if (route.override_headers_json.len > 2) {
            params.appendSlice(ta, ",\"headers\":") catch return PlaywrightError.OutOfMemory;
            CrNetworkManager.appendCdpHeadersArray(&params, ta, route.override_headers_json, "")?;
        }
        if (route.override_post_data.len > 0) {
            const b64_post = encodeBase64Alloc(ta, route.override_post_data)?;
            params.appendSlice(ta, ",\"postData\":\"") catch return PlaywrightError.OutOfMemory;
            params.appendSlice(ta, b64_post) catch return PlaywrightError.OutOfMemory;
            params.append(ta, '"') catch return PlaywrightError.OutOfMemory;
        }
        params.append(ta, '}') catch return PlaywrightError.OutOfMemory;

        _.dispatchCdp("Fetch.continueRequest", params.items)?;
        if (idx := req_idx) {
            _.requests.items[idx].state = CrNetworkRequestState.Continued;
        }
        return RouteAction.Continued;
    }

    /// Handle `Fetch.authRequired`: automatically answer HTTP Basic/Digest/Proxy
    /// challenges using configured `HTTPCredentials`, or cancel when unmatched.
    pub fn onFetchAuthRequired(
        *_,
        interception_id: []const u8,
        challenge: CrAuthChallenge,
    ): CrAuthResponseAction!PlaywrightError {
        _.auth_challenges_handled += 1;

        let action = CrAuthResponseAction.CancelAuth;
        let user: []const u8 = "";
        let pass: []const u8 = "";

        if (creds := _.credentials) {
            const origin_ok = creds.origin.len == 0 ||
                std.ascii.eqlIgnoreCase(creds.origin, challenge.origin) ||
                UrlMatcher.matches(creds.origin, challenge.origin);
            if (origin_ok) {
                action = CrAuthResponseAction.ProvideCredentials;
                user = creds.username;
                pass = creds.password;
            }
        }

        let buf: [1024]u8 = undefined;
        let params: []const u8 = "";
        if (action == CrAuthResponseAction.ProvideCredentials) {
            params = std.fmt.bufPrint(
                &buf,
                "{{\"requestId\":\"{s}\",\"authChallengeResponse\":{{\"response\":\"ProvideCredentials\",\"username\":\"{s}\",\"password\":\"{s}\"}}}}",
                .{ interception_id, user, pass },
            ) catch return PlaywrightError.BufferTooSmall;
        } else {
            params = std.fmt.bufPrint(
                &buf,
                "{{\"requestId\":\"{s}\",\"authChallengeResponse\":{{\"response\":\"{s}\"}}}}",
                .{ interception_id, action.asCdpString() },
            ) catch return PlaywrightError.BufferTooSmall;
        }

        _.dispatchCdp("Fetch.continueWithAuth", params)?;
        return action;
    }

    /// Handle `Network.responseReceived`.
    pub fn onResponseReceived(
        *_,
        request_id: []const u8,
        status: i32,
        status_text: []const u8,
        mime_type: []const u8,
        response_headers_json: []const u8,
        timing: ResourceTiming,
    ): bool!PlaywrightError {
        const idx = _.findLatestRequestIndex(request_id) orelse return false;
        let r = &_.requests.items[idx];
        r.status = status;
        r.status_text = _.intern(status_text)?;
        r.mime_type = _.intern(mime_type)?;
        r.response_headers_json = _.intern(if (response_headers_json.len > 0) response_headers_json else "{}")?;
        r.timing = timing;
        r.state = CrNetworkRequestState.ResponseReceived;
        return true;
    }

    /// Handle `Network.loadingFinished`.
    pub fn onLoadingFinished(*_, request_id: []const u8, encoded_data_length: i64): bool {
        const idx = _.findLatestRequestIndex(request_id) orelse return false;
        _.requests.items[idx].encoded_data_length = encoded_data_length;
        _.requests.items[idx].state = CrNetworkRequestState.Finished;
        return true;
    }

    /// Handle `Network.loadingFailed`.
    pub fn onLoadingFailed(*_, request_id: []const u8, error_text: []const u8, canceled: bool): bool!PlaywrightError {
        const idx = _.findLatestRequestIndex(request_id) orelse return false;
        const msg = if (error_text.len > 0) error_text else (if (canceled) "net::ERR_ABORTED" else "net::ERR_FAILED");
        _.requests.items[idx].error_text = _.intern(msg)?;
        _.requests.items[idx].state = CrNetworkRequestState.Failed;
        return true;
    }

    /// Dispatch a raw CDP Network / Fetch event JSON payload.
    pub fn handleCdpEvent(*_, method: []const u8, params_json: []const u8): bool!PlaywrightError {
        if (std.mem.eql(u8, method, "Network.requestWillBeSent")) {
            const rid = CdpFrameScanner.extractTopLevelField(params_json, "requestId", true) orelse return false;
            const fid = CdpFrameScanner.extractTopLevelField(params_json, "frameId", true) orelse "";
            const rtype = CdpFrameScanner.extractTopLevelField(params_json, "type", true) orelse "Other";
            const req_obj = CdpFrameScanner.extractTopLevelField(params_json, "request", false) orelse "{}";
            const url = CdpFrameScanner.extractTopLevelField(req_obj, "url", true) orelse "";
            const http_method = CdpFrameScanner.extractTopLevelField(req_obj, "method", true) orelse "GET";
            const headers_json = CdpFrameScanner.extractTopLevelField(req_obj, "headers", false) orelse "{}";
            const post_data = CdpFrameScanner.extractTopLevelField(req_obj, "postData", true) orelse "";
            let redir_status: i32 = 0;
            if (redir_obj := CdpFrameScanner.extractTopLevelField(params_json, "redirectResponse", false)) {
                if (st_str := CdpFrameScanner.extractTopLevelField(redir_obj, "status", false)) {
                    redir_status = std.fmt.parseInt(i32, st_str, 10) catch 302;
                }
            }
            const req_ptr = _.onRequestWillBeSent(rid, fid, url, http_method, rtype, headers_json, post_data, redir_status)?;
            return req_ptr.request_id.len > 0;
        }
        if (std.mem.eql(u8, method, "Fetch.requestPaused")) {
            const iid = CdpFrameScanner.extractTopLevelField(params_json, "requestId", true) orelse return false;
            const net_id = CdpFrameScanner.extractTopLevelField(params_json, "networkId", true) orelse iid;
            const rtype = CdpFrameScanner.extractTopLevelField(params_json, "resourceType", true) orelse "Other";
            const req_obj = CdpFrameScanner.extractTopLevelField(params_json, "request", false) orelse "{}";
            const url = CdpFrameScanner.extractTopLevelField(req_obj, "url", true) orelse "";
            const http_method = CdpFrameScanner.extractTopLevelField(req_obj, "method", true) orelse "GET";
            const headers_json = CdpFrameScanner.extractTopLevelField(req_obj, "headers", false) orelse "{}";
            const post_data = CdpFrameScanner.extractTopLevelField(req_obj, "postData", true) orelse "";
            const act = _.onFetchRequestPaused(iid, net_id, url, http_method, rtype, headers_json, post_data)?;
            return act != RouteAction.Pending;
        }
        if (std.mem.eql(u8, method, "Fetch.authRequired")) {
            const iid = CdpFrameScanner.extractTopLevelField(params_json, "requestId", true) orelse return false;
            const ch_obj = CdpFrameScanner.extractTopLevelField(params_json, "authChallenge", false) orelse "{}";
            const ch = CrAuthChallenge{
                source: CdpFrameScanner.extractTopLevelField(ch_obj, "source", true) orelse "Server",
                origin: CdpFrameScanner.extractTopLevelField(ch_obj, "origin", true) orelse "",
                scheme: CdpFrameScanner.extractTopLevelField(ch_obj, "scheme", true) orelse "Basic",
                realm: CdpFrameScanner.extractTopLevelField(ch_obj, "realm", true) orelse "",
            };
            const auth_act = _.onFetchAuthRequired(iid, ch)?;
            return auth_act.asCdpString().len > 0;
        }
        if (std.mem.eql(u8, method, "Network.responseReceived")) {
            const rid = CdpFrameScanner.extractTopLevelField(params_json, "requestId", true) orelse return false;
            const resp_obj = CdpFrameScanner.extractTopLevelField(params_json, "response", false) orelse "{}";
            const status_s = CdpFrameScanner.extractTopLevelField(resp_obj, "status", false) orelse "200";
            const status = std.fmt.parseInt(i32, status_s, 10) catch 200;
            const status_text = CdpFrameScanner.extractTopLevelField(resp_obj, "statusText", true) orelse "OK";
            const mime_type = CdpFrameScanner.extractTopLevelField(resp_obj, "mimeType", true) orelse "application/octet-stream";
            const headers_json = CdpFrameScanner.extractTopLevelField(resp_obj, "headers", false) orelse "{}";
            return _.onResponseReceived(rid, status, status_text, mime_type, headers_json, .{});
        }
        if (std.mem.eql(u8, method, "Network.loadingFinished")) {
            const rid = CdpFrameScanner.extractTopLevelField(params_json, "requestId", true) orelse return false;
            const len_s = CdpFrameScanner.extractTopLevelField(params_json, "encodedDataLength", false) orelse "0";
            const encoded_len = std.fmt.parseInt(i64, len_s, 10) catch 0;
            return _.onLoadingFinished(rid, encoded_len);
        }
        if (std.mem.eql(u8, method, "Network.loadingFailed")) {
            const rid = CdpFrameScanner.extractTopLevelField(params_json, "requestId", true) orelse return false;
            const err_text = CdpFrameScanner.extractTopLevelField(params_json, "errorText", true) orelse "net::ERR_FAILED";
            const canceled_s = CdpFrameScanner.extractTopLevelField(params_json, "canceled", false) orelse "false";
            const canceled = std.mem.eql(u8, canceled_s, "true");
            return _.onLoadingFailed(rid, err_text, canceled);
        }
        return false;
    }
}
