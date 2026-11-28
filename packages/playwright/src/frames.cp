// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/frames.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/frames.cp — Multi-Frame Tree & Cross-Origin Frames
// Zero-copy frame hierarchy (mainFrame, childFrames, parentFrame, frameByName,
// frameByUrl, locator, goto, evaluate) backed by cap:cdp.
// ============================================================================

const std = @import("std");
import {
    CdpEndpoint,
    CdpConnectionState,
    PageDomain,
    PageNavigateParams,
    RuntimeDomain,
    RuntimeEvaluateParams,
} from "cap:cdp";
import {
    PlaywrightError,
    Response,
    UrlMatcher,
} from "./network";
import { Locator } from "./locator";

pub struct Frame {
    allocator: std.mem.Allocator = undefined,
    pub endpoint: *CdpEndpoint = undefined,
    pub session_id: []const u8 = "",
    pub frame_id: []const u8 = "",
    pub frame_name: []const u8 = "",
    pub frame_url: []const u8 = "about:blank",
    pub default_timeout_ms: u32 = 30000,
    pub detached: bool = false,

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
            parent_frame: parent,
            child_frames: .empty,
            owned_strings: .empty,
        };
    }

    pub fn deinit(*_) {
        _.detached = true;
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

    pub fn isDetached(): bool {
        return _.detached;
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
}
