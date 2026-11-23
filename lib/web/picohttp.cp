// Source: cp/lib/picohttp/picohttp.cp (Pure CP Zero-Allocation HTTP/1.1 & Chunked Parser)
// Reference-hardened against seanmonstar/httparse (MIT/Apache-2.0)
// Pure CP zero-allocation HTTP/1.1 request, response, header, and chunked
// transfer-encoding parser with strict RFC 9110 / RFC 9112 validation,
// framing ambiguity guards, and 16-lane @Vector(16, u8) / cap:simd hot-path acceleration.

const std = @import("std");
import { Simd } from "cap:simd";

type HttpVec16 = @Vector(16, u8);

pub const MAX_HEADER_LINE_LEN: usize = 16384;
pub const MAX_HEADERS_BYTES: usize = 65536;
pub const HTTP_MAX_HEADERS: usize = 128;

/// Error set returned by all picohttp parsing and chunked decoding operations.
pub err PicoError {
    Incomplete,
    InvalidMethod,
    InvalidTarget,
    InvalidVersion,
    InvalidHeaderName,
    InvalidHeaderValue,
    TooManyHeaders,
    HeaderTooLarge,
    InvalidChunkSize,
    ChunkOverflow,
    ConflictingFraming,
}

/// Validated RFC 9112 §6.3 message framing result.
pub struct FramingInfo {
    pub is_chunked: bool = false,
    pub content_length: usize? = null,
}

/// Zero-copy HTTP header slice pair pointing into the caller's input buffer.
/// For obs-fold continuation lines, `name` is empty (`isMultiline() == true`).
pub struct HttpHeader {
    pub name: []const u8 = "",
    pub value: []const u8 = "",

    pub fn isMultiline(): bool {
        return _.name.len == 0;
    }
}

/// Result of parsing an HTTP/1.x request line and its headers.
pub struct ParsedRequest {
    pub method: []const u8 = "",
    pub path: []const u8 = "",
    pub minor_version: i32 = 1,
    pub headers: []HttpHeader = undefined,
    pub bytes_consumed: usize = 0,
}

/// Result of parsing an HTTP/1.x status line and its headers.
pub struct ParsedResponse {
    pub minor_version: i32 = 1,
    pub status: u16 = 200,
    pub reason: []const u8 = "",
    pub headers: []HttpHeader = undefined,
    pub bytes_consumed: usize = 0,
}

/// Result of incremental header-block parsing.
pub struct ParsedHeaders {
    pub headers: []HttpHeader = undefined,
    pub bytes_consumed: usize = 0,
}

/// State of the streaming RFC 9112 chunked transfer-encoding decoder.
pub enum ChunkedState: u8 {
    ChunkSize = 0,
    ChunkExt = 1,
    ChunkSizeLf = 2,
    ChunkData = 3,
    ChunkDataCr = 4,
    ChunkDataLf = 5,
    TrailerHead = 6,
    TrailerHeadLf = 7,
    TrailerLine = 8,
    TrailerLineLf = 9,
    Done = 10,
}

/// Result of a single streaming chunked decode step.
pub struct ChunkDecodeResult {
    pub bytes_written: usize = 0,
    pub bytes_consumed: usize = 0,
    pub done: bool = false,
}

/// RFC 9110 §5.6.2 token character check:
/// tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." /
///         "^" / "_" / "`" / "|" / "~" / DIGIT / ALPHA
fn isTokenChar(c: u8): bool {
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        return true;
    }
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
           c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' ||
           c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

/// RFC 9112 §3.2 request-target character check (VCHAR: 0x21..0x7E).
fn isTargetChar(c: u8): bool {
    return c >= 0x21 && c <= 0x7E;
}

/// RFC 9110 §5.5 field-value character check (HTAB, SP, VCHAR, obs-text; no CTL/DEL).
fn isHeaderValueChar(c: u8): bool {
    return c == 0x09 || (c >= 0x20 && c != 0x7F);
}

/// 16-lane `@Vector(16, u8)` SIMD scan returning the length of the leading valid
/// RFC 9112 request-target prefix (`0x21..=0x7E`).
pub fn simdScanTargetBytes(s: []const u8): usize {
    let i: usize = 0;
    const n = s.len;
    const v_lo: HttpVec16 = @splat(0x21);
    const v_hi: HttpVec16 = @splat(0x7E);

    while (i + 16 <= n) {
        const chunk: HttpVec16 = s[i..][0..16].*;
        const lt_lo: u16 = @bitCast(chunk < v_lo);
        const gt_hi: u16 = @bitCast(chunk > v_hi);
        const bad: u16 = lt_lo | gt_hi;
        if (bad != 0) {
            return i + (@ctz(bad) as usize);
        }
        i += 16;
    }

    while (i < n && isTargetChar(s[i])) : (i += 1) {}
    return i;
}

/// 16-lane `@Vector(16, u8)` SIMD scan returning the length of the leading valid
/// RFC 9110 header-value prefix (HTAB or `>= 0x20` and `!= 0x7F`).
pub fn simdScanHeaderValueBytes(s: []const u8): usize {
    let i: usize = 0;
    const n = s.len;
    const v_sp: HttpVec16 = @splat(0x20);
    const v_tab: HttpVec16 = @splat(0x09);
    const v_del: HttpVec16 = @splat(0x7F);

    while (i + 16 <= n) {
        const chunk: HttpVec16 = s[i..][0..16].*;
        const is_ctrl: u16 = @bitCast(chunk < v_sp);
        const is_tab: u16 = @bitCast(chunk == v_tab);
        const is_del: u16 = @bitCast(chunk == v_del);
        const bad: u16 = (is_ctrl & (~is_tab)) | is_del;
        if (bad != 0) {
            return i + (@ctz(bad) as usize);
        }
        i += 16;
    }

    while (i < n && isHeaderValueChar(s[i])) : (i += 1) {}
    return i;
}

/// 16-lane `@Vector(16, u8)` SIMD scan returning the length of the leading valid
/// RFC 9110 token prefix (fast-paths alphanumeric + `-` + `_` in 16-byte chunks).
pub fn simdScanHeaderNameBytes(s: []const u8): usize {
    let i: usize = 0;
    const n = s.len;
    const v_0: HttpVec16 = @splat('0');
    const v_10: HttpVec16 = @splat(10);
    const v_a: HttpVec16 = @splat('a');
    const v_26: HttpVec16 = @splat(26);
    const v_case: HttpVec16 = @splat(0x20);
    const v_dash: HttpVec16 = @splat('-');
    const v_under: HttpVec16 = @splat('_');

    while (i + 16 <= n) {
        const chunk: HttpVec16 = s[i..][0..16].*;
        const m_dig: u16 = @bitCast((chunk -% v_0) < v_10);
        const m_alp: u16 = @bitCast(((chunk | v_case) -% v_a) < v_26);
        const m_dash: u16 = @bitCast(chunk == v_dash);
        const m_under: u16 = @bitCast(chunk == v_under);
        const fast_ok: u16 = m_dig | m_alp | m_dash | m_under;
        const non_fast: u16 = ~fast_ok;
        if (non_fast != 0) {
            i += @ctz(non_fast) as usize;
            break;
        }
        i += 16;
    }

    while (i < n && isTokenChar(s[i])) : (i += 1) {}
    return i;
}

/// SIMD-accelerated CRLF (`\r\n`) finder using `cap:simd` `Simd.indexOfByte('\r')`.
pub fn simdFindCrlf(s: []const u8): usize? {
    let i: usize = 0;
    while (i + 1 < s.len) {
        const hit_opt = Simd.indexOfByte(s[i..], '\r');
        if (hit_opt == null) return null;
        const cr_pos = i + hit_opt.?;
        if (cr_pos + 1 < s.len && s[cr_pos + 1] == '\n') {
            return cr_pos;
        }
        i = cr_pos + 1;
    }
    return null;
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t')) {
        start += 1;
    }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) {
        end -= 1;
    }
    return s[start..end];
}

fn trimTrailingOws(s: []const u8): []const u8 {
    let end: usize = s.len;
    while (end > 0 && (s[end - 1] == ' ' || s[end - 1] == '\t')) {
        end -= 1;
    }
    return s[0..end];
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') {
            ca = ca + 32;
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = cb + 32;
        }
        if (ca != cb) {
            return false;
        }
    }
    return true;
}

struct VersionParseResult {
    minor: i32 = 1,
    next: usize = 0,
}

fn parseHttpVersion(buf: []const u8, start: usize): VersionParseResult ! PicoError {
    const prefix = "HTTP/1.";
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (start + i >= buf.len) {
            return err.Incomplete;
        }
        if (buf[start + i] != prefix[i]) {
            return err.InvalidVersion;
        }
    }
    if (start + 7 >= buf.len) {
        return err.Incomplete;
    }
    const ver_digit = buf[start + 7];
    if (ver_digit != '0' && ver_digit != '1') {
        return err.InvalidVersion;
    }
    return VersionParseResult{
        minor: (ver_digit - '0') as i32,
        next: start + 8,
    };
}

fn parseHeadersAt(buf: []const u8, start_pos: usize, headers_out: []HttpHeader): ParsedHeaders ! PicoError {
    let pos: usize = start_pos;
    let count: usize = 0;

    while (true) {
        if (pos - start_pos > MAX_HEADERS_BYTES) {
            return err.HeaderTooLarge;
        }
        if (pos >= buf.len) {
            return err.Incomplete;
        }

        // End of headers: CRLF or LF
        if (buf[pos] == '\r') {
            if (pos + 1 >= buf.len) {
                return err.Incomplete;
            }
            if (buf[pos + 1] != '\n') {
                return err.InvalidHeaderName;
            }
            pos += 2;
            break;
        }
        if (buf[pos] == '\n') {
            pos += 1;
            break;
        }

        // obs-fold continuation line (starts with SP or HTAB)
        if (buf[pos] == ' ' || buf[pos] == '\t') {
            if (count == 0) {
                return err.InvalidHeaderName;
            }
            const line_start = pos;
            while (pos < buf.len && (buf[pos] == ' ' || buf[pos] == '\t')) {
                pos += 1;
            }
            const val_start = pos;
            let val_end: usize = pos;
            while (true) {
                if (pos >= buf.len) {
                    return err.Incomplete;
                }
                if (pos - line_start > MAX_HEADER_LINE_LEN) {
                    return err.HeaderTooLarge;
                }
                const c = buf[pos];
                if (c == '\r') {
                    if (pos + 1 >= buf.len) {
                        return err.Incomplete;
                    }
                    if (buf[pos + 1] != '\n') {
                        return err.InvalidHeaderValue;
                    }
                    val_end = pos;
                    pos += 2;
                    break;
                }
                if (c == '\n') {
                    val_end = pos;
                    pos += 1;
                    break;
                }
                if (!isHeaderValueChar(c)) {
                    return err.InvalidHeaderValue;
                }
                pos += 1;
            }
            if (count >= headers_out.len) {
                return err.TooManyHeaders;
            }
            headers_out[count] = HttpHeader{
                name: "",
                value: trimTrailingOws(buf[val_start..val_end]),
            };
            count += 1;
            continue;
        }

        // Standard header: name ":" OWS value OWS CRLF
        const name_start = pos;
        const name_span = simdScanHeaderNameBytes(buf[pos..]);
        pos += name_span;
        if (pos - name_start > MAX_HEADER_LINE_LEN) {
            return err.HeaderTooLarge;
        }
        if (pos >= buf.len) {
            return err.Incomplete;
        }
        if (buf[pos] != ':') {
            return err.InvalidHeaderName;
        }
        const name_end = pos;
        if (name_end == name_start) {
            return err.InvalidHeaderName;
        }
        pos += 1; // skip ':'

        while (pos < buf.len && (buf[pos] == ' ' || buf[pos] == '\t')) {
            pos += 1;
        }
        const val_start = pos;
        const val_span = simdScanHeaderValueBytes(buf[pos..]);
        pos += val_span;
        if (pos - name_start > MAX_HEADER_LINE_LEN) {
            return err.HeaderTooLarge;
        }
        if (pos >= buf.len) {
            return err.Incomplete;
        }
        let val_end: usize = pos;
        if (buf[pos] == '\r') {
            if (pos + 1 >= buf.len) {
                return err.Incomplete;
            }
            if (buf[pos + 1] != '\n') {
                return err.InvalidHeaderValue;
            }
            pos += 2;
        } else if (buf[pos] == '\n') {
            pos += 1;
        } else {
            return err.InvalidHeaderValue;
        }

        if (count >= headers_out.len || count >= HTTP_MAX_HEADERS) {
            return err.TooManyHeaders;
        }
        headers_out[count] = HttpHeader{
            name: buf[name_start..name_end],
            value: trimOws(buf[val_start..val_end]),
        };
        count += 1;
    }

    return ParsedHeaders{
        headers: headers_out[0..count],
        bytes_consumed: pos,
    };
}

/// Parses an HTTP/1.0 or HTTP/1.1 request line and headers without heap allocation.
pub fn parseRequest(buf: []const u8, headers_out: []HttpHeader): ParsedRequest ! PicoError {
    let pos: usize = 0;

    // 1. Method
    while (true) {
        if (pos >= buf.len) {
            return err.Incomplete;
        }
        const c = buf[pos];
        if (c == ' ') {
            break;
        }
        if (!isTokenChar(c)) {
            return err.InvalidMethod;
        }
        pos += 1;
    }
    if (pos == 0) {
        return err.InvalidMethod;
    }
    const method = buf[0..pos];
    pos += 1; // skip ' '

    // 2. Request-target (16-lane SIMD fast scan)
    const path_start = pos;
    pos += simdScanTargetBytes(buf[pos..]);
    if (pos >= buf.len) {
        return err.Incomplete;
    }
    if (buf[pos] != ' ') {
        return err.InvalidTarget;
    }
    if (pos == path_start) {
        return err.InvalidTarget;
    }
    const path = buf[path_start..pos];
    pos += 1; // skip ' '

    // 3. HTTP-version + CRLF
    const ver = parseHttpVersion(buf, pos) ?;
    pos = ver.next;
    if (pos >= buf.len) {
        return err.Incomplete;
    }
    if (buf[pos] == '\r') {
        if (pos + 1 >= buf.len) {
            return err.Incomplete;
        }
        if (buf[pos + 1] != '\n') {
            return err.InvalidVersion;
        }
        pos += 2;
    } else if (buf[pos] == '\n') {
        pos += 1;
    } else {
        return err.InvalidVersion;
    }

    // 4. Headers
    const hdrs = parseHeadersAt(buf, pos, headers_out) ?;
    return ParsedRequest{
        method: method,
        path: path,
        minor_version: ver.minor,
        headers: hdrs.headers,
        bytes_consumed: hdrs.bytes_consumed,
    };
}

/// Parses an HTTP/1.0 or HTTP/1.1 response status line and headers without heap allocation.
pub fn parseResponse(buf: []const u8, headers_out: []HttpHeader): ParsedResponse ! PicoError {
    // 1. HTTP-version
    const ver = parseHttpVersion(buf, 0) ?;
    let pos = ver.next;
    if (pos >= buf.len) {
        return err.Incomplete;
    }
    if (buf[pos] != ' ') {
        return err.InvalidVersion;
    }
    pos += 1;

    // 2. 3-digit status code (100..999)
    let status_val: u16 = 0;
    let d: usize = 0;
    while (d < 3) : (d += 1) {
        if (pos >= buf.len) {
            return err.Incomplete;
        }
        const ch = buf[pos];
        if (ch < '0' || ch > '9') {
            return err.InvalidVersion;
        }
        status_val = status_val * 10 + ((ch - '0') as u16);
        pos += 1;
    }
    if (status_val < 100 || status_val > 999) {
        return err.InvalidVersion;
    }

    // 3. Optional SP + reason-phrase + CRLF
    if (pos >= buf.len) {
        return err.Incomplete;
    }
    let reason: []const u8 = "";
    if (buf[pos] == ' ') {
        pos += 1;
        const reason_start = pos;
        let reason_end = pos;
        while (true) {
            if (pos >= buf.len) {
                return err.Incomplete;
            }
            const c = buf[pos];
            if (c == '\r') {
                if (pos + 1 >= buf.len) {
                    return err.Incomplete;
                }
                if (buf[pos + 1] != '\n') {
                    return err.InvalidVersion;
                }
                reason_end = pos;
                pos += 2;
                break;
            }
            if (c == '\n') {
                reason_end = pos;
                pos += 1;
                break;
            }
            if (!isHeaderValueChar(c)) {
                return err.InvalidVersion;
            }
            pos += 1;
        }
        reason = buf[reason_start..reason_end];
    } else if (buf[pos] == '\r') {
        if (pos + 1 >= buf.len) {
            return err.Incomplete;
        }
        if (buf[pos + 1] != '\n') {
            return err.InvalidVersion;
        }
        pos += 2;
    } else if (buf[pos] == '\n') {
        pos += 1;
    } else {
        return err.InvalidVersion;
    }

    // 4. Headers
    const hdrs = parseHeadersAt(buf, pos, headers_out) ?;
    return ParsedResponse{
        minor_version: ver.minor,
        status: status_val,
        reason: reason,
        headers: hdrs.headers,
        bytes_consumed: hdrs.bytes_consumed,
    };
}

/// Parses an independent HTTP/1.x header block (terminated by CRLF/LF) without heap allocation.
pub fn parseHeaders(buf: []const u8, headers_out: []HttpHeader): ParsedHeaders ! PicoError {
    return parseHeadersAt(buf, 0, headers_out);
}

/// Case-insensitive ASCII header lookup.
pub fn findHeader(headers: []const HttpHeader, name: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < headers.len) : (i += 1) {
        if (headers[i].name.len > 0 && eqlIgnoreCaseAscii(headers[i].name, name)) {
            return headers[i].value;
        }
    }
    return null;
}

/// Parses and validates `Content-Length` across all headers.
/// Rejects invalid digits, overflow, or conflicting duplicate `Content-Length` values.
pub fn parseContentLength(headers: []const HttpHeader): (usize?) ! PicoError {
    let found: usize? = null;
    let i: usize = 0;
    while (i < headers.len) : (i += 1) {
        if (headers[i].name.len == 0 || !eqlIgnoreCaseAscii(headers[i].name, "content-length")) {
            continue;
        }
        const raw = trimOws(headers[i].value);
        if (raw.len == 0) {
            return err.InvalidHeaderValue;
        }
        let val: usize = 0;
        let k: usize = 0;
        const max_usize = std.math.maxInt(usize);
        while (k < raw.len) : (k += 1) {
            const c = raw[k];
            if (c < '0' || c > '9') {
                return err.InvalidHeaderValue;
            }
            const digit = (c - '0') as usize;
            if (val > (max_usize - digit) / 10) {
                return err.InvalidHeaderValue;
            }
            val = val * 10 + digit;
        }
        if (prev := found) {
            if (prev != val) {
                return err.InvalidHeaderValue;
            }
        } else {
            found = val;
        }
    }
    return found;
}

/// Validates RFC 9112 §6.3 HTTP/1.1 message framing headers to prevent framing ambiguity:
/// - Rejects simultaneous `Transfer-Encoding` and `Content-Length` headers (`ConflictingFraming`)
/// - Rejects conflicting or malformed `Content-Length` headers (`InvalidHeaderValue`)
/// - Validates `Transfer-Encoding` contains `chunked` as the final coding without duplicate `chunked`
pub fn validateHttp1Framing(headers: []const HttpHeader): FramingInfo ! PicoError {
    const cl: usize? = parseContentLength(headers) catch |e| {
        return e;
    };
    let has_te = false;
    let chunked_count: usize = 0;
    let last_was_chunked = false;

    let i: usize = 0;
    while (i < headers.len) : (i += 1) {
        if (headers[i].name.len == 0 || !eqlIgnoreCaseAscii(headers[i].name, "transfer-encoding")) {
            continue;
        }
        has_te = true;
        const val = trimOws(headers[i].value);
        if (val.len == 0) {
            return err.ConflictingFraming;
        }
        // Parse comma-separated transfer codings
        let p: usize = 0;
        while (p < val.len) {
            let tok_start = p;
            while (p < val.len && val[p] != ',') : (p += 1) {}
            const tok = trimOws(val[tok_start..p]);
            if (tok.len == 0) {
                return err.ConflictingFraming;
            }
            if (eqlIgnoreCaseAscii(tok, "chunked")) {
                chunked_count += 1;
                last_was_chunked = true;
            } else {
                last_was_chunked = false;
            }
            if (p < val.len && val[p] == ',') p += 1;
        }
    }

    if (has_te && cl != null) {
        return err.ConflictingFraming;
    }
    if (has_te) {
        if (chunked_count > 1 || !last_was_chunked) {
            return err.ConflictingFraming;
        }
        return FramingInfo{ is_chunked: true, content_length: null };
    }
    return FramingInfo{ is_chunked: false, content_length: cl };
}

/// Strict RFC 9112 §7.1 single chunk-size header line parser (must end with `\r\n`).
/// Rejects leading whitespace, negative signs, empty hex, invalid extensions, missing `\r\n`, and overflow.
pub fn parseChunkSizeStrict(line: []const u8): usize ! PicoError {
    const crlf_pos = simdFindCrlf(line) ?? return err.Incomplete;
    if (crlf_pos == 0) return err.InvalidChunkSize;

    const max_usize = std.math.maxInt(usize);
    let i: usize = 0;
    let size: usize = 0;
    let digits: usize = 0;
    while (i < crlf_pos) : (i += 1) {
        const c = line[i];
        if (c == ';') {
            break;
        }
        const d_opt = ChunkedDecoder.hexDigit(c);
        if (d := d_opt) {
            if (size > (max_usize - d) / 16) return err.ChunkOverflow;
            size = size * 16 + d;
            digits += 1;
        } else {
            return err.InvalidChunkSize;
        }
    }
    if (digits == 0) return err.InvalidChunkSize;

    // Validate optional ;chunk-ext characters before CRLF
    while (i < crlf_pos) : (i += 1) {
        if (!isHeaderValueChar(line[i])) return err.InvalidChunkSize;
    }
    return size;
}

/// Streaming zero-allocation RFC 9112 §7.1 Chunked Transfer-Encoding decoder.
/// Supports arbitrary 1..N byte incremental feeding, `;chunk-ext` skipping,
/// `0\r\n` final chunk termination, and optional trailer header skipping.
pub struct ChunkedDecoder {
    pub consume_trailer: bool = true,
    pub state: ChunkedState = ChunkedState.ChunkSize,
    pub bytes_left: usize = 0,
    pub hex_count: u8 = 0,
    pub total_decoded: usize = 0,

    pub static fn init(consume_trailer: bool = true): ChunkedDecoder {
        return ChunkedDecoder{
            consume_trailer: consume_trailer,
            state: ChunkedState.ChunkSize,
            bytes_left: 0,
            hex_count: 0,
            total_decoded: 0,
        };
    }

    pub fn reset(*_) {
        _.state = ChunkedState.ChunkSize;
        _.bytes_left = 0;
        _.hex_count = 0;
        _.total_decoded = 0;
    }

    pub fn isInData(): bool {
        return _.state == ChunkedState.ChunkData;
    }

    pub fn isDone(): bool {
        return _.state == ChunkedState.Done;
    }

    static fn hexDigit(c: u8): usize? {
        if (c >= '0' && c <= '9') {
            return (c - '0') as usize;
        }
        if (c >= 'a' && c <= 'f') {
            return (c - 'a' + 10) as usize;
        }
        if (c >= 'A' && c <= 'F') {
            return (c - 'A' + 10) as usize;
        }
        return null;
    }

    /// Decodes chunked data in-place inside `buf`.
    /// Decoded payload bytes are compacted to `buf[0..result.bytes_written]`.
    pub fn decode(*_, buf: []u8): ChunkDecodeResult ! PicoError {
        return _.decodeInternal(buf, buf, true);
    }

    /// Decodes chunked data from `src` into caller-provided `dst` without modifying `src`.
    pub fn decodeTo(*_, src: []const u8, dst: []u8): ChunkDecodeResult ! PicoError {
        return _.decodeInternal(src, dst, false);
    }

    fn decodeInternal(*_, src: []const u8, dst: []u8, is_inplace: bool): ChunkDecodeResult ! PicoError {
        let r: usize = 0;
        let w: usize = 0;
        const max_usize = std.math.maxInt(usize);

        while (r < src.len && _.state != ChunkedState.Done) {
            if (_.state == ChunkedState.ChunkData) {
                const avail_src = src.len - r;
                const avail_dst = dst.len - w;
                if (avail_dst == 0) {
                    break;
                }
                let n = _.bytes_left;
                if (avail_src < n) n = avail_src;
                if (avail_dst < n) n = avail_dst;
                if (n > 0) {
                    if (!is_inplace || w != r) {
                        std.mem.copyForwards(u8, dst[w .. w + n], src[r .. r + n]);
                    }
                    r += n;
                    w += n;
                    _.bytes_left -= n;
                    _.total_decoded += n;
                }
                if (_.bytes_left == 0) {
                    _.state = ChunkedState.ChunkDataCr;
                }
                continue;
            }

            const c = src[r];
            r += 1;

            if (_.state == ChunkedState.ChunkSize) {
                if (digit := hexDigit(c)) {
                    if (_.bytes_left > (max_usize - digit) / 16) {
                        return err.ChunkOverflow;
                    }
                    _.bytes_left = _.bytes_left * 16 + digit;
                    if (_.hex_count < 255) {
                        _.hex_count += 1;
                    }
                } else if (c == ';' || c == ' ' || c == '\t') {
                    if (_.hex_count == 0) {
                        return err.InvalidChunkSize;
                    }
                    _.state = ChunkedState.ChunkExt;
                } else if (c == '\r') {
                    if (_.hex_count == 0) {
                        return err.InvalidChunkSize;
                    }
                    _.state = ChunkedState.ChunkSizeLf;
                } else if (c == '\n') {
                    if (_.hex_count == 0) {
                        return err.InvalidChunkSize;
                    }
                    if (_.bytes_left == 0) {
                        if (_.consume_trailer) {
                            _.state = ChunkedState.TrailerHead;
                        } else {
                            _.state = ChunkedState.Done;
                        }
                    } else {
                        _.state = ChunkedState.ChunkData;
                    }
                } else {
                    return err.InvalidChunkSize;
                }
            } else if (_.state == ChunkedState.ChunkExt) {
                if (c == '\r') {
                    _.state = ChunkedState.ChunkSizeLf;
                } else if (c == '\n') {
                    if (_.bytes_left == 0) {
                        if (_.consume_trailer) {
                            _.state = ChunkedState.TrailerHead;
                        } else {
                            _.state = ChunkedState.Done;
                        }
                    } else {
                        _.state = ChunkedState.ChunkData;
                    }
                } else if (!isHeaderValueChar(c)) {
                    return err.InvalidChunkSize;
                }
            } else if (_.state == ChunkedState.ChunkSizeLf) {
                if (c != '\n') {
                    return err.InvalidChunkSize;
                }
                if (_.bytes_left == 0) {
                    if (_.consume_trailer) {
                        _.state = ChunkedState.TrailerHead;
                    } else {
                        _.state = ChunkedState.Done;
                    }
                } else {
                    _.state = ChunkedState.ChunkData;
                }
            } else if (_.state == ChunkedState.ChunkDataCr) {
                if (c == '\r') {
                    _.state = ChunkedState.ChunkDataLf;
                } else if (c == '\n') {
                    _.bytes_left = 0;
                    _.hex_count = 0;
                    _.state = ChunkedState.ChunkSize;
                } else {
                    return err.InvalidChunkSize;
                }
            } else if (_.state == ChunkedState.ChunkDataLf) {
                if (c != '\n') {
                    return err.InvalidChunkSize;
                }
                _.bytes_left = 0;
                _.hex_count = 0;
                _.state = ChunkedState.ChunkSize;
            } else if (_.state == ChunkedState.TrailerHead) {
                if (c == '\r') {
                    _.state = ChunkedState.TrailerHeadLf;
                } else if (c == '\n') {
                    _.state = ChunkedState.Done;
                } else if (c == 0) {
                    return err.InvalidHeaderName;
                } else {
                    _.state = ChunkedState.TrailerLine;
                }
            } else if (_.state == ChunkedState.TrailerHeadLf) {
                if (c != '\n') {
                    return err.InvalidHeaderName;
                }
                _.state = ChunkedState.Done;
            } else if (_.state == ChunkedState.TrailerLine) {
                if (c == '\r') {
                    _.state = ChunkedState.TrailerLineLf;
                } else if (c == '\n') {
                    _.state = ChunkedState.TrailerHead;
                } else if (!isHeaderValueChar(c)) {
                    return err.InvalidHeaderValue;
                }
            } else if (_.state == ChunkedState.TrailerLineLf) {
                if (c != '\n') {
                    return err.InvalidHeaderValue;
                }
                _.state = ChunkedState.TrailerHead;
            }
        }

        return ChunkDecodeResult{
            bytes_written: w,
            bytes_consumed: r,
            done: _.state == ChunkedState.Done,
        };
    }
}
