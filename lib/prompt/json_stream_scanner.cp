// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Streaming JSON Incremental Field Scanner & Tool-Call Extractor) | Created: 2026-11-22
// Pure CP zero-heap-allocation resumable byte-by-byte / chunked JSON object stream scanner for
// real-time LLM Tool-Call argument extraction:
//   - Resumable across arbitrary 1-byte to multi-KB chunks with cross-chunk escape decoding
//   - Emits `key_ready`, real-time `string_delta`, `field_complete`, `object_complete`, and `syntax_error`
//   - Supports top-level string, number, boolean, null, nested `{...}`, and nested `[...]` values
//   - Enforces max nesting depth (`JSON_STREAM_MAX_DEPTH = 16`), bracket matching, and control-char guards

const std = @import("std");

// ============================================================================
// 1. Constants & Public Enums / Models
// ============================================================================

/// Maximum total JSON nesting depth (root object = depth 1, up to 16 levels total).
pub const JSON_STREAM_MAX_DEPTH: usize = 16;
/// Maximum inline byte length for a top-level JSON object key.
pub const JSON_STREAM_MAX_KEY_LEN: usize = 64;
/// Maximum inline byte length for a captured field value or delta slice.
pub const JSON_STREAM_MAX_VAL_LEN: usize = 256;
/// Recommended maximum event batch size per `feed` call.
pub const JSON_STREAM_MAX_EVENTS: usize = 32;

/// Event kind emitted incrementally by `JsonStreamScanner.feed`.
pub enum JsonStreamEventKind {
    key_ready,
    string_delta,
    field_complete,
    object_complete,
    syntax_error,
}

/// JSON value type classification for top-level fields.
pub enum JsonStreamValueKind {
    string,
    number,
    boolean,
    null_val,
    nested_object,
    nested_array,
}

/// Inline zero-alloc event record written into caller-provided `out_events`.
pub struct JsonStreamEvent {
    kind: JsonStreamEventKind = JsonStreamEventKind.key_ready;
    val_kind: JsonStreamValueKind = JsonStreamValueKind.string;
    key_buf: [JSON_STREAM_MAX_KEY_LEN]u8 = [0; JSON_STREAM_MAX_KEY_LEN];
    key_len: usize = 0;
    data_buf: [JSON_STREAM_MAX_VAL_LEN]u8 = [0; JSON_STREAM_MAX_VAL_LEN];
    data_len: usize = 0;
    depth: usize = 0;

    /// Returns a slice view of the field key associated with this event.
    fn keySlice(): []const u8 {
        const n = if (_.key_len <= JSON_STREAM_MAX_KEY_LEN) _.key_len else JSON_STREAM_MAX_KEY_LEN;
        return _.key_buf[0..n];
    }

    /// Returns a slice view of the delta or completed value payload associated with this event.
    fn dataSlice(): []const u8 {
        const n = if (_.data_len <= JSON_STREAM_MAX_VAL_LEN) _.data_len else JSON_STREAM_MAX_VAL_LEN;
        return _.data_buf[0..n];
    }
}

/// Cumulative operational statistics for `JsonStreamScanner`.
pub struct JsonStreamStats {
    bytes_processed: u64 = 0;
    fields_completed: usize = 0;
    delta_events: usize = 0;
    completed: bool = false;
    has_error: bool = false;
}

// ============================================================================
// 2. Internal State & Helpers
// ============================================================================

enum ScanPhase {
    expect_open_brace,
    expect_key_or_end,
    expect_key,
    in_key,
    expect_colon,
    expect_value,
    in_str_val,
    in_primitive,
    in_nested,
    expect_comma_or_end,
    done,
    failed,
}

fn isJsonWs(b: u8): bool {
    return b == 32 || b == 9 || b == 10 || b == 13;
}

fn decodeSimpleEscape(b: u8): u8? {
    return switch (b) {
        34 => 34,  // \"
        92 => 92,  // \\
        47 => 47,  // \/
        98 => 8,   // \b
        102 => 12, // \f
        110 => 10, // \n
        114 => 13, // \r
        116 => 9,  // \t
        else => null,
    };
}

fn isValidNumberToken(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    if (s[i] == 45) { // '-'
        i += 1;
        if (i >= s.len) return false;
    }
    if (s[i] == 48) { // '0'
        i += 1;
    } else if (s[i] >= 49 && s[i] <= 57) {
        while (i < s.len && s[i] >= 48 && s[i] <= 57) : (i += 1) {}
    } else {
        return false;
    }
    if (i < s.len && s[i] == 46) { // '.'
        i += 1;
        if (i >= s.len || s[i] < 48 || s[i] > 57) return false;
        while (i < s.len && s[i] >= 48 && s[i] <= 57) : (i += 1) {}
    }
    if (i < s.len && (s[i] == 101 || s[i] == 69)) { // 'e' / 'E'
        i += 1;
        if (i < s.len && (s[i] == 43 || s[i] == 45)) {
            i += 1;
        }
        if (i >= s.len || s[i] < 48 || s[i] > 57) return false;
        while (i < s.len && s[i] >= 48 && s[i] <= 57) : (i += 1) {}
    }
    return i == s.len;
}

fn validatePrimitive(kind: JsonStreamValueKind, s: []const u8): bool {
    return switch (kind) {
        JsonStreamValueKind.boolean => std.mem.eql(u8, s, "true") || std.mem.eql(u8, s, "false"),
        JsonStreamValueKind.null_val => std.mem.eql(u8, s, "null"),
        JsonStreamValueKind.number => isValidNumberToken(s),
        else => false,
    };
}

// ============================================================================
// 3. Zero-Alloc Resumable Streaming JSON Object Scanner
// ============================================================================

/// Zero-heap-allocation resumable JSON object stream scanner for LLM Tool-Call extraction.
pub struct JsonStreamScanner {
    phase: ScanPhase = ScanPhase.expect_open_brace;
    depth: usize = 0;
    escape_pending: bool = false;
    nested_in_str: bool = false;
    nested_escape: bool = false;
    nested_stack: [JSON_STREAM_MAX_DEPTH]u8 = [0; JSON_STREAM_MAX_DEPTH];
    nested_top: usize = 0;

    key_buf: [JSON_STREAM_MAX_KEY_LEN]u8 = [0; JSON_STREAM_MAX_KEY_LEN];
    key_len: usize = 0;
    val_buf: [JSON_STREAM_MAX_VAL_LEN]u8 = [0; JSON_STREAM_MAX_VAL_LEN];
    val_len: usize = 0;
    cur_val_kind: JsonStreamValueKind = JsonStreamValueKind.string;

    bytes_processed: u64 = 0;
    fields_completed: usize = 0;
    delta_events: usize = 0;

    /// Initializes a fresh streaming JSON scanner expecting a top-level `{ ... }` object.
    static fn init(): JsonStreamScanner {
        return JsonStreamScanner{};
    }

    /// Resets all scanner state and counters for clean reuse.
    fn reset(*_) {
        _.phase = ScanPhase.expect_open_brace;
        _.depth = 0;
        _.escape_pending = false;
        _.nested_in_str = false;
        _.nested_escape = false;
        _.nested_top = 0;
        _.key_len = 0;
        _.val_len = 0;
        _.cur_val_kind = JsonStreamValueKind.string;
        _.bytes_processed = 0;
        _.fields_completed = 0;
        _.delta_events = 0;
    }

    fn emitEvent(
        *_,
        out_events: *[]JsonStreamEvent,
        emitted: *usize,
        kind: JsonStreamEventKind,
        val_kind: JsonStreamValueKind,
        data_src: []const u8,
    ) {
        if (emitted.* >= out_events.len) {
            return;
        }
        let ev = JsonStreamEvent{
            kind: kind,
            val_kind: val_kind,
            key_buf: [0; JSON_STREAM_MAX_KEY_LEN],
            key_len: _.key_len,
            data_buf: [0; JSON_STREAM_MAX_VAL_LEN],
            data_len: if (data_src.len <= JSON_STREAM_MAX_VAL_LEN) data_src.len else JSON_STREAM_MAX_VAL_LEN,
            depth: _.depth,
        };
        let k: usize = 0;
        while (k < ev.key_len) : (k += 1) {
            ev.key_buf[k] = _.key_buf[k];
        }
        let d: usize = 0;
        while (d < ev.data_len) : (d += 1) {
            ev.data_buf[d] = data_src[d];
        }
        out_events[emitted.*] = ev;
        emitted.* += 1;
    }

    fn failStream(*_, out_events: *[]JsonStreamEvent, emitted: *usize) {
        _.phase = ScanPhase.failed;
        _.emitEvent(out_events, emitted, JsonStreamEventKind.syntax_error, _.cur_val_kind, "");
    }

    fn flushStringDelta(
        *_,
        out_events: *[]JsonStreamEvent,
        emitted: *usize,
        delta_buf: []const u8,
        delta_len: *usize,
    ) {
        if (delta_len.* == 0) {
            return;
        }
        _.delta_events += 1;
        _.emitEvent(out_events, emitted, JsonStreamEventKind.string_delta, JsonStreamValueKind.string, delta_buf[0..delta_len.*]);
        delta_len.* = 0;
    }

    /// Feeds an arbitrary byte slice `chunk` into the scanner, emitting incremental events
    /// into `out_events`. Returns the number of events written.
    fn feed(*_, chunk: []const u8, out_events: *[]JsonStreamEvent): usize {
        if (_.phase == ScanPhase.failed) {
            return 0;
        }

        let emitted: usize = 0;
        let delta_buf: [JSON_STREAM_MAX_VAL_LEN]u8 = [0; JSON_STREAM_MAX_VAL_LEN];
        let delta_len: usize = 0;

        let i: usize = 0;
        while (i < chunk.len) : (i += 1) {
            const b = chunk[i];
            _.bytes_processed += 1;

            switch (_.phase) {
                ScanPhase.expect_open_brace => {
                    if (isJsonWs(b)) {
                        continue;
                    }
                    if (b == 123) { // '{'
                        _.depth = 1;
                        _.phase = ScanPhase.expect_key_or_end;
                    } else {
                        _.failStream(out_events, &emitted);
                        return emitted;
                    }
                },
                ScanPhase.expect_key_or_end => {
                    if (isJsonWs(b)) {
                        continue;
                    }
                    if (b == 125) { // '}'
                        _.depth = 0;
                        _.key_len = 0;
                        _.phase = ScanPhase.done;
                        _.emitEvent(out_events, &emitted, JsonStreamEventKind.object_complete, JsonStreamValueKind.nested_object, "");
                    } else if (b == 34) { // '"'
                        _.key_len = 0;
                        _.escape_pending = false;
                        _.phase = ScanPhase.in_key;
                    } else {
                        _.failStream(out_events, &emitted);
                        return emitted;
                    }
                },
                ScanPhase.expect_key => {
                    if (isJsonWs(b)) {
                        continue;
                    }
                    if (b == 34) { // '"'
                        _.key_len = 0;
                        _.escape_pending = false;
                        _.phase = ScanPhase.in_key;
                    } else {
                        _.failStream(out_events, &emitted);
                        return emitted;
                    }
                },
                ScanPhase.in_key => {
                    if (_.escape_pending) {
                        _.escape_pending = false;
                        const dec_opt = decodeSimpleEscape(b);
                        if (dec_opt == null) {
                            _.failStream(out_events, &emitted);
                            return emitted;
                        }
                        const ch = dec_opt ?? 0;
                        if (_.key_len < JSON_STREAM_MAX_KEY_LEN) {
                            _.key_buf[_.key_len] = ch;
                            _.key_len += 1;
                        }
                    } else if (b == 92) { // '\\'
                        _.escape_pending = true;
                    } else if (b == 34) { // '"'
                        _.emitEvent(out_events, &emitted, JsonStreamEventKind.key_ready, JsonStreamValueKind.string, "");
                        _.phase = ScanPhase.expect_colon;
                    } else if (b < 0x20) {
                        _.failStream(out_events, &emitted);
                        return emitted;
                    } else {
                        if (_.key_len < JSON_STREAM_MAX_KEY_LEN) {
                            _.key_buf[_.key_len] = b;
                            _.key_len += 1;
                        }
                    }
                },
                ScanPhase.expect_colon => {
                    if (isJsonWs(b)) {
                        continue;
                    }
                    if (b == 58) { // ':'
                        _.phase = ScanPhase.expect_value;
                    } else {
                        _.failStream(out_events, &emitted);
                        return emitted;
                    }
                },
                ScanPhase.expect_value => {
                    if (isJsonWs(b)) {
                        continue;
                    }
                    _.val_len = 0;
                    if (b == 34) { // '"'
                        _.cur_val_kind = JsonStreamValueKind.string;
                        _.escape_pending = false;
                        _.phase = ScanPhase.in_str_val;
                    } else if (b == 123 || b == 91) { // '{' or '['
                        _.depth = 2;
                        if (_.depth > JSON_STREAM_MAX_DEPTH) {
                            _.failStream(out_events, &emitted);
                            return emitted;
                        }
                        _.cur_val_kind = if (b == 123) JsonStreamValueKind.nested_object else JsonStreamValueKind.nested_array;
                        _.nested_stack[0] = b;
                        _.nested_top = 1;
                        _.nested_in_str = false;
                        _.nested_escape = false;
                        _.val_buf[0] = b;
                        _.val_len = 1;
                        _.phase = ScanPhase.in_nested;
                    } else if (b == 116 || b == 102) { // 't' or 'f'
                        _.cur_val_kind = JsonStreamValueKind.boolean;
                        _.val_buf[0] = b;
                        _.val_len = 1;
                        _.phase = ScanPhase.in_primitive;
                    } else if (b == 110) { // 'n'
                        _.cur_val_kind = JsonStreamValueKind.null_val;
                        _.val_buf[0] = b;
                        _.val_len = 1;
                        _.phase = ScanPhase.in_primitive;
                    } else if (b == 45 || (b >= 48 && b <= 57)) { // '-' or '0'..'9'
                        _.cur_val_kind = JsonStreamValueKind.number;
                        _.val_buf[0] = b;
                        _.val_len = 1;
                        _.phase = ScanPhase.in_primitive;
                    } else {
                        _.failStream(out_events, &emitted);
                        return emitted;
                    }
                },
                ScanPhase.in_str_val => {
                    if (_.escape_pending) {
                        _.escape_pending = false;
                        const dec_opt = decodeSimpleEscape(b);
                        if (dec_opt == null) {
                            _.failStream(out_events, &emitted);
                            return emitted;
                        }
                        const ch = dec_opt ?? 0;
                        if (_.val_len < JSON_STREAM_MAX_VAL_LEN) {
                            _.val_buf[_.val_len] = ch;
                            _.val_len += 1;
                        }
                        if (delta_len >= JSON_STREAM_MAX_VAL_LEN) {
                            _.flushStringDelta(out_events, &emitted, delta_buf[0..], &delta_len);
                        }
                        delta_buf[delta_len] = ch;
                        delta_len += 1;
                    } else if (b == 92) { // '\\'
                        _.escape_pending = true;
                    } else if (b == 34) { // '"'
                        _.flushStringDelta(out_events, &emitted, delta_buf[0..], &delta_len);
                        _.fields_completed += 1;
                        _.emitEvent(out_events, &emitted, JsonStreamEventKind.field_complete, JsonStreamValueKind.string, _.val_buf[0.._.val_len]);
                        _.phase = ScanPhase.expect_comma_or_end;
                    } else if (b < 0x20) {
                        _.failStream(out_events, &emitted);
                        return emitted;
                    } else {
                        if (_.val_len < JSON_STREAM_MAX_VAL_LEN) {
                            _.val_buf[_.val_len] = b;
                            _.val_len += 1;
                        }
                        if (delta_len >= JSON_STREAM_MAX_VAL_LEN) {
                            _.flushStringDelta(out_events, &emitted, delta_buf[0..], &delta_len);
                        }
                        delta_buf[delta_len] = b;
                        delta_len += 1;
                    }
                },
                ScanPhase.in_primitive => {
                    if (b == 44 || b == 125 || isJsonWs(b)) { // ',', '}', or whitespace terminates primitive
                        if (!validatePrimitive(_.cur_val_kind, _.val_buf[0.._.val_len])) {
                            _.failStream(out_events, &emitted);
                            return emitted;
                        }
                        _.fields_completed += 1;
                        _.emitEvent(out_events, &emitted, JsonStreamEventKind.field_complete, _.cur_val_kind, _.val_buf[0.._.val_len]);
                        if (b == 44) {
                            _.phase = ScanPhase.expect_key;
                        } else if (b == 125) {
                            _.depth = 0;
                            _.key_len = 0;
                            _.phase = ScanPhase.done;
                            _.emitEvent(out_events, &emitted, JsonStreamEventKind.object_complete, JsonStreamValueKind.nested_object, "");
                        } else {
                            _.phase = ScanPhase.expect_comma_or_end;
                        }
                    } else {
                        if (_.val_len < JSON_STREAM_MAX_VAL_LEN) {
                            _.val_buf[_.val_len] = b;
                            _.val_len += 1;
                        } else {
                            _.failStream(out_events, &emitted);
                            return emitted;
                        }
                    }
                },
                ScanPhase.in_nested => {
                    if (_.val_len < JSON_STREAM_MAX_VAL_LEN) {
                        _.val_buf[_.val_len] = b;
                        _.val_len += 1;
                    }
                    if (_.nested_in_str) {
                        if (_.nested_escape) {
                            _.nested_escape = false;
                            if (decodeSimpleEscape(b) == null) {
                                _.failStream(out_events, &emitted);
                                return emitted;
                            }
                        } else if (b == 92) {
                            _.nested_escape = true;
                        } else if (b == 34) {
                            _.nested_in_str = false;
                        } else if (b < 0x20) {
                            _.failStream(out_events, &emitted);
                            return emitted;
                        }
                    } else {
                        if (b == 34) {
                            _.nested_in_str = true;
                        } else if (b == 123 || b == 91) { // '{' or '['
                            _.depth += 1;
                            if (_.depth > JSON_STREAM_MAX_DEPTH || _.nested_top >= JSON_STREAM_MAX_DEPTH) {
                                _.failStream(out_events, &emitted);
                                return emitted;
                            }
                            _.nested_stack[_.nested_top] = b;
                            _.nested_top += 1;
                        } else if (b == 125 || b == 93) { // '}' or ']'
                            if (_.nested_top == 0) {
                                _.failStream(out_events, &emitted);
                                return emitted;
                            }
                            const open = _.nested_stack[_.nested_top - 1];
                            const ok_pair = (open == 123 && b == 125) || (open == 91 && b == 93);
                            if (!ok_pair) {
                                _.failStream(out_events, &emitted);
                                return emitted;
                            }
                            _.nested_top -= 1;
                            _.depth -= 1;
                            if (_.nested_top == 0) {
                                _.fields_completed += 1;
                                _.emitEvent(out_events, &emitted, JsonStreamEventKind.field_complete, _.cur_val_kind, _.val_buf[0.._.val_len]);
                                _.phase = ScanPhase.expect_comma_or_end;
                            }
                        }
                    }
                },
                ScanPhase.expect_comma_or_end => {
                    if (isJsonWs(b)) {
                        continue;
                    }
                    if (b == 44) { // ','
                        _.phase = ScanPhase.expect_key;
                    } else if (b == 125) { // '}'
                        _.depth = 0;
                        _.key_len = 0;
                        _.phase = ScanPhase.done;
                        _.emitEvent(out_events, &emitted, JsonStreamEventKind.object_complete, JsonStreamValueKind.nested_object, "");
                    } else {
                        _.failStream(out_events, &emitted);
                        return emitted;
                    }
                },
                ScanPhase.done => {
                    if (!isJsonWs(b)) {
                        _.failStream(out_events, &emitted);
                        return emitted;
                    }
                },
                ScanPhase.failed => {
                    return emitted;
                },
            }
        }

        // Flush any pending streaming string delta at chunk boundary for zero-latency preview
        if (_.phase == ScanPhase.in_str_val && delta_len > 0) {
            _.flushStringDelta(out_events, &emitted, delta_buf[0..], &delta_len);
        }

        return emitted;
    }

    /// Returns `true` once the top-level JSON object has cleanly closed without errors.
    fn is_complete(): bool {
        return _.phase == ScanPhase.done;
    }

    /// Returns `true` if a syntax or nesting error was encountered.
    fn has_error(): bool {
        return _.phase == ScanPhase.failed;
    }

    /// Returns a cumulative snapshot of bytes processed, completed fields, and delta count.
    fn stats(): JsonStreamStats {
        return JsonStreamStats{
            bytes_processed: _.bytes_processed,
            fields_completed: _.fields_completed,
            delta_events: _.delta_events,
            completed: _.phase == ScanPhase.done,
            has_error: _.phase == ScanPhase.failed,
        };
    }
}
