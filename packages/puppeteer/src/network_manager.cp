// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/NetworkManager.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/network_manager.cp — CDP NetworkManager State Machine
// Full request/response lifecycle correlation, redirect chains, multi-interceptor
// priority arbitration, Fetch auth, offline/cache & extra HTTP headers.
// ============================================================================

const std = @import("std");
import {
    CdpEvent,
    CdpFrameScanner,
} from "cap:cdp";
import {
    PuppeteerError,
    ResourceType,
    HttpHeader,
    RemoteAddress,
    NetworkConditions,
    Credentials,
    RequestInitiator,
    ContinueRequestOverrides,
} from "./types";
import { CDPSession } from "./cdp_session";
import {
    InterceptResolutionAction,
    InterceptResolutionState,
    HTTPRequest,
    HTTPResponse,
} from "./network";
import {
    EventHandlerFn,
    PuppeteerEventEmitter,
} from "./events";
import { SecurityDetails } from "./extras";

type NetVec16 = @Vector(16, u8);

pub struct NetworkSimdScanner {
    /// 16-lane SIMD scan for a single ASCII byte.
    pub static fn indexOfByteSimd(haystack: []const u8, needle: u8): usize? {
        let i: usize = 0;
        const n = haystack.len;
        const v_needle: NetVec16 = @splat(needle);
        while (i + 16 <= n) {
            const chunk: NetVec16 = haystack[i..][0..16].*;
            const mask: u16 = @bitCast(chunk == v_needle);
            if (mask != 0) {
                return i + (@ctz(mask) as usize);
            }
            i += 16;
        }
        while (i < n) {
            if (haystack[i] == needle) return i;
            i += 1;
        }
        return null;
    }

    /// 16-lane SIMD ASCII case-insensitive equality for HTTP header names & methods.
    pub static fn eqlIgnoreCaseSimd(a: []const u8, b: []const u8): bool {
        if (a.len != b.len) return false;
        let i: usize = 0;
        const n = a.len;
        const v_upper_a: NetVec16 = @splat('A');
        const v_upper_z: NetVec16 = @splat('Z');
        const v_bit: NetVec16 = @splat(0x20);

        while (i + 16 <= n) {
            const ca: NetVec16 = a[i..][0..16].*;
            const cb: NetVec16 = b[i..][0..16].*;
            const la = @select(u8, (ca >= v_upper_a) & (ca <= v_upper_z), ca | v_bit, ca);
            const lb = @select(u8, (cb >= v_upper_a) & (cb <= v_upper_z), cb | v_bit, cb);
            if (@reduce(.Or, la ^ lb) != 0) return false;
            i += 16;
        }
        while (i < n) {
            const ca = if (a[i] >= 'A' && a[i] <= 'Z') a[i] | 0x20 else a[i];
            const cb = if (b[i] >= 'A' && b[i] <= 'Z') b[i] | 0x20 else b[i];
            if (ca != cb) return false;
            i += 1;
        }
        return true;
    }
}

pub enum NetworkEventKind {
    Request,
    Response,
    RequestFinished,
    RequestFailed,
    RequestServedFromCache,
    AuthRequired,
}

pub struct InterceptDecision {
    state: InterceptResolutionState = .{},
    abort_reason: []const u8 = "Failed",
    respond_status: u16 = 200,
    respond_content_type: []const u8 = "text/plain",
    respond_body: []const u8 = "",
    continue_overrides: ContinueRequestOverrides = .{},

    pub static fn none(): InterceptDecision {
        return InterceptDecision{
            state: InterceptResolutionState{
                action: InterceptResolutionAction.None,
                priority: 0,
            },
        };
    }

    pub static fn abort(reason: []const u8, priority: i32): InterceptDecision {
        return InterceptDecision{
            state: InterceptResolutionState{
                action: InterceptResolutionAction.Abort,
                priority: priority,
            },
            abort_reason: reason,
        };
    }

    pub static fn respond(
        status: u16,
        content_type: []const u8,
        body: []const u8,
        priority: i32,
    ): InterceptDecision {
        return InterceptDecision{
            state: InterceptResolutionState{
                action: InterceptResolutionAction.Respond,
                priority: priority,
            },
            respond_status: status,
            respond_content_type: content_type,
            respond_body: body,
        };
    }

    pub static fn continueWith(overrides: ContinueRequestOverrides, priority: i32): InterceptDecision {
        return InterceptDecision{
            state: InterceptResolutionState{
                action: InterceptResolutionAction.Continue,
                priority: priority,
            },
            continue_overrides: overrides,
        };
    }
}

pub type InterceptHandlerFn = fn (req: *HTTPRequest, ctx: usize) InterceptDecision;

pub struct InterceptHandlerEntry {
    handler: InterceptHandlerFn = undefined,
    priority: i32 = 0,
    ctx: usize = 0,
    active: bool = false,
}

pub struct ManagedRequestRecord {
    request: HTTPRequest = .{},
    response: HTTPResponse = .{},
    has_response: bool = false,
    finished: bool = false,
    failed: bool = false,
    served_from_cache: bool = false,
    auth_attempted: bool = false,
    last_resolution_priority: i32 = 0,
}

fn parseResourceType(raw: []const u8): ResourceType {
    if (NetworkSimdScanner.eqlIgnoreCaseSimd(raw, "Document")) return ResourceType.Document;
    if (NetworkSimdScanner.eqlIgnoreCaseSimd(raw, "Stylesheet")) return ResourceType.Stylesheet;
    if (NetworkSimdScanner.eqlIgnoreCaseSimd(raw, "Image")) return ResourceType.Image;
    if (NetworkSimdScanner.eqlIgnoreCaseSimd(raw, "Media")) return ResourceType.Media;
    if (NetworkSimdScanner.eqlIgnoreCaseSimd(raw, "Font")) return ResourceType.Font;
    if (NetworkSimdScanner.eqlIgnoreCaseSimd(raw, "Script")) return ResourceType.Script;
    if (NetworkSimdScanner.eqlIgnoreCaseSimd(raw, "TextTrack")) return ResourceType.TextTrack;
    if (NetworkSimdScanner.eqlIgnoreCaseSimd(raw, "XHR")) return ResourceType.XHR;
    if (NetworkSimdScanner.eqlIgnoreCaseSimd(raw, "Fetch")) return ResourceType.Fetch;
    if (NetworkSimdScanner.eqlIgnoreCaseSimd(raw, "EventSource")) return ResourceType.EventSource;
    if (NetworkSimdScanner.eqlIgnoreCaseSimd(raw, "WebSocket")) return ResourceType.WebSocket;
    if (NetworkSimdScanner.eqlIgnoreCaseSimd(raw, "Manifest")) return ResourceType.Manifest;
    return ResourceType.Other;
}

fn actionRank(action: InterceptResolutionAction): i32 {
    switch (action) {
        InterceptResolutionAction.Abort => return 3,
        InterceptResolutionAction.Respond => return 2,
        InterceptResolutionAction.Continue => return 1,
        InterceptResolutionAction.None => return 0,
    }
}

fn decisionOutranks(candidate: InterceptDecision, current: InterceptDecision): bool {
    if (candidate.state.action == InterceptResolutionAction.None) return false;
    if (current.state.action == InterceptResolutionAction.None) return true;
    if (candidate.state.priority > current.state.priority) return true;
    if (candidate.state.priority < current.state.priority) return false;
    return actionRank(candidate.state.action) > actionRank(current.state.action);
}

pub struct NetworkManager {
    allocator: std.mem.Allocator = undefined,
    session: *CDPSession = undefined,
    emitter: PuppeteerEventEmitter = .{},
    records: std.ArrayList(*ManagedRequestRecord) = .empty,
    extra_headers_list: std.ArrayList(HttpHeader) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,
    credentials_val: Credentials? = null,
    user_interception_enabled: bool = false,
    protocol_interception_enabled: bool = false,
    cache_enabled: bool = true,
    offline_mode: bool = false,
    emulated_conditions: NetworkConditions = .{},
    interceptors: [16]InterceptHandlerEntry = undefined,
    interceptors_len: usize = 0,
    in_flight_count: usize = 0,
    disposed: bool = false,

    pub static fn init(alloc: std.mem.Allocator, session: *CDPSession): NetworkManager {
        return NetworkManager{
            allocator: alloc,
            session: session,
            emitter: PuppeteerEventEmitter.init(),
            records: .empty,
            extra_headers_list: .empty,
            owned_strings: .empty,
            credentials_val: null,
            user_interception_enabled: false,
            protocol_interception_enabled: false,
            cache_enabled: true,
            offline_mode: false,
            emulated_conditions: .{},
            interceptors_len: 0,
            in_flight_count: 0,
            disposed: false,
        };
    }

    pub fn deinit(*_) {
        if (_.disposed) return;
        _.disposed = true;

        for (rec of _.records.items) {
            _.allocator.destroy(rec);
        }
        _.records.deinit(_.allocator);
        _.records = .empty;

        _.extra_headers_list.deinit(_.allocator);
        _.extra_headers_list = .empty;

        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;

        _.emitter.removeAllListeners("");
    }

    ~init() {
        _.deinit();
    }

    fn intern(*_, s: []const u8): ([]const u8)!PuppeteerError {
        if (s.len == 0) return "";
        const duped = _.allocator.dupe(u8, s) catch return PuppeteerError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PuppeteerError.OutOfMemory;
        };
        return duped;
    }

    fn findRecord(*_, request_id: []const u8): (*ManagedRequestRecord)? {
        let i: usize = _.records.items.len;
        while (i > 0) {
            i -= 1;
            const rec = _.records.items[i];
            if (std.mem.eql(u8, rec.request.request_id, request_id) ||
                (rec.request.interception_id.len > 0 && std.mem.eql(u8, rec.request.interception_id, request_id)))
            {
                return rec;
            }
        }
        return null;
    }

    fn findRecordConst(request_id: []const u8): (*ManagedRequestRecord)? {
        let i: usize = _.records.items.len;
        while (i > 0) {
            i -= 1;
            const rec = _.records.items[i];
            if (std.mem.eql(u8, rec.request.request_id, request_id) ||
                (rec.request.interception_id.len > 0 && std.mem.eql(u8, rec.request.interception_id, request_id)))
            {
                return rec;
            }
        }
        return null;
    }

    pub fn on(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool {
        return _.emitter.on(event_name, handler, ctx);
    }

    pub fn once(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool {
        return _.emitter.once(event_name, handler, ctx);
    }

    pub fn off(*_, event_name: []const u8, handler: EventHandlerFn) {
        _.emitter.off(event_name, handler);
    }

    pub fn addInterceptor(*_, handler: InterceptHandlerFn, priority: i32, ctx: usize): bool {
        if (_.interceptors_len >= 16) return false;
        _.interceptors[_.interceptors_len] = InterceptHandlerEntry{
            handler: handler,
            priority: priority,
            ctx: ctx,
            active: true,
        };
        _.interceptors_len += 1;
        return true;
    }

    pub fn clearInterceptors(*_) {
        _.interceptors_len = 0;
    }

    pub fn setRequestInterception(*_, enabled: bool): void!PuppeteerError {
        _.user_interception_enabled = enabled;
        return _.updateProtocolRequestInterception();
    }

    pub fn authenticate(*_, creds: Credentials?): void!PuppeteerError {
        if (c := creds) {
            const u = _.intern(c.username)?;
            const p = _.intern(c.password)?;
            _.credentials_val = Credentials{ username: u, password: p };
        } else {
            _.credentials_val = null;
        }
        return _.updateProtocolRequestInterception();
    }

    fn updateProtocolRequestInterception(*_): void!PuppeteerError {
        const should_enable = _.user_interception_enabled || _.credentials_val != null;
        if (should_enable == _.protocol_interception_enabled) return;
        _.protocol_interception_enabled = should_enable;

        let fbuf: [1024]u8 = undefined;
        if (should_enable) {
            const handle_auth = _.credentials_val != null;
            let pbuf: [256]u8 = undefined;
            const params = std.fmt.bufPrint(
                &pbuf,
                "{{\"handleAuthRequests\":{},\"patterns\":[{{\"urlPattern\":\"*\"}}]}}",
                .{handle_auth},
            ) catch return PuppeteerError.BufferTooSmall;
            const _ign_1 = _.session.send("Fetch.enable", params, &fbuf)?;
            * = _ign_1;
            const _ign_2 = _.session.send("Network.setCacheDisabled", "{\"cacheDisabled\":true}", &fbuf)?;
            * = _ign_2;
        } else {
            const _ign_3 = _.session.send("Fetch.disable", "{}", &fbuf)?;
            * = _ign_3;
            let pbuf: [64]u8 = undefined;
            const params = std.fmt.bufPrint(&pbuf, "{{\"cacheDisabled\":{}}}", .{!_.cache_enabled}) catch return PuppeteerError.BufferTooSmall;
            const _ign_4 = _.session.send("Network.setCacheDisabled", params, &fbuf)?;
            * = _ign_4;
        }
    }

    pub fn setCacheEnabled(*_, enabled: bool): void!PuppeteerError {
        _.cache_enabled = enabled;
        let pbuf: [64]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const disabled = !enabled || _.protocol_interception_enabled;
        const params = std.fmt.bufPrint(&pbuf, "{{\"cacheDisabled\":{}}}", .{disabled}) catch return PuppeteerError.BufferTooSmall;
        const _ign_1 = _.session.send("Network.setCacheDisabled", params, &fbuf)?;
        * = _ign_1;
    }

    pub fn isCacheEnabled(): bool {
        return _.cache_enabled;
    }

    pub fn setOfflineMode(*_, enabled: bool): void!PuppeteerError {
        _.offline_mode = enabled;
        _.emulated_conditions.offline = enabled;
        return _.applyNetworkConditions();
    }

    pub fn isOffline(): bool {
        return _.offline_mode;
    }

    pub fn emulateNetworkConditions(*_, conditions: NetworkConditions): void!PuppeteerError {
        _.emulated_conditions = conditions;
        _.offline_mode = conditions.offline;
        return _.applyNetworkConditions();
    }

    fn applyNetworkConditions(*_): void!PuppeteerError {
        let pbuf: [256]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const c = _.emulated_conditions;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"offline\":{},\"latency\":{d:.2},\"downloadThroughput\":{d:.2},\"uploadThroughput\":{d:.2}}}",
            .{ c.offline, c.latency, c.download_throughput, c.upload_throughput },
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_1 = _.session.send("Network.emulateNetworkConditions", params, &fbuf)?;
        * = _ign_1;
    }

    pub fn setExtraHTTPHeaders(*_, headers: []const HttpHeader): void!PuppeteerError {
        _.extra_headers_list.clearRetainingCapacity();

        let pbuf: [2048]u8 = undefined;
        let fbuf: [2560]u8 = undefined;
        const prefix = "{\"headers\":{";
        @memcpy(pbuf[0..prefix.len], prefix);
        let pos: usize = prefix.len;

        let i: usize = 0;
        while (i < headers.len) {
            const k = _.intern(headers[i].name)?;
            const v = _.intern(headers[i].value)?;
            _.extra_headers_list.append(_.allocator, HttpHeader{ name: k, value: v }) catch return PuppeteerError.OutOfMemory;

            if (i > 0) {
                if (pos >= pbuf.len) return PuppeteerError.BufferTooSmall;
                pbuf[pos] = ',';
                pos += 1;
            }
            const seg = std.fmt.bufPrint(pbuf[pos..], "\"{s}\":\"{s}\"", .{ k, v }) catch return PuppeteerError.BufferTooSmall;
            pos += seg.len;
            i += 1;
        }
        if (pos + 2 > pbuf.len) return PuppeteerError.BufferTooSmall;
        pbuf[pos] = '}';
        pbuf[pos + 1] = '}';
        pos += 2;

        const _ign_1 = _.session.send("Network.setExtraHTTPHeaders", pbuf[0..pos], &fbuf)?;
        * = _ign_1;
    }

    pub fn extraHTTPHeaders(): []const HttpHeader {
        return _.extra_headers_list.items;
    }

    /// Central CDP event router for Network.* and Fetch.* domains.
    pub fn handleCdpEvent(*_, ev: CdpEvent): void!PuppeteerError {
        if (std.mem.eql(u8, ev.method, "Network.requestWillBeSent")) {
            return _.onRequestWillBeSent(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Fetch.requestPaused")) {
            return _.onRequestPaused(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Fetch.authRequired")) {
            return _.onAuthRequired(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Network.responseReceived")) {
            return _.onResponseReceived(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Network.loadingFinished")) {
            _.onLoadingFinished(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Network.loadingFailed")) {
            _.onLoadingFailed(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Network.requestServedFromCache")) {
            _.onRequestServedFromCache(ev.params_json);
        }
    }

    /// Handles `Network.requestWillBeSent` including HTTP redirect chains.
    pub fn onRequestWillBeSent(*_, params_json: []const u8): void!PuppeteerError {
        const req_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "requestId", true) orelse return;
        const frame_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "frameId", true) orelse "";
        const type_raw = CdpFrameScanner.extractTopLevelField(params_json, "type", true) orelse "Other";
        const loader_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "loaderId", true) orelse "";
        const req_obj = CdpFrameScanner.extractTopLevelField(params_json, "request", false) orelse "{}";
        const url_raw = CdpFrameScanner.extractTopLevelField(req_obj, "url", true) orelse "";
        const method_raw = CdpFrameScanner.extractTopLevelField(req_obj, "method", true) orelse "GET";
        const post_data_raw = CdpFrameScanner.extractTopLevelField(req_obj, "postData", true) orelse "";

        const req_id = _.intern(req_id_raw)?;
        const url = _.intern(url_raw)?;
        const method = _.intern(method_raw)?;
        const frame_id = _.intern(frame_id_raw)?;
        const post_data = _.intern(post_data_raw)?;
        const res_type = parseResourceType(type_raw);
        const is_nav = (loader_id_raw.len > 0 && std.mem.eql(u8, req_id_raw, loader_id_raw)) || res_type == ResourceType.Document;

        // Check for redirectResponse on an existing requestId
        if (redir_obj := CdpFrameScanner.extractTopLevelField(params_json, "redirectResponse", false)) {
            if (existing := _.findRecord(req_id)) {
                const prev_url = existing.request.url_str;
                existing.request.pushRedirectUrl(prev_url);

                const redir_status_raw = CdpFrameScanner.extractTopLevelField(redir_obj, "status", false) orelse "302";
                const redir_status = (CdpFrameScanner.parseFastI64(redir_status_raw) orelse 302) as u16;
                const redir_text_raw = CdpFrameScanner.extractTopLevelField(redir_obj, "statusText", true) orelse "Found";
                const redir_text = _.intern(redir_text_raw)?;

                existing.response = HTTPResponse.init(req_id, prev_url, redir_status, redir_text, "");
                existing.has_response = true;
                existing.request.setResponse(*existing.response);
                existing.response.setRequest(*existing.request);
                const _ign_r = _.emitter.emit("response", prev_url);
                * = _ign_r;

                // Update active request to new redirected target
                existing.request.url_str = url;
                existing.request.method_str = method;
                existing.request.resource_type_val = res_type;
                if (post_data.len > 0) existing.request.setPostData(post_data);
                const _ign_q = _.emitter.emit("request", url);
                * = _ign_q;
                return;
            }
        }

        // Reuse record if Fetch.requestPaused arrived first with same networkId
        if (existing := _.findRecord(req_id)) {
            existing.request.url_str = url;
            existing.request.method_str = method;
            existing.request.frame_id_str = frame_id;
            existing.request.resource_type_val = res_type;
            existing.request.is_navigation = is_nav;
            if (post_data.len > 0) existing.request.setPostData(post_data);
            const _ign_e = _.emitter.emit("request", url);
            * = _ign_e;
            return;
        }

        let rec = _.allocator.create(ManagedRequestRecord) catch return PuppeteerError.OutOfMemory;
        rec.* = ManagedRequestRecord{
            request: HTTPRequest.init(
                _.session,
                req_id,
                url,
                method,
                res_type,
                is_nav,
                _.user_interception_enabled,
            ),
            response: .{},
            has_response: false,
            finished: false,
            failed: false,
            served_from_cache: false,
            auth_attempted: false,
            last_resolution_priority: 0,
        };
        rec.request.frame_id_str = frame_id;
        if (post_data.len > 0) rec.request.setPostData(post_data);

        if (init_obj := CdpFrameScanner.extractTopLevelField(params_json, "initiator", false)) {
            const ikind_raw = CdpFrameScanner.extractTopLevelField(init_obj, "type", true) orelse "other";
            const iurl_raw = CdpFrameScanner.extractTopLevelField(init_obj, "url", true) orelse "";
            const iline_raw = CdpFrameScanner.extractTopLevelField(init_obj, "lineNumber", false) orelse "0";
            const ikind = _.intern(ikind_raw)?;
            const iurl = _.intern(iurl_raw)?;
            const iline = (CdpFrameScanner.parseFastI64(iline_raw) orelse 0) as u32;
            rec.request.setInitiator(RequestInitiator{
                kind: ikind,
                url: iurl,
                line_number: iline,
                column_number: 0,
            });
        }

        _.records.append(_.allocator, rec) catch {
            _.allocator.destroy(rec);
            return PuppeteerError.OutOfMemory;
        };
        _.in_flight_count += 1;

        const _ign_emit = _.emitter.emit("request", url);
        * = _ign_emit;
    }

    /// Handles `Fetch.requestPaused` with multi-interceptor priority arbitration.
    pub fn onRequestPaused(*_, params_json: []const u8): void!PuppeteerError {
        const interception_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "requestId", true) orelse return;
        const network_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "networkId", true) orelse interception_id_raw;
        const frame_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "frameId", true) orelse "";
        const type_raw = CdpFrameScanner.extractTopLevelField(params_json, "resourceType", true) orelse "Other";
        const req_obj = CdpFrameScanner.extractTopLevelField(params_json, "request", false) orelse "{}";
        const url_raw = CdpFrameScanner.extractTopLevelField(req_obj, "url", true) orelse "";
        const method_raw = CdpFrameScanner.extractTopLevelField(req_obj, "method", true) orelse "GET";

        const interception_id = _.intern(interception_id_raw)?;
        const network_id = _.intern(network_id_raw)?;
        const url = _.intern(url_raw)?;
        const method = _.intern(method_raw)?;
        const frame_id = _.intern(frame_id_raw)?;
        const res_type = parseResourceType(type_raw);

        let target_rec: *ManagedRequestRecord = undefined;
        if (existing := _.findRecord(network_id)) {
            existing.request.interception_id = interception_id;
            existing.request.allow_interception = true;
            existing.request.resolution_handled = false;
            target_rec = existing;
        } else {
            let rec = _.allocator.create(ManagedRequestRecord) catch return PuppeteerError.OutOfMemory;
            rec.* = ManagedRequestRecord{
                request: HTTPRequest.init(
                    _.session,
                    network_id,
                    url,
                    method,
                    res_type,
                    res_type == ResourceType.Document,
                    true,
                ),
            };
            rec.request.interception_id = interception_id;
            rec.request.frame_id_str = frame_id;
            _.records.append(_.allocator, rec) catch {
                _.allocator.destroy(rec);
                return PuppeteerError.OutOfMemory;
            };
            _.in_flight_count += 1;
            target_rec = rec;
        }

        // Arbitrate across all registered interceptors by (priority desc, actionRank desc)
        let winning = InterceptDecision.none();
        let i: usize = 0;
        while (i < _.interceptors_len) {
            if (_.interceptors[i].active) {
                let candidate = _.interceptors[i].handler(*target_rec.request, _.interceptors[i].ctx);
                if (candidate.state.priority == 0 && _.interceptors[i].priority != 0) {
                    candidate.state.priority = _.interceptors[i].priority;
                }
                if (decisionOutranks(candidate, winning)) {
                    winning = candidate;
                }
            }
            i += 1;
        }

        target_rec.last_resolution_priority = winning.state.priority;

        if (!target_rec.request.isInterceptResolutionHandled()) {
            switch (winning.state.action) {
                InterceptResolutionAction.Abort => {
                    target_rec.request.abort(winning.abort_reason)?;
                }
                InterceptResolutionAction.Respond => {
                    target_rec.request.respond(
                        winning.respond_status,
                        winning.respond_content_type,
                        winning.respond_body,
                    )?;
                }
                InterceptResolutionAction.Continue => {
                    const ovr = winning.continue_overrides;
                    if (ovr.url.len > 0 || ovr.method.len > 0 || ovr.post_data.len > 0 || ovr.headers.len > 0) {
                        target_rec.request.continueWithOverrides(ovr)?;
                    } else {
                        target_rec.request.continueRequest("", "")?;
                    }
                }
                InterceptResolutionAction.None => {
                    // Default continue if no interceptor handled the paused request
                    target_rec.request.continueRequest("", "")?;
                }
            }
        }
    }

    /// Handles `Fetch.authRequired` and sends `Fetch.continueWithAuth`.
    pub fn onAuthRequired(*_, params_json: []const u8): void!PuppeteerError {
        const req_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "requestId", true) orelse return;
        const _ign_ev = _.emitter.emit("authrequired", req_id_raw);
        * = _ign_ev;

        let already_attempted = false;
        if (rec := _.findRecord(req_id_raw)) {
            already_attempted = rec.auth_attempted;
            rec.auth_attempted = true;
        }

        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        let params: []const u8 = "";

        if (creds := _.credentials_val) {
            if (!already_attempted) {
                params = std.fmt.bufPrint(
                    &pbuf,
                    "{{\"requestId\":\"{s}\",\"authChallengeResponse\":{{\"response\":\"ProvideCredentials\",\"username\":\"{s}\",\"password\":\"{s}\"}}}}",
                    .{ req_id_raw, creds.username, creds.password },
                ) catch return PuppeteerError.BufferTooSmall;
            } else {
                params = std.fmt.bufPrint(
                    &pbuf,
                    "{{\"requestId\":\"{s}\",\"authChallengeResponse\":{{\"response\":\"CancelAuth\"}}}}",
                    .{req_id_raw},
                ) catch return PuppeteerError.BufferTooSmall;
            }
        } else {
            params = std.fmt.bufPrint(
                &pbuf,
                "{{\"requestId\":\"{s}\",\"authChallengeResponse\":{{\"response\":\"Default\"}}}}",
                .{req_id_raw},
            ) catch return PuppeteerError.BufferTooSmall;
        }

        const _ign_1 = _.session.send("Fetch.continueWithAuth", params, &fbuf)?;
        * = _ign_1;
    }

    /// Handles `Network.responseReceived`, binding HTTPResponse <-> HTTPRequest.
    pub fn onResponseReceived(*_, params_json: []const u8): void!PuppeteerError {
        const req_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "requestId", true) orelse return;
        const resp_obj = CdpFrameScanner.extractTopLevelField(params_json, "response", false) orelse "{}";
        const url_raw = CdpFrameScanner.extractTopLevelField(resp_obj, "url", true) orelse "";
        const status_raw = CdpFrameScanner.extractTopLevelField(resp_obj, "status", false) orelse "200";
        const status_text_raw = CdpFrameScanner.extractTopLevelField(resp_obj, "statusText", true) orelse "OK";
        const from_disk_raw = CdpFrameScanner.extractTopLevelField(resp_obj, "fromDiskCache", false) orelse "false";
        const from_sw_raw = CdpFrameScanner.extractTopLevelField(resp_obj, "fromServiceWorker", false) orelse "false";
        const remote_ip_raw = CdpFrameScanner.extractTopLevelField(resp_obj, "remoteIPAddress", true) orelse "";
        const remote_port_raw = CdpFrameScanner.extractTopLevelField(resp_obj, "remotePort", false) orelse "0";
        const enc_len_raw = CdpFrameScanner.extractTopLevelField(resp_obj, "encodedDataLength", false) orelse "0";

        const req_id = _.intern(req_id_raw)?;
        const url = _.intern(url_raw)?;
        const status_text = _.intern(status_text_raw)?;
        const remote_ip = _.intern(remote_ip_raw)?;
        const status = (CdpFrameScanner.parseFastI64(status_raw) orelse 200) as u16;
        const remote_port = (CdpFrameScanner.parseFastI64(remote_port_raw) orelse 0) as u16;
        const enc_len = (CdpFrameScanner.parseFastI64(enc_len_raw) orelse 0) as usize;

        const rec = _.findRecord(req_id) orelse return;
        rec.response = HTTPResponse.init(req_id, url, status, status_text, "");
        rec.response.from_cache_flag = std.mem.eql(u8, from_disk_raw, "true") || rec.served_from_cache;
        rec.response.from_sw_flag = std.mem.eql(u8, from_sw_raw, "true");
        rec.response.remote_addr = RemoteAddress{ ip: remote_ip, port: remote_port };
        rec.response.encoded_len = enc_len;

        if (sec_obj := CdpFrameScanner.extractTopLevelField(resp_obj, "securityDetails", false)) {
            const sec_copy = _.intern(sec_obj)?;
            rec.response.setSecurityDetails(SecurityDetails.fromCdpPayload(sec_copy));
        }

        rec.has_response = true;
        rec.request.setResponse(*rec.response);
        rec.response.setRequest(*rec.request);

        const _ign_ev = _.emitter.emit("response", url);
        * = _ign_ev;
    }

    /// Handles `Network.requestServedFromCache`.
    pub fn onRequestServedFromCache(*_, params_json: []const u8) {
        const req_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "requestId", true) orelse return;
        if (rec := _.findRecord(req_id_raw)) {
            rec.served_from_cache = true;
            rec.response.from_cache_flag = true;
            const _ign_ev = _.emitter.emit("requestservedfromcache", rec.request.url_str);
            * = _ign_ev;
        }
    }

    /// Handles `Network.loadingFinished`.
    pub fn onLoadingFinished(*_, params_json: []const u8) {
        const req_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "requestId", true) orelse return;
        const enc_raw = CdpFrameScanner.extractTopLevelField(params_json, "encodedDataLength", false) orelse "0";
        const enc_len = (CdpFrameScanner.parseFastI64(enc_raw) orelse 0) as usize;

        if (rec := _.findRecord(req_id_raw)) {
            if (!rec.finished && !rec.failed && _.in_flight_count > 0) {
                _.in_flight_count -= 1;
            }
            rec.finished = true;
            if (enc_len > 0) rec.response.encoded_len = enc_len;
            const _ign_ev = _.emitter.emit("requestfinished", rec.request.url_str);
            * = _ign_ev;
        }
    }

    /// Handles `Network.loadingFailed`.
    pub fn onLoadingFailed(*_, params_json: []const u8) {
        const req_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "requestId", true) orelse return;
        const err_raw = CdpFrameScanner.extractTopLevelField(params_json, "errorText", true) orelse "net::ERR_FAILED";
        const err_str = _.intern(err_raw) catch err_raw;

        if (rec := _.findRecord(req_id_raw)) {
            if (!rec.finished && !rec.failed && _.in_flight_count > 0) {
                _.in_flight_count -= 1;
            }
            rec.failed = true;
            rec.request.failure_reason = err_str;
            const _ign_ev = _.emitter.emit("requestfailed", err_str);
            * = _ign_ev;
        }
    }

    pub fn getRequestById(request_id: []const u8): (*HTTPRequest)? {
        if (rec := _.findRecordConst(request_id)) {
            return *rec.request;
        }
        return null;
    }

    pub fn getResponseById(request_id: []const u8): (*HTTPResponse)? {
        if (rec := _.findRecordConst(request_id)) {
            if (rec.has_response) return *rec.response;
        }
        return null;
    }

    pub fn getRecordById(request_id: []const u8): (*ManagedRequestRecord)? {
        return _.findRecordConst(request_id);
    }

    pub fn inFlightCount(): usize {
        return _.in_flight_count;
    }

    pub fn requestCount(): usize {
        return _.records.items.len;
    }
}
