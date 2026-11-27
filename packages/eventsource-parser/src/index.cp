// packages/@cap/sse-parse/src/index.cp
// Zero-allocation W3C Server-Sent Events (text/event-stream) streaming parser.
// MIT clean-room pure CP implementation.

fn sseBytesEq(a: []const u8, b: []const u8): bool {
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

/// Zero-allocation incremental W3C Server-Sent Events parser.
/// Handles arbitrary TCP chunking byte-by-byte with fixed internal buffers.
pub struct SseParser {
    pub data_buf: [2048]u8,
    pub data_len: usize,
    pub event_buf: [128]u8,
    pub event_len: usize,
    pub id_buf: [128]u8,
    pub id_len: usize,
    pub line_buf: [512]u8,
    pub line_len: usize,
    pub has_data: bool,
    pub retry_ms: i64,
    pub pending_cr: bool,

    pub static fn init(): SseParser {
        let db: [2048]u8 = undefined;
        let eb: [128]u8 = undefined;
        let ib: [128]u8 = undefined;
        let lb: [512]u8 = undefined;
        return SseParser{
            data_buf: db,
            data_len: 0,
            event_buf: eb,
            event_len: 0,
            id_buf: ib,
            id_len: 0,
            line_buf: lb,
            line_len: 0,
            has_data: false,
            retry_ms: -1 as i64,
            pending_cr: false,
        };
    }

    fn processLine(*_): i32 {
        const n = _.line_len;
        _.line_len = 0;

        // Blank line dispatches accumulated event if any `data:` field was seen.
        if (n == 0) {
            if (_.has_data) {
                return 1 as i32;
            }
            // Empty block without data resets stray event name per W3C SSE spec.
            _.event_len = 0;
            return 0 as i32;
        }

        // Comment line starting with ':' (e.g. `:keep-alive`) is ignored.
        if (_.line_buf[0] == (':' as u8)) {
            return 0 as i32;
        }

        // Split on first ':'.
        let colon: usize = 0;
        while (colon < n && _.line_buf[colon] != (':' as u8)) {
            colon += 1;
        }

        let fieldEnd = colon;
        let valStart = n;
        if (colon < n) {
            valStart = colon + 1;
            // Strip at most one leading U+0020 SPACE after ':'.
            if (valStart < n && _.line_buf[valStart] == (' ' as u8)) {
                valStart += 1;
            }
        }
        const valLen = n - valStart;
        const field = _.line_buf[0..fieldEnd];

        if (sseBytesEq(field, "data")) {
            let extra: usize = 0;
            if (_.has_data) {
                extra = 1;
            }
            if (_.data_len + extra + valLen > 2048) {
                return -1 as i32;
            }
            if (_.has_data) {
                _.data_buf[_.data_len] = '\n' as u8;
                _.data_len += 1;
            }
            let i: usize = 0;
            while (i < valLen) {
                _.data_buf[_.data_len] = _.line_buf[valStart + i];
                _.data_len += 1;
                i += 1;
            }
            _.has_data = true;
        } else if (sseBytesEq(field, "event")) {
            if (valLen > 128) {
                return -1 as i32;
            }
            let i: usize = 0;
            while (i < valLen) {
                _.event_buf[i] = _.line_buf[valStart + i];
                i += 1;
            }
            _.event_len = valLen;
        } else if (sseBytesEq(field, "id")) {
            if (valLen > 0) {
                if (valLen > 128) {
                    return -1 as i32;
                }
                // W3C spec: ignore `id` containing NUL (0x00).
                let hasNul = false;
                let ci: usize = 0;
                while (ci < valLen) {
                    if (_.line_buf[valStart + ci] == (0 as u8)) {
                        hasNul = true;
                        break;
                    }
                    ci += 1;
                }
                if (!hasNul) {
                    let i: usize = 0;
                    while (i < valLen) {
                        _.id_buf[i] = _.line_buf[valStart + i];
                        i += 1;
                    }
                    _.id_len = valLen;
                }
            }
        } else if (sseBytesEq(field, "retry")) {
            if (valLen > 0) {
                let validDigits = true;
                let acc: i64 = 0;
                let i: usize = 0;
                while (i < valLen) {
                    const c = _.line_buf[valStart + i];
                    if (c < ('0' as u8) || c > ('9' as u8)) {
                        validDigits = false;
                        break;
                    }
                    if (acc <= (100000000000 as i64)) {
                        acc = acc * (10 as i64) + ((c - ('0' as u8)) as i64);
                    }
                    i += 1;
                }
                if (validDigits) {
                    _.retry_ms = acc;
                }
            }
        }

        return 0 as i32;
    }

    /// Feed one byte from the event stream.
    /// Returns:
    /// - `0`: keep feeding bytes
    /// - `1`: a complete SSE event is ready (read via `eventType`/`eventData`/`lastEventId`, then call `consumeEvent`)
    /// - `-1`: internal buffer overflow
    pub fn pushByte(*_, b: u8): i32 {
        if (_.pending_cr) {
            _.pending_cr = false;
            if (b == ('\n' as u8)) {
                return 0 as i32;
            }
        }

        if (b == ('\r' as u8)) {
            _.pending_cr = true;
            return _.processLine();
        }
        if (b == ('\n' as u8)) {
            return _.processLine();
        }

        if (_.line_len >= 512) {
            return -1 as i32;
        }
        _.line_buf[_.line_len] = b;
        _.line_len += 1;
        return 0 as i32;
    }

    /// Current event type (defaults to `"message"` when no `event:` field was specified).
    pub fn eventType(*_): []const u8 {
        if (_.event_len == 0) {
            return "message";
        }
        return _.event_buf[0.._.event_len];
    }

    /// Accumulated event payload (multiple `data:` lines joined by `\n`).
    pub fn eventData(*_): []const u8 {
        return _.data_buf[0.._.data_len];
    }

    /// Persisted last non-empty `id:` value across events.
    pub fn lastEventId(*_): []const u8 {
        return _.id_buf[0.._.id_len];
    }

    /// Most recent valid `retry:` value in milliseconds, or `-1` if unset.
    pub fn retryMs(): i64 {
        return _.retry_ms;
    }

    /// Clear current event data and event type while preserving `last_event_id` and `retry_ms`.
    pub fn consumeEvent(*_) {
        _.data_len = 0;
        _.event_len = 0;
        _.has_data = false;
    }

    /// Reset all parser state including `last_event_id` and `retry_ms`.
    pub fn reset(*_) {
        _.data_len = 0;
        _.event_len = 0;
        _.id_len = 0;
        _.line_len = 0;
        _.has_data = false;
        _.retry_ms = -1 as i64;
        _.pending_cr = false;
    }
}

export {
    SseParser,
};
