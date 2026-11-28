// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/frames.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/frame_manager.cp — FrameManager & DomActionabilityLoop
// Absorbed from server/frames.ts & server/dom.ts:
//   - FrameLifecycleState (Commit / DomContentLoaded / Load / NetworkIdle)
//   - FrameManager: Page.frameAttached / frameNavigated / frameDetached /
//     lifecycleEvent, active inflight request tracking for exact networkidle,
//     dynamic Frame tree & cross-frame navigation.
//   - DomActionabilityLoop: full retry loop with re-location, visible/enabled/
//     editable checks, 2-frame ElementBox stability check, auto scrollIntoView,
//     elementFromPoint hit-target occlusion check, and stepped timeout backoff.
// Uses ArenaAllocator, = undefined stack buffers, and 16-lane SIMD scanning.
// ============================================================================

const std = @import("std");
import {
    CdpEndpoint,
    CdpConnectionState,
    CdpFrameScanner,
    RuntimeDomain,
    RuntimeEvaluateParams,
} from "cap:cdp";
import { PlaywrightError, UrlMatcher } from "./network";
import {
    ActionabilityState,
    ActionabilityCheckKind,
    ActionabilityOptions,
    ElementBox,
    ActionabilityReport,
    ActionabilityEngine,
} from "./actionability";

type FrameVec16 = @Vector(16, u8);

/// 16-lane SIMD scanner: returns index of first occurrence of `needle` byte, or null.
pub fn simdFindByteFrame(data: []const u8, needle: u8): usize? {
    let i: usize = 0;
    const v_needle: FrameVec16 = @splat(needle);
    while (i + 16 <= data.len) : (i += 16) {
        const chunk: FrameVec16 = data[i..][0..16].*;
        const mask = chunk == v_needle;
        if (@reduce(.Or, mask)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                if (data[i + k] == needle) return i + k;
            }
        }
    }
    while (i < data.len) : (i += 1) {
        if (data[i] == needle) return i;
    }
    return null;
}

pub enum FrameLifecycleState {
    Commit,
    DomContentLoaded,
    Load,
    NetworkIdle,

    pub fn bit(): u8 {
        return switch (_) {
            .Commit => 0x01,
            .DomContentLoaded => 0x02,
            .Load => 0x04,
            .NetworkIdle => 0x08,
        };
    }

    pub fn asName(): []const u8 {
        return switch (_) {
            .Commit => "commit",
            .DomContentLoaded => "domcontentloaded",
            .Load => "load",
            .NetworkIdle => "networkidle",
        };
    }

    pub static fn fromName(state_name: []const u8): FrameLifecycleState? {
        if (std.ascii.eqlIgnoreCase(state_name, "commit") || std.ascii.eqlIgnoreCase(state_name, "init")) {
            return FrameLifecycleState.Commit;
        }
        if (std.ascii.eqlIgnoreCase(state_name, "domcontentloaded")) {
            return FrameLifecycleState.DomContentLoaded;
        }
        if (std.ascii.eqlIgnoreCase(state_name, "load")) {
            return FrameLifecycleState.Load;
        }
        if (std.ascii.eqlIgnoreCase(state_name, "networkidle") || std.ascii.eqlIgnoreCase(state_name, "networkalmostidle")) {
            return FrameLifecycleState.NetworkIdle;
        }
        return null;
    }
}

pub struct ManagedFrame {
    pub frame_id: []const u8 = "",
    pub parent_id: []const u8 = "",
    pub name: []const u8 = "",
    pub url: []const u8 = "about:blank",
    pub loader_id: []const u8 = "",
    pub detached: bool = false,
    pub lifecycle_mask: u8 = 0,
    pub inflight_requests: usize = 0,
    pub last_activity_ms: i64 = 0,
    pub navigation_count: u32 = 0,

    pub fn hasReached(state: FrameLifecycleState): bool {
        return (_.lifecycle_mask & state.bit()) != 0;
    }

    pub fn isMainFrame(): bool {
        return _.parent_id.len == 0;
    }
}

pub struct FrameManager {
    arena: std.heap.ArenaAllocator = undefined,
    allocator: std.mem.Allocator = undefined,
    deinitialized: bool = false,

    pub main_frame_id: []const u8 = "",
    pub frames: std.ArrayList(ManagedFrame) = .empty,
    pub global_inflight_requests: usize = 0,
    pub networkidle_threshold_ms: i64 = 500,
    pub last_network_activity_ms: i64 = 0,
    pub events_processed: usize = 0,

    pub static fn init(alloc: std.mem.Allocator): FrameManager {
        return FrameManager{
            arena: std.heap.ArenaAllocator.init(alloc),
            allocator: alloc,
            deinitialized: false,
            main_frame_id: "",
            frames: .empty,
            global_inflight_requests: 0,
            networkidle_threshold_ms: 500,
            last_network_activity_ms: 0,
            events_processed: 0,
        };
    }

    pub fn deinit(*_) {
        if (_.deinitialized) return;
        _.deinitialized = true;
        _.frames.deinit(_.allocator);
        _.frames = .empty;
        _.arena.deinit();
    }

    ~init() {
        _.deinit();
    }

    fn intern(*_, s: []const u8): ([]const u8)!PlaywrightError {
        if (s.len == 0) return "";
        const aa = _.arena.allocator();
        return aa.dupe(u8, s) catch return PlaywrightError.OutOfMemory;
    }

    fn findIndex(frame_id: []const u8): usize? {
        let i: usize = 0;
        while (i < _.frames.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.frames.items[i].frame_id, frame_id)) {
                return i;
            }
        }
        return null;
    }

    /// Handle CDP `Page.frameAttached` event.
    pub fn onFrameAttached(
        *_,
        frame_id: []const u8,
        parent_frame_id: []const u8,
    ): (*ManagedFrame)!PlaywrightError {
        if (frame_id.len == 0) return PlaywrightError.ProtocolError;
        _.events_processed += 1;

        if (idx := _.findIndex(frame_id)) {
            _.frames.items[idx].detached = false;
            if (parent_frame_id.len > 0) {
                _.frames.items[idx].parent_id = _.intern(parent_frame_id)?;
            }
            return &_.frames.items[idx];
        }

        const fid = _.intern(frame_id)?;
        const pid = _.intern(parent_frame_id)?;
        if (_.main_frame_id.len == 0 && pid.len == 0) {
            _.main_frame_id = fid;
        }

        _.frames.append(_.allocator, ManagedFrame{
            frame_id: fid,
            parent_id: pid,
            name: "",
            url: "about:blank",
            loader_id: "",
            detached: false,
            lifecycle_mask: FrameLifecycleState.Commit.bit(),
            inflight_requests: 0,
            last_activity_ms: 0,
            navigation_count: 0,
        }) catch return PlaywrightError.OutOfMemory;
        return &_.frames.items[_.frames.items.len - 1];
    }

    /// Handle CDP `Page.frameNavigated` event (supports main & cross-frame navigation).
    pub fn onFrameNavigated(
        *_,
        frame_id: []const u8,
        parent_frame_id: []const u8,
        frame_name: []const u8,
        url: []const u8,
        loader_id: []const u8,
        now_ms: i64,
    ): (*ManagedFrame)!PlaywrightError {
        if (frame_id.len == 0) return PlaywrightError.ProtocolError;
        _.events_processed += 1;

        const target_url = if (url.len > 0) url else "about:blank";
        if (idx := _.findIndex(frame_id)) {
            let f = &_.frames.items[idx];
            if (parent_frame_id.len > 0) {
                f.parent_id = _.intern(parent_frame_id)?;
            }
            f.name = _.intern(frame_name)?;
            f.url = _.intern(target_url)?;
            f.loader_id = _.intern(loader_id)?;
            f.detached = false;
            f.lifecycle_mask = FrameLifecycleState.Commit.bit();
            f.last_activity_ms = now_ms;
            f.navigation_count += 1;
            return f;
        }

        const fid = _.intern(frame_id)?;
        const pid = _.intern(parent_frame_id)?;
        const fname = _.intern(frame_name)?;
        const furl = _.intern(target_url)?;
        const lid = _.intern(loader_id)?;

        if (_.main_frame_id.len == 0 && pid.len == 0) {
            _.main_frame_id = fid;
        }

        _.frames.append(_.allocator, ManagedFrame{
            frame_id: fid,
            parent_id: pid,
            name: fname,
            url: furl,
            loader_id: lid,
            detached: false,
            lifecycle_mask: FrameLifecycleState.Commit.bit(),
            inflight_requests: 0,
            last_activity_ms: now_ms,
            navigation_count: 1,
        }) catch return PlaywrightError.OutOfMemory;
        return &_.frames.items[_.frames.items.len - 1];
    }

    /// Handle CDP `Page.frameDetached` event; recursively detaches descendant frames.
    pub fn onFrameDetached(*_, frame_id: []const u8): usize {
        _.events_processed += 1;
        let detached_count: usize = 0;

        if (idx := _.findIndex(frame_id)) {
            if (!_.frames.items[idx].detached) {
                _.frames.items[idx].detached = true;
                detached_count += 1;
            }
        }

        // Propagate detachment down the dynamic frame tree (fixed-point pass).
        let changed = true;
        while (changed) {
            changed = false;
            let i: usize = 0;
            while (i < _.frames.items.len) : (i += 1) {
                if (_.frames.items[i].detached || _.frames.items[i].parent_id.len == 0) continue;
                if (pidx := _.findIndex(_.frames.items[i].parent_id)) {
                    if (_.frames.items[pidx].detached) {
                        _.frames.items[i].detached = true;
                        detached_count += 1;
                        changed = true;
                    }
                }
            }
        }
        return detached_count;
    }

    /// Handle CDP `Page.lifecycleEvent` (`commit`, `DOMContentLoaded`, `load`, `networkIdle`).
    pub fn onLifecycleEvent(
        *_,
        frame_id: []const u8,
        loader_id: []const u8,
        event_name: []const u8,
        now_ms: i64,
    ): bool {
        _.events_processed += 1;
        const idx = _.findIndex(frame_id) orelse return false;
        let f = &_.frames.items[idx];
        if (f.detached) return false;
        if (loader_id.len > 0 && f.loader_id.len > 0 && !std.mem.eql(u8, f.loader_id, loader_id)) {
            return false;
        }
        const st = FrameLifecycleState.fromName(event_name) orelse return false;
        f.lifecycle_mask |= st.bit();
        if (st == FrameLifecycleState.DomContentLoaded) {
            f.lifecycle_mask |= FrameLifecycleState.Commit.bit();
        } else if (st == FrameLifecycleState.Load) {
            f.lifecycle_mask |= FrameLifecycleState.Commit.bit() | FrameLifecycleState.DomContentLoaded.bit();
        } else if (st == FrameLifecycleState.NetworkIdle) {
            f.lifecycle_mask |= FrameLifecycleState.Commit.bit() | FrameLifecycleState.DomContentLoaded.bit() | FrameLifecycleState.Load.bit();
        }
        f.last_activity_ms = now_ms;
        return true;
    }

    /// Track a newly started network request for `frame_id`; clears `NetworkIdle` bit.
    pub fn onRequestStarted(*_, frame_id: []const u8, now_ms: i64) {
        _.global_inflight_requests += 1;
        _.last_network_activity_ms = now_ms;
        if (idx := _.findIndex(frame_id)) {
            _.frames.items[idx].inflight_requests += 1;
            _.frames.items[idx].last_activity_ms = now_ms;
            _.frames.items[idx].lifecycle_mask &= ~FrameLifecycleState.NetworkIdle.bit();
        }
    }

    /// Track a completed/failed network request for `frame_id`.
    pub fn onRequestFinished(*_, frame_id: []const u8, now_ms: i64) {
        if (_.global_inflight_requests > 0) {
            _.global_inflight_requests -= 1;
        }
        _.last_network_activity_ms = now_ms;
        if (idx := _.findIndex(frame_id)) {
            if (_.frames.items[idx].inflight_requests > 0) {
                _.frames.items[idx].inflight_requests -= 1;
            }
            _.frames.items[idx].last_activity_ms = now_ms;
        }
    }

    /// Evaluate exact `networkidle` condition: 0 inflight requests for at least
    /// `networkidle_threshold_ms` (default 500ms) after `Load` (or last activity).
    pub fn checkNetworkIdle(*_, frame_id: []const u8, now_ms: i64): bool {
        const idx = _.findIndex(frame_id) orelse return false;
        let f = &_.frames.items[idx];
        if (f.detached) return false;
        if (f.inflight_requests > 0 || _.global_inflight_requests > 0) return false;
        if (!f.hasReached(FrameLifecycleState.Load)) return false;

        const latest = if (f.last_activity_ms > _.last_network_activity_ms) f.last_activity_ms else _.last_network_activity_ms;
        if (now_ms >= latest && (now_ms - latest) >= _.networkidle_threshold_ms) {
            f.lifecycle_mask |= FrameLifecycleState.NetworkIdle.bit();
            return true;
        }
        return f.hasReached(FrameLifecycleState.NetworkIdle);
    }

    /// Dispatch a raw CDP Page event JSON payload directly into FrameManager.
    pub fn handleCdpEvent(*_, method: []const u8, params_json: []const u8, now_ms: i64): bool!PlaywrightError {
        if (std.mem.eql(u8, method, "Page.frameAttached")) {
            const fid = CdpFrameScanner.extractTopLevelField(params_json, "frameId", true) orelse return false;
            const pid = CdpFrameScanner.extractTopLevelField(params_json, "parentFrameId", true) orelse "";
            const attached = _.onFrameAttached(fid, pid)?;
            return attached.frame_id.len > 0;
        }
        if (std.mem.eql(u8, method, "Page.frameDetached")) {
            const fid = CdpFrameScanner.extractTopLevelField(params_json, "frameId", true) orelse return false;
            const cnt = _.onFrameDetached(fid);
            return cnt > 0;
        }
        if (std.mem.eql(u8, method, "Page.lifecycleEvent")) {
            const fid = CdpFrameScanner.extractTopLevelField(params_json, "frameId", true) orelse return false;
            const lid = CdpFrameScanner.extractTopLevelField(params_json, "loaderId", true) orelse "";
            const name = CdpFrameScanner.extractTopLevelField(params_json, "name", true) orelse return false;
            return _.onLifecycleEvent(fid, lid, name, now_ms);
        }
        if (std.mem.eql(u8, method, "Page.frameNavigated")) {
            const frame_obj = CdpFrameScanner.extractTopLevelField(params_json, "frame", false) orelse params_json;
            const fid = CdpFrameScanner.extractTopLevelField(frame_obj, "id", true) orelse return false;
            const pid = CdpFrameScanner.extractTopLevelField(frame_obj, "parentId", true) orelse "";
            const fname = CdpFrameScanner.extractTopLevelField(frame_obj, "name", true) orelse "";
            const url = CdpFrameScanner.extractTopLevelField(frame_obj, "url", true) orelse "about:blank";
            const lid = CdpFrameScanner.extractTopLevelField(frame_obj, "loaderId", true) orelse "";
            const nav = _.onFrameNavigated(fid, pid, fname, url, lid, now_ms)?;
            return nav.frame_id.len > 0;
        }
        return false;
    }

    pub fn mainFrame(*_): (*ManagedFrame)? {
        if (_.main_frame_id.len == 0) return null;
        const idx = _.findIndex(_.main_frame_id) orelse return null;
        return &_.frames.items[idx];
    }

    pub fn findFrame(*_, frame_id: []const u8): (*ManagedFrame)? {
        const idx = _.findIndex(frame_id) orelse return null;
        return &_.frames.items[idx];
    }

    pub fn findFrameByName(*_, target_name: []const u8): (*ManagedFrame)? {
        let i: usize = 0;
        while (i < _.frames.items.len) : (i += 1) {
            if (!_.frames.items[i].detached && std.mem.eql(u8, _.frames.items[i].name, target_name)) {
                return &_.frames.items[i];
            }
        }
        return null;
    }

    pub fn findFrameByUrl(*_, url_pattern: []const u8): (*ManagedFrame)? {
        let i: usize = 0;
        while (i < _.frames.items.len) : (i += 1) {
            if (!_.frames.items[i].detached && UrlMatcher.matches(url_pattern, _.frames.items[i].url)) {
                return &_.frames.items[i];
            }
        }
        return null;
    }

    pub fn childFrameIds(alloc: std.mem.Allocator, parent_id: []const u8): ([][]const u8)!PlaywrightError {
        let out: std.ArrayList([]const u8) = .empty;
        errdefer out.deinit(alloc);
        for (f of _.frames.items) {
            if (!f.detached && std.mem.eql(u8, f.parent_id, parent_id)) {
                out.append(alloc, f.frame_id) catch return PlaywrightError.OutOfMemory;
            }
        }
        return out.toOwnedSlice(alloc) catch return PlaywrightError.OutOfMemory;
    }

    pub fn activeFrameCount(): usize {
        let count: usize = 0;
        for (f of _.frames.items) {
            if (!f.detached) count += 1;
        }
        return count;
    }

    pub fn isLifecycleReached(frame_id: []const u8, state: FrameLifecycleState): bool {
        const idx = _.findIndex(frame_id) orelse return false;
        return _.frames.items[idx].hasReached(state);
    }
}

/// Snapshot returned by a single DOM probe iteration inside DomActionabilityLoop.
pub struct ActionabilityProbeSnapshot {
    pub attached: bool = true,
    pub visible: bool = true,
    pub enabled: bool = true,
    pub editable: bool = true,
    pub in_viewport: bool = true,
    pub hit_target_ok: bool = true,
    pub box: ElementBox = .{},
    pub obscured_by: []const u8 = "",
}

pub type ActionabilityProbeFn = fn(ctx: usize, selector: []const u8, attempt: u32, scrolled: bool): ActionabilityProbeSnapshot;

/// Full Playwright DOM Actionability retry state machine (absorbed from server/dom.ts).
/// Performs: re-location -> attached/visible/enabled/editable -> 2-frame ElementBox
/// stability verification -> auto scrollIntoView -> elementFromPoint hit-target check
/// -> stepped timeout backoff.
pub struct DomActionabilityLoop {
    pub static fn backoffDelayMs(attempt: u32): u32 {
        const schedule: [7]u32 = [0, 20, 50, 100, 100, 250, 500];
        if (attempt < 7) return schedule[attempt];
        return 500;
    }

    pub static fn run(
        selector: []const u8,
        checks: []const ActionabilityCheckKind,
        opts: ActionabilityOptions,
        probe_fn: ActionabilityProbeFn,
        probe_ctx: usize,
    ): ActionabilityReport!PlaywrightError {
        if (selector.len == 0) return PlaywrightError.InvalidSelector;

        let report = ActionabilityReport{
            state: ActionabilityState.Pending,
            attempts: 0,
            box: .{},
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

        let need_visible = false;
        let need_hidden = false;
        let need_stable = false;
        let need_enabled = false;
        let need_editable = false;
        let need_detached = false;

        for (chk of checks) {
            switch (chk) {
                .Visible => { need_visible = true; },
                .Hidden => { need_hidden = true; },
                .Stable => { need_stable = true; },
                .Enabled => { need_enabled = true; },
                .Editable => { need_editable = true; },
                .Attached => {},
                .Detached => { need_detached = true; },
            }
        }

        let elapsed_ms: u32 = 0;
        let has_prev_box = false;
        let prev_box = ElementBox{};
        let scrolled = false;
        let last_reason: []const u8 = "Timeout waiting for element actionability";
        let last_state = ActionabilityState.Timeout;

        while (true) {
            const attempt = report.attempts;
            report.attempts += 1;

            const snap = probe_fn(probe_ctx, selector, attempt, scrolled);

            // 1. Detached / Hidden target states
            if (need_detached) {
                if (!snap.attached) {
                    report.state = ActionabilityState.Done;
                    report.scrolled = scrolled;
                    return report;
                }
                last_reason = "Element is still attached to the DOM";
                last_state = ActionabilityState.Timeout;
            } else if (need_hidden) {
                if (!snap.attached || !snap.visible || !snap.box.isVisibleArea()) {
                    report.state = ActionabilityState.Hidden;
                    report.scrolled = scrolled;
                    return report;
                }
                last_reason = "Element is still visible";
                last_state = ActionabilityState.Timeout;
            } else if (!snap.attached) {
                // Element detached mid-loop -> re-locate on next iteration and reset stability
                has_prev_box = false;
                last_reason = "Element is not attached to the DOM";
                last_state = ActionabilityState.NotConnected;
            } else if (need_visible && (!snap.visible || !snap.box.isVisibleArea())) {
                has_prev_box = false;
                last_reason = "Element is not visible";
                last_state = ActionabilityState.Timeout;
            } else if (need_enabled && !snap.enabled) {
                last_reason = "Element is disabled";
                last_state = ActionabilityState.Timeout;
            } else if (need_editable && !snap.editable) {
                last_reason = "Element is not editable";
                last_state = ActionabilityState.Timeout;
            } else if (need_stable && (!has_prev_box || !prev_box.equals(snap.box))) {
                // 2-frame consecutive ElementBox comparison to detect animation stop
                prev_box = snap.box;
                has_prev_box = true;
                report.box = snap.box;
                last_reason = "Element is not stable (animating)";
                last_state = ActionabilityState.Timeout;
            } else if (opts.scroll_into_view && !snap.in_viewport && !scrolled) {
                // Auto scrollIntoView and re-verify on next frame
                scrolled = true;
                report.scrolled = true;
                has_prev_box = false;
                last_reason = "Element is outside of the viewport";
                last_state = ActionabilityState.Timeout;
            } else if (opts.check_hit_target && !snap.hit_target_ok) {
                // elementFromPoint occlusion detected -> back off and retry
                report.hit_target_ok = false;
                last_reason = if (snap.obscured_by.len > 0) snap.obscured_by else "Element is obscured by another element";
                last_state = ActionabilityState.Timeout;
            } else {
                // All actionability gates passed!
                report.box = snap.box;
                report.scrolled = scrolled;
                report.hit_target_ok = if (opts.check_hit_target) snap.hit_target_ok else true;
                report.state = ActionabilityState.Done;
                report.error_message = "";
                return report;
            }

            const delay = DomActionabilityLoop.backoffDelayMs(attempt);
            if (elapsed_ms + delay >= opts.timeout_ms && attempt > 0) {
                report.state = last_state;
                report.scrolled = scrolled;
                report.error_message = last_reason;
                return report;
            }
            elapsed_ms += if (delay == 0) 10 else delay;
        }
    }

    /// Convenience bridge that executes actionability checks over a live cap:cdp endpoint.
    pub static fn runOverCdp(
        endpoint: *CdpEndpoint,
        session_id: []const u8,
        selector: []const u8,
        checks: []const ActionabilityCheckKind,
        opts: ActionabilityOptions,
    ): ActionabilityReport!PlaywrightError {
        return ActionabilityEngine.waitForActionable(endpoint, session_id, selector, checks, opts);
    }
}
