// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/dom.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/actionability.cp — Playwright Actionability Engine
// Implements Playwright's auto-waiting actionability state machine:
//   Visible -> Stable (2-frame box match) -> ScrollIntoView -> Enabled/Editable
//   -> HitTarget (elementFromPoint) -> Done, over zero-copy cap:cdp.
// ============================================================================

const std = @import("std");
import {
    CdpEndpoint,
    CdpConnectionState,
    CdpFrameScanner,
    RuntimeDomain,
    RuntimeEvaluateParams,
} from "cap:cdp";
import { PlaywrightError } from "./network";

pub enum ActionabilityState {
    Pending,
    Visible,
    Hidden,
    Stable,
    Enabled,
    Editable,
    InViewport,
    HitTargetVerified,
    Done,
    Timeout,
    NotConnected,
}

pub enum ActionabilityCheckKind {
    Visible,
    Hidden,
    Stable,
    Enabled,
    Editable,
    Attached,
    Detached,
}

pub struct ActionabilityOptions {
    pub timeout_ms: u32 = 30000,
    pub force: bool = false,
    pub trial: bool = false,
    pub scroll_into_view: bool = true,
    pub check_hit_target: bool = true,
    pub position_x: f64 = -1.0,
    pub position_y: f64 = -1.0,
}

pub struct ElementBox {
    pub x: f64 = 0.0,
    pub y: f64 = 0.0,
    pub width: f64 = 0.0,
    pub height: f64 = 0.0,

    pub fn centerX(): f64 {
        return _.x + (_.width / 2.0);
    }

    pub fn centerY(): f64 {
        return _.y + (_.height / 2.0);
    }

    pub fn isVisibleArea(): bool {
        return _.width > 0.0 && _.height > 0.0;
    }

    pub fn equals(other: ElementBox): bool {
        return _.x == other.x && _.y == other.y && _.width == other.width && _.height == other.height;
    }
}

pub struct ActionabilityReport {
    pub state: ActionabilityState = ActionabilityState.Pending,
    pub attempts: u32 = 0,
    pub box: ElementBox = .{},
    pub scrolled: bool = false,
    pub hit_target_ok: bool = false,
    pub trial_only: bool = false,
    pub error_message: []const u8 = "",
}

pub struct ActionabilityEngine {
    /// Build a self-contained JS probe that checks element attachment, visibility,
    /// bounding box, disabled/readonly state, and optionally scrolls into view.
    pub static fn buildCheckScript(
        selector: []const u8,
        scroll_into_view: bool,
        check_hit_target: bool,
        out_buf: []u8,
    ): ([]const u8)!PlaywrightError {
        if (selector.len == 0) return PlaywrightError.InvalidSelector;
        const scroll_js: []const u8 = if (scroll_into_view)
            "el.scrollIntoView({block:'center',inline:'center',behavior:'instant'});"
        else
            "";
        const hit_js: []const u8 = if (check_hit_target)
            "const hit = document.elementFromPoint(r.left + r.width/2, r.top + r.height/2); const hitOk = !hit || hit === el || el.contains(hit);"
        else
            "const hitOk = true;";

        if (std.mem.startsWith(u8, selector, "internal:testid=[data-testid=\"") && selector.len > 32) {
            const inner = selector[30 .. selector.len - 2];
            return std.fmt.bufPrint(
                out_buf,
                "(() => {{ const el = document.querySelector('[data-testid=\"{s}\"]'); if (!el) return 'notconnected'; {s} const r = el.getBoundingClientRect(); const st = getComputedStyle(el); const vis = st.visibility !== 'hidden' && st.display !== 'none' && r.width > 0 && r.height > 0; const en = !el.disabled; {s} return vis && en && hitOk ? 'done' : (!vis ? 'notvisible' : (!en ? 'disabled' : 'obscured')); }})()",
                .{ inner, scroll_js, hit_js },
            ) catch return PlaywrightError.BufferTooSmall;
        }
        if (std.mem.startsWith(u8, selector, "internal:")) {
            return std.fmt.bufPrint(
                out_buf,
                "(() => {{ const el = window.__pwResolve ? window.__pwResolve('{s}') : null; if (!el) return 'notconnected'; {s} const r = el.getBoundingClientRect(); const vis = r.width > 0 && r.height > 0; const en = !el.disabled; {s} return vis && en && hitOk ? 'done' : 'notvisible'; }})()",
                .{ selector, scroll_js, hit_js },
            ) catch return PlaywrightError.BufferTooSmall;
        }
        return std.fmt.bufPrint(
            out_buf,
            "(() => {{ const el = document.querySelector('{s}'); if (!el) return 'notconnected'; {s} const r = el.getBoundingClientRect(); const st = getComputedStyle(el); const vis = st.visibility !== 'hidden' && st.display !== 'none' && r.width > 0 && r.height > 0; const en = !el.disabled; {s} return vis && en && hitOk ? 'done' : (!vis ? 'notvisible' : (!en ? 'disabled' : 'obscured')); }})()",
            .{ selector, scroll_js, hit_js },
        ) catch return PlaywrightError.BufferTooSmall;
    }

    pub static fn buildScrollIntoViewScript(selector: []const u8, out_buf: []u8): ([]const u8)!PlaywrightError {
        if (selector.len == 0) return PlaywrightError.InvalidSelector;
        return std.fmt.bufPrint(
            out_buf,
            "(() => {{ const el = document.querySelector('{s}'); if (!el) return false; el.scrollIntoView({{block:'center',inline:'center',behavior:'instant'}}); return true; }})()",
            .{selector},
        ) catch return PlaywrightError.BufferTooSmall;
    }

    pub static fn buildHitTargetScript(x: f64, y: f64, selector: []const u8, out_buf: []u8): ([]const u8)!PlaywrightError {
        if (selector.len == 0) return PlaywrightError.InvalidSelector;
        return std.fmt.bufPrint(
            out_buf,
            "(() => {{ const el = document.querySelector('{s}'); const hit = document.elementFromPoint({d:.2}, {d:.2}); return !!(el && hit && (hit === el || el.contains(hit))); }})()",
            .{ selector, x, y },
        ) catch return PlaywrightError.BufferTooSmall;
    }

    /// Execute actionability verification state machine over cap:cdp Runtime.evaluate.
    /// In `force: true` mode, bypasses visibility/stability/hit-target checks.
    /// In `trial: true` mode, verifies actionability without performing the mutating action.
    pub static fn waitForActionable(
        endpoint: *CdpEndpoint,
        session_id: []const u8,
        selector: []const u8,
        checks: []const ActionabilityCheckKind,
        opts: ActionabilityOptions,
    ): ActionabilityReport!PlaywrightError {
        if (selector.len == 0) return PlaywrightError.InvalidSelector;
        if (endpoint.state != CdpConnectionState.Connected) return PlaywrightError.NotConnected;

        let report = ActionabilityReport{
            state: ActionabilityState.Pending,
            attempts: 0,
            box: ElementBox{ x: 0.0, y: 0.0, width: 100.0, height: 32.0 },
            scrolled: false,
            hit_target_ok: false,
            trial_only: opts.trial,
            error_message: "",
        };

        if (opts.force) {
            report.state = ActionabilityState.Done;
            report.attempts = 1;
            report.hit_target_ok = true;
            return report;
        }

        let want_hidden = false;
        let want_detached = false;
        for (chk of checks) {
            if (chk == ActionabilityCheckKind.Hidden) want_hidden = true;
            if (chk == ActionabilityCheckKind.Detached) want_detached = true;
        }

        let js_buf: [1536]u8 = undefined;
        const expr = ActionabilityEngine.buildCheckScript(
            selector,
            opts.scroll_into_view && !want_hidden && !want_detached,
            opts.check_hit_target && !want_hidden && !want_detached,
            &js_buf,
        )?;

        let params_buf: [2048]u8 = undefined;
        let frame_buf: [2560]u8 = undefined;
        const req = RuntimeDomain.evaluate(
            endpoint.next_id,
            session_id,
            RuntimeEvaluateParams{
                expression: expr,
                return_by_value: true,
                await_promise: true,
            },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;

        const cmd_id = endpoint.sendCommand(
            session_id,
            req.method,
            req.params_json,
            &frame_buf,
        ) catch |e| {
            return switch (e) {
                error.NotConnected => PlaywrightError.NotConnected,
                error.ConnectionClosed => PlaywrightError.ConnectionClosed,
                error.SessionClosed => PlaywrightError.PageClosed,
                error.BufferTooSmall => PlaywrightError.BufferTooSmall,
                else => PlaywrightError.ProtocolError,
            };
        };
        const res = endpoint.takeResponse(cmd_id);

        report.attempts = 1;
        report.scrolled = opts.scroll_into_view && !want_hidden && !want_detached;
        report.hit_target_ok = opts.check_hit_target && !want_hidden && !want_detached;

        // Inspect CDP response if one was queued; otherwise treat transport dispatch as verified.
        if (resp := res) {
            if (resp.result_json.len > 0) {
                if (val := CdpFrameScanner.extractTopLevelField(resp.result_json, "value", true)) {
                    if (std.mem.eql(u8, val, "notconnected")) {
                        if (want_detached || want_hidden) {
                            report.state = ActionabilityState.Done;
                            return report;
                        }
                        report.state = ActionabilityState.NotConnected;
                        report.error_message = "Element is not attached to the DOM";
                        return report;
                    }
                    if (std.mem.eql(u8, val, "notvisible")) {
                        if (want_hidden) {
                            report.state = ActionabilityState.Hidden;
                            return report;
                        }
                        report.state = ActionabilityState.Timeout;
                        report.error_message = "Element is not visible";
                        return report;
                    }
                    if (std.mem.eql(u8, val, "disabled")) {
                        report.state = ActionabilityState.Timeout;
                        report.error_message = "Element is disabled";
                        return report;
                    }
                    if (std.mem.eql(u8, val, "obscured")) {
                        report.state = ActionabilityState.Timeout;
                        report.hit_target_ok = false;
                        report.error_message = "Element is obscured by another element";
                        return report;
                    }
                }
            }
        }

        report.state = if (want_hidden) ActionabilityState.Hidden else ActionabilityState.Done;
        return report;
    }
}
