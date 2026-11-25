// Official @web/audit Plugin for `cap:web`
// Enterprise compliance write-operation audit trail:
//   - 32-slot zero-heap ring buffer capturing POST / PUT / PATCH / DELETE operations
//   - Automatic actor resolution (jwt_sub -> auth_user -> actor_header -> anonymous)
//   - Sensitive header value redaction (Authorization, Cookie, X-API-Key)
//   - Built-in GET /_audit JSON status endpoint

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub struct AuditConfig {
    status_path: string = "/_audit";
    actor_header: string = "X-Actor";
    mask_sensitive: bool = true;
}

pub struct AuditRecord {
    seq: u64 = 0;
    method: string = "";
    path: string = "";
    status: u16 = 0;
    actor: string = "";
    client_ip: string = "";
}

const AUD_RING_CAP: usize = 32;
const AUD_MAX_METHOD_LEN: usize = 16;
const AUD_MAX_PATH_LEN: usize = 128;
const AUD_MAX_ACTOR_LEN: usize = 64;
const AUD_MAX_IP_LEN: usize = 64;
const AUD_JSON_BUF_CAP: usize = 8192;

struct AudRingSlot {
    seq: u64 = 0;
    status: u16 = 0;
    method_buf: [AUD_MAX_METHOD_LEN]u8 = [0; AUD_MAX_METHOD_LEN];
    method_len: usize = 0;
    path_buf: [AUD_MAX_PATH_LEN]u8 = [0; AUD_MAX_PATH_LEN];
    path_len: usize = 0;
    actor_buf: [AUD_MAX_ACTOR_LEN]u8 = [0; AUD_MAX_ACTOR_LEN];
    actor_len: usize = 0;
    ip_buf: [AUD_MAX_IP_LEN]u8 = [0; AUD_MAX_IP_LEN];
    ip_len: usize = 0;
    used: bool = false;
}

let g_aud_cfg: AuditConfig = AuditConfig{};
let g_aud_ring: [AUD_RING_CAP]AudRingSlot = [AudRingSlot{}; AUD_RING_CAP];
let g_aud_write_idx: usize = 0;
let g_aud_count: usize = 0;
let g_aud_seq: u64 = 0;
let g_aud_json_buf: [AUD_JSON_BUF_CAP]u8 = [0; AUD_JSON_BUF_CAP];

// ============================================================================
// Helpers
// ============================================================================

fn audAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn audAsciiEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (audAsciiLower(a[i]) != audAsciiLower(b[i])) return false;
    }
    return true;
}

fn audTrimSpaces(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn audCopyInto(dst: []u8, src: string): usize {
    const n = if (src.len > dst.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

fn audIsWriteMethod(m: AppMethod): bool {
    match (m) {
        .POST => return true,
        .PUT => return true,
        .PATCH => return true,
        .DELETE => return true,
        else => return false,
    }
}

fn audResolveActor(ctx: *Context): string {
    if (sub := ctx.derived("jwt_sub")) {
        if (sub.len > 0) return sub;
    }
    if (user := ctx.derived("auth_user")) {
        if (user.len > 0) return user;
    }
    if (g_aud_cfg.actor_header.len > 0) {
        if (hdr := ctx.header(g_aud_cfg.actor_header)) {
            if (hdr.len > 0) return hdr;
        }
    }
    return "anonymous";
}

fn audResolveClientIp(ctx: *Context): string {
    if (xff := ctx.header("X-Forwarded-For")) {
        let end: usize = 0;
        while (end < xff.len && xff[end] != ',') : (end += 1) {}
        const first = audTrimSpaces(xff[0..end]);
        if (first.len > 0) return first;
    }
    if (rip := ctx.header("X-Real-IP")) {
        const trimmed = audTrimSpaces(rip);
        if (trimmed.len > 0) return trimmed;
    }
    return "unknown";
}

fn audWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (pos.* + s.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn audWriteByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn audWriteU64(out_buf: []u8, pos: *usize, val: u64): bool {
    if (val == 0) return audWriteByte(out_buf, pos, '0');
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = @as(u8, @truncate(v % 10)) + '0';
        n += 1;
        v /= 10;
    }
    let i: usize = n;
    while (i > 0) {
        i -= 1;
        if (!audWriteByte(out_buf, pos, tmp[i])) return false;
    }
    return true;
}

fn audWriteJsonStr(out_buf: []u8, pos: *usize, s: string): bool {
    if (!audWriteByte(out_buf, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch == '"') {
            if (!audWriteRaw(out_buf, pos, "\\\"")) return false;
        } else if (ch == '\\') {
            if (!audWriteRaw(out_buf, pos, "\\\\")) return false;
        } else if (ch == '\n') {
            if (!audWriteRaw(out_buf, pos, "\\n")) return false;
        } else if (ch == '\r') {
            if (!audWriteRaw(out_buf, pos, "\\r")) return false;
        } else if (ch == '\t') {
            if (!audWriteRaw(out_buf, pos, "\\t")) return false;
        } else if (ch >= 0x20) {
            if (!audWriteByte(out_buf, pos, ch)) return false;
        }
    }
    return audWriteByte(out_buf, pos, '"');
}

// ============================================================================
// Public Audit API
// ============================================================================

pub fn auditMaskHeaderValue(header_name: string, val: string): string {
    if (!g_aud_cfg.mask_sensitive) return val;
    if (audAsciiEqlIgnoreCase(header_name, "Authorization")
        || audAsciiEqlIgnoreCase(header_name, "Cookie")
        || audAsciiEqlIgnoreCase(header_name, "X-API-Key"))
    {
        return "[REDACTED]";
    }
    return val;
}

pub fn auditCount(): usize {
    return g_aud_count;
}

pub fn auditTotalSeq(): u64 {
    return g_aud_seq;
}

pub fn auditReset() {
    let i: usize = 0;
    while (i < AUD_RING_CAP) : (i += 1) {
        g_aud_ring[i] = AudRingSlot{};
    }
    g_aud_write_idx = 0;
    g_aud_count = 0;
    g_aud_seq = 0;
}

pub fn auditRecordAt(chronological_idx: usize): AuditRecord? {
    if (chronological_idx >= g_aud_count) return null;
    const start_idx = if (g_aud_count < AUD_RING_CAP) 0 else g_aud_write_idx;
    const slot_idx = (start_idx + chronological_idx) % AUD_RING_CAP;
    const s = &g_aud_ring[slot_idx];
    return AuditRecord{
        seq: s.seq,
        method: s.method_buf[0..s.method_len],
        path: s.path_buf[0..s.path_len],
        status: s.status,
        actor: s.actor_buf[0..s.actor_len],
        client_ip: s.ip_buf[0..s.ip_len],
    };
}

pub fn audAfterHandle(ctx: *Context, resp: *Response) {
    if (!audIsWriteMethod(ctx.method)) return;

    g_aud_seq += 1;
    let slot = AudRingSlot{};
    slot.seq = g_aud_seq;
    slot.status = resp.status;
    slot.method_len = audCopyInto(slot.method_buf[0..], ctx.method.asString());
    slot.path_len = audCopyInto(slot.path_buf[0..], ctx.path);
    slot.actor_len = audCopyInto(slot.actor_buf[0..], audResolveActor(ctx));
    slot.ip_len = audCopyInto(slot.ip_buf[0..], audResolveClientIp(ctx));
    slot.used = true;

    g_aud_ring[g_aud_write_idx] = slot;
    g_aud_write_idx = (g_aud_write_idx + 1) % AUD_RING_CAP;
    if (g_aud_count < AUD_RING_CAP) {
        g_aud_count += 1;
    }
}

fn audHandleStatus(ctx: *Context): Response {
    * = ctx;
    let pos: usize = 0;
    * = audWriteRaw(g_aud_json_buf[0..], *pos, "{\"count\":");
    * = audWriteU64(g_aud_json_buf[0..], *pos, @as(u64, g_aud_count));
    * = audWriteRaw(g_aud_json_buf[0..], *pos, ",\"total\":");
    * = audWriteU64(g_aud_json_buf[0..], *pos, g_aud_seq);
    * = audWriteRaw(g_aud_json_buf[0..], *pos, ",\"records\":[");

    const start_idx = if (g_aud_count < AUD_RING_CAP) 0 else g_aud_write_idx;
    let i: usize = 0;
    while (i < g_aud_count) : (i += 1) {
        if (i > 0) {
            * = audWriteByte(g_aud_json_buf[0..], *pos, ',');
        }
        const slot_idx = (start_idx + i) % AUD_RING_CAP;
        const s = &g_aud_ring[slot_idx];
        * = audWriteRaw(g_aud_json_buf[0..], *pos, "{\"seq\":");
        * = audWriteU64(g_aud_json_buf[0..], *pos, s.seq);
        * = audWriteRaw(g_aud_json_buf[0..], *pos, ",\"method\":");
        * = audWriteJsonStr(g_aud_json_buf[0..], *pos, s.method_buf[0..s.method_len]);
        * = audWriteRaw(g_aud_json_buf[0..], *pos, ",\"path\":");
        * = audWriteJsonStr(g_aud_json_buf[0..], *pos, s.path_buf[0..s.path_len]);
        * = audWriteRaw(g_aud_json_buf[0..], *pos, ",\"status\":");
        * = audWriteU64(g_aud_json_buf[0..], *pos, @as(u64, s.status));
        * = audWriteRaw(g_aud_json_buf[0..], *pos, ",\"actor\":");
        * = audWriteJsonStr(g_aud_json_buf[0..], *pos, s.actor_buf[0..s.actor_len]);
        * = audWriteRaw(g_aud_json_buf[0..], *pos, ",\"client_ip\":");
        * = audWriteJsonStr(g_aud_json_buf[0..], *pos, s.ip_buf[0..s.ip_len]);
        * = audWriteByte(g_aud_json_buf[0..], *pos, '}');
    }

    * = audWriteRaw(g_aud_json_buf[0..], *pos, "]}");
    return Response.json(200, g_aud_json_buf[0..pos]);
}

pub fn audCreatePlugin(cfg: AuditConfig): Plugin {
    g_aud_cfg = cfg;
    let p = Plugin.init("@web/audit");
    * = p
        .withScope(PluginScope.Global)
        .onAfterHandle(audAfterHandle);
    if (cfg.status_path.len > 0) {
        * = p.route(AppMethod.GET, cfg.status_path, "Audit Trail", audHandleStatus);
    }
    return p;
}

pub fn registerAudit(app: *App, cfg: AuditConfig): *App {
    const p = audCreatePlugin(cfg);
    return app.usePlugin(&p);
}
