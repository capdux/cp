// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/HTTPRequest.ts, HTTPResponse.ts & Dialog.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/network.cp — HTTPRequest, HTTPResponse & Dialog
// ============================================================================

const std = @import("std");
import {
    PuppeteerError,
    DialogType,
    ResourceType,
    HttpHeader,
    RemoteAddress,
    ResponseTiming,
    RequestInitiator,
    ContinueRequestOverrides,
} from "./types";
import { CDPSession } from "./cdp_session";
import { SecurityDetails } from "./extras";

pub enum InterceptResolutionAction {
    Abort,
    Respond,
    Continue,
    None,
}

pub struct InterceptResolutionState {
    action: InterceptResolutionAction = InterceptResolutionAction.None,
    priority: i32 = 0,
}

pub struct HTTPRequest {
    session: *CDPSession = undefined,
    request_id: []const u8 = "",
    interception_id: []const u8 = "",
    url_str: []const u8 = "",
    method_str: []const u8 = "GET",
    post_data_str: []const u8 = "",
    resource_type_val: ResourceType = ResourceType.Other,
    frame_id_str: []const u8 = "",
    is_navigation: bool = false,
    headers_list: [16]HttpHeader = undefined,
    headers_len: usize = 0,
    allow_interception: bool = false,
    resolution_handled: bool = false,
    resolution_action: InterceptResolutionAction = InterceptResolutionAction.None,
    failure_reason: []const u8 = "",
    has_post_data_flag: bool = false,
    initiator_info: RequestInitiator = .{},
    redirect_chain_urls: [8][]const u8 = undefined,
    redirect_chain_len: usize = 0,
    response_ptr: (*HTTPResponse)? = null,

    pub static fn init(
        session: *CDPSession,
        request_id: []const u8,
        req_url: []const u8,
        req_method: []const u8,
        resource_type: ResourceType,
        is_navigation: bool,
        allow_interception: bool,
    ): HTTPRequest {
        return HTTPRequest{
            session: session,
            request_id: request_id,
            interception_id: request_id,
            url_str: req_url,
            method_str: req_method,
            resource_type_val: resource_type,
            is_navigation: is_navigation,
            allow_interception: allow_interception,
            redirect_chain_len: 0,
            response_ptr: null,
        };
    }

    pub fn id(): []const u8 {
        return _.request_id;
    }

    pub fn url(): []const u8 {
        return _.url_str;
    }

    pub fn method(): []const u8 {
        return _.method_str;
    }

    pub fn hasPostData(): bool {
        return _.has_post_data_flag || _.post_data_str.len > 0;
    }

    pub fn postData(): []const u8 {
        return _.post_data_str;
    }

    pub fn setPostData(*_, data: []const u8) {
        _.post_data_str = data;
        _.has_post_data_flag = data.len > 0;
    }

    pub fn resourceType(): ResourceType {
        return _.resource_type_val;
    }

    pub fn frameId(): []const u8 {
        return _.frame_id_str;
    }

    pub fn isNavigationRequest(): bool {
        return _.is_navigation;
    }

    pub fn headers(): []const HttpHeader {
        return _.headers_list[0.._.headers_len];
    }

    pub fn redirectChain(): []const []const u8 {
        return _.redirect_chain_urls[0.._.redirect_chain_len];
    }

    pub fn pushRedirectUrl(*_, redirect_url: []const u8) {
        if (_.redirect_chain_len < 8) {
            _.redirect_chain_urls[_.redirect_chain_len] = redirect_url;
            _.redirect_chain_len += 1;
        }
    }

    pub fn failure(): ([]const u8)? {
        if (_.failure_reason.len == 0) return null;
        return _.failure_reason;
    }

    pub fn initiator(): RequestInitiator {
        return _.initiator_info;
    }

    pub fn setInitiator(*_, init_info: RequestInitiator) {
        _.initiator_info = init_info;
    }

    pub fn response(): (*HTTPResponse)? {
        return _.response_ptr;
    }

    pub fn setResponse(*_, resp: *HTTPResponse) {
        _.response_ptr = resp;
    }

    pub fn isInterceptResolutionHandled(): bool {
        return _.resolution_handled;
    }

    pub fn interceptResolutionState(): InterceptResolutionState {
        return InterceptResolutionState{
            action: _.resolution_action,
            priority: 0,
        };
    }

    pub fn continueWithOverrides(*_, overrides: ContinueRequestOverrides): void!PuppeteerError {
        if (!_.allow_interception) return PuppeteerError.InvalidArgument;
        if (_.resolution_handled) return PuppeteerError.InvalidArgument;
        let pbuf: [2048]u8 = undefined;
        let fbuf: [2560]u8 = undefined;
        let pos: usize = 0;
        const head = std.fmt.bufPrint(&pbuf, "{{\"requestId\":\"{s}\"", .{_.interception_id}) catch return PuppeteerError.BufferTooSmall;
        pos = head.len;
        if (overrides.url.len > 0) {
            const s = std.fmt.bufPrint(pbuf[pos..], ",\"url\":\"{s}\"", .{overrides.url}) catch return PuppeteerError.BufferTooSmall;
            pos += s.len;
        }
        if (overrides.method.len > 0) {
            const s = std.fmt.bufPrint(pbuf[pos..], ",\"method\":\"{s}\"", .{overrides.method}) catch return PuppeteerError.BufferTooSmall;
            pos += s.len;
        }
        if (overrides.post_data.len > 0) {
            const s = std.fmt.bufPrint(pbuf[pos..], ",\"postData\":\"{s}\"", .{overrides.post_data}) catch return PuppeteerError.BufferTooSmall;
            pos += s.len;
        }
        if (overrides.headers.len > 0) {
            const hstart = ",\"headers\":[";
            if (pos + hstart.len >= pbuf.len) return PuppeteerError.BufferTooSmall;
            @memcpy(pbuf[pos .. pos + hstart.len], hstart);
            pos += hstart.len;
            let i: usize = 0;
            while (i < overrides.headers.len) {
                if (i > 0) {
                    if (pos >= pbuf.len) return PuppeteerError.BufferTooSmall;
                    pbuf[pos] = ',';
                    pos += 1;
                }
                const hs = std.fmt.bufPrint(
                    pbuf[pos..],
                    "{{\"name\":\"{s}\",\"value\":\"{s}\"}}",
                    .{ overrides.headers[i].name, overrides.headers[i].value },
                ) catch return PuppeteerError.BufferTooSmall;
                pos += hs.len;
                i += 1;
            }
            if (pos >= pbuf.len) return PuppeteerError.BufferTooSmall;
            pbuf[pos] = ']';
            pos += 1;
        }
        if (pos >= pbuf.len) return PuppeteerError.BufferTooSmall;
        pbuf[pos] = '}';
        pos += 1;
        const _ign_ovr = _.session.send("Fetch.continueRequest", pbuf[0..pos], &fbuf)?;
        * = _ign_ovr;
        _.resolution_handled = true;
        _.resolution_action = InterceptResolutionAction.Continue;
    }

    pub fn continueRequest(*_, override_url: []const u8, override_method: []const u8): void!PuppeteerError {
        if (!_.allow_interception) return PuppeteerError.InvalidArgument;
        if (_.resolution_handled) return PuppeteerError.InvalidArgument;
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        let params: []const u8 = "";
        if (override_url.len > 0 && override_method.len > 0) {
            params = std.fmt.bufPrint(
                &pbuf,
                "{{\"requestId\":\"{s}\",\"url\":\"{s}\",\"method\":\"{s}\"}}",
                .{ _.interception_id, override_url, override_method },
            ) catch return PuppeteerError.BufferTooSmall;
        } else if (override_url.len > 0) {
            params = std.fmt.bufPrint(
                &pbuf,
                "{{\"requestId\":\"{s}\",\"url\":\"{s}\"}}",
                .{ _.interception_id, override_url },
            ) catch return PuppeteerError.BufferTooSmall;
        } else {
            params = std.fmt.bufPrint(
                &pbuf,
                "{{\"requestId\":\"{s}\"}}",
                .{_.interception_id},
            ) catch return PuppeteerError.BufferTooSmall;
        }
        const _ign_1 = _.session.send("Fetch.continueRequest", params, &fbuf)?;
        * = _ign_1;
        _.resolution_handled = true;
        _.resolution_action = InterceptResolutionAction.Continue;
    }

    pub fn respond(*_, status: u16, content_type: []const u8, body: []const u8): void!PuppeteerError {
        if (!_.allow_interception) return PuppeteerError.InvalidArgument;
        if (_.resolution_handled) return PuppeteerError.InvalidArgument;
        let pbuf: [1024]u8 = undefined;
        let fbuf: [1536]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"requestId\":\"{s}\",\"responseCode\":{d},\"responseHeaders\":[{{\"name\":\"Content-Type\",\"value\":\"{s}\"}}],\"body\":\"{s}\"}}",
            .{ _.interception_id, status, content_type, body },
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_2 = _.session.send("Fetch.fulfillRequest", params, &fbuf)?;
        * = _ign_2;
        _.resolution_handled = true;
        _.resolution_action = InterceptResolutionAction.Respond;
    }

    pub fn abort(*_, error_reason: []const u8): void!PuppeteerError {
        if (!_.allow_interception) return PuppeteerError.InvalidArgument;
        if (_.resolution_handled) return PuppeteerError.InvalidArgument;
        const reason = if (error_reason.len > 0) error_reason else "Failed";
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"requestId\":\"{s}\",\"errorReason\":\"{s}\"}}",
            .{ _.interception_id, reason },
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_3 = _.session.send("Fetch.failRequest", params, &fbuf)?;
        * = _ign_3;
        _.resolution_handled = true;
        _.resolution_action = InterceptResolutionAction.Abort;
        _.failure_reason = reason;
    }
}

pub struct HTTPResponse {
    request_id: []const u8 = "",
    url_str: []const u8 = "",
    status_code: u16 = 200,
    status_text_str: []const u8 = "OK",
    from_cache_flag: bool = false,
    from_sw_flag: bool = false,
    remote_addr: RemoteAddress = .{},
    timing_info: ResponseTiming = .{},
    headers_list: [16]HttpHeader = undefined,
    headers_len: usize = 0,
    body_bytes: []const u8 = "",
    encoded_len: usize = 0,
    security_details: SecurityDetails? = null,
    request_ptr: (*HTTPRequest)? = null,

    pub static fn init(
        request_id: []const u8,
        resp_url: []const u8,
        resp_status: u16,
        status_text: []const u8,
        body: []const u8,
    ): HTTPResponse {
        return HTTPResponse{
            request_id: request_id,
            url_str: resp_url,
            status_code: resp_status,
            status_text_str: status_text,
            body_bytes: body,
            encoded_len: body.len,
            security_details: null,
            request_ptr: null,
        };
    }

    pub fn url(): []const u8 {
        return _.url_str;
    }

    pub fn status(): u16 {
        return _.status_code;
    }

    pub fn statusText(): []const u8 {
        return _.status_text_str;
    }

    pub fn ok(): bool {
        return (_.status_code >= 200 && _.status_code <= 299) || _.status_code == 0;
    }

    pub fn headers(): []const HttpHeader {
        return _.headers_list[0.._.headers_len];
    }

    pub fn remoteAddress(): RemoteAddress {
        return _.remote_addr;
    }

    pub fn timing(): ResponseTiming {
        return _.timing_info;
    }

    pub fn fromCache(): bool {
        return _.from_cache_flag;
    }

    pub fn fromServiceWorker(): bool {
        return _.from_sw_flag;
    }

    pub fn securityDetails(): SecurityDetails? {
        return _.security_details;
    }

    pub fn setSecurityDetails(*_, details: SecurityDetails) {
        _.security_details = details;
    }

    pub fn request(): (*HTTPRequest)? {
        return _.request_ptr;
    }

    pub fn setRequest(*_, req: *HTTPRequest) {
        _.request_ptr = req;
    }

    pub fn encodedDataLength(): usize {
        return _.encoded_len;
    }

    pub fn buffer(): []const u8 {
        return _.body_bytes;
    }

    pub fn text(): []const u8 {
        return _.body_bytes;
    }

    pub fn json(): ([]const u8)!PuppeteerError {
        const trimmed = std.mem.trim(u8, _.body_bytes, " \t\r\n");
        if (trimmed.len == 0) return PuppeteerError.EvaluationFailed;
        const first = trimmed[0];
        const valid = (first == '{') || (first == '[') || (first == '"') ||
            (first == 't') || (first == 'f') || (first == 'n') ||
            (first == '-') || (first >= '0' && first <= '9');
        if (!valid) return PuppeteerError.EvaluationFailed;
        return trimmed;
    }
}

pub struct Dialog {
    session: *CDPSession = undefined,
    dialog_type: DialogType = DialogType.Alert,
    message_str: []const u8 = "",
    default_value_str: []const u8 = "",
    handled: bool = false,

    pub static fn init(
        session: *CDPSession,
        dialog_type: DialogType,
        msg: []const u8,
        default_value: []const u8,
    ): Dialog {
        return Dialog{
            session: session,
            dialog_type: dialog_type,
            message_str: msg,
            default_value_str: default_value,
            handled: false,
        };
    }

    pub fn kind(): DialogType {
        return _.dialog_type;
    }

    pub fn message(): []const u8 {
        return _.message_str;
    }

    pub fn defaultValue(): []const u8 {
        return _.default_value_str;
    }

    pub fn accept(*_, prompt_text: []const u8): void!PuppeteerError {
        if (_.handled) return PuppeteerError.InvalidArgument;
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const params = if (prompt_text.len > 0)
            std.fmt.bufPrint(&pbuf, "{{\"accept\":true,\"promptText\":\"{s}\"}}", .{prompt_text}) catch return PuppeteerError.BufferTooSmall
        else
            "{\"accept\":true}";
        const _ign_4 = _.session.send("Page.handleJavaScriptDialog", params, &fbuf)?;
        * = _ign_4;
        _.handled = true;
    }

    pub fn dismiss(*_): void!PuppeteerError {
        if (_.handled) return PuppeteerError.InvalidArgument;
        let fbuf: [512]u8 = undefined;
        const _ign_5 = _.session.send("Page.handleJavaScriptDialog", "{\"accept\":false}", &fbuf)?;
        * = _ign_5;
        _.handled = true;
    }
}
