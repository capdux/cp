// @cap/json-repair — Zero-allocation malformed & LLM-generated JSON repair engine for CP
// Clean-room MIT implementation inspired by npm `jsonrepair`, `dirty-json`, and Rust `llm_json`.
// Accelerated with comptime 256-entry LUT, 16-lane @Vector(16, u8) SIMD scanning, and @memcpy.

import { strEq, isSpace, trim } from "cap:ascii";

type JrVec16 = @Vector(16, u8);

const MAX_STACK: usize = 64;

const JR_BIT_WS: u8 = 1;
const JR_BIT_IDENT_START: u8 = 2;
const JR_BIT_IDENT_CHAR: u8 = 4;
const JR_BIT_NUM_CHAR: u8 = 8;
const JR_BIT_DIGIT: u8 = 16;

fn jrBuildCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let f: u8 = 0;
        if (c == 32 || c == 9 || c == 10 || c == 13 || c == 11 || c == 12) {
            f |= JR_BIT_WS;
        }
        const is_alpha = (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
        const is_digit = c >= 48 && c <= 57;
        if (is_alpha || c == 95 || c == 36) {
            f |= JR_BIT_IDENT_START | JR_BIT_IDENT_CHAR;
        }
        if (is_digit) {
            f |= JR_BIT_DIGIT | JR_BIT_IDENT_CHAR | JR_BIT_NUM_CHAR;
        }
        if (c == 45) { // '-'
            f |= JR_BIT_IDENT_CHAR | JR_BIT_NUM_CHAR;
        }
        if (c == 46 || c == 101 || c == 69 || c == 43) { // '.', 'e', 'E', '+'
            f |= JR_BIT_NUM_CHAR;
        }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

const JR_CHAR_LUT: [256]u8 = jrBuildCharLut();

fn isIdentStart(c: u8) -> bool {
    return (JR_CHAR_LUT[c as usize] & JR_BIT_IDENT_START) != 0;
}

fn isIdentChar(c: u8) -> bool {
    return (JR_CHAR_LUT[c as usize] & JR_BIT_IDENT_CHAR) != 0;
}

fn jrFindByte(s: string, start: usize, target: u8) -> usize {
    let i = start;
    const len = s.len;
    const v_t: JrVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: JrVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_t)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == target) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && s[i] != target) {
        i += 1;
    }
    return i;
}

fn jrFindStrSpecial(s: string, start: usize, quote: u8) -> usize {
    let i = start;
    const len = s.len;
    const v_q: JrVec16 = @splat(quote);
    const v_bs: JrVec16 = @splat(92 as u8);
    const v_dq: JrVec16 = @splat(34 as u8);
    const v_lf: JrVec16 = @splat(10 as u8);
    const v_cr: JrVec16 = @splat(13 as u8);
    const v_tab: JrVec16 = @splat(9 as u8);
    while (i + 16 <= len) {
        const v: JrVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_q) ||
            @reduce(.Or, v == v_bs) ||
            @reduce(.Or, v == v_dq) ||
            @reduce(.Or, v == v_lf) ||
            @reduce(.Or, v == v_cr) ||
            @reduce(.Or, v == v_tab))
        {
            let k: usize = 0;
            while (k < 16) {
                const c = s[i + k];
                if (c == quote || c == '\\' || (quote == '\'' && c == '"') || c == '\n' || c == '\r' || c == '\t') {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = s[i];
        if (c == quote || c == '\\' || (quote == '\'' && c == '"') || c == '\n' || c == '\r' || c == '\t') {
            return i;
        }
        i += 1;
    }
    return len;
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn jrCopySlice(out: []u8, pos: usize, src: string) -> usize {
    if (pos >= out.len || src.len == 0) {
        return pos;
    }
    const avail = out.len - pos;
    const n = if (src.len < avail) { src.len } else { avail };
    @memcpy(out[pos .. pos + n], src[0..n]);
    return pos + n;
}

fn stripMarkdownFence(raw: string) -> string {
    // Trim leading whitespace and optional ```json ... ``` wrapper common in LLM outputs
    let t = trim(raw);
    if (t.len >= 3 && t[0] == '`' && t[1] == '`' && t[2] == '`') {
        let start = jrFindByte(t, 3, '\n');
        let end = t.len;
        if (start < end && t[start] == '\n') {
            start += 1;
        }
        if (end - start >= 3 && t[end - 3] == '`' && t[end - 2] == '`' && t[end - 1] == '`') {
            end -= 3;
        }
        return trim(t[start..end]);
    }
    return t;
}

fn skipWsAndComments(s: string, start: usize) -> usize {
    let i = start;
    while (i < s.len) {
        if ((JR_CHAR_LUT[s[i] as usize] & JR_BIT_WS) != 0) {
            i += 1;
            continue;
        }
        // Single-line comment // ...
        if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '/') {
            i = jrFindByte(s, i + 2, '\n');
            continue;
        }
        // Block comment /* ... */
        if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < s.len) {
                i = jrFindByte(s, i, '*');
                if (i + 1 >= s.len) { break; }
                if (s[i + 1] == '/') { break; }
                i += 1;
            }
            if (i + 1 < s.len) {
                i += 2;
            } else {
                i = s.len;
            }
            continue;
        }
        break;
    }
    return i;
}

pub fn repairJson(raw: string, out: []u8) -> string {
    const s = stripMarkdownFence(raw);
    let pos: usize = 0;
    let stack: [MAX_STACK]u8 = undefined; // stores expected closing char '}' or ']'
    let sp: usize = 0;

    let i: usize = 0;
    while (i < s.len) {
        i = skipWsAndComments(s, i);
        if (i >= s.len) {
            break;
        }
        const c = s[i];

        // Opening containers
        if (c == '{' || c == '[') {
            pos = appendByte(out, pos, c);
            if (sp < MAX_STACK) {
                stack[sp] = if (c == '{') { '}' as u8 } else { ']' as u8 };
                sp += 1;
            }
            i += 1;
            continue;
        }

        // Closing containers
        if (c == '}' || c == ']') {
            // Strip any trailing comma right before closing bracket
            while (pos > 0 && isSpace(out[pos - 1])) {
                pos -= 1;
            }
            if (pos > 0 && out[pos - 1] == ',') {
                pos -= 1;
            }
            if (sp > 0) {
                pos = appendByte(out, pos, stack[sp - 1]);
                sp -= 1;
            } else {
                pos = appendByte(out, pos, c);
            }
            i += 1;
            continue;
        }

        // Punctuation ':' and ','
        if (c == ':' || c == ',') {
            // Skip duplicate commas
            if (c == ',' && pos > 0 && out[pos - 1] == ',') {
                i += 1;
                continue;
            }
            pos = appendByte(out, pos, c);
            i += 1;
            continue;
        }

        // Double-quoted or single-quoted string
        if (c == '"' || c == '\'') {
            const quote = c;
            pos = appendByte(out, pos, '"');
            i += 1;
            while (i < s.len) {
                const next_sp = jrFindStrSpecial(s, i, quote);
                if (next_sp > i) {
                    pos = jrCopySlice(out, pos, s[i..next_sp]);
                    i = next_sp;
                    if (i >= s.len) { break; }
                }
                const sc = s[i];
                if (sc == '\\' && i + 1 < s.len) {
                    const nxt = s[i + 1];
                    if (quote == '\'' && nxt == '\'') {
                        pos = appendByte(out, pos, '\'');
                    } else {
                        pos = appendByte(out, pos, '\\');
                        pos = appendByte(out, pos, nxt);
                    }
                    i += 2;
                    continue;
                }
                if (sc == quote) {
                    i += 1;
                    break;
                }
                if (quote == '\'' && sc == '"') {
                    pos = jrCopySlice(out, pos, "\\\"");
                } else if (sc == '\n') {
                    pos = jrCopySlice(out, pos, "\\n");
                } else if (sc == '\r') {
                    pos = jrCopySlice(out, pos, "\\r");
                } else if (sc == '\t') {
                    pos = jrCopySlice(out, pos, "\\t");
                } else {
                    pos = appendByte(out, pos, sc);
                }
                i += 1;
            }
            pos = appendByte(out, pos, '"');
            continue;
        }

        // Bare identifiers: unquoted object keys, Python True/False/None, undefined, or bare string values
        if (isIdentStart(c)) {
            const start_id = i;
            while (i < s.len && isIdentChar(s[i])) {
                i += 1;
            }
            const word = s[start_id..i];
            const after = skipWsAndComments(s, i);
            if (after < s.len && s[after] == ':') {
                // Unquoted object key -> quote it
                pos = appendByte(out, pos, '"');
                pos = jrCopySlice(out, pos, word);
                pos = appendByte(out, pos, '"');
            } else if (strEq(word, "true") || strEq(word, "True") || strEq(word, "TRUE")) {
                pos = jrCopySlice(out, pos, "true");
            } else if (strEq(word, "false") || strEq(word, "False") || strEq(word, "FALSE")) {
                pos = jrCopySlice(out, pos, "false");
            } else if (strEq(word, "null") || strEq(word, "None") || strEq(word, "NULL") || strEq(word, "undefined")) {
                pos = jrCopySlice(out, pos, "null");
            } else {
                // Unknown bare word in value position -> quote as string
                pos = appendByte(out, pos, '"');
                pos = jrCopySlice(out, pos, word);
                pos = appendByte(out, pos, '"');
            }
            continue;
        }

        // Numbers (including negative, floats, scientific)
        if ((JR_CHAR_LUT[c as usize] & JR_BIT_DIGIT) != 0 || c == '-' || c == '+') {
            if (c == '+') {
                i += 1; // JSON forbids leading '+'
            }
            const num_start = i;
            if (i < s.len && s[i] == '-') {
                i += 1;
            }
            while (i < s.len && (JR_CHAR_LUT[s[i] as usize] & JR_BIT_NUM_CHAR) != 0) {
                i += 1;
            }
            pos = jrCopySlice(out, pos, s[num_start..i]);
            continue;
        }

        // Skip stray unexpected characters
        i += 1;
    }

    // Strip any dangling trailing comma before auto-closing truncated containers
    while (pos > 0 && isSpace(out[pos - 1])) {
        pos -= 1;
    }
    if (pos > 0 && out[pos - 1] == ',') {
        pos -= 1;
    }
    // Auto-close any unclosed objects/arrays (common in truncated LLM streams)
    while (sp > 0) {
        sp -= 1;
        pos = appendByte(out, pos, stack[sp]);
    }
    return out[0..pos];
}

export {
    repairJson,
};
