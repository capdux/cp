// Source: Absorbed from /data/dev/bun/src/resolver/data_url.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation RFC 2397 Data URL parser, percent-encoder/decoder,
// base64 payload decoder/encoder, and shortest Data URL encoder.

type DataUrlVec16 = @Vector(16, u8);

fn makeDataUrlB64Lut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0xFF; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i - 'A'));
        } else if (i >= 'a' && i <= 'z') {
            lut[i] = @as(u8, @intCast(i - 'a' + 26));
        } else if (i >= '0' && i <= '9') {
            lut[i] = @as(u8, @intCast(i - '0' + 52));
        } else if (i == '+') {
            lut[i] = 62;
        } else if (i == '/') {
            lut[i] = 63;
        }
    }
    return lut;
}

fn makeDataUrlHexLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0xFF; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= '0' && i <= '9') {
            lut[i] = @as(u8, @intCast(i - '0'));
        } else if (i >= 'a' && i <= 'f') {
            lut[i] = @as(u8, @intCast(i - 'a' + 10));
        } else if (i >= 'A' && i <= 'F') {
            lut[i] = @as(u8, @intCast(i - 'A' + 10));
        }
    }
    return lut;
}

fn makeDataUrlEscLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i < 0x20 || i >= 0x7F || i == ' ' || i == '%' || i == '#' || i == '"' || i == '<' || i == '>' || i == '?' || i == '`') {
            lut[i] = 1;
        }
    }
    return lut;
}

const DATAURL_B64_LUT: [u8; 256] = makeDataUrlB64Lut();
const DATAURL_HEX_LUT: [u8; 256] = makeDataUrlHexLut();
const DATAURL_ESC_LUT: [u8; 256] = makeDataUrlEscLut();

fn findByteSimd(buf: []const u8, start: usize, needle: u8): usize? {
    let i = start;
    const v_needle: DataUrlVec16 = @splat(needle);
    const v_zero: DataUrlVec16 = @splat(0 as u8);
    const v_one: DataUrlVec16 = @splat(1 as u8);
    while (i + 16 <= buf.len) {
        const v: DataUrlVec16 = buf[i .. i + 16][0..16].*;
        if (@reduce(.Or, @select(u8, v == v_needle, v_one, v_zero)) != 0) {
            break;
        }
        i += 16;
    }
    while (i < buf.len) : (i += 1) {
        if (buf[i] == needle) return i;
    }
    return null;
}

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
        const c_idx = findByteSimd(url, 5, ',') ?? return null;
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
        if (findByteSimd(_.data, 0, '%') == null) {
            return base64Decode(_.data, out);
        }
        let tmp: [4096]u8 = undefined;
        const unesc = percentDecode(_.data, tmp[0..]) ?? return null;
        return base64Decode(unesc, out);
    }
}

/// Percent-decode `input` into `out` (unstrict: leaves invalid `%` sequences intact).
pub fn percentDecode(input: []const u8, out: []u8): ([]const u8)? {
    let i: usize = 0;
    let p: usize = 0;
    const v_pct: DataUrlVec16 = @splat(37 as u8);
    const v_zero: DataUrlVec16 = @splat(0 as u8);
    const v_one: DataUrlVec16 = @splat(1 as u8);
    while (i < input.len) {
        if (i + 16 <= input.len && p + 16 <= out.len) {
            const v: DataUrlVec16 = input[i .. i + 16][0..16].*;
            if (@reduce(.Or, @select(u8, v == v_pct, v_one, v_zero)) == 0) {
                @memcpy(out[p .. p + 16], input[i .. i + 16]);
                p += 16;
                i += 16;
                continue;
            }
        }
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
    const v_lo: DataUrlVec16 = @splat(0x21 as u8);
    const v_hi: DataUrlVec16 = @splat(0x7E as u8);
    const v_pct: DataUrlVec16 = @splat(37 as u8);
    const v_hash: DataUrlVec16 = @splat(35 as u8);
    const v_dq: DataUrlVec16 = @splat(34 as u8);
    const v_lt: DataUrlVec16 = @splat(60 as u8);
    const v_gt: DataUrlVec16 = @splat(62 as u8);
    const v_qm: DataUrlVec16 = @splat(63 as u8);
    const v_bt: DataUrlVec16 = @splat(96 as u8);
    const v_zero: DataUrlVec16 = @splat(0 as u8);
    const v_one: DataUrlVec16 = @splat(1 as u8);
    while (i < text.len) {
        if (i + 16 <= text.len) {
            const v: DataUrlVec16 = text[i .. i + 16][0..16].*;
            const needs = (v < v_lo) | (v > v_hi) | (v == v_pct) | (v == v_hash) | (v == v_dq) | (v == v_lt) | (v == v_gt) | (v == v_qm) | (v == v_bt);
            if (@reduce(.Or, @select(u8, needs, v_one, v_zero)) == 0) {
                @memcpy(out[p .. p + 16], text[i .. i + 16]);
                p += 16;
                i += 16;
                continue;
            }
        }
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
        i += 1;
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
    const v_lo: DataUrlVec16 = @splat(0x21 as u8);
    const v_hi: DataUrlVec16 = @splat(0x7E as u8);
    const v_pct: DataUrlVec16 = @splat(37 as u8);
    const v_hash: DataUrlVec16 = @splat(35 as u8);
    const v_dq: DataUrlVec16 = @splat(34 as u8);
    const v_lt: DataUrlVec16 = @splat(60 as u8);
    const v_gt: DataUrlVec16 = @splat(62 as u8);
    const v_qm: DataUrlVec16 = @splat(63 as u8);
    const v_bt: DataUrlVec16 = @splat(96 as u8);
    const v_zero: DataUrlVec16 = @splat(0 as u8);
    const v_one: DataUrlVec16 = @splat(1 as u8);
    while (i + 16 <= text.len) : (i += 16) {
        const v: DataUrlVec16 = text[i .. i + 16][0..16].*;
        const needs = (v < v_lo) | (v > v_hi) | (v == v_pct) | (v == v_hash) | (v == v_dq) | (v == v_lt) | (v == v_gt) | (v == v_qm) | (v == v_bt);
        const extra: usize = @as(usize, @reduce(.Add, @select(u8, needs, v_one, v_zero)));
        n += 16 + extra * 2;
    }
    while (i < text.len) : (i += 1) {
        n += if (needsDataUrlEscape(text[i])) @as(usize, 3) else @as(usize, 1);
    }
    return n;
}

fn needsDataUrlEscape(c: u8): bool {
    return DATAURL_ESC_LUT[usize(c)] != 0;
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
    const v = DATAURL_B64_LUT[usize(c)];
    if (v == 0xFF) return null;
    return v;
}

fn hexVal(c: u8): u8? {
    const v = DATAURL_HEX_LUT[usize(c)];
    if (v == 0xFF) return null;
    return v;
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
    const lo_A: DataUrlVec16 = @splat(65 as u8);
    const hi_Z: DataUrlVec16 = @splat(90 as u8);
    const case_bit: DataUrlVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: DataUrlVec16 = a[i .. i + 16][0..16].*;
        const vb: DataUrlVec16 = b[i .. i + 16][0..16].*;
        const la = @select(u8, (va >= lo_A) & (va <= hi_Z), va | case_bit, va);
        const lb = @select(u8, (vb >= lo_A) & (vb <= hi_Z), vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
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
    if (pos >= out.len) return pos;
    const avail = out.len - pos;
    const n = if (s.len < avail) s.len else avail;
    if (n > 0) {
        @memcpy(out[pos .. pos + n], s[0..n]);
    }
    return pos + n;
}
