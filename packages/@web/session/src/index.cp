// Origin: Official @web/session Plugin (Pure-CP Zero-Heap Tamper-Proof Sessions) | Created: 2026-11-25
// ============================================================================
// @web/session — Zero-Heap Signed Cookie Session Plugin for cap:web
//   - Inspired by Axum `tower-sessions` & Fastify `@fastify/session`
//   - Tamper-proof `<id>.<hex_sig>` session cookies via pure-CP keyed FNV-HMAC
//   - Constant-time MAC verification + fixed-slot KV store with TTL expiration
//   - Zero heap allocations, zero external C dependencies
// ============================================================================

const std = @import("std");
import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub const SESS_MAX_SLOTS: usize = 64;
pub const SESS_MAX_ID_LEN: usize = 48;
pub const SESS_MAX_VAL_LEN: usize = 256;
pub const SESS_SIG_HEX_LEN: usize = 32;

pub struct SessionConfig {
    secret: string = "dev-session-secret";
    cookie_name: string = "cp_sid";
    ttl_secs: u64 = 3600;
    secure: bool = true;
    http_only: bool = true;
    same_site: string = "Lax";
}

pub struct SessionSlot {
    used: bool = false;
    id_buf: [SESS_MAX_ID_LEN]u8 = [0; SESS_MAX_ID_LEN];
    id_len: usize = 0;
    val_buf: [SESS_MAX_VAL_LEN]u8 = [0; SESS_MAX_VAL_LEN];
    val_len: usize = 0;
    created_at: u64 = 0;
    expires_at: u64 = 0;

    fn idSlice(): string {
        return _.id_buf[0.._.id_len];
    }

    fn valSlice(): string {
        return _.val_buf[0.._.val_len];
    }

    fn matchesId(id: string): bool {
        if (!_.used || _.id_len != id.len) return false;
        let i: usize = 0;
        while (i < id.len) : (i += 1) {
            if (_.id_buf[i] != id[i]) return false;
        }
        return true;
    }
}

// ============================================================================
// Internal Prefix-Scoped Helpers (sess*)
// ============================================================================

const SESS_HEX_CHARS: string = "0123456789abcdef";
const SESS_FNV_OFFSET_0: u64 = (@as(u64, 0xcbf29ce4) << 32) | 0x84222325;
const SESS_FNV_OFFSET_1: u64 = (@as(u64, 0x84222325) << 32) | 0xcbf29ce4;
const SESS_FNV_PRIME: u64 = (@as(u64, 0x00000100) << 32) | 0x000001b3;
const SESS_SM_GAMMA: u64 = (@as(u64, 0x9e3779b9) << 32) | 0x7f4a7c15;
const SESS_SM_MUL1: u64 = (@as(u64, 0xbf58476d) << 32) | 0x1ce4e5b9;
const SESS_SM_MUL2: u64 = (@as(u64, 0x94d049bb) << 32) | 0x133111eb;

let g_sess_cfg: SessionConfig = SessionConfig{};
let g_sess_slots: [SESS_MAX_SLOTS]SessionSlot = [SessionSlot{}; SESS_MAX_SLOTS];
let g_sess_seq: u64 = 1;
let g_sess_test_clock: u64 = 0;

fn sessMonotonicNowSec(): u64 {
    const sess_builtin = @import("builtin");
    if (comptime sess_builtin.os.tag == .linux) {
        let ts: std.os.linux.timespec = undefined;
        * = std.os.linux.clock_gettime(.MONOTONIC, &ts);
        return if (ts.sec > 0) @intCast(ts.sec) else 1;
    }
    let ts: std.c.timespec = undefined;
    * = std.c.clock_gettime(.MONOTONIC, &ts);
    return if (ts.sec > 0) @intCast(ts.sec) else 1;
}

fn sessNowSec(): u64 {
    if (g_sess_test_clock > 0) return g_sess_test_clock;
    return sessMonotonicNowSec();
}

fn sessFnv1a64Update(seed: u64, data: []const u8): u64 {
    let h: u64 = seed;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h ^= @as(u64, data[i]);
        h = h *% SESS_FNV_PRIME;
    }
    return h;
}

fn sessSplitMix64(x: u64): u64 {
    let z = x +% SESS_SM_GAMMA;
    z = (z ^ (z >> 30)) *% SESS_SM_MUL1;
    z = (z ^ (z >> 27)) *% SESS_SM_MUL2;
    return z ^ (z >> 31);
}

fn sessWriteHexU64(val: u64, out_buf: []u8, offset: usize) {
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        const shift: u6 = @truncate((15 - i) * 4);
        const nibble: usize = @as(usize, (val >> shift) & 0x0f);
        out_buf[offset + i] = SESS_HEX_CHARS[nibble];
    }
}

/// Pure-CP keyed HMAC-style 128-bit MAC (32 lowercase hex chars) using two-pass ipad/opad FNV-1a + SplitMix64.
fn sessComputeMacHex(secret: string, id: string, out_hex: *[SESS_SIG_HEX_LEN]u8) {
    let ipad: [64]u8 = [0x36; 64];
    let opad: [64]u8 = [0x5c; 64];
    let i: usize = 0;
    while (i < secret.len) : (i += 1) {
        const idx = i % 64;
        ipad[idx] ^= secret[i];
        opad[idx] ^= secret[i];
    }

    // Lane 0
    let inner0 = sessFnv1a64Update(SESS_FNV_OFFSET_0, ipad[0..64]);
    inner0 = sessFnv1a64Update(inner0, id);
    let inner0_bytes: [8]u8 = [0; 8];
    i = 0;
    while (i < 8) : (i += 1) {
        const sh: u6 = @truncate((7 - i) * 8);
        inner0_bytes[i] = @truncate(inner0 >> sh);
    }
    let outer0 = sessFnv1a64Update(SESS_FNV_OFFSET_0, opad[0..64]);
    outer0 = sessFnv1a64Update(outer0, inner0_bytes[0..8]);
    const mac0 = sessSplitMix64(outer0);

    // Lane 1 (domain-separated)
    let inner1 = sessFnv1a64Update(SESS_FNV_OFFSET_1, ipad[0..64]);
    inner1 = sessFnv1a64Update(inner1, "cp-sess-v1:");
    inner1 = sessFnv1a64Update(inner1, id);
    let inner1_bytes: [8]u8 = [0; 8];
    i = 0;
    while (i < 8) : (i += 1) {
        const sh: u6 = @truncate((7 - i) * 8);
        inner1_bytes[i] = @truncate(inner1 >> sh);
    }
    let outer1 = sessFnv1a64Update(SESS_FNV_OFFSET_1, opad[0..64]);
    outer1 = sessFnv1a64Update(outer1, inner1_bytes[0..8]);
    const mac1 = sessSplitMix64(outer1 ^ mac0);

    sessWriteHexU64(mac0, out_hex[0..], 0);
    sessWriteHexU64(mac1, out_hex[0..], 16);
}

fn sessConstTimeEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

fn sessWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (pos.* + s.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn sessWriteU64(out_buf: []u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        if (pos.* >= out_buf.len) return false;
        out_buf[pos.*] = '0';
        pos.* += 1;
        return true;
    }
    while (v > 0) {
        tmp[n] = (48 + @as(u8, @truncate(v % 10))) as u8;
        n += 1;
        v = v / 10;
    }
    if (pos.* + n > out_buf.len) return false;
    while (n > 0) {
        n -= 1;
        out_buf[pos.*] = tmp[n];
        pos.* += 1;
    }
    return true;
}

fn sessVerifySignedCookie(signed_cookie: string): string? {
    let dot_idx: usize? = null;
    let i: usize = 0;
    while (i < signed_cookie.len) : (i += 1) {
        if (signed_cookie[i] == '.') {
            if (dot_idx != null) return null;
            dot_idx = i;
        }
    }
    const split = dot_idx orelse return null;
    if (split == 0 || split > SESS_MAX_ID_LEN) return null;
    const id = signed_cookie[0..split];
    const sig = signed_cookie[split + 1 ..];
    if (sig.len != SESS_SIG_HEX_LEN) return null;

    let expected_sig: [SESS_SIG_HEX_LEN]u8 = [0; SESS_SIG_HEX_LEN];
    sessComputeMacHex(g_sess_cfg.secret, id, *expected_sig);
    if (!sessConstTimeEql(sig, expected_sig[0..SESS_SIG_HEX_LEN])) {
        return null;
    }
    return id;
}

fn sessFindValidSlotIndex(id: string, now_sec: u64): usize? {
    let i: usize = 0;
    while (i < SESS_MAX_SLOTS) : (i += 1) {
        if (g_sess_slots[i].matchesId(id)) {
            if (now_sec >= g_sess_slots[i].expires_at) {
                g_sess_slots[i].used = false;
                return null;
            }
            return i;
        }
    }
    return null;
}

// ============================================================================
// Public @web/session API
// ============================================================================

/// Optional deterministic clock override for unit tests (`0` restores monotonic clock).
pub fn setSessionTestClock(now_sec: u64) {
    g_sess_test_clock = now_sec;
}

/// Clear all in-memory session slots and reset sequence counter.
pub fn resetSessionStore() {
    let i: usize = 0;
    while (i < SESS_MAX_SLOTS) : (i += 1) {
        g_sess_slots[i] = SessionSlot{};
    }
    g_sess_seq = 1;
}

/// Create a new session in the fixed-slot store and write `<id>.<hex_sig>` into `out_cookie_val`.
pub fn sessionCreate(user_val: string, now_sec: u64, out_cookie_val: []u8): usize? {
    if (user_val.len > SESS_MAX_VAL_LEN) return null;
    const ttl: u64 = if (g_sess_cfg.ttl_secs == 0) 1 else g_sess_cfg.ttl_secs;

    // Pick free, expired, or earliest-expiring slot
    let target_idx: usize? = null;
    let oldest_idx: usize = 0;
    let oldest_exp: u64 = std.math.maxInt(u64);
    let i: usize = 0;
    while (i < SESS_MAX_SLOTS) : (i += 1) {
        if (!g_sess_slots[i].used || now_sec >= g_sess_slots[i].expires_at) {
            target_idx = i;
            break;
        }
        if (g_sess_slots[i].expires_at < oldest_exp) {
            oldest_exp = g_sess_slots[i].expires_at;
            oldest_idx = i;
        }
    }
    const idx = target_idx orelse oldest_idx;

    // Generate deterministic-entropy 16-hex-char ID: mix(seq, now_sec, secret)
    const seq = g_sess_seq;
    g_sess_seq += 1;
    let seed = sessFnv1a64Update(SESS_FNV_OFFSET_0, g_sess_cfg.secret);
    seed = sessSplitMix64(seed ^ (seq *% SESS_SM_GAMMA) ^ now_sec);

    let id_hex: [16]u8 = [0; 16];
    sessWriteHexU64(seed, id_hex[0..], 0);

    // Format `<id>.<sig>`
    const total_len: usize = SESS_SIG_HEX_LEN + 17;
    if (out_cookie_val.len < total_len) return null;

    let sig_hex: [SESS_SIG_HEX_LEN]u8 = [0; SESS_SIG_HEX_LEN];
    sessComputeMacHex(g_sess_cfg.secret, id_hex[0..16], *sig_hex);

    // Commit slot
    let slot = SessionSlot{};
    slot.used = true;
    i = 0;
    while (i < 16) : (i += 1) {
        slot.id_buf[i] = id_hex[i];
    }
    slot.id_len = 16;
    i = 0;
    while (i < user_val.len) : (i += 1) {
        slot.val_buf[i] = user_val[i];
    }
    slot.val_len = user_val.len;
    slot.created_at = now_sec;
    slot.expires_at = now_sec + ttl;
    g_sess_slots[idx] = slot;

    i = 0;
    while (i < 16) : (i += 1) {
        out_cookie_val[i] = id_hex[i];
    }
    out_cookie_val[16] = '.';
    i = 0;
    while (i < SESS_SIG_HEX_LEN) : (i += 1) {
        out_cookie_val[17 + i] = sig_hex[i];
    }
    return total_len;
}

/// Verify signature and TTL, returning the stored session value slice if valid.
pub fn sessionGet(signed_cookie: string, now_sec: u64): string? {
    const id = sessVerifySignedCookie(signed_cookie) orelse return null;
    const idx = sessFindValidSlotIndex(id, now_sec) orelse return null;
    return g_sess_slots[idx].valSlice();
}

/// Verify signature and remove the session from the store.
pub fn sessionDestroy(signed_cookie: string): bool {
    const id = sessVerifySignedCookie(signed_cookie) orelse return false;
    let i: usize = 0;
    while (i < SESS_MAX_SLOTS) : (i += 1) {
        if (g_sess_slots[i].matchesId(id)) {
            g_sess_slots[i].used = false;
            return true;
        }
    }
    return false;
}

/// Format an RFC 6265 `Set-Cookie` header value for the signed session cookie.
pub fn formatSessionSetCookie(cfg: SessionConfig, signed_cookie: string, out_buf: []u8): usize? {
    if (cfg.cookie_name.len == 0 || signed_cookie.len == 0) return null;
    let pos: usize = 0;
    if (!sessWriteRaw(out_buf, *pos, cfg.cookie_name)) return null;
    if (!sessWriteRaw(out_buf, *pos, "=")) return null;
    if (!sessWriteRaw(out_buf, *pos, signed_cookie)) return null;
    if (!sessWriteRaw(out_buf, *pos, "; Path=/; Max-Age=")) return null;
    if (!sessWriteU64(out_buf, *pos, cfg.ttl_secs)) return null;
    if (cfg.http_only) {
        if (!sessWriteRaw(out_buf, *pos, "; HttpOnly")) return null;
    }
    if (cfg.secure) {
        if (!sessWriteRaw(out_buf, *pos, "; Secure")) return null;
    }
    if (cfg.same_site.len > 0) {
        if (!sessWriteRaw(out_buf, *pos, "; SameSite=")) return null;
        if (!sessWriteRaw(out_buf, *pos, cfg.same_site)) return null;
    }
    return pos;
}

/// Request lifecycle hook: reads `cfg.cookie_name` cookie, validates MAC & TTL,
/// and injects `session_id` and `session_val` via `ctx.setDerived(...)`.
pub fn sessionHook(ctx: *Context): bool {
    if (raw_cookie := ctx.cookie(g_sess_cfg.cookie_name)) {
        if (id := sessVerifySignedCookie(raw_cookie)) {
            const now_sec = sessNowSec();
            if (idx := sessFindValidSlotIndex(id, now_sec)) {
                * = ctx.setDerived("session_id", g_sess_slots[idx].idSlice());
                * = ctx.setDerived("session_val", g_sess_slots[idx].valSlice());
            }
        }
    }
    return true;
}

/// Create a composable Elysia-style `Plugin` for `@web/session`.
pub fn createSessionPlugin(cfg: SessionConfig): Plugin {
    g_sess_cfg = cfg;
    let p = Plugin.init("@web/session");
    * = p.withScope(PluginScope.Global);
    * = p.onRequest(sessionHook);
    return p;
}

/// Register `@web/session` directly onto `app` (fluent chainable).
pub fn registerSession(app: *App, cfg: SessionConfig): *App {
    const p = createSessionPlugin(cfg);
    return app.usePlugin(&p);
}
