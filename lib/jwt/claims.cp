// Reference-hardened against Keats/jsonwebtoken (MIT)
// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (RFC 7519 JWT Structure & Claims Inspector)
// Pure-CP zero-allocation JWT/JWS (RFC 7515/7519) signer, verifier, Base64URL codec,
// alg:"none" / algorithm-confusion defense, and 16-lane SIMD constant-time primitives.

const std = @import("std");
import { Simd } from "cap:simd";

type Vec16u8 = @Vector(16, u8);

pub enum JwtAlgorithm {
    HS256,
    HS384,
    HS512,
}

pub err JwtError {
    InvalidToken,
    InvalidBase64Url,
    InvalidHeader,
    NoneAlgorithmForbidden,
    AlgorithmMismatch,
    InvalidSignature,
    TokenExpired,
    TokenNotYetValid,
    IssuerMismatch,
    SubjectMismatch,
    AudienceMismatch,
    BufferTooSmall,
}

pub struct JwtValidation {
    expected_alg: JwtAlgorithm = JwtAlgorithm.HS256,
    allow_none_alg: bool = false,
    leeway_sec: i64 = 60,
    validate_exp: bool = true,
    validate_nbf: bool = true,
    expected_iss: ([]const u8)? = null,
    expected_sub: ([]const u8)? = null,
    expected_aud: ([]const u8)? = null,
}

pub struct JwtVerified {
    header: JwtHeader,
    claims: JwtClaims,
    header_json: []const u8,
    payload_json: []const u8,
}

const B64URL_ALPHA: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

/// Scalar reference: constant-time byte slice equality.
pub fn jwtCtEqualScalar(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

/// 16-lane SIMD accelerated: constant-time byte slice equality (no early exit on mismatch).
pub fn jwtCtEqualSimd(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let acc: Vec16u8 = @splat(0);
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: Vec16u8 = a[i .. i + 16][0..16].*;
        const vb: Vec16u8 = b[i .. i + 16][0..16].*;
        acc |= (va ^ vb);
        i += 16;
    }
    let tail: u8 = 0;
    while (i < a.len) : (i += 1) {
        tail |= a[i] ^ b[i];
    }
    return (@reduce(.Or, acc) | tail) == 0;
}

/// Scalar reference: find next '.' separator in JWT token.
pub fn jwtFindDotScalar(slice: []const u8): usize? {
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        if (slice[i] == '.') return i;
    }
    return null;
}

/// cap:simd accelerated: find next '.' separator in JWT token.
pub fn jwtFindDotSimd(slice: []const u8): usize? {
    return Simd.indexOfByte(slice, '.');
}

/// Scalar reference: check unpadded Base64URL alphabet (`A-Za-z0-9-_`).
pub fn jwtIsValidBase64UrlScalar(slice: []const u8): bool {
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        if (b64UrlVal(slice[i]) == null) return false;
    }
    return true;
}

/// 16-lane SIMD accelerated: check unpadded Base64URL alphabet (`A-Za-z0-9-_`, rejects `+`, `/`, `=`).
pub fn jwtIsValidBase64UrlSimd(slice: []const u8): bool {
    const lo_A: Vec16u8 = @splat('A');
    const hi_Z: Vec16u8 = @splat('Z');
    const lo_a: Vec16u8 = @splat('a');
    const hi_z: Vec16u8 = @splat('z');
    const lo_0: Vec16u8 = @splat('0');
    const hi_9: Vec16u8 = @splat('9');
    const v_dash: Vec16u8 = @splat('-');
    const v_under: Vec16u8 = @splat('_');

    let i: usize = 0;
    while (i + 16 <= slice.len) {
        const v: Vec16u8 = slice[i .. i + 16][0..16].*;
        const ok = ((v >= lo_A) & (v <= hi_Z))
            | ((v >= lo_a) & (v <= hi_z))
            | ((v >= lo_0) & (v <= hi_9))
            | (v == v_dash)
            | (v == v_under);
        const mask: u16 = @bitCast(ok);
        if (mask != 0xFFFF) return false;
        i += 16;
    }
    while (i < slice.len) : (i += 1) {
        if (b64UrlVal(slice[i]) == null) return false;
    }
    return true;
}

fn b64UrlVal(c: u8): u8? {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return null;
}

/// Decode unpadded (or padded) Base64URL string into `out`. Returns decoded slice or null on error.
pub fn decodeBase64Url(input: []const u8, out: []u8): ([]const u8)? {
    // Strip optional trailing '=' padding
    let len = input.len;
    while (len > 0 && input[len - 1] == '=') {
        len -= 1;
    }
    const rem = len % 4;
    if (rem == 1) return null;

    const full_groups = len / 4;
    let out_len = full_groups * 3;
    if (rem == 2) out_len += 1;
    if (rem == 3) out_len += 2;
    if (out_len > out.len) return null;

    let in_i: usize = 0;
    let out_i: usize = 0;
    while (in_i + 4 <= len) {
        const a = b64UrlVal(input[in_i]) ?? return null;
        const b = b64UrlVal(input[in_i + 1]) ?? return null;
        const c = b64UrlVal(input[in_i + 2]) ?? return null;
        const d = b64UrlVal(input[in_i + 3]) ?? return null;
        out[out_i] = (a << 2) | (b >> 4);
        out[out_i + 1] = ((b & 0x0F) << 4) | (c >> 2);
        out[out_i + 2] = ((c & 0x03) << 6) | d;
        in_i += 4;
        out_i += 3;
    }

    if (rem == 2) {
        const a = b64UrlVal(input[in_i]) ?? return null;
        const b = b64UrlVal(input[in_i + 1]) ?? return null;
        out[out_i] = (a << 2) | (b >> 4);
        out_i += 1;
    } else if (rem == 3) {
        const a = b64UrlVal(input[in_i]) ?? return null;
        const b = b64UrlVal(input[in_i + 1]) ?? return null;
        const c = b64UrlVal(input[in_i + 2]) ?? return null;
        out[out_i] = (a << 2) | (b >> 4);
        out[out_i + 1] = ((b & 0x0F) << 4) | (c >> 2);
        out_i += 2;
    }

    return out[0..out_i];
}

export struct JwtParts {
    header_b64: []const u8,
    payload_b64: []const u8,
    signature_b64: []const u8,
    signing_input: []const u8,
}

/// Split a compact JWS/JWT `header.payload.signature` into its 3 Base64URL segments.
pub fn splitJwt(token: []const u8): JwtParts? {
    let dot1: usize? = null;
    let dot2: usize? = null;
    let i: usize = 0;
    while (i < token.len) {
        if (token[i] == '.') {
            if (dot1 == null) {
                dot1 = i;
            } else if (dot2 == null) {
                dot2 = i;
            } else {
                return null; // >2 dots invalid for compact JWS
            }
        }
        i += 1;
    }
    const d1 = dot1 ?? return null;
    const d2 = dot2 ?? return null;
    if (d1 == 0 || d2 == d1 + 1) return null;
    return JwtParts {
        header_b64: token[0..d1],
        payload_b64: token[d1 + 1..d2],
        signature_b64: token[d2 + 1..],
        signing_input: token[0..d2],
    };
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// Skip a JSON value at `pos` (handles nested objects, arrays, strings, numbers, literals).
fn skipJsonValue(json: []const u8, pos: *usize): bool {
    while (pos.* < json.len && isWs(json[pos.*])) {
        pos.* += 1;
    }
    if (pos.* >= json.len) return false;
    const c = json[pos.*];
    if (c == '"') {
        pos.* += 1;
        while (pos.* < json.len) {
            const ch = json[pos.*];
            pos.* += 1;
            if (ch == '\\') {
                if (pos.* >= json.len) return false;
                pos.* += 1;
            } else if (ch == '"') {
                return true;
            }
        }
        return false;
    }
    if (c == '{' || c == '[') {
        let depth: usize = 0;
        let in_str = false;
        while (pos.* < json.len) {
            const ch = json[pos.*];
            pos.* += 1;
            if (in_str) {
                if (ch == '\\') {
                    if (pos.* >= json.len) return false;
                    pos.* += 1;
                } else if (ch == '"') {
                    in_str = false;
                }
            } else {
                if (ch == '"') {
                    in_str = true;
                } else if (ch == '{' || ch == '[') {
                    depth += 1;
                } else if (ch == '}' || ch == ']') {
                    if (depth == 0) return false;
                    depth -= 1;
                    if (depth == 0) return true;
                }
            }
        }
        return false;
    }
    while (pos.* < json.len && json[pos.*] != ',' && json[pos.*] != '}' && json[pos.*] != ']' && !isWs(json[pos.*])) {
        pos.* += 1;
    }
    return true;
}

/// Extract a top-level string claim value slice (raw JSON string contents) from a decoded JSON object.
pub fn extractStringClaim(json: []const u8, key: []const u8): ([]const u8)? {
    let pos: usize = 0;
    while (pos < json.len && isWs(json[pos])) pos += 1;
    if (pos >= json.len || json[pos] != '{') return null;
    pos += 1;

    while (pos < json.len) {
        while (pos < json.len && (isWs(json[pos]) || json[pos] == ',')) pos += 1;
        if (pos >= json.len || json[pos] == '}') return null;
        if (json[pos] != '"') return null;
        pos += 1;
        const kstart = pos;
        while (pos < json.len && json[pos] != '"') {
            if (json[pos] == '\\') pos += 1;
            pos += 1;
        }
        if (pos >= json.len) return null;
        const k = json[kstart..pos];
        pos += 1;

        while (pos < json.len && isWs(json[pos])) pos += 1;
        if (pos >= json.len || json[pos] != ':') return null;
        pos += 1;
        while (pos < json.len && isWs(json[pos])) pos += 1;

        if (strEq(k, key)) {
            if (pos >= json.len || json[pos] != '"') return null;
            pos += 1;
            const vstart = pos;
            while (pos < json.len && json[pos] != '"') {
                if (json[pos] == '\\') pos += 1;
                pos += 1;
            }
            if (pos >= json.len) return null;
            return json[vstart..pos];
        }
        if (!skipJsonValue(json, &pos)) return null;
    }
    return null;
}

/// Extract a top-level integer claim (e.g. `exp`, `nbf`, `iat`) as `i64` from a decoded JSON object.
pub fn extractIntClaim(json: []const u8, key: []const u8): i64? {
    let pos: usize = 0;
    while (pos < json.len && isWs(json[pos])) pos += 1;
    if (pos >= json.len || json[pos] != '{') return null;
    pos += 1;

    while (pos < json.len) {
        while (pos < json.len && (isWs(json[pos]) || json[pos] == ',')) pos += 1;
        if (pos >= json.len || json[pos] == '}') return null;
        if (json[pos] != '"') return null;
        pos += 1;
        const kstart = pos;
        while (pos < json.len && json[pos] != '"') {
            if (json[pos] == '\\') pos += 1;
            pos += 1;
        }
        if (pos >= json.len) return null;
        const k = json[kstart..pos];
        pos += 1;

        while (pos < json.len && isWs(json[pos])) pos += 1;
        if (pos >= json.len || json[pos] != ':') return null;
        pos += 1;
        while (pos < json.len && isWs(json[pos])) pos += 1;

        if (strEq(k, key)) {
            if (pos >= json.len) return null;
            let neg = false;
            if (json[pos] == '-') {
                neg = true;
                pos += 1;
            }
            const dstart = pos;
            let val: i64 = 0;
            while (pos < json.len && json[pos] >= '0' && json[pos] <= '9') {
                const digit: i64 = @as(i64, json[pos] - '0');
                if (val > 922337203685477580) return null;
                val = val * 10 + digit;
                pos += 1;
            }
            if (pos == dstart) return null;
            if (neg) return 0 - val;
            return val;
        }
        if (!skipJsonValue(json, &pos)) return null;
    }
    return null;
}

export struct JwtHeader {
    alg: ([]const u8)?,
    typ: ([]const u8)?,
    kid: ([]const u8)?,

    pub static fn parseJson(header_json: []const u8): JwtHeader {
        return JwtHeader {
            alg: extractStringClaim(header_json, "alg"),
            typ: extractStringClaim(header_json, "typ"),
            kid: extractStringClaim(header_json, "kid"),
        };
    }

    pub fn isNoneAlg(): bool {
        if (a := _.alg) {
            if (a.len == 4) {
                const c0 = if (a[0] >= 'A' && a[0] <= 'Z') a[0] + 32 else a[0];
                const c1 = if (a[1] >= 'A' && a[1] <= 'Z') a[1] + 32 else a[1];
                const c2 = if (a[2] >= 'A' && a[2] <= 'Z') a[2] + 32 else a[2];
                const c3 = if (a[3] >= 'A' && a[3] <= 'Z') a[3] + 32 else a[3];
                return c0 == 'n' && c1 == 'o' && c2 == 'n' && c3 == 'e';
            }
        }
        return false;
    }
}

export enum ClaimStatus {
    valid,
    expired,
    not_yet_valid,
    issuer_mismatch,
    subject_mismatch,
    audience_mismatch,
}

export struct JwtClaims {
    iss: ([]const u8)?,
    sub: ([]const u8)?,
    aud: ([]const u8)?,
    jti: ([]const u8)?,
    exp: i64?,
    nbf: i64?,
    iat: i64?,

    pub static fn parseJson(payload_json: []const u8): JwtClaims {
        return JwtClaims {
            iss: extractStringClaim(payload_json, "iss"),
            sub: extractStringClaim(payload_json, "sub"),
            aud: extractStringClaim(payload_json, "aud"),
            jti: extractStringClaim(payload_json, "jti"),
            exp: extractIntClaim(payload_json, "exp"),
            nbf: extractIntClaim(payload_json, "nbf"),
            iat: extractIntClaim(payload_json, "iat"),
        };
    }

    /// Validate `exp` and `nbf` against `now_unix_sec` with `leeway_sec` clock skew tolerance.
    pub fn validateTime(now_unix_sec: i64, leeway_sec: i64): ClaimStatus {
        if (e := _.exp) {
            if (now_unix_sec >= e + leeway_sec) return ClaimStatus.expired;
        }
        if (n := _.nbf) {
            if (now_unix_sec + leeway_sec < n) return ClaimStatus.not_yet_valid;
        }
        return ClaimStatus.valid;
    }

    /// Validate time window plus optional expected `iss`, `sub`, `aud`.
    pub fn validate(
        now_unix_sec: i64,
        leeway_sec: i64,
        expected_iss: ([]const u8)?,
        expected_sub: ([]const u8)?,
        expected_aud: ([]const u8)?,
    ): ClaimStatus {
        const ts = _.validateTime(now_unix_sec, leeway_sec);
        if (ts != ClaimStatus.valid) return ts;
        if (want_iss := expected_iss) {
            const got = _.iss ?? return ClaimStatus.issuer_mismatch;
            if (!strEq(got, want_iss)) return ClaimStatus.issuer_mismatch;
        }
        if (want_sub := expected_sub) {
            const got = _.sub ?? return ClaimStatus.subject_mismatch;
            if (!strEq(got, want_sub)) return ClaimStatus.subject_mismatch;
        }
        if (want_aud := expected_aud) {
            const got = _.aud ?? return ClaimStatus.audience_mismatch;
            if (!strEq(got, want_aud)) return ClaimStatus.audience_mismatch;
        }
        return ClaimStatus.valid;
    }
}

/// Unpadded RFC 4648 §5 Base64URL encoder into caller buffer `out`.
pub fn encodeBase64Url(input: []const u8, out: []u8): ([]const u8)? {
    const full = input.len / 3;
    const rem = input.len % 3;
    const needed = full * 4 + (if (rem == 0) (0 as usize) else (rem + 1));
    if (needed > out.len) return null;

    let i: usize = 0;
    let o: usize = 0;
    while (i + 3 <= input.len) {
        const b0 = input[i] as u32;
        const b1 = input[i + 1] as u32;
        const b2 = input[i + 2] as u32;
        const t = (b0 << 16) | (b1 << 8) | b2;
        out[o] = B64URL_ALPHA[((t >> 18) & 0x3F) as usize];
        out[o + 1] = B64URL_ALPHA[((t >> 12) & 0x3F) as usize];
        out[o + 2] = B64URL_ALPHA[((t >> 6) & 0x3F) as usize];
        out[o + 3] = B64URL_ALPHA[(t & 0x3F) as usize];
        i += 3;
        o += 4;
    }
    if (rem == 1) {
        const b0 = input[i] as u32;
        out[o] = B64URL_ALPHA[((b0 >> 2) & 0x3F) as usize];
        out[o + 1] = B64URL_ALPHA[((b0 & 0x03) << 4) as usize];
        o += 2;
    } else if (rem == 2) {
        const b0 = input[i] as u32;
        const b1 = input[i + 1] as u32;
        out[o] = B64URL_ALPHA[((b0 >> 2) & 0x3F) as usize];
        out[o + 1] = B64URL_ALPHA[(((b0 & 0x03) << 4) | (b1 >> 4)) as usize];
        out[o + 2] = B64URL_ALPHA[((b1 & 0x0F) << 2) as usize];
        o += 3;
    }
    return out[0..o];
}

/// Strict unpadded Base64URL decoder (rejects `=`, `+`, `/`, and non-zero canonical trailing bits).
pub fn decodeBase64UrlStrict(input: []const u8, out: []u8): ([]const u8)? {
    if (!jwtIsValidBase64UrlSimd(input)) return null;
    const rem = input.len % 4;
    if (rem == 1) return null;
    const out_len = (input.len / 4) * 3 + (if (rem == 2) (1 as usize) else if (rem == 3) (2 as usize) else (0 as usize));
    if (out_len > out.len) return null;

    let in_i: usize = 0;
    let out_i: usize = 0;
    while (in_i + 4 <= input.len) {
        const a = b64UrlVal(input[in_i]) ?? return null;
        const b = b64UrlVal(input[in_i + 1]) ?? return null;
        const c = b64UrlVal(input[in_i + 2]) ?? return null;
        const d = b64UrlVal(input[in_i + 3]) ?? return null;
        out[out_i] = (a << 2) | (b >> 4);
        out[out_i + 1] = ((b & 0x0F) << 4) | (c >> 2);
        out[out_i + 2] = ((c & 0x03) << 6) | d;
        in_i += 4;
        out_i += 3;
    }
    if (rem == 2) {
        const a = b64UrlVal(input[in_i]) ?? return null;
        const b = b64UrlVal(input[in_i + 1]) ?? return null;
        if ((b & 0x0F) != 0) return null;
        out[out_i] = (a << 2) | (b >> 4);
        out_i += 1;
    } else if (rem == 3) {
        const a = b64UrlVal(input[in_i]) ?? return null;
        const b = b64UrlVal(input[in_i + 1]) ?? return null;
        const c = b64UrlVal(input[in_i + 2]) ?? return null;
        if ((c & 0x03) != 0) return null;
        out[out_i] = (a << 2) | (b >> 4);
        out[out_i + 1] = ((b & 0x0F) << 4) | (c >> 2);
        out_i += 2;
    }
    return out[0..out_i];
}

/// Strict 3-part JWS splitter using `cap:simd` dot search and Base64URL character pre-check.
pub fn splitJwtStrict(token: []const u8): JwtParts? {
    const d1 = jwtFindDotSimd(token) ?? return null;
    if (d1 == 0) return null;
    const rest1 = token[d1 + 1..];
    const d2_rel = jwtFindDotSimd(rest1) ?? return null;
    if (d2_rel == 0) return null;
    const d2 = d1 + 1 + d2_rel;
    if (jwtFindDotSimd(token[d2 + 1..]) != null) return null;

    const h = token[0..d1];
    const p = token[d1 + 1..d2];
    const s = token[d2 + 1..];
    if (!jwtIsValidBase64UrlSimd(h) || !jwtIsValidBase64UrlSimd(p) || !jwtIsValidBase64UrlSimd(s)) {
        return null;
    }
    return JwtParts{
        header_b64: h,
        payload_b64: p,
        signature_b64: s,
        signing_input: token[0..d2],
    };
}

fn algName(alg: JwtAlgorithm): []const u8 {
    return switch (alg) {
        .HS256 => "HS256",
        .HS384 => "HS384",
        .HS512 => "HS512",
    };
}

/// Zero-heap signs a compact JWS/JWT (`header.payload.sig`) using HS256 / HS384 / HS512.
pub fn signJwtHmac(
    alg: JwtAlgorithm,
    header_json: []const u8,
    payload_json: []const u8,
    secret: []const u8,
    out: []u8,
): ([]const u8) ! JwtError {
    const h_b64 = encodeBase64Url(header_json, out) orelse return JwtError.BufferTooSmall;
    let pos = h_b64.len;
    if (pos + 1 > out.len) return JwtError.BufferTooSmall;
    out[pos] = '.';
    pos += 1;
    const p_b64 = encodeBase64Url(payload_json, out[pos..]) orelse return JwtError.BufferTooSmall;
    pos += p_b64.len;
    const signing_input = out[0..pos];
    if (pos + 1 > out.len) return JwtError.BufferTooSmall;
    out[pos] = '.';
    pos += 1;

    let mac_buf: [64]u8 = [0; 64];
    let mac_len: usize = 0;
    switch (alg) {
        .HS256 => {
            let tag: [32]u8 = [0; 32];
            std.crypto.auth.hmac.sha2.HmacSha256.create(&tag, signing_input, secret);
            @memcpy(mac_buf[0..32], &tag);
            mac_len = 32;
        },
        .HS384 => {
            let tag: [48]u8 = [0; 48];
            std.crypto.auth.hmac.sha2.HmacSha384.create(&tag, signing_input, secret);
            @memcpy(mac_buf[0..48], &tag);
            mac_len = 48;
        },
        .HS512 => {
            let tag: [64]u8 = [0; 64];
            std.crypto.auth.hmac.sha2.HmacSha512.create(&tag, signing_input, secret);
            @memcpy(mac_buf[0..64], &tag);
            mac_len = 64;
        },
    }
    const sig_b64 = encodeBase64Url(mac_buf[0..mac_len], out[pos..]) orelse return JwtError.BufferTooSmall;
    return out[0 .. pos + sig_b64.len];
}

/// Zero-heap verifies a compact JWS/JWT against `Keats/jsonwebtoken` security rules:
/// rejects `alg: "none"` by default, enforces explicit `expected_alg`, constant-time SIMD HMAC check,
/// and validates `exp`/`nbf`/`iss`/`sub`/`aud` claims.
pub fn verifyJwtHmac(
    token: []const u8,
    secret: []const u8,
    now_unix_sec: i64,
    cfg: JwtValidation,
    header_buf: []u8,
    payload_buf: []u8,
): JwtVerified ! JwtError {
    const parts = splitJwtStrict(token) orelse return JwtError.InvalidToken;
    const h_json = decodeBase64UrlStrict(parts.header_b64, header_buf) orelse return JwtError.InvalidBase64Url;
    const p_json = decodeBase64UrlStrict(parts.payload_b64, payload_buf) orelse return JwtError.InvalidBase64Url;

    const hdr = (JwtHeader { alg: null, typ: null, kid: null }).parseJson(h_json);
    if (hdr.isNoneAlg()) {
        if (!cfg.allow_none_alg) return JwtError.NoneAlgorithmForbidden;
    } else {
        const got_alg = hdr.alg orelse return JwtError.InvalidHeader;
        if (!strEq(got_alg, algName(cfg.expected_alg))) {
            return JwtError.AlgorithmMismatch;
        }
        let sig_raw_buf: [64]u8 = [0; 64];
        const sig_raw = decodeBase64UrlStrict(parts.signature_b64, &sig_raw_buf) orelse return JwtError.InvalidSignature;

        let expected_buf: [64]u8 = [0; 64];
        let expected_len: usize = 0;
        switch (cfg.expected_alg) {
            .HS256 => {
                let tag: [32]u8 = [0; 32];
                std.crypto.auth.hmac.sha2.HmacSha256.create(&tag, parts.signing_input, secret);
                @memcpy(expected_buf[0..32], &tag);
                expected_len = 32;
            },
            .HS384 => {
                let tag: [48]u8 = [0; 48];
                std.crypto.auth.hmac.sha2.HmacSha384.create(&tag, parts.signing_input, secret);
                @memcpy(expected_buf[0..48], &tag);
                expected_len = 48;
            },
            .HS512 => {
                let tag: [64]u8 = [0; 64];
                std.crypto.auth.hmac.sha2.HmacSha512.create(&tag, parts.signing_input, secret);
                @memcpy(expected_buf[0..64], &tag);
                expected_len = 64;
            },
        }
        if (!jwtCtEqualSimd(expected_buf[0..expected_len], sig_raw)) {
            return JwtError.InvalidSignature;
        }
    }

    const claims = (JwtClaims { iss: null, sub: null, aud: null, jti: null, exp: null, nbf: null, iat: null }).parseJson(p_json);
    if (cfg.validate_exp) {
        if (e := claims.exp) {
            if (now_unix_sec >= e + cfg.leeway_sec) return JwtError.TokenExpired;
        }
    }
    if (cfg.validate_nbf) {
        if (n := claims.nbf) {
            if (now_unix_sec + cfg.leeway_sec < n) return JwtError.TokenNotYetValid;
        }
    }
    if (want_iss := cfg.expected_iss) {
        const got = claims.iss orelse return JwtError.IssuerMismatch;
        if (!strEq(got, want_iss)) return JwtError.IssuerMismatch;
    }
    if (want_sub := cfg.expected_sub) {
        const got = claims.sub orelse return JwtError.SubjectMismatch;
        if (!strEq(got, want_sub)) return JwtError.SubjectMismatch;
    }
    if (want_aud := cfg.expected_aud) {
        const got = claims.aud orelse return JwtError.AudienceMismatch;
        if (!strEq(got, want_aud)) return JwtError.AudienceMismatch;
    }

    return JwtVerified{
        .header = hdr,
        .claims = claims,
        .header_json = h_json,
        .payload_json = p_json,
    };
}
