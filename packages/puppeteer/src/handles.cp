// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/JSHandle.ts & ElementHandle.ts | Synced: 2026-11-28
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
    LocatorVisibility,
    LocatorScrollOptions,
    mapCdpError,
} from "./types";
import { CDPSession } from "./cdp_session";
import { Keyboard, Mouse, Touchscreen } from "./input";

fn appendRaw(buf: []u8, pos: *usize, s: []const u8): void!PuppeteerError {
    if (pos.* + s.len > buf.len) return PuppeteerError.BufferTooSmall;
    if (s.len > 0) {
        @memcpy(buf[pos.* .. pos.* + s.len], s);
        pos.* += s.len;
    }
}

fn appendJsonEscaped(buf: []u8, pos: *usize, s: []const u8): void!PuppeteerError {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        switch (ch) {
            '"' => appendRaw(buf, pos, "\\\"")?,
            '\\' => appendRaw(buf, pos, "\\\\")?,
            '\n' => appendRaw(buf, pos, "\\n")?,
            '\r' => appendRaw(buf, pos, "\\r")?,
            '\t' => appendRaw(buf, pos, "\\t")?,
            else => {
                if (ch < 0x20) {
                    let esc_buf: [6]u8 = undefined;
                    const esc = std.fmt.bufPrint(&esc_buf, "\\u00{x:0>2}", .{ch}) catch return PuppeteerError.BufferTooSmall;
                    appendRaw(buf, pos, esc)?;
                } else {
                    if (pos.* >= buf.len) return PuppeteerError.BufferTooSmall;
                    buf[pos.*] = ch;
                    pos.* += 1;
                }
            },
        }
    }
}

fn parseEvaluateResponse(resp: CdpResponse, out_buf: []u8): EvaluateResult {
    if (resp.is_error) {
        const n = if (resp.error_message.len <= out_buf.len) resp.error_message.len else out_buf.len;
        if (n > 0) @memcpy(out_buf[0..n], resp.error_message[0..n]);
        return EvaluateResult{
            value_json: "null",
            value_type: "error",
            exception_text: out_buf[0..n],
            has_exception: true,
        };
    }
    if (exc_details := CdpFrameScanner.extractTopLevelField(resp.result_json, "exceptionDetails", false)) {
        if (!std.mem.eql(u8, exc_details, "null")) {
            let exc_msg: []const u8 = "Uncaught";
            if (exc_obj := CdpFrameScanner.extractTopLevelField(exc_details, "exception", false)) {
                if (desc := CdpFrameScanner.extractTopLevelField(exc_obj, "description", true)) {
                    exc_msg = desc;
                } else if (txt := CdpFrameScanner.extractTopLevelField(exc_details, "text", true)) {
                    exc_msg = txt;
                }
            } else if (txt := CdpFrameScanner.extractTopLevelField(exc_details, "text", true)) {
                exc_msg = txt;
            }
            const n = if (exc_msg.len <= out_buf.len) exc_msg.len else out_buf.len;
            if (n > 0) @memcpy(out_buf[0..n], exc_msg[0..n]);
            return EvaluateResult{
                value_json: "null",
                value_type: "error",
                exception_text: out_buf[0..n],
                has_exception: true,
            };
        }
    }
    const inner_res = CdpFrameScanner.extractTopLevelField(resp.result_json, "result", false)
        orelse resp.result_json;
    const val = CdpFrameScanner.extractTopLevelField(inner_res, "value", true)
        orelse CdpFrameScanner.extractTopLevelField(inner_res, "value", false)
        orelse inner_res;
    const n = if (val.len <= out_buf.len) val.len else out_buf.len;
    if (n > 0) @memcpy(out_buf[0..n], val[0..n]);
    const vtype = CdpFrameScanner.extractTopLevelField(inner_res, "type", true) orelse "object";
    return EvaluateResult{
        value_json: out_buf[0..n],
        value_type: vtype,
        has_exception: false,
    };
}

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
        let pbuf: [2048]u8 = undefined;
        let fbuf: [2560]u8 = undefined;
        if (_.object_id.len > 0) {
            let pos: usize = 0;
            appendRaw(&pbuf, &pos, "{\"objectId\":\"")?;
            appendJsonEscaped(&pbuf, &pos, _.object_id)?;
            appendRaw(&pbuf, &pos, "\",\"functionDeclaration\":\"")?;
            appendJsonEscaped(&pbuf, &pos, expression)?;
            appendRaw(&pbuf, &pos, "\",\"arguments\":[{\"objectId\":\"")?;
            appendJsonEscaped(&pbuf, &pos, _.object_id)?;
            appendRaw(&pbuf, &pos, "\"}],\"returnByValue\":true,\"awaitPromise\":true}")?;
            const params = pbuf[0..pos];
            const req_id = _.session.send("Runtime.callFunctionOn", params, &fbuf)?;
            if (resp := _.session.endpoint.takeResponse(req_id)) {
                return parseEvaluateResponse(resp, out_buf);
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
            if (resp := _.session.endpoint.takeResponse(req_id)) {
                return parseEvaluateResponse(resp, out_buf);
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
    resolved_obj_id_buf: [256]u8 = undefined,
    resolved_obj_id_len: usize = 0,

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
            resolved_obj_id_len: 0,
        };
    }

    pub fn nodeId(): i64 {
        return _.node_id;
    }

    pub fn isDisposed(): bool {
        return _.handle.isDisposed();
    }

    pub fn ensureObjectId(*_): []const u8 {
        if (_.resolved_obj_id_len > 0) {
            _.handle.object_id = _.resolved_obj_id_buf[0.._.resolved_obj_id_len];
            return _.handle.object_id;
        }
        if (_.handle.object_id.len > 0) return _.handle.object_id;
        if (_.node_id <= 0 || _.handle.isDisposed() || _.handle.session.detached()) return "";
        let pbuf: [256]u8 = undefined;
        let fbuf: [512]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"nodeId\":{d}}}",
            .{_.node_id},
        ) catch return "";
        const req_id = _.handle.session.send("DOM.resolveNode", params, &fbuf) catch 0;
        if (req_id > 0) {
            if (resp := _.handle.session.endpoint.takeResponse(req_id)) {
                if (!resp.is_error) {
                    if (obj_json := CdpFrameScanner.extractTopLevelField(resp.result_json, "object", false)) {
                        if (oid := CdpFrameScanner.extractTopLevelField(obj_json, "objectId", true)) {
                            if (oid.len > 0) {
                                const n = if (oid.len <= 256) oid.len else 256;
                                @memcpy(_.resolved_obj_id_buf[0..n], oid[0..n]);
                                _.resolved_obj_id_len = n;
                                _.handle.object_id = _.resolved_obj_id_buf[0..n];
                                return _.handle.object_id;
                            }
                        }
                    }
                }
            }
        }
        return "";
    }

    static fn parseQuadToBox(quad_json: []const u8): BoundingBox? {
        let coords: [8]f64 = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
        let count: usize = 0;
        let i: usize = 0;
        while (i < quad_json.len && count < 8) {
            while (i < quad_json.len && (quad_json[i] == '[' || quad_json[i] == ']' || quad_json[i] == ',' || quad_json[i] == ' ' || quad_json[i] == '\t' || quad_json[i] == '\r' || quad_json[i] == '\n')) {
                i += 1;
            }
            if (i >= quad_json.len) break;
            const start = i;
            while (i < quad_json.len && quad_json[i] != ',' && quad_json[i] != ']' && quad_json[i] != ' ' && quad_json[i] != '\t' && quad_json[i] != '\r' && quad_json[i] != '\n') {
                i += 1;
            }
            if (i > start) {
                const v = std.fmt.parseFloat(f64, quad_json[start..i]) catch return null;
                coords[count] = v;
                count += 1;
            }
        }
        if (count < 8) return null;
        let min_x = coords[0];
        let max_x = coords[0];
        let min_y = coords[1];
        let max_y = coords[1];
        let k: usize = 2;
        while (k < 8) : (k += 2) {
            if (coords[k] < min_x) min_x = coords[k];
            if (coords[k] > max_x) max_x = coords[k];
            if (coords[k + 1] < min_y) min_y = coords[k + 1];
            if (coords[k + 1] > max_y) max_y = coords[k + 1];
        }
        return BoundingBox{
            x: min_x,
            y: min_y,
            width: max_x - min_x,
            height: max_y - min_y,
        };
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
            const req_id = _.handle.session.send("DOM.getBoxModel", params, &fbuf) catch 0;
            if (req_id > 0) {
                if (resp := _.handle.session.endpoint.takeResponse(req_id)) {
                    if (resp.is_error) return null;
                    if (model_json := CdpFrameScanner.extractTopLevelField(resp.result_json, "model", false)) {
                        if (border_json := CdpFrameScanner.extractTopLevelField(model_json, "border", false)) {
                            if (parsed := ElementHandle.parseQuadToBox(border_json)) {
                                _.box = parsed;
                                return parsed;
                            }
                        }
                        if (content_json := CdpFrameScanner.extractTopLevelField(model_json, "content", false)) {
                            if (parsed := ElementHandle.parseQuadToBox(content_json)) {
                                _.box = parsed;
                                return parsed;
                            }
                        }
                    }
                }
            }
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
        const box = _.boundingBox() orelse return PuppeteerError.ElementNotFound;
        const cx = box.x + (box.width / 2.0);
        const cy = box.y + (box.height / 2.0);
        let mouse = Mouse.init(_.handle.session);
        mouse.move(cx, cy)?;
    }

    pub fn click(*_, options: ClickOptions): void!PuppeteerError {
        _.scrollIntoViewIfNeeded()?;
        const box = _.boundingBox() orelse return PuppeteerError.ElementNotFound;
        const cx = box.x + (box.width / 2.0);
        const cy = box.y + (box.height / 2.0);
        let mouse = Mouse.init(_.handle.session);
        mouse.click(cx, cy, options)?;
    }

    pub fn tap(*_): void!PuppeteerError {
        _.scrollIntoViewIfNeeded()?;
        const box = _.boundingBox() orelse return PuppeteerError.ElementNotFound;
        const cx = box.x + (box.width / 2.0);
        const cy = box.y + (box.height / 2.0);
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
        const _oid = _.ensureObjectId();
        * = _oid;
        return _.handle.evaluate(expression, out_buf);
    }

    pub fn dispose(*_): void!PuppeteerError {
        return _.handle.dispose();
    }

    ~init() {
        _.dispose() catch {};
    }
}

pub struct PuppeteerLocator {
    session: *CDPSession = undefined,
    selector_str: []const u8 = "",
    timeout_ms: u32 = 30000,
    ensure_in_viewport: bool = true,
    visibility_opt: LocatorVisibility = LocatorVisibility.Visible,
    wait_for_enabled: bool = true,
    wait_for_stable_bounding_box: bool = true,
    filter_predicate_js: []const u8 = "",

    pub static fn init(session: *CDPSession, selector_expr: []const u8): PuppeteerLocator {
        return PuppeteerLocator{
            session: session,
            selector_str: selector_expr,
            timeout_ms: 30000,
            ensure_in_viewport: true,
            visibility_opt: LocatorVisibility.Visible,
            wait_for_enabled: true,
            wait_for_stable_bounding_box: true,
            filter_predicate_js: "",
        };
    }

    pub fn selector(): []const u8 {
        return _.selector_str;
    }

    pub fn timeout(): u32 {
        return _.timeout_ms;
    }

    pub fn ensureElementIsInTheViewport(): bool {
        return _.ensure_in_viewport;
    }

    pub fn visibility(): LocatorVisibility {
        return _.visibility_opt;
    }

    pub fn waitForEnabled(): bool {
        return _.wait_for_enabled;
    }

    pub fn waitForStableBoundingBox(): bool {
        return _.wait_for_stable_bounding_box;
    }

    pub fn filterExpression(): []const u8 {
        return _.filter_predicate_js;
    }

    pub fn setTimeout(timeout_ms: u32): PuppeteerLocator {
        let next = _.*;
        next.timeout_ms = timeout_ms;
        return next;
    }

    pub fn setEnsureElementIsInTheViewport(value: bool): PuppeteerLocator {
        let next = _.*;
        next.ensure_in_viewport = value;
        return next;
    }

    pub fn setVisibility(vis: LocatorVisibility): PuppeteerLocator {
        let next = _.*;
        next.visibility_opt = vis;
        return next;
    }

    pub fn setWaitForEnabled(value: bool): PuppeteerLocator {
        let next = _.*;
        next.wait_for_enabled = value;
        return next;
    }

    pub fn setWaitForStableBoundingBox(value: bool): PuppeteerLocator {
        let next = _.*;
        next.wait_for_stable_bounding_box = value;
        return next;
    }

    pub fn filter(predicate_js: []const u8): PuppeteerLocator {
        let next = _.*;
        next.filter_predicate_js = predicate_js;
        return next;
    }

    pub fn wait(*_): ElementHandle!PuppeteerError {
        if (_.session.detached()) return PuppeteerError.SessionClosed;
        let pbuf: [1024]u8 = undefined;
        let fbuf: [1536]u8 = undefined;

        const req = DomDomain.querySelector(
            _.session.endpoint.allocateId(),
            _.session.id(),
            DomQuerySelectorParams{ node_id: 1, selector: _.selector_str },
            &pbuf,
        ) catch |e| {
            return mapCdpError(e);
        };
        const req_id = _.session.send(req.method, req.params_json, &fbuf)?;
        let node_id: i64 = 2;
        if (resp := _.session.endpoint.takeResponse(req_id)) {
            if (resp.is_error) return PuppeteerError.ElementNotFound;
            if (nid_str := CdpFrameScanner.extractTopLevelField(resp.result_json, "nodeId", false)) {
                const parsed_id = std.fmt.parseInt(i64, nid_str, 10) catch 0;
                if (parsed_id <= 0) return PuppeteerError.ElementNotFound;
                node_id = parsed_id;
            }
        }

        let el = ElementHandle.init(_.session, _.selector_str, node_id, "");
        if (_.filter_predicate_js.len > 0) {
            let expr_buf: [1024]u8 = undefined;
            let eval_buf: [256]u8 = undefined;
            const expr = std.fmt.bufPrint(
                &expr_buf,
                "(() => {{ const el = document.querySelector('{s}'); if (!el) return false; return Boolean(({s})(el)); }})()",
                .{ _.selector_str, _.filter_predicate_js },
            ) catch return PuppeteerError.BufferTooSmall;
            const _ign_flt = el.evaluate(expr, &eval_buf)?;
            * = _ign_flt;
        }
        if (_.ensure_in_viewport) {
            el.scrollIntoViewIfNeeded()?;
        }
        return el;
    }

    pub fn click(*_, options: ClickOptions): void!PuppeteerError {
        let el = _.wait()?;
        el.click(options)?;
    }

    pub fn fill(*_, value: []const u8): void!PuppeteerError {
        let el = _.wait()?;
        el.focus()?;
        let expr_buf: [1024]u8 = undefined;
        let eval_buf: [256]u8 = undefined;
        const expr = std.fmt.bufPrint(
            &expr_buf,
            "(() => {{ const el = document.querySelector('{s}'); if (el) {{ el.value = '{s}'; el.dispatchEvent(new Event('input', {{bubbles:true}})); el.dispatchEvent(new Event('change', {{bubbles:true}})); }} }})()",
            .{ _.selector_str, value },
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_fill = el.evaluate(expr, &eval_buf)?;
        * = _ign_fill;
    }

    pub fn hover(*_): void!PuppeteerError {
        let el = _.wait()?;
        el.hover()?;
    }

    pub fn scroll(*_, options: LocatorScrollOptions): void!PuppeteerError {
        let el = _.wait()?;
        let expr_buf: [1024]u8 = undefined;
        let eval_buf: [256]u8 = undefined;
        const expr = std.fmt.bufPrint(
            &expr_buf,
            "(() => {{ const el = document.querySelector('{s}'); if (el) {{ el.scrollTop = {d:.2}; el.scrollLeft = {d:.2}; }} }})()",
            .{ _.selector_str, options.scroll_top, options.scroll_left },
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_scr = el.evaluate(expr, &eval_buf)?;
        * = _ign_scr;
    }
}
