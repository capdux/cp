// Source: Absorbed from /data/dev/bun/src/uws_sys/quic/Header.cp | Synced: 2026-11-22T12:49:17Z

/// Sentinel indicating a header has no QPACK static table index.
pub const QPACK_NO_INDEX: i32 = -1;

// RFC 9204 Appendix A static table indices used by Bun's HTTP/3 client (src/uws_sys/quic/Header.cp).
pub const QPACK_IDX_AUTHORITY: i32 = 0;
pub const QPACK_IDX_PATH: i32 = 1;
pub const QPACK_IDX_CONTENT_DISPOSITION: i32 = 3;
pub const QPACK_IDX_CONTENT_LENGTH: i32 = 4;
pub const QPACK_IDX_COOKIE: i32 = 5;
pub const QPACK_IDX_DATE: i32 = 6;
pub const QPACK_IDX_ETAG: i32 = 7;
pub const QPACK_IDX_IF_MODIFIED_SINCE: i32 = 8;
pub const QPACK_IDX_IF_NONE_MATCH: i32 = 9;
pub const QPACK_IDX_LAST_MODIFIED: i32 = 10;
pub const QPACK_IDX_LINK: i32 = 11;
pub const QPACK_IDX_LOCATION: i32 = 12;
pub const QPACK_IDX_REFERER: i32 = 13;
pub const QPACK_IDX_SET_COOKIE: i32 = 14;
pub const QPACK_IDX_METHOD_GET: i32 = 17;
pub const QPACK_IDX_SCHEME_HTTPS: i32 = 23;
pub const QPACK_IDX_ACCEPT: i32 = 29;
pub const QPACK_IDX_ACCEPT_ENCODING: i32 = 31;
pub const QPACK_IDX_ACCEPT_RANGES: i32 = 32;
pub const QPACK_IDX_CACHE_CONTROL: i32 = 36;
pub const QPACK_IDX_CONTENT_ENCODING: i32 = 43;
pub const QPACK_IDX_CONTENT_TYPE: i32 = 44;
pub const QPACK_IDX_RANGE: i32 = 55;
pub const QPACK_IDX_VARY: i32 = 59;
pub const QPACK_IDX_ACCEPT_LANGUAGE: i32 = 72;
pub const QPACK_IDX_AUTHORIZATION: i32 = 84;
pub const QPACK_IDX_FORWARDED: i32 = 88;
pub const QPACK_IDX_IF_RANGE: i32 = 89;
pub const QPACK_IDX_ORIGIN: i32 = 90;
pub const QPACK_IDX_SERVER: i32 = 92;
pub const QPACK_IDX_USER_AGENT: i32 = 95;
pub const QPACK_IDX_X_FORWARDED_FOR: i32 = 96;

/// Zero-alloc HTTP/3 QPACK header representation.
pub struct QpackHeader {
    name: []const u8,
    value: []const u8,
    qpack_index: i32,

    static fn init(name: []const u8, value: []const u8, qpack_index: i32): QpackHeader {
        return QpackHeader {
            name: name,
            value: value,
            qpack_index: qpack_index,
        };
    }
}

/// Classification outcome for an HTTP header name under HTTP/3 QPACK rules.
pub enum QpackClassKind {
    Forbidden,
    Host,
    Indexed,
    Literal,
}

/// Result of `classifyHeader`.
pub struct QpackClass {
    kind: QpackClassKind,
    canonical_name: []const u8,
    static_index: i32,
}

/// Result of `decodePrefixedInt`.
pub struct IntDecodeResult {
    value: u64,
    bytes_read: usize,
}

fn toAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqlIgnoreAsciiCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toAsciiLower(a[i]) != toAsciiLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isNeverIndexName(name: []const u8): bool {
    return eqlIgnoreAsciiCase(name, "authorization") ||
        eqlIgnoreAsciiCase(name, "cookie") ||
        eqlIgnoreAsciiCase(name, "set-cookie");
}

fn indexedClass(canonical: []const u8, idx: i32): QpackClass {
    return QpackClass {
        kind: QpackClassKind.Indexed,
        canonical_name: canonical,
        static_index: idx,
    };
}

/// Classifies an HTTP header name case-insensitively according to Bun's HTTP/3 QPACK table
/// (`src/uws_sys/quic/Header.cp`): identifies forbidden hop-by-hop headers, `host`,
/// RFC 9204 static-indexed headers, or custom literal headers.
pub fn classifyHeader(name: []const u8): QpackClass {
    if (eqlIgnoreAsciiCase(name, "host")) {
        return QpackClass { kind: QpackClassKind.Host, canonical_name: "host", static_index: QPACK_NO_INDEX };
    }
    if (
        eqlIgnoreAsciiCase(name, "connection") ||
        eqlIgnoreAsciiCase(name, "keep-alive") ||
        eqlIgnoreAsciiCase(name, "proxy-connection") ||
        eqlIgnoreAsciiCase(name, "transfer-encoding") ||
        eqlIgnoreAsciiCase(name, "upgrade")
    ) {
        return QpackClass { kind: QpackClassKind.Forbidden, canonical_name: "", static_index: QPACK_NO_INDEX };
    }

    if (eqlIgnoreAsciiCase(name, ":authority")) return indexedClass(":authority", QPACK_IDX_AUTHORITY);
    if (eqlIgnoreAsciiCase(name, ":path")) return indexedClass(":path", QPACK_IDX_PATH);
    if (eqlIgnoreAsciiCase(name, ":method")) return indexedClass(":method", QPACK_IDX_METHOD_GET);
    if (eqlIgnoreAsciiCase(name, ":scheme")) return indexedClass(":scheme", QPACK_IDX_SCHEME_HTTPS);
    if (eqlIgnoreAsciiCase(name, "accept")) return indexedClass("accept", QPACK_IDX_ACCEPT);
    if (eqlIgnoreAsciiCase(name, "accept-encoding")) return indexedClass("accept-encoding", QPACK_IDX_ACCEPT_ENCODING);
    if (eqlIgnoreAsciiCase(name, "accept-language")) return indexedClass("accept-language", QPACK_IDX_ACCEPT_LANGUAGE);
    if (eqlIgnoreAsciiCase(name, "accept-ranges")) return indexedClass("accept-ranges", QPACK_IDX_ACCEPT_RANGES);
    if (eqlIgnoreAsciiCase(name, "authorization")) return indexedClass("authorization", QPACK_IDX_AUTHORIZATION);
    if (eqlIgnoreAsciiCase(name, "cache-control")) return indexedClass("cache-control", QPACK_IDX_CACHE_CONTROL);
    if (eqlIgnoreAsciiCase(name, "content-disposition")) return indexedClass("content-disposition", QPACK_IDX_CONTENT_DISPOSITION);
    if (eqlIgnoreAsciiCase(name, "content-encoding")) return indexedClass("content-encoding", QPACK_IDX_CONTENT_ENCODING);
    if (eqlIgnoreAsciiCase(name, "content-length")) return indexedClass("content-length", QPACK_IDX_CONTENT_LENGTH);
    if (eqlIgnoreAsciiCase(name, "content-type")) return indexedClass("content-type", QPACK_IDX_CONTENT_TYPE);
    if (eqlIgnoreAsciiCase(name, "cookie")) return indexedClass("cookie", QPACK_IDX_COOKIE);
    if (eqlIgnoreAsciiCase(name, "date")) return indexedClass("date", QPACK_IDX_DATE);
    if (eqlIgnoreAsciiCase(name, "etag")) return indexedClass("etag", QPACK_IDX_ETAG);
    if (eqlIgnoreAsciiCase(name, "forwarded")) return indexedClass("forwarded", QPACK_IDX_FORWARDED);
    if (eqlIgnoreAsciiCase(name, "if-modified-since")) return indexedClass("if-modified-since", QPACK_IDX_IF_MODIFIED_SINCE);
    if (eqlIgnoreAsciiCase(name, "if-none-match")) return indexedClass("if-none-match", QPACK_IDX_IF_NONE_MATCH);
    if (eqlIgnoreAsciiCase(name, "if-range")) return indexedClass("if-range", QPACK_IDX_IF_RANGE);
    if (eqlIgnoreAsciiCase(name, "last-modified")) return indexedClass("last-modified", QPACK_IDX_LAST_MODIFIED);
    if (eqlIgnoreAsciiCase(name, "link")) return indexedClass("link", QPACK_IDX_LINK);
    if (eqlIgnoreAsciiCase(name, "location")) return indexedClass("location", QPACK_IDX_LOCATION);
    if (eqlIgnoreAsciiCase(name, "origin")) return indexedClass("origin", QPACK_IDX_ORIGIN);
    if (eqlIgnoreAsciiCase(name, "range")) return indexedClass("range", QPACK_IDX_RANGE);
    if (eqlIgnoreAsciiCase(name, "referer")) return indexedClass("referer", QPACK_IDX_REFERER);
    if (eqlIgnoreAsciiCase(name, "server")) return indexedClass("server", QPACK_IDX_SERVER);
    if (eqlIgnoreAsciiCase(name, "set-cookie")) return indexedClass("set-cookie", QPACK_IDX_SET_COOKIE);
    if (eqlIgnoreAsciiCase(name, "user-agent")) return indexedClass("user-agent", QPACK_IDX_USER_AGENT);
    if (eqlIgnoreAsciiCase(name, "vary")) return indexedClass("vary", QPACK_IDX_VARY);
    if (eqlIgnoreAsciiCase(name, "x-forwarded-for")) return indexedClass("x-forwarded-for", QPACK_IDX_X_FORWARDED_FOR);

    return QpackClass {
        kind: QpackClassKind.Literal,
        canonical_name: "",
        static_index: QPACK_NO_INDEX,
    };
}

/// Resolves an RFC 9204 static table index back to its canonical lowercase header name.
pub fn staticIndexToName(index: u8): string? {
    const idx = index as i32;
    if (idx == QPACK_IDX_AUTHORITY) return ":authority";
    if (idx == QPACK_IDX_PATH) return ":path";
    if (idx == QPACK_IDX_CONTENT_DISPOSITION) return "content-disposition";
    if (idx == QPACK_IDX_CONTENT_LENGTH) return "content-length";
    if (idx == QPACK_IDX_COOKIE) return "cookie";
    if (idx == QPACK_IDX_DATE) return "date";
    if (idx == QPACK_IDX_ETAG) return "etag";
    if (idx == QPACK_IDX_IF_MODIFIED_SINCE) return "if-modified-since";
    if (idx == QPACK_IDX_IF_NONE_MATCH) return "if-none-match";
    if (idx == QPACK_IDX_LAST_MODIFIED) return "last-modified";
    if (idx == QPACK_IDX_LINK) return "link";
    if (idx == QPACK_IDX_LOCATION) return "location";
    if (idx == QPACK_IDX_REFERER) return "referer";
    if (idx == QPACK_IDX_SET_COOKIE) return "set-cookie";
    if (idx == QPACK_IDX_METHOD_GET) return ":method";
    if (idx == QPACK_IDX_SCHEME_HTTPS) return ":scheme";
    if (idx == QPACK_IDX_ACCEPT) return "accept";
    if (idx == QPACK_IDX_ACCEPT_ENCODING) return "accept-encoding";
    if (idx == QPACK_IDX_ACCEPT_RANGES) return "accept-ranges";
    if (idx == QPACK_IDX_CACHE_CONTROL) return "cache-control";
    if (idx == QPACK_IDX_CONTENT_ENCODING) return "content-encoding";
    if (idx == QPACK_IDX_CONTENT_TYPE) return "content-type";
    if (idx == QPACK_IDX_RANGE) return "range";
    if (idx == QPACK_IDX_VARY) return "vary";
    if (idx == QPACK_IDX_ACCEPT_LANGUAGE) return "accept-language";
    if (idx == QPACK_IDX_AUTHORIZATION) return "authorization";
    if (idx == QPACK_IDX_FORWARDED) return "forwarded";
    if (idx == QPACK_IDX_IF_RANGE) return "if-range";
    if (idx == QPACK_IDX_ORIGIN) return "origin";
    if (idx == QPACK_IDX_SERVER) return "server";
    if (idx == QPACK_IDX_USER_AGENT) return "user-agent";
    if (idx == QPACK_IDX_X_FORWARDED_FOR) return "x-forwarded-for";
    return null;
}

/// Encodes an RFC 9204 §4.1.1 / RFC 7541 §5.1 N-bit prefixed integer into `out_buf`.
/// `prefix_bits` must be in `1..8`.
pub fn encodePrefixedInt(prefix_bits: u8, prefix_mask: u8, value: u64, out_buf: *[]u8): usize? {
    if (prefix_bits == 0 || prefix_bits > 8 || out_buf.*.len == 0) {
        return null;
    }
    const max_prefix: u64 = if (prefix_bits == 8) 255 else (((1 as u64) << (prefix_bits as u6)) - 1);
    if (value < max_prefix) {
        out_buf.*[0] = prefix_mask | (value as u8);
        return 1;
    }
    out_buf.*[0] = prefix_mask | (max_prefix as u8);
    let pos: usize = 1;
    let rest: u64 = value - max_prefix;
    while (rest >= 128) {
        if (pos >= out_buf.*.len) {
            return null;
        }
        out_buf.*[pos] = ((rest & 0x7F) as u8) | 0x80;
        pos += 1;
        rest >>= 7;
    }
    if (pos >= out_buf.*.len) {
        return null;
    }
    out_buf.*[pos] = rest as u8;
    return pos + 1;
}

/// Decodes an RFC 9204 §4.1.1 N-bit prefixed integer from `data`.
/// Returns `null` on truncated input, invalid `prefix_bits`, or u64 overflow.
pub fn decodePrefixedInt(prefix_bits: u8, data: []const u8): IntDecodeResult? {
    if (prefix_bits == 0 || prefix_bits > 8 || data.len == 0) {
        return null;
    }
    const max_prefix: u64 = if (prefix_bits == 8) 255 else (((1 as u64) << (prefix_bits as u6)) - 1);
    const first: u64 = (data[0] as u64) & max_prefix;
    if (first < max_prefix) {
        return IntDecodeResult { value: first, bytes_read: 1 };
    }

    let val: u64 = max_prefix;
    let shift: u64 = 0;
    let pos: usize = 1;
    while (pos < data.len) {
        if (shift >= 63) {
            return null;
        }
        const byte = data[pos];
        const low: u64 = (byte & 0x7F) as u64;
        const add = low << (shift as u6);
        if (val > (0xFFFFFFFFFFFFFFFF - add)) {
            return null;
        }
        val += add;
        pos += 1;
        if ((byte & 0x80) == 0) {
            return IntDecodeResult { value: val, bytes_read: pos };
        }
        shift += 7;
    }
    return null;
}

/// Builds a normalized HTTP/3 outbound request header list (absorbed from Bun `src/http/h3_client/encode.cp`).
/// Places `:method`, `:scheme`, `:authority` (overridable by `Host`), and `:path` in slots `0..3`,
/// strips forbidden hop-by-hop headers, resolves static QPACK indices, and lowercases custom names into `lower_buf`.
pub fn buildHttp3RequestHeaders(
    method: []const u8,
    scheme: []const u8,
    default_authority: []const u8,
    path: []const u8,
    raw_headers: []const QpackHeader,
    out_headers: *[]QpackHeader,
    lower_buf: *[]u8,
): usize? {
    if (method.len == 0 || scheme.len == 0 || out_headers.*.len < 4) {
        return null;
    }

    let authority = default_authority;
    let out_count: usize = 4;
    let lower_pos: usize = 0;

    let i: usize = 0;
    while (i < raw_headers.len) {
        const h = raw_headers[i];
        const cls = classifyHeader(h.name);
        match (cls.kind) {
            QpackClassKind.Forbidden => {}
            QpackClassKind.Host => {
                authority = h.value;
            }
            QpackClassKind.Indexed => {
                if (out_count >= out_headers.*.len) {
                    return null;
                }
                out_headers.*[out_count] = QpackHeader.init(cls.canonical_name, h.value, cls.static_index);
                out_count += 1;
            }
            QpackClassKind.Literal => {
                if (out_count >= out_headers.*.len || lower_buf.*.len - lower_pos < h.name.len) {
                    return null;
                }
                let j: usize = 0;
                while (j < h.name.len) {
                    lower_buf.*[lower_pos + j] = toAsciiLower(h.name[j]);
                    j += 1;
                }
                const lowered_name: []const u8 = lower_buf.*[lower_pos..lower_pos + h.name.len];
                lower_pos += h.name.len;
                out_headers.*[out_count] = QpackHeader.init(lowered_name, h.value, QPACK_NO_INDEX);
                out_count += 1;
            }
        }
        i += 1;
    }

    if (authority.len == 0) {
        return null;
    }
    const req_path = if (path.len > 0) path else "/";
    out_headers.*[0] = QpackHeader.init(":method", method, QPACK_IDX_METHOD_GET);
    out_headers.*[1] = QpackHeader.init(":scheme", scheme, QPACK_IDX_SCHEME_HTTPS);
    out_headers.*[2] = QpackHeader.init(":authority", authority, QPACK_IDX_AUTHORITY);
    out_headers.*[3] = QpackHeader.init(":path", req_path, QPACK_IDX_PATH);
    return out_count;
}

/// Encodes `headers` into an RFC 9204 static-only QPACK Field Section (`RIC = 0, Delta Base = 0`).
/// Uses static name references (`01N1xxxx`, 4-bit prefix) when `qpack_index` is valid,
/// and literal names (`001N0xxx`, 3-bit prefix) otherwise.
pub fn encodeFieldSection(headers: []const QpackHeader, out_buf: *[]u8): usize? {
    if (out_buf.*.len < 2) {
        return null;
    }
    // RFC 9204 §4.5.1 Encoded Field Section Prefix: Required Insert Count = 0, S = 0, Delta Base = 0.
    out_buf.*[0] = 0x00;
    out_buf.*[1] = 0x00;
    let pos: usize = 2;

    let i: usize = 0;
    while (i < headers.len) {
        const h = headers[i];
        const never_idx = isNeverIndexName(h.name);

        if (h.qpack_index >= 0 && h.qpack_index <= 98) {
            // RFC 9204 §4.5.4: Literal Field Line With Name Reference (T=1 static).
            const mask: u8 = if (never_idx) 0x70 else 0x50;
            let tail: []u8 = out_buf.*[pos..];
            const w = encodePrefixedInt(4, mask, h.qpack_index as u64, &tail) ?? return null;
            pos += w;
        } else {
            // RFC 9204 §4.5.6: Literal Field Line With Literal Name (H=0 plain ASCII).
            const mask: u8 = if (never_idx) 0x30 else 0x20;
            let tail: []u8 = out_buf.*[pos..];
            const wn = encodePrefixedInt(3, mask, h.name.len as u64, &tail) ?? return null;
            pos += wn;
            if (out_buf.*.len - pos < h.name.len) {
                return null;
            }
            let k: usize = 0;
            while (k < h.name.len) {
                out_buf.*[pos + k] = h.name[k];
                k += 1;
            }
            pos += h.name.len;
        }

        // Value string (H=0 plain bytes, 7-bit prefixed length).
        let vtail: []u8 = out_buf.*[pos..];
        const wv = encodePrefixedInt(7, 0x00, h.value.len as u64, &vtail) ?? return null;
        pos += wv;
        if (out_buf.*.len - pos < h.value.len) {
            return null;
        }
        let m: usize = 0;
        while (m < h.value.len) {
            out_buf.*[pos + m] = h.value[m];
            m += 1;
        }
        pos += h.value.len;
        i += 1;
    }

    return pos;
}

/// Decodes an RFC 9204 static-only QPACK Field Section (`RIC = 0, Delta Base = 0`) zero-copy into `out_headers`.
/// Returns number of headers decoded, or `null` on malformed/unsupported dynamic-table input or small buffer.
pub fn decodeFieldSection(data: []const u8, out_headers: *[]QpackHeader): usize? {
    if (data.len < 2) {
        return null;
    }
    const ric = decodePrefixedInt(8, data) ?? return null;
    if (ric.value != 0) {
        return null;
    }
    const db = decodePrefixedInt(7, data[ric.bytes_read..]) ?? return null;
    if (db.value != 0) {
        return null;
    }

    let pos: usize = ric.bytes_read + db.bytes_read;
    let count: usize = 0;

    while (pos < data.len) {
        if (count >= out_headers.*.len) {
            return null;
        }
        const b0 = data[pos];
        let hdr_name: []const u8 = "";
        let hdr_idx: i32 = QPACK_NO_INDEX;

        if ((b0 & 0xC0) == 0x40) {
            // Literal Field Line With Name Reference: require T=1 (static table).
            if ((b0 & 0x10) == 0) {
                return null;
            }
            const idx_dec = decodePrefixedInt(4, data[pos..]) ?? return null;
            if (idx_dec.value > 255) {
                return null;
            }
            const cname = staticIndexToName(idx_dec.value as u8) ?? return null;
            hdr_name = cname;
            hdr_idx = idx_dec.value as i32;
            pos += idx_dec.bytes_read;
        } else if ((b0 & 0xE0) == 0x20) {
            // Literal Field Line With Literal Name: require H=0 (non-Huffman).
            if ((b0 & 0x08) != 0) {
                return null;
            }
            const nlen = decodePrefixedInt(3, data[pos..]) ?? return null;
            pos += nlen.bytes_read;
            const nlen_u = nlen.value as usize;
            if (data.len - pos < nlen_u) {
                return null;
            }
            hdr_name = data[pos..pos + nlen_u];
            pos += nlen_u;
        } else {
            return null;
        }

        if (pos >= data.len || (data[pos] & 0x80) != 0) {
            return null;
        }
        const vlen = decodePrefixedInt(7, data[pos..]) ?? return null;
        pos += vlen.bytes_read;
        const vlen_u = vlen.value as usize;
        if (data.len - pos < vlen_u) {
            return null;
        }
        const hdr_val = data[pos..pos + vlen_u];
        pos += vlen_u;

        out_headers.*[count] = QpackHeader.init(hdr_name, hdr_val, hdr_idx);
        count += 1;
    }

    return count;
}
