// Official @web/webhook Plugin for `cap:web`
// Stripe / GitHub / Standard Webhooks signature verification & replay protection:
//   - Built-in pure-CP FIPS 180-4 SHA-256 + RFC 2104 HMAC-SHA256
//   - Constant-time hex signature comparison ("sha256=<hex>" or bare "<hex>")
//   - Timestamp drift window validation + 64-slot event ID idempotency table
//   - Zero heap allocations, zero external C dependencies

const std = @import("std");
import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub struct WebhookConfig {
    secret: string = "";
    endpoint_path: string = "/webhooks";
    sig_header: string = "X-Hub-Signature-256";
    ts_header: string = "X-Webhook-Timestamp";
    id_header: string = "X-Webhook-Id";
    max_skew_secs: u64 = 300;
}

const WH_SHA256_K: [64]u32 = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
];

const WH_HEX_LOWER: string = "0123456789abcdef";
const WH_MAX_SEEN_IDS: usize = 64;
const WH_MAX_ID_LEN: usize = 128;

struct WhEventSlot {
    id_buf: [WH_MAX_ID_LEN]u8 = [0; WH_MAX_ID_LEN];
    id_len: usize = 0;
    used: bool = false;
}

let g_wh_cfg: WebhookConfig = WebhookConfig{};
let g_wh_test_clock: u64 = 0;
let g_wh_seen: [WH_MAX_SEEN_IDS]WhEventSlot = [WhEventSlot{}; WH_MAX_SEEN_IDS];
let g_wh_seen_cursor: usize = 0;
let g_wh_seen_count: usize = 0;

// ============================================================================
// Pure-CP FIPS 180-4 SHA-256 & RFC 2104 HMAC-SHA256
// ============================================================================

fn whRotr32(x: u32, n: u5): u32 {
    const inv: u5 = @truncate(32 - @as(u6, n));
    return (x >> n) | (x << inv);
}

fn whSha256Compress(state: *[8]u32, block: *const [64]u8) {
    let w: [64]u32 = [0; 64];
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        const j = i * 4;
        w[i] = (@as(u32, block[j]) << 24)
            | (@as(u32, block[j + 1]) << 16)
            | (@as(u32, block[j + 2]) << 8)
            | @as(u32, block[j + 3]);
    }
    while (i < 64) : (i += 1) {
        const s0 = whRotr32(w[i - 15], 7) ^ whRotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const s1 = whRotr32(w[i - 2], 17) ^ whRotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] +% s0 +% w[i - 7] +% s1;
    }

    let a = state[0];
    let b = state[1];
    let c = state[2];
    let d = state[3];
    let e = state[4];
    let f = state[5];
    let g = state[6];
    let h = state[7];

    i = 0;
    while (i < 64) : (i += 1) {
        const S1 = whRotr32(e, 6) ^ whRotr32(e, 11) ^ whRotr32(e, 25);
        const ch = (e & f) ^ ((~e) & g);
        const temp1 = h +% S1 +% ch +% WH_SHA256_K[i] +% w[i];
        const S0 = whRotr32(a, 2) ^ whRotr32(a, 13) ^ whRotr32(a, 22);
        const maj = (a & b) ^ (a & c) ^ (b & c);
        const temp2 = S0 +% maj;

        h = g;
        g = f;
        f = e;
        e = d +% temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 +% temp2;
    }

    state[0] = state[0] +% a;
    state[1] = state[1] +% b;
    state[2] = state[2] +% c;
    state[3] = state[3] +% d;
    state[4] = state[4] +% e;
    state[5] = state[5] +% f;
    state[6] = state[6] +% g;
    state[7] = state[7] +% h;
}

struct WhSha256Hasher {
    state: [8]u32 = [
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    ];
    buf: [64]u8 = [0; 64];
    buf_len: usize = 0;
    total_len: u64 = 0;

    static fn init(): WhSha256Hasher {
        return WhSha256Hasher{};
    }

    fn update(*_, data: []const u8) {
        let i: usize = 0;
        while (i < data.len) : (i += 1) {
            _.buf[_.buf_len] = data[i];
            _.buf_len += 1;
            _.total_len += 1;
            if (_.buf_len == 64) {
                whSha256Compress(*_.state, &_.buf);
                _.buf_len = 0;
            }
        }
    }

    fn finalize(*_, out: *[32]u8) {
        const bit_len: u64 = _.total_len * 8;
        _.buf[_.buf_len] = 0x80;
        _.buf_len += 1;
        if (_.buf_len > 56) {
            while (_.buf_len < 64) : (_.buf_len += 1) {
                _.buf[_.buf_len] = 0;
            }
            whSha256Compress(*_.state, &_.buf);
            _.buf_len = 0;
        }
        while (_.buf_len < 56) : (_.buf_len += 1) {
            _.buf[_.buf_len] = 0;
        }
        _.buf[56] = @truncate(bit_len >> 56);
        _.buf[57] = @truncate(bit_len >> 48);
        _.buf[58] = @truncate(bit_len >> 40);
        _.buf[59] = @truncate(bit_len >> 32);
        _.buf[60] = @truncate(bit_len >> 24);
        _.buf[61] = @truncate(bit_len >> 16);
        _.buf[62] = @truncate(bit_len >> 8);
        _.buf[63] = @truncate(bit_len);
        whSha256Compress(*_.state, &_.buf);

        let i: usize = 0;
        while (i < 8) : (i += 1) {
            const v = _.state[i];
            out[i * 4] = @truncate(v >> 24);
            out[i * 4 + 1] = @truncate(v >> 16);
            out[i * 4 + 2] = @truncate(v >> 8);
            out[i * 4 + 3] = @truncate(v);
        }
    }
}

fn whSha256(data: []const u8, out: *[32]u8) {
    let h = WhSha256Hasher.init();
    h.update(data);
    h.finalize(out);
}

fn whHmacSha256(key: string, msg: []const u8, out: *[32]u8) {
    let k_block: [64]u8 = [0; 64];
    if (key.len > 64) {
        let hashed_key: [32]u8 = [0; 32];
        whSha256(key, *hashed_key);
        let i: usize = 0;
        while (i < 32) : (i += 1) {
            k_block[i] = hashed_key[i];
        }
    } else {
        let i: usize = 0;
        while (i < key.len) : (i += 1) {
            k_block[i] = key[i];
        }
    }

    let ipad: [64]u8 = [0; 64];
    let opad: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        ipad[i] = k_block[i] ^ 0x36;
        opad[i] = k_block[i] ^ 0x5c;
    }

    let inner_digest: [32]u8 = [0; 32];
    let inner = WhSha256Hasher.init();
    inner.update(ipad[0..64]);
    inner.update(msg);
    inner.finalize(*inner_digest);

    let outer = WhSha256Hasher.init();
    outer.update(opad[0..64]);
    outer.update(inner_digest[0..32]);
    outer.finalize(out);
}

// ============================================================================
// Helpers: Constant-Time Comparison, String, Clock & Replay Store
// ============================================================================

fn whAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn whStrEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn whAsciiEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (whAsciiLower(a[i]) != whAsciiLower(b[i])) return false;
    }
    return true;
}

fn whConstTimeHexEql(expected_lower: []const u8, candidate: string): bool {
    if (expected_lower.len != candidate.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < expected_lower.len) : (i += 1) {
        diff |= expected_lower[i] ^ whAsciiLower(candidate[i]);
    }
    return diff == 0;
}

fn whParseU64(s: string): u64? {
    if (s.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch < '0' || ch > '9') return null;
        acc = acc * 10 + @as(u64, ch - '0');
    }
    return acc;
}

fn whNowSec(): u64 {
    if (g_wh_test_clock > 0) return g_wh_test_clock;
    const wh_builtin = @import("builtin");
    if (comptime wh_builtin.os.tag == .linux) {
        let ts: std.os.linux.timespec = undefined;
        * = std.os.linux.clock_gettime(.REALTIME, &ts);
        return if (ts.sec > 0) @intCast(ts.sec) else 0;
    }
    let ts: std.c.timespec = undefined;
    * = std.c.clock_gettime(.REALTIME, &ts);
    return if (ts.sec > 0) @intCast(ts.sec) else 0;
}

fn whHasSeenEvent(evt_id: string): bool {
    let i: usize = 0;
    while (i < WH_MAX_SEEN_IDS) : (i += 1) {
        if (g_wh_seen[i].used && g_wh_seen[i].id_len == evt_id.len) {
            if (whStrEql(g_wh_seen[i].id_buf[0..g_wh_seen[i].id_len], evt_id)) {
                return true;
            }
        }
    }
    return false;
}

fn whRecordEvent(evt_id: string) {
    const copy_len = if (evt_id.len > WH_MAX_ID_LEN) WH_MAX_ID_LEN else evt_id.len;
    let slot = WhEventSlot{};
    let i: usize = 0;
    while (i < copy_len) : (i += 1) {
        slot.id_buf[i] = evt_id[i];
    }
    slot.id_len = copy_len;
    slot.used = true;
    g_wh_seen[g_wh_seen_cursor] = slot;
    g_wh_seen_cursor = (g_wh_seen_cursor + 1) % WH_MAX_SEEN_IDS;
    if (g_wh_seen_count < WH_MAX_SEEN_IDS) {
        g_wh_seen_count += 1;
    }
}

// ============================================================================
// Public Webhook API
// ============================================================================

pub fn webhookSignHex(secret: string, payload: string, out_hex: []u8): usize? {
    if (out_hex.len < 64) return null;
    let digest: [32]u8 = [0; 32];
    whHmacSha256(secret, payload, *digest);
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        const byte_val = digest[i];
        out_hex[i * 2] = WH_HEX_LOWER[@as(usize, (byte_val >> 4) & 0x0F)];
        out_hex[i * 2 + 1] = WH_HEX_LOWER[@as(usize, byte_val & 0x0F)];
    }
    return 64;
}

pub fn webhookVerifySignature(secret: string, payload: string, header_sig: string): bool {
    let sig_hex = header_sig;
    if (header_sig.len >= 7 && whAsciiEqlIgnoreCase(header_sig[0..7], "sha256=")) {
        sig_hex = header_sig[7..];
    }
    if (sig_hex.len != 64) return false;
    let expected: [64]u8 = [0; 64];
    const n = webhookSignHex(secret, payload, expected[0..]) orelse return false;
    if (n != 64) return false;
    return whConstTimeHexEql(expected[0..64], sig_hex);
}

pub fn setWebhookTestClock(now_sec: u64) {
    g_wh_test_clock = now_sec;
}

pub fn resetWebhookStore() {
    let i: usize = 0;
    while (i < WH_MAX_SEEN_IDS) : (i += 1) {
        g_wh_seen[i] = WhEventSlot{};
    }
    g_wh_seen_cursor = 0;
    g_wh_seen_count = 0;
}

pub fn whSeenCount(): usize {
    return g_wh_seen_count;
}

pub fn whBeforeHandle(ctx: *Context): bool {
    const cfg = g_wh_cfg;
    if (ctx.method != AppMethod.POST || !whStrEql(ctx.path, cfg.endpoint_path)) {
        return true;
    }

    // 1. Verify HMAC-SHA256 signature
    const sig_opt = ctx.header(cfg.sig_header);
    if (sig_opt == null) {
        ctx.abort(Response.json(401, "{\"error\":\"missing_webhook_signature\"}"));
        return false;
    }
    const sig_val = sig_opt orelse "";
    if (!webhookVerifySignature(cfg.secret, ctx.body, sig_val)) {
        ctx.abort(Response.json(401, "{\"error\":\"invalid_webhook_signature\"}"));
        return false;
    }

    // 2. Verify timestamp drift window (when timestamp header is provided)
    if (cfg.ts_header.len > 0) {
        if (ts_raw := ctx.header(cfg.ts_header)) {
            const ts_val = whParseU64(ts_raw) orelse {
                ctx.abort(Response.json(401, "{\"error\":\"invalid_webhook_timestamp\"}"));
                return false;
            };
            const now = whNowSec();
            const skew = if (now >= ts_val) (now - ts_val) else (ts_val - now);
            if (skew > cfg.max_skew_secs) {
                ctx.abort(Response.json(401, "{\"error\":\"webhook_timestamp_expired\"}"));
                return false;
            }
        }
    }

    // 3. Event ID idempotency deduplication (64-slot store)
    if (cfg.id_header.len > 0) {
        if (evt_id := ctx.header(cfg.id_header)) {
            if (evt_id.len > 0) {
                if (whHasSeenEvent(evt_id)) {
                    ctx.abort(Response.json(200, "{\"duplicate\":true}"));
                    return false;
                }
                whRecordEvent(evt_id);
            }
        }
    }

    return true;
}

fn whDefaultHandler(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"received\":true}");
}

pub fn whCreatePlugin(cfg: WebhookConfig): Plugin {
    g_wh_cfg = cfg;
    let p = Plugin.init("@web/webhook");
    * = p
        .withScope(PluginScope.Global)
        .onBeforeHandle(whBeforeHandle);
    if (cfg.endpoint_path.len > 0) {
        * = p.route(AppMethod.POST, cfg.endpoint_path, "Webhook Receiver", whDefaultHandler);
    }
    return p;
}

pub fn registerWebhook(app: *App, cfg: WebhookConfig): *App {
    const p = whCreatePlugin(cfg);
    return app.usePlugin(&p);
}
