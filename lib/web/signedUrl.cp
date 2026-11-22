// Origin: Pure-CP Stdlib (Zero-Alloc HMAC-SHA256 Pre-Signed URL & Multi-Key Rotation Verifier) | Created: 2026-11-23
// Pure CP zero-heap-allocation, zero-C-dependency Pre-Signed URL Generator & Constant-Time Verifier:
//   - Inline FIPS 180-4 SHA-256, RFC 2104 HMAC-SHA256, unpadded Base64URL & constant-time comparison
//   - Domain-separated canonical signing over `(method, path, query_without_sig, exp_epoch_s, key_id)`
//     producing pre-signed URLs with `exp`, optional `kid`, and 43-char Base64URL `sig` query params
//   - Constant-time verifier (`verifySignedUrl`) stripping `sig`/`exp`/`kid` without heap allocation,
//     checking expiry window with configurable clock skew, detecting method mismatch, and guarding
//     against CRLF/control-char injection, malformed queries, duplicate reserved params & buffer overflow
//   - Multi-key rotation keyring (`SignedUrlKeyring`, up to 8 named keys) enabling zero-downtime key
//     rotation via `kid` lookup and zero-heap global telemetry snapshot (`SignedUrlKeyringStats`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Encoded Base64URL HMAC-SHA256 signature length (32 raw bytes -> 43 unpadded Base64URL chars).
pub const SIGNED_URL_SIG_LEN: usize = 43;

/// Maximum number of named keys stored inline in `SignedUrlKeyring`.
pub const SIGNED_URL_MAX_KEYS: usize = 8;

/// Maximum byte length of a key identifier (`kid`).
pub const SIGNED_URL_MAX_KID_LEN: usize = 32;

/// Maximum byte length of a raw HMAC secret stored inline in `SignedUrlKeyring`.
pub const SIGNED_URL_MAX_SECRET_LEN: usize = 128;

/// Default allowed clock skew in seconds during expiry validation.
pub const SIGNED_URL_DEFAULT_CLOCK_SKEW_S: i64 = 30;

/// Verification outcome returned by `verifySignedUrl` and `SignedUrlKeyring.verify`.
pub enum SignedUrlStatus {
    valid,
    missing_signature,
    missing_expiry,
    expired,
    invalid_signature,
    method_mismatch,
    malformed_url,
}

/// Detailed result returned by `verifySignedUrl` and `SignedUrlKeyring.verify`.
pub struct SignedUrlVerifyResult {
    status: SignedUrlStatus = SignedUrlStatus.malformed_url;
    valid: bool = false;
    exp_epoch_s: i64 = 0;
    kid_buf: [SIGNED_URL_MAX_KID_LEN]u8 = [0; SIGNED_URL_MAX_KID_LEN];
    kid_len: usize = 0;

    /// Return the parsed key identifier (`kid`) slice, or empty slice if absent.
    fn kid(): []const u8 {
        return kid_buf[0..kid_len];
    }
}

/// Zero-heap global telemetry snapshot for `SignedUrlKeyring`.
pub struct SignedUrlKeyringStats {
    key_count: usize = 0;
    capacity: usize = SIGNED_URL_MAX_KEYS;
    signed_total: u64 = 0;
    verified_total: u64 = 0;
    valid_total: u64 = 0;
    expired_total: u64 = 0;
    invalid_sig_total: u64 = 0;
    method_mismatch_total: u64 = 0;
    malformed_total: u64 = 0;
    unknown_kid_total: u64 = 0;
}

struct KeyringSlot {
    active: bool = false;
    is_primary: bool = false;
    kid_buf: [SIGNED_URL_MAX_KID_LEN]u8 = [0; SIGNED_URL_MAX_KID_LEN];
    kid_len: usize = 0;
    secret_buf: [SIGNED_URL_MAX_SECRET_LEN]u8 = [0; SIGNED_URL_MAX_SECRET_LEN];
    secret_len: usize = 0;

    fn kidSlice(): []const u8 {
        return kid_buf[0..kid_len];
    }

    fn secretSlice(): []const u8 {
        return secret_buf[0..secret_len];
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

        let r: usize = 0;
        while (r < 64) : (r += 1) {
            const S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
            const ch = (e & f) ^ ((~e) & g);
            const temp1 = hh +% S1 +% ch +% SHA256_K[r] +% w[r];
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

    fn updateByte(*_, b: u8) {
        block[block_len] = b;
        block_len += 1;
        total_bytes += 1;
        if (block_len == 64) {
            compress();
            block_len = 0;
        }
    }

    fn updateU32Be(*_, v: u32) {
        updateByte(@as(u8, @intCast((v >> 24) & 0xFF)));
        updateByte(@as(u8, @intCast((v >> 16) & 0xFF)));
        updateByte(@as(u8, @intCast((v >> 8) & 0xFF)));
        updateByte(@as(u8, @intCast(v & 0xFF)));
    }

    fn updateI64Be(*_, v: i64) {
        const u: u64 = @bitCast(v);
        updateByte(@as(u8, @intCast((u >> 56) & 0xFF)));
        updateByte(@as(u8, @intCast((u >> 48) & 0xFF)));
        updateByte(@as(u8, @intCast((u >> 40) & 0xFF)));
        updateByte(@as(u8, @intCast((u >> 32) & 0xFF)));
        updateByte(@as(u8, @intCast((u >> 24) & 0xFF)));
        updateByte(@as(u8, @intCast((u >> 16) & 0xFF)));
        updateByte(@as(u8, @intCast((u >> 8) & 0xFF)));
        updateByte(@as(u8, @intCast(u & 0xFF)));
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

const B64URL_ALPHABET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

/// Encode exact 32-byte HMAC-SHA256 digest into 43 unpadded Base64URL ASCII bytes.
fn encodeB64Url32(digest: [32]u8): [SIGNED_URL_SIG_LEN]u8 {
    let out: [SIGNED_URL_SIG_LEN]u8 = [0; SIGNED_URL_SIG_LEN];
    let in_i: usize = 0;
    let out_i: usize = 0;
    while (in_i < 30) : (in_i += 3) {
        const b0 = @as(u32, digest[in_i]);
        const b1 = @as(u32, digest[in_i + 1]);
        const b2 = @as(u32, digest[in_i + 2]);
        const v = (b0 << 16) | (b1 << 8) | b2;
        out[out_i] = B64URL_ALPHABET[@as(usize, (v >> 18) & 0x3F)];
        out[out_i + 1] = B64URL_ALPHABET[@as(usize, (v >> 12) & 0x3F)];
        out[out_i + 2] = B64URL_ALPHABET[@as(usize, (v >> 6) & 0x3F)];
        out[out_i + 3] = B64URL_ALPHABET[@as(usize, v & 0x3F)];
        out_i += 4;
    }
    // Final 2 bytes (digest[30], digest[31]) -> 3 Base64URL chars (16 bits -> 6 + 6 + 4)
    const b0 = @as(u32, digest[30]);
    const b1 = @as(u32, digest[31]);
    const v = (b0 << 16) | (b1 << 8);
    out[40] = B64URL_ALPHABET[@as(usize, (v >> 18) & 0x3F)];
    out[41] = B64URL_ALPHABET[@as(usize, (v >> 12) & 0x3F)];
    out[42] = B64URL_ALPHABET[@as(usize, (v >> 6) & 0x3F)];
    return out;
}

/// Constant-time byte-slice equality (never short-circuits on content mismatch).
pub fn signedUrlConstantTimeEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

/// Format a `SignedUrlStatus` enum variant as a canonical lowercase ASCII label.
pub fn formatSignedUrlStatus(status: SignedUrlStatus): []const u8 {
    if (status == SignedUrlStatus.valid) return "valid";
    if (status == SignedUrlStatus.missing_signature) return "missing_signature";
    if (status == SignedUrlStatus.missing_expiry) return "missing_expiry";
    if (status == SignedUrlStatus.expired) return "expired";
    if (status == SignedUrlStatus.invalid_signature) return "invalid_signature";
    if (status == SignedUrlStatus.method_mismatch) return "method_mismatch";
    return "malformed_url";
}

// ============================================================================
// 3. Input Hygiene, Canonical Query Stripping & Buffer Formatting Helpers
// ============================================================================

fn hasCtlOrCrlf(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < 0x20 || s[i] == 0x7F) return true;
    }
    return false;
}

fn isValidMethod(method: []const u8): bool {
    if (method.len == 0 || method.len > 16) return false;
    let i: usize = 0;
    while (i < method.len) : (i += 1) {
        const c = method[i];
        const is_upper = c >= 'A' && c <= 'Z';
        const is_lower = c >= 'a' && c <= 'z';
        if (!is_upper && !is_lower) return false;
    }
    return true;
}

fn toUpperAscii(c: u8): u8 {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

fn methodEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toUpperAscii(a[i]) != toUpperAscii(b[i])) return false;
    }
    return true;
}

fn isValidKid(kid: []const u8): bool {
    if (kid.len > SIGNED_URL_MAX_KID_LEN) return false;
    let i: usize = 0;
    while (i < kid.len) : (i += 1) {
        const c = kid[i];
        const ok = (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.';
        if (!ok) return false;
    }
    return true;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn isReservedParamName(name: []const u8): bool {
    return bytesEq(name, "sig") || bytesEq(name, "exp") || bytesEq(name, "kid");
}

fn isValidPath(path: []const u8): bool {
    if (path.len == 0 || path[0] != '/') return false;
    if (hasCtlOrCrlf(path)) return false;
    let i: usize = 0;
    while (i < path.len) : (i += 1) {
        const c = path[i];
        if (c == '?' || c == '#' || c == ' ') return false;
    }
    return true;
}

fn isValidUnsignedQuery(query: []const u8): bool {
    if (hasCtlOrCrlf(query)) return false;
    let i: usize = 0;
    while (i < query.len) : (i += 1) {
        const c = query[i];
        if (c == '?' || c == '#' || c == ' ') return false;
    }
    // Reject pre-existing reserved params `sig`, `exp`, `kid` to prevent query parameter pollution.
    if (query.len == 0) return true;
    let start: usize = 0;
    while (start <= query.len) {
        let end = start;
        while (end < query.len && query[end] != '&') : (end += 1) {}
        const seg = query[start..end];
        let eq_pos = seg.len;
        let k: usize = 0;
        while (k < seg.len) : (k += 1) {
            if (seg[k] == '=') {
                eq_pos = k;
                break;
            }
        }
        const key = seg[0..eq_pos];
        if (isReservedParamName(key)) return false;
        if (end >= query.len) break;
        start = end + 1;
    }
    return true;
}

fn appendBytes(out_buf: *[]u8, pos: *usize, s: []const u8): bool {
    if (pos + s.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos + i] = s[i];
    }
    pos += s.len;
    return true;
}

fn appendU64(out_buf: *[]u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) : (n += 1) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v = v / 10;
        }
    }
    if (pos + n > out_buf.len) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out_buf[pos + i] = tmp[n - 1 - i];
    }
    pos += n;
    return true;
}

fn parsePositiveI64(s: []const u8): i64? {
    if (s.len == 0 || s.len > 19) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const digit = @as(u64, c - '0');
        if (acc > (9223372036854775807 - digit) / 10) return null;
        acc = acc * 10 + digit;
    }
    if (acc == 0) return null;
    return @as(i64, @intCast(acc));
}

/// Compute length of canonical query string when `sig`, `exp`, and `kid` params are stripped.
fn strippedQueryLen(raw_query: []const u8): usize {
    if (raw_query.len == 0) return 0;
    let total: usize = 0;
    let count: usize = 0;
    let start: usize = 0;
    while (start <= raw_query.len) {
        let end = start;
        while (end < raw_query.len && raw_query[end] != '&') : (end += 1) {}
        const seg = raw_query[start..end];
        let eq_pos = seg.len;
        let k: usize = 0;
        while (k < seg.len) : (k += 1) {
            if (seg[k] == '=') {
                eq_pos = k;
                break;
            }
        }
        const key = seg[0..eq_pos];
        if (!isReservedParamName(key)) {
            if (count > 0) total += 1; // '&'
            total += seg.len;
            count += 1;
        }
        if (end >= raw_query.len) break;
        start = end + 1;
    }
    return total;
}

/// Compute domain-separated HMAC-SHA256 signature (43-char Base64URL) directly by streaming
/// canonical fields into SHA-256 without any heap allocation or URL length limit.
/// When `strip_reserved_from_query` is true, `sig`/`exp`/`kid` query segments are skipped on the fly.
fn computeSignedUrlMac(
    secret: []const u8,
    method: []const u8,
    path: []const u8,
    query: []const u8,
    strip_reserved_from_query: bool,
    exp_epoch_s: i64,
    key_id: []const u8,
): [SIGNED_URL_SIG_LEN]u8 {
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

    let inner = Sha256State{};
    inner.update(ipad[0..]);
    inner.update("cap-signed-url-v1\n");

    // 1. Canonical uppercase HTTP method
    inner.updateU32Be(@as(u32, @intCast(method.len)));
    let m_i: usize = 0;
    while (m_i < method.len) : (m_i += 1) {
        inner.updateByte(toUpperAscii(method[m_i]));
    }
    inner.updateByte('\n');

    // 2. Path
    inner.updateU32Be(@as(u32, @intCast(path.len)));
    inner.update(path);
    inner.updateByte('\n');

    // 3. Canonical query without `sig`/`exp`/`kid`
    if (!strip_reserved_from_query) {
        inner.updateU32Be(@as(u32, @intCast(query.len)));
        inner.update(query);
    } else {
        const q_len = strippedQueryLen(query);
        inner.updateU32Be(@as(u32, @intCast(q_len)));
        if (query.len > 0) {
            let count: usize = 0;
            let start: usize = 0;
            while (start <= query.len) {
                let end = start;
                while (end < query.len && query[end] != '&') : (end += 1) {}
                const seg = query[start..end];
                let eq_pos = seg.len;
                let k: usize = 0;
                while (k < seg.len) : (k += 1) {
                    if (seg[k] == '=') {
                        eq_pos = k;
                        break;
                    }
                }
                const key = seg[0..eq_pos];
                if (!isReservedParamName(key)) {
                    if (count > 0) inner.updateByte('&');
                    inner.update(seg);
                    count += 1;
                }
                if (end >= query.len) break;
                start = end + 1;
            }
        }
    }
    inner.updateByte('\n');

    // 4. Expiry epoch seconds (big-endian i64)
    inner.updateI64Be(exp_epoch_s);
    inner.updateByte('\n');

    // 5. Key identifier (`kid`)
    inner.updateU32Be(@as(u32, @intCast(key_id.len)));
    inner.update(key_id);

    const inner_hash = inner.finalize();

    let outer = Sha256State{};
    outer.update(opad[0..]);
    outer.update(inner_hash[0..]);
    const mac = outer.finalize();
    return encodeB64Url32(mac);
}

// ============================================================================
// 4. Stateless Pre-Signed URL Generation & Constant-Time Verification
// ============================================================================

/// Generate a domain-separated HMAC-SHA256 pre-signed URL into `out_buf`.
/// Appends `exp=<exp_epoch_s>`, optional `kid=<key_id>`, and `sig=<43B Base64URL>`.
/// Returns the number of bytes written, or `null` on invalid inputs or insufficient buffer.
pub fn signUrl(
    secret: []const u8,
    method: []const u8,
    path: []const u8,
    query_without_sig: []const u8,
    exp_epoch_s: i64,
    key_id: []const u8,
    out_buf: *[]u8,
): usize? {
    if (secret.len == 0) return null;
    if (!isValidMethod(method)) return null;
    if (!isValidPath(path)) return null;
    if (!isValidUnsignedQuery(query_without_sig)) return null;
    if (exp_epoch_s <= 0) return null;
    if (!isValidKid(key_id)) return null;

    const sig = computeSignedUrlMac(secret, method, path, query_without_sig, false, exp_epoch_s, key_id);

    let pos: usize = 0;
    if (!appendBytes(out_buf, *pos, path)) return null;
    if (!appendBytes(out_buf, *pos, "?")) return null;
    if (query_without_sig.len > 0) {
        if (!appendBytes(out_buf, *pos, query_without_sig)) return null;
        if (!appendBytes(out_buf, *pos, "&")) return null;
    }
    if (!appendBytes(out_buf, *pos, "exp=")) return null;
    if (!appendU64(out_buf, *pos, @as(u64, @intCast(exp_epoch_s)))) return null;
    if (key_id.len > 0) {
        if (!appendBytes(out_buf, *pos, "&kid=")) return null;
        if (!appendBytes(out_buf, *pos, key_id)) return null;
    }
    if (!appendBytes(out_buf, *pos, "&sig=")) return null;
    if (!appendBytes(out_buf, *pos, sig[0..])) return null;
    return pos;
}

struct ParsedUrlParts {
    ok: bool = false;
    path: []const u8 = "";
    raw_query: []const u8 = "";
    has_query: bool = false;
    has_sig: bool = false;
    sig: []const u8 = "";
    has_exp: bool = false;
    exp_raw: []const u8 = "";
    has_kid: bool = false;
    kid: []const u8 = "";
}

fn parseSignedUrlParts(full_path_and_query: []const u8): ParsedUrlParts {
    let p = ParsedUrlParts{};
    if (full_path_and_query.len == 0 || full_path_and_query[0] != '/') return p;
    if (hasCtlOrCrlf(full_path_and_query)) return p;

    let q_idx: usize = full_path_and_query.len;
    let i: usize = 0;
    while (i < full_path_and_query.len) : (i += 1) {
        const c = full_path_and_query[i];
        if (c == '#' || c == ' ') return p;
        if (c == '?') {
            if (q_idx == full_path_and_query.len) {
                q_idx = i;
            } else {
                // Multiple '?' in URL is malformed
                return p;
            }
        }
    }

    p.path = full_path_and_query[0..q_idx];
    if (p.path.len == 0) return p;

    if (q_idx == full_path_and_query.len) {
        p.ok = true;
        p.has_query = false;
        return p;
    }

    p.has_query = true;
    p.raw_query = full_path_and_query[q_idx + 1 ..];

    if (p.raw_query.len > 0) {
        let start: usize = 0;
        while (start <= p.raw_query.len) {
            let end = start;
            while (end < p.raw_query.len && p.raw_query[end] != '&') : (end += 1) {}
            const seg = p.raw_query[start..end];
            let eq_pos = seg.len;
            let k: usize = 0;
            while (k < seg.len) : (k += 1) {
                if (seg[k] == '=') {
                    eq_pos = k;
                    break;
                }
            }
            const key = seg[0..eq_pos];
            const val: []const u8 = if (eq_pos < seg.len) seg[eq_pos + 1 ..] else "";

            if (bytesEq(key, "sig")) {
                if (p.has_sig) return p; // duplicate sig
                p.has_sig = true;
                p.sig = val;
            } else if (bytesEq(key, "exp")) {
                if (p.has_exp) return p; // duplicate exp
                p.has_exp = true;
                p.exp_raw = val;
            } else if (bytesEq(key, "kid")) {
                if (p.has_kid) return p; // duplicate kid
                if (!isValidKid(val)) return p;
                p.has_kid = true;
                p.kid = val;
            }

            if (end >= p.raw_query.len) break;
            start = end + 1;
        }
    }

    p.ok = true;
    return p;
}

fn fillResultKid(res: *SignedUrlVerifyResult, kid: []const u8) {
    const copy_len = if (kid.len > SIGNED_URL_MAX_KID_LEN) SIGNED_URL_MAX_KID_LEN else kid.len;
    let i: usize = 0;
    while (i < copy_len) : (i += 1) {
        res.kid_buf[i] = kid[i];
    }
    res.kid_len = copy_len;
}

/// Verify a pre-signed URL in constant time against a single `secret`.
/// Automatically strips `sig`/`exp`/`kid`, validates expiry with `clock_skew_s`,
/// and distinguishes `method_mismatch` from `invalid_signature`.
pub fn verifySignedUrl(
    secret: []const u8,
    method: []const u8,
    full_path_and_query: []const u8,
    now_epoch_s: i64,
    clock_skew_s: i64,
): SignedUrlVerifyResult {
    let res = SignedUrlVerifyResult{};
    if (secret.len == 0 || !isValidMethod(method)) {
        res.status = SignedUrlStatus.malformed_url;
        return res;
    }

    const parts = parseSignedUrlParts(full_path_and_query);
    if (!parts.ok) {
        res.status = SignedUrlStatus.malformed_url;
        return res;
    }
    if (parts.has_kid) {
        fillResultKid(*res, parts.kid);
    }

    if (!parts.has_sig || parts.sig.len == 0) {
        res.status = SignedUrlStatus.missing_signature;
        return res;
    }
    if (!parts.has_exp || parts.exp_raw.len == 0) {
        res.status = SignedUrlStatus.missing_expiry;
        return res;
    }

    const exp_opt = parsePositiveI64(parts.exp_raw);
    if (exp_opt == null) {
        res.status = SignedUrlStatus.malformed_url;
        return res;
    }
    const exp_s = exp_opt ?? 0;
    res.exp_epoch_s = exp_s;

    if (parts.sig.len != SIGNED_URL_SIG_LEN) {
        res.status = SignedUrlStatus.invalid_signature;
        return res;
    }

    const expected = computeSignedUrlMac(secret, method, parts.path, parts.raw_query, true, exp_s, parts.kid);
    if (!signedUrlConstantTimeEq(expected[0..], parts.sig)) {
        // Check if the signature is valid for another standard HTTP method -> report exact method_mismatch.
        const std_methods: [7][]const u8 = ["GET", "HEAD", "POST", "PUT", "DELETE", "PATCH", "OPTIONS"];
        let m_i: usize = 0;
        while (m_i < 7) : (m_i += 1) {
            if (methodEqIgnoreCase(method, std_methods[m_i])) continue;
            const alt_mac = computeSignedUrlMac(secret, std_methods[m_i], parts.path, parts.raw_query, true, exp_s, parts.kid);
            if (signedUrlConstantTimeEq(alt_mac[0..], parts.sig)) {
                res.status = SignedUrlStatus.method_mismatch;
                return res;
            }
        }
        res.status = SignedUrlStatus.invalid_signature;
        return res;
    }

    const skew: i64 = if (clock_skew_s < 0) 0 else clock_skew_s;
    if (now_epoch_s > exp_s + skew) {
        res.status = SignedUrlStatus.expired;
        return res;
    }

    res.status = SignedUrlStatus.valid;
    res.valid = true;
    return res;
}

// ============================================================================
// 5. SignedUrlKeyring — Multi-Key Rotation Engine (Up to 8 Named Keys)
// ============================================================================

/// Zero-heap-allocation multi-key rotation store for pre-signed URLs (up to 8 named keys).
pub struct SignedUrlKeyring {
    slots: [SIGNED_URL_MAX_KEYS]KeyringSlot = [KeyringSlot{}; SIGNED_URL_MAX_KEYS];
    count: usize = 0;
    signed_total: u64 = 0;
    verified_total: u64 = 0;
    valid_total: u64 = 0;
    expired_total: u64 = 0;
    invalid_sig_total: u64 = 0;
    method_mismatch_total: u64 = 0;
    malformed_total: u64 = 0;
    unknown_kid_total: u64 = 0;

    /// Create an empty `SignedUrlKeyring`.
    static fn init(): SignedUrlKeyring {
        return SignedUrlKeyring{};
    }

    /// Return current number of active keys in the keyring.
    fn keyCount(): usize {
        return count;
    }

    fn findKeySlot(kid: []const u8): usize? {
        if (kid.len == 0) return null;
        let i: usize = 0;
        while (i < SIGNED_URL_MAX_KEYS) : (i += 1) {
            if (slots[i].active && bytesEq(slots[i].kidSlice(), kid)) {
                return i;
            }
        }
        return null;
    }

    fn findPrimarySlot(): usize? {
        let i: usize = 0;
        while (i < SIGNED_URL_MAX_KEYS) : (i += 1) {
            if (slots[i].active && slots[i].is_primary) {
                return i;
            }
        }
        return null;
    }

    /// Return true if a key with `kid` is active in the keyring.
    fn hasKey(kid: []const u8): bool {
        return findKeySlot(kid) != null;
    }

    /// Add or rotate (overwrite in-place) a named key.
    /// If `make_primary` is true (or if this is the first key), marks it as primary.
    /// Returns `false` if `kid`/`secret` is invalid or keyring is full (8 keys).
    fn addKey(*_, kid: []const u8, secret: []const u8, make_primary: bool): bool {
        if (kid.len == 0 || !isValidKid(kid)) return false;
        if (secret.len == 0 || secret.len > SIGNED_URL_MAX_SECRET_LEN) return false;

        let target_idx: usize = SIGNED_URL_MAX_KEYS;
        let is_update = false;
        if (existing := findKeySlot(kid)) {
            target_idx = existing;
            is_update = true;
        } else {
            let i: usize = 0;
            while (i < SIGNED_URL_MAX_KEYS) : (i += 1) {
                if (!slots[i].active) {
                    target_idx = i;
                    break;
                }
            }
        }
        if (target_idx == SIGNED_URL_MAX_KEYS) return false;

        const should_be_primary = make_primary || count == 0 || (is_update && slots[target_idx].is_primary);
        if (should_be_primary) {
            let j: usize = 0;
            while (j < SIGNED_URL_MAX_KEYS) : (j += 1) {
                slots[j].is_primary = false;
            }
        }

        let s = KeyringSlot{};
        s.active = true;
        s.is_primary = should_be_primary;
        let k: usize = 0;
        while (k < kid.len) : (k += 1) {
            s.kid_buf[k] = kid[k];
        }
        s.kid_len = kid.len;
        let m: usize = 0;
        while (m < secret.len) : (m += 1) {
            s.secret_buf[m] = secret[m];
        }
        s.secret_len = secret.len;

        slots[target_idx] = s;
        if (!is_update) {
            count += 1;
        }
        return true;
    }

    /// Promote an existing active key to primary signing key.
    fn setPrimaryKey(*_, kid: []const u8): bool {
        const idx = findKeySlot(kid) ?? return false;
        let i: usize = 0;
        while (i < SIGNED_URL_MAX_KEYS) : (i += 1) {
            slots[i].is_primary = false;
        }
        slots[idx].is_primary = true;
        return true;
    }

    /// Revoke and wipe a key by `kid`. If it was primary, promotes the first remaining active key.
    fn revokeKey(*_, kid: []const u8): bool {
        const idx = findKeySlot(kid) ?? return false;
        const was_primary = slots[idx].is_primary;
        slots[idx] = KeyringSlot{};
        if (count > 0) count -= 1;

        if (was_primary && count > 0) {
            let i: usize = 0;
            while (i < SIGNED_URL_MAX_KEYS) : (i += 1) {
                if (slots[i].active) {
                    slots[i].is_primary = true;
                    break;
                }
            }
        }
        return true;
    }

    /// Sign a URL using the current primary key (automatically embedding its `kid`).
    fn signWithPrimary(
        *_,
        method: []const u8,
        path: []const u8,
        query_without_sig: []const u8,
        exp_epoch_s: i64,
        out_buf: *[]u8,
    ): usize? {
        const idx = findPrimarySlot() ?? return null;
        const written = signUrl(
            slots[idx].secretSlice(),
            method,
            path,
            query_without_sig,
            exp_epoch_s,
            slots[idx].kidSlice(),
            out_buf,
        ) ?? return null;
        signed_total += 1;
        return written;
    }

    /// Sign a URL using a specific named key `kid`.
    fn signWithKey(
        *_,
        kid: []const u8,
        method: []const u8,
        path: []const u8,
        query_without_sig: []const u8,
        exp_epoch_s: i64,
        out_buf: *[]u8,
    ): usize? {
        const idx = findKeySlot(kid) ?? return null;
        const written = signUrl(
            slots[idx].secretSlice(),
            method,
            path,
            query_without_sig,
            exp_epoch_s,
            slots[idx].kidSlice(),
            out_buf,
        ) ?? return null;
        signed_total += 1;
        return written;
    }

    /// Verify a pre-signed URL by resolving its `kid` in the keyring (or falling back to
    /// the primary key when `kid` is absent). Updates keyring telemetry counters.
    fn verify(
        *_,
        method: []const u8,
        full_path_and_query: []const u8,
        now_epoch_s: i64,
        clock_skew_s: i64,
    ): SignedUrlVerifyResult {
        verified_total += 1;
        let res = SignedUrlVerifyResult{};

        if (!isValidMethod(method)) {
            res.status = SignedUrlStatus.malformed_url;
            malformed_total += 1;
            return res;
        }

        const parts = parseSignedUrlParts(full_path_and_query);
        if (!parts.ok) {
            res.status = SignedUrlStatus.malformed_url;
            malformed_total += 1;
            return res;
        }
        if (parts.has_kid) {
            fillResultKid(*res, parts.kid);
        }
        if (!parts.has_sig || parts.sig.len == 0) {
            res.status = SignedUrlStatus.missing_signature;
            return res;
        }
        if (!parts.has_exp || parts.exp_raw.len == 0) {
            res.status = SignedUrlStatus.missing_expiry;
            return res;
        }

        const key_idx_opt: usize? = if (parts.has_kid && parts.kid.len > 0)
            findKeySlot(parts.kid)
        else
            findPrimarySlot();

        if (key_idx := key_idx_opt) {
            const vres = verifySignedUrl(
                slots[key_idx].secretSlice(),
                method,
                full_path_and_query,
                now_epoch_s,
                clock_skew_s,
            );
            if (vres.status == SignedUrlStatus.valid) {
                valid_total += 1;
            } else if (vres.status == SignedUrlStatus.expired) {
                expired_total += 1;
            } else if (vres.status == SignedUrlStatus.invalid_signature) {
                invalid_sig_total += 1;
            } else if (vres.status == SignedUrlStatus.method_mismatch) {
                method_mismatch_total += 1;
            } else if (vres.status == SignedUrlStatus.malformed_url) {
                malformed_total += 1;
            }
            return vres;
        }

        unknown_kid_total += 1;
        invalid_sig_total += 1;
        res.status = SignedUrlStatus.invalid_signature;
        return res;
    }

    /// Return a zero-heap global telemetry snapshot.
    fn stats(): SignedUrlKeyringStats {
        return SignedUrlKeyringStats{
            key_count: count,
            capacity: SIGNED_URL_MAX_KEYS,
            signed_total: signed_total,
            verified_total: verified_total,
            valid_total: valid_total,
            expired_total: expired_total,
            invalid_sig_total: invalid_sig_total,
            method_mismatch_total: method_mismatch_total,
            malformed_total: malformed_total,
            unknown_kid_total: unknown_kid_total,
        };
    }

    /// Reset telemetry counters while keeping active keys intact.
    fn resetStats(*_) {
        signed_total = 0;
        verified_total = 0;
        valid_total = 0;
        expired_total = 0;
        invalid_sig_total = 0;
        method_mismatch_total = 0;
        malformed_total = 0;
        unknown_kid_total = 0;
    }
}
