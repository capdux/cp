// Origin: Pure-CP Stdlib (AI Agent LLM Streaming SSE Parser & TTFT/TPS Telemetry) | Created: 2026-11-22

fn satAddU32(a: u32, b: u32): u32 {
    if (a > 0xFFFFFFFF - b) {
        return 0xFFFFFFFF;
    }
    return a + b;
}

fn bytesEqual(a: []const u8, b: string): bool {
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

fn isJsonWs(b: u8): bool {
    return b == 32 || b == 9 || b == 10 || b == 13;
}

fn hexDigitVal(b: u8): u16? {
    if (b >= 48 && b <= 57) return (b - 48) as u16;
    if (b >= 65 && b <= 70) return (b - 55) as u16;
    if (b >= 97 && b <= 102) return (b - 87) as u16;
    return null;
}

/// Parsed Server-Sent Event view backed by `SseStreamParser`'s caller-provided buffers.
/// Valid until the next call to `SseStreamParser.nextEvent` or `reset`.
pub struct SseEvent {
    event_type: string,
    data: string,
    id: string,
    is_done: bool,
}

/// Result returned by `SseStreamParser.nextEvent`.
pub struct SseFeedResult {
    consumed: usize,
    has_event: bool,
    overflow: bool,
}

/// Zero-allocation incremental WHATWG EventSource / OpenAI & Anthropic SSE stream parser.
/// Supports arbitrary chunk boundaries, `\n`/`\r\n`/`\r` line endings, UTF-8 BOM stripping,
/// `:comment` keepalive skipping, and multi-line `data:` concatenation with `\n`.
pub struct SseStreamParser {
    line_buf: []u8,
    line_len: usize,
    data_buf: []u8,
    data_len: usize,
    has_data: bool,
    event_buf: []u8,
    event_len: usize,
    id_buf: []u8,
    id_len: usize,
    last_was_cr: bool,
    bom_state: u8,
    need_reset_event: bool,

    /// Initializes `SseStreamParser` over caller-provided scratch buffers.
    static fn init(
        line_buf: *[]u8,
        data_buf: *[]u8,
        event_buf: *[]u8,
        id_buf: *[]u8,
    ): SseStreamParser {
        return SseStreamParser {
            line_buf: line_buf.*,
            line_len: 0,
            data_buf: data_buf.*,
            data_len: 0,
            has_data: false,
            event_buf: event_buf.*,
            event_len: 0,
            id_buf: id_buf.*,
            id_len: 0,
            last_was_cr: false,
            bom_state: 0,
            need_reset_event: false,
        };
    }

    /// Resets parser state for a new SSE stream while retaining the caller buffers.
    fn reset(*_) {
        _.line_len = 0;
        _.data_len = 0;
        _.has_data = false;
        _.event_len = 0;
        _.id_len = 0;
        _.last_was_cr = false;
        _.bom_state = 0;
        _.need_reset_event = false;
    }

    fn appendLineByte(*_, b: u8): bool {
        if (_.line_len >= _.line_buf.len) {
            return false;
        }
        _.line_buf[_.line_len] = b;
        _.line_len += 1;
        return true;
    }

    fn processNonEmptyLine(*_, llen: usize): bool {
        // Comment line (starts with ':')
        if (_.line_buf[0] == 58) {
            return true;
        }

        let colon_idx: usize = llen;
        let i: usize = 0;
        while (i < llen) {
            if (_.line_buf[i] == 58) {
                colon_idx = i;
                break;
            }
            i += 1;
        }

        const field: []const u8 = _.line_buf[0..colon_idx];
        let val_start: usize = llen;
        if (colon_idx < llen) {
            val_start = colon_idx + 1;
            if (val_start < llen && _.line_buf[val_start] == 32) {
                val_start += 1;
            }
        }
        const val_len = llen - val_start;

        if (bytesEqual(field, "data")) {
            const extra: usize = if (_.has_data) 1 else 0;
            if (_.data_len + extra + val_len > _.data_buf.len) {
                return false;
            }
            if (_.has_data) {
                _.data_buf[_.data_len] = 10;
                _.data_len += 1;
            }
            let k: usize = 0;
            while (k < val_len) {
                _.data_buf[_.data_len + k] = _.line_buf[val_start + k];
                k += 1;
            }
            _.data_len += val_len;
            _.has_data = true;
            return true;
        }

        if (bytesEqual(field, "event")) {
            if (val_len > _.event_buf.len) {
                return false;
            }
            let k: usize = 0;
            while (k < val_len) {
                _.event_buf[k] = _.line_buf[val_start + k];
                k += 1;
            }
            _.event_len = val_len;
            return true;
        }

        if (bytesEqual(field, "id")) {
            // WHATWG spec: ignore id field if it contains NUL
            let k: usize = 0;
            while (k < val_len) {
                if (_.line_buf[val_start + k] == 0) {
                    return true;
                }
                k += 1;
            }
            if (val_len > _.id_buf.len) {
                return false;
            }
            let j: usize = 0;
            while (j < val_len) {
                _.id_buf[j] = _.line_buf[val_start + j];
                j += 1;
            }
            _.id_len = val_len;
            return true;
        }

        // Unknown / retry fields are ignored cleanly
        return true;
    }

    /// Consumes bytes from `chunk` until either a complete SSE event is dispatched into `out_event`,
    /// `chunk` is exhausted, or a caller buffer overflows.
    /// Call repeatedly advancing `chunk = chunk[res.consumed..]` until `chunk.len == 0` or `res.overflow`.
    fn nextEvent(*_, chunk: []const u8, out_event: *SseEvent): SseFeedResult {
        if (_.need_reset_event) {
            _.data_len = 0;
            _.has_data = false;
            _.event_len = 0;
            _.need_reset_event = false;
        }

        let pos: usize = 0;
        while (pos < chunk.len) {
            const b = chunk[pos];

            if (_.last_was_cr) {
                _.last_was_cr = false;
                if (b == 10) {
                    pos += 1;
                    continue;
                }
            }

            // Optional UTF-8 BOM (0xEF 0xBB 0xBF) at stream start
            if (_.bom_state < 3) {
                if (_.bom_state == 0) {
                    if (b == 0xEF) {
                        _.bom_state = 1;
                        pos += 1;
                        continue;
                    }
                    _.bom_state = 3;
                } else if (_.bom_state == 1) {
                    if (b == 0xBB) {
                        _.bom_state = 2;
                        pos += 1;
                        continue;
                    }
                    _.bom_state = 3;
                    if (!_.appendLineByte(0xEF)) {
                        return SseFeedResult { consumed: pos, has_event: false, overflow: true };
                    }
                } else {
                    if (b == 0xBF) {
                        _.bom_state = 3;
                        pos += 1;
                        continue;
                    }
                    _.bom_state = 3;
                    if (!_.appendLineByte(0xEF) || !_.appendLineByte(0xBB)) {
                        return SseFeedResult { consumed: pos, has_event: false, overflow: true };
                    }
                }
            }

            if (b == 10 || b == 13) {
                if (b == 13) {
                    _.last_was_cr = true;
                }
                pos += 1;

                const llen = _.line_len;
                _.line_len = 0;

                if (llen == 0) {
                    if (_.has_data) {
                        const ev_type: string = if (_.event_len > 0)
                            (_.event_buf[0.._.event_len] as string)
                        else
                            "message";
                        const ev_data: string = _.data_buf[0.._.data_len] as string;
                        const ev_id: string = _.id_buf[0.._.id_len] as string;
                        out_event.* = SseEvent {
                            event_type: ev_type,
                            data: ev_data,
                            id: ev_id,
                            is_done: bytesEqual(ev_data, "[DONE]"),
                        };
                        _.need_reset_event = true;
                        return SseFeedResult {
                            consumed: pos,
                            has_event: true,
                            overflow: false,
                        };
                    }
                    // Blank line with no data resets pending event type
                    _.event_len = 0;
                    continue;
                }

                if (!_.processNonEmptyLine(llen)) {
                    return SseFeedResult {
                        consumed: pos,
                        has_event: false,
                        overflow: true,
                    };
                }
                continue;
            }

            if (!_.appendLineByte(b)) {
                return SseFeedResult {
                    consumed: pos,
                    has_event: false,
                    overflow: true,
                };
            }
            pos += 1;
        }

        return SseFeedResult {
            consumed: pos,
            has_event: false,
            overflow: false,
        };
    }
}

/// Finalized LLM streaming latency and throughput telemetry snapshot.
pub struct StreamMetrics {
    ttft_ms: i64,
    total_ms: i64,
    chunk_count: u32,
    total_tokens: u32,
    tokens_per_sec: f64,
    inter_token_avg_ms: f64,
}

/// Zero-allocation TTFT (Time-To-First-Token) and TPS (Tokens-Per-Second) streaming telemetry tracker.
pub struct StreamTelemetry {
    start_ms: i64,
    first_token_ms: i64,
    last_token_ms: i64,
    has_first_token: bool,
    chunk_count: u32,
    total_tokens: u32,

    /// Starts telemetry recording at timestamp `start_ms`.
    static fn init(start_ms: i64): StreamTelemetry {
        return StreamTelemetry {
            start_ms: start_ms,
            first_token_ms: start_ms,
            last_token_ms: start_ms,
            has_first_token: false,
            chunk_count: 0,
            total_tokens: 0,
        };
    }

    /// Records arrival of a streamed token chunk at `now_ms` containing `tokens`.
    fn onTokenChunk(*_, now_ms: i64, tokens: u32) {
        _.chunk_count = satAddU32(_.chunk_count, 1);
        _.total_tokens = satAddU32(_.total_tokens, tokens);
        if (tokens > 0) {
            if (!_.has_first_token) {
                _.has_first_token = true;
                _.first_token_ms = now_ms;
            }
            _.last_token_ms = now_ms;
        }
    }

    /// Finalizes telemetry at `now_ms` and returns computed `StreamMetrics`.
    fn finish(*_, now_ms: i64): StreamMetrics {
        const total_ms: i64 = if (now_ms >= _.start_ms) (now_ms - _.start_ms) else 0;
        const ttft_ms: i64 = if (!_.has_first_token)
            -1
        else if (_.first_token_ms >= _.start_ms)
            (_.first_token_ms - _.start_ms)
        else
            0;

        const tps: f64 = if (total_ms > 0 && _.total_tokens > 0)
            ((_.total_tokens as f64) * 1000.0) / (total_ms as f64)
        else
            0.0;

        const inter_avg: f64 = if (_.chunk_count > 1 && _.has_first_token && _.last_token_ms >= _.first_token_ms)
            ((_.last_token_ms - _.first_token_ms) as f64) / ((_.chunk_count - 1) as f64)
        else
            0.0;

        return StreamMetrics {
            ttft_ms: ttft_ms,
            total_ms: total_ms,
            chunk_count: _.chunk_count,
            total_tokens: _.total_tokens,
            tokens_per_sec: tps,
            inter_token_avg_ms: inter_avg,
        };
    }
}

/// Zero-allocation lightweight JSON string field extractor for OpenAI/Anthropic streaming deltas
/// (e.g., extracting `"content"` or `"text"`). Decodes `\n`, `\r`, `\t`, `\"`, `\\`, `\/`, `\b`, `\f`, and `\uXXXX`.
/// Returns the number of decoded bytes written into `out_buf`, or `null` if not found, non-string, malformed, or overflowing.
pub fn extractJsonStringField(json_slice: string, field_name: string, out_buf: *[]u8): usize? {
    const n = json_slice.len;
    let i: usize = 0;

    while (i < n) {
        if (json_slice[i] != 34) { // '"'
            i += 1;
            continue;
        }

        // Parse quoted string starting at i + 1
        i += 1;
        const key_start = i;
        let has_escape: bool = false;
        while (i < n && json_slice[i] != 34) {
            if (json_slice[i] == 92) { // '\\'
                has_escape = true;
                i += 2;
            } else {
                i += 1;
            }
        }
        if (i >= n) {
            return null;
        }
        const key_end = i;
        i += 1; // skip closing '"'

        // Skip whitespace after string
        let p = i;
        while (p < n && isJsonWs(json_slice[p])) {
            p += 1;
        }

        // Not an object key if not followed by ':'
        if (p >= n || json_slice[p] != 58) {
            continue;
        }

        // Check key match
        if (has_escape || (key_end - key_start) != field_name.len) {
            continue;
        }
        let matched: bool = true;
        let k: usize = 0;
        while (k < field_name.len) {
            if (json_slice[key_start + k] != field_name[k]) {
                matched = false;
                break;
            }
            k += 1;
        }
        if (!matched) {
            continue;
        }

        // Skip ':' and whitespace
        p += 1;
        while (p < n && isJsonWs(json_slice[p])) {
            p += 1;
        }
        if (p >= n || json_slice[p] != 34) {
            return null;
        }
        p += 1; // enter value string

        let out_len: usize = 0;
        while (p < n) {
            const c = json_slice[p];
            if (c == 34) { // closing '"'
                return out_len;
            }
            if (c == 92) { // '\\'
                p += 1;
                if (p >= n) {
                    return null;
                }
                const esc = json_slice[p];
                if (esc == 117) { // 'u' -> \uXXXX
                    if (p + 4 >= n) {
                        return null;
                    }
                    const d0 = hexDigitVal(json_slice[p + 1]) ?? return null;
                    const d1 = hexDigitVal(json_slice[p + 2]) ?? return null;
                    const d2 = hexDigitVal(json_slice[p + 3]) ?? return null;
                    const d3 = hexDigitVal(json_slice[p + 4]) ?? return null;
                    const cp: u16 = (d0 << 12) | (d1 << 8) | (d2 << 4) | d3;
                    p += 5;

                    if (cp < 0x80) {
                        if (out_len + 1 > out_buf.*.len) return null;
                        out_buf.*[out_len] = cp as u8;
                        out_len += 1;
                    } else if (cp < 0x800) {
                        if (out_len + 2 > out_buf.*.len) return null;
                        out_buf.*[out_len] = (0xC0 | (cp >> 6)) as u8;
                        out_buf.*[out_len + 1] = (0x80 | (cp & 0x3F)) as u8;
                        out_len += 2;
                    } else {
                        if (out_len + 3 > out_buf.*.len) return null;
                        out_buf.*[out_len] = (0xE0 | (cp >> 12)) as u8;
                        out_buf.*[out_len + 1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
                        out_buf.*[out_len + 2] = (0x80 | (cp & 0x3F)) as u8;
                        out_len += 3;
                    }
                    continue;
                }

                let decoded: u8 = 0;
                if (esc == 110) {
                    decoded = 10; // \n
                } else if (esc == 114) {
                    decoded = 13; // \r
                } else if (esc == 116) {
                    decoded = 9; // \t
                } else if (esc == 34) {
                    decoded = 34; // \"
                } else if (esc == 92) {
                    decoded = 92; // \\
                } else if (esc == 47) {
                    decoded = 47; // \/
                } else if (esc == 98) {
                    decoded = 8; // \b
                } else if (esc == 102) {
                    decoded = 12; // \f
                } else {
                    return null;
                }
                if (out_len >= out_buf.*.len) {
                    return null;
                }
                out_buf.*[out_len] = decoded;
                out_len += 1;
                p += 1;
            } else {
                if (out_len >= out_buf.*.len) {
                    return null;
                }
                out_buf.*[out_len] = c;
                out_len += 1;
                p += 1;
            }
        }
        return null;
    }

    return null;
}
