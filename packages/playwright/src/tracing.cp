// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/trace/recorder/tracing.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/tracing.cp — Playwright Trace Recorder & ZIP Packager
// Records trace events & writes PKZIP trace archives via cap:pack (ZipWriter)
// and cap:json (JSON.stringifyValue) with ArenaAllocator.
// ============================================================================

const std = @import("std");
import { CdpEndpoint, CdpConnectionState } from "cap:cdp";
import { JSON, JsonValue, JsonField } from "cap:json";
import { ZipWriter } from "cap:pack";
import { writeFileSync } from "cap:fs";
import { PlaywrightError } from "./network";

pub struct TracingOptions {
    pub name: []const u8 = "trace",
    pub title: []const u8 = "",
    pub screenshots: bool = false,
    pub snapshots: bool = false,
    pub sources: bool = false,
}

pub struct TraceEvent {
    pub event_type: []const u8 = "action",
    pub timestamp_ms: i64 = 0,
    pub method: []const u8 = "",
    pub page_id: []const u8 = "",
    pub detail_json: []const u8 = "{}",
}

pub struct Tracing {
    allocator: std.mem.Allocator = undefined,
    endpoint: *CdpEndpoint = undefined,
    context_id: []const u8 = "",
    pub recording: bool = false,
    pub chunk_active: bool = false,
    pub options: TracingOptions = .{},
    pub chunk_title: []const u8 = "",

    events: std.ArrayList(TraceEvent) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(
        alloc: std.mem.Allocator,
        endpoint: *CdpEndpoint,
        context_id: []const u8,
    ): Tracing {
        return Tracing{
            allocator: alloc,
            endpoint: endpoint,
            context_id: context_id,
            recording: false,
            chunk_active: false,
            options: .{},
            chunk_title: "",
            events: .empty,
            owned_strings: .empty,
        };
    }

    pub fn deinit(*_) {
        _.recording = false;
        _.chunk_active = false;
        _.events.deinit(_.allocator);
        _.events = .empty;
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

    pub fn isRecording(): bool {
        return _.recording;
    }

    pub fn eventCount(): usize {
        return _.events.items.len;
    }

    pub fn start(*_, opts: TracingOptions): void!PlaywrightError {
        if (_.recording) return PlaywrightError.ProtocolError;
        _.options = opts;
        _.options.name = _.intern(if (opts.name.len > 0) opts.name else "trace")?;
        _.options.title = _.intern(opts.title)?;
        _.recording = true;
        _.chunk_active = true;
        _.chunk_title = _.options.title;
        _.events.clearRetainingCapacity();

        if (_.endpoint.state == CdpConnectionState.Connected) {
            let frame_buf: [512]u8 = undefined;
            const cmd_id = _.endpoint.sendCommand("", "Tracing.start", "{}", &frame_buf) catch 0;
            if (cmd_id > 0) {
                * = _.endpoint.takeResponse(cmd_id);
            }
        }

        _.recordEvent("context-options", "BrowserContext.tracing.start", _.context_id, "{}")?;
    }

    pub fn startChunk(*_, title: []const u8): void!PlaywrightError {
        if (!_.recording) return PlaywrightError.ProtocolError;
        _.chunk_active = true;
        _.chunk_title = _.intern(title)?;
        _.events.clearRetainingCapacity();
        _.recordEvent("context-options", "BrowserContext.tracing.startChunk", _.context_id, "{}")?;
    }

    pub fn recordEvent(
        *_,
        event_type: []const u8,
        method: []const u8,
        page_id: []const u8,
        detail_json: []const u8,
    ): void!PlaywrightError {
        if (!_.recording) return;
        const et = _.intern(event_type)?;
        const m = _.intern(method)?;
        const pid = _.intern(page_id)?;
        const dj = _.intern(if (detail_json.len > 0) detail_json else "{}")?;
        const ts: i64 = ((_.events.items.len as i64) + 1) * 10;
        _.events.append(_.allocator, TraceEvent{
            event_type: et,
            timestamp_ms: ts,
            method: m,
            page_id: pid,
            detail_json: dj,
        }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn stopChunk(*_, out_path: []const u8): ([]u8)!PlaywrightError {
        if (!_.recording || !_.chunk_active) return PlaywrightError.ProtocolError;
        _.chunk_active = false;

        let arena = std.heap.ArenaAllocator.init(_.allocator);
        defer arena.deinit();
        const aa = arena.allocator();

        let ndjson: std.ArrayList(u8) = .empty;
        for (ev of _.events.items) {
            let fields = aa.alloc(JsonField, 5) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "type", value: JsonValue{ tag: .String, str_val: ev.event_type } };
            fields[1] = JsonField{ key: "time", value: JsonValue{ tag: .Number, num_val: @floatFromInt(ev.timestamp_ms) } };
            fields[2] = JsonField{ key: "method", value: JsonValue{ tag: .String, str_val: ev.method } };
            fields[3] = JsonField{ key: "pageId", value: JsonValue{ tag: .String, str_val: ev.page_id } };
            fields[4] = JsonField{ key: "detail", value: JsonValue{ tag: .String, str_val: ev.detail_json } };

            const line = JSON.stringifyValue(aa, JsonValue{ tag: .Object, obj_val: fields }) catch return PlaywrightError.OutOfMemory;
            ndjson.appendSlice(aa, line) catch return PlaywrightError.OutOfMemory;
            ndjson.append(aa, '\n') catch return PlaywrightError.OutOfMemory;
        }

        let zw = ZipWriter.init();
        if (!zw.addStored("trace.trace", ndjson.items)) return PlaywrightError.ProtocolError;
        const meta = std.fmt.allocPrint(
            aa,
            "{{\"version\":6,\"contextId\":\"{s}\",\"title\":\"{s}\",\"screenshots\":{},\"snapshots\":{}}}",
            .{ _.context_id, _.chunk_title, _.options.screenshots, _.options.snapshots },
        ) catch return PlaywrightError.OutOfMemory;
        if (!zw.addStored("trace.network", "")) return PlaywrightError.ProtocolError;
        if (!zw.addStored("trace.meta.json", meta)) return PlaywrightError.OutOfMemory;

        const raw_zip = zw.finish();
        const out_zip = _.allocator.dupe(u8, raw_zip) catch return PlaywrightError.OutOfMemory;
        errdefer _.allocator.free(out_zip);

        if (out_path.len > 0) {
            writeFileSync(out_path, out_zip);
        }

        return out_zip;
    }

    pub fn stop(*_, out_path: []const u8): ([]u8)!PlaywrightError {
        if (!_.recording) return PlaywrightError.ProtocolError;

        if (_.endpoint.state == CdpConnectionState.Connected) {
            let frame_buf: [512]u8 = undefined;
            const cmd_id = _.endpoint.sendCommand("", "Tracing.end", "{}", &frame_buf) catch 0;
            if (cmd_id > 0) {
                * = _.endpoint.takeResponse(cmd_id);
            }
        }

        if (_.chunk_active) {
            const zip_bytes = _.stopChunk(out_path)?;
            _.recording = false;
            return zip_bytes;
        }

        _.recording = false;
        return _.allocator.alloc(u8, 0) catch return PlaywrightError.OutOfMemory;
    }
}
