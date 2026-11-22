// Source: Absorbed from /data/dev/bun/src/http/Headers.cp | Synced: 2026-11-22T11:12:13Z (RFC 9110 §11 / RFC 7617 Basic / RFC 6750 Bearer)
// Pure-CP zero-allocation HTTP `Authorization` & `WWW-Authenticate` parser/encoder
// supporting Basic (Base64 user:pass), Bearer tokens, and auth scheme extraction.

pub enum AuthScheme: u8 {
    unknown = 0,
    basic = 1,
    bearer = 2,
    digest = 3,
    aws4_hmac_sha256 = 4,

    pub static fn parse(s: []const u8): AuthScheme {
        if (eqIgnoreCase(s, "Basic")) return .basic;
        if (eqIgnoreCase(s, "Bearer")) return .bearer;
        if (eqIgnoreCase(s, "Digest")) return .digest;
        if (eqIgnoreCase(s, "AWS4-HMAC-SHA256")) return .aws4_hmac_sha256;
        return .unknown;
    }

    pub fn asStr(): []const u8 {
        return switch (_) {
            .unknown => "",
            .basic => "Basic",
            .bearer => "Bearer",
            .digest => "Digest",
            .aws4_hmac_sha256 => "AWS4-HMAC-SHA256",
        };
    }
}

pub struct AuthHeader {
    scheme: AuthScheme,
    scheme_raw: []const u8,
    credentials: []const u8,

    /// Parse an HTTP `Authorization` header (`<scheme> <credentials>`).
    pub static fn parse(header: []const u8): AuthHeader? {
        const s = trimOws(header);
        if (s.len == 0) return null;
        let sp: usize = 0;
        while (sp < s.len && !isOws(s[sp])) : (sp += 1) {}
        if (sp == 0 || sp >= s.len) return null;
        const scheme_raw = s[0..sp];
        const creds = trimOws(s[sp + 1 ..]);
        if (creds.len == 0) return null;
        return AuthHeader{
            scheme: AuthScheme.parse(scheme_raw),
            scheme_raw: scheme_raw,
            credentials: creds,
        };
    }

    /// Return Bearer token slice if scheme is `Bearer`, else null.
    pub fn bearerToken(): ([]const u8)? {
        if (_.scheme != .bearer) return null;
        return _.credentials;
    }
}

pub struct BasicCredentials {
    username: []const u8,
    password: []const u8,
}

/// Decode an HTTP `Authorization: Basic <base64>` header into `buf`, returning
/// zero-copy slices for `{ username, password }` pointing into `buf`.
pub fn decodeBasicAuth(header: []const u8, buf: []u8): BasicCredentials? {
    const parsed = AuthHeader.parse(header) ?? return null;
    if (parsed.scheme != .basic) return null;
    const decoded = base64Decode(parsed.credentials, buf) ?? return null;
    let colon: usize? = null;
    let i: usize = 0;
    while (i < decoded.len) : (i += 1) {
        if (decoded[i] == ':') {
            colon = i;
            break;
        }
    }
    const c_idx = colon ?? return null;
    return BasicCredentials{
        username: decoded[0..c_idx],
        password: decoded[c_idx + 1 ..],
    };
}

/// Encode `username:password` into an HTTP `Basic <base64>` header value in `out`.
pub fn encodeBasicAuth(username: []const u8, password: []const u8, out: []u8): ([]const u8)? {
    let raw: [512]u8 = [0; 512];
    if (username.len + 1 + password.len > 512) return null;
    let rlen: usize = 0;
    let i: usize = 0;
    while (i < username.len) : (i += 1) {
        raw[rlen] = username[i];
        rlen += 1;
    }
    raw[rlen] = ':';
    rlen += 1;
    i = 0;
    while (i < password.len) : (i += 1) {
        raw[rlen] = password[i];
        rlen += 1;
    }

    if (out.len < 6) return null;
    out[0] = 'B';
    out[1] = 'a';
    out[2] = 's';
    out[3] = 'i';
    out[4] = 'c';
    out[5] = ' ';
    const enc = base64Encode(raw[0..rlen], out[6..]) ?? return null;
    return out[0 .. 6 + enc.len];
}

/// Format a `Bearer <token>` header value into `out`.
pub fn encodeBearerAuth(token: []const u8, out: []u8): ([]const u8)? {
    if (token.len == 0 || 7 + token.len > out.len) return null;
    const prefix = "Bearer ";
    let i: usize = 0;
    while (i < 7) : (i += 1) {
        out[i] = prefix[i];
    }
    let k: usize = 0;
    while (k < token.len) : (k += 1) {
        out[7 + k] = token[k];
    }
    return out[0 .. 7 + token.len];
}

// ---------- Helpers ----------

fn base64Encode(src: []const u8, out: []u8): ([]const u8)? {
    const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const needed = ((src.len + 2) / 3) * 4;
    if (needed > out.len) return null;

    let i: usize = 0;
    let p: usize = 0;
    while (i + 3 <= src.len) : (i += 3) {
        const b0 = src[i];
        const b1 = src[i + 1];
        const b2 = src[i + 2];
        out[p] = alphabet[b0 >> 2];
        out[p + 1] = alphabet[((b0 & 0x03) << 4) | (b1 >> 4)];
        out[p + 2] = alphabet[((b1 & 0x0F) << 2) | (b2 >> 6)];
        out[p + 3] = alphabet[b2 & 0x3F];
        p += 4;
    }

    const rem = src.len - i;
    if (rem == 1) {
        const b0 = src[i];
        out[p] = alphabet[b0 >> 2];
        out[p + 1] = alphabet[(b0 & 0x03) << 4];
        out[p + 2] = '=';
        out[p + 3] = '=';
        p += 4;
    } else if (rem == 2) {
        const b0 = src[i];
        const b1 = src[i + 1];
        out[p] = alphabet[b0 >> 2];
        out[p + 1] = alphabet[((b0 & 0x03) << 4) | (b1 >> 4)];
        out[p + 2] = alphabet[(b1 & 0x0F) << 2];
        out[p + 3] = '=';
        p += 4;
    }
    return out[0..p];
}

fn base64Decode(src: []const u8, out: []u8): ([]const u8)? {
    let p: usize = 0;
    let acc: u32 = 0;
    let bits: u32 = 0;
    let pad_seen = false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const c = src[i];
        if (c == '=') {
            pad_seen = true;
            continue;
        }
        if (pad_seen) return null;
        const v = b64Val(c) ?? return null;
        acc = (acc << 6) | @as(u32, v);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (p >= out.len) return null;
            out[p] = @intCast((acc >> @intCast(bits)) & 0xFF);
            p += 1;
        }
    }
    return out[0..p];
}

fn b64Val(c: u8): u8? {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return null;
}

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isOws(s[start])) : (start += 1) {}
    let end = s.len;
    while (end > start && isOws(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLower(a[i]) != toLower(b[i])) return false;
    }
    return true;
}
