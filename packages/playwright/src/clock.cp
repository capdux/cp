// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/clock.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/clock.cp — Playwright Fake Clock Controller
// Implements install, fastForward, runFor, pauseAt, resume, setFixedTime,
// and setSystemTime over cap:cdp (Page.addScriptToEvaluateOnNewDocument +
// Runtime.evaluate) with zero-copy stack buffers.
// ============================================================================

const std = @import("std");
import {
    CdpEndpoint,
    CdpConnectionState,
    RuntimeDomain,
    RuntimeEvaluateParams,
} from "cap:cdp";
import { PlaywrightError } from "./network";

pub enum ClockMode {
    Real,
    Installed,
    Paused,
    Fixed,
}

pub struct Clock {
    endpoint: *CdpEndpoint = undefined,
    session_id: []const u8 = "",
    pub installed: bool = false,
    pub mode: ClockMode = ClockMode.Real,
    pub current_time_ms: i64 = 0,
    pub paused_at_ms: i64 = 0,
    pub fixed_time_ms: i64 = 0,
    pub init_scripts_count: usize = 0,

    pub static fn init(endpoint: *CdpEndpoint, session_id: []const u8): Clock {
        return Clock{
            endpoint: endpoint,
            session_id: session_id,
            installed: false,
            mode: ClockMode.Real,
            current_time_ms: 0,
            paused_at_ms: 0,
            fixed_time_ms: 0,
            init_scripts_count: 0,
        };
    }

    pub fn now(): i64 {
        if (_.mode == ClockMode.Fixed) return _.fixed_time_ms;
        if (_.mode == ClockMode.Paused) return _.paused_at_ms;
        return _.current_time_ms;
    }

    /// Parse human-readable tick durations: "ss", "mm:ss", or "hh:mm:ss" into milliseconds.
    pub static fn parseTicks(value: []const u8): i64!PlaywrightError {
        if (value.len == 0) return 0;
        let parts: [3]i64 = .{ 0, 0, 0 };
        let count: usize = 0;
        let cur: i64 = 0;
        let has_digit = false;

        for (c of value) {
            if (c == ':') {
                if (!has_digit || count >= 2 || cur >= 60) return PlaywrightError.InvalidUrl;
                parts[count] = cur;
                count += 1;
                cur = 0;
                has_digit = false;
            } else if (c >= '0' && c <= '9') {
                cur = cur * 10 + ((c - '0') as i64);
                has_digit = true;
            } else {
                return PlaywrightError.InvalidUrl;
            }
        }
        if (!has_digit) return PlaywrightError.InvalidUrl;
        if (count > 0 && cur >= 60) return PlaywrightError.InvalidUrl;
        parts[count] = cur;
        count += 1;

        let seconds: i64 = 0;
        if (count == 1) {
            seconds = parts[0];
        } else if (count == 2) {
            seconds = parts[0] * 60 + parts[1];
        } else {
            seconds = parts[0] * 3600 + parts[1] * 60 + parts[2];
        }
        return seconds * 1000;
    }

    fn dispatchRuntimeEval(*_, expr: []const u8): void!PlaywrightError {
        if (_.endpoint.state != CdpConnectionState.Connected) return PlaywrightError.NotConnected;
        let params_buf: [1024]u8 = undefined;
        let frame_buf: [1536]u8 = undefined;
        const req = RuntimeDomain.evaluate(
            _.endpoint.next_id,
            _.session_id,
            RuntimeEvaluateParams{
                expression: expr,
                return_by_value: true,
                await_promise: false,
            },
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;
        const cmd_id = _.endpoint.sendCommand(
            _.session_id,
            req.method,
            req.params_json,
            &frame_buf,
        ) catch return PlaywrightError.ProtocolError;
        const resp = _.endpoint.takeResponse(cmd_id);
        * = resp;
    }

    fn installIfNeeded(*_): void!PlaywrightError {
        if (_.installed) return;
        if (_.endpoint.state != CdpConnectionState.Connected) return PlaywrightError.NotConnected;

        // Register init script on new document via CDP Page.addScriptToEvaluateOnNewDocument
        let params_buf: [512]u8 = undefined;
        let frame_buf: [768]u8 = undefined;
        const params = std.fmt.bufPrint(
            &params_buf,
            "{{\"source\":\"globalThis.__pwClockInstalled=true;\"}}",
            .{},
        ) catch return PlaywrightError.BufferTooSmall;
        const cmd_id = _.endpoint.sendCommand(
            _.session_id,
            "Page.addScriptToEvaluateOnNewDocument",
            params,
            &frame_buf,
        ) catch return PlaywrightError.ProtocolError;
        const resp = _.endpoint.takeResponse(cmd_id);
        * = resp;

        _.installed = true;
        _.init_scripts_count += 1;
        if (_.mode == ClockMode.Real) {
            _.mode = ClockMode.Installed;
        }
    }

    pub fn install(*_, time_ms: i64?): void!PlaywrightError {
        _.installIfNeeded()?;
        const t = time_ms orelse 0;
        _.current_time_ms = t;
        _.mode = ClockMode.Installed;

        let js_buf: [256]u8 = undefined;
        const expr = std.fmt.bufPrint(
            &js_buf,
            "globalThis.__pwClock && globalThis.__pwClock.controller.install({d})",
            .{t},
        ) catch return PlaywrightError.BufferTooSmall;
        _.dispatchRuntimeEval(expr)?;
        _.init_scripts_count += 1;
    }

    pub fn fastForward(*_, ticks_ms: i64): void!PlaywrightError {
        _.installIfNeeded()?;
        _.current_time_ms += ticks_ms;
        if (_.mode == ClockMode.Paused) {
            _.paused_at_ms += ticks_ms;
        }
        let js_buf: [256]u8 = undefined;
        const expr = std.fmt.bufPrint(
            &js_buf,
            "globalThis.__pwClock && globalThis.__pwClock.controller.fastForward({d})",
            .{ticks_ms},
        ) catch return PlaywrightError.BufferTooSmall;
        _.dispatchRuntimeEval(expr)?;
    }

    pub fn fastForwardStr(*_, ticks_str: []const u8): void!PlaywrightError {
        const ms = Clock.parseTicks(ticks_str)?;
        _.fastForward(ms)?;
    }

    pub fn runFor(*_, ticks_ms: i64): void!PlaywrightError {
        _.installIfNeeded()?;
        _.current_time_ms += ticks_ms;
        let js_buf: [256]u8 = undefined;
        const expr = std.fmt.bufPrint(
            &js_buf,
            "globalThis.__pwClock && globalThis.__pwClock.controller.runFor({d})",
            .{ticks_ms},
        ) catch return PlaywrightError.BufferTooSmall;
        _.dispatchRuntimeEval(expr)?;
    }

    pub fn pauseAt(*_, time_ms: i64): void!PlaywrightError {
        _.installIfNeeded()?;
        _.paused_at_ms = time_ms;
        _.current_time_ms = time_ms;
        _.mode = ClockMode.Paused;
        let js_buf: [256]u8 = undefined;
        const expr = std.fmt.bufPrint(
            &js_buf,
            "globalThis.__pwClock && globalThis.__pwClock.controller.pauseAt({d})",
            .{time_ms},
        ) catch return PlaywrightError.BufferTooSmall;
        _.dispatchRuntimeEval(expr)?;
    }

    pub fn resume_(*_): void!PlaywrightError {
        _.installIfNeeded()?;
        _.mode = ClockMode.Installed;
        _.dispatchRuntimeEval("globalThis.__pwClock && globalThis.__pwClock.controller.resume()")?;
    }

    pub fn setFixedTime(*_, time_ms: i64): void!PlaywrightError {
        _.installIfNeeded()?;
        _.fixed_time_ms = time_ms;
        _.current_time_ms = time_ms;
        _.mode = ClockMode.Fixed;
        let js_buf: [256]u8 = undefined;
        const expr = std.fmt.bufPrint(
            &js_buf,
            "globalThis.__pwClock && globalThis.__pwClock.controller.setFixedTime({d})",
            .{time_ms},
        ) catch return PlaywrightError.BufferTooSmall;
        _.dispatchRuntimeEval(expr)?;
    }

    pub fn setSystemTime(*_, time_ms: i64): void!PlaywrightError {
        _.installIfNeeded()?;
        _.current_time_ms = time_ms;
        let js_buf: [256]u8 = undefined;
        const expr = std.fmt.bufPrint(
            &js_buf,
            "globalThis.__pwClock && globalThis.__pwClock.controller.setSystemTime({d})",
            .{time_ms},
        ) catch return PlaywrightError.BufferTooSmall;
        _.dispatchRuntimeEval(expr)?;
    }

    pub fn uninstall(*_) {
        _.installed = false;
        _.mode = ClockMode.Real;
        _.init_scripts_count = 0;
    }
}
