// Source: Absorbed from /data/dev/bun/src/md/line_analysis.cp | Synced: 2026-11-20T09:22:57Z
// lib/md/line_analysis.cp
// CommonMark & GFM Line Classification (Setext, ATX, HR, Code Fences, HTML Blocks 1-7, Tables, Containers)

const std = @import("std");
import { isBlank, isNewline, isAlpha, isAlphaNum, isDigit, asciiCaseEql } from "./helpers.cp";
import { Align, Container, Md4cParserState, TABLE_MAXCOLCOUNT } from "./types.cp";

pub struct SetextResult {
    is_setext: bool = false,
    level: u32 = 0,
}

pub struct AtxHeaderResult {
    is_atx: bool = false,
    level: u32 = 0,
    content_beg: u32 = 0,
}

pub struct CodeFenceResult {
    is_fence: bool = false,
    fence_data: u32 = 0,
}

pub struct TableUnderlineResult {
    is_underline: bool = false,
    col_count: u32 = 0,
}

pub struct ContainerMarkResult {
    is_container: bool = false,
    container: Container = Container{},
    off: u32 = 0,
}

pub fn isSetextUnderline(p: *const Md4cParserState, off: u32): SetextResult {
    const c = p.text[off];
    if (c != '=' && c != '-') return SetextResult{};

    let pos = off;
    while (pos < p.size && p.text[pos] == c) pos += 1;
    while (pos < p.size && isBlank(p.text[pos])) pos += 1;

    if (pos >= p.size || isNewline(p.text[pos])) {
        const level: u32 = if (c == '=') 1 else 2;
        return SetextResult{ is_setext: true, level: level };
    }
    return SetextResult{};
}

pub fn isHrLine(p: *const Md4cParserState, off: u32): bool {
    const c = p.text[off];
    if (c != '-' && c != '_' && c != '*') return false;

    let pos = off;
    let count: u32 = 0;
    while (pos < p.size && !isNewline(p.text[pos])) {
        if (p.text[pos] == c) {
            count += 1;
        } else if (!isBlank(p.text[pos])) {
            return false;
        }
        pos += 1;
    }
    return count >= 3;
}

pub fn isAtxHeaderLine(p: *const Md4cParserState, off: u32): AtxHeaderResult {
    let pos = off;
    let level: u32 = 0;

    while (pos < p.size && p.text[pos] == '#') {
        level += 1;
        pos += 1;
    }

    if (level == 0 || level > 6) return AtxHeaderResult{};

    if (pos < p.size && !isBlank(p.text[pos]) && !isNewline(p.text[pos])) {
        if (!p.flags.permissive_atx_headers) return AtxHeaderResult{};
    }

    while (pos < p.size && isBlank(p.text[pos])) pos += 1;
    return AtxHeaderResult{ is_atx: true, level: level, content_beg: pos };
}

pub fn isOpeningCodeFence(p: *const Md4cParserState, off: u32): CodeFenceResult {
    if (off >= p.size) return CodeFenceResult{};
    const fence_char = p.text[off];
    let pos = off;
    let count: u32 = 0;

    while (pos < p.size && p.text[pos] == fence_char) {
        count += 1;
        pos += 1;
    }

    if (count < 3) return CodeFenceResult{};

    if (fence_char == '`') {
        let check = pos;
        while (check < p.size && !isNewline(p.text[check])) {
            if (p.text[check] == '`') return CodeFenceResult{};
            check += 1;
        }
    }

    const data: u32 = (fence_char as u32) | (count << 8);
    return CodeFenceResult{ is_fence: true, fence_data: data };
}

pub fn isClosingCodeFence(p: *const Md4cParserState, off: u32, fence_data: u32): bool {
    const fence_char: u8 = @intCast(fence_data & 0xFF);
    const fence_count = fence_data >> 8;

    let pos = off;
    let count: u32 = 0;
    while (pos < p.size && p.text[pos] == fence_char) {
        count += 1;
        pos += 1;
    }

    if (count < fence_count) return false;
    while (pos < p.size && isBlank(p.text[pos])) pos += 1;
    return pos >= p.size || isNewline(p.text[pos]);
}

pub fn matchHtmlTag(p: *const Md4cParserState, off: u32, tag: []const u8): bool {
    if ((off as usize) + 1 + tag.len >= (p.size as usize)) return false;
    const start = off + 1;
    let pos = start;
    if (pos < p.size && p.text[pos] == '/') pos += 1;
    if ((pos as usize) + tag.len > (p.size as usize)) return false;
    if (!asciiCaseEql(p.text[pos .. (pos as usize) + tag.len], tag)) return false;
    pos += @as(u32, @intCast(tag.len));
    if (pos >= p.size) return true;
    const after = p.text[pos];
    return after == '>' || after == '/' || isBlank(after) || isNewline(after);
}

const block_html_tags: [62][]const u8 = [
    "address", "article",  "aside",   "base",     "basefont", "blockquote", "body",
    "caption", "center",   "col",     "colgroup", "dd",       "details",    "dialog",
    "dir",     "div",      "dl",      "dt",       "fieldset", "figcaption", "figure",
    "footer",  "form",     "frame",   "frameset", "h1",       "h2",         "h3",
    "h4",      "h5",       "h6",      "head",     "header",   "hr",         "html",
    "iframe",  "legend",   "li",      "link",     "main",     "menu",       "menuitem",
    "nav",     "noframes", "ol",      "optgroup", "option",   "p",          "param",
    "search",  "section",  "summary", "table",    "tbody",    "td",         "tfoot",
    "th",      "thead",    "title",   "tr",       "track",    "ul",
];

pub fn isBlockLevelHtmlTag(p: *const Md4cParserState, off: u32): bool {
    for (tag of block_html_tags) {
        if (matchHtmlTag(p, off, tag)) return true;
    }
    return false;
}

pub fn isCompleteHtmlTag(p: *const Md4cParserState, off: u32): bool {
    if (off + 1 >= p.size) return false;
    let pos = off + 1;

    if (pos < p.size && p.text[pos] == '/') {
        pos += 1;
        if (pos >= p.size || !isAlpha(p.text[pos])) return false;
        while (pos < p.size && (isAlphaNum(p.text[pos]) || p.text[pos] == '-')) pos += 1;
        while (pos < p.size && isBlank(p.text[pos])) pos += 1;
        if (pos >= p.size || p.text[pos] != '>') return false;
        pos += 1;
        while (pos < p.size && isBlank(p.text[pos])) pos += 1;
        return pos >= p.size || isNewline(p.text[pos]);
    }

    if (!isAlpha(p.text[pos])) return false;
    while (pos < p.size && (isAlphaNum(p.text[pos]) || p.text[pos] == '-')) pos += 1;

    while (true) {
        const ws_start = pos;
        while (pos < p.size && isBlank(p.text[pos])) pos += 1;
        if (pos >= p.size || isNewline(p.text[pos])) return false;

        if (p.text[pos] == '>') {
            pos += 1;
            break;
        }
        if (p.text[pos] == '/' && pos + 1 < p.size && p.text[pos + 1] == '>') {
            pos += 2;
            break;
        }

        if (pos == ws_start) return false;

        if (!isAlpha(p.text[pos]) && p.text[pos] != '_' && p.text[pos] != ':') return false;
        pos += 1;
        while (pos < p.size && (isAlphaNum(p.text[pos]) ||
            p.text[pos] == '_' || p.text[pos] == '.' ||
            p.text[pos] == ':' || p.text[pos] == '-'))
        {
            pos += 1;
        }

        let ws_pos = pos;
        while (ws_pos < p.size && isBlank(p.text[ws_pos])) ws_pos += 1;
        if (ws_pos < p.size && p.text[ws_pos] == '=') {
            pos = ws_pos + 1;
            while (pos < p.size && isBlank(p.text[pos])) pos += 1;
            if (pos >= p.size || isNewline(p.text[pos])) return false;

            if (p.text[pos] == '"') {
                pos += 1;
                while (pos < p.size && p.text[pos] != '"' && !isNewline(p.text[pos])) pos += 1;
                if (pos >= p.size || p.text[pos] != '"') return false;
                pos += 1;
            } else if (p.text[pos] == '\'') {
                pos += 1;
                while (pos < p.size && p.text[pos] != '\'' && !isNewline(p.text[pos])) pos += 1;
                if (pos >= p.size || p.text[pos] != '\'') return false;
                pos += 1;
            } else {
                while (pos < p.size && !isBlank(p.text[pos]) &&
                    !isNewline(p.text[pos]) &&
                    p.text[pos] != '"' && p.text[pos] != '\'' &&
                    p.text[pos] != '=' && p.text[pos] != '<' &&
                    p.text[pos] != '>' && p.text[pos] != '`')
                {
                    pos += 1;
                }
            }
        }
    }

    while (pos < p.size && isBlank(p.text[pos])) pos += 1;
    return pos >= p.size || isNewline(p.text[pos]);
}

pub fn isHtmlBlockStartCondition(p: *const Md4cParserState, off: u32): u8 {
    if (off + 1 >= p.size) return 0;

    if (p.text[off + 1] != '/' &&
        (matchHtmlTag(p, off, "script") || matchHtmlTag(p, off, "pre") ||
            matchHtmlTag(p, off, "style") || matchHtmlTag(p, off, "textarea")))
    {
        return 1;
    }

    if (off + 3 < p.size && p.text[off + 1] == '!' && p.text[off + 2] == '-' && p.text[off + 3] == '-') {
        return 2;
    }

    if (p.text[off + 1] == '?') return 3;

    if (p.text[off + 1] == '!' && off + 2 < p.size && p.text[off + 2] >= 'A' && p.text[off + 2] <= 'Z') {
        return 4;
    }

    if (off + 9 <= p.size && std.mem.eql(u8, p.text[off + 1 .. off + 9], "![CDATA[")) {
        return 5;
    }

    if (isBlockLevelHtmlTag(p, off)) return 6;
    if (isCompleteHtmlTag(p, off)) return 7;
    return 0;
}

pub fn isHtmlBlockEndCondition(p: *const Md4cParserState, off: u32, block_type: u8): bool {
    if (block_type >= 6) {
        return off >= p.size || isNewline(p.text[off]);
    }

    let pos = off;
    while (pos < p.size && !isNewline(p.text[pos])) {
        switch (block_type) {
            1 => {
                if (p.text[pos] == '<' && pos + 1 < p.size && p.text[pos + 1] == '/') {
                    if (matchHtmlTag(p, pos, "script") || matchHtmlTag(p, pos, "pre") ||
                        matchHtmlTag(p, pos, "style") || matchHtmlTag(p, pos, "textarea"))
                    {
                        return true;
                    }
                }
            },
            2 => {
                if (p.text[pos] == '-' && pos + 2 < p.size && p.text[pos + 1] == '-' && p.text[pos + 2] == '>') {
                    return true;
                }
            },
            3 => {
                if (p.text[pos] == '?' && pos + 1 < p.size && p.text[pos + 1] == '>') {
                    return true;
                }
            },
            4 => {
                if (p.text[pos] == '>') return true;
            },
            5 => {
                if (p.text[pos] == ']' && pos + 2 < p.size && p.text[pos + 1] == ']' && p.text[pos + 2] == '>') {
                    return true;
                }
            },
            else => return false,
        }
        pos += 1;
    }
    return false;
}

pub fn isTableUnderline(p: *Md4cParserState, off: u32): TableUnderlineResult {
    let pos = off;
    let col_count: u32 = 0;
    let had_pipe = false;

    if (pos < p.size && p.text[pos] == '|') {
        had_pipe = true;
        pos += 1;
        while (pos < p.size && isBlank(p.text[pos])) pos += 1;
    }

    while (pos < p.size && !isNewline(p.text[pos])) {
        const has_left_colon = pos < p.size && p.text[pos] == ':';
        if (has_left_colon) pos += 1;

        let dash_count: u32 = 0;
        while (pos < p.size && p.text[pos] == '-') {
            dash_count += 1;
            pos += 1;
        }

        if (dash_count == 0) return TableUnderlineResult{};

        const has_right_colon = pos < p.size && p.text[pos] == ':';
        if (has_right_colon) pos += 1;

        if (col_count < TABLE_MAXCOLCOUNT) {
            p.table_alignments[col_count] = if (has_left_colon && has_right_colon)
                Align.center
            else if (has_left_colon)
                Align.left
            else if (has_right_colon)
                Align.right
            else
                Align.default;
        }

        col_count += 1;
        while (pos < p.size && isBlank(p.text[pos])) pos += 1;

        if (pos < p.size && p.text[pos] == '|') {
            had_pipe = true;
            pos += 1;
            while (pos < p.size && isBlank(p.text[pos])) pos += 1;
            if (pos >= p.size || isNewline(p.text[pos])) break;
        } else if (pos >= p.size || isNewline(p.text[pos])) {
            break;
        } else {
            return TableUnderlineResult{};
        }
    }

    if (col_count == 0 || (!had_pipe && col_count < 2)) {
        return TableUnderlineResult{};
    }

    p.table_col_count = col_count;
    return TableUnderlineResult{ is_underline: true, col_count: col_count };
}

pub fn countTableRowColumns(p: *const Md4cParserState, beg: u32, end: u32): u32 {
    const row = p.text[beg..end];
    let col_count: u32 = 0;
    let pos: usize = 0;

    while (pos < row.len && isBlank(row[pos])) pos += 1;
    if (pos < row.len && row[pos] == '|') pos += 1;

    let in_cell = false;
    while (pos < row.len) {
        if (row[pos] == '|') {
            col_count += 1;
            in_cell = false;
            pos += 1;
        } else if (row[pos] == '\\' && pos + 1 < row.len) {
            in_cell = true;
            pos += 2;
        } else if (isNewline(row[pos])) {
            break;
        } else {
            in_cell = true;
            pos += 1;
        }
    }
    if (in_cell) col_count += 1;
    return col_count;
}

pub fn isContainerMark(p: *const Md4cParserState, indent: u32, off: u32): ContainerMarkResult {
    if (off >= p.size) return ContainerMarkResult{ off: off };
    if (indent >= p.code_indent_offset) return ContainerMarkResult{ off: off };

    const c = p.text[off];

    if (c == '>') {
        return ContainerMarkResult{
            is_container: true,
            container: Container{
                ch: '>',
                mark_indent: indent,
                contents_indent: indent + 1,
            },
            off: off + 1,
        };
    }

    if ((c == '-' || c == '+' || c == '*') && off + 1 < p.size && isBlank(p.text[off + 1])) {
        return ContainerMarkResult{
            is_container: true,
            container: Container{
                ch: c,
                mark_indent: indent,
                contents_indent: indent + 1,
            },
            off: off + 1,
        };
    }

    if ((c == '-' || c == '+' || c == '*') && (off + 1 >= p.size || isNewline(p.text[off + 1]))) {
        return ContainerMarkResult{
            is_container: true,
            container: Container{
                ch: c,
                mark_indent: indent,
                contents_indent: indent + 1,
            },
            off: off + 1,
        };
    }

    if (isDigit(c)) {
        let pos = off;
        let num: u32 = 0;
        while (pos < p.size && isDigit(p.text[pos]) && (pos - off) < 9) {
            num = num * 10 + ((p.text[pos] - '0') as u32);
            pos += 1;
        }
        if (pos < p.size && (p.text[pos] == '.' || p.text[pos] == ')')) {
            const delim = p.text[pos];
            pos += 1;
            if (pos < p.size && isBlank(p.text[pos])) {
                const mark_width = pos - off;
                return ContainerMarkResult{
                    is_container: true,
                    container: Container{
                        ch: delim,
                        start: num,
                        mark_indent: indent,
                        contents_indent: indent + mark_width,
                    },
                    off: pos,
                };
            }
            if (pos >= p.size || isNewline(p.text[pos])) {
                const mark_width = pos - off;
                return ContainerMarkResult{
                    is_container: true,
                    container: Container{
                        ch: delim,
                        start: num,
                        mark_indent: indent,
                        contents_indent: indent + mark_width,
                    },
                    off: pos,
                };
            }
        }
    }

    return ContainerMarkResult{ off: off };
}
