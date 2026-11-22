// Source: Absorbed from /data/dev/bun/src/http/h2_client/encode.cp | Synced: 2026-11-22T02:34:49Z

/// RFC 9113 §3.4 HTTP/2 client connection preface (24 bytes).
pub const H2_CLIENT_PREFACE: []const u8 = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

pub const H2_FRAME_HEADER_LEN: usize = 9;
pub const H2_DEFAULT_WINDOW_SIZE: u32 = 65535;
pub const H2_DEFAULT_MAX_FRAME_SIZE: u32 = 16384;
pub const H2_MAX_FRAME_SIZE_LIMIT: u32 = 16777215;

pub const H2_FRAME_DATA: u8 = 0x0;
pub const H2_FRAME_HEADERS: u8 = 0x1;
pub const H2_FRAME_SETTINGS: u8 = 0x4;
pub const H2_FRAME_WINDOW_UPDATE: u8 = 0x8;
pub const H2_FRAME_CONTINUATION: u8 = 0x9;

pub const H2_FLAG_END_STREAM: u8 = 0x1;
pub const H2_FLAG_END_HEADERS: u8 = 0x4;

pub const H2_SETTINGS_ENABLE_PUSH: u16 = 0x2;
pub const H2_SETTINGS_INITIAL_WINDOW_SIZE: u16 = 0x4;
pub const H2_SETTINGS_MAX_HEADER_LIST_SIZE: u16 = 0x6;

/// Classification of outbound HTTP/1.x headers when translating to HTTP/2 (RFC 9113 §8.2.2).
pub enum RequestHeaderAction {
    Forward,
    ForwardNeverIndex,
    Drop,
    Host,
    Te,
    ContentLength,
    Expect,
}

/// Summary of HEADERS + CONTINUATION frame block encoding.
pub struct HeaderBlockEncodeResult {
    bytes_written: usize,
    frame_count: usize,
}

/// Mutable flow-control window pair for `encodeDataWindowed`.
pub struct DataWindowState {
    stream_window: i32,
    conn_window: i32,
}

/// Summary of flow-controlled DATA frame encoding.
pub struct DataWriteResult {
    bytes_written: usize,
    payload_consumed: usize,
    frame_count: usize,
    sent_end_stream: bool,
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

fn trimAsciiSpaces(s: []const u8): []const u8 {
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

/// Classifies an outbound HTTP header name for HTTP/2 request encoding (absorbed from Bun `classifyHeader`).
pub fn classifyRequestHeader(name: []const u8): RequestHeaderAction {
    if (eqlIgnoreAsciiCase(name, "host")) {
        return RequestHeaderAction.Host;
    }
    if (eqlIgnoreAsciiCase(name, "expect")) {
        return RequestHeaderAction.Expect;
    }
    if (
        eqlIgnoreAsciiCase(name, "connection") ||
        eqlIgnoreAsciiCase(name, "keep-alive") ||
        eqlIgnoreAsciiCase(name, "proxy-connection") ||
        eqlIgnoreAsciiCase(name, "transfer-encoding") ||
        eqlIgnoreAsciiCase(name, "upgrade")
    ) {
        return RequestHeaderAction.Drop;
    }
    if (eqlIgnoreAsciiCase(name, "te")) {
        return RequestHeaderAction.Te;
    }
    if (eqlIgnoreAsciiCase(name, "content-length")) {
        return RequestHeaderAction.ContentLength;
    }
    if (
        eqlIgnoreAsciiCase(name, "authorization") ||
        eqlIgnoreAsciiCase(name, "cookie") ||
        eqlIgnoreAsciiCase(name, "set-cookie")
    ) {
        return RequestHeaderAction.ForwardNeverIndex;
    }
    return RequestHeaderAction.Forward;
}

/// Returns true if the header must be stripped when translating to HTTP/2 per RFC 9113 §8.2.2 & Bun `writeRequest`.
pub fn shouldSkipRequestHeader(name: []const u8, value: []const u8, has_expect_continue: bool): bool {
    const action = classifyRequestHeader(name);
    match (action) {
        RequestHeaderAction.Drop => return true,
        RequestHeaderAction.Host => return true,
        RequestHeaderAction.Te => return !eqlIgnoreAsciiCase(trimAsciiSpaces(value), "trailers"),
        RequestHeaderAction.ContentLength => return has_expect_continue,
        RequestHeaderAction.Forward => return false,
        RequestHeaderAction.ForwardNeverIndex => return false,
        RequestHeaderAction.Expect => return false,
    }
}

/// Returns true if the header carries credentials/cookies and should use HPACK never-indexed encoding.
pub fn isSensitiveHeader(name: []const u8): bool {
    return classifyRequestHeader(name) == RequestHeaderAction.ForwardNeverIndex;
}

/// Copies `name` into `out_buf` converted to ASCII lowercase (RFC 9113 requires lowercase header names).
/// Returns `null` if `out_buf` is too small.
pub fn lowercaseHeaderName(name: []const u8, out_buf: *[]u8): usize? {
    if (out_buf.*.len < name.len) {
        return null;
    }
    let i: usize = 0;
    while (i < name.len) {
        out_buf.*[i] = toAsciiLower(name[i]);
        i += 1;
    }
    return name.len;
}

/// Encodes an RFC 7541 §6.3 HPACK Dynamic Table Size Update (5-bit prefix `0x20`) into `out_buf`.
/// Returns bytes written, or `null` if `out_buf` is too small.
pub fn encodeHpackTableSizeUpdate(value: u32, out_buf: *[]u8): usize? {
    if (out_buf.*.len == 0) {
        return null;
    }
    if (value < 31) {
        out_buf.*[0] = (0x20 as u8) | (value as u8);
        return 1;
    }
    out_buf.*[0] = 0x3F as u8;
    let pos: usize = 1;
    let rest: u32 = value - 31;
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

/// Writes a 9-byte big-endian RFC 9113 HTTP/2 frame header into `out_buf[0..9]`.
/// Validates `payload_len <= 0xFFFFFF` and `stream_id <= 0x7FFFFFFF`.
pub fn encodeFrameHeader(
    payload_len: u32,
    frame_type: u8,
    flags: u8,
    stream_id: u32,
    out_buf: *[]u8,
): usize? {
    if (
        payload_len > H2_MAX_FRAME_SIZE_LIMIT ||
        stream_id > 0x7FFFFFFF ||
        out_buf.*.len < H2_FRAME_HEADER_LEN
    ) {
        return null;
    }
    out_buf.*[0] = ((payload_len >> 16) & 0xFF) as u8;
    out_buf.*[1] = ((payload_len >> 8) & 0xFF) as u8;
    out_buf.*[2] = (payload_len & 0xFF) as u8;
    out_buf.*[3] = frame_type;
    out_buf.*[4] = flags;
    out_buf.*[5] = ((stream_id >> 24) & 0x7F) as u8;
    out_buf.*[6] = ((stream_id >> 16) & 0xFF) as u8;
    out_buf.*[7] = ((stream_id >> 8) & 0xFF) as u8;
    out_buf.*[8] = (stream_id & 0xFF) as u8;
    return H2_FRAME_HEADER_LEN;
}

fn writeSettingPair(out_buf: *[]u8, offset: usize, id: u16, value: u32) {
    out_buf.*[offset + 0] = ((id >> 8) & 0xFF) as u8;
    out_buf.*[offset + 1] = (id & 0xFF) as u8;
    out_buf.*[offset + 2] = ((value >> 24) & 0xFF) as u8;
    out_buf.*[offset + 3] = ((value >> 16) & 0xFF) as u8;
    out_buf.*[offset + 4] = ((value >> 8) & 0xFF) as u8;
    out_buf.*[offset + 5] = (value & 0xFF) as u8;
}

/// Encodes the full HTTP/2 client connection preface sequence (absorbed from Bun `writePreface`):
/// 24-byte connection preface + SETTINGS frame (`ENABLE_PUSH=0`, `INITIAL_WINDOW_SIZE`, `MAX_HEADER_LIST_SIZE`)
/// + optional connection-level WINDOW_UPDATE when `local_initial_window > 65535`.
pub fn encodeClientPreface(
    local_initial_window: u32,
    local_max_header_list: u32,
    out_buf: *[]u8,
): usize? {
    if (local_initial_window > 0x7FFFFFFF) {
        return null;
    }
    const need_win_upd = local_initial_window > H2_DEFAULT_WINDOW_SIZE;
    const settings_payload_len: usize = 18;
    const total_needed: usize = H2_CLIENT_PREFACE.len + H2_FRAME_HEADER_LEN + settings_payload_len +
        (if (need_win_upd) (H2_FRAME_HEADER_LEN + 4) else 0);
    if (out_buf.*.len < total_needed) {
        return null;
    }

    let pos: usize = 0;
    let i: usize = 0;
    while (i < H2_CLIENT_PREFACE.len) {
        out_buf.*[pos] = H2_CLIENT_PREFACE[i];
        pos += 1;
        i += 1;
    }

    // SETTINGS frame (stream 0, flags 0, 3 settings = 18 bytes)
    let hdr_slice: []u8 = out_buf.*[pos..pos + H2_FRAME_HEADER_LEN];
    * = encodeFrameHeader(settings_payload_len as u32, H2_FRAME_SETTINGS, 0, 0, &hdr_slice) ?? return null;
    pos += H2_FRAME_HEADER_LEN;
    writeSettingPair(out_buf, pos, H2_SETTINGS_ENABLE_PUSH, 0);
    writeSettingPair(out_buf, pos + 6, H2_SETTINGS_INITIAL_WINDOW_SIZE, local_initial_window);
    writeSettingPair(out_buf, pos + 12, H2_SETTINGS_MAX_HEADER_LIST_SIZE, local_max_header_list);
    pos += settings_payload_len;

    if (need_win_upd) {
        const inc: u32 = local_initial_window - H2_DEFAULT_WINDOW_SIZE;
        let wu_hdr: []u8 = out_buf.*[pos..pos + H2_FRAME_HEADER_LEN];
        * = encodeFrameHeader(4, H2_FRAME_WINDOW_UPDATE, 0, 0, &wu_hdr) ?? return null;
        pos += H2_FRAME_HEADER_LEN;
        out_buf.*[pos + 0] = ((inc >> 24) & 0x7F) as u8;
        out_buf.*[pos + 1] = ((inc >> 16) & 0xFF) as u8;
        out_buf.*[pos + 2] = ((inc >> 8) & 0xFF) as u8;
        out_buf.*[pos + 3] = (inc & 0xFF) as u8;
        pos += 4;
    }

    return pos;
}

/// Encodes an HPACK header block into a HEADERS frame followed by zero or more CONTINUATION frames
/// respecting `max_frame_size` (absorbed from Bun `writeHeaderBlock`).
pub fn encodeHeaderBlockFrames(
    stream_id: u32,
    block: []const u8,
    max_frame_size: usize,
    end_stream: bool,
    out_buf: *[]u8,
): HeaderBlockEncodeResult? {
    if (
        stream_id == 0 ||
        stream_id > 0x7FFFFFFF ||
        max_frame_size == 0 ||
        max_frame_size > (H2_MAX_FRAME_SIZE_LIMIT as usize)
    ) {
        return null;
    }

    let pos: usize = 0;
    let offset: usize = 0;
    let frame_count: usize = 0;
    let first: bool = true;

    while (true) {
        const rem = block.len - offset;
        const chunk_len = if (rem < max_frame_size) rem else max_frame_size;
        const last = (offset + chunk_len) == block.len;

        if (out_buf.*.len - pos < H2_FRAME_HEADER_LEN + chunk_len) {
            return null;
        }

        let flags: u8 = 0;
        if (last) {
            flags = flags | H2_FLAG_END_HEADERS;
        }
        if (first && end_stream) {
            flags = flags | H2_FLAG_END_STREAM;
        }
        const ftype: u8 = if (first) H2_FRAME_HEADERS else H2_FRAME_CONTINUATION;

        let hdr_s: []u8 = out_buf.*[pos..pos + H2_FRAME_HEADER_LEN];
        * = encodeFrameHeader(chunk_len as u32, ftype, flags, stream_id, &hdr_s) ?? return null;
        pos += H2_FRAME_HEADER_LEN;

        let j: usize = 0;
        while (j < chunk_len) {
            out_buf.*[pos + j] = block[offset + j];
            j += 1;
        }
        pos += chunk_len;
        offset += chunk_len;
        frame_count += 1;
        first = false;

        if (last) {
            break;
        }
    }

    return HeaderBlockEncodeResult {
        bytes_written: pos,
        frame_count: frame_count,
    };
}

/// Encodes flow-controlled HTTP/2 DATA frames bounded by `min(stream_window, conn_window)`,
/// `max_frame_size`, and `send_cap` (absorbed from Bun `writeDataWindowed`).
/// Decrements both windows in `win` and sets `END_STREAM` only when the final byte is emitted.
pub fn encodeDataWindowed(
    stream_id: u32,
    data: []const u8,
    max_frame_size: usize,
    end_stream: bool,
    send_cap: usize,
    win: *DataWindowState,
    out_buf: *[]u8,
): DataWriteResult? {
    if (
        stream_id == 0 ||
        stream_id > 0x7FFFFFFF ||
        max_frame_size == 0 ||
        max_frame_size > (H2_MAX_FRAME_SIZE_LIMIT as usize)
    ) {
        return null;
    }

    let pos: usize = 0;
    let offset: usize = 0;
    let frame_count: usize = 0;
    let sent_es: bool = false;

    while (true) {
        const rem = data.len - offset;
        const min_win_i32 = if (win.stream_window < win.conn_window) win.stream_window else win.conn_window;
        const avail_win: usize = if (min_win_i32 > 0) (min_win_i32 as usize) else 0;

        if (rem > 0 && avail_win == 0) {
            break;
        }
        if (rem > 0 && offset >= send_cap) {
            break;
        }

        let chunk_len = rem;
        if (chunk_len > max_frame_size) {
            chunk_len = max_frame_size;
        }
        if (chunk_len > avail_win) {
            chunk_len = avail_win;
        }
        if (rem > 0) {
            const cap_rem = send_cap - offset;
            if (chunk_len > cap_rem) {
                chunk_len = cap_rem;
            }
        }

        if (out_buf.*.len - pos < H2_FRAME_HEADER_LEN + chunk_len) {
            return null;
        }

        const last = (offset + chunk_len) == data.len;
        let flags: u8 = 0;
        if (last && end_stream) {
            flags = H2_FLAG_END_STREAM;
            sent_es = true;
        }

        let hdr_s: []u8 = out_buf.*[pos..pos + H2_FRAME_HEADER_LEN];
        * = encodeFrameHeader(chunk_len as u32, H2_FRAME_DATA, flags, stream_id, &hdr_s) ?? return null;
        pos += H2_FRAME_HEADER_LEN;

        let j: usize = 0;
        while (j < chunk_len) {
            out_buf.*[pos + j] = data[offset + j];
            j += 1;
        }
        pos += chunk_len;
        offset += chunk_len;
        frame_count += 1;

        win.stream_window -= chunk_len as i32;
        win.conn_window -= chunk_len as i32;

        if (last) {
            break;
        }
    }

    return DataWriteResult {
        bytes_written: pos,
        payload_consumed: offset,
        frame_count: frame_count,
        sent_end_stream: sent_es,
    };
}
