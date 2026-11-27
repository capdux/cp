// Origin: Pure-CP Stdlib (LLM Structured Output Extraction, Fault-Tolerant JSON Repair & Auto Self-Correction Prompting) | Created: 2026-11-22
// Pure CP zero-heap-allocation structured output pipeline for LLM responses:
//   1. `extractJsonCandidate`: zero-copy extraction stripping `<think>...</think>` blocks, Markdown fences & prose
//   2. `repairLlmJson`: deterministic repair of trailing commas, truncated brackets/strings, single quotes & raw newlines
//   3. `checkRequiredJsonFields` & `formatRepairRetryPrompt`: top-level required field/type validation & LLM self-correction prompt generation

const std = @import("std");

// ============================================================================
// Internal ASCII, Buffer & Scanning Helpers
// ============================================================================

type SoVec16 = @Vector(16, u8);

fn makeSoWsLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[' '] = 1;
    lut['\t'] = 1;
    lut['\n'] = 1;
    lut['\r'] = 1;
    return lut;
}

fn makeSoLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

const SO_WS_LUT: [u8; 256] = makeSoWsLut();
const SO_LOWER_LUT: [u8; 256] = makeSoLowerLut();

fn isWs(c: u8): bool {
    return SO_WS_LUT[@as(usize, c)] != 0;
}

fn asciiLower(c: u8): u8 {
    return SO_LOWER_LUT[@as(usize, c)];
}

fn matchCaseInsensitiveAt(text: string, pos: usize, target: string): bool {
    if (pos + target.len > text.len) return false;
    let i: usize = 0;
    const lo_A: SoVec16 = @splat(65 as u8);
    const hi_Z: SoVec16 = @splat(90 as u8);
    const case_bit: SoVec16 = @splat(0x20 as u8);
    while (i + 16 <= target.len) : (i += 16) {
        const va: SoVec16 = text[pos + i .. pos + i + 16][0..16].*;
        const vb: SoVec16 = target[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        if (@reduce(.Or, la ^ vb) != 0) return false;
    }
    while (i < target.len) : (i += 1) {
        if (asciiLower(text[pos + i]) != target[i]) return false;
    }
    return true;
}

fn findCaseInsensitive(text: string, start_pos: usize, target: string): usize? {
    if (target.len == 0 || start_pos + target.len > text.len) return null;
    const first_lo: u8 = target[0];
    const first_hi: u8 = if (first_lo >= 'a' && first_lo <= 'z') first_lo - 32 else first_lo;
    const v_lo: SoVec16 = @splat(first_lo);
    const v_hi: SoVec16 = @splat(first_hi);
    let i: usize = start_pos;
    const limit: usize = text.len - target.len;
    while (i <= limit) {
        if (i + 16 <= limit + 1) {
            const chunk: SoVec16 = text[i .. i + 16][0..16].*;
            const hit = (chunk == v_lo) | (chunk == v_hi);
            if (!@reduce(.Or, hit)) {
                i += 16;
                continue;
            }
        }
        if (matchCaseInsensitiveAt(text, i, target)) return i;
        i += 1;
    }
    return null;
}

fn findTripleBacktick(text: string, start_pos: usize, end: usize): usize? {
    if (start_pos + 3 > end) return null;
    let i: usize = start_pos;
    const limit: usize = end - 3;
    const v_bt: SoVec16 = @splat('`' as u8);
    while (i <= limit) {
        if (i + 16 <= limit + 1) {
            const chunk: SoVec16 = text[i .. i + 16][0..16].*;
            if (!@reduce(.Or, chunk == v_bt)) {
                i += 16;
                continue;
            }
        }
        if (text[i] == '`' && text[i + 1] == '`' && text[i + 2] == '`') {
            return i;
        }
        i += 1;
    }
    return null;
}

fn writeByte(out_buf: *[]u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeRaw(out_buf: *[]u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    if (s.len > 0) {
        @memcpy(out_buf[pos.* .. pos.* + s.len], s);
    }
    pos.* += s.len;
    return true;
}

// ============================================================================
// 1. Zero-Copy JSON Candidate Extraction (`extractJsonCandidate`)
// ============================================================================

/// Zero-copy byte span `[start, end)` locating a JSON candidate inside raw LLM output.
pub struct JsonCandidateSpan {
    start: usize = 0;
    end: usize = 0;
    from_code_fence: bool = false;
    skipped_thought: bool = false;
    is_truncated: bool = false;
}

fn scanOuterJsonSpan(
    raw: string,
    region_start: usize,
    region_end: usize,
    from_code_fence: bool,
    skipped_thought: bool,
): JsonCandidateSpan? {
    // Locate first '{' or '[' in [region_start, region_end).
    let start: usize = region_start;
    let found_open = false;
    const v_lb: SoVec16 = @splat('{' as u8);
    const v_ls: SoVec16 = @splat('[' as u8);
    while (start + 16 <= region_end) {
        const v: SoVec16 = raw[start .. start + 16][0..16].*;
        const hit = (v == v_lb) | (v == v_ls);
        if (@reduce(.Or, hit)) break;
        start += 16;
    }
    while (start < region_end) : (start += 1) {
        const c: u8 = raw[start];
        if (c == '{' || c == '[') {
            found_open = true;
            break;
        }
    }
    if (!found_open) return null;

    let depth: usize = 0;
    let in_string = false;
    let quote_char: u8 = 0;
    let escaped = false;

    let i: usize = start;
    while (i < region_end) : (i += 1) {
        const c: u8 = raw[i];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == quote_char) {
                in_string = false;
                quote_char = 0;
            }
            continue;
        }

        if (c == '"' || c == '\'') {
            in_string = true;
            quote_char = c;
        } else if (c == '{' || c == '[') {
            depth += 1;
        } else if (c == '}' || c == ']') {
            if (depth > 0) {
                depth -= 1;
                if (depth == 0) {
                    return JsonCandidateSpan{
                        start: start,
                        end: i + 1,
                        from_code_fence: from_code_fence,
                        skipped_thought: skipped_thought,
                        is_truncated: false,
                    };
                }
            }
        }
    }

    // Reached end of region while depth > 0 -> truncated JSON candidate.
    let trimmed_end: usize = region_end;
    while (trimmed_end > start && isWs(raw[trimmed_end - 1])) : (trimmed_end -= 1) {}
    return JsonCandidateSpan{
        start: start,
        end: trimmed_end,
        from_code_fence: from_code_fence,
        skipped_thought: skipped_thought,
        is_truncated: true,
    };
}

/// Zero-copy extraction of the outermost `{...}` or `[...]` JSON candidate from raw LLM output.
///
/// Automatically strips:
/// 1. One or more `<think>...</think>` reasoning blocks (case-insensitive, including orphan `</think>`).
/// 2. Markdown code fences (e.g. ```` ```json ... ``` ```` or ```` ``` ... ``` ````) if they contain JSON.
/// 3. Leading/trailing natural-language prose around the JSON payload.
///
/// Returns `null` if no JSON object or array start (`{` / `[`) is present.
pub fn extractJsonCandidate(raw_output: string): JsonCandidateSpan? {
    if (raw_output.len == 0) return null;

    let scan_start: usize = 0;
    let skipped_thought = false;

    // 1. Skip leading/embedded <think>...</think> blocks.
    while (scan_start < raw_output.len) {
        if (close_pos := findCaseInsensitive(raw_output, scan_start, "</think>")) {
            if (open_pos := findCaseInsensitive(raw_output, scan_start, "<think>")) {
                if (open_pos < close_pos) {
                    scan_start = close_pos + 8;
                    skipped_thought = true;
                    continue;
                }
            }
            // Orphan </think> (e.g., prompt prefilled "<think>")
            scan_start = close_pos + 8;
            skipped_thought = true;
            continue;
        }
        break;
    }

    // 2. Check for Markdown code fence containing JSON in [scan_start, raw_output.len).
    let fence_search: usize = scan_start;
    while (fence_open := findTripleBacktick(raw_output, fence_search, raw_output.len)) {
        // Advance past opening ``` and optional language tag line (e.g. "json\n").
        let body_start: usize = fence_open + 3;
        while (body_start < raw_output.len && raw_output[body_start] != '\n' && raw_output[body_start] != '{' && raw_output[body_start] != '[') : (body_start += 1) {}
        if (body_start < raw_output.len && raw_output[body_start] == '\n') {
            body_start += 1;
        }

        let body_end: usize = raw_output.len;
        let next_search: usize = raw_output.len;
        if (fence_close := findTripleBacktick(raw_output, body_start, raw_output.len)) {
            body_end = fence_close;
            next_search = fence_close + 3;
        }

        if (span := scanOuterJsonSpan(raw_output, body_start, body_end, true, skipped_thought)) {
            return span;
        }
        fence_search = next_search;
    }

    // 3. Fallback: scan post-thought raw text directly for outermost {...} / [...].
    return scanOuterJsonSpan(raw_output, scan_start, raw_output.len, false, skipped_thought);
}

// ============================================================================
// 2. Deterministic Fault-Tolerant LLM JSON Repair (`repairLlmJson`)
// ============================================================================

/// Detailed diagnostics returned by `repairLlmJson`.
pub struct JsonRepairReport {
    bytes_written: usize = 0;
    removed_trailing_commas: u32 = 0;
    closed_brackets: u32 = 0;
    closed_string: bool = false;
    normalized_single_quotes: u32 = 0;
    escaped_newlines: u32 = 0;
    was_modified: bool = false;
}

/// Deterministically repairs common LLM JSON malformations into caller-provided `out_buf` (zero heap allocation):
/// - Removes trailing commas before `}` / `]` or at EOF (`{"a": 1,}` -> `{"a": 1}`).
/// - Closes truncated unclosed string literals and nested `{` / `[` containers.
/// - Completes truncated dangling key-value colons (`{"a":` -> `{"a":null}`).
/// - Normalizes single-quoted keys/strings to RFC 8259 double quotes while escaping inner `"`.
/// - Escapes raw unescaped `\n` / `\r` inside string literals.
///
/// Returns `null` on empty input, nesting deeper than 64 levels, or insufficient `out_buf` capacity.
pub fn repairLlmJson(input: string, out_buf: *[]u8): JsonRepairReport? {
    let start: usize = 0;
    let end: usize = input.len;
    while (start < end && isWs(input[start])) : (start += 1) {}
    while (end > start && isWs(input[end - 1])) : (end -= 1) {}
    if (start >= end) return null;

    let report = JsonRepairReport{};
    let pos: usize = 0;
    let stack: [64]u8 = [0; 64];
    let sp: usize = 0;

    let in_string = false;
    let quote_char: u8 = 0;
    let escaped = false;

    let i: usize = start;
    while (i < end) : (i += 1) {
        const c: u8 = input[i];

        if (in_string) {
            if (escaped) {
                if (quote_char == '\'' && c == '\'') {
                    if (!writeByte(out_buf, &pos, '\'')) return null;
                    report.was_modified = true;
                } else {
                    if (!writeByte(out_buf, &pos, '\\')) return null;
                    if (!writeByte(out_buf, &pos, c)) return null;
                }
                escaped = false;
                continue;
            }

            if (c == '\\') {
                escaped = true;
                continue;
            }

            if (c == quote_char) {
                if (!writeByte(out_buf, &pos, '"')) return null;
                in_string = false;
                quote_char = 0;
                continue;
            }

            if (quote_char == '\'' && c == '"') {
                if (!writeRaw(out_buf, &pos, "\\\"")) return null;
                report.was_modified = true;
                continue;
            }

            if (c == '\n') {
                if (!writeRaw(out_buf, &pos, "\\n")) return null;
                report.escaped_newlines += 1;
                report.was_modified = true;
                continue;
            }

            if (c == '\r') {
                if (!writeRaw(out_buf, &pos, "\\r")) return null;
                report.escaped_newlines += 1;
                report.was_modified = true;
                continue;
            }

            if (!writeByte(out_buf, &pos, c)) return null;
            continue;
        }

        // Outside string literal
        if (c == '"') {
            if (!writeByte(out_buf, &pos, '"')) return null;
            in_string = true;
            quote_char = '"';
            continue;
        }

        if (c == '\'') {
            if (!writeByte(out_buf, &pos, '"')) return null;
            in_string = true;
            quote_char = '\'';
            report.normalized_single_quotes += 1;
            report.was_modified = true;
            continue;
        }

        if (c == ',') {
            // Look ahead past whitespace to drop trailing commas before '}' / ']' or EOF.
            let la: usize = i + 1;
            while (la < end && isWs(input[la])) : (la += 1) {}
            if (la >= end || input[la] == '}' || input[la] == ']') {
                report.removed_trailing_commas += 1;
                report.was_modified = true;
                continue;
            }
            if (!writeByte(out_buf, &pos, ',')) return null;
            continue;
        }

        if (c == '{') {
            if (sp >= 64) return null;
            stack[sp] = '}';
            sp += 1;
            if (!writeByte(out_buf, &pos, '{')) return null;
            continue;
        }

        if (c == '[') {
            if (sp >= 64) return null;
            stack[sp] = ']';
            sp += 1;
            if (!writeByte(out_buf, &pos, '[')) return null;
            continue;
        }

        if (c == '}' || c == ']') {
            if (sp > 0) {
                const expected: u8 = stack[sp - 1];
                sp -= 1;
                if (c != expected) {
                    report.was_modified = true;
                }
                if (!writeByte(out_buf, &pos, expected)) return null;
            } else {
                // Drop stray unmatched closing bracket.
                report.was_modified = true;
            }
            continue;
        }

        if (!writeByte(out_buf, &pos, c)) return null;
    }

    // EOF recovery for truncated LLM outputs
    if (escaped) {
        report.was_modified = true;
    }
    if (in_string) {
        if (!writeByte(out_buf, &pos, '"')) return null;
        report.closed_string = true;
        report.was_modified = true;
    }

    // If truncated right after ':' (e.g. `{"key":`), emit `null` before closing brackets.
    if (pos > 0) {
        let last_non_ws: usize = pos;
        while (last_non_ws > 0 && isWs(out_buf[last_non_ws - 1])) : (last_non_ws -= 1) {}
        if (last_non_ws > 0 && out_buf[last_non_ws - 1] == ':') {
            if (!writeRaw(out_buf, &pos, "null")) return null;
            report.was_modified = true;
        }
    }

    while (sp > 0) {
        sp -= 1;
        if (!writeByte(out_buf, &pos, stack[sp])) return null;
        report.closed_brackets += 1;
        report.was_modified = true;
    }

    report.bytes_written = pos;
    return report;
}

// ============================================================================
// 3. Required Field Validation & Self-Correction Retry Prompt Generation
// ============================================================================

/// Expected or observed JSON value type for top-level field validation.
pub enum JsonValueKind {
    Any,
    String,
    Integer,
    Number,
    Boolean,
    Array,
    Object,
    Null,
}

/// Specification for a single required top-level JSON object field.
pub struct RequiredFieldSpec {
    name: string = "";
    expected_kind: JsonValueKind = JsonValueKind.Any;
}

/// Classification of a structured-output validation failure.
pub enum FieldCheckIssueKind {
    Missing,
    TypeMismatch,
    NotAnObject,
}

/// Detailed validation failure entry suitable for telemetry or retry prompt formatting.
pub struct FieldCheckIssue {
    field_name: string = "";
    issue_kind: FieldCheckIssueKind = FieldCheckIssueKind.Missing;
    expected_kind: JsonValueKind = JsonValueKind.Any;
    actual_kind: JsonValueKind = JsonValueKind.Null;
}

/// Summary returned by `checkRequiredJsonFields`.
pub struct RequiredFieldsReport {
    passed: bool = false;
    checked_fields: usize = 0;
    missing_count: usize = 0;
    type_mismatch_count: usize = 0;
    issue_count: usize = 0;
    is_valid_object: bool = false;
}

fn jsonValueKindStr(kind: JsonValueKind): string {
    return switch (kind) {
        JsonValueKind.Any => "any",
        JsonValueKind.String => "string",
        JsonValueKind.Integer => "integer",
        JsonValueKind.Number => "number",
        JsonValueKind.Boolean => "boolean",
        JsonValueKind.Array => "array",
        JsonValueKind.Object => "object",
        JsonValueKind.Null => "null",
    };
}

fn kindMatches(expected: JsonValueKind, actual: JsonValueKind): bool {
    if (expected == JsonValueKind.Any) return true;
    if (expected == actual) return true;
    // JSON integer is also a valid JSON number.
    if (expected == JsonValueKind.Number && actual == JsonValueKind.Integer) return true;
    return false;
}

fn skipJsonString(text: string, start: usize): usize? {
    if (start >= text.len || text[start] != '"') return null;
    let i: usize = start + 1;
    let escaped = false;
    while (i < text.len) : (i += 1) {
        const c: u8 = text[i];
        if (escaped) {
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            return i + 1;
        }
    }
    return null;
}

fn skipJsonValue(text: string, start: usize): usize? {
    let i: usize = start;
    while (i < text.len && isWs(text[i])) : (i += 1) {}
    if (i >= text.len) return null;

    const c: u8 = text[i];
    if (c == '"') {
        return skipJsonString(text, i);
    }
    if (c == '{' || c == '[') {
        let depth: usize = 0;
        let in_str = false;
        let esc = false;
        while (i < text.len) : (i += 1) {
            const ch: u8 = text[i];
            if (in_str) {
                if (esc) {
                    esc = false;
                } else if (ch == '\\') {
                    esc = true;
                } else if (ch == '"') {
                    in_str = false;
                }
                continue;
            }
            if (ch == '"') {
                in_str = true;
            } else if (ch == '{' || ch == '[') {
                depth += 1;
            } else if (ch == '}' || ch == ']') {
                if (depth > 0) {
                    depth -= 1;
                    if (depth == 0) return i + 1;
                }
            }
        }
        return null;
    }

    // Primitive literal (number, true, false, null)
    while (i < text.len && !isWs(text[i]) && text[i] != ',' && text[i] != '}' && text[i] != ']') : (i += 1) {}
    return i;
}

fn classifyJsonValue(text: string, start: usize): JsonValueKind? {
    let i: usize = start;
    while (i < text.len && isWs(text[i])) : (i += 1) {}
    if (i >= text.len) return null;

    const c: u8 = text[i];
    if (c == '"') return JsonValueKind.String;
    if (c == '{') return JsonValueKind.Object;
    if (c == '[') return JsonValueKind.Array;
    if (c == 't' && i + 4 <= text.len && std.mem.eql(u8, text[i .. i + 4], "true")) {
        return JsonValueKind.Boolean;
    }
    if (c == 'f' && i + 5 <= text.len && std.mem.eql(u8, text[i .. i + 5], "false")) {
        return JsonValueKind.Boolean;
    }
    if (c == 'n' && i + 4 <= text.len && std.mem.eql(u8, text[i .. i + 4], "null")) {
        return JsonValueKind.Null;
    }
    if (c == '-' || (c >= '0' && c <= '9')) {
        let j: usize = i;
        let is_float = false;
        while (j < text.len && !isWs(text[j]) && text[j] != ',' && text[j] != '}' && text[j] != ']') : (j += 1) {
            if (text[j] == '.' || text[j] == 'e' || text[j] == 'E') {
                is_float = true;
            }
        }
        return if (is_float) JsonValueKind.Number else JsonValueKind.Integer;
    }
    return null;
}

/// Looks up a top-level field `target_key` inside a JSON object string and returns its `JsonValueKind`,
/// or `null` if the field is absent at the top level.
fn findTopLevelFieldKind(json_text: string, target_key: string): JsonValueKind? {
    let i: usize = 0;
    while (i < json_text.len && isWs(json_text[i])) : (i += 1) {}
    if (i >= json_text.len || json_text[i] != '{') return null;
    i += 1;

    while (i < json_text.len) {
        while (i < json_text.len && isWs(json_text[i])) : (i += 1) {}
        if (i >= json_text.len || json_text[i] == '}') return null;
        if (json_text[i] == ',') {
            i += 1;
            continue;
        }
        if (json_text[i] != '"') return null;

        const key_start: usize = i + 1;
        const key_end_quote: usize = skipJsonString(json_text, i) ?? return null;
        const key_slice: string = json_text[key_start .. key_end_quote - 1];
        i = key_end_quote;

        while (i < json_text.len && isWs(json_text[i])) : (i += 1) {}
        if (i >= json_text.len || json_text[i] != ':') return null;
        i += 1;

        while (i < json_text.len && isWs(json_text[i])) : (i += 1) {}
        if (std.mem.eql(u8, key_slice, target_key)) {
            return classifyJsonValue(json_text, i);
        }

        i = skipJsonValue(json_text, i) ?? return null;
    }
    return null;
}

/// Validates that `json_text` is a top-level JSON object containing every field in `specs`
/// with a matching `JsonValueKind`. Records up to `out_issues.len` failures in `out_issues`.
pub fn checkRequiredJsonFields(
    json_text: string,
    specs: []const RequiredFieldSpec,
    out_issues: *[]FieldCheckIssue,
): RequiredFieldsReport {
    let report = RequiredFieldsReport{
        passed: false,
        checked_fields: specs.len,
        missing_count: 0,
        type_mismatch_count: 0,
        issue_count: 0,
        is_valid_object: false,
    };

    let start: usize = 0;
    while (start < json_text.len && isWs(json_text[start])) : (start += 1) {}
    if (start >= json_text.len || json_text[start] != '{') {
        if (out_issues.len > 0) {
            out_issues[0] = FieldCheckIssue{
                field_name: "",
                issue_kind: FieldCheckIssueKind.NotAnObject,
                expected_kind: JsonValueKind.Object,
                actual_kind: JsonValueKind.Null,
            };
            report.issue_count = 1;
        }
        return report;
    }

    report.is_valid_object = true;

    let idx: usize = 0;
    while (idx < specs.len) : (idx += 1) {
        const spec = specs[idx];
        if (actual := findTopLevelFieldKind(json_text, spec.name)) {
            if (!kindMatches(spec.expected_kind, actual)) {
                report.type_mismatch_count += 1;
                if (report.issue_count < out_issues.len) {
                    out_issues[report.issue_count] = FieldCheckIssue{
                        field_name: spec.name,
                        issue_kind: FieldCheckIssueKind.TypeMismatch,
                        expected_kind: spec.expected_kind,
                        actual_kind: actual,
                    };
                    report.issue_count += 1;
                }
            }
        } else {
            report.missing_count += 1;
            if (report.issue_count < out_issues.len) {
                out_issues[report.issue_count] = FieldCheckIssue{
                    field_name: spec.name,
                    issue_kind: FieldCheckIssueKind.Missing,
                    expected_kind: spec.expected_kind,
                    actual_kind: JsonValueKind.Null,
                };
                report.issue_count += 1;
            }
        }
    }

    report.passed = (report.missing_count == 0 && report.type_mismatch_count == 0);
    return report;
}

/// Formats a concise, actionable LLM self-correction retry prompt from validation `issues[0..issue_count]`
/// into caller-provided `out_buf` (zero heap allocation).
///
/// Returns the number of bytes written, or `null` if `issue_count > issues.len` or `out_buf` is too small.
pub fn formatRepairRetryPrompt(
    issues: []const FieldCheckIssue,
    issue_count: usize,
    schema_hint: string,
    out_buf: *[]u8,
): usize? {
    if (issue_count > issues.len) return null;

    let pos: usize = 0;
    if (!writeRaw(out_buf, &pos, "Your previous JSON output was invalid. Fix these issues and return ONLY valid JSON:\n")) {
        return null;
    }

    let i: usize = 0;
    while (i < issue_count) : (i += 1) {
        const iss = issues[i];
        if (iss.issue_kind == FieldCheckIssueKind.NotAnObject) {
            if (!writeRaw(out_buf, &pos, "- Root value must be a JSON object {...}\n")) return null;
        } else if (iss.issue_kind == FieldCheckIssueKind.Missing) {
            if (!writeRaw(out_buf, &pos, "- Missing required field \"")) return null;
            if (!writeRaw(out_buf, &pos, iss.field_name)) return null;
            if (!writeRaw(out_buf, &pos, "\" (expected ")) return null;
            if (!writeRaw(out_buf, &pos, jsonValueKindStr(iss.expected_kind))) return null;
            if (!writeRaw(out_buf, &pos, ")\n")) return null;
        } else {
            if (!writeRaw(out_buf, &pos, "- Field \"")) return null;
            if (!writeRaw(out_buf, &pos, iss.field_name)) return null;
            if (!writeRaw(out_buf, &pos, "\" has wrong type: expected ")) return null;
            if (!writeRaw(out_buf, &pos, jsonValueKindStr(iss.expected_kind))) return null;
            if (!writeRaw(out_buf, &pos, ", got ")) return null;
            if (!writeRaw(out_buf, &pos, jsonValueKindStr(iss.actual_kind))) return null;
            if (!writeRaw(out_buf, &pos, "\n")) return null;
        }
    }

    if (schema_hint.len > 0) {
        if (!writeRaw(out_buf, &pos, "Expected schema: ")) return null;
        if (!writeRaw(out_buf, &pos, schema_hint)) return null;
    }

    return pos;
}
