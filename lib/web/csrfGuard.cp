// Origin: Pure-CP Stdlib (Zero-Alloc Stateless HMAC-SHA256 CSRF Token & Sec-Fetch-Site/Origin Guard) | Created: 2026-11-23
// Pure CP zero-heap-allocation, zero-C-dependency Defense-in-Depth CSRF Protection Engine:
//   - Inline FIPS 180-4 SHA-256, RFC 2104 HMAC-SHA256, unpadded Base64URL & constant-time comparison
//   - Stateless session-bound CSRF token minting (`mintCsrfToken`) and constant-time verification
//     (`verifyCsrfToken`) binding `(version, issued_at_s, 16B nonce, session_id)` against cross-session
//     replay, tampering, and expiration
//   - Defense-in-depth `CsrfGuard.evaluateCsrfRequest`: auto-allows safe methods (GET/HEAD/OPTIONS),
//     blocks `Sec-Fetch-Site: cross-site`, validates `Origin` against trusted allowlist or `Host`
//     same-origin authority, enforces optional double-submit cookie equality, and verifies HMAC token
//   - Exact rejection verdicts & zero-heap global telemetry snapshot (`CsrfGuardStats`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Required nonce byte length for `mintCsrfToken` (16 bytes).
pub const CSRF_NONCE_LEN: usize = 16;

/// Encoded Base64URL CSRF token byte length (57 raw bytes -> exact 76 Base64URL chars, no padding).
pub const CSRF_TOKEN_LEN: usize = 76;

/// Maximum number of trusted origins stored inline in `CsrfGuard`.
pub const CSRF_MAX_TRUSTED_ORIGINS: usize = 16;

/// Maximum byte length of a single trusted origin string.
pub const CSRF_MAX_ORIGIN_LEN: usize = 128;

/// Default CSRF token lifetime in seconds (1 hour).
pub const CSRF_DEFAULT_TTL_S: i64 = 3600;

/// Token-level verification status returned by `verifyCsrfToken`.
pub enum CsrfTokenStatus {
    valid,
    invalid,
    expired,
}

/// Request-level defense-in-depth CSRF verdict returned by `CsrfGuard.evaluateCsrfRequest`.
pub enum CsrfVerdict {
    allowed_safe_method,
    allowed_verified,
    denied_cross_site,
    denied_origin_mismatch,
    denied_missing_token,
    denied_invalid_token,
    denied_expired_token,
}

/// HTTP request metadata supplied to `CsrfGuard.evaluateCsrfRequest` (zero-copy slices).
pub struct CsrfRequest {
    method: []const u8 = "GET";
    session_id: []const u8 = "";
    sec_fetch_site: []const u8 = "";
    origin: []const u8 = "";
    host: []const u8 = "";
    submitted_token: []const u8 = "";
    cookie_token: []const u8 = "";
}

/// Detailed authorization decision returned by `evaluateCsrfRequest`.
pub struct CsrfDecision {
    verdict: CsrfVerdict = CsrfVerdict.denied_invalid_token;
    allowed: bool = false;
    http_status: u16 = 403;
}

/// Zero-heap global telemetry snapshot for `CsrfGuard`.
pub struct CsrfGuardStats {
    trusted_origin_count: usize = 0;
    total_requests: u64 = 0;
    allowed_safe_count: u64 = 0;
    allowed_verified_count: u64 = 0;
    denied_cross_site_count: u64 = 0;
    denied_origin_mismatch_count: u64 = 0;
    denied_missing_token_count: u64 = 0;
    denied_invalid_token_count: u64 = 0;
    denied_expired_token_count: u64 = 0;
}

struct TrustedOriginSlot {
    active: bool = false;
    buf: [CSRF_MAX_ORIGIN_LEN]u8 = [0; CSRF_MAX_ORIGIN_LEN];
    len: usize = 0;

    fn slice(): []const u8 {
        return buf[0..len];
    }
}

// ============================================================================
// 2. Inline Pure-CP SHA-256, HMAC-SHA256, Base64URL & Constant-Time Helpers
// ============================================================================

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

fn rotr32(x: u32, n: u5): u32 {
    return (x >> n) | (x << (@as(u5, @intCast(32 - @as(u6, n)))));
}

struct Sha256State {
    h: [8]u32 = [
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    ];
    block: [64]u8 = [0; 64];
    block_len: usize = 0;
    total_bytes: u64 = 0;

    fn compress(*_) {
        let w: [64]u32 = [0; 64];
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            const j = i * 4;
            w[i] = (@as(u32, block[j]) << 24) |
                (@as(u32, block[j + 1]) << 16) |
                (@as(u32, block[j + 2]) << 8) |
                @as(u32, block[j + 3]);
        }
        while (i < 64) : (i += 1) {
            const s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] +% s0 +% w[i - 7] +% s1;
        }

        let a = h[0];
        let b = h[1];
        let c = h[2];
        let d = h[3];
        let e = h[4];
        let f = h[5];
        let g = h[6];
        let hh = h[7];

        i = 0;
        while (i < 64) : (i += 1) {
            const S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
            const ch = (e & f) ^ ((~e) & g);
            const temp1 = hh +% S1 +% ch +% SHA256_K[i] +% w[i];
            const S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
            const maj = (a & b) ^ (a & c) ^ (b & c);
            const temp2 = S0 +% maj;

            hh = g;
            g = f;
            f = e;
            e = d +% temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 +% temp2;
        }

        h[0] = h[0] +% a;
        h[1] = h[1] +% b;
        h[2] = h[2] +% c;
        h[3] = h[3] +% d;
        h[4] = h[4] +% e;
        h[5] = h[5] +% f;
        h[6] = h[6] +% g;
        h[7] = h[7] +% hh;
    }

    fn update(*_, data: []const u8) {
        let i: usize = 0;
        while (i < data.len) : (i += 1) {
            block[block_len] = data[i];
            block_len += 1;
            total_bytes += 1;
            if (block_len == 64) {
                compress();
                block_len = 0;
            }
        }
    }

    fn finalize(*_): [32]u8 {
        const bit_len = total_bytes * 8;
        block[block_len] = 0x80;
        block_len += 1;
        if (block_len > 56) {
            while (block_len < 64) : (block_len += 1) {
                block[block_len] = 0;
            }
            compress();
            block_len = 0;
        }
        while (block_len < 56) : (block_len += 1) {
            block[block_len] = 0;
        }
        block[56] = @as(u8, @intCast((bit_len >> 56) & 0xFF));
        block[57] = @as(u8, @intCast((bit_len >> 48) & 0xFF));
        block[58] = @as(u8, @intCast((bit_len >> 40) & 0xFF));
        block[59] = @as(u8, @intCast((bit_len >> 32) & 0xFF));
        block[60] = @as(u8, @intCast((bit_len >> 24) & 0xFF));
        block[61] = @as(u8, @intCast((bit_len >> 16) & 0xFF));
        block[62] = @as(u8, @intCast((bit_len >> 8) & 0xFF));
        block[63] = @as(u8, @intCast(bit_len & 0xFF));
        compress();

        let out: [32]u8 = [0; 32];
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            out[i * 4] = @as(u8, @intCast((h[i] >> 24) & 0xFF));
            out[i * 4 + 1] = @as(u8, @intCast((h[i] >> 16) & 0xFF));
            out[i * 4 + 2] = @as(u8, @intCast((h[i] >> 8) & 0xFF));
            out[i * 4 + 3] = @as(u8, @intCast(h[i] & 0xFF));
        }
        return out;
    }
}

fn sha256Bytes(data: []const u8): [32]u8 {
    let st = Sha256State{};
    st.update(data);
    return st.finalize();
}

fn hmacSha256Csrf(
    secret: []const u8,
    header_25: [25]u8,
    session_id: []const u8,
): [32]u8 {
    let key_block: [64]u8 = [0; 64];
    if (secret.len > 64) {
        const kh = sha256Bytes(secret);
        let i: usize = 0;
        while (i < 32) : (i += 1) {
            key_block[i] = kh[i];
        }
    } else {
        let i: usize = 0;
        while (i < secret.len) : (i += 1) {
            key_block[i] = secret[i];
        }
    }

    let ipad: [64]u8 = [0; 64];
    let opad: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        ipad[i] = key_block[i] ^ 0x36;
        opad[i] = key_block[i] ^ 0x5C;
    }

    let slen_be: [4]u8 = [0; 4];
    const slen_u32 = @as(u32, @intCast(session_id.len));
    slen_be[0] = @as(u8, @intCast((slen_u32 >> 24) & 0xFF));
    slen_be[1] = @as(u8, @intCast((slen_u32 >> 16) & 0xFF));
    slen_be[2] = @as(u8, @intCast((slen_u32 >> 8) & 0xFF));
    slen_be[3] = @as(u8, @intCast(slen_u32 & 0xFF));

    let inner = Sha256State{};
    inner.update(ipad[0..]);
    inner.update("cap-csrf-v1:");
    inner.update(header_25[0..]);
    inner.update(slen_be[0..]);
    inner.update(session_id);
    const inner_hash = inner.finalize();

    let outer = Sha256State{};
    outer.update(opad[0..]);
    outer.update(inner_hash[0..]);
    return outer.finalize();
}

/// Constant-time byte-slice equality (never short-circuits on content mismatch).
pub fn csrfConstantTimeEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

const B64URL_ALPHABET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

fn b64UrlDecodeChar(c: u8): u8? {
    if (c >= 'A' and c <= 'Z') return c - 'A';
    if (c >= 'a' and c <= 'z') return c - 'a' + 26;
    if (c >= '0' and c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return null;
}

fn encodeRaw57ToB64Url76(raw: [57]u8, out_buf: *[]u8): bool {
    if (out_buf.len < CSRF_TOKEN_LEN) return false;
    let src_i: usize = 0;
    let dst_i: usize = 0;
    while (src_i < 57) : (src_i += 3) {
        const b0 = @as(u32, raw[src_i]);
        const b1 = @as(u32, raw[src_i + 1]);
        const b2 = @as(u32, raw[src_i + 2]);
        const triple = (b0 << 16) | (b1 << 8) | b2;
        out_buf[dst_i] = B64URL_ALPHABET[@as(usize, (triple >> 18) & 0x3F)];
        out_buf[dst_i + 1] = B64URL_ALPHABET[@as(usize, (triple >> 12) & 0x3F)];
        out_buf[dst_i + 2] = B64URL_ALPHABET[@as(usize, (triple >> 6) & 0x3F)];
        out_buf[dst_i + 3] = B64URL_ALPHABET[@as(usize, triple & 0x3F)];
        dst_i += 4;
    }
    return true;
}

fn decodeB64Url76ToRaw57(tok: []const u8): ([57]u8)? {
    if (tok.len != CSRF_TOKEN_LEN) return null;
    let out: [57]u8 = [0; 57];
    let src_i: usize = 0;
    let dst_i: usize = 0;
    while (src_i < CSRF_TOKEN_LEN) : (src_i += 4) {
        const v0 = b64UrlDecodeChar(tok[src_i]) ?? return null;
        const v1 = b64UrlDecodeChar(tok[src_i + 1]) ?? return null;
        const v2 = b64UrlDecodeChar(tok[src_i + 2]) ?? return null;
        const v3 = b64UrlDecodeChar(tok[src_i + 3]) ?? return null;
        const triple = (@as(u32, v0) << 18) | (@as(u32, v1) << 12) | (@as(u32, v2) << 6) | @as(u32, v3);
        out[dst_i] = @as(u8, @intCast((triple >> 16) & 0xFF));
        out[dst_i + 1] = @as(u8, @intCast((triple >> 8) & 0xFF));
        out[dst_i + 2] = @as(u8, @intCast(triple & 0xFF));
        dst_i += 3;
    }
    return out;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' and c <= 'Z') return c + 32;
    return c;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn hasUnsafeCtrl(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] <= 0x20 or s[i] >= 0x7F) return true;
    }
    return false;
}

fn isSafeHttpMethod(method: []const u8): bool {
    return eqIgnoreCase(method, "GET") or
        eqIgnoreCase(method, "HEAD") or
        eqIgnoreCase(method, "OPTIONS");
}

/// Extract the `authority` (`host[:port]`) from a valid `http://` or `https://` Origin header.
/// Rejects path/query/fragment/userinfo/control chars or `"null"`.
fn extractOriginAuthority(origin: []const u8): ([]const u8)? {
    if (origin.len == 0 or hasUnsafeCtrl(origin)) return null;
    let rest: []const u8 = "";
    if (origin.len > 8 and eqIgnoreCase(origin[0..8], "https://")) {
        rest = origin[8..];
    } else if (origin.len > 7 and eqIgnoreCase(origin[0..7], "http://")) {
        rest = origin[7..];
    } else {
        return null;
    }
    if (rest.len == 0) return null;
    let i: usize = 0;
    while (i < rest.len) : (i += 1) {
        const c = rest[i];
        if (c == '/' or c == '?' or c == '#' or c == '@' or c == '\\') return null;
    }
    return rest;
}

// ============================================================================
// 3. Stateless CSRF Token Mint & Verify APIs
// ============================================================================

/// Mint a stateless session-bound HMAC-SHA256 CSRF token into `out_buf` (writes 76 Base64URL bytes).
/// Requires non-empty `secret`, non-empty `session_id`, `issued_at_s > 0`, and `nonce.len == 16`.
pub fn mintCsrfToken(
    secret: []const u8,
    session_id: []const u8,
    issued_at_s: i64,
    nonce: []const u8,
    out_buf: *[]u8,
): usize? {
    if (secret.len == 0 or session_id.len == 0) return null;
    if (issued_at_s <= 0 or nonce.len != CSRF_NONCE_LEN) return null;
    if (out_buf.len < CSRF_TOKEN_LEN) return null;

    let hdr: [25]u8 = [0; 25];
    hdr[0] = 1; // format version 1
    const ts_u64 = @as(u64, @intCast(issued_at_s));
    hdr[1] = @as(u8, @intCast((ts_u64 >> 56) & 0xFF));
    hdr[2] = @as(u8, @intCast((ts_u64 >> 48) & 0xFF));
    hdr[3] = @as(u8, @intCast((ts_u64 >> 40) & 0xFF));
    hdr[4] = @as(u8, @intCast((ts_u64 >> 32) & 0xFF));
    hdr[5] = @as(u8, @intCast((ts_u64 >> 24) & 0xFF));
    hdr[6] = @as(u8, @intCast((ts_u64 >> 16) & 0xFF));
    hdr[7] = @as(u8, @intCast((ts_u64 >> 8) & 0xFF));
    hdr[8] = @as(u8, @intCast(ts_u64 & 0xFF));

    let i: usize = 0;
    while (i < CSRF_NONCE_LEN) : (i += 1) {
        hdr[9 + i] = nonce[i];
    }

    const mac = hmacSha256Csrf(secret, hdr, session_id);

    let raw: [57]u8 = [0; 57];
    i = 0;
    while (i < 25) : (i += 1) {
        raw[i] = hdr[i];
    }
    i = 0;
    while (i < 32) : (i += 1) {
        raw[25 + i] = mac[i];
    }

    if (!encodeRaw57ToB64Url76(raw, out_buf)) return null;
    return CSRF_TOKEN_LEN;
}

/// Verify a stateless session-bound HMAC-SHA256 CSRF token in constant time against `secret`,
/// `session_id`, and `[now_s - ttl_s, now_s + 30]` validity window.
pub fn verifyCsrfToken(
    secret: []const u8,
    session_id: []const u8,
    token: []const u8,
    now_s: i64,
    ttl_s: i64,
): CsrfTokenStatus {
    if (secret.len == 0 or session_id.len == 0 or ttl_s <= 0) {
        return CsrfTokenStatus.invalid;
    }

    const raw = decodeB64Url76ToRaw57(token) ?? return CsrfTokenStatus.invalid;
    if (raw[0] != 1) return CsrfTokenStatus.invalid;

    let hdr: [25]u8 = [0; 25];
    let i: usize = 0;
    while (i < 25) : (i += 1) {
        hdr[i] = raw[i];
    }

    const expected_mac = hmacSha256Csrf(secret, hdr, session_id);
    if (!csrfConstantTimeEq(expected_mac[0..], raw[25..57])) {
        return CsrfTokenStatus.invalid;
    }

    const ts_u64: u64 = (@as(u64, raw[1]) << 56) |
        (@as(u64, raw[2]) << 48) |
        (@as(u64, raw[3]) << 40) |
        (@as(u64, raw[4]) << 32) |
        (@as(u64, raw[5]) << 24) |
        (@as(u64, raw[6]) << 16) |
        (@as(u64, raw[7]) << 8) |
        @as(u64, raw[8]);
    if (ts_u64 == 0 or ts_u64 > 0x7FFF_FFFF_FFFF_FFFF) {
        return CsrfTokenStatus.invalid;
    }
    const issued_at_s = @as(i64, @intCast(ts_u64));

    // Reject far-future tokens (>30s clock skew) as invalid.
    if (issued_at_s > now_s and (issued_at_s - now_s) > 30) {
        return CsrfTokenStatus.invalid;
    }
    // Check TTL expiration safely without overflow.
    if (now_s > issued_at_s and (now_s - issued_at_s) > ttl_s) {
        return CsrfTokenStatus.expired;
    }
    return CsrfTokenStatus.valid;
}

// ============================================================================
// 4. CsrfGuard — Defense-in-Depth Sec-Fetch-Site + Origin + Token Evaluator
// ============================================================================

/// Zero-heap defense-in-depth CSRF guard combining `Sec-Fetch-Site`, `Origin`/`Host`,
/// optional double-submit cookie equality, and stateless HMAC-SHA256 token verification.
pub struct CsrfGuard {
    ttl_s: i64 = CSRF_DEFAULT_TTL_S;
    allow_same_site: bool = false;
    require_origin_header: bool = false;
    trusted_origins: [CSRF_MAX_TRUSTED_ORIGINS]TrustedOriginSlot = [TrustedOriginSlot{}; CSRF_MAX_TRUSTED_ORIGINS];
    trusted_origin_count: usize = 0;

    total_requests: u64 = 0;
    allowed_safe_count: u64 = 0;
    allowed_verified_count: u64 = 0;
    denied_cross_site_count: u64 = 0;
    denied_origin_mismatch_count: u64 = 0;
    denied_missing_token_count: u64 = 0;
    denied_invalid_token_count: u64 = 0;
    denied_expired_token_count: u64 = 0;

    static fn init(ttl_s: i64, allow_same_site: bool, require_origin_header: bool): CsrfGuard {
        let g = CsrfGuard{};
        g.ttl_s = if (ttl_s <= 0) CSRF_DEFAULT_TTL_S else ttl_s;
        g.allow_same_site = allow_same_site;
        g.require_origin_header = require_origin_header;
        return g;
    }

    /// Add an exact trusted `Origin` (e.g. `"https://app.cap.dev"`) to the allowlist.
    pub fn addTrustedOrigin(*_, origin: []const u8): bool {
        if (origin.len == 0 or origin.len > CSRF_MAX_ORIGIN_LEN) return false;
        if (extractOriginAuthority(origin) == null) return false;

        let free_slot: usize = CSRF_MAX_TRUSTED_ORIGINS;
        let i: usize = 0;
        while (i < CSRF_MAX_TRUSTED_ORIGINS) : (i += 1) {
            if (trusted_origins[i].active) {
                if (eqIgnoreCase(trusted_origins[i].slice(), origin)) return true;
            } else if (free_slot == CSRF_MAX_TRUSTED_ORIGINS) {
                free_slot = i;
            }
        }
        if (free_slot == CSRF_MAX_TRUSTED_ORIGINS) return false;

        trusted_origins[free_slot].active = true;
        let k: usize = 0;
        while (k < origin.len) : (k += 1) {
            trusted_origins[free_slot].buf[k] = origin[k];
        }
        trusted_origins[free_slot].len = origin.len;
        trusted_origin_count += 1;
        return true;
    }

    fn isTrustedOrigin(origin: []const u8): bool {
        let i: usize = 0;
        while (i < CSRF_MAX_TRUSTED_ORIGINS) : (i += 1) {
            if (trusted_origins[i].active and eqIgnoreCase(trusted_origins[i].slice(), origin)) {
                return true;
            }
        }
        return false;
    }

    /// Evaluate an incoming HTTP request through all CSRF defense layers.
    pub fn evaluateCsrfRequest(
        *_,
        req: CsrfRequest,
        secret: []const u8,
        now_s: i64,
    ): CsrfDecision {
        total_requests += 1;

        // 1. Safe idempotent methods (GET / HEAD / OPTIONS) pass automatically.
        if (isSafeHttpMethod(req.method)) {
            allowed_safe_count += 1;
            return CsrfDecision{
                verdict: CsrfVerdict.allowed_safe_method,
                allowed: true,
                http_status: 200,
            };
        }

        // 2. Layer 1: Sec-Fetch-Site check (when header is present).
        if (req.sec_fetch_site.len > 0) {
            const is_same_origin = eqIgnoreCase(req.sec_fetch_site, "same-origin");
            const is_none = eqIgnoreCase(req.sec_fetch_site, "none");
            const is_same_site = eqIgnoreCase(req.sec_fetch_site, "same-site");
            const site_ok = is_same_origin or is_none or (allow_same_site and is_same_site);
            if (!site_ok) {
                denied_cross_site_count += 1;
                return CsrfDecision{
                    verdict: CsrfVerdict.denied_cross_site,
                    allowed: false,
                    http_status: 403,
                };
            }
        }

        // 3. Layer 2: Origin allowlist / Host same-origin authority verification.
        if (req.origin.len > 0) {
            if (!isTrustedOrigin(req.origin)) {
                const auth_opt = extractOriginAuthority(req.origin);
                if (auth_opt == null or req.host.len == 0 or hasUnsafeCtrl(req.host)) {
                    denied_origin_mismatch_count += 1;
                    return CsrfDecision{
                        verdict: CsrfVerdict.denied_origin_mismatch,
                        allowed: false,
                        http_status: 403,
                    };
                }
                const auth = auth_opt ?? "";
                if (!eqIgnoreCase(auth, req.host)) {
                    denied_origin_mismatch_count += 1;
                    return CsrfDecision{
                        verdict: CsrfVerdict.denied_origin_mismatch,
                        allowed: false,
                        http_status: 403,
                    };
                }
            }
        } else if (require_origin_header) {
            denied_origin_mismatch_count += 1;
            return CsrfDecision{
                verdict: CsrfVerdict.denied_origin_mismatch,
                allowed: false,
                http_status: 403,
            };
        }

        // 4. Layer 3: CSRF token presence, optional double-submit cookie match & HMAC verification.
        if (req.submitted_token.len == 0) {
            denied_missing_token_count += 1;
            return CsrfDecision{
                verdict: CsrfVerdict.denied_missing_token,
                allowed: false,
                http_status: 403,
            };
        }

        if (req.cookie_token.len > 0 and !csrfConstantTimeEq(req.submitted_token, req.cookie_token)) {
            denied_invalid_token_count += 1;
            return CsrfDecision{
                verdict: CsrfVerdict.denied_invalid_token,
                allowed: false,
                http_status: 403,
            };
        }

        const tok_status = verifyCsrfToken(secret, req.session_id, req.submitted_token, now_s, ttl_s);
        if (tok_status == CsrfTokenStatus.valid) {
            allowed_verified_count += 1;
            return CsrfDecision{
                verdict: CsrfVerdict.allowed_verified,
                allowed: true,
                http_status: 200,
            };
        }
        if (tok_status == CsrfTokenStatus.expired) {
            denied_expired_token_count += 1;
            return CsrfDecision{
                verdict: CsrfVerdict.denied_expired_token,
                allowed: false,
                http_status: 403,
            };
        }

        denied_invalid_token_count += 1;
        return CsrfDecision{
            verdict: CsrfVerdict.denied_invalid_token,
            allowed: false,
            http_status: 403,
        };
    }

    /// Return a zero-heap global telemetry snapshot.
    pub fn stats(): CsrfGuardStats {
        return CsrfGuardStats{
            trusted_origin_count: trusted_origin_count,
            total_requests: total_requests,
            allowed_safe_count: allowed_safe_count,
            allowed_verified_count: allowed_verified_count,
            denied_cross_site_count: denied_cross_site_count,
            denied_origin_mismatch_count: denied_origin_mismatch_count,
            denied_missing_token_count: denied_missing_token_count,
            denied_invalid_token_count: denied_invalid_token_count,
            denied_expired_token_count: denied_expired_token_count,
        };
    }

    pub fn resetStats(*_) {
        total_requests = 0;
        allowed_safe_count = 0;
        allowed_verified_count = 0;
        denied_cross_site_count = 0;
        denied_origin_mismatch_count = 0;
        denied_missing_token_count = 0;
        denied_invalid_token_count = 0;
        denied_expired_token_count = 0;
    }
}
