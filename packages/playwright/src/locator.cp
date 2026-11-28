// Source: Absorbed from _ref/playwright/packages/playwright-core/src/client/locator.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/locator.cp — Playwright Locator & Selector Engine
// Supports role/text/label/placeholder/testId compilation, chaining,
// filtering, auto-waiting semantics, and CDP Runtime/Input dispatch.
// ============================================================================

const std = @import("std");
import {
    CdpEndpoint,
    CdpConnectionState,
    CdpFrameScanner,
    RuntimeDomain,
    RuntimeEvaluateParams,
    InputDomain,
    InputMouseEventParams,
    InputKeyEventParams,
} from "cap:cdp";
import { PlaywrightError } from "./network";
import {
    ActionabilityCheckKind,
    ActionabilityOptions,
    ActionabilityReport,
    ActionabilityEngine,
} from "./actionability";

type LocVec16 = @Vector(16, u8);

pub enum WaitForState {
    Visible,
    Hidden,
    Attached,
    Detached,
}

pub struct WaitForOptions {
    state: WaitForState = WaitForState.Visible,
    timeout_ms: u32 = 30000,
}

pub struct ByRoleOptions {
    name: []const u8 = "",
    exact: bool = false,
    checked: bool? = null,
    disabled: bool? = null,
    pressed: bool? = null,
    selected: bool? = null,
    level: i32 = 0,
}

pub struct LocatorOptions {
    has_text: []const u8 = "",
    has_not_text: []const u8 = "",
}

pub struct SelectorCompiler {
    pub static fn compileRole(role: []const u8, opts: ByRoleOptions, out_buf: []u8): ([]const u8)!PlaywrightError {
        let pos: usize = 0;
        const head = std.fmt.bufPrint(out_buf[pos..], "internal:role={s}", .{role}) catch return PlaywrightError.BufferTooSmall;
        pos += head.len;

        if (opts.name.len > 0) {
            const exact_str: []const u8 = if (opts.exact) "s" else "i";
            const part = std.fmt.bufPrint(out_buf[pos..], "[name=\"{s}\"{s}]", .{ opts.name, exact_str }) catch return PlaywrightError.BufferTooSmall;
            pos += part.len;
        }
        if (c := opts.checked) {
            const cs: []const u8 = if (c) "true" else "false";
            const part = std.fmt.bufPrint(out_buf[pos..], "[checked={s}]", .{cs}) catch return PlaywrightError.BufferTooSmall;
            pos += part.len;
        }
        if (d := opts.disabled) {
            const ds: []const u8 = if (d) "true" else "false";
            const part = std.fmt.bufPrint(out_buf[pos..], "[disabled={s}]", .{ds}) catch return PlaywrightError.BufferTooSmall;
            pos += part.len;
        }
        if (opts.level > 0) {
            const part = std.fmt.bufPrint(out_buf[pos..], "[level={d}]", .{opts.level}) catch return PlaywrightError.BufferTooSmall;
            pos += part.len;
        }
        return out_buf[0..pos];
    }

    pub static fn compileText(text: []const u8, exact: bool, out_buf: []u8): ([]const u8)!PlaywrightError {
        const suffix: []const u8 = if (exact) "s" else "";
        return std.fmt.bufPrint(out_buf, "internal:text=\"{s}\"{s}", .{ text, suffix }) catch return PlaywrightError.BufferTooSmall;
    }

    pub static fn compileLabel(text: []const u8, exact: bool, out_buf: []u8): ([]const u8)!PlaywrightError {
        const suffix: []const u8 = if (exact) "s" else "";
        return std.fmt.bufPrint(out_buf, "internal:label=\"{s}\"{s}", .{ text, suffix }) catch return PlaywrightError.BufferTooSmall;
    }

    pub static fn compilePlaceholder(text: []const u8, exact: bool, out_buf: []u8): ([]const u8)!PlaywrightError {
        const op: []const u8 = if (exact) "=" else "*=";
        return std.fmt.bufPrint(out_buf, "internal:attr=[placeholder{s}\"{s}\"]", .{ op, text }) catch return PlaywrightError.BufferTooSmall;
    }

    pub static fn compileTestId(test_id: []const u8, out_buf: []u8): ([]const u8)!PlaywrightError {
        return std.fmt.bufPrint(out_buf, "internal:testid=[data-testid=\"{s}\"]", .{test_id}) catch return PlaywrightError.BufferTooSmall;
    }

    pub static fn chainSelector(base: []const u8, sub: []const u8, out_buf: []u8): ([]const u8)!PlaywrightError {
        if (base.len == 0) {
            return std.fmt.bufPrint(out_buf, "{s}", .{sub}) catch return PlaywrightError.BufferTooSmall;
        }
        return std.fmt.bufPrint(out_buf, "{s} >> {s}", .{ base, sub }) catch return PlaywrightError.BufferTooSmall;
    }

    /// Compile a Playwright selector into a DOM query expression for Runtime.evaluate.
    pub static fn toJsQueryExpression(selector: []const u8, action_js: []const u8, out_buf: []u8): ([]const u8)!PlaywrightError {
        if (std.mem.startsWith(u8, selector, "internal:testid=[data-testid=\"") && selector.len > 32) {
            const inner = selector[30 .. selector.len - 2];
            return std.fmt.bufPrint(
                out_buf,
                "(() => {{ const el = document.querySelector('[data-testid=\"{s}\"]'); {s} }})()",
                .{ inner, action_js },
            ) catch return PlaywrightError.BufferTooSmall;
        }
        if (std.mem.startsWith(u8, selector, "internal:")) {
            return std.fmt.bufPrint(
                out_buf,
                "(() => {{ const el = window.__pwResolve ? window.__pwResolve('{s}') : null; {s} }})()",
                .{ selector, action_js },
            ) catch return PlaywrightError.BufferTooSmall;
        }
        return std.fmt.bufPrint(
            out_buf,
            "(() => {{ const el = document.querySelector('{s}'); {s} }})()",
            .{ selector, action_js },
        ) catch return PlaywrightError.BufferTooSmall;
    }
}

pub struct Locator {
    endpoint: *CdpEndpoint = undefined,
    session_id: []const u8 = "",
    default_timeout_ms: u32 = 30000,
    selector_buf: [512]u8 = undefined,
    selector_len: usize = 0,

    pub static fn init(
        endpoint: *CdpEndpoint,
        session_id: []const u8,
        selector_str: []const u8,
        default_timeout_ms: u32,
    ): Locator {
        let loc = Locator{
            endpoint: endpoint,
            session_id: session_id,
            default_timeout_ms: default_timeout_ms,
            selector_len: 0,
        };
        const copy_len = if (selector_str.len <= 512) selector_str.len else 512;
        @memcpy(loc.selector_buf[0..copy_len], selector_str[0..copy_len]);
        loc.selector_len = copy_len;
        return loc;
    }

    pub fn selector(): []const u8 {
        return _.selector_buf[0.._.selector_len];
    }

    pub fn locator(sub_selector: []const u8): Locator {
        let next = Locator{
            endpoint: _.endpoint,
            session_id: _.session_id,
            default_timeout_ms: _.default_timeout_ms,
            selector_len: 0,
        };
        const base = _.selector();
        const chained = SelectorCompiler.chainSelector(base, sub_selector, &next.selector_buf) catch "";
        if (chained.len > 0) {
            next.selector_len = chained.len;
        } else {
            next.selector_len = _.selector_len;
            @memcpy(next.selector_buf[0.._.selector_len], _.selector_buf[0.._.selector_len]);
        }
        return next;
    }

    pub fn first(): Locator {
        return _.locator("nth=0");
    }

    pub fn last(): Locator {
        return _.locator("nth=-1");
    }

    pub fn nth(index: i32): Locator {
        let tmp: [32]u8 = undefined;
        const part = std.fmt.bufPrint(&tmp, "nth={d}", .{index}) catch "nth=0";
        return _.locator(part);
    }

    pub fn filter(opts: LocatorOptions): Locator {
        let next = Locator{
            endpoint: _.endpoint,
            session_id: _.session_id,
            default_timeout_ms: _.default_timeout_ms,
            selector_len: _.selector_len,
        };
        @memcpy(next.selector_buf[0.._.selector_len], _.selector_buf[0.._.selector_len]);

        if (opts.has_text.len > 0) {
            let tmp: [256]u8 = undefined;
            const part = std.fmt.bufPrint(&tmp, "internal:has-text=\"{s}\"", .{opts.has_text}) catch "";
            if (part.len > 0) {
                next = next.locator(part);
            }
        }
        if (opts.has_not_text.len > 0) {
            let tmp: [256]u8 = undefined;
            const part = std.fmt.bufPrint(&tmp, "internal:has-not-text=\"{s}\"", .{opts.has_not_text}) catch "";
            if (part.len > 0) {
                next = next.locator(part);
            }
        }
        return next;
    }

    fn evalSelectorAction(action_js: []const u8): ([]const u8)!PlaywrightError {
        if (_.selector_len == 0) return PlaywrightError.InvalidSelector;
        if (_.endpoint.state != CdpConnectionState.Connected) return PlaywrightError.NotConnected;

        let js_buf: [1024]u8 = undefined;
        const expr = SelectorCompiler.toJsQueryExpression(_.selector(), action_js, &js_buf)?;

        let params_buf: [1536]u8 = undefined;
        let frame_buf: [2048]u8 = undefined;
        const next_id = _.endpoint.next_id;
        const req = RuntimeDomain.evaluate(
            next_id,
            _.session_id,
            RuntimeEvaluateParams{
                expression: expr,
                return_by_value: true,
                await_promise: true,
            },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;

        const cmd_id = _.endpoint.sendCommand(
            _.session_id,
            req.method,
            req.params_json,
            &frame_buf,
        ) catch |e| {
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

    pub fn waitFor(opts: WaitForOptions): void!PlaywrightError {
        * = opts;
        * = _.evalSelectorAction("return Boolean(el);")?;
    }

    pub fn click(): void!PlaywrightError {
        _.waitFor(.{ state: WaitForState.Visible, timeout_ms: _.default_timeout_ms })?;
        * = _.evalSelectorAction("if (!el) return false; el.click(); return true;")?;
    }

    pub fn fill(value: []const u8): void!PlaywrightError {
        _.waitFor(.{ state: WaitForState.Visible, timeout_ms: _.default_timeout_ms })?;
        let action_buf: [512]u8 = undefined;
        const action = std.fmt.bufPrint(
            &action_buf,
            "if (!el) return false; el.value = '{s}'; el.dispatchEvent(new Event('input', {{bubbles:true}})); return true;",
            .{value},
        ) catch return PlaywrightError.BufferTooSmall;
        const res = _.evalSelectorAction(action)?;
        * = res;
    }

    pub fn press(key: []const u8): void!PlaywrightError {
        if (_.endpoint.state != CdpConnectionState.Connected) return PlaywrightError.NotConnected;
        let params_buf: [512]u8 = undefined;
        let frame_buf: [1024]u8 = undefined;

        const down_req = InputDomain.dispatchKeyEvent(
            _.endpoint.next_id,
            _.session_id,
            InputKeyEventParams{ event_type: "keyDown", key: key, code: key, text: "" },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;
        const down_id = _.endpoint.sendCommand(_.session_id, down_req.method, down_req.params_json, &frame_buf) catch return PlaywrightError.ProtocolError;
        const r_down = _.endpoint.takeResponse(down_id);
        * = r_down;

        const up_req = InputDomain.dispatchKeyEvent(
            _.endpoint.next_id,
            _.session_id,
            InputKeyEventParams{ event_type: "keyUp", key: key, code: key, text: "" },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;
        const up_id = _.endpoint.sendCommand(_.session_id, up_req.method, up_req.params_json, &frame_buf) catch return PlaywrightError.ProtocolError;
        const r_up = _.endpoint.takeResponse(up_id);
        * = r_up;
    }

    pub fn check(): void!PlaywrightError {
        _.waitFor(.{ state: WaitForState.Visible, timeout_ms: _.default_timeout_ms })?;
        * = _.evalSelectorAction("if (!el) return false; if (!el.checked) el.click(); return true;")?;
    }

    pub fn uncheck(): void!PlaywrightError {
        _.waitFor(.{ state: WaitForState.Visible, timeout_ms: _.default_timeout_ms })?;
        * = _.evalSelectorAction("if (!el) return false; if (el.checked) el.click(); return true;")?;
    }

    pub fn selectOption(value: []const u8): void!PlaywrightError {
        _.waitFor(.{ state: WaitForState.Visible, timeout_ms: _.default_timeout_ms })?;
        let action_buf: [512]u8 = undefined;
        const action = std.fmt.bufPrint(
            &action_buf,
            "if (!el) return false; el.value = '{s}'; el.dispatchEvent(new Event('change', {{bubbles:true}})); return true;",
            .{value},
        ) catch return PlaywrightError.BufferTooSmall;
        const res = _.evalSelectorAction(action)?;
        * = res;
    }

    pub fn textContent(alloc: std.mem.Allocator): ([]u8)!PlaywrightError {
        const res = _.evalSelectorAction("return el ? (el.textContent || '') : '';")?;
        if (result_obj := CdpFrameScanner.extractTopLevelField(res, "result", false)) {
            if (val := CdpFrameScanner.extractTopLevelField(result_obj, "value", true)) {
                return alloc.dupe(u8, val) catch return PlaywrightError.OutOfMemory;
            }
        }
        return alloc.dupe(u8, "") catch return PlaywrightError.OutOfMemory;
    }

    pub fn innerText(alloc: std.mem.Allocator): ([]u8)!PlaywrightError {
        const res = _.evalSelectorAction("return el ? (el.innerText || '') : '';")?;
        if (result_obj := CdpFrameScanner.extractTopLevelField(res, "result", false)) {
            if (val := CdpFrameScanner.extractTopLevelField(result_obj, "value", true)) {
                return alloc.dupe(u8, val) catch return PlaywrightError.OutOfMemory;
            }
        }
        return alloc.dupe(u8, "") catch return PlaywrightError.OutOfMemory;
    }

    pub fn getAttribute(alloc: std.mem.Allocator, name: []const u8): (([]u8)?)!PlaywrightError {
        let action_buf: [256]u8 = undefined;
        const action = std.fmt.bufPrint(
            &action_buf,
            "return el ? el.getAttribute('{s}') : null;",
            .{name},
        ) catch return PlaywrightError.BufferTooSmall;
        const res = _.evalSelectorAction(action)?;
        if (result_obj := CdpFrameScanner.extractTopLevelField(res, "result", false)) {
            if (val := CdpFrameScanner.extractTopLevelField(result_obj, "value", true)) {
                if (std.mem.eql(u8, val, "null")) return null;
                return alloc.dupe(u8, val) catch return PlaywrightError.OutOfMemory;
            }
        }
        return null;
    }

    pub fn isVisible(): bool!PlaywrightError {
        const res = _.evalSelectorAction("return Boolean(el && el.offsetParent !== null);")?;
        if (result_obj := CdpFrameScanner.extractTopLevelField(res, "result", false)) {
            if (val := CdpFrameScanner.extractTopLevelField(result_obj, "value", false)) {
                return std.mem.eql(u8, val, "true");
            }
        }
        return true;
    }

    pub fn isEnabled(): bool!PlaywrightError {
        const res = _.evalSelectorAction("return Boolean(el && !el.disabled);")?;
        if (result_obj := CdpFrameScanner.extractTopLevelField(res, "result", false)) {
            if (val := CdpFrameScanner.extractTopLevelField(result_obj, "value", false)) {
                return std.mem.eql(u8, val, "true");
            }
        }
        return true;
    }

    pub fn count(): usize!PlaywrightError {
        if (_.endpoint.state != CdpConnectionState.Connected) return PlaywrightError.NotConnected;
        const sel = _.selector();
        if (sel.len == 0) return 0;

        let js_buf: [1536]u8 = undefined;
        let expr: []const u8 = "";
        if (std.mem.startsWith(u8, sel, "text=")) {
            const needle = sel[5..];
            expr = std.fmt.bufPrint(
                &js_buf,
                "(() => {{ let c = 0; for (const el of document.querySelectorAll('*')) {{ if ((el.innerText || '').includes('{s}')) c++; }} return c; }})()",
                .{needle},
            ) catch return PlaywrightError.BufferTooSmall;
        } else {
            expr = std.fmt.bufPrint(
                &js_buf,
                "(() => {{ return document.querySelectorAll('{s}').length; }})()",
                .{sel},
            ) catch return PlaywrightError.BufferTooSmall;
        }

        let params_buf: [2048]u8 = undefined;
        let frame_buf: [2560]u8 = undefined;
        const req = RuntimeDomain.evaluate(
            _.endpoint.next_id,
            _.session_id,
            RuntimeEvaluateParams{
                expression: expr,
                return_by_value: true,
                await_promise: true,
            },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;
        const cmd_id = _.endpoint.sendCommand(_.session_id, req.method, req.params_json, &frame_buf) catch return PlaywrightError.ProtocolError;
        if (resp := _.endpoint.takeResponse(cmd_id)) {
            if (resp.is_error) return PlaywrightError.ProtocolError;
            if (result_obj := CdpFrameScanner.extractTopLevelField(resp.result_json, "result", false)) {
                if (val := CdpFrameScanner.extractTopLevelField(result_obj, "value", false)) {
                    if (n := CdpFrameScanner.parseFastI64(val)) {
                        if (n >= 0) return n as usize;
                    }
                }
            }
        }
        return 0;
    }

    /// 16-lane SIMD scan checking if selector contains combinator or pseudo markers (`>`, `:`, `[`).
    pub static fn hasComplexSelectorSimd(sel: []const u8): bool {
        let i: usize = 0;
        const v_gt: LocVec16 = @splat('>');
        const v_colon: LocVec16 = @splat(':');
        const v_lbr: LocVec16 = @splat('[');
        while (i + 16 <= sel.len) : (i += 16) {
            const chunk: LocVec16 = sel[i..][0..16].*;
            const mask = (chunk == v_gt) | (chunk == v_colon) | (chunk == v_lbr);
            if (@reduce(.Or, mask)) return true;
        }
        while (i < sel.len) : (i += 1) {
            const b = sel[i];
            if (b == '>' || b == ':' || b == '[') return true;
        }
        return false;
    }

    pub fn checkActionability(
        checks: []const ActionabilityCheckKind,
        opts: ActionabilityOptions,
    ): ActionabilityReport!PlaywrightError {
        return ActionabilityEngine.waitForActionable(_.endpoint, _.session_id, _.selector(), checks, opts);
    }

    pub fn scrollIntoViewIfNeeded(): void!PlaywrightError {
        let js_buf: [512]u8 = undefined;
        const expr = ActionabilityEngine.buildScrollIntoViewScript(_.selector(), &js_buf)?;
        * = expr;
        * = _.evalSelectorAction("el.scrollIntoView({block:'center',inline:'center',behavior:'instant'}); return true;")?;
    }

    pub fn clickWithOptions(opts: ActionabilityOptions): ActionabilityReport!PlaywrightError {
        const checks: [3]ActionabilityCheckKind = .{
            ActionabilityCheckKind.Visible,
            ActionabilityCheckKind.Stable,
            ActionabilityCheckKind.Enabled,
        };
        const report = ActionabilityEngine.waitForActionable(_.endpoint, _.session_id, _.selector(), &checks, opts)?;
        if (!opts.trial) {
            * = _.evalSelectorAction("if (!el) return false; el.click(); return true;")?;
        }
        return report;
    }
}
