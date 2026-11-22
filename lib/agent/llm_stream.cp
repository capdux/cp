// Origin: Pure-CP Stdlib (Unified LLM Streaming SSE Delta Parser for OpenAI & Anthropic) | Created: 2026-11-22
// Zero-heap-allocation unified LLM streaming delta parser supporting OpenAI Chat Completions SSE
// and Anthropic Messages SSE protocols over caller-provided scratch buffers.

const MAX_JSON_DEPTH: usize = 64;

/// Wire protocol family for unified LLM streaming delta parsing.
pub enum LlmProviderWire {
    OpenAi,
    Anthropic,
}

/// Semantic kind of a parsed LLM streaming delta event.
pub enum LlmDeltaKind {
    TextDelta,
    ReasoningDelta,
    ToolCallStart,
    ToolCallArgsDelta,
    Usage,
    Done,
    Ignore,
}

/// Unified zero-allocation LLM streaming delta event.
/// All string slices (`text`, `tool_call_id`, `tool_name`, `finish_reason`) are backed by
/// the caller-provided `out_buf` passed to the parse call.
pub struct LlmDeltaEvent {
    kind: LlmDeltaKind = LlmDeltaKind.Ignore;
    text: string = "";
    tool_call_id: string = "";
    tool_name: string = "";
    finish_reason: string = "";
    tool_index: u32 = 0;
    input_tokens: u64 = 0;
    output_tokens: u64 = 0;
    truncated: bool = false;
}

/// Cumulative statistics maintained by `LlmDeltaParser` across an SSE stream.
pub struct LlmStreamStats {
    events_parsed: u32 = 0;
    text_deltas: u32 = 0;
    reasoning_deltas: u32 = 0;
    tool_calls_started: u32 = 0;
    tool_args_deltas: u32 = 0;
    ignored_events: u32 = 0;
    parse_errors: u32 = 0;
    truncated_events: u32 = 0;
    text_bytes: u64 = 0;
    reasoning_bytes: u64 = 0;
    tool_args_bytes: u64 = 0;
    input_tokens: u64 = 0;
    output_tokens: u64 = 0;
    done: bool = false;
}

// ============================================================================
// Internal Zero-Allocation JSON Navigation & Decoding Primitives
// ============================================================================

fn satAddU32(a: u32, b: u32): u32 {
    if (a > 0xFFFFFFFF - b) return 0xFFFFFFFF;
    return a + b;
}

fn satAddU64(a: u64, b: u64): u64 {
    if (a > 0xFFFFFFFFFFFFFFFF - b) return 0xFFFFFFFFFFFFFFFF;
    return a + b;
}

fn isJsonWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn trimWs(s: string): string {
    let start: usize = 0;
    while (start < s.len && isJsonWs(s[start])) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && isJsonWs(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn stripSseDataPrefix(raw: string): string {
    const t = trimWs(raw);
    if (t.len >= 5 && t[0] == 'd' && t[1] == 'a' && t[2] == 't' && t[3] == 'a' && t[4] == ':') {
        return trimWs(t[5..]);
    }
    return t;
}

fn hexVal(b: u8): u16? {
    if (b >= '0' && b <= '9') return (b - '0') as u16;
    if (b >= 'A' && b <= 'F') return (b - 'A' + 10) as u16;
    if (b >= 'a' && b <= 'f') return (b - 'a' + 10) as u16;
    return null;
}

/// Validates that `s` is a structurally balanced JSON object `{...}` with properly
/// terminated strings/escapes and nesting depth <= `MAX_JSON_DEPTH`.
fn validateJsonObjectBalanced(s: string): bool {
    if (s.len < 2 || s[0] != '{' || s[s.len - 1] != '}') return false;

    let stack: [64]u8 = [0; 64];
    let depth: usize = 0;
    let in_str = false;
    let esc = false;

    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (depth == 0 && i > 0) {
            // Trailing non-whitespace after root object closed
            return false;
        }
        if (in_str) {
            if (esc) {
                esc = false;
            } else if (b == '\\') {
                esc = true;
            } else if (b == '"') {
                in_str = false;
            }
            continue;
        }
        if (b == '"') {
            in_str = true;
        } else if (b == '{') {
            if (depth >= MAX_JSON_DEPTH) return false;
            stack[depth] = '}';
            depth += 1;
        } else if (b == '[') {
            if (depth >= MAX_JSON_DEPTH) return false;
            stack[depth] = ']';
            depth += 1;
        } else if (b == '}' || b == ']') {
            if (depth == 0 || stack[depth - 1] != b) return false;
            depth -= 1;
        }
    }
    return !in_str && !esc && depth == 0;
}

fn skipJsonString(s: string, quote_pos: usize): usize? {
    if (quote_pos >= s.len || s[quote_pos] != '"') return null;
    let i = quote_pos + 1;
    let esc = false;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (esc) {
            esc = false;
        } else if (b == '\\') {
            esc = true;
        } else if (b == '"') {
            return i + 1;
        }
    }
    return null;
}

fn skipJsonValueEnd(s: string, start: usize): usize? {
    let i = start;
    while (i < s.len && isJsonWs(s[i])) : (i += 1) {}
    if (i >= s.len) return null;

    const first = s[i];
    if (first == '"') {
        return skipJsonString(s, i);
    }
    if (first == '{' || first == '[') {
        let depth: usize = 0;
        let in_str = false;
        let esc = false;
        while (i < s.len) : (i += 1) {
            const b = s[i];
            if (in_str) {
                if (esc) {
                    esc = false;
                } else if (b == '\\') {
                    esc = true;
                } else if (b == '"') {
                    in_str = false;
                }
            } else {
                if (b == '"') {
                    in_str = true;
                } else if (b == '{' || b == '[') {
                    depth += 1;
                } else if (b == '}' || b == ']') {
                    if (depth == 0) return null;
                    depth -= 1;
                    if (depth == 0) return i + 1;
                }
            }
        }
        return null;
    }

    // Primitive value: number, true, false, null
    const prim_start = i;
    while (i < s.len && !isJsonWs(s[i]) && s[i] != ',' && s[i] != '}' && s[i] != ']') : (i += 1) {}
    if (i == prim_start) return null;
    return i;
}

/// Finds a top-level field `key` in a JSON object `obj` and returns the raw value slice.
fn findObjectFieldRaw(obj: string, key: string): string? {
    const t = trimWs(obj);
    if (t.len < 2 || t[0] != '{' || t[t.len - 1] != '}') return null;

    let i: usize = 1;
    const end = t.len - 1;
    while (i < end) {
        while (i < end && (isJsonWs(t[i]) || t[i] == ',')) : (i += 1) {}
        if (i >= end) return null;
        if (t[i] != '"') return null;

        const key_end_after = skipJsonString(t, i) ?? return null;
        const raw_key = t[i + 1 .. key_end_after - 1];
        i = key_end_after;

        while (i < end && isJsonWs(t[i])) : (i += 1) {}
        if (i >= end || t[i] != ':') return null;
        i += 1;
        while (i < end && isJsonWs(t[i])) : (i += 1) {}
        if (i >= end) return null;

        const val_start = i;
        const val_end = skipJsonValueEnd(t, val_start) ?? return null;
        if (strEq(raw_key, key)) {
            return trimWs(t[val_start..val_end]);
        }
        i = val_end;
    }
    return null;
}

/// Extracts the first element raw slice from a JSON array `[elem0, ...]`.
fn firstArrayElementRaw(arr: string): string? {
    const t = trimWs(arr);
    if (t.len < 2 || t[0] != '[' || t[t.len - 1] != ']') return null;
    let i: usize = 1;
    const end = t.len - 1;
    while (i < end && isJsonWs(t[i])) : (i += 1) {}
    if (i >= end) return null;
    const val_end = skipJsonValueEnd(t, i) ?? return null;
    return trimWs(t[i..val_end]);
}

/// Compares a raw JSON string literal `"..."` (without escapes) against `expected`.
fn jsonRawStringEquals(raw_val: string, expected: string): bool {
    if (raw_val.len < 2 || raw_val[0] != '"' || raw_val[raw_val.len - 1] != '"') return false;
    return strEq(raw_val[1 .. raw_val.len - 1], expected);
}

fn parseJsonU64(raw_val: string): u64? {
    const t = trimWs(raw_val);
    if (t.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < t.len) : (i += 1) {
        const b = t[i];
        if (b < '0' || b > '9') return null;
        const digit = (b - '0') as u64;
        if (acc > (0xFFFFFFFFFFFFFFFF - digit) / 10) {
            acc = 0xFFFFFFFFFFFFFFFF;
        } else {
            acc = acc * 10 + digit;
        }
    }
    return acc;
}

fn writeBufByte(out_buf: []u8, cursor: *usize, truncated: *bool, b: u8) {
    if (cursor.* >= out_buf.len) {
        truncated.* = true;
        return;
    }
    out_buf[cursor.*] = b;
    cursor.* += 1;
}

/// Decodes a raw JSON string literal `"..."` into `out_buf` starting at `cursor.*`.
/// Handles `\"`, `\\`, `\/`, `\n`, `\r`, `\t`, `\b`, `\f`, and `\uXXXX` (UTF-8).
/// Safely truncates without out-of-bounds writes when `out_buf` is full, setting `truncated.* = true`.
/// Returns `null` if `raw_val` is not a valid JSON string literal or contains malformed escapes.
fn decodeJsonStringInto(raw_val: string, out_buf: []u8, cursor: *usize, truncated: *bool): string? {
    if (raw_val.len < 2 || raw_val[0] != '"' || raw_val[raw_val.len - 1] != '"') return null;

    const start = cursor.*;
    let i: usize = 1;
    const end = raw_val.len - 1;

    while (i < end) {
        const c = raw_val[i];
        if (c == '\\') {
            i += 1;
            if (i >= end) return null;
            const esc = raw_val[i];
            if (esc == 'u') {
                if (i + 4 >= end) return null;
                const d0 = hexVal(raw_val[i + 1]) ?? return null;
                const d1 = hexVal(raw_val[i + 2]) ?? return null;
                const d2 = hexVal(raw_val[i + 3]) ?? return null;
                const d3 = hexVal(raw_val[i + 4]) ?? return null;
                const cp: u16 = (d0 << 12) | (d1 << 8) | (d2 << 4) | d3;
                i += 5;
                if (cp < 0x80) {
                    writeBufByte(out_buf, cursor, truncated, cp as u8);
                } else if (cp < 0x800) {
                    if (cursor.* + 2 > out_buf.len) {
                        truncated.* = true;
                    } else {
                        out_buf[cursor.*] = (0xC0 | (cp >> 6)) as u8;
                        out_buf[cursor.* + 1] = (0x80 | (cp & 0x3F)) as u8;
                        cursor.* += 2;
                    }
                } else {
                    if (cursor.* + 3 > out_buf.len) {
                        truncated.* = true;
                    } else {
                        out_buf[cursor.*] = (0xE0 | (cp >> 12)) as u8;
                        out_buf[cursor.* + 1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
                        out_buf[cursor.* + 2] = (0x80 | (cp & 0x3F)) as u8;
                        cursor.* += 3;
                    }
                }
                continue;
            }
            const decoded: u8 = switch (esc) {
                '"' => '"',
                '\\' => '\\',
                '/' => '/',
                'n' => '\n',
                'r' => '\r',
                't' => '\t',
                'b' => 0x08,
                'f' => 0x0C,
                else => return null,
            };
            writeBufByte(out_buf, cursor, truncated, decoded);
            i += 1;
        } else {
            writeBufByte(out_buf, cursor, truncated, c);
            i += 1;
        }
    }

    return out_buf[start..cursor.*] as string;
}

// ============================================================================
// OpenAI Chat Completions SSE Data Parser
// ============================================================================

/// Parses a single OpenAI Chat Completions SSE `data:` payload into a unified `LlmDeltaEvent`.
/// Accepts either raw JSON / `[DONE]` or a line prefixed with `data:`.
/// Returns `null` on malformed JSON or invalid escape sequences.
pub fn parseOpenAiSseData(data: string, out_buf: []u8): LlmDeltaEvent? {
    const payload = stripSseDataPrefix(data);
    if (payload.len == 0) {
        return LlmDeltaEvent{ kind: LlmDeltaKind.Ignore };
    }
    if (strEq(payload, "[DONE]")) {
        return LlmDeltaEvent{ kind: LlmDeltaKind.Done };
    }
    if (!validateJsonObjectBalanced(payload)) {
        return null;
    }

    let ev = LlmDeltaEvent{};
    let cursor: usize = 0;
    let truncated = false;
    let has_usage = false;

    // 1. Parse optional top-level usage object
    if (usage_raw := findObjectFieldRaw(payload, "usage")) {
        if (!strEq(usage_raw, "null") && usage_raw.len >= 2 && usage_raw[0] == '{') {
            if (pt_raw := findObjectFieldRaw(usage_raw, "prompt_tokens")) {
                if (v := parseJsonU64(pt_raw)) {
                    ev.input_tokens = v;
                    has_usage = true;
                }
            } else if (it_raw := findObjectFieldRaw(usage_raw, "input_tokens")) {
                if (v := parseJsonU64(it_raw)) {
                    ev.input_tokens = v;
                    has_usage = true;
                }
            }
            if (ct_raw := findObjectFieldRaw(usage_raw, "completion_tokens")) {
                if (v := parseJsonU64(ct_raw)) {
                    ev.output_tokens = v;
                    has_usage = true;
                }
            } else if (ot_raw := findObjectFieldRaw(usage_raw, "output_tokens")) {
                if (v := parseJsonU64(ot_raw)) {
                    ev.output_tokens = v;
                    has_usage = true;
                }
            }
        }
    }

    // 2. Parse choices[0] if present
    if (choices_raw := findObjectFieldRaw(payload, "choices")) {
        if (choice0 := firstArrayElementRaw(choices_raw)) {
            if (fr_raw := findObjectFieldRaw(choice0, "finish_reason")) {
                if (!strEq(fr_raw, "null")) {
                    const fr = decodeJsonStringInto(fr_raw, out_buf, *cursor, *truncated) ?? return null;
                    ev.finish_reason = fr;
                }
            }

            if (delta_raw := findObjectFieldRaw(choice0, "delta")) {
                if (!strEq(delta_raw, "null") && delta_raw.len >= 2 && delta_raw[0] == '{') {
                    // 2a. Tool call deltas
                    if (tcs_raw := findObjectFieldRaw(delta_raw, "tool_calls")) {
                        if (tc0 := firstArrayElementRaw(tcs_raw)) {
                            if (idx_raw := findObjectFieldRaw(tc0, "index")) {
                                if (idx_v := parseJsonU64(idx_raw)) {
                                    ev.tool_index = if (idx_v > 0xFFFFFFFF) 0xFFFFFFFF else (idx_v as u32);
                                }
                            }
                            if (id_raw := findObjectFieldRaw(tc0, "id")) {
                                if (!strEq(id_raw, "null")) {
                                    ev.tool_call_id = decodeJsonStringInto(id_raw, out_buf, *cursor, *truncated) ?? return null;
                                }
                            }
                            let has_args = false;
                            if (fn_raw := findObjectFieldRaw(tc0, "function")) {
                                if (!strEq(fn_raw, "null") && fn_raw.len >= 2 && fn_raw[0] == '{') {
                                    if (name_raw := findObjectFieldRaw(fn_raw, "name")) {
                                        if (!strEq(name_raw, "null")) {
                                            ev.tool_name = decodeJsonStringInto(name_raw, out_buf, *cursor, *truncated) ?? return null;
                                        }
                                    }
                                    if (args_raw := findObjectFieldRaw(fn_raw, "arguments")) {
                                        if (!strEq(args_raw, "null")) {
                                            ev.text = decodeJsonStringInto(args_raw, out_buf, *cursor, *truncated) ?? return null;
                                            has_args = true;
                                        }
                                    }
                                }
                            }
                            if (ev.tool_call_id.len > 0 || ev.tool_name.len > 0) {
                                ev.kind = LlmDeltaKind.ToolCallStart;
                                ev.truncated = truncated;
                                return ev;
                            }
                            if (has_args) {
                                ev.kind = LlmDeltaKind.ToolCallArgsDelta;
                                ev.truncated = truncated;
                                return ev;
                            }
                        }
                    }

                    // 2b. Reasoning content delta (o-series / DeepSeek / Qwen)
                    if (rc_raw := findObjectFieldRaw(delta_raw, "reasoning_content")) {
                        if (!strEq(rc_raw, "null")) {
                            const rc = decodeJsonStringInto(rc_raw, out_buf, *cursor, *truncated) ?? return null;
                            if (rc.len > 0 || truncated) {
                                ev.kind = LlmDeltaKind.ReasoningDelta;
                                ev.text = rc;
                                ev.truncated = truncated;
                                return ev;
                            }
                        }
                    }

                    // 2c. Standard text content delta
                    if (content_raw := findObjectFieldRaw(delta_raw, "content")) {
                        if (!strEq(content_raw, "null")) {
                            const txt = decodeJsonStringInto(content_raw, out_buf, *cursor, *truncated) ?? return null;
                            if (txt.len > 0 || truncated) {
                                ev.kind = LlmDeltaKind.TextDelta;
                                ev.text = txt;
                                ev.truncated = truncated;
                                return ev;
                            }
                        }
                    }
                }
            }

            if (has_usage) {
                ev.kind = LlmDeltaKind.Usage;
                ev.truncated = truncated;
                return ev;
            }
            if (ev.finish_reason.len > 0) {
                ev.kind = LlmDeltaKind.Done;
                ev.truncated = truncated;
                return ev;
            }
            ev.kind = LlmDeltaKind.Ignore;
            ev.truncated = truncated;
            return ev;
        }
    }

    if (has_usage) {
        ev.kind = LlmDeltaKind.Usage;
        ev.truncated = truncated;
        return ev;
    }

    ev.kind = LlmDeltaKind.Ignore;
    ev.truncated = truncated;
    return ev;
}

// ============================================================================
// Anthropic Messages SSE Data Parser
// ============================================================================

/// Parses a single Anthropic Messages SSE event payload into a unified `LlmDeltaEvent`.
/// `event_type` may be empty `""`, in which case the JSON top-level `"type"` field is used.
/// Returns `null` on malformed JSON or invalid escape sequences.
pub fn parseAnthropicSseData(event_type: string, data: string, out_buf: []u8): LlmDeltaEvent? {
    const payload = stripSseDataPrefix(data);
    if (payload.len == 0) {
        return LlmDeltaEvent{ kind: LlmDeltaKind.Ignore };
    }
    if (!validateJsonObjectBalanced(payload)) {
        return null;
    }

    let ev = LlmDeltaEvent{};
    let cursor: usize = 0;
    let truncated = false;

    // Determine event type from JSON `"type"` field or fallback to `event_type` parameter
    let ev_name: string = trimWs(event_type);
    if (type_raw := findObjectFieldRaw(payload, "type")) {
        if (type_raw.len >= 2 && type_raw[0] == '"' && type_raw[type_raw.len - 1] == '"') {
            ev_name = type_raw[1 .. type_raw.len - 1] as string;
        }
    }

    if (strEq(ev_name, "content_block_start")) {
        if (idx_raw := findObjectFieldRaw(payload, "index")) {
            if (idx_v := parseJsonU64(idx_raw)) {
                ev.tool_index = if (idx_v > 0xFFFFFFFF) 0xFFFFFFFF else (idx_v as u32);
            }
        }
        if (cb_raw := findObjectFieldRaw(payload, "content_block")) {
            if (cb_type := findObjectFieldRaw(cb_raw, "type")) {
                if (jsonRawStringEquals(cb_type, "tool_use")) {
                    if (id_raw := findObjectFieldRaw(cb_raw, "id")) {
                        if (!strEq(id_raw, "null")) {
                            ev.tool_call_id = decodeJsonStringInto(id_raw, out_buf, *cursor, *truncated) ?? return null;
                        }
                    }
                    if (name_raw := findObjectFieldRaw(cb_raw, "name")) {
                        if (!strEq(name_raw, "null")) {
                            ev.tool_name = decodeJsonStringInto(name_raw, out_buf, *cursor, *truncated) ?? return null;
                        }
                    }
                    ev.kind = LlmDeltaKind.ToolCallStart;
                    ev.truncated = truncated;
                    return ev;
                }
                if (jsonRawStringEquals(cb_type, "text")) {
                    if (txt_raw := findObjectFieldRaw(cb_raw, "text")) {
                        if (!strEq(txt_raw, "null")) {
                            const txt = decodeJsonStringInto(txt_raw, out_buf, *cursor, *truncated) ?? return null;
                            if (txt.len > 0 || truncated) {
                                ev.kind = LlmDeltaKind.TextDelta;
                                ev.text = txt;
                                ev.truncated = truncated;
                                return ev;
                            }
                        }
                    }
                }
            }
        }
        ev.kind = LlmDeltaKind.Ignore;
        ev.truncated = truncated;
        return ev;
    }

    if (strEq(ev_name, "content_block_delta")) {
        if (idx_raw := findObjectFieldRaw(payload, "index")) {
            if (idx_v := parseJsonU64(idx_raw)) {
                ev.tool_index = if (idx_v > 0xFFFFFFFF) 0xFFFFFFFF else (idx_v as u32);
            }
        }
        if (delta_raw := findObjectFieldRaw(payload, "delta")) {
            if (dt_raw := findObjectFieldRaw(delta_raw, "type")) {
                if (jsonRawStringEquals(dt_raw, "text_delta")) {
                    if (txt_raw := findObjectFieldRaw(delta_raw, "text")) {
                        ev.text = decodeJsonStringInto(txt_raw, out_buf, *cursor, *truncated) ?? return null;
                        ev.kind = LlmDeltaKind.TextDelta;
                        ev.truncated = truncated;
                        return ev;
                    }
                } else if (jsonRawStringEquals(dt_raw, "thinking_delta")) {
                    if (th_raw := findObjectFieldRaw(delta_raw, "thinking")) {
                        ev.text = decodeJsonStringInto(th_raw, out_buf, *cursor, *truncated) ?? return null;
                        ev.kind = LlmDeltaKind.ReasoningDelta;
                        ev.truncated = truncated;
                        return ev;
                    }
                } else if (jsonRawStringEquals(dt_raw, "input_json_delta")) {
                    if (pj_raw := findObjectFieldRaw(delta_raw, "partial_json")) {
                        ev.text = decodeJsonStringInto(pj_raw, out_buf, *cursor, *truncated) ?? return null;
                        ev.kind = LlmDeltaKind.ToolCallArgsDelta;
                        ev.truncated = truncated;
                        return ev;
                    }
                }
            }
        }
        ev.kind = LlmDeltaKind.Ignore;
        ev.truncated = truncated;
        return ev;
    }

    if (strEq(ev_name, "message_delta")) {
        if (delta_raw := findObjectFieldRaw(payload, "delta")) {
            if (sr_raw := findObjectFieldRaw(delta_raw, "stop_reason")) {
                if (!strEq(sr_raw, "null")) {
                    ev.finish_reason = decodeJsonStringInto(sr_raw, out_buf, *cursor, *truncated) ?? return null;
                }
            }
        }
        if (usage_raw := findObjectFieldRaw(payload, "usage")) {
            if (!strEq(usage_raw, "null") && usage_raw.len >= 2 && usage_raw[0] == '{') {
                if (it_raw := findObjectFieldRaw(usage_raw, "input_tokens")) {
                    if (v := parseJsonU64(it_raw)) {
                        ev.input_tokens = v;
                    }
                }
                if (ot_raw := findObjectFieldRaw(usage_raw, "output_tokens")) {
                    if (v := parseJsonU64(ot_raw)) {
                        ev.output_tokens = v;
                    }
                }
            }
        }
        ev.kind = LlmDeltaKind.Usage;
        ev.truncated = truncated;
        return ev;
    }

    if (strEq(ev_name, "message_start")) {
        if (msg_raw := findObjectFieldRaw(payload, "message")) {
            if (usage_raw := findObjectFieldRaw(msg_raw, "usage")) {
                let found_u = false;
                if (it_raw := findObjectFieldRaw(usage_raw, "input_tokens")) {
                    if (v := parseJsonU64(it_raw)) {
                        ev.input_tokens = v;
                        found_u = true;
                    }
                }
                if (ot_raw := findObjectFieldRaw(usage_raw, "output_tokens")) {
                    if (v := parseJsonU64(ot_raw)) {
                        ev.output_tokens = v;
                        found_u = true;
                    }
                }
                if (found_u) {
                    ev.kind = LlmDeltaKind.Usage;
                    ev.truncated = truncated;
                    return ev;
                }
            }
        }
        ev.kind = LlmDeltaKind.Ignore;
        ev.truncated = truncated;
        return ev;
    }

    if (strEq(ev_name, "message_stop")) {
        ev.kind = LlmDeltaKind.Done;
        ev.truncated = truncated;
        return ev;
    }

    ev.kind = LlmDeltaKind.Ignore;
    ev.truncated = truncated;
    return ev;
}

// ============================================================================
// Stateful Unified LLM Delta Parser & Stream Statistics Accumulator
// ============================================================================

/// Stateful zero-allocation unified LLM delta parser that dispatches SSE events
/// according to `LlmProviderWire` and accumulates `LlmStreamStats`.
pub struct LlmDeltaParser {
    provider: LlmProviderWire = LlmProviderWire.OpenAi;
    cur_stats: LlmStreamStats = LlmStreamStats{};

    /// Creates a new `LlmDeltaParser` bound to `provider`.
    static fn init(provider: LlmProviderWire): LlmDeltaParser {
        return LlmDeltaParser{
            provider: provider,
            cur_stats: LlmStreamStats{},
        };
    }

    /// Resets accumulated statistics for reusing the parser on a new stream.
    fn reset(*_) {
        _.cur_stats = LlmStreamStats{};
    }

    /// Returns a snapshot of accumulated stream statistics.
    fn stats(): LlmStreamStats {
        return _.cur_stats;
    }

    /// Returns `true` once a terminal `Done` event has been observed.
    fn isDone(): bool {
        return _.cur_stats.done;
    }

    /// Parses one SSE event (`event_type`, `data`) into `out_buf` and updates statistics.
    /// Returns `null` on malformed JSON while incrementing `stats().parse_errors`.
    fn parseEvent(*_, event_type: string, data: string, out_buf: []u8): LlmDeltaEvent? {
        const parsed_opt: LlmDeltaEvent? = switch (_.provider) {
            LlmProviderWire.OpenAi => parseOpenAiSseData(data, out_buf),
            LlmProviderWire.Anthropic => parseAnthropicSseData(event_type, data, out_buf),
        };

        if (ev := parsed_opt) {
            _.cur_stats.events_parsed = satAddU32(_.cur_stats.events_parsed, 1);
            if (ev.truncated) {
                _.cur_stats.truncated_events = satAddU32(_.cur_stats.truncated_events, 1);
            }
            if (ev.input_tokens > 0) {
                _.cur_stats.input_tokens = ev.input_tokens;
            }
            if (ev.output_tokens > 0) {
                _.cur_stats.output_tokens = ev.output_tokens;
            }

            switch (ev.kind) {
                LlmDeltaKind.TextDelta => {
                    _.cur_stats.text_deltas = satAddU32(_.cur_stats.text_deltas, 1);
                    _.cur_stats.text_bytes = satAddU64(_.cur_stats.text_bytes, ev.text.len as u64);
                }
                LlmDeltaKind.ReasoningDelta => {
                    _.cur_stats.reasoning_deltas = satAddU32(_.cur_stats.reasoning_deltas, 1);
                    _.cur_stats.reasoning_bytes = satAddU64(_.cur_stats.reasoning_bytes, ev.text.len as u64);
                }
                LlmDeltaKind.ToolCallStart => {
                    _.cur_stats.tool_calls_started = satAddU32(_.cur_stats.tool_calls_started, 1);
                    _.cur_stats.tool_args_bytes = satAddU64(_.cur_stats.tool_args_bytes, ev.text.len as u64);
                }
                LlmDeltaKind.ToolCallArgsDelta => {
                    _.cur_stats.tool_args_deltas = satAddU32(_.cur_stats.tool_args_deltas, 1);
                    _.cur_stats.tool_args_bytes = satAddU64(_.cur_stats.tool_args_bytes, ev.text.len as u64);
                }
                LlmDeltaKind.Done => {
                    _.cur_stats.done = true;
                }
                LlmDeltaKind.Ignore => {
                    _.cur_stats.ignored_events = satAddU32(_.cur_stats.ignored_events, 1);
                }
                LlmDeltaKind.Usage => {}
            }
            return ev;
        }

        _.cur_stats.parse_errors = satAddU32(_.cur_stats.parse_errors, 1);
        return null;
    }

    /// Convenience helper equivalent to `parseEvent("", data, out_buf)`.
    fn parseData(*_, data: string, out_buf: []u8): LlmDeltaEvent? {
        return _.parseEvent("", data, out_buf);
    }
}
