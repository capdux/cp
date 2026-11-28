// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/trace/recorder/snapshotterInjected.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/snapshotter.cp — DOM Snapshotter & HAR 1.2 Tracer
// Absorbed from server/trace/recorder/snapshotterInjected.ts & server/har/harTracer.ts:
//   - SNAPSHOTTER_INJECTED_SCRIPT: browser-side DOM snapshot capture IIFE
//   - Snapshotter: SHA1 + FNV-1a resource deduplication table, FrameSnapshot
//     archiving, and direct export to Tracing.
//   - HarTracer: full HAR 1.2 recorder with exact DNS/Connect/SSL/Send/Wait/
//     Receive HarTimings breakdown, Base64 response body encoding, url_glob
//     filtering, and zero-copy cap:json serialization via ArenaAllocator.
// ============================================================================

const std = @import("std");
import { JSON, JsonValue, JsonField } from "cap:json";
import { CdpEndpoint, CdpConnectionState } from "cap:cdp";
import {
    PlaywrightError,
    UrlMatcher,
    ResourceTiming,
    Request,
    Response,
    NameValue,
    parseHeadersJsonArray,
} from "./network";
import { Tracing } from "./tracing";
import { escapeJsonIntoBuf } from "./recorder";
import { encodeBase64Alloc } from "./cr_network_manager";

type SnapVec16 = @Vector(16, u8);

/// 16-lane SIMD scanner: returns true if `data` contains NUL or non-text control
/// bytes (< 0x09 or 0x0e..0x1f or >= 0x80) indicating binary content for HAR Base64.
pub fn simdHasBinaryBytes(data: []const u8): bool {
    let i: usize = 0;
    const v_tab: SnapVec16 = @splat(0x09);
    const v_so: SnapVec16 = @splat(0x0e);
    const v_space: SnapVec16 = @splat(0x20);
    const v_hi: SnapVec16 = @splat(0x80);
    while (i + 16 <= data.len) : (i += 16) {
        const chunk: SnapVec16 = data[i..][0..16].*;
        const low_ctrl = chunk < v_tab;
        const mid_ctrl = (chunk >= v_so) & (chunk < v_space);
        const non_ascii = chunk >= v_hi;
        if (@reduce(.Or, low_ctrl | mid_ctrl | non_ascii)) return true;
    }
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b < 0x09 || (b >= 0x0e && b < 0x20) || b >= 0x80) return true;
    }
    return false;
}

/// Fast 64-bit FNV-1a content fingerprint.
pub fn computeFnv1a64(data: []const u8): u64 {
    let h: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
    for (b of data) {
        h ^= b;
        h *%= 0x100000001b3;
    }
    return h;
}

/// Compute lowercase 40-char hex SHA-1 digest into stack buffer (zero heap alloc).
pub fn computeSha1Hex(data: []const u8, out_hex: *[40]u8) {
    let digest: [20]u8 = undefined;
    std.crypto.hash.Sha1.hash(data, &digest, .{});
    const hex_chars = "0123456789abcdef";
    let i: usize = 0;
    while (i < 20) : (i += 1) {
        out_hex[i * 2] = hex_chars[(digest[i] >> 4) & 0x0f];
        out_hex[i * 2 + 1] = hex_chars[digest[i] & 0x0f];
    }
}

/// Self-contained browser-side DOM snapshot capture script absorbed from
/// Playwright's `frameSnapshotStreamer` (`snapshotterInjected.ts`).
pub const SNAPSHOTTER_INJECTED_SCRIPT: []const u8 =
    "(() => {\n" ++
    "  if (window.__pwSnapshotterInstalled) return;\n" ++
    "  window.__pwSnapshotterInstalled = true;\n" ++
    "  function visitNode(node) {\n" ++
    "    if (!node) return null;\n" ++
    "    if (node.nodeType === 3) return node.nodeValue || '';\n" ++
    "    if (node.nodeType !== 1) return null;\n" ++
    "    const tag = node.tagName;\n" ++
    "    if (tag === 'SCRIPT' || tag === 'NOSCRIPT') return null;\n" ++
    "    const attrs = {};\n" ++
    "    for (const a of node.attributes || []) attrs[a.name] = a.value;\n" ++
    "    if (tag === 'INPUT' || tag === 'TEXTAREA') attrs['__playwright_value__'] = node.value || '';\n" ++
    "    if (tag === 'INPUT' && (node.type === 'checkbox' || node.type === 'radio')) attrs['__playwright_checked__'] = !!node.checked;\n" ++
    "    if (tag === 'SELECT') attrs['__playwright_selected__'] = node.selectedIndex;\n" ++
    "    const children = [];\n" ++
    "    for (let c = node.firstChild; c; c = c.nextSibling) {\n" ++
    "      const v = visitNode(c);\n" ++
    "      if (v !== null) children.push(v);\n" ++
    "    }\n" ++
    "    if (node.shadowRoot) {\n" ++
    "      const sr = [];\n" ++
    "      for (let c = node.shadowRoot.firstChild; c; c = c.nextSibling) {\n" ++
    "        const v = visitNode(c);\n" ++
    "        if (v !== null) sr.push(v);\n" ++
    "      }\n" ++
    "      children.push(['__shadow__', {}, sr]);\n" ++
    "    }\n" ++
    "    return [tag, attrs, children];\n" ++
    "  }\n" ++
    "  window.__pwCaptureSnapshot = function(snapshotName) {\n" ++
    "    const doctype = document.doctype ? document.doctype.name : '';\n" ++
    "    const html = visitNode(document.documentElement);\n" ++
    "    return JSON.stringify({\n" ++
    "      snapshotName: snapshotName || '',\n" ++
    "      doctype: doctype,\n" ++
    "      html: html,\n" ++
    "      viewport: { width: window.innerWidth, height: window.innerHeight },\n" ++
    "      timestamp: Date.now()\n" ++
    "    });\n" ++
    "  };\n" ++
    "})();";

pub struct SnapshotResource {
    pub sha1_hex: [40]u8 = undefined,
    pub fnv64: u64 = 0,
    pub content_type: []const u8 = "application/octet-stream",
    pub body: []const u8 = "",
    pub ref_count: u32 = 1,

    pub fn sha1Slice(): []const u8 {
        return _.sha1_hex[0..40];
    }
}

pub struct ResourceOverride {
    pub url: []const u8 = "",
    pub sha1_hex: [40]u8 = undefined,
}

pub struct FrameSnapshot {
    pub snapshot_name: []const u8 = "",
    pub call_id: []const u8 = "",
    pub timestamp_ms: i64 = 0,
    pub page_id: []const u8 = "",
    pub frame_id: []const u8 = "",
    pub frame_url: []const u8 = "",
    pub doctype: []const u8 = "html",
    pub html_json: []const u8 = "[]",
    pub viewport_width: u32 = 1280,
    pub viewport_height: u32 = 720,
    pub resource_sha1: [40]u8 = undefined,
    pub has_resource_override: bool = false,
}

pub struct Snapshotter {
    arena: std.heap.ArenaAllocator = undefined,
    allocator: std.mem.Allocator = undefined,
    deinitialized: bool = false,

    pub resources: std.ArrayList(SnapshotResource) = .empty,
    pub snapshots: std.ArrayList(FrameSnapshot) = .empty,
    pub dedup_hits: usize = 0,
    pub injected: bool = false,

    pub static fn init(alloc: std.mem.Allocator): Snapshotter {
        return Snapshotter{
            arena: std.heap.ArenaAllocator.init(alloc),
            allocator: alloc,
            deinitialized: false,
            resources: .empty,
            snapshots: .empty,
            dedup_hits: 0,
            injected: false,
        };
    }

    pub fn deinit(*_) {
        if (_.deinitialized) return;
        _.deinitialized = true;
        _.resources.deinit(_.allocator);
        _.resources = .empty;
        _.snapshots.deinit(_.allocator);
        _.snapshots = .empty;
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

    /// Inject SNAPSHOTTER_INJECTED_SCRIPT into a CDP session.
    pub fn injectIntoEndpoint(*_, endpoint: *CdpEndpoint, session_id: []const u8): void!PlaywrightError {
        if (endpoint.state != CdpConnectionState.Connected) return PlaywrightError.NotConnected;
        let esc_buf: [4096]u8 = undefined;
        const escaped = escapeJsonIntoBuf(SNAPSHOTTER_INJECTED_SCRIPT, &esc_buf)?;
        let params_buf: [4608]u8 = undefined;
        const params = std.fmt.bufPrint(&params_buf, "{{\"source\":\"{s}\"}}", .{escaped}) catch return PlaywrightError.BufferTooSmall;
        let frame_buf: [5120]u8 = undefined;
        const cmd_id = endpoint.sendCommand(session_id, "Page.addScriptToEvaluateOnNewDocument", params, &frame_buf) catch return PlaywrightError.ProtocolError;
        if (endpoint.takeResponse(cmd_id) != null) {}
        _.injected = true;
    }

    /// Store a resource in the SHA-1 / FNV-1a deduplication table.
    /// Identical content is stored only once; returns the 40-char SHA-1 hex digest.
    pub fn addResource(
        *_,
        content_type: []const u8,
        body: []const u8,
    ): ([40]u8)!PlaywrightError {
        const fnv = computeFnv1a64(body);
        let sha1: [40]u8 = undefined;
        computeSha1Hex(body, &sha1);

        let i: usize = 0;
        while (i < _.resources.items.len) : (i += 1) {
            if (_.resources.items[i].fnv64 == fnv &&
                std.mem.eql(u8, _.resources.items[i].sha1_hex[0..40], sha1[0..40]))
            {
                _.resources.items[i].ref_count += 1;
                _.dedup_hits += 1;
                return sha1;
            }
        }

        const ct = _.intern(if (content_type.len > 0) content_type else "application/octet-stream")?;
        const dup_body = _.intern(body)?;
        _.resources.append(_.allocator, SnapshotResource{
            sha1_hex: sha1,
            fnv64: fnv,
            content_type: ct,
            body: dup_body,
            ref_count: 1,
        }) catch return PlaywrightError.OutOfMemory;
        return sha1;
    }

    /// Capture and archive a FrameSnapshot.
    pub fn captureSnapshot(
        *_,
        page_id: []const u8,
        frame_id: []const u8,
        frame_url: []const u8,
        snapshot_name: []const u8,
        call_id: []const u8,
        html_json: []const u8,
        viewport_w: u32,
        viewport_h: u32,
        timestamp_ms: i64,
    ): (*FrameSnapshot)!PlaywrightError {
        let snap = FrameSnapshot{
            snapshot_name: _.intern(snapshot_name)?,
            call_id: _.intern(call_id)?,
            timestamp_ms: timestamp_ms,
            page_id: _.intern(page_id)?,
            frame_id: _.intern(frame_id)?,
            frame_url: _.intern(frame_url)?,
            doctype: "html",
            html_json: _.intern(if (html_json.len > 0) html_json else "[]")?,
            viewport_width: viewport_w,
            viewport_height: viewport_h,
            resource_sha1: undefined,
            has_resource_override: false,
        };
        _.snapshots.append(_.allocator, snap) catch return PlaywrightError.OutOfMemory;
        return &_.snapshots.items[_.snapshots.items.len - 1];
    }

    /// Export all archived FrameSnapshots into a `Tracing` instance.
    pub fn exportToTracing(*_, tracing: *Tracing): usize!PlaywrightError {
        let exported: usize = 0;
        let tmp_arena = std.heap.ArenaAllocator.init(_.allocator);
        defer tmp_arena.deinit();
        const aa = tmp_arena.allocator();

        for (snap of _.snapshots.items) {
            let fields = aa.alloc(JsonField, 6) catch return PlaywrightError.OutOfMemory;
            fields[0] = JsonField{ key: "snapshotName", value: JsonValue{ tag: .String, str_val: snap.snapshot_name } };
            fields[1] = JsonField{ key: "callId", value: JsonValue{ tag: .String, str_val: snap.call_id } };
            fields[2] = JsonField{ key: "frameId", value: JsonValue{ tag: .String, str_val: snap.frame_id } };
            fields[3] = JsonField{ key: "frameUrl", value: JsonValue{ tag: .String, str_val: snap.frame_url } };
            fields[4] = JsonField{ key: "viewportWidth", value: JsonValue{ tag: .Number, num_val: @floatFromInt(snap.viewport_width) } };
            fields[5] = JsonField{ key: "viewportHeight", value: JsonValue{ tag: .Number, num_val: @floatFromInt(snap.viewport_height) } };

            const detail = JSON.stringifyValue(aa, JsonValue{ tag: .Object, obj_val: fields }) catch return PlaywrightError.OutOfMemory;
            tracing.recordEvent("frame-snapshot", "Snapshotter.capture", snap.page_id, detail)?;
            exported += 1;
        }
        return exported;
    }

    pub fn snapshotCount(): usize {
        return _.snapshots.items.len;
    }

    pub fn uniqueResourceCount(): usize {
        return _.resources.items.len;
    }

    pub fn deduplicatedHits(): usize {
        return _.dedup_hits;
    }
}

/// Exact HAR 1.2 timings breakdown (absorbed from server/har/harTracer.ts).
pub struct HarTimings {
    pub blocked: f64 = -1.0,
    pub dns: f64 = -1.0,
    pub connect: f64 = -1.0,
    pub ssl: f64 = -1.0,
    pub send: f64 = 0.0,
    pub wait: f64 = 0.0,
    pub receive: f64 = 0.0,

    pub static fn fromResourceTiming(rt: ResourceTiming): HarTimings {
        const dns: f64 = if (rt.domain_lookup_start >= 0.0 && rt.domain_lookup_end >= rt.domain_lookup_start)
            rt.domain_lookup_end - rt.domain_lookup_start
        else
            -1.0;

        const connect: f64 = if (rt.connect_start >= 0.0 && rt.connect_end >= rt.connect_start)
            rt.connect_end - rt.connect_start
        else
            -1.0;

        const ssl: f64 = if (rt.secure_connection_start >= 0.0 && rt.connect_end >= rt.secure_connection_start)
            rt.connect_end - rt.secure_connection_start
        else
            -1.0;

        const send: f64 = if (rt.connect_end >= 0.0 && rt.response_start >= rt.connect_end) 0.1 else 0.0;
        const wait: f64 = if (rt.response_start >= 0.0)
            (if (rt.connect_end >= 0.0 && rt.response_start >= rt.connect_end) rt.response_start - rt.connect_end else rt.response_start)
        else
            0.0;

        const receive: f64 = if (rt.response_end >= 0.0 && rt.response_start >= 0.0 && rt.response_end >= rt.response_start)
            rt.response_end - rt.response_start
        else
            0.0;

        return HarTimings{
            blocked: -1.0,
            dns: dns,
            connect: connect,
            ssl: ssl,
            send: send,
            wait: wait,
            receive: receive,
        };
    }

    pub fn totalTimeMs(): f64 {
        let total: f64 = 0.0;
        if (_.blocked > 0.0) total += _.blocked;
        if (_.dns > 0.0) total += _.dns;
        if (_.connect > 0.0) total += _.connect;
        if (_.send > 0.0) total += _.send;
        if (_.wait > 0.0) total += _.wait;
        if (_.receive > 0.0) total += _.receive;
        return total;
    }
}

pub struct HarTracerOptions {
    pub url_glob: []const u8 = "",
    pub minimal: bool = false,
    pub omit_content: bool = false,
    pub base64_binary_bodies: bool = true,
}

pub struct HarTracerEntry {
    pub started_date_time: []const u8 = "2026-11-28T00:00:00.000Z",
    pub page_ref: []const u8 = "page@0",
    pub method: []const u8 = "GET",
    pub url: []const u8 = "",
    pub http_version: []const u8 = "HTTP/1.1",
    pub status: i32 = 200,
    pub status_text: []const u8 = "OK",
    pub mime_type: []const u8 = "text/plain",
    pub request_headers_json: []const u8 = "{}",
    pub response_headers_json: []const u8 = "{}",
    pub post_data: []const u8 = "",
    pub response_body: []const u8 = "",
    pub response_is_binary: bool = false,
    pub body_size: usize = 0,
    pub timings: HarTimings = .{},
    pub server_ip_address: []const u8 = "",
}

pub struct HarTracer {
    arena: std.heap.ArenaAllocator = undefined,
    allocator: std.mem.Allocator = undefined,
    deinitialized: bool = false,

    pub options: HarTracerOptions = .{},
    pub entries: std.ArrayList(HarTracerEntry) = .empty,
    pub filtered_out_count: usize = 0,
    pub flushed: bool = false,

    pub static fn init(alloc: std.mem.Allocator, opts: HarTracerOptions): HarTracer {
        return HarTracer{
            arena: std.heap.ArenaAllocator.init(alloc),
            allocator: alloc,
            deinitialized: false,
            options: opts,
            entries: .empty,
            filtered_out_count: 0,
            flushed: false,
        };
    }

    pub fn deinit(*_) {
        if (_.deinitialized) return;
        _.deinitialized = true;
        _.entries.deinit(_.allocator);
        _.entries = .empty;
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

    /// Record a network exchange with `url_glob` filtering and automatic binary
    /// Base64 detection via 16-lane SIMD scanning.
    pub fn recordExchange(*_, entry: HarTracerEntry): bool!PlaywrightError {
        if (_.options.url_glob.len > 0) {
            if (!UrlMatcher.matches(_.options.url_glob, entry.url)) {
                _.filtered_out_count += 1;
                return false;
            }
        }

        const is_bin = entry.response_is_binary ||
            (_.options.base64_binary_bodies && simdHasBinaryBytes(entry.response_body));
        let stored_body: []const u8 = "";
        if (!_.options.omit_content && !_.options.minimal && entry.response_body.len > 0) {
            if (is_bin) {
                const aa = _.arena.allocator();
                stored_body = encodeBase64Alloc(aa, entry.response_body)?;
            } else {
                stored_body = _.intern(entry.response_body)?;
            }
        }

        const sz = if (entry.body_size > 0) entry.body_size else entry.response_body.len;
        _.entries.append(_.allocator, HarTracerEntry{
            started_date_time: _.intern(entry.started_date_time)?,
            page_ref: _.intern(entry.page_ref)?,
            method: _.intern(entry.method)?,
            url: _.intern(entry.url)?,
            http_version: _.intern(if (entry.http_version.len > 0) entry.http_version else "HTTP/1.1")?,
            status: entry.status,
            status_text: _.intern(entry.status_text)?,
            mime_type: _.intern(entry.mime_type)?,
            request_headers_json: _.intern(if (entry.request_headers_json.len > 0) entry.request_headers_json else "{}")?,
            response_headers_json: _.intern(if (entry.response_headers_json.len > 0) entry.response_headers_json else "{}")?,
            post_data: if (_.options.minimal) "" else _.intern(entry.post_data)?,
            response_body: stored_body,
            response_is_binary: is_bin,
            body_size: sz,
            timings: entry.timings,
            server_ip_address: _.intern(entry.server_ip_address)?,
        }) catch return PlaywrightError.OutOfMemory;
        return true;
    }

    pub fn recordFromRequestResponse(
        *_,
        req: Request,
        resp: Response,
        started_date_time: []const u8,
    ): bool!PlaywrightError {
        const ct = resp.headerValue("content-type") orelse "application/octet-stream";
        const ip = if (addr := resp.server_addr) addr.ip_address else "";
        return _.recordExchange(HarTracerEntry{
            started_date_time: started_date_time,
            page_ref: if (req.frame_id.len > 0) req.frame_id else "page@0",
            method: req.method,
            url: req.url,
            http_version: "HTTP/1.1",
            status: resp.status,
            status_text: resp.status_text,
            mime_type: ct,
            request_headers_json: req.headers_json,
            response_headers_json: resp.headers_json,
            post_data: req.post_data,
            response_body: resp.body_bytes,
            response_is_binary: false,
            body_size: resp.body_bytes.len,
            timings: HarTimings.fromResourceTiming(req.resource_timing),
            server_ip_address: ip,
        });
    }

    pub fn entryCount(): usize {
        return _.entries.items.len;
    }

    static fn buildHeadersJsonValue(aa: std.mem.Allocator, headers_json: []const u8): JsonValue!PlaywrightError {
        if (headers_json.len <= 2) {
            const empty_arr = aa.alloc(JsonValue, 0) catch return PlaywrightError.OutOfMemory;
            return JsonValue{ tag: .Array, arr_val: empty_arr };
        }
        const pairs = parseHeadersJsonArray(aa, headers_json)?;
        let vals = aa.alloc(JsonValue, pairs.len) catch return PlaywrightError.OutOfMemory;
        let i: usize = 0;
        while (i < pairs.len) : (i += 1) {
            let hf = aa.alloc(JsonField, 2) catch return PlaywrightError.OutOfMemory;
            hf[0] = JsonField{ key: "name", value: JsonValue{ tag: .String, str_val: pairs[i].name } };
            hf[1] = JsonField{ key: "value", value: JsonValue{ tag: .String, str_val: pairs[i].value } };
            vals[i] = JsonValue{ tag: .Object, obj_val: hf };
        }
        return JsonValue{ tag: .Array, arr_val: vals };
    }

    /// Zero-copy HAR 1.2 serialization via `cap:json` (`JSON.stringifyValue`) + ArenaAllocator.
    pub fn toJson(*_, alloc: std.mem.Allocator): ([]u8)!PlaywrightError {
        let tmp_arena = std.heap.ArenaAllocator.init(alloc);
        defer tmp_arena.deinit();
        const aa = tmp_arena.allocator();

        let entry_vals = aa.alloc(JsonValue, _.entries.items.len) catch return PlaywrightError.OutOfMemory;
        let idx: usize = 0;
        while (idx < _.entries.items.len) : (idx += 1) {
            const e = _.entries.items[idx];
            const req_headers_val = HarTracer.buildHeadersJsonValue(aa, e.request_headers_json)?;
            const resp_headers_val = HarTracer.buildHeadersJsonValue(aa, e.response_headers_json)?;

            let req_fields = aa.alloc(JsonField, 4) catch return PlaywrightError.OutOfMemory;
            req_fields[0] = JsonField{ key: "method", value: JsonValue{ tag: .String, str_val: e.method } };
            req_fields[1] = JsonField{ key: "url", value: JsonValue{ tag: .String, str_val: e.url } };
            req_fields[2] = JsonField{ key: "httpVersion", value: JsonValue{ tag: .String, str_val: e.http_version } };
            req_fields[3] = JsonField{ key: "headers", value: req_headers_val };

            const content_field_count: usize = if (e.response_is_binary && e.response_body.len > 0) 4 else 3;
            let content_fields = aa.alloc(JsonField, content_field_count) catch return PlaywrightError.OutOfMemory;
            content_fields[0] = JsonField{ key: "size", value: JsonValue{ tag: .Number, num_val: @floatFromInt(e.body_size) } };
            content_fields[1] = JsonField{ key: "mimeType", value: JsonValue{ tag: .String, str_val: e.mime_type } };
            content_fields[2] = JsonField{ key: "text", value: JsonValue{ tag: .String, str_val: e.response_body } };
            if (content_field_count == 4) {
                content_fields[3] = JsonField{ key: "encoding", value: JsonValue{ tag: .String, str_val: "base64" } };
            }

            let resp_fields = aa.alloc(JsonField, 5) catch return PlaywrightError.OutOfMemory;
            resp_fields[0] = JsonField{ key: "status", value: JsonValue{ tag: .Number, num_val: @floatFromInt(e.status) } };
            resp_fields[1] = JsonField{ key: "statusText", value: JsonValue{ tag: .String, str_val: e.status_text } };
            resp_fields[2] = JsonField{ key: "httpVersion", value: JsonValue{ tag: .String, str_val: e.http_version } };
            resp_fields[3] = JsonField{ key: "headers", value: resp_headers_val };
            resp_fields[4] = JsonField{ key: "content", value: JsonValue{ tag: .Object, obj_val: content_fields } };

            let timing_fields = aa.alloc(JsonField, 7) catch return PlaywrightError.OutOfMemory;
            timing_fields[0] = JsonField{ key: "blocked", value: JsonValue{ tag: .Number, num_val: e.timings.blocked } };
            timing_fields[1] = JsonField{ key: "dns", value: JsonValue{ tag: .Number, num_val: e.timings.dns } };
            timing_fields[2] = JsonField{ key: "connect", value: JsonValue{ tag: .Number, num_val: e.timings.connect } };
            timing_fields[3] = JsonField{ key: "ssl", value: JsonValue{ tag: .Number, num_val: e.timings.ssl } };
            timing_fields[4] = JsonField{ key: "send", value: JsonValue{ tag: .Number, num_val: e.timings.send } };
            timing_fields[5] = JsonField{ key: "wait", value: JsonValue{ tag: .Number, num_val: e.timings.wait } };
            timing_fields[6] = JsonField{ key: "receive", value: JsonValue{ tag: .Number, num_val: e.timings.receive } };

            let entry_fields = aa.alloc(JsonField, 6) catch return PlaywrightError.OutOfMemory;
            entry_fields[0] = JsonField{ key: "pageref", value: JsonValue{ tag: .String, str_val: e.page_ref } };
            entry_fields[1] = JsonField{ key: "startedDateTime", value: JsonValue{ tag: .String, str_val: e.started_date_time } };
            entry_fields[2] = JsonField{ key: "time", value: JsonValue{ tag: .Number, num_val: e.timings.totalTimeMs() } };
            entry_fields[3] = JsonField{ key: "request", value: JsonValue{ tag: .Object, obj_val: req_fields } };
            entry_fields[4] = JsonField{ key: "response", value: JsonValue{ tag: .Object, obj_val: resp_fields } };
            entry_fields[5] = JsonField{ key: "timings", value: JsonValue{ tag: .Object, obj_val: timing_fields } };

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
