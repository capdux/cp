// Source: /data/dev/bun/src/md/inlines.cp
// lib/md/inline_core.cp
// Shared Low-Level Inline Span Helpers, Code Span Scanner, Flanking Rules & HTML Tag Scanner

const std = @import("std");
import {
    decodeUtf8,
    decodeUtf8Backward,
    isAlpha,
    isAlphaNum,
    isUnicodePunctuation,
    isUnicodeWhitespace,
    isWhitespace,
} from "./helpers.cp";
import { Md4cParserState, SpanDetail, SpanType, TextType } from "./types.cp";

pub fn enterSpan(p: *Md4cParserState, span_type: SpanType): ! {
    if (p.image_nesting_level > 0) return;
    p.renderer.enterSpan(span_type, SpanDetail{})?;
}

pub fn leaveSpan(p: *Md4cParserState, span_type: SpanType): ! {
    if (p.image_nesting_level > 0) return;
    p.renderer.leaveSpan(span_type)?;
}

pub fn emitText(p: *Md4cParserState, text_type: TextType, content: []const u8): ! {
    p.renderer.text(text_type, content)?;
}

pub fn emitEmphOpenTags(p: *Md4cParserState, sizes: []const u2): ! {
    let idx: usize = 0;
    while (idx < sizes.len) : (idx += 1) {
        const j = sizes.len - 1 - idx;
        if (sizes[j] == 2) {
            enterSpan(p, .strong)?;
        } else {
            enterSpan(p, .em)?;
        }
    }
}

pub fn emitEmphCloseTags(p: *Md4cParserState, sizes: []const u2): ! {
    for (size of sizes) {
        if (size == 2) {
            leaveSpan(p, .strong)?;
        } else {
            leaveSpan(p, .em)?;
        }
    }
}

pub fn countBackticks(content: []const u8, start: usize): usize {
    let pos = start;
    while (pos < content.len && content[pos] == '`') pos += 1;
    return pos - start;
}

pub fn findCodeSpanEnd(content: []const u8, start: usize, count: usize): usize? {
    let pos = start;
    while (backtick_pos := std.mem.indexOfScalarPos(u8, content, pos, '`')) {
        pos = backtick_pos + 1;
        while (pos < content.len && content[pos] == '`') pos += 1;
        if (pos - backtick_pos == count) {
            return backtick_pos;
        }
    }
    return null;
}

pub fn normalizeCodeSpanContent(content: []const u8): []const u8 {
    if (content.len >= 2) {
        const first_is_space = content[0] == ' ' || content[0] == '\n';
        const last_is_space = content[content.len - 1] == ' ' || content[content.len - 1] == '\n';
        if (first_is_space && last_is_space) {
            if (std.mem.indexOfNone(u8, content, " \n") != null) {
                return content[1 .. content.len - 1];
            }
        }
    }
    return content;
}

pub fn isLeftFlanking(content: []const u8, run_start: usize, run_end: usize): bool {
    if (run_end >= content.len) return false;
    const after_cp = decodeUtf8(content, run_end).codepoint;
    if (isUnicodeWhitespace(after_cp)) return false;
    if (isUnicodePunctuation(after_cp)) {
        if (run_start == 0) return true;
        const before_cp = decodeUtf8Backward(content, run_start).codepoint;
        return isUnicodeWhitespace(before_cp) || isUnicodePunctuation(before_cp);
    }
    return true;
}

pub fn isRightFlanking(content: []const u8, run_start: usize, run_end: usize): bool {
    if (run_start == 0) return false;
    const before_cp = decodeUtf8Backward(content, run_start).codepoint;
    if (isUnicodeWhitespace(before_cp)) return false;
    if (isUnicodePunctuation(before_cp)) {
        if (run_end >= content.len) return true;
        const after_cp = decodeUtf8(content, run_end).codepoint;
        return isUnicodeWhitespace(after_cp) || isUnicodePunctuation(after_cp);
    }
    return true;
}

pub fn canOpenEmphasis(emph_char: u8, content: []const u8, run_start: usize, run_end: usize): bool {
    const lf = isLeftFlanking(content, run_start, run_end);
    if (!lf) return false;
    if (emph_char == '*') return true;
    const rf = isRightFlanking(content, run_start, run_end);
    return !rf || (run_start > 0 && isUnicodePunctuation(decodeUtf8Backward(content, run_start).codepoint));
}

pub fn canCloseEmphasis(emph_char: u8, content: []const u8, run_start: usize, run_end: usize): bool {
    const rf = isRightFlanking(content, run_start, run_end);
    if (!rf) return false;
    if (emph_char == '*') return true;
    const lf = isLeftFlanking(content, run_start, run_end);
    return !lf || (run_end < content.len && isUnicodePunctuation(decodeUtf8(content, run_end).codepoint));
}

pub fn findHtmlTag(content: []const u8, start: usize): usize? {
    if (start + 1 >= content.len) return null;

    let pos = start + 1;
    const c = content[pos];

    if (c == '/') {
        pos += 1;
        if (pos >= content.len || !isAlpha(content[pos])) return null;
        while (pos < content.len && (isAlphaNum(content[pos]) || content[pos] == '-')) pos += 1;
        while (pos < content.len && isWhitespace(content[pos])) pos += 1;
        if (pos < content.len && content[pos] == '>') return pos + 1;
        return null;
    }

    if (c == '!' && pos + 1 < content.len && content[pos + 1] == '-' &&
        pos + 2 < content.len && content[pos + 2] == '-')
    {
        pos += 3;
        if (pos < content.len && content[pos] == '>') return pos + 1;
        if (pos + 1 < content.len && content[pos] == '-' && content[pos + 1] == '>') return pos + 2;
        while (pos + 2 < content.len) {
            if (content[pos] == '-' && content[pos + 1] == '-' && content[pos + 2] == '>') {
                return pos + 3;
            }
            pos += 1;
        }
        return null;
    }

    if (c == '!' && pos + 1 < content.len && content[pos + 1] >= 'A' && content[pos + 1] <= 'Z') {
        pos += 2;
        while (pos < content.len && content[pos] != '>') pos += 1;
        if (pos < content.len) return pos + 1;
        return null;
    }

    if (c == '!' && pos + 7 < content.len &&
        content[pos + 1] == '[' && content[pos + 2] == 'C' && content[pos + 3] == 'D' &&
        content[pos + 4] == 'A' && content[pos + 5] == 'T' && content[pos + 6] == 'A' && content[pos + 7] == '[')
    {
        pos += 8;
        while (pos + 2 < content.len) {
            if (content[pos] == ']' && content[pos + 1] == ']' && content[pos + 2] == '>') {
                return pos + 3;
            }
            pos += 1;
        }
        return null;
    }

    if (c == '?') {
        pos += 1;
        while (pos + 1 < content.len) {
            if (content[pos] == '?' && content[pos + 1] == '>') {
                return pos + 2;
            }
            pos += 1;
        }
        return null;
    }

    if (isAlpha(c)) {
        while (pos < content.len && (isAlphaNum(content[pos]) || content[pos] == '-')) pos += 1;

        while (pos < content.len) {
            let had_ws = false;
            while (pos < content.len && isWhitespace(content[pos])) {
                had_ws = true;
                pos += 1;
            }

            if (pos >= content.len) break;
            if (content[pos] == '>') return pos + 1;
            if (content[pos] == '/' && pos + 1 < content.len && content[pos + 1] == '>') return pos + 2;
            if (!had_ws) return null;

            if (!isAlpha(content[pos]) && content[pos] != '_' && content[pos] != ':') return null;
            while (pos < content.len && (isAlphaNum(content[pos]) ||
                content[pos] == '_' || content[pos] == ':' || content[pos] == '.' || content[pos] == '-'))
            {
                pos += 1;
            }

            const before_eq_ws = pos;
            while (pos < content.len && isWhitespace(content[pos])) pos += 1;
            if (pos < content.len && content[pos] == '=') {
                pos += 1;
                while (pos < content.len && isWhitespace(content[pos])) pos += 1;
                if (pos >= content.len) return null;

                if (content[pos] == '"') {
                    pos += 1;
                    while (pos < content.len && content[pos] != '"') pos += 1;
                    if (pos >= content.len) return null;
                    pos += 1;
                } else if (content[pos] == '\'') {
                    pos += 1;
                    while (pos < content.len && content[pos] != '\'') pos += 1;
                    if (pos >= content.len) return null;
                    pos += 1;
                } else {
                    while (pos < content.len && !isWhitespace(content[pos]) &&
                        content[pos] != '"' && content[pos] != '\'' &&
                        content[pos] != '=' && content[pos] != '<' &&
                        content[pos] != '>' && content[pos] != '`')
                    {
                        pos += 1;
                    }
                }
            } else {
                pos = before_eq_ws;
            }
        }
    }

    return null;
}
