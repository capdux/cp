// @cap/json-repair — Zero-allocation malformed & LLM-generated JSON repair engine for CP
// Clean-room MIT implementation inspired by npm `jsonrepair`, `dirty-json`, and Rust `llm_json`.

import { strEq, isSpace, isAlpha, isDigit, trim } from "cap:ascii";
import { writeStr } from "cap:fmt";

const MAX_STACK: usize = 64;

fn isIdentStart(c: u8) -> bool {
    return isAlpha(c) || c == '_' || c == '$';
}

fn isIdentChar(c: u8) -> bool {
    return isIdentStart(c) || isDigit(c) || c == '-';
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn stripMarkdownFence(raw: string) -> string {
    // Trim leading whitespace and optional ```json ... ``` wrapper common in LLM outputs
    let t = trim(raw);
    if (t.len >= 3 && t[0] == '`' && t[1] == '`' && t[2] == '`') {
        let start: usize = 3;
        let end = t.len;
        while (start < end && t[start] != '\n') {
            start += 1;
        }
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
        if (isSpace(s[i])) {
            i += 1;
            continue;
        }
        // Single-line comment // ...
        if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '/') {
            i += 2;
            while (i < s.len && s[i] != '\n') {
                i += 1;
            }
            continue;
        }
        // Block comment /* ... */
        if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < s.len && !(s[i] == '*' && s[i + 1] == '/')) {
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
                    pos = writeStr("\\\"", out, pos);
                } else if (sc == '\n') {
                    pos = writeStr("\\n", out, pos);
                } else if (sc == '\r') {
                    pos = writeStr("\\r", out, pos);
                } else if (sc == '\t') {
                    pos = writeStr("\\t", out, pos);
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
                pos = writeStr(word, out, pos);
                pos = appendByte(out, pos, '"');
            } else if (strEq(word, "true") || strEq(word, "True") || strEq(word, "TRUE")) {
                pos = writeStr("true", out, pos);
            } else if (strEq(word, "false") || strEq(word, "False") || strEq(word, "FALSE")) {
                pos = writeStr("false", out, pos);
            } else if (strEq(word, "null") || strEq(word, "None") || strEq(word, "NULL") || strEq(word, "undefined")) {
                pos = writeStr("null", out, pos);
            } else {
                // Unknown bare word in value position -> quote as string
                pos = appendByte(out, pos, '"');
                pos = writeStr(word, out, pos);
                pos = appendByte(out, pos, '"');
            }
            continue;
        }

        // Numbers (including negative, floats, scientific)
        if (isDigit(c) || c == '-' || c == '+') {
            if (c == '+') {
                i += 1; // JSON forbids leading '+'
            } else {
                pos = appendByte(out, pos, c);
                i += 1;
            }
            while (i < s.len) {
                const nc = s[i];
                const ok = isDigit(nc) || nc == '.' || nc == 'e' || nc == 'E' || nc == '+' || nc == '-';
                if (!ok) {
                    break;
                }
                pos = appendByte(out, pos, nc);
                i += 1;
            }
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
