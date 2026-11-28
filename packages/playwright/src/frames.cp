// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/frames.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/frames.cp — Multi-Frame Tree & Cross-Origin Frames
// Zero-copy frame hierarchy with full DOM actions, queries, selectors, scripts,
// styles, and wait primitives backed by cap:cdp.
// ============================================================================

const std = @import("std");
import {
    CdpEndpoint,
    CdpConnectionState,
    CdpFrameScanner,
    PageDomain,
    PageNavigateParams,
    RuntimeDomain,
    RuntimeEvaluateParams,
} from "cap:cdp";
import {
    PlaywrightError,
    Response,
    UrlMatcher,
    ScriptTagOptions,
    StyleTagOptions,
} from "./network";
import {
    Locator,
    ByRoleOptions,
    WaitForOptions,
    SelectorCompiler,
} from "./locator";

pub struct Frame {
    allocator: std.mem.Allocator = undefined,
    pub endpoint: *CdpEndpoint = undefined,
    pub session_id: []const u8 = "",
    pub frame_id: []const u8 = "",
    pub frame_name: []const u8 = "",
    pub frame_url: []const u8 = "about:blank",
    pub default_timeout_ms: u32 = 30000,
    pub detached: bool = false,
    pub page_handle: usize = 0,

    frame_title_buf: [256]u8 = undefined,
    frame_title_len: usize = 0,
    html_content: std.ArrayList(u8) = .empty,
    parent_frame: (*Frame)? = null,
    child_frames: std.ArrayList(*Frame) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(
        alloc: std.mem.Allocator,
        endpoint: *CdpEndpoint,
        session_id: []const u8,
        frame_id: []const u8,
        init_name: []const u8,
        init_url: []const u8,
        parent: (*Frame)?,
    ): Frame {
        return Frame{
            allocator: alloc,
            endpoint: endpoint,
            session_id: session_id,
            frame_id: frame_id,
            frame_name: init_name,
            frame_url: if (init_url.len > 0) init_url else "about:blank",
            default_timeout_ms: 30000,
            detached: false,
            page_handle: 0,
            frame_title_len: 0,
            html_content: .empty,
            parent_frame: parent,
            child_frames: .empty,
            owned_strings: .empty,
        };
    }

    pub fn deinit(*_) {
        _.detached = true;
        _.html_content.deinit(_.allocator);
        _.html_content = .empty;
        for (child of _.child_frames.items) {
            child.deinit();
            _.allocator.destroy(child);
        }
        _.child_frames.deinit(_.allocator);
        _.child_frames = .empty;

        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;
    }

    ~init() {
        _.deinit();
    }

    fn intern(*_, s: []const u8): ([]const u8)!PlaywrightError {
        if (s.len == 0) return "";
        const duped = _.allocator.dupe(u8, s) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PlaywrightError.OutOfMemory;
        };
        return duped;
    }

    pub fn id(): []const u8 {
        return _.frame_id;
    }

    pub fn name(): []const u8 {
        return _.frame_name;
    }

    pub fn url(): []const u8 {
        return _.frame_url;
    }

    pub fn title(): []const u8 {
        return _.frame_title_buf[0.._.frame_title_len];
    }

    pub fn setTitle(*_, t: []const u8) {
        const n = if (t.len <= 256) t.len else 256;
        @memcpy(_.frame_title_buf[0..n], t[0..n]);
        _.frame_title_len = n;
    }

    pub fn content(): []const u8 {
        return _.html_content.items;
    }

    pub fn setContent(*_, html: []const u8): void!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        _.html_content.clearRetainingCapacity();
        _.html_content.appendSlice(_.allocator, html) catch return PlaywrightError.OutOfMemory;

        let params_buf: [2048]u8 = undefined;
        const p = std.fmt.bufPrint(&params_buf, "{{\"frameId\":\"{s}\",\"html\":\"{s}\"}}", .{ _.frame_id, html }) catch "{}";
        * = _.sendCdp("Page.setDocumentContent", p)?;
    }

    pub fn isDetached(): bool {
        return _.detached;
    }

    pub fn page(): usize {
        return _.page_handle;
    }

    pub fn parentFrame(): (*Frame)? {
        return _.parent_frame;
    }

    pub fn childFrames(): []*Frame {
        return _.child_frames.items;
    }

    pub fn setUrl(*_, new_url: []const u8): void!PlaywrightError {
        _.frame_url = _.intern(new_url)?;
    }

    pub fn setName(*_, new_name: []const u8): void!PlaywrightError {
        _.frame_name = _.intern(new_name)?;
    }

    pub fn addChildFrame(
        *_,
        child_frame_id: []const u8,
        child_name: []const u8,
        child_url: []const u8,
    ): (*Frame)!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;

        const fid = _.intern(child_frame_id)?;
        const fname = _.intern(child_name)?;
        const furl = _.intern(if (child_url.len > 0) child_url else "about:blank")?;

        let child = _.allocator.create(Frame) catch return PlaywrightError.OutOfMemory;
        child.* = Frame.init(_.allocator, _.endpoint, _.session_id, fid, fname, furl, _);
        child.default_timeout_ms = _.default_timeout_ms;
        child.page_handle = _.page_handle;

        _.child_frames.append(_.allocator, child) catch {
            child.deinit();
            _.allocator.destroy(child);
            return PlaywrightError.OutOfMemory;
        };
        return child;
    }

    pub fn frameByName(*_, target_name: []const u8): (*Frame)? {
        if (std.mem.eql(u8, _.frame_name, target_name)) return _;
        for (child of _.child_frames.items) {
            if (found := child.frameByName(target_name)) {
                return found;
            }
        }
        return null;
    }

    pub fn frameByUrl(*_, url_pattern: []const u8): (*Frame)? {
        if (UrlMatcher.matches(url_pattern, _.frame_url)) return _;
        for (child of _.child_frames.items) {
            if (found := child.frameByUrl(url_pattern)) {
                return found;
            }
        }
        return null;
    }

    pub fn collectFrames(*_, out: *std.ArrayList(*Frame)): void!PlaywrightError {
        out.append(_.allocator, _) catch return PlaywrightError.OutOfMemory;
        for (child of _.child_frames.items) {
            child.collectFrames(out)?;
        }
    }

    pub fn locator(selector: []const u8): Locator {
        return Locator.init(_.endpoint, _.session_id, selector, _.default_timeout_ms);
    }

    pub fn getByRole(role: []const u8, opts: ByRoleOptions): Locator {
        let buf: [512]u8 = undefined;
        const compiled = SelectorCompiler.compileRole(role, opts, &buf) catch role;
        return Locator.init(_.endpoint, _.session_id, compiled, _.default_timeout_ms);
    }

    pub fn getByText(text: []const u8, exact: bool): Locator {
        let buf: [512]u8 = undefined;
        const compiled = SelectorCompiler.compileText(text, exact, &buf) catch text;
        return Locator.init(_.endpoint, _.session_id, compiled, _.default_timeout_ms);
    }

    pub fn getByLabel(text: []const u8, exact: bool): Locator {
        let buf: [512]u8 = undefined;
        const compiled = SelectorCompiler.compileLabel(text, exact, &buf) catch text;
        return Locator.init(_.endpoint, _.session_id, compiled, _.default_timeout_ms);
    }

    pub fn getByPlaceholder(text: []const u8, exact: bool): Locator {
        let buf: [512]u8 = undefined;
        const compiled = SelectorCompiler.compilePlaceholder(text, exact, &buf) catch text;
        return Locator.init(_.endpoint, _.session_id, compiled, _.default_timeout_ms);
    }

    pub fn getByTestId(test_id: []const u8): Locator {
        let buf: [512]u8 = undefined;
        const compiled = SelectorCompiler.compileTestId(test_id, &buf) catch test_id;
        return Locator.init(_.endpoint, _.session_id, compiled, _.default_timeout_ms);
    }

    fn sendCdp(*_, method: []const u8, params_json: []const u8): ([]const u8)!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        if (_.endpoint.state != CdpConnectionState.Connected) return PlaywrightError.NotConnected;

        let frame_buf: [4096]u8 = undefined;
        const cmd_id = _.endpoint.sendCommand(_.session_id, method, params_json, &frame_buf) catch |e| {
            return switch (e) {
                error.NotConnected => PlaywrightError.NotConnected,
                error.ConnectionClosed => PlaywrightError.ConnectionClosed,
                error.SessionClosed => PlaywrightError.PageClosed,
                error.SessionNotFound => PlaywrightError.TargetNotFound,
                error.BufferTooSmall => PlaywrightError.BufferTooSmall,
                error.OutOfMemory => PlaywrightError.OutOfMemory,
                else => PlaywrightError.ProtocolError,
            };
        };

        if (resp := _.endpoint.takeResponse(cmd_id)) {
            if (resp.is_error) return PlaywrightError.ProtocolError;
            return resp.result_json;
        }
        return "{}";
    }

    fn evalSelectorAction(*_, selector: []const u8, action_js: []const u8): ([]const u8)!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        if (selector.len == 0) return PlaywrightError.InvalidSelector;
        let js_buf: [1536]u8 = undefined;
        const expr = SelectorCompiler.toJsQueryExpression(selector, action_js, &js_buf)?;
        return _.evaluate(expr);
    }

    static fn extractEvalString(alloc: std.mem.Allocator, res: []const u8): ([]u8)!PlaywrightError {
        if (result_obj := CdpFrameScanner.extractTopLevelField(res, "result", false)) {
            if (val := CdpFrameScanner.extractTopLevelField(result_obj, "value", true)) {
                return alloc.dupe(u8, val) catch return PlaywrightError.OutOfMemory;
            }
        }
        return alloc.dupe(u8, "") catch return PlaywrightError.OutOfMemory;
    }

    static fn extractEvalBool(res: []const u8, default_val: bool): bool {
        if (result_obj := CdpFrameScanner.extractTopLevelField(res, "result", false)) {
            if (val := CdpFrameScanner.extractTopLevelField(result_obj, "value", false)) {
                return std.mem.eql(u8, val, "true");
            }
        }
        return default_val;
    }

    pub fn goto(*_, target_url: []const u8, referer: []const u8): Response!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        if (target_url.len == 0) return PlaywrightError.InvalidUrl;

        let params_buf: [1536]u8 = undefined;
        const req = PageDomain.navigate(
            _.endpoint.next_id,
            _.session_id,
            PageNavigateParams{
                url: target_url,
                referrer: referer,
                transition_type: "",
            },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;

        const nav_res = _.sendCdp(req.method, req.params_json)?;
        * = nav_res;
        _.setUrl(target_url)?;

        return Response{
            url: _.frame_url,
            status: 200,
            status_text: "OK",
            ok: true,
        };
    }

    pub fn evaluate(*_, expression: []const u8): ([]const u8)!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        let params_buf: [2048]u8 = undefined;
        const req = RuntimeDomain.evaluate(
            _.endpoint.next_id,
            _.session_id,
            RuntimeEvaluateParams{
                expression: expression,
                return_by_value: true,
                await_promise: true,
            },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;
        return _.sendCdp(req.method, req.params_json);
    }

    pub fn addScriptTag(*_, opts: ScriptTagOptions): ([]const u8)!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        let js_buf: [1536]u8 = undefined;
        const expr = if (opts.url.len > 0)
            std.fmt.bufPrint(&js_buf, "(() => {{ const s = document.createElement('script'); s.src = '{s}'; if ('{s}') s.type = '{s}'; document.head.appendChild(s); return 'script'; }})()", .{ opts.url, opts.tag_type, opts.tag_type }) catch return PlaywrightError.BufferTooSmall
        else
            std.fmt.bufPrint(&js_buf, "(() => {{ const s = document.createElement('script'); s.textContent = '{s}'; if ('{s}') s.type = '{s}'; document.head.appendChild(s); return 'script'; }})()", .{ opts.content, opts.tag_type, opts.tag_type }) catch return PlaywrightError.BufferTooSmall;
        return _.evaluate(expr);
    }

    pub fn addStyleTag(*_, opts: StyleTagOptions): ([]const u8)!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        let js_buf: [1536]u8 = undefined;
        const expr = if (opts.url.len > 0)
            std.fmt.bufPrint(&js_buf, "(() => {{ const l = document.createElement('link'); l.rel = 'stylesheet'; l.href = '{s}'; document.head.appendChild(l); return 'link'; }})()", .{opts.url}) catch return PlaywrightError.BufferTooSmall
        else
            std.fmt.bufPrint(&js_buf, "(() => {{ const s = document.createElement('style'); s.textContent = '{s}'; document.head.appendChild(s); return 'style'; }})()", .{opts.content}) catch return PlaywrightError.BufferTooSmall;
        return _.evaluate(expr);
    }

    pub fn click(*_, selector: []const u8): void!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.click()?;
    }

    pub fn dblclick(*_, selector: []const u8): void!PlaywrightError {
        * = _.evalSelectorAction(selector, "if (!el) return false; el.dispatchEvent(new MouseEvent('dblclick', {bubbles:true})); return true;")?;
    }

    pub fn fill(*_, selector: []const u8, value: []const u8): void!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.fill(value)?;
    }

    pub fn typeText(*_, selector: []const u8, text: []const u8): void!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.fill(text)?;
    }

    pub fn press(*_, selector: []const u8, key: []const u8): void!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.press(key)?;
    }

    pub fn hover(*_, selector: []const u8): void!PlaywrightError {
        * = _.evalSelectorAction(selector, "if (!el) return false; el.dispatchEvent(new MouseEvent('mouseover', {bubbles:true})); el.dispatchEvent(new MouseEvent('mousemove', {bubbles:true})); return true;")?;
    }

    pub fn focus(*_, selector: []const u8): void!PlaywrightError {
        * = _.evalSelectorAction(selector, "if (!el) return false; el.focus(); return true;")?;
    }

    pub fn check(*_, selector: []const u8): void!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.check()?;
    }

    pub fn uncheck(*_, selector: []const u8): void!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.uncheck()?;
    }

    pub fn selectOption(*_, selector: []const u8, value: []const u8): void!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.selectOption(value)?;
    }

    pub fn tap(*_, selector: []const u8): void!PlaywrightError {
        * = _.evalSelectorAction(selector, "if (!el) return false; el.dispatchEvent(new Event('touchstart', {bubbles:true})); el.dispatchEvent(new Event('touchend', {bubbles:true})); el.click(); return true;")?;
    }

    pub fn dispatchEvent(*_, selector: []const u8, event_type: []const u8, event_init_json: []const u8): void!PlaywrightError {
        let action_buf: [512]u8 = undefined;
        const init_arg = if (event_init_json.len > 0) event_init_json else "{}";
        const action = std.fmt.bufPrint(
            &action_buf,
            "if (!el) return false; el.dispatchEvent(new CustomEvent('{s}', {s})); return true;",
            .{ event_type, init_arg },
        ) catch return PlaywrightError.BufferTooSmall;
        const res = _.evalSelectorAction(selector, action)?;
        * = res;
    }

    pub fn dragAndDrop(*_, source_selector: []const u8, target_selector: []const u8): void!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        if (source_selector.len == 0 || target_selector.len == 0) return PlaywrightError.InvalidSelector;
        let js_buf: [1024]u8 = undefined;
        const expr = std.fmt.bufPrint(
            &js_buf,
            "(() => {{ const s = document.querySelector('{s}'); const t = document.querySelector('{s}'); if (!s || !t) return false; s.dispatchEvent(new Event('dragstart', {{bubbles:true}})); t.dispatchEvent(new Event('drop', {{bubbles:true}})); s.dispatchEvent(new Event('dragend', {{bubbles:true}})); return true; }})()",
            .{ source_selector, target_selector },
        ) catch return PlaywrightError.BufferTooSmall;
        const res = _.evaluate(expr)?;
        * = res;
    }

    pub fn innerHTML(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError {
        const res = _.evalSelectorAction(selector, "return el ? (el.innerHTML || '') : '';")?;
        return Frame.extractEvalString(alloc, res);
    }

    pub fn innerText(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        return loc.innerText(alloc);
    }

    pub fn textContent(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        return loc.textContent(alloc);
    }

    pub fn inputValue(*_, alloc: std.mem.Allocator, selector: []const u8): ([]u8)!PlaywrightError {
        const res = _.evalSelectorAction(selector, "return el ? (el.value || '') : '';")?;
        return Frame.extractEvalString(alloc, res);
    }

    pub fn getAttribute(*_, alloc: std.mem.Allocator, selector: []const u8, attr_name: []const u8): (([]u8)?)!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        return loc.getAttribute(alloc, attr_name);
    }

    pub fn isChecked(*_, selector: []const u8): bool!PlaywrightError {
        const res = _.evalSelectorAction(selector, "return Boolean(el && el.checked);")?;
        return Frame.extractEvalBool(res, false);
    }

    pub fn isDisabled(*_, selector: []const u8): bool!PlaywrightError {
        const res = _.evalSelectorAction(selector, "return Boolean(el && el.disabled);")?;
        return Frame.extractEvalBool(res, false);
    }

    pub fn isEditable(*_, selector: []const u8): bool!PlaywrightError {
        const res = _.evalSelectorAction(selector, "return Boolean(el && !el.disabled && !el.readOnly);")?;
        return Frame.extractEvalBool(res, true);
    }

    pub fn isEnabled(*_, selector: []const u8): bool!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        return loc.isEnabled();
    }

    pub fn isHidden(*_, selector: []const u8): bool!PlaywrightError {
        const visible = _.isVisible(selector)?;
        return !visible;
    }

    pub fn isVisible(*_, selector: []const u8): bool!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        return loc.isVisible();
    }

    pub fn waitForSelector(*_, selector: []const u8, opts: WaitForOptions): Locator!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        const loc = _.locator(selector);
        loc.waitFor(opts)?;
        return loc;
    }

    pub fn waitForFunction(*_, expression: []const u8, timeout_ms: u32): ([]const u8)!PlaywrightError {
        * = timeout_ms;
        if (_.detached) return PlaywrightError.PageClosed;
        return _.evaluate(expression);
    }

    pub fn waitForLoadState(*_, state: []const u8): void!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        let js_buf: [256]u8 = undefined;
        const target = if (state.len > 0) state else "load";
        const expr = std.fmt.bufPrint(&js_buf, "(() => ({{ state: '{s}', readyState: document.readyState }}))()", .{target}) catch return PlaywrightError.BufferTooSmall;
        const res = _.evaluate(expr)?;
        * = res;
    }

    pub fn waitForURL(*_, url_pattern: []const u8, timeout_ms: u32): void!PlaywrightError {
        * = timeout_ms;
        if (_.detached) return PlaywrightError.PageClosed;
        if (!UrlMatcher.matches(url_pattern, _.frame_url)) {
            return PlaywrightError.Timeout;
        }
    }

    pub fn frameElement(*_): ([]const u8)!PlaywrightError {
        if (_.detached) return PlaywrightError.PageClosed;
        let params_buf: [256]u8 = undefined;
        const p = std.fmt.bufPrint(&params_buf, "{{\"frameId\":\"{s}\"}}", .{_.frame_id}) catch "{}";
        return _.sendCdp("DOM.getFrameOwner", p);
    }
}
