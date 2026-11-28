// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/FrameManager.ts | Synced: 2026-11-28
// Also absorbs _ref/puppeteer/packages/puppeteer-core/src/cdp/ExecutionContext.ts
// ============================================================================
// packages/puppeteer/src/frame_manager.cp — ExecutionContext, IsolatedWorld & FrameManager
// Dynamic Frame tree management, Runtime execution contexts & lifecycle state machine.
// ============================================================================

const std = @import("std");
import {
    CdpEvent,
    CdpFrameScanner,
} from "cap:cdp";
import {
    PuppeteerError,
    WaitUntil,
    EvaluateResult,
} from "./types";
import { CDPSession } from "./cdp_session";
import { Frame } from "./frame";
import {
    EventHandlerFn,
    PuppeteerEventEmitter,
} from "./events";

type FrameVec16 = @Vector(16, u8);

pub struct FrameSimdScanner {
    /// 16-lane SIMD scan for a URL delimiter (e.g. '#', '?', '/')
    pub static fn indexOfByteSimd(haystack: []const u8, needle: u8): usize? {
        let i: usize = 0;
        const n = haystack.len;
        const v_needle: FrameVec16 = @splat(needle);
        while (i + 16 <= n) {
            const chunk: FrameVec16 = haystack[i..][0..16].*;
            const mask: u16 = @bitCast(chunk == v_needle);
            if (mask != 0) {
                return i + (@ctz(mask) as usize);
            }
            i += 16;
        }
        while (i < n) {
            if (haystack[i] == needle) return i;
            i += 1;
        }
        return null;
    }

    /// Strip URL fragment `#...` using 16-lane SIMD scan.
    pub static fn stripFragment(url: []const u8): []const u8 {
        if (idx := FrameSimdScanner.indexOfByteSimd(url, '#')) {
            return url[0..idx];
        }
        return url;
    }
}

pub struct ExecutionContext {
    session: *CDPSession = undefined,
    context_id: i64 = 0,
    frame_id_str: []const u8 = "",
    origin_str: []const u8 = "",
    name_str: []const u8 = "",
    unique_id_str: []const u8 = "",
    is_default_flag: bool = true,
    destroyed_flag: bool = false,

    pub static fn init(
        session: *CDPSession,
        context_id: i64,
        frame_id_val: []const u8,
        origin_val: []const u8,
        name_val: []const u8,
        unique_id_val: []const u8,
        is_default_val: bool,
    ): ExecutionContext {
        return ExecutionContext{
            session: session,
            context_id: context_id,
            frame_id_str: frame_id_val,
            origin_str: origin_val,
            name_str: name_val,
            unique_id_str: unique_id_val,
            is_default_flag: is_default_val,
            destroyed_flag: false,
        };
    }

    pub fn id(): i64 {
        return _.context_id;
    }

    pub fn frameId(): []const u8 {
        return _.frame_id_str;
    }

    pub fn origin(): []const u8 {
        return _.origin_str;
    }

    pub fn name(): []const u8 {
        return _.name_str;
    }

    pub fn uniqueId(): []const u8 {
        return _.unique_id_str;
    }

    pub fn isDefault(): bool {
        return _.is_default_flag;
    }

    pub fn isDestroyed(): bool {
        return _.destroyed_flag || _.session.detached();
    }

    pub fn markDestroyed(*_) {
        _.destroyed_flag = true;
    }

    /// Safe evaluation bound strictly to this contextId (prevents cross-navigation execution).
    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError {
        if (_.isDestroyed()) return PuppeteerError.SessionClosed;
        let pbuf: [2048]u8 = undefined;
        let fbuf: [2560]u8 = undefined;
        const params = std.fmt.bufPrint(
            &pbuf,
            "{{\"expression\":\"{s}\",\"contextId\":{d},\"returnByValue\":true,\"awaitPromise\":true}}",
            .{ expression, _.context_id },
        ) catch return PuppeteerError.BufferTooSmall;
        const req_id = _.session.send("Runtime.evaluate", params, &fbuf)?;
        if (resp := _.session.endpoint.takeResponse(req_id)) {
            if (resp.is_error) {
                return EvaluateResult{
                    value_json: "null",
                    value_type: "error",
                    exception_text: resp.error_message,
                    has_exception: true,
                };
            }
            if (exc := CdpFrameScanner.extractTopLevelField(resp.result_json, "exceptionDetails", false)) {
                const etext = CdpFrameScanner.extractTopLevelField(exc, "text", true) orelse "Uncaught exception";
                return EvaluateResult{
                    value_json: "null",
                    value_type: "error",
                    exception_text: etext,
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
}

pub struct IsolatedWorld {
    frame_id_str: []const u8 = "",
    world_name: []const u8 = "",
    context_ptr: (*ExecutionContext)? = null,

    pub static fn init(frame_id: []const u8, world_name: []const u8): IsolatedWorld {
        return IsolatedWorld{
            frame_id_str: frame_id,
            world_name: world_name,
            context_ptr: null,
        };
    }

    pub fn frameId(): []const u8 {
        return _.frame_id_str;
    }

    pub fn name(): []const u8 {
        return _.world_name;
    }

    pub fn hasContext(): bool {
        if (ctx := _.context_ptr) {
            return !ctx.isDestroyed();
        }
        return false;
    }

    pub fn executionContext(): (*ExecutionContext)? {
        if (ctx := _.context_ptr) {
            if (!ctx.isDestroyed()) return ctx;
        }
        return null;
    }

    pub fn setContext(*_, ctx: *ExecutionContext) {
        _.context_ptr = ctx;
    }

    pub fn clearContext(*_) {
        _.context_ptr = null;
    }

    pub fn evaluate(*_, expression: []const u8, out_buf: []u8): EvaluateResult!PuppeteerError {
        if (ctx := _.executionContext()) {
            return ctx.evaluate(expression, out_buf);
        }
        return PuppeteerError.SessionClosed;
    }
}

pub struct ManagedFrame {
    frame: Frame = .{},
    loader_id_str: []const u8 = "",
    main_world: IsolatedWorld = .{},
    puppeteer_world: IsolatedWorld = .{},
    lifecycle_events: [16][]const u8 = undefined,
    lifecycle_len: usize = 0,
    has_load: bool = false,
    has_dom_content_loaded: bool = false,
    has_network_idle: bool = false,
    has_network_almost_idle: bool = false,

    pub static fn init(session: *CDPSession, frame_id: []const u8, parent_frame_id: []const u8): ManagedFrame {
        return ManagedFrame{
            frame: Frame.init(session, frame_id, parent_frame_id),
            loader_id_str: "",
            main_world: IsolatedWorld.init(frame_id, ""),
            puppeteer_world: IsolatedWorld.init(frame_id, "__puppeteer_utility_world__"),
            lifecycle_len: 0,
            has_load: false,
            has_dom_content_loaded: false,
            has_network_idle: false,
            has_network_almost_idle: false,
        };
    }

    pub fn id(): []const u8 {
        return _.frame.id();
    }

    pub fn loaderId(): []const u8 {
        return _.loader_id_str;
    }

    pub fn url(): []const u8 {
        return _.frame.url();
    }

    pub fn name(): []const u8 {
        return _.frame.name();
    }

    pub fn isDetached(): bool {
        return _.frame.isDetached();
    }

    pub fn lifecycleEvents(): []const []const u8 {
        return _.lifecycle_events[0.._.lifecycle_len];
    }

    pub fn resetLifecycle(*_, new_loader_id: []const u8) {
        _.loader_id_str = new_loader_id;
        _.lifecycle_len = 0;
        _.has_load = false;
        _.has_dom_content_loaded = false;
        _.has_network_idle = false;
        _.has_network_almost_idle = false;
    }

    pub fn recordLifecycleEvent(*_, event_name: []const u8) {
        if (std.mem.eql(u8, event_name, "load")) {
            _.has_load = true;
        } else if (std.mem.eql(u8, event_name, "DOMContentLoaded")) {
            _.has_dom_content_loaded = true;
        } else if (std.mem.eql(u8, event_name, "networkIdle")) {
            _.has_network_idle = true;
        } else if (std.mem.eql(u8, event_name, "networkAlmostIdle")) {
            _.has_network_almost_idle = true;
        }
        let i: usize = 0;
        while (i < _.lifecycle_len) {
            if (std.mem.eql(u8, _.lifecycle_events[i], event_name)) return;
            i += 1;
        }
        if (_.lifecycle_len < 16) {
            _.lifecycle_events[_.lifecycle_len] = event_name;
            _.lifecycle_len += 1;
        }
    }

    pub fn hasLifecycleEvent(event_name: []const u8): bool {
        let i: usize = 0;
        while (i < _.lifecycle_len) {
            if (std.mem.eql(u8, _.lifecycle_events[i], event_name)) return true;
            i += 1;
        }
        return false;
    }

    pub fn isWaitUntilSatisfied(wait_until: WaitUntil): bool {
        switch (wait_until) {
            WaitUntil.Load => return _.has_load,
            WaitUntil.DomContentLoaded => return _.has_dom_content_loaded,
            WaitUntil.NetworkIdle0 => return _.has_network_idle,
            WaitUntil.NetworkIdle2 => return _.has_network_almost_idle || _.has_network_idle,
        }
    }
}

pub struct FrameManager {
    allocator: std.mem.Allocator = undefined,
    session: *CDPSession = undefined,
    emitter: PuppeteerEventEmitter = .{},
    frames_list: std.ArrayList(*ManagedFrame) = .empty,
    contexts_list: std.ArrayList(*ExecutionContext) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,
    main_frame_id: []const u8 = "",
    disposed: bool = false,

    pub static fn init(alloc: std.mem.Allocator, session: *CDPSession): FrameManager {
        return FrameManager{
            allocator: alloc,
            session: session,
            emitter: PuppeteerEventEmitter.init(),
            frames_list: .empty,
            contexts_list: .empty,
            owned_strings: .empty,
            main_frame_id: "",
            disposed: false,
        };
    }

    pub fn deinit(*_) {
        if (_.disposed) return;
        _.disposed = true;

        for (f of _.frames_list.items) {
            _.allocator.destroy(f);
        }
        _.frames_list.deinit(_.allocator);
        _.frames_list = .empty;

        for (ctx of _.contexts_list.items) {
            _.allocator.destroy(ctx);
        }
        _.contexts_list.deinit(_.allocator);
        _.contexts_list = .empty;

        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;

        _.emitter.removeAllListeners("");
    }

    ~init() {
        _.deinit();
    }

    fn intern(*_, s: []const u8): ([]const u8)!PuppeteerError {
        if (s.len == 0) return "";
        const duped = _.allocator.dupe(u8, s) catch return PuppeteerError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PuppeteerError.OutOfMemory;
        };
        return duped;
    }

    pub fn on(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool {
        return _.emitter.on(event_name, handler, ctx);
    }

    pub fn once(*_, event_name: []const u8, handler: EventHandlerFn, ctx: usize): bool {
        return _.emitter.once(event_name, handler, ctx);
    }

    pub fn off(*_, event_name: []const u8, handler: EventHandlerFn) {
        _.emitter.off(event_name, handler);
    }

    pub fn findManagedFrame(frame_id: []const u8): (*ManagedFrame)? {
        for (mf of _.frames_list.items) {
            if (std.mem.eql(u8, mf.frame.frame_id, frame_id)) {
                return mf;
            }
        }
        return null;
    }

    pub fn mainFrame(): (*ManagedFrame)? {
        if (_.main_frame_id.len == 0) return null;
        return _.findManagedFrame(_.main_frame_id);
    }

    pub fn frames(): []const *ManagedFrame {
        return _.frames_list.items;
    }

    pub fn frameCount(): usize {
        return _.frames_list.items.len;
    }

    pub fn activeFrameCount(): usize {
        let total: usize = 0;
        for (mf of _.frames_list.items) {
            if (!mf.frame.detached_flag) total += 1;
        }
        return total;
    }

    pub fn contextById(context_id: i64): (*ExecutionContext)? {
        for (ctx of _.contexts_list.items) {
            if (ctx.context_id == context_id) return ctx;
        }
        return null;
    }

    pub fn contextCount(): usize {
        return _.contexts_list.items.len;
    }

    pub fn activeContextCount(): usize {
        let total: usize = 0;
        for (ctx of _.contexts_list.items) {
            if (!ctx.destroyed_flag) total += 1;
        }
        return total;
    }

    /// Central CDP event router for Page.* frame/lifecycle & Runtime.executionContext* events.
    pub fn handleCdpEvent(*_, ev: CdpEvent): void!PuppeteerError {
        if (std.mem.eql(u8, ev.method, "Page.frameAttached")) {
            return _.onFrameAttached(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Page.frameNavigated")) {
            return _.onFrameNavigated(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Page.frameDetached")) {
            _.onFrameDetached(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Page.navigatedWithinDocument")) {
            return _.onNavigatedWithinDocument(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Page.lifecycleEvent")) {
            return _.onLifecycleEvent(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Runtime.executionContextCreated")) {
            return _.onExecutionContextCreated(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Runtime.executionContextDestroyed")) {
            _.onExecutionContextDestroyed(ev.params_json);
        } else if (std.mem.eql(u8, ev.method, "Runtime.executionContextsCleared")) {
            _.onExecutionContextsCleared();
        }
    }

    fn ensureFrame(*_, frame_id: []const u8, parent_frame_id: []const u8): (*ManagedFrame)!PuppeteerError {
        if (existing := _.findManagedFrame(frame_id)) {
            return existing;
        }
        const fid = _.intern(frame_id)?;
        const pid = _.intern(parent_frame_id)?;
        let mf = _.allocator.create(ManagedFrame) catch return PuppeteerError.OutOfMemory;
        mf.* = ManagedFrame.init(_.session, fid, pid);
        _.frames_list.append(_.allocator, mf) catch {
            _.allocator.destroy(mf);
            return PuppeteerError.OutOfMemory;
        };
        if (pid.len == 0 && _.main_frame_id.len == 0) {
            _.main_frame_id = fid;
        }
        if (pid.len > 0) {
            if (parent := _.findManagedFrame(pid)) {
                parent.frame.addChildFrame(*mf.frame);
            }
        }
        return mf;
    }

    static fn setFrameUrl(mf: *ManagedFrame, new_url: []const u8) {
        const n = if (new_url.len <= 512) new_url.len else 512;
        if (n > 0) @memcpy(mf.frame.url_buf[0..n], new_url[0..n]);
        mf.frame.url_len = n;
    }

    /// Handles `Page.frameAttached`.
    pub fn onFrameAttached(*_, params_json: []const u8): void!PuppeteerError {
        const frame_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "frameId", true) orelse return;
        const parent_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "parentFrameId", true) orelse "";
        const mf = _.ensureFrame(frame_id_raw, parent_id_raw)?;
        const _ign_ev = _.emitter.emit("frameattached", mf.frame.frame_id);
        * = _ign_ev;
    }

    /// Handles `Page.frameNavigated`.
    pub fn onFrameNavigated(*_, params_json: []const u8): void!PuppeteerError {
        const frame_obj = CdpFrameScanner.extractTopLevelField(params_json, "frame", false) orelse return;
        const frame_id_raw = CdpFrameScanner.extractTopLevelField(frame_obj, "id", true) orelse return;
        const parent_id_raw = CdpFrameScanner.extractTopLevelField(frame_obj, "parentId", true) orelse "";
        const loader_id_raw = CdpFrameScanner.extractTopLevelField(frame_obj, "loaderId", true) orelse "";
        const name_raw = CdpFrameScanner.extractTopLevelField(frame_obj, "name", true) orelse "";
        const url_raw = CdpFrameScanner.extractTopLevelField(frame_obj, "url", true) orelse "about:blank";
        const url_frag_raw = CdpFrameScanner.extractTopLevelField(frame_obj, "urlFragment", true) orelse "";

        const mf = _.ensureFrame(frame_id_raw, parent_id_raw)?;
        if (parent_id_raw.len == 0) {
            _.main_frame_id = mf.frame.frame_id;
        }
        if (name_raw.len > 0) {
            const name_owned = _.intern(name_raw)?;
            mf.frame.setName(name_owned);
        }

        // Compose full URL (url + optional urlFragment) on stack buffer
        let full_url_buf: [512]u8 = undefined;
        let full_url: []const u8 = url_raw;
        if (url_frag_raw.len > 0) {
            full_url = std.fmt.bufPrint(&full_url_buf, "{s}{s}", .{ url_raw, url_frag_raw }) catch url_raw;
        }
        FrameManager.setFrameUrl(mf, full_url);

        if (loader_id_raw.len > 0 && !std.mem.eql(u8, mf.loader_id_str, loader_id_raw)) {
            const lid = _.intern(loader_id_raw)?;
            mf.resetLifecycle(lid);
        }

        const _ign_ev = _.emitter.emit("framenavigated", mf.frame.url());
        * = _ign_ev;
    }

    /// Handles `Page.navigatedWithinDocument` (SPA pushState / hashchange).
    pub fn onNavigatedWithinDocument(*_, params_json: []const u8): void!PuppeteerError {
        const frame_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "frameId", true) orelse return;
        const url_raw = CdpFrameScanner.extractTopLevelField(params_json, "url", true) orelse return;
        const mf = _.findManagedFrame(frame_id_raw) orelse return;
        FrameManager.setFrameUrl(mf, url_raw);
        const _ign_ev = _.emitter.emit("framenavigatedwithindocument", mf.frame.url());
        * = _ign_ev;
    }

    fn detachFrameRecursive(*_, mf: *ManagedFrame) {
        if (mf.frame.detached_flag) return;
        mf.frame.detached_flag = true;
        mf.main_world.clearContext();
        mf.puppeteer_world.clearContext();

        // Mark all execution contexts belonging to this frame as destroyed
        for (ctx of _.contexts_list.items) {
            if (std.mem.eql(u8, ctx.frame_id_str, mf.frame.frame_id)) {
                ctx.markDestroyed();
            }
        }

        // Recursively detach child frames
        for (other of _.frames_list.items) {
            if (std.mem.eql(u8, other.frame.parent_frame_id, mf.frame.frame_id)) {
                _.detachFrameRecursive(other);
            }
        }
    }

    /// Handles `Page.frameDetached`.
    pub fn onFrameDetached(*_, params_json: []const u8) {
        const frame_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "frameId", true) orelse return;
        const mf = _.findManagedFrame(frame_id_raw) orelse return;
        _.detachFrameRecursive(mf);
        const _ign_ev = _.emitter.emit("framedetached", mf.frame.frame_id);
        * = _ign_ev;
    }

    /// Handles `Page.lifecycleEvent` (`init`, `DOMContentLoaded`, `load`, `networkIdle`, `networkAlmostIdle`).
    pub fn onLifecycleEvent(*_, params_json: []const u8): void!PuppeteerError {
        const frame_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "frameId", true) orelse return;
        const loader_id_raw = CdpFrameScanner.extractTopLevelField(params_json, "loaderId", true) orelse "";
        const name_raw = CdpFrameScanner.extractTopLevelField(params_json, "name", true) orelse return;

        const mf = _.findManagedFrame(frame_id_raw) orelse return;
        const ev_name = _.intern(name_raw)?;

        if (std.mem.eql(u8, ev_name, "init") && loader_id_raw.len > 0) {
            const lid = _.intern(loader_id_raw)?;
            mf.resetLifecycle(lid);
        }
        mf.recordLifecycleEvent(ev_name);

        const _ign_1 = _.emitter.emit("lifecycleevent", ev_name);
        * = _ign_1;
        if (std.mem.eql(u8, ev_name, "load")) {
            const _ign_2 = _.emitter.emit("load", mf.frame.frame_id);
            * = _ign_2;
        } else if (std.mem.eql(u8, ev_name, "DOMContentLoaded")) {
            const _ign_3 = _.emitter.emit("domcontentloaded", mf.frame.frame_id);
            * = _ign_3;
        } else if (std.mem.eql(u8, ev_name, "networkIdle")) {
            const _ign_4 = _.emitter.emit("networkidle", mf.frame.frame_id);
            * = _ign_4;
        }
    }

    /// Handles `Runtime.executionContextCreated`.
    pub fn onExecutionContextCreated(*_, params_json: []const u8): void!PuppeteerError {
        const ctx_obj = CdpFrameScanner.extractTopLevelField(params_json, "context", false) orelse return;
        const id_raw = CdpFrameScanner.extractTopLevelField(ctx_obj, "id", false) orelse return;
        const ctx_id = CdpFrameScanner.parseFastI64(id_raw) orelse return;
        const origin_raw = CdpFrameScanner.extractTopLevelField(ctx_obj, "origin", true) orelse "";
        const name_raw = CdpFrameScanner.extractTopLevelField(ctx_obj, "name", true) orelse "";
        const unique_id_raw = CdpFrameScanner.extractTopLevelField(ctx_obj, "uniqueId", true) orelse "";

        let frame_id_raw: []const u8 = "";
        let is_default = true;
        if (aux_obj := CdpFrameScanner.extractTopLevelField(ctx_obj, "auxData", false)) {
            frame_id_raw = CdpFrameScanner.extractTopLevelField(aux_obj, "frameId", true) orelse "";
            const is_def_raw = CdpFrameScanner.extractTopLevelField(aux_obj, "isDefault", false) orelse "true";
            is_default = std.mem.eql(u8, is_def_raw, "true");
        }

        const fid = _.intern(frame_id_raw)?;
        const origin = _.intern(origin_raw)?;
        const name = _.intern(name_raw)?;
        const uid = _.intern(unique_id_raw)?;

        let ctx = _.allocator.create(ExecutionContext) catch return PuppeteerError.OutOfMemory;
        ctx.* = ExecutionContext.init(_.session, ctx_id, fid, origin, name, uid, is_default);
        _.contexts_list.append(_.allocator, ctx) catch {
            _.allocator.destroy(ctx);
            return PuppeteerError.OutOfMemory;
        };

        if (fid.len > 0) {
            if (mf := _.findManagedFrame(fid)) {
                if (is_default) {
                    mf.main_world.setContext(ctx);
                } else {
                    mf.puppeteer_world.setContext(ctx);
                }
            }
        }
    }

    /// Handles `Runtime.executionContextDestroyed`.
    pub fn onExecutionContextDestroyed(*_, params_json: []const u8) {
        const id_raw = CdpFrameScanner.extractTopLevelField(params_json, "executionContextId", false) orelse return;
        const ctx_id = CdpFrameScanner.parseFastI64(id_raw) orelse return;

        for (ctx of _.contexts_list.items) {
            if (ctx.context_id == ctx_id) {
                ctx.markDestroyed();
                if (mf := _.findManagedFrame(ctx.frame_id_str)) {
                    if (mc := mf.main_world.context_ptr) {
                        if (mc.context_id == ctx_id) mf.main_world.clearContext();
                    }
                    if (pc := mf.puppeteer_world.context_ptr) {
                        if (pc.context_id == ctx_id) mf.puppeteer_world.clearContext();
                    }
                }
            }
        }
    }

    /// Handles `Runtime.executionContextsCleared`.
    pub fn onExecutionContextsCleared(*_) {
        for (ctx of _.contexts_list.items) {
            ctx.markDestroyed();
        }
        for (mf of _.frames_list.items) {
            mf.main_world.clearContext();
            mf.puppeteer_world.clearContext();
        }
    }

    pub fn isLifecycleSatisfied(frame_id: []const u8, wait_until: WaitUntil): bool {
        if (mf := _.findManagedFrame(frame_id)) {
            return mf.isWaitUntilSatisfied(wait_until);
        }
        return false;
    }
}
