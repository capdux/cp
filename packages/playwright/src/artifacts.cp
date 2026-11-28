// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/download.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/artifacts.cp — Download, FileChooser, ConsoleMessage & HarRecorder
// Uses cap:json (JSON.stringifyValue + JsonValue) for zero-copy HAR 1.2
// serialization and 16-lane @Vector(16, u8) SIMD scanning for URL/HAR checks.
// ============================================================================

const std = @import("std");
import { JSON, JsonValue, JsonField } from "cap:json";
import {
    CdpEndpoint,
    CdpConnectionState,
} from "cap:cdp";
import { PlaywrightError, UrlMatcher } from "./network";

type ArtifactsVec16 = @Vector(16, u8);

/// 16-lane SIMD scanner: returns index of first JSON-escapable or control byte, or null if clean.
pub fn simdScanHarCleanSpan(data: []const u8): usize? {
    let i: usize = 0;
    const v_quote: ArtifactsVec16 = @splat('"');
    const v_bslash: ArtifactsVec16 = @splat('\\');
    const v_ctrl: ArtifactsVec16 = @splat(0x20);
    while (i + 16 <= data.len) : (i += 16) {
        const chunk: ArtifactsVec16 = data[i..][0..16].*;
        const mask = (chunk == v_quote) | (chunk == v_bslash) | (chunk < v_ctrl);
        if (@reduce(.Or, mask)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                const b = data[i + k];
                if (b == '"' || b == '\\' || b < 0x20) return i + k;
            }
        }
    }
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b == '"' || b == '\\' || b < 0x20) return i;
    }
    return null;
}

pub struct Download {
    allocator: std.mem.Allocator = undefined,
    pub url: []const u8 = "",
    pub uuid: []const u8 = "",
    pub suggested_filename: []const u8 = "",
    pub save_path: []const u8 = "",
    pub canceled: bool = false,
    pub failure_reason: []const u8 = "",
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(
        alloc: std.mem.Allocator,
        url: []const u8,
        uuid: []const u8,
        suggested_filename: []const u8,
    ): Download!PlaywrightError {
        let dl = Download{
            allocator: alloc,
            url: "",
            uuid: "",
            suggested_filename: "",
            save_path: "",
            canceled: false,
            failure_reason: "",
            owned_strings: .empty,
        };
        errdefer dl.deinit();
        dl.url = dl.intern(url)?;
        dl.uuid = dl.intern(uuid)?;
        dl.suggested_filename = dl.intern(suggested_filename)?;
        return dl;
    }

    pub fn deinit(*_) {
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

    pub fn suggestedFilename(): []const u8 {
        return _.suggested_filename;
    }

    pub fn saveAs(*_, dest_path: []const u8): void!PlaywrightError {
        if (_.canceled) return PlaywrightError.ProtocolError;
        if (dest_path.len == 0) return PlaywrightError.InvalidUrl;
        _.save_path = _.intern(dest_path)?;
    }

    pub fn path(): ([]const u8)? {
        if (_.canceled || _.save_path.len == 0) return null;
        return _.save_path;
    }

    pub fn cancel(*_) {
        _.canceled = true;
        _.failure_reason = "canceled";
    }
}

pub struct FileChooser {
    allocator: std.mem.Allocator = undefined,
    endpoint: *CdpEndpoint? = null,
    session_id: []const u8 = "",
    pub selector: []const u8 = "",
    pub is_multiple: bool = false,
    pub files: std.ArrayList([]u8) = .empty,

    pub static fn init(
        alloc: std.mem.Allocator,
        selector: []const u8,
        is_multiple: bool,
        endpoint: *CdpEndpoint?,
        session_id: []const u8,
    ): FileChooser {
        return FileChooser{
            allocator: alloc,
            endpoint: endpoint,
            session_id: session_id,
            selector: selector,
            is_multiple: is_multiple,
            files: .empty,
        };
    }

    pub fn deinit(*_) {
        for (f of _.files.items) {
            _.allocator.free(f);
        }
        _.files.deinit(_.allocator);
        _.files = .empty;
    }

    ~init() {
        _.deinit();
    }

    pub fn isMultiple(): bool {
        return _.is_multiple;
    }

    pub fn setFiles(*_, file_paths: []const []const u8): void!PlaywrightError {
        if (!_.is_multiple && file_paths.len > 1) return PlaywrightError.ProtocolError;
        for (f of _.files.items) {
            _.allocator.free(f);
        }
        _.files.clearRetainingCapacity();

        for (p of file_paths) {
            const duped = _.allocator.dupe(u8, p) catch return PlaywrightError.OutOfMemory;
            _.files.append(_.allocator, duped) catch {
                _.allocator.free(duped);
                return PlaywrightError.OutOfMemory;
            };
        }

        if (ep := _.endpoint) {
            if (ep.state == CdpConnectionState.Connected && file_paths.len > 0) {
                let params_buf: [512]u8 = undefined;
                let frame_buf: [768]u8 = undefined;
                const p = std.fmt.bufPrint(
                    &params_buf,
                    "{{\"files\":[\"{s}\"]}}",
                    .{file_paths[0]},
                ) catch return PlaywrightError.BufferTooSmall;
                const cmd_id = ep.sendCommand(_.session_id, "DOM.setFileInputFiles", p, &frame_buf) catch return PlaywrightError.ProtocolError;
                const resp = ep.takeResponse(cmd_id);
                * = resp;
            }
        }
    }
}

pub struct ConsoleMessage {
    pub msg_type: []const u8 = "log",
    pub text: []const u8 = "",
    pub url: []const u8 = "",
    pub line_number: u32 = 0,
    pub column_number: u32 = 0,
    pub timestamp_ms: i64 = 0,

    pub fn type_(): []const u8 {
        return _.msg_type;
    }
}

pub struct HarRecordOptions {
    pub har_path: []const u8 = "",
    pub minimal: bool = false,
    pub url_glob: []const u8 = "",
}

pub struct HarEntry {
    pub started_date_time: []const u8 = "2026-11-28T00:00:00.000Z",
    pub time_ms: f64 = 0.0,
    pub method: []const u8 = "GET",
    pub url: []const u8 = "",
    pub status: i32 = 200,
    pub status_text: []const u8 = "OK",
    pub mime_type: []const u8 = "text/plain",
    pub response_body: []const u8 = "",
    pub body_size: usize = 0,
}

pub struct HarRecorder {
    allocator: std.mem.Allocator = undefined,
    pub options: HarRecordOptions = .{},
    pub entries: std.ArrayList(HarEntry) = .empty,
    pub flushed: bool = false,
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(alloc: std.mem.Allocator, opts: HarRecordOptions): HarRecorder {
        return HarRecorder{
            allocator: alloc,
            options: opts,
            entries: .empty,
            flushed: false,
            owned_strings: .empty,
        };
    }

    pub fn deinit(*_) {
        _.entries.deinit(_.allocator);
        _.entries = .empty;
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

    pub fn recordEntry(*_, entry: HarEntry): bool!PlaywrightError {
        if (_.options.url_glob.len > 0) {
            if (!UrlMatcher.matches(_.options.url_glob, entry.url)) return false;
        }
        const dt = _.intern(entry.started_date_time)?;
        const m = _.intern(entry.method)?;
        const u = _.intern(entry.url)?;
        const st = _.intern(entry.status_text)?;
        const mt = _.intern(entry.mime_type)?;
        const rb = if (_.options.minimal) "" else _.intern(entry.response_body)?;
        _.entries.append(_.allocator, HarEntry{
            started_date_time: dt,
            time_ms: entry.time_ms,
            method: m,
            url: u,
            status: entry.status,
            status_text: st,
            mime_type: mt,
            response_body: rb,
            body_size: entry.body_size,
        }) catch return PlaywrightError.OutOfMemory;
        return true;
    }

    pub fn entryCount(): usize {
        return _.entries.items.len;
    }

    /// Serialize HAR 1.2 document using cap:json `JsonValue` + `JSON.stringifyValue`.
    pub fn toJson(*_, alloc: std.mem.Allocator): ([]u8)!PlaywrightError {
        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        let entry_vals = aa.alloc(JsonValue, _.entries.items.len) catch return PlaywrightError.OutOfMemory;
        let idx: usize = 0;
        while (idx < _.entries.items.len) : (idx += 1) {
            const e = _.entries.items[idx];
            let req_fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            req_fields[0] = JsonField{ key: "method", value: JsonValue{ tag: .String, str_val: e.method } };
            req_fields[1] = JsonField{ key: "url", value: JsonValue{ tag: .String, str_val: e.url } };

            let content_fields = aa.alloc(JsonField, 3) catch return PlaywrightError.OutOfMemory;
            content_fields[0] = JsonField{ key: "size", value: JsonValue{ tag: .Number, num_val: @floatFromInt(e.body_size) } };
            content_fields[1] = JsonField{ key: "mimeType", value: JsonValue{ tag: .String, str_val: e.mime_type } };
            content_fields[2] = JsonField{ key: "text", value: JsonValue{ tag: .String, str_val: e.response_body } };

            let resp_fields = aa.alloc(JsonField, 3) catch return PlaywrightError.OutOfMemory;
            resp_fields[0] = JsonField{ key: "status", value: JsonValue{ tag: .Number, num_val: @floatFromInt(e.status) } };
            resp_fields[1] = JsonField{ key: "statusText", value: JsonValue{ tag: .String, str_val: e.status_text } };
            resp_fields[2] = JsonField{ key: "content", value: JsonValue{ tag: .Object, obj_val: content_fields } };

            let entry_fields = aa.alloc(JsonField, 4) catch return PlaywrightError.OutOfMemory;
            entry_fields[0] = JsonField{ key: "startedDateTime", value: JsonValue{ tag: .String, str_val: e.started_date_time } };
            entry_fields[1] = JsonField{ key: "time", value: JsonValue{ tag: .Number, num_val: e.time_ms } };
            entry_fields[2] = JsonField{ key: "request", value: JsonValue{ tag: .Object, obj_val: req_fields } };
            entry_fields[3] = JsonField{ key: "response", value: JsonValue{ tag: .Object, obj_val: resp_fields } };

            entry_vals[idx] = JsonValue{ tag: .Object, obj_val: entry_fields };
        }

        let creator_fields = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
        creator_fields[0] = JsonField{ key: "name", value: JsonValue{ tag: .String, str_val: "Playwright-CP" } };
        creator_fields[1] = JsonField{ key: "version", value: JsonValue{ tag: .String, str_val: "1.66.0-next" } };

        let log_fields = aa.alloc(JsonField, 3) catch return PlaywrightError.OutOfMemory;
        log_fields[0] = JsonField{ key: "version", value: JsonValue{ tag: .String, str_val: "1.2" } };
        log_fields[1] = JsonField{ key: "creator", value: JsonValue{ tag: .Object, obj_val: creator_fields } };
        log_fields[2] = JsonField{ key: "entries", value: JsonValue{ tag: .Array, arr_val: entry_vals } };

        let root_fields = aa.alloc(JsonField, 1) catch return PlaywrightError.OutOfMemory;
        root_fields[0] = JsonField{ key: "log", value: JsonValue{ tag: .Object, obj_val: log_fields } };

        const root = JsonValue{ tag: .Object, obj_val: root_fields };
        _.flushed = true;
        return JSON.stringifyValue(alloc, root) catch return PlaywrightError.OutOfMemory;
    }
}
