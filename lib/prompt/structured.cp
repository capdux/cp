// Origin: Pure-CP Stdlib (AI Agent Structured Output Extraction, Think-Tag Split & Guardrails) | Created: 2026-11-22
// Pure CP zero-allocation AI Agent structured output extraction, reasoning/think-tag splitting,
// lightweight lexical JSON top-level key pre-checking, and PII/secret redaction guardrails.

/// Result of splitting a reasoning/think block (`<think>...</think>` or `<reasoning>...</reasoning>`)
/// from a reasoning LLM response. Both `thought` and `answer` are zero-copy slices of the input.
pub struct ThoughtSplit {
    thought: string = "",
    answer: string = "",
    has_thought: bool = false,
    unclosed: bool = false,
}

/// Zero-copy extracted Markdown fenced code block with byte offsets into the source text.
pub struct ExtractedBlock {
    lang: string = "",
    content: string = "",
    start: usize = 0,
    end: usize = 0,
}

/// Summary statistics returned by `redactSensitiveText`.
pub struct RedactStats {
    bytes_written: usize = 0,
    redacted_emails: usize = 0,
    redacted_keys: usize = 0,
    redacted_phones: usize = 0,
}

const REDACTED_MARKER: string = "[REDACTED]";
const MAX_JSON_NESTING: usize = 256;

// ============================================================================
// Internal ASCII / Zero-Allocation Helpers
// ============================================================================

type PsVec16 = @Vector(16, u8);

fn makePsLowerLut(): [u8; 256] {
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

fn makePsClassLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[' '] = 1 | 8;
    lut['\t'] = 1 | 8;
    lut['\n'] = 1;
    lut['\r'] = 1;
    let i: usize = 'a';
    while (i <= 'z') : (i += 1) {
        lut[i] |= 2;
    }
    i = 'A';
    while (i <= 'Z') : (i += 1) {
        lut[i] |= 2;
    }
    i = '0';
    while (i <= '9') : (i += 1) {
        lut[i] |= 4;
    }
    return lut;
}

const PS_LOWER_LUT: [u8; 256] = makePsLowerLut();
const PS_CLASS_LUT: [u8; 256] = makePsClassLut();

fn asciiLower(c: u8): u8 {
    return PS_LOWER_LUT[@as(usize, c)];
}

fn isAsciiSpace(c: u8): bool {
    return (PS_CLASS_LUT[@as(usize, c)] & 1) != 0;
}

fn isSpaceOrTab(c: u8): bool {
    return (PS_CLASS_LUT[@as(usize, c)] & 8) != 0;
}

fn isAsciiAlpha(c: u8): bool {
    return (PS_CLASS_LUT[@as(usize, c)] & 2) != 0;
}

fn isAsciiDigit(c: u8): bool {
    return (PS_CLASS_LUT[@as(usize, c)] & 4) != 0;
}

fn isAsciiAlnum(c: u8): bool {
    return (PS_CLASS_LUT[@as(usize, c)] & 6) != 0;
}

fn trimAscii(s: string): string {
    let start: usize = 0;
    while (start < s.len && isAsciiSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isAsciiSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn equalsIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    const lo_A: PsVec16 = @splat(65 as u8);
    const hi_Z: PsVec16 = @splat(90 as u8);
    const case_bit: PsVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: PsVec16 = a[i .. i + 16][0..16].*;
        const vb: PsVec16 = b[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const mb = (vb >= lo_A) & (vb <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        const lb = @select(u8, mb, vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) {
            return false;
        }
    }
    while (i < a.len) : (i += 1) {
        if (asciiLower(a[i]) != asciiLower(b[i])) {
            return false;
        }
    }
    return true;
}

fn startsWithIgnoreCaseAt(haystack: string, pos: usize, needle: string): bool {
    if (pos + needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    const lo_A: PsVec16 = @splat(65 as u8);
    const hi_Z: PsVec16 = @splat(90 as u8);
    const case_bit: PsVec16 = @splat(0x20 as u8);
    while (i + 16 <= needle.len) : (i += 16) {
        const va: PsVec16 = haystack[pos + i .. pos + i + 16][0..16].*;
        const vb: PsVec16 = needle[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const mb = (vb >= lo_A) & (vb <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        const lb = @select(u8, mb, vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) {
            return false;
        }
    }
    while (i < needle.len) : (i += 1) {
        if (asciiLower(haystack[pos + i]) != asciiLower(needle[i])) {
            return false;
        }
    }
    return true;
}

fn startsWithExactAt(haystack: string, pos: usize, needle: string): bool {
    if (pos + needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i + 16 <= needle.len) : (i += 16) {
        const va: PsVec16 = haystack[pos + i .. pos + i + 16][0..16].*;
        const vb: PsVec16 = needle[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) {
            return false;
        }
    }
    while (i < needle.len) : (i += 1) {
        if (haystack[pos + i] != needle[i]) {
            return false;
        }
    }
    return true;
}

fn findIgnoreCase(haystack: string, start_pos: usize, needle: string): usize? {
    if (needle.len == 0 || start_pos + needle.len > haystack.len) {
        return null;
    }
    const first_lo: u8 = asciiLower(needle[0]);
    const first_hi: u8 = if (first_lo >= 'a' && first_lo <= 'z') first_lo - 32 else first_lo;
    const v_lo: PsVec16 = @splat(first_lo);
    const v_hi: PsVec16 = @splat(first_hi);
    let i: usize = start_pos;
    const limit: usize = haystack.len - needle.len;
    while (i <= limit) {
        if (i + 16 <= limit + 1) {
            const chunk: PsVec16 = haystack[i .. i + 16][0..16].*;
            const hit = (chunk == v_lo) | (chunk == v_hi);
            if (!@reduce(.Or, hit)) {
                i += 16;
                continue;
            }
        }
        if (startsWithIgnoreCaseAt(haystack, i, needle)) {
            return i;
        }
        i += 1;
    }
    return null;
}

// ============================================================================
// 1. Reasoning / Think-Tag Splitting (Zero-Copy)
// ============================================================================

/// Splits `<think>...</think>` or `<reasoning>...</reasoning>` blocks from an LLM response.
/// Handles case-insensitive tags, nested same-kind tags, leading/trailing whitespace,
/// and unclosed tags (e.g., truncated streaming responses).
pub fn splitThoughtBlock(text: string): ThoughtSplit {
    const think_open: string = "<think>";
    const think_close: string = "</think>";
    const reason_open: string = "<reasoning>";
    const reason_close: string = "</reasoning>";

    const think_pos = findIgnoreCase(text, 0, think_open);
    const reason_pos = findIgnoreCase(text, 0, reason_open);

    let open_pos: usize = 0;
    let open_tag: string = "";
    let close_tag: string = "";
    let found_open: bool = false;

    if (tp := think_pos) {
        open_pos = tp;
        open_tag = think_open;
        close_tag = think_close;
        found_open = true;
    }

    if (rp := reason_pos) {
        if (!found_open || rp < open_pos) {
            open_pos = rp;
            open_tag = reason_open;
            close_tag = reason_close;
            found_open = true;
        }
    }

    if (!found_open) {
        return ThoughtSplit{
            thought: "",
            answer: trimAscii(text),
            has_thought: false,
            unclosed: false,
        };
    }

    const content_start: usize = open_pos + open_tag.len;
    let scan: usize = content_start;
    let depth: usize = 1;

    while (scan < text.len) {
        if (startsWithIgnoreCaseAt(text, scan, open_tag)) {
            depth += 1;
            scan += open_tag.len;
            continue;
        }
        if (startsWithIgnoreCaseAt(text, scan, close_tag)) {
            depth -= 1;
            if (depth == 0) {
                const thought_slice = trimAscii(text[content_start..scan]);
                const after_close = trimAscii(text[scan + close_tag.len..]);
                let final_answer = after_close;
                if (final_answer.len == 0 && open_pos > 0) {
                    final_answer = trimAscii(text[0..open_pos]);
                }
                return ThoughtSplit{
                    thought: thought_slice,
                    answer: final_answer,
                    has_thought: true,
                    unclosed: false,
                };
            }
            scan += close_tag.len;
            continue;
        }
        scan += 1;
    }

    // Unclosed tag: treat everything after the opening tag as thought, and any prefix as answer.
    const prefix_answer = if (open_pos > 0) trimAscii(text[0..open_pos]) else "";
    return ThoughtSplit{
        thought: trimAscii(text[content_start..]),
        answer: prefix_answer,
        has_thought: true,
        unclosed: true,
    };
}

// ============================================================================
// 2. Markdown Code Fence & Balanced JSON Extraction (Zero-Copy)
// ============================================================================

struct LineInfo {
    start: usize = 0,
    end: usize = 0,
    next_start: usize = 0,
}

fn readLine(text: string, pos: usize): LineInfo {
    let i: usize = pos;
    while (i < text.len && text[i] != '\n') : (i += 1) {}
    let end: usize = i;
    if (end > pos && text[end - 1] == '\r') {
        end -= 1;
    }
    const next_start: usize = if (i < text.len && text[i] == '\n') i + 1 else text.len;
    return LineInfo{
        start: pos,
        end: end,
        next_start: next_start,
    };
}

struct ParsedFence {
    valid: bool = false,
    marker: u8 = '`',
    marker_len: usize = 0,
    lang: string = "",
}

fn parseOpenFenceLine(line: string): ParsedFence {
    let i: usize = 0;
    let lead_spaces: usize = 0;
    while (i < line.len && line[i] == ' ' && lead_spaces < 3) {
        i += 1;
        lead_spaces += 1;
    }
    if (i >= line.len) {
        return ParsedFence{ valid: false };
    }
    const ch: u8 = line[i];
    if (ch != '`' && ch != '~') {
        return ParsedFence{ valid: false };
    }
    let count: usize = 0;
    while (i < line.len && line[i] == ch) {
        count += 1;
        i += 1;
    }
    if (count < 3) {
        return ParsedFence{ valid: false };
    }
    // CommonMark: backtick fence info strings cannot contain backticks.
    if (ch == '`') {
        let check: usize = i;
        while (check < line.len) : (check += 1) {
            if (line[check] == '`') {
                return ParsedFence{ valid: false };
            }
        }
    }
    while (i < line.len && isSpaceOrTab(line[i])) : (i += 1) {}
    const lang_start: usize = i;
    while (i < line.len && !isSpaceOrTab(line[i])) : (i += 1) {}
    return ParsedFence{
        valid: true,
        marker: ch,
        marker_len: count,
        lang: line[lang_start..i],
    };
}

fn isCloseFenceLine(line: string, marker: u8, min_len: usize): bool {
    let i: usize = 0;
    let lead_spaces: usize = 0;
    while (i < line.len && line[i] == ' ' && lead_spaces < 3) {
        i += 1;
        lead_spaces += 1;
    }
    let count: usize = 0;
    while (i < line.len && line[i] == marker) {
        count += 1;
        i += 1;
    }
    if (count < min_len) {
        return false;
    }
    while (i < line.len) : (i += 1) {
        if (!isSpaceOrTab(line[i])) {
            return false;
        }
    }
    return true;
}

/// Extracts a closed Markdown fenced code block (```lang ... ``` or ~~~lang ... ~~~).
/// If `preferred_lang` is non-empty, returns the first closed block whose language tag
/// matches `preferred_lang` (ASCII case-insensitive); if `preferred_lang` is empty,
/// returns the first closed code block regardless of language tag.
pub fn extractCodeFence(text: string, preferred_lang: string): ExtractedBlock? {
    let pos: usize = 0;
    while (pos < text.len) {
        const line_info = readLine(text, pos);
        const line = text[line_info.start..line_info.end];
        const open = parseOpenFenceLine(line);
        if (!open.valid) {
            pos = line_info.next_start;
            continue;
        }

        const block_start: usize = line_info.start;
        const content_start: usize = line_info.next_start;
        let scan: usize = content_start;
        let closed: bool = false;
        let content_end: usize = content_start;
        let block_end: usize = content_start;
        let resume_pos: usize = text.len;

        while (scan < text.len) {
            const inner = readLine(text, scan);
            const inner_line = text[inner.start..inner.end];
            if (isCloseFenceLine(inner_line, open.marker, open.marker_len)) {
                closed = true;
                content_end = inner.start;
                if (content_end > content_start && text[content_end - 1] == '\n') {
                    content_end -= 1;
                    if (content_end > content_start && text[content_end - 1] == '\r') {
                        content_end -= 1;
                    }
                }
                block_end = inner.end;
                resume_pos = inner.next_start;
                break;
            }
            scan = inner.next_start;
        }

        if (!closed) {
            return null;
        }

        if (preferred_lang.len == 0 || equalsIgnoreCase(open.lang, preferred_lang)) {
            return ExtractedBlock{
                lang: open.lang,
                content: text[content_start..content_end],
                start: block_start,
                end: block_end,
            };
        }

        pos = resume_pos;
    }

    return null;
}

fn scanBalancedJsonAt(text: string, start: usize): usize? {
    const first: u8 = text[start];
    if (first != '{' && first != '[') {
        return null;
    }

    let stack: [MAX_JSON_NESTING]u8 = undefined;
    let depth: usize = 0;
    let in_string: bool = false;
    let escaped: bool = false;

    let i: usize = start;
    while (i < text.len) : (i += 1) {
        const ch: u8 = text[i];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                in_string = false;
            }
            continue;
        }

        if (ch == '"') {
            in_string = true;
            continue;
        }

        if (ch == '{') {
            if (depth >= MAX_JSON_NESTING) {
                return null;
            }
            stack[depth] = '}';
            depth += 1;
            continue;
        }

        if (ch == '[') {
            if (depth >= MAX_JSON_NESTING) {
                return null;
            }
            stack[depth] = ']';
            depth += 1;
            continue;
        }

        if (ch == '}' || ch == ']') {
            if (depth == 0 || stack[depth - 1] != ch) {
                return null;
            }
            depth -= 1;
            if (depth == 0) {
                return i + 1;
            }
        }
    }

    return null;
}

/// Extracts the first syntactically balanced JSON object (`{...}`) or array (`[...]`)
/// slice from mixed natural-language LLM output. Correctly ignores braces/brackets
/// inside JSON string literals and handles `\"` / `\\` escapes.
pub fn extractFirstJsonSlice(text: string): string? {
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        const ch: u8 = text[i];
        if (ch == '{' || ch == '[') {
            if (end_pos := scanBalancedJsonAt(text, i)) {
                return text[i..end_pos];
            }
        }
    }
    return null;
}

// ============================================================================
// 3. Lightweight Lexical JSON Top-Level Key Validator (Zero-Allocation)
// ============================================================================

fn skipJsonWhitespace(s: string, start: usize): usize {
    let i: usize = start;
    while (i < s.len && isAsciiSpace(s[i])) : (i += 1) {}
    return i;
}

/// Scans a JSON string starting at `s[quote_pos] == '"'`.
/// Returns the byte index immediately AFTER the closing quote, or `null` if malformed/unclosed.
fn scanJsonStringEnd(s: string, quote_pos: usize): usize? {
    if (quote_pos >= s.len || s[quote_pos] != '"') {
        return null;
    }
    let i: usize = quote_pos + 1;
    let escaped: bool = false;
    while (i < s.len) : (i += 1) {
        const ch: u8 = s[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            return i + 1;
        }
    }
    return null;
}

/// Compares a raw JSON string interior (between quotes) against an unescaped expected key,
/// decoding common single-character JSON escapes (`\"`, `\\`, `\/`, `\b`, `\f`, `\n`, `\r`, `\t`).
fn jsonRawKeyMatches(raw_inner: string, expected: string): bool {
    let ri: usize = 0;
    let ei: usize = 0;
    while (ri < raw_inner.len) {
        if (ei >= expected.len) {
            return false;
        }
        let ch: u8 = raw_inner[ri];
        if (ch == '\\') {
            if (ri + 1 >= raw_inner.len) {
                return false;
            }
            const next_ch: u8 = raw_inner[ri + 1];
            if (next_ch == '"' || next_ch == '\\' || next_ch == '/') {
                ch = next_ch;
            } else if (next_ch == 'n') {
                ch = '\n';
            } else if (next_ch == 'r') {
                ch = '\r';
            } else if (next_ch == 't') {
                ch = '\t';
            } else if (next_ch == 'b') {
                ch = 8;
            } else if (next_ch == 'f') {
                ch = 12;
            } else {
                // Unknown or \uXXXX escape: fall back to literal comparison of both chars
                if (expected[ei] != '\\') {
                    return false;
                }
                ri += 1;
                ei += 1;
                continue;
            }
            ri += 2;
        } else {
            ri += 1;
        }
        if (ch != expected[ei]) {
            return false;
        }
        ei += 1;
    }
    return ei == expected.len;
}

/// Skips one JSON value starting at `pos` (whitespace already skipped).
/// Returns the byte index immediately after the value, or `null` if malformed.
fn skipJsonValue(s: string, pos: usize): usize? {
    if (pos >= s.len) {
        return null;
    }
    const ch: u8 = s[pos];
    if (ch == '"') {
        return scanJsonStringEnd(s, pos);
    }
    if (ch == '{' || ch == '[') {
        return scanBalancedJsonAt(s, pos);
    }
    if (ch == ',' || ch == '}' || ch == ']' || ch == ':') {
        return null;
    }
    // Primitive token: number, true, false, null
    let i: usize = pos;
    while (i < s.len) : (i += 1) {
        const c: u8 = s[i];
        if (isAsciiSpace(c) || c == ',' || c == '}' || c == ']') {
            break;
        }
    }
    if (i == pos) {
        return null;
    }
    return i;
}

fn jsonObjectHasSingleTopKey(trimmed: string, target_key: string): bool {
    if (trimmed.len < 2 || trimmed[0] != '{') {
        return false;
    }

    let pos: usize = skipJsonWhitespace(trimmed, 1);
    if (pos >= trimmed.len) {
        return false;
    }
    if (trimmed[pos] == '}') {
        return false;
    }

    while (pos < trimmed.len) {
        pos = skipJsonWhitespace(trimmed, pos);
        if (pos >= trimmed.len || trimmed[pos] != '"') {
            return false;
        }
        const key_end_opt = scanJsonStringEnd(trimmed, pos);
        let key_end: usize = 0;
        if (ke := key_end_opt) {
            key_end = ke;
        } else {
            return false;
        }
        const raw_key = trimmed[pos + 1 .. key_end - 1];

        pos = skipJsonWhitespace(trimmed, key_end);
        if (pos >= trimmed.len || trimmed[pos] != ':') {
            return false;
        }
        pos = skipJsonWhitespace(trimmed, pos + 1);
        const val_end_opt = skipJsonValue(trimmed, pos);
        let val_end: usize = 0;
        if (ve := val_end_opt) {
            val_end = ve;
        } else {
            return false;
        }

        if (jsonRawKeyMatches(raw_key, target_key)) {
            return true;
        }

        pos = skipJsonWhitespace(trimmed, val_end);
        if (pos >= trimmed.len) {
            return false;
        }
        if (trimmed[pos] == ',') {
            pos += 1;
            continue;
        }
        if (trimmed[pos] == '}') {
            return false;
        }
        return false;
    }

    return false;
}

/// Checks whether a JSON object contains all `required_keys` at the TOP LEVEL only.
/// Zero heap allocation; keys inside nested objects or arrays are strictly ignored.
pub fn jsonHasTopLevelKeys(json_obj: string, required_keys: []const string): bool {
    const trimmed = trimAscii(json_obj);
    if (trimmed.len < 2 || trimmed[0] != '{') {
        return false;
    }
    // Verify the top-level object is syntactically balanced before scanning.
    if (end_pos := scanBalancedJsonAt(trimmed, 0)) {
        if (end_pos != trimmed.len) {
            return false;
        }
    } else {
        return false;
    }

    if (required_keys.len == 0) {
        return true;
    }

    let idx: usize = 0;
    while (idx < required_keys.len) : (idx += 1) {
        if (!jsonObjectHasSingleTopKey(trimmed, required_keys[idx])) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// 4. PII & API Key Redaction Guardrail (Zero-Allocation)
// ============================================================================

fn isKeyChar(c: u8): bool {
    return isAsciiAlnum(c) || c == '_' || c == '-';
}

fn isEmailLocalChar(c: u8): bool {
    return isAsciiAlnum(c) || c == '.' || c == '_' || c == '%' || c == '+' || c == '-';
}

fn isEmailDomainChar(c: u8): bool {
    return isAsciiAlnum(c) || c == '-';
}

fn isWordBoundaryBefore(input: string, pos: usize): bool {
    if (pos == 0) {
        return true;
    }
    const prev: u8 = input[pos - 1];
    return !isAsciiAlnum(prev) && prev != '_';
}

/// Attempts to match a known API key pattern (`sk-...`, `ghp_...`, `AKIA...`) at `pos`.
/// Returns the end byte offset if matched, or `null` otherwise.
fn matchApiKeyAt(input: string, pos: usize): usize? {
    if (!isWordBoundaryBefore(input, pos)) {
        return null;
    }

    if (startsWithExactAt(input, pos, "sk-")) {
        let end: usize = pos + 3;
        while (end < input.len && isKeyChar(input[end])) : (end += 1) {}
        if (end - (pos + 3) >= 8) {
            return end;
        }
        return null;
    }

    if (startsWithExactAt(input, pos, "ghp_")) {
        let end: usize = pos + 4;
        while (end < input.len && (isAsciiAlnum(input[end]) || input[end] == '_')) : (end += 1) {}
        if (end - (pos + 4) >= 8) {
            return end;
        }
        return null;
    }

    if (startsWithExactAt(input, pos, "AKIA")) {
        let end: usize = pos + 4;
        while (end < input.len && isAsciiAlnum(input[end])) : (end += 1) {}
        if (end - (pos + 4) >= 12) {
            return end;
        }
        return null;
    }

    return null;
}

/// Attempts to match an email address `<local>@<domain>.<tld>` starting at `pos`.
fn matchEmailAt(input: string, pos: usize): usize? {
    if (pos > 0 && isEmailLocalChar(input[pos - 1])) {
        return null;
    }
    if (pos >= input.len || !isAsciiAlnum(input[pos])) {
        return null;
    }

    let i: usize = pos;
    while (i < input.len && isEmailLocalChar(input[i])) : (i += 1) {}
    if (i == pos || i >= input.len || input[i] != '@') {
        return null;
    }
    if (input[i - 1] == '.') {
        return null;
    }

    // Parse domain labels separated by '.'
    i += 1;
    let label_count: usize = 0;
    let last_label_start: usize = i;
    let last_label_end: usize = i;

    while (i < input.len) {
        const label_start: usize = i;
        if (!isAsciiAlnum(input[i])) {
            break;
        }
        while (i < input.len && isEmailDomainChar(input[i])) : (i += 1) {}
        if (i == label_start || input[i - 1] == '-') {
            return null;
        }
        label_count += 1;
        last_label_start = label_start;
        last_label_end = i;

        if (i < input.len && input[i] == '.') {
            if (i + 1 < input.len && isAsciiAlnum(input[i + 1])) {
                i += 1;
                continue;
            }
            break;
        }
        break;
    }

    if (label_count < 2) {
        return null;
    }

    // TLD must be at least 2 ASCII letters.
    const tld_len: usize = last_label_end - last_label_start;
    if (tld_len < 2) {
        return null;
    }
    let t: usize = last_label_start;
    while (t < last_label_end) : (t += 1) {
        if (!isAsciiAlpha(input[t])) {
            return null;
        }
    }

    return last_label_end;
}

/// Attempts to match a long digit sequence or phone number (>= 10 digits) starting at `pos`.
fn matchPhoneOrLongDigitsAt(input: string, pos: usize): usize? {
    if (pos >= input.len) {
        return null;
    }
    if (!isWordBoundaryBefore(input, pos)) {
        return null;
    }

    let i: usize = pos;
    if (input[i] == '+') {
        if (i + 1 >= input.len || !isAsciiDigit(input[i + 1])) {
            return null;
        }
        i += 1;
    } else if (!isAsciiDigit(input[i])) {
        return null;
    }

    let digits: usize = 0;
    let last_digit_end: usize = i;
    let prev_sep: bool = false;

    while (i < input.len) {
        const ch: u8 = input[i];
        if (isAsciiDigit(ch)) {
            digits += 1;
            i += 1;
            last_digit_end = i;
            prev_sep = false;
            continue;
        }
        if ((ch == '-' || ch == ' ') && !prev_sep && i + 1 < input.len && isAsciiDigit(input[i + 1])) {
            prev_sep = true;
            i += 1;
            continue;
        }
        break;
    }

    if (digits < 10) {
        return null;
    }

    // Do not match if immediately glued to an ASCII letter or underscore (e.g., hex/identifier).
    if (last_digit_end < input.len && (isAsciiAlpha(input[last_digit_end]) || input[last_digit_end] == '_')) {
        return null;
    }

    return last_digit_end;
}

fn appendMarker(out_buf: *[]u8, written: *usize, text: string): bool {
    if (out_buf.len - written.* < text.len) {
        return false;
    }
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        out_buf[written.* + i] = text[i];
    }
    written.* += text.len;
    return true;
}

/// Redacts sensitive PII and secrets from `input` into caller-provided `out_buf` with zero heap allocation.
/// Detects and replaces:
///   - API keys (`sk-...`, `ghp_...`, `AKIA...`)
///   - Email addresses (`user@domain.tld`)
///   - Long digit sequences / phone numbers (>= 10 digits)
/// Each matched span is replaced with `[REDACTED]`.
/// Returns `null` if `out_buf` does not have sufficient capacity.
pub fn redactSensitiveText(input: string, out_buf: *[]u8): RedactStats? {
    let stats = RedactStats{};
    let written: usize = 0;
    let i: usize = 0;

    while (i < input.len) {
        if (key_end := matchApiKeyAt(input, i)) {
            if (!appendMarker(out_buf, *written, REDACTED_MARKER)) {
                return null;
            }
            stats.redacted_keys += 1;
            i = key_end;
            continue;
        }

        if (email_end := matchEmailAt(input, i)) {
            if (!appendMarker(out_buf, *written, REDACTED_MARKER)) {
                return null;
            }
            stats.redacted_emails += 1;
            i = email_end;
            continue;
        }

        if (phone_end := matchPhoneOrLongDigitsAt(input, i)) {
            if (!appendMarker(out_buf, *written, REDACTED_MARKER)) {
                return null;
            }
            stats.redacted_phones += 1;
            i = phone_end;
            continue;
        }

        if (written >= out_buf.len) {
            return null;
        }
        out_buf[written] = input[i];
        written += 1;
        i += 1;
    }

    stats.bytes_written = written;
    return stats;
}
