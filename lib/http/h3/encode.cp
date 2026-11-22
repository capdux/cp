// Source: Absorbed from /data/dev/bun/src/http/h3_client/encode.cp | Synced: 2026-11-22T01:46:00Z
// Pure-CP zero-allocation HTTP/3 (RFC 9114) request pseudo-header builder,
// hop-by-hop forbidden header filter, lowercase normalizer, and QPACK static index mapper.

pub err H3EncodeError {
    MissingAuthority,
    HeaderSlotsExhausted,
    LowerBufferTooSmall,
    InvalidHeaderName,
}

/// Raw HTTP/1.1-style input header pair before HTTP/3 normalization.
pub struct H3InputHeader {
    name: []const u8 = "",
    value: []const u8 = "",
}

/// Normalized HTTP/3 header entry with optional RFC 9204 QPACK static table index (`-1` if unindexed).
pub struct H3Header {
    name: []const u8 = "",
    value: []const u8 = "",
    qpack_index: i16 = -1,

    pub fn isIndexed(): bool {
        return _.qpack_index >= 0;
    }
}

/// Request metadata used to construct the 4 mandatory RFC 9114 pseudo-headers and `end_stream` flag.
pub struct H3RequestMeta {
    method: []const u8 = "GET",
    scheme: []const u8 = "https",
    url_host: []const u8 = "",
    session_hostname: []const u8 = "",
    path: []const u8 = "/",
    has_inline_body: bool = false,
    is_streaming: bool = false,
}

/// Summary of an encoded HTTP/3 request header block.
pub struct H3EncodedRequest {
    header_count: usize = 0,
    lower_bytes_used: usize = 0,
    dropped_forbidden: usize = 0,
    end_stream: bool = true,
}

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (asciiLower(a[i]) != asciiLower(b[i])) return false;
    }
    return true;
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn isForbiddenHopByHop(name: []const u8, value: []const u8): bool {
    if (eqIgnoreCase(name, "connection") ||
        eqIgnoreCase(name, "keep-alive") ||
        eqIgnoreCase(name, "proxy-connection") ||
        eqIgnoreCase(name, "transfer-encoding") ||
        eqIgnoreCase(name, "upgrade"))
    {
        return true;
    }
    // RFC 9114 §4.2: `te` is forbidden unless its value is exactly "trailers".
    if (eqIgnoreCase(name, "te")) {
        return !eqIgnoreCase(trimOws(value), "trailers");
    }
    return false;
}

struct QpackStaticHit {
    canonical_name: []const u8,
    index: i16,
}

fn lookupQpackHeader(name: []const u8): QpackStaticHit? {
    if (eqIgnoreCase(name, "accept")) return QpackStaticHit{ canonical_name: "accept", index: 29 };
    if (eqIgnoreCase(name, "accept-encoding")) return QpackStaticHit{ canonical_name: "accept-encoding", index: 31 };
    if (eqIgnoreCase(name, "accept-language")) return QpackStaticHit{ canonical_name: "accept-language", index: 72 };
    if (eqIgnoreCase(name, "accept-ranges")) return QpackStaticHit{ canonical_name: "accept-ranges", index: 32 };
    if (eqIgnoreCase(name, "authorization")) return QpackStaticHit{ canonical_name: "authorization", index: 84 };
    if (eqIgnoreCase(name, "cache-control")) return QpackStaticHit{ canonical_name: "cache-control", index: 36 };
    if (eqIgnoreCase(name, "content-disposition")) return QpackStaticHit{ canonical_name: "content-disposition", index: 3 };
    if (eqIgnoreCase(name, "content-encoding")) return QpackStaticHit{ canonical_name: "content-encoding", index: 43 };
    if (eqIgnoreCase(name, "content-length")) return QpackStaticHit{ canonical_name: "content-length", index: 4 };
    if (eqIgnoreCase(name, "content-type")) return QpackStaticHit{ canonical_name: "content-type", index: 44 };
    if (eqIgnoreCase(name, "cookie")) return QpackStaticHit{ canonical_name: "cookie", index: 5 };
    if (eqIgnoreCase(name, "date")) return QpackStaticHit{ canonical_name: "date", index: 6 };
    if (eqIgnoreCase(name, "etag")) return QpackStaticHit{ canonical_name: "etag", index: 7 };
    if (eqIgnoreCase(name, "forwarded")) return QpackStaticHit{ canonical_name: "forwarded", index: 88 };
    if (eqIgnoreCase(name, "if-modified-since")) return QpackStaticHit{ canonical_name: "if-modified-since", index: 8 };
    if (eqIgnoreCase(name, "if-none-match")) return QpackStaticHit{ canonical_name: "if-none-match", index: 9 };
    if (eqIgnoreCase(name, "if-range")) return QpackStaticHit{ canonical_name: "if-range", index: 89 };
    if (eqIgnoreCase(name, "last-modified")) return QpackStaticHit{ canonical_name: "last-modified", index: 10 };
    if (eqIgnoreCase(name, "link")) return QpackStaticHit{ canonical_name: "link", index: 11 };
    if (eqIgnoreCase(name, "location")) return QpackStaticHit{ canonical_name: "location", index: 12 };
    if (eqIgnoreCase(name, "origin")) return QpackStaticHit{ canonical_name: "origin", index: 90 };
    if (eqIgnoreCase(name, "range")) return QpackStaticHit{ canonical_name: "range", index: 55 };
    if (eqIgnoreCase(name, "referer")) return QpackStaticHit{ canonical_name: "referer", index: 13 };
    if (eqIgnoreCase(name, "server")) return QpackStaticHit{ canonical_name: "server", index: 92 };
    if (eqIgnoreCase(name, "set-cookie")) return QpackStaticHit{ canonical_name: "set-cookie", index: 14 };
    if (eqIgnoreCase(name, "user-agent")) return QpackStaticHit{ canonical_name: "user-agent", index: 95 };
    if (eqIgnoreCase(name, "vary")) return QpackStaticHit{ canonical_name: "vary", index: 59 };
    if (eqIgnoreCase(name, "x-forwarded-for")) return QpackStaticHit{ canonical_name: "x-forwarded-for", index: 96 };
    return null;
}

fn qpackMethodIndex(method: []const u8): i16 {
    if (eqIgnoreCase(method, "GET")) return 17;
    if (eqIgnoreCase(method, "POST")) return 20;
    if (eqIgnoreCase(method, "PUT")) return 21;
    if (eqIgnoreCase(method, "DELETE")) return 16;
    if (eqIgnoreCase(method, "HEAD")) return 18;
    if (eqIgnoreCase(method, "OPTIONS")) return 19;
    if (eqIgnoreCase(method, "CONNECT")) return 15;
    return -1;
}

fn qpackSchemeIndex(scheme: []const u8): i16 {
    if (eqIgnoreCase(scheme, "https")) return 23;
    if (eqIgnoreCase(scheme, "http")) return 22;
    return -1;
}

/// Normalize and frame an HTTP/3 request header block into caller-provided `out_headers` and `lower_buf`.
///
/// Follows Bun `src/http/h3_client/encode.cp`:
/// - Reserves slots `[0..4]` for `:method`, `:scheme`, `:authority`, `:path`.
/// - Filters RFC 9114 forbidden hop-by-hop headers (`connection`, `keep-alive`, `transfer-encoding`, `upgrade`, non-trailers `te`).
/// - Promotes explicit `Host` header value to `:authority` (falling back to `url_host`, then `session_hostname`).
/// - Normalizes custom header names to lowercase inside `lower_buf` with zero heap allocation.
pub fn encodeH3RequestHeaders(
    meta: H3RequestMeta,
    in_headers: []const H3InputHeader,
    out_headers: []H3Header,
    lower_buf: []u8,
): H3EncodedRequest ! H3EncodeError {
    if (out_headers.len < 4) return err.HeaderSlotsExhausted;

    let authority: []const u8 = meta.url_host;
    let out_idx: usize = 4;
    let lower_used: usize = 0;
    let dropped: usize = 0;

    let i: usize = 0;
    while (i < in_headers.len) : (i += 1) {
        const h = in_headers[i];
        if (h.name.len == 0 || h.name[0] == ':') {
            return err.InvalidHeaderName;
        }
        if (isForbiddenHopByHop(h.name, h.value)) {
            dropped += 1;
            continue;
        }
        if (eqIgnoreCase(h.name, "host")) {
            authority = h.value;
            continue;
        }
        if (out_idx >= out_headers.len) {
            return err.HeaderSlotsExhausted;
        }
        if (hit := lookupQpackHeader(h.name)) {
            out_headers[out_idx] = H3Header{
                name: hit.canonical_name,
                value: h.value,
                qpack_index: hit.index,
            };
            out_idx += 1;
        } else {
            if (lower_used + h.name.len > lower_buf.len) {
                return err.LowerBufferTooSmall;
            }
            let dst = lower_buf[lower_used .. lower_used + h.name.len];
            let k: usize = 0;
            while (k < h.name.len) : (k += 1) {
                dst[k] = asciiLower(h.name[k]);
            }
            lower_used += h.name.len;
            out_headers[out_idx] = H3Header{
                name: dst,
                value: h.value,
                qpack_index: -1,
            };
            out_idx += 1;
        }
    }

    if (authority.len == 0) {
        authority = meta.session_hostname;
    }
    if (authority.len == 0) {
        return err.MissingAuthority;
    }

    const req_method: []const u8 = if (meta.method.len > 0) meta.method else "GET";
    const req_scheme: []const u8 = if (meta.scheme.len > 0) meta.scheme else "https";
    const req_path: []const u8 = if (meta.path.len > 0) meta.path else "/";

    out_headers[0] = H3Header{
        name: ":method",
        value: req_method,
        qpack_index: qpackMethodIndex(req_method),
    };
    out_headers[1] = H3Header{
        name: ":scheme",
        value: req_scheme,
        qpack_index: qpackSchemeIndex(req_scheme),
    };
    out_headers[2] = H3Header{
        name: ":authority",
        value: authority,
        qpack_index: 0,
    };
    out_headers[3] = H3Header{
        name: ":path",
        value: req_path,
        qpack_index: 1,
    };

    return H3EncodedRequest{
        header_count: out_idx,
        lower_bytes_used: lower_used,
        dropped_forbidden: dropped,
        end_stream: !meta.has_inline_body && !meta.is_streaming,
    };
}
