// @cap/stomp — Zero-allocation STOMP 1.2 Frame Encoder & Decoder
// Clean-room MIT implementation in pure CP.

pub const STOMP_OK: i32 = 0;
pub const STOMP_INCOMPLETE: i32 = 1;
pub const STOMP_INVALID: i32 = -1;
pub const MAX_HEADERS: usize = 16;

pub struct StompHeader {
    pub key: string,
    pub value: string,
}

pub struct StompFrame {
    pub command: string,
    pub headers: [16]StompHeader,
    pub header_count: usize,
    pub body: string,
    pub consumed: usize,

    pub static fn empty() -> StompFrame {
        let headers: [16]StompHeader = undefined;
        let i: usize = 0;
        while (i < MAX_HEADERS) {
            headers[i] = StompHeader { key: "", value: "" };
            i += 1;
        }
        return StompFrame {
            command: "",
            headers: headers,
            header_count: 0,
            body: "",
            consumed: 0,
        };
    }
}

fn strEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Escapes a STOMP 1.2 header key or value:
/// `\r` -> `\r`, `\n` -> `\n`, `:` -> `\c`, `\` -> `\\`.
/// Returns bytes written into `out`.
pub fn escapeHeader(input: string, out: []u8) -> usize {
    let i: usize = 0;
    let w: usize = 0;
    while (i < input.len) {
        const ch = input[i];
        if (ch == 13) {
            if (w + 2 > out.len) { break; }
            out[w] = 92;      // '\'
            out[w + 1] = 114; // 'r'
            w += 2;
        } else if (ch == 10) {
            if (w + 2 > out.len) { break; }
            out[w] = 92;      // '\'
            out[w + 1] = 110; // 'n'
            w += 2;
        } else if (ch == 58) {
            if (w + 2 > out.len) { break; }
            out[w] = 92;      // '\'
            out[w + 1] = 99;  // 'c'
            w += 2;
        } else if (ch == 92) {
            if (w + 2 > out.len) { break; }
            out[w] = 92;      // '\'
            out[w + 1] = 92;  // '\'
            w += 2;
        } else {
            if (w >= out.len) { break; }
            out[w] = ch;
            w += 1;
        }
        i += 1;
    }
    return w;
}

/// Unescapes a STOMP 1.2 header key or value (`\r`, `\n`, `\c`, `\\`).
/// Returns bytes written into `out`.
pub fn unescapeHeader(input: string, out: []u8) -> usize {
    let i: usize = 0;
    let w: usize = 0;
    while (i < input.len && w < out.len) {
        if (input[i] == 92 && i + 1 < input.len) {
            const nxt = input[i + 1];
            if (nxt == 114) {
                out[w] = 13; // '\r'
                w += 1;
                i += 2;
                continue;
            }
            if (nxt == 110) {
                out[w] = 10; // '\n'
                w += 1;
                i += 2;
                continue;
            }
            if (nxt == 99) {
                out[w] = 58; // ':'
                w += 1;
                i += 2;
                continue;
            }
            if (nxt == 92) {
                out[w] = 92; // '\'
                w += 1;
                i += 2;
                continue;
            }
        }
        out[w] = input[i];
        w += 1;
        i += 1;
    }
    return w;
}

/// Encodes a STOMP 1.2 frame `COMMAND\nk:v\n...\n\nBODY\0` into `out`.
/// Returns total bytes written, or `0` if `command` is empty or `out` is too small.
pub fn encodeFrame(command: string, headers: []const StompHeader, body: string, out: []u8) -> usize {
    if (command.len == 0) {
        return 0;
    }

    // Calculate required size up front
    let needed: usize = command.len + 1; // COMMAND + '\n'
    let hi: usize = 0;
    while (hi < headers.len) {
        needed += headers[hi].key.len + 1 + headers[hi].value.len + 1;
        hi += 1;
    }
    needed += 1 + body.len + 1; // '\n' + BODY + '\0'

    if (needed > out.len) {
        return 0;
    }

    let w: usize = 0;
    let i: usize = 0;
    while (i < command.len) {
        out[w] = command[i];
        w += 1;
        i += 1;
    }
    out[w] = 10; // '\n'
    w += 1;

    hi = 0;
    while (hi < headers.len) {
        const h = headers[hi];
        let ki: usize = 0;
        while (ki < h.key.len) {
            out[w] = h.key[ki];
            w += 1;
            ki += 1;
        }
        out[w] = 58; // ':'
        w += 1;
        let vi: usize = 0;
        while (vi < h.value.len) {
            out[w] = h.value[vi];
            w += 1;
            vi += 1;
        }
        out[w] = 10; // '\n'
        w += 1;
        hi += 1;
    }

    out[w] = 10; // blank line separating headers and body
    w += 1;

    let bi: usize = 0;
    while (bi < body.len) {
        out[w] = body[bi];
        w += 1;
        bi += 1;
    }
    out[w] = 0; // NUL frame terminator
    w += 1;

    return w;
}

fn parseContentLength(val: string) -> i64 {
    if (val.len == 0) {
        return -1;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < val.len) {
        const ch = val[i];
        if (ch < 48 || ch > 57) {
            return -1;
        }
        acc = acc * 10 + ((ch - 48) as u64);
        if (acc > 1073741824) {
            return -1;
        }
        i += 1;
    }
    return acc as i64;
}

/// Zero-copy STOMP 1.2 frame decoder.
/// Skips leading heartbeat EOLs (`\n`/`\r\n`), supports binary NUL-containing bodies via
/// `content-length`, and returns `STOMP_OK (0)`, `STOMP_INCOMPLETE (1)`, or `STOMP_INVALID (-1)`.
pub fn decodeFrame(data: string, out_frame: *StompFrame) -> i32 {
    const n = data.len;
    let pos: usize = 0;

    // Skip leading heartbeat EOLs
    while (pos < n) {
        if (data[pos] == 10) {
            pos += 1;
        } else if (data[pos] == 13 && pos + 1 < n && data[pos + 1] == 10) {
            pos += 2;
        } else {
            break;
        }
    }
    if (pos >= n) {
        return STOMP_INCOMPLETE;
    }

    // Parse command line
    const cmd_start = pos;
    while (pos < n && data[pos] != 10) {
        pos += 1;
    }
    if (pos >= n) {
        return STOMP_INCOMPLETE;
    }
    let cmd_end = pos;
    if (cmd_end > cmd_start && data[cmd_end - 1] == 13) {
        cmd_end -= 1;
    }
    if (cmd_end == cmd_start) {
        return STOMP_INVALID;
    }
    pos += 1; // consume '\n'

    out_frame.command = data[cmd_start..cmd_end];
    out_frame.header_count = 0;
    out_frame.body = "";
    out_frame.consumed = 0;

    let has_content_len = false;
    let content_len: usize = 0;

    // Parse headers until empty line
    while (true) {
        if (pos >= n) {
            return STOMP_INCOMPLETE;
        }
        const line_start = pos;
        while (pos < n && data[pos] != 10) {
            pos += 1;
        }
        if (pos >= n) {
            return STOMP_INCOMPLETE;
        }
        let line_end = pos;
        if (line_end > line_start && data[line_end - 1] == 13) {
            line_end -= 1;
        }
        pos += 1; // consume '\n'

        if (line_end == line_start) {
            // Blank line ends header section
            break;
        }

        // Find first ':'
        let colon = line_start;
        while (colon < line_end && data[colon] != 58) {
            colon += 1;
        }
        if (colon >= line_end || colon == line_start) {
            return STOMP_INVALID;
        }

        const key = data[line_start..colon];
        const val = data[colon + 1..line_end];

        if (!has_content_len && strEq(key, "content-length")) {
            const parsed_cl = parseContentLength(val);
            if (parsed_cl < 0) {
                return STOMP_INVALID;
            }
            has_content_len = true;
            content_len = parsed_cl as usize;
        }

        if (out_frame.header_count < MAX_HEADERS) {
            out_frame.headers[out_frame.header_count] = StompHeader {
                key: key,
                value: val,
            };
            out_frame.header_count += 1;
        }
    }

    // Parse body + NUL terminator
    if (has_content_len) {
        if (pos + content_len + 1 > n) {
            return STOMP_INCOMPLETE;
        }
        if (data[pos + content_len] != 0) {
            return STOMP_INVALID;
        }
        out_frame.body = data[pos..pos + content_len];
        out_frame.consumed = pos + content_len + 1;
    } else {
        const body_start = pos;
        while (pos < n && data[pos] != 0) {
            pos += 1;
        }
        if (pos >= n) {
            return STOMP_INCOMPLETE;
        }
        out_frame.body = data[body_start..pos];
        out_frame.consumed = pos + 1;
    }

    return STOMP_OK;
}

/// Returns the first header value matching `key` (per STOMP 1.2 precedence), or `""` if absent.
pub fn getHeader(frame: StompFrame, key: string) -> string {
    let i: usize = 0;
    while (i < frame.header_count) {
        if (strEq(frame.headers[i].key, key)) {
            return frame.headers[i].value;
        }
        i += 1;
    }
    return "";
}
