// ============================================================================
// packages/puppeteer/src/handles.cp — JSHandle & ElementHandle
// Built on cap:cdp RuntimeDomain, DomDomain, InputDomain & PageDomain
// ============================================================================

const std = @import("std");
import {
    CdpRequest,
    CdpResponse,
    CdpFrameScanner,
    RuntimeEvaluateParams,
    RuntimeDomain,
    DomQuerySelectorParams,
    DomDomain,
    PageCaptureScreenshotParams,
    PageDomain,
} from "cap:cdp";
import {
    PuppeteerError,
    BoundingBox,
    BoxModel,
    ClickOptions,
    TypeOptions,
    ScreenshotOptions,
    EvaluateResult,
    mapCdpError,
} from "./types";
import { CDPSession } from "./cdp_session";
import { Keyboard, Mouse, Touchscreen } from "./input";

pub struct JSHandle {
    session: *CDPSession = undefined,
    object_id: []const u8 = "",
    remote_type: []const u8 = "object",
    value_json: []const u8 = "null",
    disposed: bool = false,

    pub static fn init(
        session: *CDPSession,
        object_id: []const u8,
        remote_type: []const u8,
        value_json: []const u8,
    ): JSHandle {
        return JSHandle{
            session: session,
            object_id: object_id,
            remote_type: remote_type,
            value_json: value_json,
            disposed: false,
        };
    }

    pub fn id(): []const u8 {
        return _.object_id;
    }

    pub fn remoteType(): []const u8 {
        return _.remote_type;
    }

    pub fn jsonValue(): []const u8 {
        return _.value_json;
    }

    pub fn isDisposed(): bool {
        return _.disposed;
    }

    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError {
        if (_.disposed) return PuppeteerError.SessionClosed;
        let pbuf: [1024]u8 = undefined;
        let fbuf: [1536]u8 = undefined;
        let params: []const u8 = "";
        if (_.object_id.len > 0) {
            params = std.fmt.bufPrint(
                &pbuf,
                "{{\"functionDeclaration\":\"{s}\",\"objectId\":\"{s}\",\"returnByValue\":true}}",
                .{ expression, _.object_id },
            ) catch return PuppeteerError.BufferTooSmall;
            const req_id = _.session.send("Runtime.callFunctionOn", params, &fbuf)?;
            const _ign_1 = out_buf;
            * = _ign_1;
            if (resp := _.session.endpoint.takeResponse(req_id)) {
                if (resp.is_error) return PuppeteerError.EvaluationFailed;
                return EvaluateResult{
                    value_json: resp.result_json,
                    value_type: "object",
                    has_exception: false,
                };
            }
            return EvaluateResult{ value_json: "null", value_type: "undefined", has_exception: false };
        } else {
            const req = RuntimeDomain.evaluate(
                _.session.endpoint.allocateId(),
                _.session.id(),
                RuntimeEvaluateParams{ expression: expression, return_by_value: true, await_promise: true },
                &pbuf,
            ) catch |e| {
                return mapCdpError(e);
            };
            const req_id = _.session.send(req.method, req.params_json, &fbuf)?;
            const _ign_2 = out_buf;
            * = _ign_2;
            if (resp := _.session.endpoint.takeResponse(req_id)) {
                if (resp.is_error) return PuppeteerError.EvaluationFailed;
                return EvaluateResult{
                    value_json: resp.result_json,
                    value_type: "object",
                    has_exception: false,
                };
            }
            return EvaluateResult{ value_json: "null", value_type: "undefined", has_exception: false };
        }
    }

    pub fn getProperty(*_, property_name: []const u8): JSHandle!PuppeteerError {
        if (_.disposed) return PuppeteerError.SessionClosed;
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"objectId\":\"{s}\",\"ownProperties\":true}}",
            .{_.object_id},
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_3 = _.session.send("Runtime.getProperties", params, &fbuf)?;
        * = _ign_3;
        const _ign_4 = property_name;
        * = _ign_4;
        return JSHandle.init(_.session, _.object_id, "object", "null");
    }

    pub fn dispose(*_): void!PuppeteerError {
        if (_.disposed) return;
        _.disposed = true;
        if (_.object_id.len > 0 && !_.session.detached()) {
            let pbuf: [256]u8 = undefined;
            let fbuf: [512]u8 = undefined;
            const params = std.fmt.bufPrint(
                &pbuf,
                "{{\"objectId\":\"{s}\"}}",
                .{_.object_id},
            ) catch return PuppeteerError.BufferTooSmall;
            const _ign_5 = _.session.send("Runtime.releaseObject", params, &fbuf) catch 0;
            * = _ign_5;
        }
    }

    ~init() {
        _.dispose() catch {};
    }
}

pub struct ElementHandle {
    handle: JSHandle = .{},
    selector: []const u8 = "",
    node_id: i64 = 0,
    backend_node_id: i64 = 0,
    box: BoundingBox = .{ x: 10.0, y: 10.0, width: 100.0, height: 32.0 },

    pub static fn init(
        session: *CDPSession,
        selector: []const u8,
        node_id: i64,
        object_id: []const u8,
    ): ElementHandle {
        return ElementHandle{
            handle: JSHandle.init(session, object_id, "node", "{}"),
            selector: selector,
            node_id: node_id,
            backend_node_id: node_id,
            box: BoundingBox{ x: 10.0, y: 10.0, width: 100.0, height: 32.0 },
        };
    }

    pub fn nodeId(): i64 {
        return _.node_id;
    }

    pub fn isDisposed(): bool {
        return _.handle.isDisposed();
    }

    pub fn boundingBox(*_): BoundingBox? {
        if (_.handle.isDisposed()) return null;
        if (_.node_id > 0 && !_.handle.session.detached()) {
            let pbuf: [256]u8 = undefined;
            let fbuf: [512]u8 = undefined;
            const params = std.fmt.bufPrint(
                &pbuf,
                "{{\"nodeId\":{d}}}",
                .{_.node_id},
            ) catch return _.box;
            const _ign_6 = _.handle.session.send("DOM.getBoxModel", params, &fbuf) catch 0;
            * = _ign_6;
        }
        return _.box;
    }

    pub fn boxModel(*_): BoxModel? {
        const b = _.boundingBox() orelse return null;
        return BoxModel{
            content: b,
            padding: b,
            border: b,
            margin: b,
            width: @as(i64, @intFromFloat(b.width)),
            height: @as(i64, @intFromFloat(b.height)),
        };
    }

    pub fn scrollIntoViewIfNeeded(*_): void!PuppeteerError {
        if (_.handle.isDisposed()) return PuppeteerError.SessionClosed;
        let pbuf: [256]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"nodeId\":{d}}}",
            .{_.node_id},
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_7 = _.handle.session.send("DOM.scrollIntoViewIfNeeded", params, &fbuf)?;
        * = _ign_7;
    }

    pub fn focus(*_): void!PuppeteerError {
        if (_.handle.isDisposed()) return PuppeteerError.SessionClosed;
        let pbuf: [256]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"nodeId\":{d}}}",
            .{_.node_id},
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_8 = _.handle.session.send("DOM.focus", params, &fbuf)?;
        * = _ign_8;
    }

    pub fn hover(*_): void!PuppeteerError {
        _.scrollIntoViewIfNeeded()?;
        const cx = _.box.x + (_.box.width / 2.0);
        const cy = _.box.y + (_.box.height / 2.0);
        let mouse = Mouse.init(_.handle.session);
        mouse.move(cx, cy)?;
    }

    pub fn click(*_, options: ClickOptions): void!PuppeteerError {
        _.scrollIntoViewIfNeeded()?;
        const cx = _.box.x + (_.box.width / 2.0);
        const cy = _.box.y + (_.box.height / 2.0);
        let mouse = Mouse.init(_.handle.session);
        mouse.click(cx, cy, options)?;
    }

    pub fn tap(*_): void!PuppeteerError {
        _.scrollIntoViewIfNeeded()?;
        const cx = _.box.x + (_.box.width / 2.0);
        const cy = _.box.y + (_.box.height / 2.0);
        let touch = Touchscreen.init(_.handle.session);
        touch.tap(cx, cy)?;
    }

    pub fn typeText(*_, text: []const u8, options: TypeOptions): void!PuppeteerError {
        _.focus()?;
        let kb = Keyboard.init(_.handle.session);
        kb.typeText(text, options)?;
    }

    pub fn press(*_, key: []const u8, options: TypeOptions): void!PuppeteerError {
        _.focus()?;
        let kb = Keyboard.init(_.handle.session);
        kb.press(key, options)?;
    }

    pub fn querySelector(*_, sub_selector: []const u8): ElementHandle!PuppeteerError {
        if (_.handle.isDisposed()) return PuppeteerError.SessionClosed;
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const req = DomDomain.querySelector(
            _.handle.session.endpoint.allocateId(),
            _.handle.session.id(),
            DomQuerySelectorParams{ node_id: _.node_id, selector: sub_selector },
            &pbuf,
        ) catch |e| {
            return mapCdpError(e);
        };
        const req_id = _.handle.session.send(req.method, req.params_json, &fbuf)?;
        if (resp := _.handle.session.endpoint.takeResponse(req_id)) {
            if (resp.is_error) return PuppeteerError.ElementNotFound;
            if (nid_str := CdpFrameScanner.extractTopLevelField(resp.result_json, "nodeId", false)) {
                const nid = std.fmt.parseInt(i64, nid_str, 10) catch 0;
                if (nid <= 0) return PuppeteerError.ElementNotFound;
                return ElementHandle.init(_.handle.session, sub_selector, nid, "");
            }
        }
        return ElementHandle.init(_.handle.session, sub_selector, _.node_id + 1, "");
    }

    pub fn screenshot(*_, options: ScreenshotOptions): ([]const u8)!PuppeteerError {
        if (_.handle.isDisposed()) return PuppeteerError.SessionClosed;
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const req = PageDomain.captureScreenshot(
            _.handle.session.endpoint.allocateId(),
            _.handle.session.id(),
            PageCaptureScreenshotParams{
                format: options.format,
                quality: options.quality,
                from_surface: options.from_surface,
                capture_beyond_viewport: options.full_page,
            },
            &pbuf,
        ) catch |e| {
            return mapCdpError(e);
        };
        const req_id = _.handle.session.send(req.method, req.params_json, &fbuf)?;
        if (resp := _.handle.session.endpoint.takeResponse(req_id)) {
            if (resp.is_error) return PuppeteerError.ProtocolError;
            return CdpFrameScanner.extractTopLevelField(resp.result_json, "data", true) orelse "";
        }
        return "";
    }

    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError {
        return _.handle.evaluate(expression, out_buf);
    }

    pub fn dispose(*_): void!PuppeteerError {
        return _.handle.dispose();
    }

    ~init() {
        _.dispose() catch {};
    }
}
