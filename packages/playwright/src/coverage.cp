// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/coverageRecorder.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/coverage.cp — JS & CSS Precise Coverage Recorder
// Reuses cap:cdp ProfilerDomain & CSSDomain for zero-copy coverage tracking.
// ============================================================================

const std = @import("std");
import {
    CdpEndpoint,
    CdpConnectionState,
    CdpFrameScanner,
    ProfilerDomain,
    CSSDomain,
} from "cap:cdp";
import { PlaywrightError } from "./network";

pub struct JSCoverageOptions {
    pub reset_on_navigation: bool = true,
    pub report_anonymous_scripts: bool = false,
    pub call_count: bool = false,
    pub detailed: bool = true,
}

pub struct CSSCoverageOptions {
    pub reset_on_navigation: bool = true,
}

pub struct CoverageRange {
    pub start_offset: u32 = 0,
    pub end_offset: u32 = 0,
    pub count: u32 = 0,
}

pub struct JSCoverageEntry {
    pub url: []const u8 = "",
    pub script_id: []const u8 = "",
    pub source: []const u8 = "",
    pub ranges: std.ArrayList(CoverageRange) = .empty,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.ranges.deinit(alloc);
        _.ranges = .empty;
    }
}

pub struct CSSCoverageEntry {
    pub url: []const u8 = "",
    pub text: []const u8 = "",
    pub ranges: std.ArrayList(CoverageRange) = .empty,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.ranges.deinit(alloc);
        _.ranges = .empty;
    }
}

pub struct Coverage {
    allocator: std.mem.Allocator = undefined,
    endpoint: *CdpEndpoint = undefined,
    session_id: []const u8 = "",
    pub js_active: bool = false,
    pub css_active: bool = false,
    pub js_options: JSCoverageOptions = .{},
    pub css_options: CSSCoverageOptions = .{},

    pub static fn init(
        alloc: std.mem.Allocator,
        endpoint: *CdpEndpoint,
        session_id: []const u8,
    ): Coverage {
        return Coverage{
            allocator: alloc,
            endpoint: endpoint,
            session_id: session_id,
            js_active: false,
            css_active: false,
            js_options: .{},
            css_options: .{},
        };
    }

    pub fn deinit(*_) {
        _.js_active = false;
        _.css_active = false;
    }

    ~init() {
        _.deinit();
    }

    fn sendCdp(*_, method: []const u8, params_json: []const u8): ([]const u8)!PlaywrightError {
        if (_.endpoint.state != CdpConnectionState.Connected) return PlaywrightError.NotConnected;
        let frame_buf: [4096]u8 = undefined;
        const cmd_id = _.endpoint.sendCommand(_.session_id, method, params_json, &frame_buf) catch |e| {
            return switch (e) {
                error.NotConnected => PlaywrightError.NotConnected,
                error.ConnectionClosed => PlaywrightError.ConnectionClosed,
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

    pub fn isJSCoverageActive(): bool {
        return _.js_active;
    }

    pub fn isCSSCoverageActive(): bool {
        return _.css_active;
    }

    pub fn startJSCoverage(*_, opts: JSCoverageOptions): void!PlaywrightError {
        if (_.js_active) return PlaywrightError.ProtocolError;
        _.js_options = opts;

        const en_req = ProfilerDomain.enable(_.endpoint.next_id, _.session_id);
        const en_res = _.sendCdp(en_req.method, en_req.params_json)?;
        * = en_res;

        let params_buf: [256]u8 = undefined;
        const st_req = ProfilerDomain.startPreciseCoverage(
            _.endpoint.next_id,
            _.session_id,
            opts.call_count,
            opts.detailed,
            &params_buf,
        ) catch return PlaywrightError.BufferTooSmall;
        const st_res = _.sendCdp(st_req.method, st_req.params_json)?;
        * = st_res;
        _.js_active = true;
    }

    pub fn stopJSCoverage(*_, alloc: std.mem.Allocator): ([]JSCoverageEntry)!PlaywrightError {
        if (!_.js_active) return PlaywrightError.ProtocolError;
        _.js_active = false;

        const take_req = ProfilerDomain.takePreciseCoverage(_.endpoint.next_id, _.session_id);
        const res = _.sendCdp(take_req.method, take_req.params_json)?;

        const stop_req = ProfilerDomain.stopPreciseCoverage(_.endpoint.next_id, _.session_id);
        const stop_res = _.sendCdp(stop_req.method, stop_req.params_json)?;
        * = stop_res;

        let entries: std.ArrayList(JSCoverageEntry) = .empty;
        errdefer {
            for (e of entries.items) {
                let mut_e = e;
                mut_e.deinit(alloc);
            }
            entries.deinit(alloc);
        }

        if (arr := CdpFrameScanner.extractTopLevelField(res, "result", false)) {
            if (arr.len > 2) {
                let entry = JSCoverageEntry{
                    url: CdpFrameScanner.extractTopLevelField(arr, "url", true) orelse "",
                    script_id: CdpFrameScanner.extractTopLevelField(arr, "scriptId", true) orelse "",
                    source: "",
                    ranges: .empty,
                };
                entries.append(alloc, entry) catch return PlaywrightError.OutOfMemory;
            }
        }
        return entries.toOwnedSlice(alloc) catch return PlaywrightError.OutOfMemory;
    }

    pub fn startCSSCoverage(*_, opts: CSSCoverageOptions): void!PlaywrightError {
        if (_.css_active) return PlaywrightError.ProtocolError;
        _.css_options = opts;

        const en_req = CSSDomain.enable(_.endpoint.next_id, _.session_id);
        const en_res = _.sendCdp(en_req.method, en_req.params_json)?;
        * = en_res;

        const st_req = CSSDomain.startRuleUsageTracking(_.endpoint.next_id, _.session_id);
        const st_res = _.sendCdp(st_req.method, st_req.params_json)?;
        * = st_res;
        _.css_active = true;
    }

    pub fn stopCSSCoverage(*_, alloc: std.mem.Allocator): ([]CSSCoverageEntry)!PlaywrightError {
        if (!_.css_active) return PlaywrightError.ProtocolError;
        _.css_active = false;

        const stop_req = CSSDomain.stopRuleUsageTracking(_.endpoint.next_id, _.session_id);
        const res = _.sendCdp(stop_req.method, stop_req.params_json)?;

        let entries: std.ArrayList(CSSCoverageEntry) = .empty;
        errdefer {
            for (e of entries.items) {
                let mut_e = e;
                mut_e.deinit(alloc);
            }
            entries.deinit(alloc);
        }

        if (arr := CdpFrameScanner.extractTopLevelField(res, "ruleUsage", false)) {
            if (arr.len > 2) {
                let entry = CSSCoverageEntry{
                    url: CdpFrameScanner.extractTopLevelField(arr, "styleSheetId", true) orelse "",
                    text: "",
                    ranges: .empty,
                };
                entries.append(alloc, entry) catch return PlaywrightError.OutOfMemory;
            }
        }
        return entries.toOwnedSlice(alloc) catch return PlaywrightError.OutOfMemory;
    }
}
