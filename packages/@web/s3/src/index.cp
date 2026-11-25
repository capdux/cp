// Origin: Official @web/s3 Plugin (Pure-CP Object Key Sanitizer & AWS SigV4 Presigned URL) | Created: 2026-11-25
// ============================================================================
// @web/s3 — Safe Object Key Normalization & AWS SigV4 Presigned URL Plugin
//   - Inspired by `elysia-s3`
//   - Strict object key normalization (strips leading `/`, blocks `..`, `\`, control bytes)
//   - Pure-CP FIPS 180-4 SHA-256 + RFC 2104 HMAC-SHA256 AWS SigV4 query presigner
//   - Built-in `GET /upload/presign?key=...` endpoint returning JSON metadata
//   - Zero heap allocations, zero external C dependencies
// ============================================================================

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub struct S3PluginConfig {
    endpoint: string = "https://s3.amazonaws.com";
    bucket: string = "uploads";
    region: string = "us-east-1";
    access_key: string = "";
    secret_key: string = "";
    presign_path: string = "/upload/presign";
    default_expires_secs: u64 = 900;
}

// ============================================================================
// Internal Prefix-Scoped SHA-256 / HMAC-SHA256 & Formatting Helpers (s3w*)
// ============================================================================

const S3W_HEX_CHARS: string = "0123456789abcdef";

const S3W_SHA256_K: [64]u32 = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
];

let g_s3w_cfg: S3PluginConfig = S3PluginConfig{};
let g_s3w_resp_buf: [2048]u8 = [0; 2048];

fn s3wRotr32(x: u32, n: u5): u32 {
    const inv: u5 = @truncate(32 - @as(u6, n));
    return (x >> n) | (x << inv);
}

fn s3wSha256Compress(state: *[8]u32, block: *const [64]u8) {
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
        const s0 = s3wRotr32(w[i - 15], 7) ^ s3wRotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const s1 = s3wRotr32(w[i - 2], 17) ^ s3wRotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
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
        const s1_val = s3wRotr32(e, 6) ^ s3wRotr32(e, 11) ^ s3wRotr32(e, 25);
        const ch = (e & f) ^ ((~e) & g);
        const temp1 = h +% s1_val +% ch +% S3W_SHA256_K[i] +% w[i];
        const s0_val = s3wRotr32(a, 2) ^ s3wRotr32(a, 13) ^ s3wRotr32(a, 22);
        const maj = (a & b) ^ (a & c) ^ (b & c);
        const temp2 = s0_val +% maj;

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

struct S3wSha256Hasher {
    state: [8]u32 = [
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    ];
    buf: [64]u8 = [0; 64];
    buf_len: usize = 0;
    total_len: u64 = 0;

    static fn init(): S3wSha256Hasher {
        return S3wSha256Hasher{};
    }

    fn update(*_, data: []const u8) {
        let i: usize = 0;
        while (i < data.len) : (i += 1) {
            _.buf[_.buf_len] = data[i];
            _.buf_len += 1;
            _.total_len += 1;
            if (_.buf_len == 64) {
                s3wSha256Compress(*_.state, &_.buf);
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
            s3wSha256Compress(*_.state, &_.buf);
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
        s3wSha256Compress(*_.state, &_.buf);

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

fn s3wSha256(data: []const u8, out: *[32]u8) {
    let h = S3wSha256Hasher.init();
    h.update(data);
    h.finalize(out);
}

fn s3wHmacSha256(key: []const u8, msg: []const u8, out: *[32]u8) {
    let k_block: [64]u8 = [0; 64];
    if (key.len > 64) {
        let hashed_key: [32]u8 = [0; 32];
        s3wSha256(key, *hashed_key);
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
    let inner = S3wSha256Hasher.init();
    inner.update(ipad[0..64]);
    inner.update(msg);
    inner.finalize(*inner_digest);

    let outer = S3wSha256Hasher.init();
    outer.update(opad[0..64]);
    outer.update(inner_digest[0..32]);
    outer.finalize(out);
}

fn s3wBytesToHex(bytes: *const [32]u8, out_hex: *[64]u8) {
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        const b = bytes[i];
        out_hex[i * 2] = S3W_HEX_CHARS[@as(usize, (b >> 4) & 0x0f)];
        out_hex[i * 2 + 1] = S3W_HEX_CHARS[@as(usize, b & 0x0f)];
    }
}

fn s3wWriteByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn s3wWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (pos.* + s.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn s3wWriteU64(out_buf: []u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        return s3wWriteByte(out_buf, pos, '0');
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

fn s3wExtractHost(endpoint: string): string {
    let start: usize = 0;
    if (endpoint.len >= 8 && endpoint[0] == 'h' && endpoint[1] == 't' && endpoint[2] == 't' && endpoint[3] == 'p'
        && endpoint[4] == 's' && endpoint[5] == ':' && endpoint[6] == '/' && endpoint[7] == '/')
    {
        start = 8;
    } else if (endpoint.len >= 7 && endpoint[0] == 'h' && endpoint[1] == 't' && endpoint[2] == 't' && endpoint[3] == 'p'
        && endpoint[4] == ':' && endpoint[5] == '/' && endpoint[6] == '/')
    {
        start = 7;
    }
    let end: usize = start;
    while (end < endpoint.len && endpoint[end] != '/') : (end += 1) {}
    return endpoint[start..end];
}

fn s3wTrimTrailingSlash(s: string): string {
    let end = s.len;
    while (end > 0 && s[end - 1] == '/') : (end -= 1) {}
    return s[0..end];
}

fn s3wWriteJsonEscaped(out_buf: []u8, pos: *usize, s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '"' || c == '\\') {
            if (!s3wWriteByte(out_buf, pos, '\\')) return false;
        }
        if (!s3wWriteByte(out_buf, pos, c)) return false;
    }
    return true;
}

// ============================================================================
// Public @web/s3 API
// ============================================================================

/// Normalize an S3 object key into `out_buf`:
/// strips leading `/`, rejects empty keys, `..` traversal, `\`, and control bytes.
pub fn s3NormalizeKey(raw_key: string, out_buf: []u8): usize? {
    let start: usize = 0;
    while (start < raw_key.len && raw_key[start] == '/') : (start += 1) {}
    const trimmed = raw_key[start..];
    if (trimmed.len == 0) return null;
    if (trimmed.len > out_buf.len) return null;

    let i: usize = 0;
    while (i < trimmed.len) : (i += 1) {
        const c = trimmed[i];
        if (c < 0x20 || c == 0x7f || c == '\\') return null;
        if (c == '.' && i + 1 < trimmed.len && trimmed[i + 1] == '.') {
            return null;
        }
        out_buf[i] = c;
    }
    return trimmed.len;
}

/// Build an AWS SigV4 query-string presigned URL into `out_buf` (pure-CP, zero heap).
pub fn s3BuildPresignedUrl(
    cfg: S3PluginConfig,
    method: string,
    object_key: string,
    expires_secs: u64,
    amz_date: string,
    out_buf: []u8,
): usize? {
    if (cfg.endpoint.len == 0 || cfg.bucket.len == 0 || cfg.region.len == 0) return null;
    if (method.len == 0 || amz_date.len < 8) return null;

    let norm_buf: [512]u8 = [0; 512];
    const norm_len = s3NormalizeKey(object_key, norm_buf[0..]) orelse return null;
    const norm_key = norm_buf[0..norm_len];

    const eff_expires: u64 = if (expires_secs > 0) expires_secs else (if (cfg.default_expires_secs > 0) cfg.default_expires_secs else 900);
    const date_stamp = amz_date[0..8];
    const base_endpoint = s3wTrimTrailingSlash(cfg.endpoint);
    const host = s3wExtractHost(base_endpoint);
    if (host.len == 0) return null;

    // 1. Build canonical query string (sorted lexicographically per AWS SigV4)
    let cq_buf: [512]u8 = [0; 512];
    let cq_pos: usize = 0;
    if (!s3wWriteRaw(cq_buf[0..], *cq_pos, "X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=")) return null;
    if (!s3wWriteRaw(cq_buf[0..], *cq_pos, cfg.access_key)) return null;
    if (!s3wWriteRaw(cq_buf[0..], *cq_pos, "%2F")) return null;
    if (!s3wWriteRaw(cq_buf[0..], *cq_pos, date_stamp)) return null;
    if (!s3wWriteRaw(cq_buf[0..], *cq_pos, "%2F")) return null;
    if (!s3wWriteRaw(cq_buf[0..], *cq_pos, cfg.region)) return null;
    if (!s3wWriteRaw(cq_buf[0..], *cq_pos, "%2Fs3%2Faws4_request&X-Amz-Date=")) return null;
    if (!s3wWriteRaw(cq_buf[0..], *cq_pos, amz_date)) return null;
    if (!s3wWriteRaw(cq_buf[0..], *cq_pos, "&X-Amz-Expires=")) return null;
    if (!s3wWriteU64(cq_buf[0..], *cq_pos, eff_expires)) return null;
    if (!s3wWriteRaw(cq_buf[0..], *cq_pos, "&X-Amz-SignedHeaders=host")) return null;
    const canonical_query = cq_buf[0..cq_pos];

    // 2. Hash canonical request with SHA-256
    let cr_hasher = S3wSha256Hasher.init();
    cr_hasher.update(method);
    cr_hasher.update("\n/");
    cr_hasher.update(cfg.bucket);
    cr_hasher.update("/");
    cr_hasher.update(norm_key);
    cr_hasher.update("\n");
    cr_hasher.update(canonical_query);
    cr_hasher.update("\nhost:");
    cr_hasher.update(host);
    cr_hasher.update("\n\nhost\nUNSIGNED-PAYLOAD");

    let cr_digest: [32]u8 = [0; 32];
    cr_hasher.finalize(*cr_digest);
    let cr_hex: [64]u8 = [0; 64];
    s3wBytesToHex(&cr_digest, *cr_hex);

    // 3. Build String-to-Sign
    let sts_buf: [512]u8 = [0; 512];
    let sts_pos: usize = 0;
    if (!s3wWriteRaw(sts_buf[0..], *sts_pos, "AWS4-HMAC-SHA256\n")) return null;
    if (!s3wWriteRaw(sts_buf[0..], *sts_pos, amz_date)) return null;
    if (!s3wWriteRaw(sts_buf[0..], *sts_pos, "\n")) return null;
    if (!s3wWriteRaw(sts_buf[0..], *sts_pos, date_stamp)) return null;
    if (!s3wWriteRaw(sts_buf[0..], *sts_pos, "/")) return null;
    if (!s3wWriteRaw(sts_buf[0..], *sts_pos, cfg.region)) return null;
    if (!s3wWriteRaw(sts_buf[0..], *sts_pos, "/s3/aws4_request\n")) return null;
    if (!s3wWriteRaw(sts_buf[0..], *sts_pos, cr_hex[0..64])) return null;

    // 4. Derive SigV4 signing key: HMAC chain
    let aws4_key_buf: [256]u8 = [0; 256];
    let aws4_key_pos: usize = 0;
    if (!s3wWriteRaw(aws4_key_buf[0..], *aws4_key_pos, "AWS4")) return null;
    if (!s3wWriteRaw(aws4_key_buf[0..], *aws4_key_pos, cfg.secret_key)) return null;

    let k_date: [32]u8 = [0; 32];
    s3wHmacSha256(aws4_key_buf[0..aws4_key_pos], date_stamp, *k_date);
    let k_region: [32]u8 = [0; 32];
    s3wHmacSha256(k_date[0..32], cfg.region, *k_region);
    let k_service: [32]u8 = [0; 32];
    s3wHmacSha256(k_region[0..32], "s3", *k_service);
    let k_signing: [32]u8 = [0; 32];
    s3wHmacSha256(k_service[0..32], "aws4_request", *k_signing);

    let sig_digest: [32]u8 = [0; 32];
    s3wHmacSha256(k_signing[0..32], sts_buf[0..sts_pos], *sig_digest);
    let sig_hex: [64]u8 = [0; 64];
    s3wBytesToHex(&sig_digest, *sig_hex);

    // 5. Assemble final presigned URL
    let out_pos: usize = 0;
    if (!s3wWriteRaw(out_buf, *out_pos, base_endpoint)) return null;
    if (!s3wWriteByte(out_buf, *out_pos, '/')) return null;
    if (!s3wWriteRaw(out_buf, *out_pos, cfg.bucket)) return null;
    if (!s3wWriteByte(out_buf, *out_pos, '/')) return null;
    if (!s3wWriteRaw(out_buf, *out_pos, norm_key)) return null;
    if (!s3wWriteByte(out_buf, *out_pos, '?')) return null;
    if (!s3wWriteRaw(out_buf, *out_pos, canonical_query)) return null;
    if (!s3wWriteRaw(out_buf, *out_pos, "&X-Amz-Signature=")) return null;
    if (!s3wWriteRaw(out_buf, *out_pos, sig_hex[0..64])) return null;
    return out_pos;
}

/// Built-in `GET <presign_path>?key=...` handler returning presigned upload JSON.
pub fn s3wPresignHandler(ctx: *Context): Response {
    const raw_key = ctx.query("key") orelse {
        return Response.json(400, "{\"error\":\"missing_object_key\"}");
    };

    let norm_buf: [512]u8 = [0; 512];
    const norm_len = s3NormalizeKey(raw_key, norm_buf[0..]) orelse {
        return Response.json(400, "{\"error\":\"invalid_object_key\"}");
    };
    const norm_key = norm_buf[0..norm_len];

    const method = ctx.query("method") orelse "PUT";
    const expires = ctx.queryU64("expires") orelse g_s3w_cfg.default_expires_secs;
    const amz_date = ctx.query("date") orelse "20261125T000000Z";

    let url_buf: [1024]u8 = [0; 1024];
    const url_len = s3BuildPresignedUrl(g_s3w_cfg, method, norm_key, expires, amz_date, url_buf[0..]) orelse {
        return Response.json(500, "{\"error\":\"presign_failed\"}");
    };

    let pos: usize = 0;
    if (!s3wWriteRaw(g_s3w_resp_buf[0..], *pos, "{\"bucket\":\"")) return Response.json(500, "{\"error\":\"buf_overflow\"}");
    if (!s3wWriteJsonEscaped(g_s3w_resp_buf[0..], *pos, g_s3w_cfg.bucket)) return Response.json(500, "{\"error\":\"buf_overflow\"}");
    if (!s3wWriteRaw(g_s3w_resp_buf[0..], *pos, "\",\"key\":\"")) return Response.json(500, "{\"error\":\"buf_overflow\"}");
    if (!s3wWriteJsonEscaped(g_s3w_resp_buf[0..], *pos, norm_key)) return Response.json(500, "{\"error\":\"buf_overflow\"}");
    if (!s3wWriteRaw(g_s3w_resp_buf[0..], *pos, "\",\"url\":\"")) return Response.json(500, "{\"error\":\"buf_overflow\"}");
    if (!s3wWriteJsonEscaped(g_s3w_resp_buf[0..], *pos, url_buf[0..url_len])) return Response.json(500, "{\"error\":\"buf_overflow\"}");
    if (!s3wWriteRaw(g_s3w_resp_buf[0..], *pos, "\",\"expires\":")) return Response.json(500, "{\"error\":\"buf_overflow\"}");
    if (!s3wWriteU64(g_s3w_resp_buf[0..], *pos, expires)) return Response.json(500, "{\"error\":\"buf_overflow\"}");
    if (!s3wWriteRaw(g_s3w_resp_buf[0..], *pos, "}")) return Response.json(500, "{\"error\":\"buf_overflow\"}");

    return Response.json(200, g_s3w_resp_buf[0..pos]);
}

/// Create a composable Elysia-style `Plugin` for `@web/s3`.
pub fn createS3Plugin(cfg: S3PluginConfig): Plugin {
    g_s3w_cfg = cfg;
    const path = if (cfg.presign_path.len > 0) cfg.presign_path else "/upload/presign";
    let p = Plugin.init("@web/s3");
    * = p.withScope(PluginScope.Global);
    * = p.setState("s3_bucket", cfg.bucket);
    * = p.setState("s3_region", cfg.region);
    * = p.get(path, s3wPresignHandler);
    return p;
}

/// Register `@web/s3` directly onto `app` (mounts `GET <presign_path>?key=...`, fluent chainable).
pub fn registerS3(app: *App, cfg: S3PluginConfig): *App {
    const p = createS3Plugin(cfg);
    return app.usePlugin(&p);
}
