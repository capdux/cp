// Source: Absorbed from /data/dev/bun/src/resolver/data_url.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation RFC 2397 Data URL parser, percent-encoder/decoder,
// base64 payload decoder/encoder, and shortest Data URL encoder.

pub struct DataUrl {
    /// Raw MIME type & parameters (excluding `;base64`). Empty implies `text/plain;charset=US-ASCII`.
    mime_type: []const u8,
    /// Raw payload slice after `,`.
    data: []const u8,
    /// Whether `;base64` flag was present.
    is_base64: bool,

    /// Parse a `data:[<mediatype>][;base64],<data>` URL. Returns null if invalid.
    pub static fn parse(url: []const u8): DataUrl? {
        if (url.len < 5 || !startsWithIgnoreCase(url, "data:")) return null;
        let comma: usize? = null;
        let i: usize = 5;
        while (i < url.len) : (i += 1) {
            if (url[i] == ',') {
                comma = i;
                break;
            }
        }
        const c_idx = comma ?? return null;
        let meta = url[5..c_idx];
        let is_b64 = false;
        if (meta.len >= 7 && eqIgnoreCase(meta[meta.len - 7 ..], ";base64")) {
            meta = meta[0 .. meta.len - 7];
            is_b64 = true;
        }
        return DataUrl{
            mime_type: meta,
            data: url[c_idx + 1 ..],
            is_base64: is_b64,
        };
    }

    /// Return effective MIME type (`text/plain;charset=US-ASCII` when omitted per RFC 2397).
    pub fn effectiveMimeType(): []const u8 {
        if (_.mime_type.len == 0) return "text/plain;charset=US-ASCII";
        return _.mime_type;
    }

    /// Decode the data payload (percent-decoding + optional base64 decoding) into `out`.
    /// Returns the decoded byte slice, or null on invalid base64 or insufficient `out` capacity.
    pub fn decodePayload(out: []u8): ([]const u8)? {
        if (!_.is_base64) {
            return percentDecode(_.data, out);
        }
        // First percent-decode into a temporary stack buffer if small, or decode directly when no '%'
        let has_pct = false;
        let i: usize = 0;
        while (i < _.data.len) : (i += 1) {
            if (_.data[i] == '%') {
                has_pct = true;
                break;
            }
        }
        if (!has_pct) {
            return base64Decode(_.data, out);
        }
        let tmp: [4096]u8 = [0; 4096];
        const unesc = percentDecode(_.data, tmp[0..]) ?? return null;
        return base64Decode(unesc, out);
    }
}

/// Percent-decode `input` into `out` (unstrict: leaves invalid `%` sequences intact).
pub fn percentDecode(input: []const u8, out: []u8): ([]const u8)? {
    let i: usize = 0;
    let p: usize = 0;
    while (i < input.len) {
        if (input[i] == '%' && i + 2 < input.len) {
            const hi = hexVal(input[i + 1]);
            const lo = hexVal(input[i + 2]);
            if (hi != null && lo != null) {
                if (p >= out.len) return null;
                out[p] = (hi.? << 4) | lo.?;
                p += 1;
                i += 3;
                continue;
            }
        }
        if (p >= out.len) return null;
        out[p] = input[i];
        p += 1;
        i += 1;
    }
    return out[0..p];
}

/// Encode `payload` as a `data:<mime>;base64,<b64>` URL into `out`.
pub fn encodeBase64DataUrl(mime_type: []const u8, payload: []const u8, out: []u8): ([]const u8)? {
    const b64_len = ((payload.len + 2) / 3) * 4;
    const total = mime_type.len + 13 + b64_len;
    if (total > out.len) return null;

    let p = appendStr(out, 0, "data:");
    p = appendStr(out, p, mime_type);
    p = appendStr(out, p, ";base64,");
    const enc = base64Encode(payload, out[p..]) ?? return null;
    return out[0 .. p + enc.len];
}

/// Encode `text` as a percent-escaped `data:<mime>,<escaped>` URL into `out`.
pub fn encodePercentDataUrl(mime_type: []const u8, text: []const u8, out: []u8): ([]const u8)? {
    const needed = countPercentDataUrlLen(mime_type, text);
    if (needed > out.len) return null;

    let p = appendStr(out, 0, "data:");
    p = appendStr(out, p, mime_type);
    p = appendStr(out, p, ",");

    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        const c = text[i];
        if (needsDataUrlEscape(c)) {
            out[p] = '%';
            out[p + 1] = hexUpper((c >> 4) & 0xF);
            out[p + 2] = hexUpper(c & 0xF);
            p += 3;
        } else {
            out[p] = c;
            p += 1;
        }
    }
    return out[0..p];
}

/// Choose the shorter representation between percent-escaped and base64 Data URLs
/// (matching Bun's `DataURL.encodeStringAsShortestDataURL`).
pub fn encodeShortestDataUrl(mime_type: []const u8, payload: []const u8, out: []u8): ([]const u8)? {
    const pct_len = countPercentDataUrlLen(mime_type, payload);
    const b64_len = mime_type.len + 13 + ((payload.len + 2) / 3) * 4;
    if (pct_len <= b64_len) {
        return encodePercentDataUrl(mime_type, payload, out);
    }
    return encodeBase64DataUrl(mime_type, payload, out);
}

// ---------- Base64 & Helpers ----------

fn countPercentDataUrlLen(mime_type: []const u8, text: []const u8): usize {
    let n: usize = mime_type.len + 6;
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        n += if (needsDataUrlEscape(text[i])) @as(usize, 3) else @as(usize, 1);
    }
    return n;
}

fn needsDataUrlEscape(c: u8): bool {
    if (c < 0x20 || c >= 0x7F) return true;
    return c == ' ' || c == '%' || c == '#' || c == '"' || c == '<' || c == '>' || c == '?' || c == '`';
}

pub fn base64Encode(src: []const u8, out: []u8): ([]const u8)? {
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

pub fn base64Decode(src: []const u8, out: []u8): ([]const u8)? {
    let p: usize = 0;
    let acc: u32 = 0;
    let bits: u32 = 0;
    let pad_seen = false;

    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const c = src[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
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

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn hexUpper(n: u8): u8 {
    return if (n < 10) '0' + n else 'A' + (n - 10);
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

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return eqIgnoreCase(s[0..prefix.len], prefix);
}

fn appendStr(out: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}
