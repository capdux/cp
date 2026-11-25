// Origin: Official @web/jwt Plugin (Pure-CP Zero-Heap HS256 JWT) | Created: 2026-11-25
// ============================================================================
// @web/jwt — Pure-CP Zero-Heap HS256 JWT Authentication Plugin for cap:web
//   - Built-in pure-CP FIPS 180-4 SHA-256 + RFC 2104 HMAC-SHA256
//   - Built-in pure-CP RFC 4648 Base64URL encoder/decoder (no padding)
//   - Constant-time signature verification & 3-segment structure validation
//   - Zero heap allocations, zero external C dependencies
// ============================================================================

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

// Base64URL("{\"alg\":\"HS256\",\"typ\":\"JWT\"}")
const JWT_HS256_HEADER_B64: string = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";

const BASE64URL_ALPHABET: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

const SHA256_K: [64]u32 = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
];

// ============================================================================
// Pure-CP SHA-256 & HMAC-SHA256
// ============================================================================

fn rotr32(x: u32, n: u5): u32 {
    const inv: u5 = @truncate(32 - @as(u6, n));
    return (x >> n) | (x << inv);
}

fn sha256Compress(state: *[8]u32, block: *const [64]u8) {
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
        const s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
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
        const S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        const ch = (e & f) ^ ((~e) & g);
        const temp1 = h +% S1 +% ch +% SHA256_K[i] +% w[i];
        const S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
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

struct Sha256Hasher {
    state: [8]u32 = [
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    ];
    buf: [64]u8 = [0; 64];
    buf_len: usize = 0;
    total_len: u64 = 0;

    static fn init(): Sha256Hasher {
        return Sha256Hasher{};
    }

    fn update(*_, data: []const u8) {
        let i: usize = 0;
        while (i < data.len) : (i += 1) {
            _.buf[_.buf_len] = data[i];
            _.buf_len += 1;
            _.total_len += 1;
            if (_.buf_len == 64) {
                sha256Compress(*_.state, &_.buf);
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
            sha256Compress(*_.state, &_.buf);
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
        sha256Compress(*_.state, &_.buf);

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

pub fn sha256(data: []const u8, out: *[32]u8) {
    let h = Sha256Hasher.init();
    h.update(data);
    h.finalize(out);
}

pub fn hmacSha256(key: string, msg: []const u8, out: *[32]u8) {
    let k_block: [64]u8 = [0; 64];
    if (key.len > 64) {
        let hashed_key: [32]u8 = [0; 32];
        sha256(key, *hashed_key);
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
    let inner = Sha256Hasher.init();
    inner.update(ipad[0..64]);
    inner.update(msg);
    inner.finalize(*inner_digest);

    let outer = Sha256Hasher.init();
    outer.update(opad[0..64]);
    outer.update(inner_digest[0..32]);
    outer.finalize(out);
}

// ============================================================================
// Pure-CP Base64URL (RFC 4648 Section 5, No Padding)
// ============================================================================

pub fn base64UrlEncode(input: []const u8, out_buf: []u8): usize? {
    const full_chunks = input.len / 3;
    const rem = input.len % 3;
    const needed: usize = full_chunks * 4 + (if (rem == 0) 0 else rem + 1);
    if (needed > out_buf.len) return null;

    let in_idx: usize = 0;
    let out_idx: usize = 0;
    let c: usize = 0;
    while (c < full_chunks) : (c += 1) {
        const b0: u32 = @as(u32, input[in_idx]);
        const b1: u32 = @as(u32, input[in_idx + 1]);
        const b2: u32 = @as(u32, input[in_idx + 2]);
        const triple: u32 = (b0 << 16) | (b1 << 8) | b2;
        out_buf[out_idx] = BASE64URL_ALPHABET[@as(usize, (triple >> 18) & 0x3F)];
        out_buf[out_idx + 1] = BASE64URL_ALPHABET[@as(usize, (triple >> 12) & 0x3F)];
        out_buf[out_idx + 2] = BASE64URL_ALPHABET[@as(usize, (triple >> 6) & 0x3F)];
        out_buf[out_idx + 3] = BASE64URL_ALPHABET[@as(usize, triple & 0x3F)];
        in_idx += 3;
        out_idx += 4;
    }

    if (rem == 1) {
        const b0: u32 = @as(u32, input[in_idx]);
        out_buf[out_idx] = BASE64URL_ALPHABET[@as(usize, (b0 >> 2) & 0x3F)];
        out_buf[out_idx + 1] = BASE64URL_ALPHABET[@as(usize, (b0 & 0x03) << 4)];
        out_idx += 2;
    } else if (rem == 2) {
        const b0: u32 = @as(u32, input[in_idx]);
        const b1: u32 = @as(u32, input[in_idx + 1]);
        const pair: u32 = (b0 << 8) | b1;
        out_buf[out_idx] = BASE64URL_ALPHABET[@as(usize, (pair >> 10) & 0x3F)];
        out_buf[out_idx + 1] = BASE64URL_ALPHABET[@as(usize, (pair >> 4) & 0x3F)];
        out_buf[out_idx + 2] = BASE64URL_ALPHABET[@as(usize, (pair & 0x0F) << 2)];
        out_idx += 3;
    }

    return out_idx;
}

fn base64UrlCharVal(ch: u8): u8? {
    if (ch >= 'A' && ch <= 'Z') return ch - 'A';
    if (ch >= 'a' && ch <= 'z') return (ch - 'a') + 26;
    if (ch >= '0' && ch <= '9') return (ch - '0') + 52;
    if (ch == '-') return 62;
    if (ch == '_') return 63;
    return null;
}

pub fn base64UrlDecode(input: string, out_buf: []u8): usize? {
    let eff_len = input.len;
    while (eff_len > 0 && input[eff_len - 1] == '=') : (eff_len -= 1) {}
    const rem = eff_len % 4;
    if (rem == 1) return null;

    const full_quads = eff_len / 4;
    let extra: usize = 0;
    if (rem == 2) {
        extra = 1;
    } else if (rem == 3) {
        extra = 2;
    }
    const needed: usize = full_quads * 3 + extra;
    if (needed > out_buf.len) return null;

    let in_idx: usize = 0;
    let out_idx: usize = 0;
    let q: usize = 0;
    while (q < full_quads) : (q += 1) {
        const v0 = base64UrlCharVal(input[in_idx]) orelse return null;
        const v1 = base64UrlCharVal(input[in_idx + 1]) orelse return null;
        const v2 = base64UrlCharVal(input[in_idx + 2]) orelse return null;
        const v3 = base64UrlCharVal(input[in_idx + 3]) orelse return null;
        const triple: u32 = (@as(u32, v0) << 18) | (@as(u32, v1) << 12) | (@as(u32, v2) << 6) | @as(u32, v3);
        out_buf[out_idx] = @truncate((triple >> 16) & 0xFF);
        out_buf[out_idx + 1] = @truncate((triple >> 8) & 0xFF);
        out_buf[out_idx + 2] = @truncate(triple & 0xFF);
        in_idx += 4;
        out_idx += 3;
    }

    if (rem == 2) {
        const v0 = base64UrlCharVal(input[in_idx]) orelse return null;
        const v1 = base64UrlCharVal(input[in_idx + 1]) orelse return null;
        const val: u32 = (@as(u32, v0) << 6) | @as(u32, v1);
        out_buf[out_idx] = @truncate((val >> 4) & 0xFF);
        out_idx += 1;
    } else if (rem == 3) {
        const v0 = base64UrlCharVal(input[in_idx]) orelse return null;
        const v1 = base64UrlCharVal(input[in_idx + 1]) orelse return null;
        const v2 = base64UrlCharVal(input[in_idx + 2]) orelse return null;
        const val: u32 = (@as(u32, v0) << 12) | (@as(u32, v1) << 6) | @as(u32, v2);
        out_buf[out_idx] = @truncate((val >> 10) & 0xFF);
        out_buf[out_idx + 1] = @truncate((val >> 2) & 0xFF);
        out_idx += 2;
    }

    return out_idx;
}

// ============================================================================
// Constant-Time Comparison & String Helpers
// ============================================================================

fn jwtConstantTimeEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

fn jwtAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn jwtAsciiEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (jwtAsciiLower(a[i]) != jwtAsciiLower(b[i])) return false;
    }
    return true;
}

fn jwtTrimSpaces(s: string): string {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn jwtContainsSubstr(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    const limit = haystack.len - needle.len;
    while (i <= limit) : (i += 1) {
        let j: usize = 0;
        let matched: bool = true;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                matched = false;
                break;
            }
        }
        if (matched) return true;
    }
    return false;
}

fn jwtWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

// ============================================================================
// Public HS256 JWT API
// ============================================================================

pub struct JwtConfig {
    secret: string = "";
    issuer: string = "";
    header_name: string = "Authorization";
    optional_auth: bool = false;
}

let g_jwt_cfg: JwtConfig = JwtConfig{};
let g_jwt_payload_buf: [1024]u8 = [0; 1024];

/// Sign `payload_json` as an HS256 JWT into `out_buf` (zero heap).
pub fn signHs256(secret: string, payload_json: string, out_buf: []u8): usize? {
    let pos: usize = 0;
    if (!jwtWriteRaw(out_buf, *pos, JWT_HS256_HEADER_B64)) return null;
    if (pos >= out_buf.len) return null;
    out_buf[pos] = '.';
    pos += 1;

    const payload_b64_len = base64UrlEncode(payload_json, out_buf[pos..]) orelse return null;
    pos += payload_b64_len;
    const signing_input_len = pos;

    let mac: [32]u8 = [0; 32];
    hmacSha256(secret, out_buf[0..signing_input_len], *mac);

    if (pos >= out_buf.len) return null;
    out_buf[pos] = '.';
    pos += 1;

    const sig_b64_len = base64UrlEncode(mac[0..32], out_buf[pos..]) orelse return null;
    pos += sig_b64_len;
    return pos;
}

/// Verify an HS256 JWT (`header.payload.signature`) with constant-time MAC check.
/// Decodes the JSON payload into `out_payload` and returns its byte length.
pub fn verifyHs256(secret: string, token: string, out_payload: []u8): usize? {
    let dot1: usize = 0;
    let dot2: usize = 0;
    let dots: usize = 0;
    let i: usize = 0;
    while (i < token.len) : (i += 1) {
        if (token[i] == '.') {
            if (dots == 0) {
                dot1 = i;
            } else if (dots == 1) {
                dot2 = i;
            } else {
                return null;
            }
            dots += 1;
        }
    }
    if (dots != 2) return null;
    if (dot1 == 0 || dot2 <= dot1 + 1 || dot2 + 1 >= token.len) return null;

    const hdr_b64 = token[0..dot1];
    const pay_b64 = token[dot1 + 1 .. dot2];
    const sig_b64 = token[dot2 + 1 ..];

    // Validate header decodes as valid Base64URL and declares HS256
    let hdr_raw: [256]u8 = [0; 256];
    const hdr_len = base64UrlDecode(hdr_b64, hdr_raw[0..]) orelse return null;
    if (!jwtContainsSubstr(hdr_raw[0..hdr_len], "\"HS256\"")) return null;

    // Decode signature (must be exactly 32 bytes for HS256)
    let sig_bytes: [64]u8 = [0; 64];
    const sig_len = base64UrlDecode(sig_b64, sig_bytes[0..]) orelse return null;
    if (sig_len != 32) return null;

    // Constant-time HMAC-SHA256 verification
    let expected_mac: [32]u8 = [0; 32];
    hmacSha256(secret, token[0..dot2], *expected_mac);
    if (!jwtConstantTimeEql(sig_bytes[0..32], expected_mac[0..32])) return null;

    return base64UrlDecode(pay_b64, out_payload);
}

fn jwtCheckIssuerClaim(payload: string, issuer: string): bool {
    if (issuer.len == 0) return true;
    let pattern: [192]u8 = [0; 192];
    let pos: usize = 0;
    if (!jwtWriteRaw(pattern[0..], *pos, "\"iss\":\"")) return false;
    if (!jwtWriteRaw(pattern[0..], *pos, issuer)) return false;
    if (!jwtWriteRaw(pattern[0..], *pos, "\"")) return false;
    return jwtContainsSubstr(payload, pattern[0..pos]);
}

pub fn configure(cfg: JwtConfig) {
    g_jwt_cfg = cfg;
}

/// Lifecycle hook that authenticates `Authorization: Bearer <token>` (or configured header).
pub fn authHook(ctx: *Context): bool {
    const hdr_name = if (g_jwt_cfg.header_name.len > 0) g_jwt_cfg.header_name else "Authorization";
    if (raw_hdr := ctx.header(hdr_name)) {
        if (raw_hdr.len > 7 && jwtAsciiEqlIgnoreCase(raw_hdr[0..7], "Bearer ")) {
            const tok = jwtTrimSpaces(raw_hdr[7..]);
            if (tok.len > 0) {
                if (plen := verifyHs256(g_jwt_cfg.secret, tok, g_jwt_payload_buf[0..])) {
                    const payload_slice = g_jwt_payload_buf[0..plen];
                    if (jwtCheckIssuerClaim(payload_slice, g_jwt_cfg.issuer)) {
                        * = ctx.setDerived("jwt_verified", "1");
                        * = ctx.setDerived("jwt_payload", payload_slice);
                        * = ctx.setHeader("X-Auth-Scheme", "Bearer");
                        return true;
                    }
                }
            }
        }
    }

    if (g_jwt_cfg.optional_auth) {
        return true;
    }

    ctx.abort(Response.json(401, "{\"error\":\"Unauthorized\",\"message\":\"Invalid or missing Bearer JWT\"}"));
    return false;
}

/// Create a composable Elysia-style `Plugin` for `@web/jwt`.
pub fn createJwtPlugin(cfg: JwtConfig): Plugin {
    g_jwt_cfg = cfg;
    let p = Plugin.init("@web/jwt");
    * = p.withScope(PluginScope.Global);
    * = p.onBeforeHandle(authHook);
    return p;
}

/// Register `@web/jwt` directly onto `app` (fluent chainable).
pub fn registerJwt(app: *App, cfg: JwtConfig): *App {
    g_jwt_cfg = cfg;
    return app.onBeforeHandle(authHook);
}
