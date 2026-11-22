// Origin: Pure-CP Stdlib (Zero-Alloc Streaming LLM Tool-Call Extractor & State Machine) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation incremental streaming tool-call parser:
//   - Supports arbitrary chunking (down to 1-byte chunks) via feed(chunk) + finish()
//   - Extracts <tool_call>...</tool_call>, Markdown ```json ... ``` blocks, and bare JSON objects
//   - Brace/string/escape depth FSM safely handles nested braces inside quotes, escaped quotes & UTF-8
//   - Separates natural language text from concurrent tool calls with complete/truncated/malformed status

/// Maximum number of tool calls extracted per assistant turn.
pub const TOOL_CALL_MAX_CALLS: usize = 16;

/// Maximum byte length of an extracted tool name.
pub const TOOL_CALL_NAME_MAX_LEN: usize = 64;

/// Maximum byte length of an optional tool call ID.
pub const TOOL_CALL_ID_MAX_LEN: usize = 64;

/// Maximum byte length of raw JSON arguments per tool call.
pub const TOOL_CALL_ARGS_MAX_LEN: usize = 512;

/// Maximum byte length of separated natural language text buffer.
pub const TOOL_CALL_TEXT_MAX_LEN: usize = 1024;

/// Maximum byte length of the internal working buffer per candidate tool call.
pub const TOOL_CALL_WORK_BUF_LEN: usize = 1024;

// ============================================================================
// Enums & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Completeness status of an extracted tool call.
pub enum ToolCallStatus: u8 {
    complete = 0,
    truncated = 1,
    malformed = 2,
}

/// Syntactic wrapper from which the tool call was extracted.
pub enum ToolCallWrapperKind: u8 {
    xml_tag = 0,
    code_fence = 1,
    bare_json = 2,
}

/// Extracted tool call entry with name, optional id, raw arguments JSON, and status.
pub struct ParsedToolCall {
    name_buf: [TOOL_CALL_NAME_MAX_LEN]u8 = [0; TOOL_CALL_NAME_MAX_LEN];
    name_len: usize = 0;
    id_buf: [TOOL_CALL_ID_MAX_LEN]u8 = [0; TOOL_CALL_ID_MAX_LEN];
    id_len: usize = 0;
    args_buf: [TOOL_CALL_ARGS_MAX_LEN]u8 = [0; TOOL_CALL_ARGS_MAX_LEN];
    args_len: usize = 0;
    status: ToolCallStatus = ToolCallStatus.complete;
    wrapper: ToolCallWrapperKind = ToolCallWrapperKind.bare_json;

    static fn empty(): ParsedToolCall {
        return ParsedToolCall{};
    }

    fn nameSlice(): []const u8 {
        const n: usize = if (_.name_len <= TOOL_CALL_NAME_MAX_LEN) _.name_len else TOOL_CALL_NAME_MAX_LEN;
        return _.name_buf[0..n];
    }

    fn idSlice(): []const u8 {
        const n: usize = if (_.id_len <= TOOL_CALL_ID_MAX_LEN) _.id_len else TOOL_CALL_ID_MAX_LEN;
        return _.id_buf[0..n];
    }

    fn argsJsonSlice(): []const u8 {
        const n: usize = if (_.args_len <= TOOL_CALL_ARGS_MAX_LEN) _.args_len else TOOL_CALL_ARGS_MAX_LEN;
        return _.args_buf[0..n];
    }
}

/// Telemetry snapshot for `ToolCallStreamParser`.
pub struct ToolCallParserStats {
    bytes_fed: u64 = 0;
    text_bytes_emitted: usize = 0;
    text_bytes_dropped: usize = 0;
    total_calls: usize = 0;
    complete_calls: usize = 0;
    truncated_calls: usize = 0;
    malformed_calls: usize = 0;
    dropped_calls: usize = 0;
}

enum ToolParseMode: u8 {
    normal_text = 0,
    match_open_tag = 1,
    match_open_fence = 2,
    in_xml_body = 3,
    in_fence_body = 4,
    in_bare_json = 5,
}

// ============================================================================
// Internal JSON Field Extraction Helpers (Zero-Heap)
// ============================================================================

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn endsWith(buf: []const u8, suffix: []const u8): bool {
    if (buf.len < suffix.len) return false;
    const off: usize = buf.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) : (i += 1) {
        if (buf[off + i] != suffix[i]) return false;
    }
    return true;
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

/// Finds the value start offset of a top-level JSON key inside object `obj` (starting at '{').
fn findTopLevelKeyValueStart(obj: []const u8, target_key: []const u8): usize? {
    let p: usize = 0;
    while (p < obj.len && isWs(obj[p])) : (p += 1) {}
    if (p >= obj.len || obj[p] != '{') return null;
    p += 1;

    let depth: usize = 1;
    let in_str: bool = false;
    let esc: bool = false;

    while (p < obj.len && depth > 0) {
        const c: u8 = obj[p];
        if (in_str) {
            if (esc) {
                esc = false;
            } else if (c == '\\') {
                esc = true;
            } else if (c == '"') {
                in_str = false;
            }
            p += 1;
            continue;
        }

        if (depth == 1 && c == '"') {
            // Potential top-level key string
            const k_start: usize = p + 1;
            p += 1;
            let k_esc: bool = false;
            while (p < obj.len) : (p += 1) {
                if (k_esc) {
                    k_esc = false;
                    continue;
                }
                if (obj[p] == '\\') {
                    k_esc = true;
                    continue;
                }
                if (obj[p] == '"') break;
            }
            if (p >= obj.len) return null;
            const k_end: usize = p;
            p += 1; // skip closing quote
            while (p < obj.len && isWs(obj[p])) : (p += 1) {}
            if (p < obj.len && obj[p] == ':') {
                p += 1;
                while (p < obj.len && isWs(obj[p])) : (p += 1) {}
                if (bytesEq(obj[k_start..k_end], target_key)) {
                    if (p < obj.len) return p;
                    return null;
                }
            }
            continue;
        }

        if (c == '"') {
            in_str = true;
        } else if (c == '{' || c == '[') {
            depth += 1;
        } else if (c == '}' || c == ']') {
            depth -= 1;
        }
        p += 1;
    }
    return null;
}

/// Decodes a JSON string literal starting at `obj[start] == '"'` into `out`, returning decoded length.
fn decodeJsonStringAt(obj: []const u8, start: usize, out: *[]u8): usize? {
    if (start >= obj.len || obj[start] != '"') return null;
    let p: usize = start + 1;
    let w: usize = 0;
    while (p < obj.len) : (p += 1) {
        const c: u8 = obj[p];
        if (c == '"') return w;
        if (c == '\\') {
            p += 1;
            if (p >= obj.len) return null;
            const e: u8 = obj[p];
            let ch: u8 = e;
            if (e == 'n') ch = '\n' as u8;
            else if (e == 'r') ch = '\r' as u8;
            else if (e == 't') ch = '\t' as u8;
            else if (e == '"' || e == '\\' || e == '/') ch = e;
            if (w >= out.len) return null;
            out[w] = ch;
            w += 1;
        } else {
            if (w >= out.len) return null;
            out[w] = c;
            w += 1;
        }
    }
    return null;
}

/// Extracts the raw JSON value span starting at `obj[start]`.
/// Returns `(end_offset, is_complete)`.
fn measureJsonValueSpan(obj: []const u8, start: usize): [2]usize {
    if (start >= obj.len) return [start, 0];
    const first: u8 = obj[start];
    if (first == '{' || first == '[') {
        let p: usize = start;
        let depth: usize = 0;
        let in_str: bool = false;
        let esc: bool = false;
        while (p < obj.len) : (p += 1) {
            const c: u8 = obj[p];
            if (in_str) {
                if (esc) {
                    esc = false;
                } else if (c == '\\') {
                    esc = true;
                } else if (c == '"') {
                    in_str = false;
                }
                continue;
            }
            if (c == '"') {
                in_str = true;
            } else if (c == '{' || c == '[') {
                depth += 1;
            } else if (c == '}' || c == ']') {
                if (depth > 0) depth -= 1;
                if (depth == 0) return [p + 1, 1];
            }
        }
        return [obj.len, 0]; // truncated!
    }
    if (first == '"') {
        let p: usize = start + 1;
        let esc: bool = false;
        while (p < obj.len) : (p += 1) {
            const c: u8 = obj[p];
            if (esc) {
                esc = false;
            } else if (c == '\\') {
                esc = true;
            } else if (c == '"') {
                return [p + 1, 1];
            }
        }
        return [obj.len, 0];
    }
    // Primitive value (number/bool/null)
    let p: usize = start;
    while (p < obj.len && obj[p] != ',' && obj[p] != '}' && obj[p] != ']' && !isWs(obj[p])) : (p += 1) {}
    return [p, if (p > start) 1 else 0];
}

/// Attempts to parse a single JSON object slice into a `ParsedToolCall`.
/// Returns `null` if the object does not look like a tool call (no `"name"` key at all).
fn parseToolCallObject(
    raw: []const u8,
    wrapper: ToolCallWrapperKind,
    force_truncated: bool,
): ParsedToolCall? {
    const trimmed = trimWs(raw);
    if (trimmed.len == 0 || trimmed[0] != '{') {
        return null;
    }

    const name_pos: usize = findTopLevelKeyValueStart(trimmed, "name") ?? return null;
    let call = ParsedToolCall.empty();
    call.wrapper = wrapper;

    let name_s: []u8 = call.name_buf[0..];
    if (nlen := decodeJsonStringAt(trimmed, name_pos, *name_s)) {
        if (nlen == 0) {
            call.status = ToolCallStatus.malformed;
            return call;
        }
        call.name_len = nlen;
    } else {
        call.status = if (force_truncated) ToolCallStatus.truncated else ToolCallStatus.malformed;
        return call;
    }

    // Optional `id` field (supports string id)
    if (id_pos := findTopLevelKeyValueStart(trimmed, "id")) {
        let id_s: []u8 = call.id_buf[0..];
        if (ilen := decodeJsonStringAt(trimmed, id_pos, *id_s)) {
            call.id_len = ilen;
        }
    }

    // Extract `arguments` (or fallback `parameters`) raw JSON slice
    let args_pos_opt: usize? = findTopLevelKeyValueStart(trimmed, "arguments");
    if (args_pos_opt == null) {
        args_pos_opt = findTopLevelKeyValueStart(trimmed, "parameters");
    }

    if (args_pos := args_pos_opt) {
        const span = measureJsonValueSpan(trimmed, args_pos);
        const raw_args = trimmed[args_pos..span[0]];
        const args_complete: bool = span[1] == 1;
        const copy_len: usize = if (raw_args.len <= TOOL_CALL_ARGS_MAX_LEN) raw_args.len else TOOL_CALL_ARGS_MAX_LEN;
        let i: usize = 0;
        while (i < copy_len) : (i += 1) {
            call.args_buf[i] = raw_args[i];
        }
        call.args_len = copy_len;

        if (force_truncated || !args_complete || raw_args.len > TOOL_CALL_ARGS_MAX_LEN) {
            call.status = ToolCallStatus.truncated;
        } else {
            call.status = ToolCallStatus.complete;
        }
        return call;
    }

    // No `arguments` / `parameters` field:
    // For bare JSON without `arguments`, require wrapper != bare_json unless truncated
    if (force_truncated) {
        call.status = ToolCallStatus.truncated;
        return call;
    }
    if (wrapper == ToolCallWrapperKind.bare_json) {
        return null;
    }
    // XML/fence wrapper with `{"name":"ping"}` defaults arguments to "{}"
    call.args_buf[0] = '{';
    call.args_buf[1] = '}';
    call.args_len = 2;
    call.status = ToolCallStatus.complete;
    return call;
}

// ============================================================================
// Core Incremental Streaming Tool-Call Parser Engine
// ============================================================================

/// Zero-heap-allocation incremental streaming LLM tool-call parser.
pub struct ToolCallStreamParser {
    mode: ToolParseMode = ToolParseMode.normal_text;

    pending_buf: [24]u8 = [0; 24];
    pending_len: usize = 0;

    work_buf: [TOOL_CALL_WORK_BUF_LEN]u8 = [0; TOOL_CALL_WORK_BUF_LEN];
    work_len: usize = 0;
    work_overflow: bool = false;

    brace_depth: usize = 0;
    in_string: bool = false;
    escape_next: bool = false;

    text_buf: [TOOL_CALL_TEXT_MAX_LEN]u8 = [0; TOOL_CALL_TEXT_MAX_LEN];
    text_len: usize = 0;
    text_dropped: usize = 0;

    calls: [TOOL_CALL_MAX_CALLS]ParsedToolCall = [ParsedToolCall.empty(); TOOL_CALL_MAX_CALLS];
    call_count: usize = 0;
    complete_calls: usize = 0;
    truncated_calls: usize = 0;
    malformed_calls: usize = 0;
    dropped_calls: usize = 0;
    bytes_fed: u64 = 0;

    /// Initializes a clean `ToolCallStreamParser`.
    static fn init(): ToolCallStreamParser {
        return ToolCallStreamParser{};
    }

    /// Resets all parser state and buffers for reuse on the next LLM turn.
    fn reset(*_) {
        _.mode = ToolParseMode.normal_text;
        _.pending_len = 0;
        _.work_len = 0;
        _.work_overflow = false;
        _.brace_depth = 0;
        _.in_string = false;
        _.escape_next = false;
        _.text_len = 0;
        _.text_dropped = 0;
        _.call_count = 0;
        _.complete_calls = 0;
        _.truncated_calls = 0;
        _.malformed_calls = 0;
        _.dropped_calls = 0;
        _.bytes_fed = 0;
    }

    fn emitTextByte(*_, b: u8) {
        if (_.text_len < TOOL_CALL_TEXT_MAX_LEN) {
            _.text_buf[_.text_len] = b;
            _.text_len += 1;
        } else {
            _.text_dropped += 1;
        }
    }

    fn emitTextSlice(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            _.emitTextByte(s[i]);
        }
    }

    fn pushWorkByte(*_, b: u8) {
        if (_.work_len < TOOL_CALL_WORK_BUF_LEN) {
            _.work_buf[_.work_len] = b;
            _.work_len += 1;
        } else {
            _.work_overflow = true;
        }
    }

    fn resetWork(*_) {
        _.work_len = 0;
        _.work_overflow = false;
        _.brace_depth = 0;
        _.in_string = false;
        _.escape_next = false;
    }

    fn recordCall(*_, call: ParsedToolCall) {
        if (call.status == ToolCallStatus.complete) _.complete_calls += 1;
        else if (call.status == ToolCallStatus.truncated) _.truncated_calls += 1;
        else _.malformed_calls += 1;

        if (_.call_count < TOOL_CALL_MAX_CALLS) {
            _.calls[_.call_count] = call;
            _.call_count += 1;
        } else {
            _.dropped_calls += 1;
        }
    }

    fn updateJsonLexState(*_, b: u8) {
        if (_.in_string) {
            if (_.escape_next) {
                _.escape_next = false;
            } else if (b == '\\') {
                _.escape_next = true;
            } else if (b == '"') {
                _.in_string = false;
            }
            return;
        }
        if (b == '"') {
            _.in_string = true;
        } else if (b == '{') {
            _.brace_depth += 1;
        } else if (b == '}') {
            if (_.brace_depth > 0) {
                _.brace_depth -= 1;
            }
        }
    }

    fn finalizeWrappedBody(*_, body: []const u8, wrapper: ToolCallWrapperKind, was_truncated: bool) {
        if (call := parseToolCallObject(body, wrapper, was_truncated || _.work_overflow)) {
            _.recordCall(call);
            return;
        }
        // In an explicit <tool_call> tag, non-matching content is malformed (or truncated)
        if (wrapper == ToolCallWrapperKind.xml_tag) {
            let bad = ParsedToolCall.empty();
            bad.wrapper = wrapper;
            bad.status = if (was_truncated || _.work_overflow) ToolCallStatus.truncated else ToolCallStatus.malformed;
            _.recordCall(bad);
            return;
        }
        // In a ```json fence: if it doesn't look like a tool call and wasn't truncated, keep as normal text
        if (was_truncated || _.work_overflow) {
            let bad = ParsedToolCall.empty();
            bad.wrapper = wrapper;
            bad.status = ToolCallStatus.truncated;
            _.recordCall(bad);
        } else {
            _.emitTextSlice(body);
        }
    }

    fn processByteNormal(*_, b: u8) {
        if (b == '<') {
            _.pending_buf[0] = '<';
            _.pending_len = 1;
            _.mode = ToolParseMode.match_open_tag;
            return;
        }
        if (b == '`') {
            _.pending_buf[0] = '`';
            _.pending_len = 1;
            _.mode = ToolParseMode.match_open_fence;
            return;
        }
        if (b == '{') {
            _.resetWork();
            _.pushWorkByte('{');
            _.brace_depth = 1;
            _.mode = ToolParseMode.in_bare_json;
            return;
        }
        _.emitTextByte(b);
    }

    fn stepByte(*_, b: u8) {
        if (_.mode == ToolParseMode.normal_text) {
            _.processByteNormal(b);
            return;
        }

        if (_.mode == ToolParseMode.match_open_tag) {
            const target: []const u8 = "<tool_call>";
            if (_.pending_len < target.len && b == target[_.pending_len]) {
                _.pending_buf[_.pending_len] = b;
                _.pending_len += 1;
                if (_.pending_len == target.len) {
                    _.pending_len = 0;
                    _.resetWork();
                    _.mode = ToolParseMode.in_xml_body;
                }
                return;
            }
            // Mismatch: flush buffered prefix as text and reprocess current byte in normal mode
            _.emitTextSlice(_.pending_buf[0.._.pending_len]);
            _.pending_len = 0;
            _.mode = ToolParseMode.normal_text;
            _.processByteNormal(b);
            return;
        }

        if (_.mode == ToolParseMode.match_open_fence) {
            if (_.pending_len < 3) {
                if (b == '`') {
                    _.pending_buf[_.pending_len] = b;
                    _.pending_len += 1;
                    return;
                }
                _.emitTextSlice(_.pending_buf[0.._.pending_len]);
                _.pending_len = 0;
                _.mode = ToolParseMode.normal_text;
                _.processByteNormal(b);
                return;
            }
            // We have "```" and are reading optional language tag until '\n'
            if (b == '\n') {
                const lang = trimWs(_.pending_buf[3.._.pending_len]);
                if (lang.len == 0 || eqIgnoreCase(lang, "json")) {
                    _.pending_len = 0;
                    _.resetWork();
                    _.mode = ToolParseMode.in_fence_body;
                    return;
                }
                // Non-json code fence: treat as normal text
                _.emitTextSlice(_.pending_buf[0.._.pending_len]);
                _.emitTextByte('\n');
                _.pending_len = 0;
                _.mode = ToolParseMode.normal_text;
                return;
            }
            if (_.pending_len < 20 && b != '`') {
                _.pending_buf[_.pending_len] = b;
                _.pending_len += 1;
                return;
            }
            _.emitTextSlice(_.pending_buf[0.._.pending_len]);
            _.pending_len = 0;
            _.mode = ToolParseMode.normal_text;
            _.processByteNormal(b);
            return;
        }

        if (_.mode == ToolParseMode.in_xml_body) {
            _.updateJsonLexState(b);
            _.pushWorkByte(b);
            const close_tag: []const u8 = "</tool_call>";
            if (!_.in_string && !_.work_overflow && endsWith(_.work_buf[0.._.work_len], close_tag)) {
                const body_len: usize = _.work_len - close_tag.len;
                _.finalizeWrappedBody(_.work_buf[0..body_len], ToolCallWrapperKind.xml_tag, false);
                _.resetWork();
                _.mode = ToolParseMode.normal_text;
            }
            return;
        }

        if (_.mode == ToolParseMode.in_fence_body) {
            _.updateJsonLexState(b);
            _.pushWorkByte(b);
            const close_fence: []const u8 = "```";
            if (!_.in_string && !_.work_overflow && endsWith(_.work_buf[0.._.work_len], close_fence)) {
                const body_len: usize = _.work_len - close_fence.len;
                _.finalizeWrappedBody(_.work_buf[0..body_len], ToolCallWrapperKind.code_fence, false);
                _.resetWork();
                _.mode = ToolParseMode.normal_text;
            }
            return;
        }

        // ToolParseMode.in_bare_json
        _.updateJsonLexState(b);
        _.pushWorkByte(b);
        if (!_.in_string && _.brace_depth == 0) {
            if (_.work_overflow) {
                if (call := parseToolCallObject(_.work_buf[0.._.work_len], ToolCallWrapperKind.bare_json, true)) {
                    _.recordCall(call);
                } else {
                    _.emitTextSlice(_.work_buf[0.._.work_len]);
                }
            } else if (call := parseToolCallObject(_.work_buf[0.._.work_len], ToolCallWrapperKind.bare_json, false)) {
                _.recordCall(call);
            } else {
                // Not a tool call JSON object -> preserve as normal text
                _.emitTextSlice(_.work_buf[0.._.work_len]);
            }
            _.resetWork();
            _.mode = ToolParseMode.normal_text;
        }
    }

    /// Incrementally feeds a chunk of LLM stream bytes (supports arbitrary chunk sizes including 1 byte).
    fn feed(*_, chunk: []const u8) {
        _.bytes_fed += chunk.len as u64;
        let i: usize = 0;
        while (i < chunk.len) : (i += 1) {
            _.stepByte(chunk[i]);
        }
    }

    /// Finalizes the stream, flushing pending text and recovering any truncated in-flight tool call.
    fn finish(*_): ToolCallParserStats {
        if (_.mode == ToolParseMode.match_open_tag || _.mode == ToolParseMode.match_open_fence) {
            _.emitTextSlice(_.pending_buf[0.._.pending_len]);
            _.pending_len = 0;
        } else if (_.mode == ToolParseMode.in_xml_body) {
            _.finalizeWrappedBody(_.work_buf[0.._.work_len], ToolCallWrapperKind.xml_tag, true);
            _.resetWork();
        } else if (_.mode == ToolParseMode.in_fence_body) {
            _.finalizeWrappedBody(_.work_buf[0.._.work_len], ToolCallWrapperKind.code_fence, true);
            _.resetWork();
        } else if (_.mode == ToolParseMode.in_bare_json) {
            if (call := parseToolCallObject(_.work_buf[0.._.work_len], ToolCallWrapperKind.bare_json, true)) {
                _.recordCall(call);
            } else {
                _.emitTextSlice(_.work_buf[0.._.work_len]);
            }
            _.resetWork();
        }
        _.mode = ToolParseMode.normal_text;
        return _.stats();
    }

    /// Returns the separated natural language text extracted outside tool call blocks.
    fn textSlice(): []const u8 {
        return _.text_buf[0.._.text_len];
    }

    /// Returns a read-only slice over all extracted tool calls.
    fn toolCalls(): []const ParsedToolCall {
        return _.calls[0.._.call_count];
    }

    /// Returns full parser telemetry statistics.
    fn stats(): ToolCallParserStats {
        return ToolCallParserStats{
            bytes_fed: _.bytes_fed,
            text_bytes_emitted: _.text_len,
            text_bytes_dropped: _.text_dropped,
            total_calls: _.call_count,
            complete_calls: _.complete_calls,
            truncated_calls: _.truncated_calls,
            malformed_calls: _.malformed_calls,
            dropped_calls: _.dropped_calls,
        };
    }
}
