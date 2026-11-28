// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/Tracing.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/tracing.cp — Page Tracing Controller
// Zero-heap-allocation Tracing.start / Tracing.end wrapper using cap:cdp TracingDomain
// ============================================================================

const std = @import("std");
import { CdpFrameScanner, TracingDomain } from "cap:cdp";
import { PuppeteerError } from "./types";
import { CDPSession } from "./cdp_session";

extern fn open(path: [*:0]const u8, oflag: i32, mode: u32): i32;
extern fn write(fd: i32, buf: [*]const u8, count: usize): isize;
extern fn close(fd: i32): i32;

pub struct TracingOptions {
    path: []const u8 = "",
    screenshots: bool = false,
    categories: []const []const u8 = &.{},
    buffer_size: u32 = 0,
}

pub struct Tracing {
    session: *CDPSession = undefined,
    recording: bool = false,
    path_buf: [512]u8 = undefined,
    path_len: usize = 0,

    pub static fn init(session: *CDPSession): Tracing {
        return Tracing{
            session: session,
            recording: false,
            path_len: 0,
        };
    }

    pub fn isRecording(): bool {
        return _.recording;
    }

    pub fn start(*_, options: TracingOptions): void!PuppeteerError {
        if (_.recording) return PuppeteerError.InvalidArgument;

        let cat_buf: [1024]u8 = undefined;
        let cat_len: usize = 0;

        if (options.categories.len > 0) {
            let i: usize = 0;
            while (i < options.categories.len) : (i += 1) {
                const c = options.categories[i];
                if (i > 0) {
                    if (cat_len + 1 > cat_buf.len) return PuppeteerError.BufferTooSmall;
                    cat_buf[cat_len] = ',';
                    cat_len += 1;
                }
                if (cat_len + c.len > cat_buf.len) return PuppeteerError.BufferTooSmall;
                if (c.len > 0) @memcpy(cat_buf[cat_len .. cat_len + c.len], c);
                cat_len += c.len;
            }
        } else {
            const def_cats = "-*,devtools.timeline,v8.execute,disabled-by-default-devtools.timeline,disabled-by-default-devtools.timeline.frame,toplevel,blink.console,blink.user_timing,latencyInfo,disabled-by-default-devtools.timeline.stack,disabled-by-default-v8.cpu_profiler";
            @memcpy(cat_buf[0..def_cats.len], def_cats);
            cat_len = def_cats.len;
        }

        if (options.screenshots) {
            const ss_cat = ",disabled-by-default-devtools.screenshot";
            if (cat_len + ss_cat.len > cat_buf.len) return PuppeteerError.BufferTooSmall;
            @memcpy(cat_buf[cat_len .. cat_len + ss_cat.len], ss_cat);
            cat_len += ss_cat.len;
        }

        let pbuf: [1536]u8 = undefined;
        let params: []const u8 = "";
        if (options.buffer_size > 0) {
            params = std.fmt.bufPrint(
                &pbuf,
                "{{\"categories\":\"{s}\",\"transferMode\":\"ReturnAsStream\",\"traceBufferSizeInKb\":{d}}}",
                .{ cat_buf[0..cat_len], options.buffer_size },
            ) catch return PuppeteerError.BufferTooSmall;
        } else {
            const req = TracingDomain.start(0, cat_buf[0..cat_len], &pbuf) catch return PuppeteerError.BufferTooSmall;
            params = req.params_json;
        }

        const pn = if (options.path.len <= _.path_buf.len) options.path.len else _.path_buf.len;
        if (pn > 0) @memcpy(_.path_buf[0..pn], options.path[0..pn]);
        _.path_len = pn;

        let fbuf: [2048]u8 = undefined;
        const _ign = _.session.send("Tracing.start", params, &fbuf)?;
        * = _ign;
        _.recording = true;
    }

    pub fn stop(*_, out_buf: []u8): ([]const u8)!PuppeteerError {
        if (!_.recording) return PuppeteerError.InvalidArgument;
        _.recording = false;

        const end_req = TracingDomain.end(0);
        let fbuf: [1024]u8 = undefined;
        const resp = _.session.sendAndTakeResponse(end_req.method, end_req.params_json, &fbuf)?;
        if (resp.is_error) return PuppeteerError.ProtocolError;

        let trace_data: []const u8 = "";
        if (data_field := CdpFrameScanner.extractTopLevelField(resp.result_json, "data", true)) {
            trace_data = data_field;
        } else if (stream_id := CdpFrameScanner.extractTopLevelField(resp.result_json, "stream", true)) {
            let io_pbuf: [256]u8 = undefined;
            const io_params = std.fmt.bufPrint(
                &io_pbuf,
                "{{\"handle\":\"{s}\"}}",
                .{stream_id},
            ) catch return PuppeteerError.BufferTooSmall;
            const io_resp = _.session.sendAndTakeResponse("IO.read", io_params, &fbuf)?;
            if (!io_resp.is_error) {
                trace_data = CdpFrameScanner.extractTopLevelField(io_resp.result_json, "data", true) orelse "";
            }
            let close_buf: [512]u8 = undefined;
            const _ign_close = _.session.send("IO.close", io_params, &close_buf) catch 0;
            * = _ign_close;
        }

        const n = if (trace_data.len <= out_buf.len) trace_data.len else out_buf.len;
        if (n > 0) @memcpy(out_buf[0..n], trace_data[0..n]);

        if (_.path_len > 0 && n > 0 && _.path_len < 512) {
            let zpath: [512]u8 = undefined;
            @memcpy(zpath[0.._.path_len], _.path_buf[0.._.path_len]);
            zpath[_.path_len] = 0;
            const zptr: [*:0]const u8 = unsafe { @ptrCast(&zpath[0]) };
            // O_WRONLY (1) | O_CREAT (64) | O_TRUNC (512) = 577, mode 0o644 = 420
            const fd = unsafe { open(zptr, 577, 420) };
            if (fd >= 0) {
                const wptr: [*]const u8 = unsafe { @ptrCast(&out_buf[0]) };
                * = unsafe { write(fd, wptr, n) };
                * = unsafe { close(fd) };
            }
        }

        return out_buf[0..n];
    }
}
