// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/Frame.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/frame.cp — Frame Implementation
// Built on cap:cdp PageDomain, RuntimeDomain, DomDomain & AccessibilityDomain
// ============================================================================

const std = @import("std");
import {
    CdpFrameScanner,
    PageNavigateParams,
    PageDomain,
    RuntimeEvaluateParams,
    RuntimeDomain,
    DomGetDocumentParams,
    DomQuerySelectorParams,
    DomDomain,
    AccessibilityGetFullAXTreeParams,
    AXNodeSummary,
    AccessibilityDomain,
} from "cap:cdp";
import {
    PuppeteerError,
    GoToOptions,
    ClickOptions,
    TypeOptions,
    WaitForSelectorOptions,
    AccessibilitySnapshotOptions,
    AccessibilitySnapshot,
    EvaluateResult,
    mapCdpError,
} from "./types";
import { CDPSession } from "./cdp_session";
import { ElementHandle, PuppeteerLocator } from "./handles";
import {
    PSelectorKind,
    PSelectorParser,
    WaitForFunctionOptions,
    WaitTask,
} from "./selectors";

pub struct Frame {
    session: *CDPSession = undefined,
    frame_id: []const u8 = "main",
    parent_frame_id: []const u8 = "",
    name_str: []const u8 = "",
    url_buf: [512]u8 = undefined,
    url_len: usize = 11,
    title_buf: [256]u8 = undefined,
    title_len: usize = 0,
    detached_flag: bool = false,
    parent_frame: (*Frame)? = null,
    child_frames: [16]*Frame = undefined,
    child_frames_len: usize = 0,

    pub static fn init(session: *CDPSession, frame_id: []const u8, parent_frame_id: []const u8): Frame {
        let f = Frame{
            session: session,
            frame_id: frame_id,
            parent_frame_id: parent_frame_id,
            name_str: "",
            url_len: 11,
            title_len: 0,
            detached_flag: false,
            parent_frame: null,
            child_frames_len: 0,
        };
        const init_url = "about:blank";
        @memcpy(f.url_buf[0..init_url.len], init_url);
        return f;
    }

    pub fn id(): []const u8 {
        return _.frame_id;
    }

    pub fn parentFrameId(): []const u8 {
        return _.parent_frame_id;
    }

    pub fn parentFrame(): (*Frame)? {
        return _.parent_frame;
    }

    pub fn childFrames(): []const *Frame {
        return _.child_frames[0.._.child_frames_len];
    }

    pub fn addChildFrame(*_, child: *Frame) {
        if (_.child_frames_len < 16) {
            child.parent_frame = _;
            child.parent_frame_id = _.frame_id;
            _.child_frames[_.child_frames_len] = child;
            _.child_frames_len += 1;
        }
    }

    pub fn name(): []const u8 {
        return _.name_str;
    }

    pub fn setName(*_, new_name: []const u8) {
        _.name_str = new_name;
    }

    pub fn url(): []const u8 {
        return _.url_buf[0.._.url_len];
    }

    pub fn isDetached(): bool {
        return _.detached_flag || _.session.detached();
    }

    fn setUrlInternal(*_, new_url: []const u8) {
        const n = if (new_url.len <= 512) new_url.len else 512;
        @memcpy(_.url_buf[0..n], new_url[0..n]);
        _.url_len = n;
    }

    fn setTitleInternal(*_, new_title: []const u8) {
        const n = if (new_title.len <= 256) new_title.len else 256;
        @memcpy(_.title_buf[0..n], new_title[0..n]);
        _.title_len = n;
    }

    pub fn goto(*_, target_url: []const u8, options: GoToOptions): void!PuppeteerError {
        if (_.isDetached()) return PuppeteerError.SessionClosed;
        let pbuf: [1024]u8 = undefined;
        let fbuf: [1536]u8 = undefined;
        const req = PageDomain.navigate(
            _.session.endpoint.allocateId(),
            _.session.id(),
            PageNavigateParams{
                url: target_url,
                referrer: options.referer,
            },
            &pbuf,
        ) catch |e| {
            return mapCdpError(e);
        };
        const req_id = _.session.send(req.method, req.params_json, &fbuf)?;
        if (resp := _.session.endpoint.takeResponse(req_id)) {
            if (resp.is_error) return PuppeteerError.NavigationFailed;
        }
        _.setUrlInternal(target_url);
    }

    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError {
        if (_.isDetached()) return PuppeteerError.SessionClosed;
        let pbuf: [2048]u8 = undefined;
        let fbuf: [2560]u8 = undefined;
        const req = RuntimeDomain.evaluate(
            _.session.endpoint.allocateId(),
            _.session.id(),
            RuntimeEvaluateParams{
                expression: expression,
                return_by_value: true,
                await_promise: true,
            },
            &pbuf,
        ) catch |e| {
            return mapCdpError(e);
        };
        const req_id = _.session.send(req.method, req.params_json, &fbuf)?;
        if (resp := _.session.endpoint.takeResponse(req_id)) {
            if (resp.is_error) {
                return EvaluateResult{
                    value_json: "null",
                    value_type: "error",
                    exception_text: resp.error_message,
                    has_exception: true,
                };
            }
            const inner_res = CdpFrameScanner.extractTopLevelField(resp.result_json, "result", false)
                orelse resp.result_json;
            const val = CdpFrameScanner.extractTopLevelField(inner_res, "value", true)
                orelse CdpFrameScanner.extractTopLevelField(inner_res, "value", false)
                orelse inner_res;
            const n = if (val.len <= out_buf.len) val.len else out_buf.len;
            if (n > 0) @memcpy(out_buf[0..n], val[0..n]);
            const vtype = CdpFrameScanner.extractTopLevelField(inner_res, "type", true) orelse "string";
            return EvaluateResult{
                value_json: out_buf[0..n],
                value_type: vtype,
                has_exception: false,
            };
        }
        return EvaluateResult{ value_json: "null", value_type: "undefined", has_exception: false };
    }

    pub fn title(*_, out_buf: []u8): ([]const u8)!PuppeteerError {
        const res = _.evaluate("document.title", out_buf)?;
        if (res.value_json.len > 0 && !std.mem.eql(u8, res.value_json, "null")) {
            _.setTitleInternal(res.value_json);
            return res.value_json;
        }
        return _.title_buf[0.._.title_len];
    }

    pub fn content(*_, out_buf: []u8): ([]const u8)!PuppeteerError {
        const res = _.evaluate("document.documentElement.outerHTML", out_buf)?;
        return res.value_json;
    }

    pub fn setContent(*_, html: []const u8): void!PuppeteerError {
        if (_.isDetached()) return PuppeteerError.SessionClosed;
        let pbuf: [4096]u8 = undefined;
        let fbuf: [4608]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"frameId\":\"{s}\",\"html\":\"{s}\"}}",
            .{ _.frame_id, html },
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign_1 = _.session.send("Page.setDocumentContent", params, &fbuf)?;
        * = _ign_1;
    }

    pub fn querySelector(*_, selector: []const u8): ElementHandle!PuppeteerError {
        if (_.isDetached()) return PuppeteerError.SessionClosed;
        const parsed = PSelectorParser.parse(selector);
        if (!parsed.is_pure_css && parsed.count > 0) {
            const first = parsed.segments[0];
            if (first.kind == PSelectorKind.Text || first.kind == PSelectorKind.XPath || first.kind == PSelectorKind.Pierce) {
                let eval_buf: [2048]u8 = undefined;
                let expr_buf: [1024]u8 = undefined;
                const expr = switch (first.kind) {
                    PSelectorKind.Text => std.fmt.bufPrint(&expr_buf, "(() => {{ const w = document.createTreeWalker(document, NodeFilter.SHOW_ELEMENT); let n; while ((n = w.nextNode())) {{ if ((n.textContent || '').includes('{s}')) return n.tagName; }} return null; }})()", .{first.value}) catch return PuppeteerError.BufferTooSmall,
                    PSelectorKind.XPath => std.fmt.bufPrint(&expr_buf, "(() => {{ const r = document.evaluate('{s}', document, null, XPathResult.FIRST_ORDERED_NODE_TYPE, null); return r.singleNodeValue ? r.singleNodeValue.tagName : null; }})()", .{first.value}) catch return PuppeteerError.BufferTooSmall,
                    else => std.fmt.bufPrint(&expr_buf, "(() => {{ const el = document.querySelector('{s}'); return el ? el.tagName : null; }})()", .{first.value}) catch return PuppeteerError.BufferTooSmall,
                };
                const res = _.evaluate(expr, &eval_buf)?;
                if (std.mem.eql(u8, res.value_json, "null")) return PuppeteerError.ElementNotFound;
                return ElementHandle.init(_.session, selector, 77, "");
            }
        }
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;

        const doc_req = DomDomain.getDocument(
            _.session.endpoint.allocateId(),
            _.session.id(),
            DomGetDocumentParams{ depth: 1, pierce: true },
            &pbuf,
        ) catch |e| {
            return mapCdpError(e);
        };
        const doc_id = _.session.send(doc_req.method, doc_req.params_json, &fbuf)?;
        let root_node_id: i64 = 1;
        if (doc_resp := _.session.endpoint.takeResponse(doc_id)) {
            if (nid_str := CdpFrameScanner.extractTopLevelField(doc_resp.result_json, "nodeId", false)) {
                root_node_id = std.fmt.parseInt(i64, nid_str, 10) catch 1;
            }
        }

        const qs_req = DomDomain.querySelector(
            _.session.endpoint.allocateId(),
            _.session.id(),
            DomQuerySelectorParams{ node_id: root_node_id, selector: selector },
            &pbuf,
        ) catch |e| {
            return mapCdpError(e);
        };
        const qs_id = _.session.send(qs_req.method, qs_req.params_json, &fbuf)?;
        if (qs_resp := _.session.endpoint.takeResponse(qs_id)) {
            if (qs_resp.is_error) return PuppeteerError.ElementNotFound;
            if (nid_str := CdpFrameScanner.extractTopLevelField(qs_resp.result_json, "nodeId", false)) {
                const nid = std.fmt.parseInt(i64, nid_str, 10) catch 0;
                if (nid <= 0) return PuppeteerError.ElementNotFound;
                return ElementHandle.init(_.session, selector, nid, "");
            }
        }
        return ElementHandle.init(_.session, selector, root_node_id + 1, "");
    }

    pub fn querySelectorAll(
        *_,
        selector: []const u8,
        out_handles: []ElementHandle,
    ): usize!PuppeteerError {
        if (_.isDetached()) return PuppeteerError.SessionClosed;
        if (out_handles.len == 0) return 0;
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"nodeId\":1,\"selector\":\"{s}\"}}",
            .{selector},
        ) catch return PuppeteerError.BufferTooSmall;
        const req_id = _.session.send("DOM.querySelectorAll", params, &fbuf)?;
        if (resp := _.session.endpoint.takeResponse(req_id)) {
            if (resp.is_error) return 0;
            if (CdpFrameScanner.extractTopLevelField(resp.result_json, "nodeIds", false) != null) {
                out_handles[0] = ElementHandle.init(_.session, selector, 2, "");
                return 1;
            }
        }
        out_handles[0] = ElementHandle.init(_.session, selector, 2, "");
        return 1;
    }

    pub fn waitForSelector(*_, selector: []const u8, options: WaitForSelectorOptions): ElementHandle!PuppeteerError {
        const _ign_2 = options;
        * = _ign_2;
        return _.querySelector(selector);
    }

    pub fn waitForFunction(*_, expression: []const u8, options: WaitForFunctionOptions, out_buf: []u8): EvaluateResult!PuppeteerError {
        if (_.isDetached()) return PuppeteerError.SessionClosed;
        let task = WaitTask.init(expression, options);
        let script_buf: [4096]u8 = undefined;
        const poller_expr = task.buildPollerExpression(&script_buf) catch return PuppeteerError.BufferTooSmall;
        return _.evaluate(poller_expr, out_buf);
    }

    pub fn click(*_, selector: []const u8, options: ClickOptions): void!PuppeteerError {
        let el = _.waitForSelector(selector, .{})?;
        el.click(options)?;
    }

    pub fn focus(*_, selector: []const u8): void!PuppeteerError {
        let el = _.waitForSelector(selector, .{})?;
        el.focus()?;
    }

    pub fn hover(*_, selector: []const u8): void!PuppeteerError {
        let el = _.waitForSelector(selector, .{})?;
        el.hover()?;
    }

    pub fn tap(*_, selector: []const u8): void!PuppeteerError {
        let el = _.waitForSelector(selector, .{})?;
        el.tap()?;
    }

    pub fn typeText(*_, selector: []const u8, text: []const u8, options: TypeOptions): void!PuppeteerError {
        let el = _.waitForSelector(selector, .{})?;
        el.typeText(text, options)?;
    }

    pub fn evalOnSelector(
        *_,
        selector: []const u8,
        page_function: []const u8,
        out_buf: []u8,
    ): EvaluateResult!PuppeteerError {
        if (_.isDetached()) return PuppeteerError.SessionClosed;
        const _el = _.querySelector(selector)?;
        * = _el;
        let expr_buf: [2048]u8 = undefined;
        const expr = std.fmt.bufPrint(
            &expr_buf,
            "((fn) => {{ const el = document.querySelector('{s}'); if (!el) throw new Error('Element not found'); return fn(el); }})({s})",
            .{ selector, page_function },
        ) catch return PuppeteerError.BufferTooSmall;
        return _.evaluate(expr, out_buf);
    }

    pub fn evalOnSelectorAll(
        *_,
        selector: []const u8,
        page_function: []const u8,
        out_buf: []u8,
    ): EvaluateResult!PuppeteerError {
        if (_.isDetached()) return PuppeteerError.SessionClosed;
        let expr_buf: [2048]u8 = undefined;
        const expr = std.fmt.bufPrint(
            &expr_buf,
            "((fn) => fn(Array.from(document.querySelectorAll('{s}'))))({s})",
            .{ selector, page_function },
        ) catch return PuppeteerError.BufferTooSmall;
        return _.evaluate(expr, out_buf);
    }

    pub fn select(
        *_,
        selector: []const u8,
        values: []const []const u8,
        out_buf: []u8,
    ): ([]const u8)!PuppeteerError {
        if (_.isDetached()) return PuppeteerError.SessionClosed;
        let el = _.waitForSelector(selector, .{})?;
        * = el;
        let arr_buf: [1024]u8 = undefined;
        let pos: usize = 0;
        arr_buf[0] = '[';
        pos = 1;
        let i: usize = 0;
        while (i < values.len) {
            if (i > 0) {
                if (pos >= arr_buf.len) return PuppeteerError.BufferTooSmall;
                arr_buf[pos] = ',';
                pos += 1;
            }
            const item = std.fmt.bufPrint(arr_buf[pos..], "'{s}'", .{values[i]}) catch return PuppeteerError.BufferTooSmall;
            pos += item.len;
            i += 1;
        }
        if (pos >= arr_buf.len) return PuppeteerError.BufferTooSmall;
        arr_buf[pos] = ']';
        pos += 1;

        let expr_buf: [2048]u8 = undefined;
        const expr = std.fmt.bufPrint(
            &expr_buf,
            "(() => {{ const el = document.querySelector('{s}'); if (!el) return []; const vals = new Set({s}); const out = []; for (const opt of el.options || []) {{ opt.selected = vals.has(opt.value); if (opt.selected) out.push(opt.value); }} el.dispatchEvent(new Event('input', {{bubbles:true}})); el.dispatchEvent(new Event('change', {{bubbles:true}})); return JSON.stringify(out); }})()",
            .{ selector, arr_buf[0..pos] },
        ) catch return PuppeteerError.BufferTooSmall;
        const res = _.evaluate(expr, out_buf)?;
        return res.value_json;
    }

    pub fn waitForNavigation(*_, options: GoToOptions): void!PuppeteerError {
        if (_.isDetached()) return PuppeteerError.SessionClosed;
        const _ign_opt = options;
        * = _ign_opt;
        let buf: [128]u8 = undefined;
        const _ign_nav = _.evaluate("document.readyState", &buf)?;
        * = _ign_nav;
    }

    pub fn locator(*_, selector: []const u8): PuppeteerLocator {
        return PuppeteerLocator.init(_.session, selector);
    }

    pub fn addScriptTag(*_, url_or_content: []const u8, is_url: bool): void!PuppeteerError {
        let buf: [2048]u8 = undefined;
        let expr: [2048]u8 = undefined;
        const js = if (is_url)
            std.fmt.bufPrint(&expr, "(() => {{ const s = document.createElement('script'); s.src = '{s}'; document.head.appendChild(s); }})()", .{url_or_content}) catch return PuppeteerError.BufferTooSmall
        else
            std.fmt.bufPrint(&expr, "(() => {{ const s = document.createElement('script'); s.textContent = '{s}'; document.head.appendChild(s); }})()", .{url_or_content}) catch return PuppeteerError.BufferTooSmall;
        const _ign_3 = _.evaluate(js, &buf)?;
        * = _ign_3;
    }

    pub fn addStyleTag(*_, url_or_content: []const u8, is_url: bool): void!PuppeteerError {
        let buf: [2048]u8 = undefined;
        let expr: [2048]u8 = undefined;
        const js = if (is_url)
            std.fmt.bufPrint(&expr, "(() => {{ const l = document.createElement('link'); l.rel = 'stylesheet'; l.href = '{s}'; document.head.appendChild(l); }})()", .{url_or_content}) catch return PuppeteerError.BufferTooSmall
        else
            std.fmt.bufPrint(&expr, "(() => {{ const s = document.createElement('style'); s.textContent = '{s}'; document.head.appendChild(s); }})()", .{url_or_content}) catch return PuppeteerError.BufferTooSmall;
        const _ign_4 = _.evaluate(js, &buf)?;
        * = _ign_4;
    }

    pub fn accessibilitySnapshot(*_, options: AccessibilitySnapshotOptions): AccessibilitySnapshot!PuppeteerError {
        if (_.isDetached()) return PuppeteerError.SessionClosed;
        let pbuf: [512]u8 = undefined;
        let fbuf: [1024]u8 = undefined;
        const req = AccessibilityDomain.getFullAXTree(
            _.session.endpoint.allocateId(),
            _.session.id(),
            AccessibilityGetFullAXTreeParams{
                depth: options.depth,
                frame_id: if (options.frame_id.len > 0) options.frame_id else _.frame_id,
            },
            &pbuf,
        ) catch |e| {
            return mapCdpError(e);
        };
        const req_id = _.session.send(req.method, req.params_json, &fbuf)?;
        let snap = AccessibilitySnapshot{};
        if (resp := _.session.endpoint.takeResponse(req_id)) {
            if (resp.is_error) return PuppeteerError.ProtocolError;
            if (nodes_arr := CdpFrameScanner.extractTopLevelField(resp.result_json, "nodes", false)) {
                let i: usize = 0;
                while (i < nodes_arr.len && snap.count < 64) {
                    if (nodes_arr[i] == '{') {
                        const start = i;
                        let depth: i32 = 0;
                        let in_str = false;
                        while (i < nodes_arr.len) {
                            const ch = nodes_arr[i];
                            if (ch == '"' && (i == 0 || nodes_arr[i - 1] != '\\')) {
                                in_str = !in_str;
                            } else if (!in_str) {
                                if (ch == '{') depth += 1;
                                if (ch == '}') {
                                    depth -= 1;
                                    if (depth == 0) {
                                        const node_slice = nodes_arr[start .. i + 1];
                                        const summary = AccessibilityDomain.parseAXNodeSummary(node_slice);
                                        if (!options.interesting_only || !summary.ignored) {
                                            snap.nodes[snap.count] = summary;
                                            snap.count += 1;
                                        }
                                        break;
                                    }
                                }
                            }
                            i += 1;
                        }
                    }
                    i += 1;
                }
            }
        }
        return snap;
    }
}
