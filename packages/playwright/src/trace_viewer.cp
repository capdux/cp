// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/trace/viewer/traceViewer.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/trace_viewer.cp — Playwright Native Trace Viewer
// Loads & indexes PKZIP trace archives via cap:pack (ZipReader), serves
// `/`, `/contexts`, `/trace/events`, `/trace/file?path=...`, and `/health`,
// and optionally binds a native SO_REUSEPORT epoll server via cap:web.
// ============================================================================

const std = @import("std");
import { JSON, JsonValue, JsonField } from "cap:json";
import { ZipReader } from "cap:pack";
import { readFileSync, exists } from "cap:fs";
import { App, Context, Response } from "cap:web/app";
import { WebServer, ServerConfig } from "cap:web/server";
import { PlaywrightError } from "./network";

pub struct TraceViewerOptions {
    pub host: []const u8 = "127.0.0.1",
    pub port: u16 = 9323,
    pub workers: usize = 1,
    pub headless: bool = true,
    pub trace_path: []const u8 = "",
}

pub struct TraceEntryRecord {
    pub name: []const u8 = "",
    pub data: []const u8 = "",
    pub mime_type: []const u8 = "application/octet-stream",
    pub size: usize = 0,
}

pub struct TraceHttpResponse {
    allocator: std.mem.Allocator = undefined,
    pub status: u16 = 200,
    pub content_type: []const u8 = "text/plain; charset=utf-8",
    pub body: []u8 = &.{},

    pub fn deinit(*_) {
        if (_.body.len > 0) {
            _.allocator.free(_.body);
            _.body = &.{};
        }
    }

    ~init() {
        _.deinit();
    }
}

const TRACE_VIEWER_HTML =
    \\<!DOCTYPE html>
    \\<html lang="en">
    \\<head>
    \\<meta charset="utf-8"/>
    \\<meta name="viewport" content="width=device-width,initial-scale=1"/>
    \\<title>Playwright Trace Viewer (CP)</title>
    \\<style>
    \\body{margin:0;font-family:system-ui,-apple-system,sans-serif;background:#111827;color:#f3f4f6}
    \\header{display:flex;justify-content:space-between;align-items:center;padding:12px 20px;background:#1f2937;border-bottom:1px solid #374151}
    \\main{display:grid;grid-template-columns:360px 1fr;height:calc(100vh - 53px)}
    \\#actions{overflow:auto;border-right:1px solid #374151;padding:12px}
    \\#details{overflow:auto;padding:16px}
    \\.step{padding:8px 10px;border-radius:6px;margin-bottom:6px;background:#1f2937;cursor:pointer;border:1px solid transparent}
    \\.step:hover,.step.active{border-color:#60a5fa;background:#263244}
    \\.meta{font-size:12px;color:#9ca3af}
    \\pre{background:#0b1120;padding:12px;border-radius:6px;overflow:auto;font-size:12px}
    \\</style>
    \\</head>
    \\<body>
    \\<header><strong>Playwright Trace Viewer (CP)</strong><span id="summary" class="meta">Loading trace...</span></header>
    \\<main>
    \\  <section id="actions"></section>
    \\  <section id="details"><div class="meta">Select an action or resource to inspect.</div></section>
    \\</main>
    \\<script>
    \\async function boot(){
    \\  const ctx = await fetch('/contexts').then(r => r.json());
    \\  document.getElementById('summary').textContent =
    \\    `Events: ${ctx.eventCount} | Entries: ${ctx.entryCount} | Snapshots: ${ctx.snapshotCount}`;
    \\  const raw = await fetch('/trace/file?path=trace.trace').then(r => r.ok ? r.text() : '');
    \\  const lines = raw.split('\n').filter(Boolean).map(l => { try { return JSON.parse(l); } catch { return { raw: l }; } });
    \\  const list = document.getElementById('actions');
    \\  const det = document.getElementById('details');
    \\  lines.forEach((ev, idx) => {
    \\    const el = document.createElement('div');
    \\    el.className = 'step';
    \\    el.innerHTML = `<div><strong>${ev.method || ev.type || 'event'}</strong></div><div class="meta">#${idx+1} • ${ev.time ?? 0}ms • ${ev.pageId || ''}</div>`;
    \\    el.onclick = () => {
    \\      document.querySelectorAll('.step').forEach(s => s.classList.remove('active'));
    \\      el.classList.add('active');
    \\      det.innerHTML = `<h3>${ev.method || ev.type || 'Event'}</h3><pre>${JSON.stringify(ev, null, 2)}</pre>`;
    \\    };
    \\    list.appendChild(el);
    \\  });
    \\  if (!lines.length) {
    \\    det.innerHTML = `<h3>Archive Contexts</h3><pre>${JSON.stringify(ctx, null, 2)}</pre>`;
    \\  }
    \\}
    \\boot();
    \\</script>
    \\</body>
    \\</html>
;

fn inferMimeType(path: []const u8): []const u8 {
    if (std.mem.endsWith(u8, path, ".html") || std.mem.endsWith(u8, path, ".htm")) return "text/html; charset=utf-8";
    if (std.mem.endsWith(u8, path, ".json") || std.mem.endsWith(u8, path, ".meta.json")) return "application/json; charset=utf-8";
    if (std.mem.endsWith(u8, path, ".trace") || std.mem.endsWith(u8, path, ".network")) return "application/x-ndjson; charset=utf-8";
    if (std.mem.endsWith(u8, path, ".js") || std.mem.endsWith(u8, path, ".mjs")) return "application/javascript; charset=utf-8";
    if (std.mem.endsWith(u8, path, ".css")) return "text/css; charset=utf-8";
    if (std.mem.endsWith(u8, path, ".png")) return "image/png";
    if (std.mem.endsWith(u8, path, ".jpg") || std.mem.endsWith(u8, path, ".jpeg")) return "image/jpeg";
    if (std.mem.endsWith(u8, path, ".webp")) return "image/webp";
    if (std.mem.endsWith(u8, path, ".svg")) return "image/svg+xml";
    if (std.mem.endsWith(u8, path, ".txt") || std.mem.endsWith(u8, path, ".log")) return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

fn countNonEmptyLines(buf: []const u8): usize {
    if (buf.len == 0) return 0;
    let count: usize = 0;
    let line_has_char = false;
    for (b of buf) {
        if (b == '\n') {
            if (line_has_char) count += 1;
            line_has_char = false;
        } else if (b != '\r' && b != ' ' && b != '\t') {
            line_has_char = true;
        }
    }
    if (line_has_char) count += 1;
    return count;
}

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn decodeQueryValue(alloc: std.mem.Allocator, raw: []const u8): ([]u8)!PlaywrightError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);
    let i: usize = 0;
    while (i < raw.len) : (i += 1) {
        if (raw[i] == '%' && i + 2 < raw.len) {
            if (hi := hexVal(raw[i + 1])) {
                if (lo := hexVal(raw[i + 2])) {
                    out.append(alloc, (hi << 4) | lo) catch return PlaywrightError.OutOfMemory;
                    i += 2;
                    continue;
                }
            }
        } else if (raw[i] == '+') {
            out.append(alloc, ' ') catch return PlaywrightError.OutOfMemory;
            continue;
        }
        out.append(alloc, raw[i]) catch return PlaywrightError.OutOfMemory;
    }
    return out.toOwnedSlice(alloc) catch return PlaywrightError.OutOfMemory;
}

fn extractQueryParam(query: []const u8, key: []const u8): ([]const u8)? {
    if (query.len == 0) return null;
    let start: usize = 0;
    while (start <= query.len) {
        const amp = std.mem.indexOfScalarPos(u8, query, start, '&') orelse query.len;
        const pair = query[start..amp];
        if (eq := std.mem.indexOfScalar(u8, pair, '=')) {
            if (std.mem.eql(u8, pair[0..eq], key)) {
                return pair[eq + 1..];
            }
        }
        if (amp == query.len) break;
        start = amp + 1;
    }
    return null;
}

fn isSafeTraceEntryPath(p: []const u8): bool {
    if (p.len == 0) return false;
    if (p[0] == '/') return false;
    if (std.mem.indexOf(u8, p, "..") != null) return false;
    return true;
}

fn webHandleRoot(ctx: *Context): Response {
    return webDispatch(ctx, "GET", "/");
}

fn webHandleContexts(ctx: *Context): Response {
    return webDispatch(ctx, "GET", "/contexts");
}

fn webHandleEvents(ctx: *Context): Response {
    return webDispatch(ctx, "GET", "/trace/events");
}

fn webHandleFile(ctx: *Context): Response {
    const path_q = ctx.query("path") ?? "";
    let buf: [1024]u8 = undefined;
    const target = std.fmt.bufPrint(&buf, "/trace/file?path={s}", .{path_q}) catch "/trace/file";
    return webDispatch(ctx, "GET", target);
}

fn webHandleHealth(ctx: *Context): Response {
    return webDispatch(ctx, "GET", "/health");
}

fn webDispatch(ctx: *Context, method: []const u8, target: []const u8): Response {
    if (ctx.state_ptr == 0) {
        return Response.text(500, "TraceViewerServer not initialized");
    }
    const srv = unsafe { ctx.state_ptr as *TraceViewerServer! };
    let arena = std.heap.ArenaAllocator.init(std.heap.c_allocator);
    defer arena.deinit();
    const aa = arena.allocator();
    let resp = srv.handleHttpRequest(aa, method, target) catch {
        return Response.text(500, "Internal TraceViewer error");
    };
    if (copied := ctx.arenaAlloc(resp.body.len)) {
        std.mem.copyForwards(u8, copied, resp.body);
        return Response.bytes(resp.status, copied, resp.content_type);
    }
    return Response.text(500, "Arena allocation failed");
}

pub struct TraceViewerServer {
    allocator: std.mem.Allocator = undefined,
    pub options: TraceViewerOptions = .{},
    zip_bytes: []u8 = &.{},
    entries: std.ArrayList(TraceEntryRecord) = .empty,
    trace_ndjson: []const u8 = "",
    network_ndjson: []const u8 = "",
    meta_json: []const u8 = "",
    pub event_count: usize = 0,
    pub snapshot_count: usize = 0,
    pub resource_count: usize = 0,
    pub listening: bool = false,
    web_server: *WebServer? = null,
    app: *App? = null,

    pub static fn init(alloc: std.mem.Allocator, opts: TraceViewerOptions): TraceViewerServer {
        return TraceViewerServer{
            allocator: alloc,
            options: opts,
            zip_bytes: &.{},
            entries: .empty,
            trace_ndjson: "",
            network_ndjson: "",
            meta_json: "",
            event_count: 0,
            snapshot_count: 0,
            resource_count: 0,
            listening: false,
            web_server: null,
            app: null,
        };
    }

    pub fn clearArchive(*_) {
        _.entries.deinit(_.allocator);
        _.entries = .empty;
        if (_.zip_bytes.len > 0) {
            _.allocator.free(_.zip_bytes);
            _.zip_bytes = &.{};
        }
        _.trace_ndjson = "";
        _.network_ndjson = "";
        _.meta_json = "";
        _.event_count = 0;
        _.snapshot_count = 0;
        _.resource_count = 0;
    }

    pub fn deinit(*_) {
        _.stop();
        _.clearArchive();
    }

    ~init() {
        _.deinit();
    }

    pub fn loadZipBytes(*_, raw_zip: []const u8): void!PlaywrightError {
        if (raw_zip.len < 22) return PlaywrightError.ProtocolError;
        _.clearArchive();

        _.zip_bytes = _.allocator.dupe(u8, raw_zip) catch return PlaywrightError.OutOfMemory;
        errdefer _.clearArchive();

        let zr = ZipReader.init(_.zip_bytes);
        while (entry := zr.next()) {
            const mime = inferMimeType(entry.name);
            _.entries.append(_.allocator, TraceEntryRecord{
                name: entry.name,
                data: entry.data,
                mime_type: mime,
                size: entry.data.len,
            }) catch return PlaywrightError.OutOfMemory;

            if (std.mem.eql(u8, entry.name, "trace.trace")) {
                _.trace_ndjson = entry.data;
                _.event_count = countNonEmptyLines(entry.data);
            } else if (std.mem.eql(u8, entry.name, "trace.network")) {
                _.network_ndjson = entry.data;
            } else if (std.mem.eql(u8, entry.name, "trace.meta.json")) {
                _.meta_json = entry.data;
            } else if (std.mem.startsWith(u8, entry.name, "snapshots/") || std.mem.startsWith(u8, entry.name, "screenshots/")) {
                _.snapshot_count += 1;
            } else if (std.mem.startsWith(u8, entry.name, "resources/")) {
                _.resource_count += 1;
            }
        }
    }

    pub fn loadTraceFile(*_, path: []const u8): void!PlaywrightError {
        if (path.len == 0 || !exists(path)) return PlaywrightError.InvalidUrl;
        const raw = readFileSync(path);
        if (raw.len < 22) return PlaywrightError.ProtocolError;
        _.loadZipBytes(raw)?;
    }

    pub fn entryCount(): usize {
        return _.entries.items.len;
    }

    pub fn findEntry(path: []const u8): TraceEntryRecord? {
        for (e of _.entries.items) {
            if (std.mem.eql(u8, e.name, path)) {
                return e;
            }
        }
        return null;
    }

    fn buildContextsJson(*_, alloc: std.mem.Allocator): ([]u8)!PlaywrightError {
        let arena = std.heap.ArenaAllocator.init(alloc);
        defer arena.deinit();
        const aa = arena.allocator();

        let entry_vals = aa.alloc(JsonValue, _.entries.items.len) catch return PlaywrightError.OutOfMemory;
        let idx: usize = 0;
        for (e of _.entries.items) {
            let ef = aa.alloc(JsonField, 3) catch return PlaywrightError.OutOfMemory;
            ef[0] = JsonField{ key: "name", value: JsonValue{ tag: .String, str_val: e.name } };
            ef[1] = JsonField{ key: "size", value: JsonValue{ tag: .Number, num_val: @floatFromInt(e.size) } };
            ef[2] = JsonField{ key: "mimeType", value: JsonValue{ tag: .String, str_val: e.mime_type } };
            entry_vals[idx] = JsonValue{ tag: .Object, obj_val: ef };
            idx += 1;
        }

        let root = aa.alloc(JsonField, 6) catch return PlaywrightError.OutOfMemory;
        root[0] = JsonField{ key: "version", value: JsonValue{ tag: .Number, num_val: 6.0 } };
        root[1] = JsonField{ key: "eventCount", value: JsonValue{ tag: .Number, num_val: @floatFromInt(_.event_count) } };
        root[2] = JsonField{ key: "snapshotCount", value: JsonValue{ tag: .Number, num_val: @floatFromInt(_.snapshot_count) } };
        root[3] = JsonField{ key: "resourceCount", value: JsonValue{ tag: .Number, num_val: @floatFromInt(_.resource_count) } };
        root[4] = JsonField{ key: "entryCount", value: JsonValue{ tag: .Number, num_val: @floatFromInt(_.entries.items.len) } };
        root[5] = JsonField{ key: "entries", value: JsonValue{ tag: .Array, arr_val: entry_vals } };

        return JSON.stringifyValue(alloc, JsonValue{ tag: .Object, obj_val: root }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn handleHttpRequest(
        *_,
        alloc: std.mem.Allocator,
        method: []const u8,
        path_and_query: []const u8,
    ): TraceHttpResponse!PlaywrightError {
        if (!std.mem.eql(u8, method, "GET") && !std.mem.eql(u8, method, "HEAD")) {
            const msg = alloc.dupe(u8, "Method Not Allowed") catch return PlaywrightError.OutOfMemory;
            return TraceHttpResponse{
                allocator: alloc,
                status: 405,
                content_type: "text/plain; charset=utf-8",
                body: msg,
            };
        }

        const q_idx = std.mem.indexOfScalar(u8, path_and_query, '?');
        const route_path = if (qi := q_idx) path_and_query[0..qi] else path_and_query;
        const query_str = if (qi := q_idx) path_and_query[qi + 1..] else "";

        if (std.mem.eql(u8, route_path, "/") ||
            std.mem.eql(u8, route_path, "/index.html") ||
            std.mem.eql(u8, route_path, "/trace/index.html"))
        {
            const html = alloc.dupe(u8, TRACE_VIEWER_HTML) catch return PlaywrightError.OutOfMemory;
            return TraceHttpResponse{
                allocator: alloc,
                status: 200,
                content_type: "text/html; charset=utf-8",
                body: html,
            };
        }

        if (std.mem.eql(u8, route_path, "/contexts") || std.mem.eql(u8, route_path, "/trace/contexts")) {
            const json_body = _.buildContextsJson(alloc)?;
            return TraceHttpResponse{
                allocator: alloc,
                status: 200,
                content_type: "application/json; charset=utf-8",
                body: json_body,
            };
        }

        if (std.mem.eql(u8, route_path, "/trace/events")) {
            const ev_body = alloc.dupe(u8, _.trace_ndjson) catch return PlaywrightError.OutOfMemory;
            return TraceHttpResponse{
                allocator: alloc,
                status: 200,
                content_type: "application/x-ndjson; charset=utf-8",
                body: ev_body,
            };
        }

        if (std.mem.eql(u8, route_path, "/trace/file")) {
            const raw_param = extractQueryParam(query_str, "path") orelse {
                const nf = alloc.dupe(u8, "Missing path query parameter") catch return PlaywrightError.OutOfMemory;
                return TraceHttpResponse{
                    allocator: alloc,
                    status: 400,
                    content_type: "text/plain; charset=utf-8",
                    body: nf,
                };
            };
            const decoded = decodeQueryValue(alloc, raw_param)?;
            defer alloc.free(decoded);

            if (!isSafeTraceEntryPath(decoded)) {
                const forb = alloc.dupe(u8, "Forbidden path") catch return PlaywrightError.OutOfMemory;
                return TraceHttpResponse{
                    allocator: alloc,
                    status: 403,
                    content_type: "text/plain; charset=utf-8",
                    body: forb,
                };
            }

            if (entry := _.findEntry(decoded)) {
                const payload = alloc.dupe(u8, entry.data) catch return PlaywrightError.OutOfMemory;
                return TraceHttpResponse{
                    allocator: alloc,
                    status: 200,
                    content_type: entry.mime_type,
                    body: payload,
                };
            }

            const nf = alloc.dupe(u8, "Trace entry not found") catch return PlaywrightError.OutOfMemory;
            return TraceHttpResponse{
                allocator: alloc,
                status: 404,
                content_type: "text/plain; charset=utf-8",
                body: nf,
            };
        }

        if (std.mem.eql(u8, route_path, "/health")) {
            const ok = alloc.dupe(u8, "{\"status\":\"ok\"}") catch return PlaywrightError.OutOfMemory;
            return TraceHttpResponse{
                allocator: alloc,
                status: 200,
                content_type: "application/json; charset=utf-8",
                body: ok,
            };
        }

        const nf = alloc.dupe(u8, "Not Found") catch return PlaywrightError.OutOfMemory;
        return TraceHttpResponse{
            allocator: alloc,
            status: 404,
            content_type: "text/plain; charset=utf-8",
            body: nf,
        };
    }

    pub fn startServer(*_): void!PlaywrightError {
        if (_.listening) return;
        let app_ptr = _.allocator.create(App) catch return PlaywrightError.OutOfMemory;
        app_ptr.* = App.init();
        errdefer _.allocator.destroy(app_ptr);

        * = app_ptr
            .state(unsafe { @intFromPtr(_) })
            .get("/", webHandleRoot)
            .get("/contexts", webHandleContexts)
            .get("/trace/events", webHandleEvents)
            .get("/trace/file", webHandleFile)
            .get("/health", webHandleHealth);

        let ws_ptr = _.allocator.create(WebServer) catch return PlaywrightError.OutOfMemory;
        ws_ptr.* = WebServer.init(ServerConfig{
            port: _.options.port,
            workers: if (_.options.workers == 0) 1 else _.options.workers,
            backlog: 128,
            pin_cpu: false,
            shutdown_grace_ms: 50,
        });
        errdefer _.allocator.destroy(ws_ptr);

        const ok = ws_ptr.start(app_ptr);
        if (!ok) return PlaywrightError.ProtocolError;
        _.app = app_ptr;
        _.web_server = ws_ptr;
        _.listening = true;
    }

    pub fn url(out_buf: []u8): ([]const u8)!PlaywrightError {
        return std.fmt.bufPrint(
            out_buf,
            "http://{s}:{d}",
            .{ _.options.host, _.options.port },
        ) catch return PlaywrightError.BufferTooSmall;
    }

    pub fn stop(*_) {
        if (_.listening) {
            if (ws := _.web_server) {
                ws.stop();
            }
            _.listening = false;
        }
        if (ws := _.web_server) {
            _.allocator.destroy(ws);
            _.web_server = null;
        }
        if (a := _.app) {
            _.allocator.destroy(a);
            _.app = null;
        }
    }
}
