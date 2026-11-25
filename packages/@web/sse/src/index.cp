// Official @web/sse Plugin for `cap:web`
// Server-Sent Events (WHATWG EventSource) with zero-heap event encoding,
// Last-Event-ID ring-buffer replay, heartbeat comments, and AI LLM token streaming helpers.

import {
    App,
    AppMethod,
    Context,
    Response,
    Plugin,
    PluginScope,
} from "../../../../lib/web/app.cp";

pub const SSE_RING_CAP: usize = 32;
pub const SSE_WIRE_BUF_CAP: usize = 8192;

pub struct SseConfig {
    endpoint_path: string = "/events";
    retry_ms: u32 = 3000;
    cors_allow_origin: string = "*";
}

pub struct SseMessage {
    id: u64 = 0;
    event_name: string = "";
    data: string = "";
}

let g_sse_cfg: SseConfig = SseConfig{};
let g_sse_ring: [SSE_RING_CAP]SseMessage = [SseMessage{}; SSE_RING_CAP];
let g_sse_next_id: u64 = 1;
let g_sse_count: usize = 0;
let g_sse_wire_buf: [SSE_WIRE_BUF_CAP]u8 = undefined;

fn sseWriteRaw(dst: []u8, pos: *usize, s: string): bool {
    if (s.len > dst.len || pos.* > dst.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        dst[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn sseWriteU64(dst: []u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = undefined;
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        if (pos.* >= dst.len) return false;
        dst[pos.*] = '0';
        pos.* += 1;
        return true;
    }
    while (v > 0) {
        tmp[n] = (48 + ((v % 10) as u8)) as u8;
        n += 1;
        v = v / 10;
    }
    if (pos.* + n > dst.len) return false;
    while (n > 0) {
        n -= 1;
        dst[pos.*] = tmp[n];
        pos.* += 1;
    }
    return true;
}

fn sseParseU64(s: string): u64 {
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') return 0;
        acc = acc * 10 + ((c - '0') as u64);
    }
    return acc;
}

/// Encode a single WHATWG SSE event frame (`id:` / `event:` / `data:` / `\n\n`).
pub fn encodeSseEvent(id: u64, event_name: string, data: string, out_buf: []u8, pos: *usize): bool {
    if (id > 0) {
        if (!sseWriteRaw(out_buf, pos, "id: ")) return false;
        if (!sseWriteU64(out_buf, pos, id)) return false;
        if (!sseWriteRaw(out_buf, pos, "\n")) return false;
    }
    if (event_name.len > 0) {
        if (!sseWriteRaw(out_buf, pos, "event: ")) return false;
        if (!sseWriteRaw(out_buf, pos, event_name)) return false;
        if (!sseWriteRaw(out_buf, pos, "\n")) return false;
    }
    if (!sseWriteRaw(out_buf, pos, "data: ")) return false;
    if (!sseWriteRaw(out_buf, pos, data)) return false;
    return sseWriteRaw(out_buf, pos, "\n\n");
}

/// Encode an SSE keep-alive heartbeat comment (`: ping\n\n`).
pub fn encodeSseHeartbeat(out_buf: []u8, pos: *usize): bool {
    return sseWriteRaw(out_buf, pos, ": ping\n\n");
}

/// Reset the SSE replay ring buffer (for tests or topic reset).
pub fn sseResetBroker() {
    g_sse_next_id = 1;
    g_sse_count = 0;
}

/// Publish a new SSE event into the ring buffer and return its monotonic event ID.
pub fn ssePublish(event_name: string, data: string): u64 {
    const id = g_sse_next_id;
    g_sse_next_id += 1;
    const slot = ((id - 1) as usize) % SSE_RING_CAP;
    g_sse_ring[slot] = SseMessage{
        id: id,
        event_name: event_name,
        data: data,
    };
    if (g_sse_count < SSE_RING_CAP) {
        g_sse_count += 1;
    }
    return id;
}

/// Build an HTTP `text/event-stream` response replaying all events with `id > last_event_id`.
pub fn buildSseReplayResponse(last_event_id: u64): Response {
    let pos: usize = 0;
    if (g_sse_cfg.retry_ms > 0) {
        * = sseWriteRaw(g_sse_wire_buf[0..], *pos, "retry: ");
        * = sseWriteU64(g_sse_wire_buf[0..], *pos, g_sse_cfg.retry_ms as u64);
        * = sseWriteRaw(g_sse_wire_buf[0..], *pos, "\n\n");
    }

    const total = g_sse_next_id - 1;
    const start_id: u64 = if (total > (SSE_RING_CAP as u64)) (total - (SSE_RING_CAP as u64) + 1) else 1;
    let cur = start_id;
    let emitted: usize = 0;
    while (cur <= total) : (cur += 1) {
        if (cur > last_event_id) {
            const slot = ((cur - 1) as usize) % SSE_RING_CAP;
            const m = g_sse_ring[slot];
            if (!encodeSseEvent(m.id, m.event_name, m.data, g_sse_wire_buf[0..], *pos)) break;
            emitted += 1;
        }
    }
    if (emitted == 0) {
        * = encodeSseHeartbeat(g_sse_wire_buf[0..], *pos);
    }

    let r = Response{
        status: 200,
        content_type: "text/event-stream; charset=utf-8",
        body: g_sse_wire_buf[0..pos],
        body_buf: undefined,
        body_buf_len: 0,
        headers: undefined,
        header_len: 0,
    };
    * = r.setHeader("Cache-Control", "no-cache");
    * = r.setHeader("X-Accel-Buffering", "no");
    return r;
}

fn handleSseEndpoint(ctx: *Context): Response {
    const last_raw = ctx.header("Last-Event-ID") ?? "0";
    const last_id = sseParseU64(last_raw);
    return buildSseReplayResponse(last_id);
}

pub fn createSsePlugin(cfg: SseConfig): Plugin {
    g_sse_cfg = cfg;
    let p = Plugin.init("@web/sse");
    * = p
        .withScope(PluginScope.Global)
        .route(AppMethod.GET, cfg.endpoint_path, "Server-Sent Events Stream", handleSseEndpoint);
    return p;
}

pub fn registerSse(app: *App, cfg: SseConfig): *App {
    const p = createSsePlugin(cfg);
    return app.usePlugin(&p);
}
