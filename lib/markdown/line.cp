// Source: Absorbed from /data/dev/bun/src/md/line_analysis.cp | Synced: 2026-11-21T17:04:34Z
// Reference-hardened against pulldown-cmark/pulldown-cmark (MIT)
// Pure-CP CommonMark 0.31.2 / GFM block-level line analyzer with cap:simd line-end acceleration:
// Setext & ATX headings, thematic breaks (HR), fenced code blocks, HTML block types 1-7, and GFM table underlines.

import { Simd } from "cap:simd";

/// SIMD-accelerated line-end finder returning byte offset of '\n' or '\r', or `text.len` if none.
fn mdFindLineEnd(text: []const u8, off: usize): usize {
    if (off >= text.len) return text.len;
    const rem = text[off..];
    const hit_opt = Simd.indexOfAny2(rem, '\n', '\r');
    if (h := hit_opt) {
        return off + h;
    }
    return text.len;
}

pub struct SetextResult {
    is_setext: bool,
    /// Heading level: 1 for `===`, 2 for `---`, 0 if not setext.
    level: u8,
}

pub struct AtxHeaderResult {
    is_atx: bool,
    /// Heading level 1..6.
    level: u8,
    /// Byte offset where heading content begins (after `#` run and spaces).
    content_beg: usize,
    /// Byte offset where heading content ends (excluding optional closing `#` run).
    content_end: usize,
}

pub struct CodeFenceResult {
    is_fence: bool,
    /// Fence character: '`' or '~'.
    fence_char: u8,
    /// Number of fence characters (>= 3).
    fence_count: u32,
    /// Info string slice (trimmed) for opening fence.
    info: []const u8,
}

pub enum TableAlign: u8 {
    none = 0,
    left = 1,
    center = 2,
    right = 3,
}

pub struct TableUnderlineResult {
    is_underline: bool,
    col_count: usize,
    alignments: [32]TableAlign,
}

/// Check if line starting at `off` is a CommonMark Setext heading underline (`===` or `---`).
pub fn isSetextUnderline(text: []const u8, off: usize): SetextResult {
    if (off >= text.len) return SetextResult{ is_setext: false, level: 0 };
    const c = text[off];
    if (c != '=' && c != '-') return SetextResult{ is_setext: false, level: 0 };

    let pos = off;
    while (pos < text.len && text[pos] == c) : (pos += 1) {}
    while (pos < text.len && isBlank(text[pos])) : (pos += 1) {}

    if (pos >= text.len || isNewline(text[pos])) {
        const level: u8 = if (c == '=') 1 else 2;
        return SetextResult{ is_setext: true, level: level };
    }
    return SetextResult{ is_setext: false, level: 0 };
}

/// Check if line starting at `off` is a CommonMark thematic break (`---`, `***`, `___`).
pub fn isHrLine(text: []const u8, off: usize): bool {
    if (off >= text.len) return false;
    const c = text[off];
    if (c != '-' && c != '_' && c != '*') return false;

    let pos = off;
    let count: u32 = 0;
    while (pos < text.len && !isNewline(text[pos])) : (pos += 1) {
        if (text[pos] == c) {
            count += 1;
        } else if (!isBlank(text[pos])) {
            return false;
        }
    }
    return count >= 3;
}

/// Check if line starting at `off` is a CommonMark ATX heading (`#`..`######`).
/// Strips optional trailing `#` sequence preceded by whitespace.
pub fn isAtxHeaderLine(text: []const u8, off: usize, permissive: bool): AtxHeaderResult {
    let pos = off;
    let level: u8 = 0;
    while (pos < text.len && text[pos] == '#') : (pos += 1) {
        if (level < 7) level += 1;
    }
    if (level == 0 || level > 6) {
        return AtxHeaderResult{ is_atx: false, level: 0, content_beg: 0, content_end: 0 };
    }

    if (pos < text.len && !isBlank(text[pos]) && !isNewline(text[pos])) {
        if (!permissive) {
            return AtxHeaderResult{ is_atx: false, level: 0, content_beg: 0, content_end: 0 };
        }
    }

    while (pos < text.len && isBlank(text[pos])) : (pos += 1) {}
    const content_beg = pos;

    const line_end = mdFindLineEnd(text, pos);

    // Trim trailing spaces/tabs
    let end = line_end;
    while (end > content_beg && isBlank(text[end - 1])) : (end -= 1) {}

    // Optional closing '#' sequence preceded by space/tab or start of content
    let hash_scan = end;
    while (hash_scan > content_beg && text[hash_scan - 1] == '#') : (hash_scan -= 1) {}
    if (hash_scan < end) {
        if (hash_scan == content_beg || isBlank(text[hash_scan - 1])) {
            end = hash_scan;
            while (end > content_beg && isBlank(text[end - 1])) : (end -= 1) {}
        }
    }

    return AtxHeaderResult{
        is_atx: true,
        level: level,
        content_beg: content_beg,
        content_end: end,
    };
}

/// Check if line starting at `off` opens a fenced code block (``` or ~~~).
pub fn isOpeningCodeFence(text: []const u8, off: usize): CodeFenceResult {
    if (off >= text.len) {
        return CodeFenceResult{ is_fence: false, fence_char: 0, fence_count: 0, info: "" };
    }
    const fence_char = text[off];
    if (fence_char != '`' && fence_char != '~') {
        return CodeFenceResult{ is_fence: false, fence_char: 0, fence_count: 0, info: "" };
    }

    let pos = off;
    let count: u32 = 0;
    while (pos < text.len && text[pos] == fence_char) : (pos += 1) {
        count += 1;
    }
    if (count < 3) {
        return CodeFenceResult{ is_fence: false, fence_char: 0, fence_count: 0, info: "" };
    }

    const info_raw_start = pos;
    let line_end = pos;
    while (line_end < text.len && !isNewline(text[line_end])) : (line_end += 1) {
        if (fence_char == '`' && text[line_end] == '`') {
            return CodeFenceResult{ is_fence: false, fence_char: 0, fence_count: 0, info: "" };
        }
    }

    let is_start = info_raw_start;
    while (is_start < line_end && isBlank(text[is_start])) : (is_start += 1) {}
    let is_end = line_end;
    while (is_end > is_start && isBlank(text[is_end - 1])) : (is_end -= 1) {}

    return CodeFenceResult{
        is_fence: true,
        fence_char: fence_char,
        fence_count: count,
        info: text[is_start..is_end],
    };
}

/// Check if line starting at `off` closes an open fenced code block.
pub fn isClosingCodeFence(text: []const u8, off: usize, fence_char: u8, min_count: u32): bool {
    let pos = off;
    let count: u32 = 0;
    while (pos < text.len && text[pos] == fence_char) : (pos += 1) {
        count += 1;
    }
    if (count < min_count) return false;
    while (pos < text.len && isBlank(text[pos])) : (pos += 1) {}
    return pos >= text.len || isNewline(text[pos]);
}

/// Detect CommonMark §4.6 HTML block start condition (returns 1..7, or 0 if none).
pub fn isHtmlBlockStartCondition(text: []const u8, off: usize): u8 {
    if (off + 1 >= text.len || text[off] != '<') return 0;

    // Type 1: <script, <pre, <style, <textarea (opening tags only)
    if (text[off + 1] != '/' &&
        (matchHtmlTag(text, off, "script") || matchHtmlTag(text, off, "pre") ||
            matchHtmlTag(text, off, "style") || matchHtmlTag(text, off, "textarea")))
    {
        return 1;
    }

    // Type 2: <!--
    if (off + 3 < text.len && text[off + 1] == '!' && text[off + 2] == '-' && text[off + 3] == '-') {
        return 2;
    }

    // Type 3: <?
    if (text[off + 1] == '?') return 3;

    // Type 4: <! followed by ASCII letter
    if (text[off + 1] == '!' && off + 2 < text.len && isAlpha(text[off + 2])) {
        return 4;
    }

    // Type 5: <![CDATA[
    if (off + 9 <= text.len && mdLineStrEq(text[off + 1 .. off + 9], "![CDATA[")) {
        return 5;
    }

    // Type 6: CommonMark 62 block-level tags
    if (isBlockLevelHtmlTag(text, off)) return 6;

    // Type 7: Complete standalone open/close HTML tag on line
    if (isCompleteHtmlTag(text, off)) return 7;

    return 0;
}

/// Detect CommonMark §4.6 HTML block end condition on the line starting at `off`.
pub fn isHtmlBlockEndCondition(text: []const u8, off: usize, block_type: u8): bool {
    if (block_type >= 6) {
        let p = off;
        while (p < text.len && isBlank(text[p])) : (p += 1) {}
        return p >= text.len || isNewline(text[p]);
    }

    let pos = off;
    while (pos < text.len && !isNewline(text[pos])) : (pos += 1) {
        if (block_type == 1) {
            if (text[pos] == '<' && pos + 1 < text.len && text[pos + 1] == '/') {
                if (matchHtmlTag(text, pos, "script") || matchHtmlTag(text, pos, "pre") ||
                    matchHtmlTag(text, pos, "style") || matchHtmlTag(text, pos, "textarea"))
                {
                    return true;
                }
            }
        } else if (block_type == 2) {
            if (text[pos] == '-' && pos + 2 < text.len && text[pos + 1] == '-' && text[pos + 2] == '>') return true;
        } else if (block_type == 3) {
            if (text[pos] == '?' && pos + 1 < text.len && text[pos + 1] == '>') return true;
        } else if (block_type == 4) {
            if (text[pos] == '>') return true;
        } else if (block_type == 5) {
            if (text[pos] == ']' && pos + 2 < text.len && text[pos + 1] == ']' && text[pos + 2] == '>') return true;
        }
    }
    return false;
}

/// Match `<tag` or `</tag` followed by `>`, `/`, whitespace, or end-of-line.
pub fn matchHtmlTag(text: []const u8, off: usize, tag: []const u8): bool {
    if (off >= text.len || text[off] != '<') return false;
    let pos = off + 1;
    if (pos < text.len && text[pos] == '/') pos += 1;
    if (pos + tag.len > text.len) return false;
    if (!eqIgnoreCase(text[pos .. pos + tag.len], tag)) return false;
    pos += tag.len;
    if (pos >= text.len) return true;
    const after = text[pos];
    return after == '>' || after == '/' || isBlank(after) || isNewline(after);
}

/// Check if `<tag` at `off` is one of the 62 CommonMark block-level HTML tags.
pub fn isBlockLevelHtmlTag(text: []const u8, off: usize): bool {
    const block_tags: [62][]const u8 = [
        "address", "article", "aside", "base", "basefont", "blockquote", "body",
        "caption", "center", "col", "colgroup", "dd", "details", "dialog",
        "dir", "div", "dl", "dt", "fieldset", "figcaption", "figure",
        "footer", "form", "frame", "frameset", "h1", "h2", "h3",
        "h4", "h5", "h6", "head", "header", "hr", "html",
        "iframe", "legend", "li", "link", "main", "menu", "menuitem",
        "nav", "noframes", "ol", "optgroup", "option", "p", "param",
        "search", "section", "summary", "table", "tbody", "td", "tfoot",
        "th", "thead", "title", "tr", "track", "ul",
    ];
    let i: usize = 0;
    while (i < 62) : (i += 1) {
        if (matchHtmlTag(text, off, block_tags[i])) return true;
    }
    return false;
}

/// Check if line starting at `off` is a complete standalone HTML open or closing tag (Type 7).
pub fn isCompleteHtmlTag(text: []const u8, off: usize): bool {
    if (off + 1 >= text.len || text[off] != '<') return false;
    let pos = off + 1;

    // Closing tag: </tagname ws*>
    if (text[pos] == '/') {
        pos += 1;
        if (pos >= text.len || !isAlpha(text[pos])) return false;
        while (pos < text.len && (isAlphaNum(text[pos]) || text[pos] == '-')) : (pos += 1) {}
        while (pos < text.len && isBlank(text[pos])) : (pos += 1) {}
        if (pos >= text.len || text[pos] != '>') return false;
        pos += 1;
        while (pos < text.len && isBlank(text[pos])) : (pos += 1) {}
        return pos >= text.len || isNewline(text[pos]);
    }

    // Opening tag: <tagname (attrs)* /?>
    if (!isAlpha(text[pos])) return false;
    while (pos < text.len && (isAlphaNum(text[pos]) || text[pos] == '-')) : (pos += 1) {}

    while (pos < text.len && !isNewline(text[pos])) {
        if (text[pos] == '>') {
            pos += 1;
            while (pos < text.len && isBlank(text[pos])) : (pos += 1) {}
            return pos >= text.len || isNewline(text[pos]);
        }
        if (text[pos] == '/' && pos + 1 < text.len && text[pos + 1] == '>') {
            pos += 2;
            while (pos < text.len && isBlank(text[pos])) : (pos += 1) {}
            return pos >= text.len || isNewline(text[pos]);
        }
        if (!isBlank(text[pos])) return false;
        while (pos < text.len && isBlank(text[pos])) : (pos += 1) {}
        if (pos >= text.len) return false;
        if (text[pos] == '>' || text[pos] == '/') continue;

        // Attribute name
        if (!isAlpha(text[pos]) && text[pos] != '_' && text[pos] != ':') return false;
        while (pos < text.len && (isAlphaNum(text[pos]) || text[pos] == '_' || text[pos] == ':' || text[pos] == '.' || text[pos] == '-')) : (pos += 1) {}

        // Optional = value
        let save = pos;
        while (pos < text.len && isBlank(text[pos])) : (pos += 1) {}
        if (pos < text.len && text[pos] == '=') {
            pos += 1;
            while (pos < text.len && isBlank(text[pos])) : (pos += 1) {}
            if (pos >= text.len) return false;
            if (text[pos] == '"' || text[pos] == '\'') {
                const q = text[pos];
                pos += 1;
                while (pos < text.len && text[pos] != q && !isNewline(text[pos])) : (pos += 1) {}
                if (pos >= text.len || text[pos] != q) return false;
                pos += 1;
            } else {
                const vstart = pos;
                while (pos < text.len && !isBlank(text[pos]) && !isNewline(text[pos]) &&
                    text[pos] != '"' && text[pos] != '\'' && text[pos] != '=' &&
                    text[pos] != '<' && text[pos] != '>' && text[pos] != '`') : (pos += 1) {}
                if (pos == vstart) return false;
            }
        } else {
            pos = save;
        }
    }
    return false;
}

/// Check if line starting at `off` is a GFM table delimiter row (`| :--- | :---: | ---: |`).
pub fn isTableUnderline(text: []const u8, off: usize): TableUnderlineResult {
    let res = TableUnderlineResult{
        is_underline: false,
        col_count: 0,
        alignments: [TableAlign.none; 32],
    };

    const line_end = mdFindLineEnd(text, off);

    let p = off;
    while (p < line_end && isBlank(text[p])) : (p += 1) {}
    let end = line_end;
    while (end > p && isBlank(text[end - 1])) : (end -= 1) {}
    if (p >= end) return res;

    let has_pipe = false;
    if (text[p] == '|') {
        has_pipe = true;
        p += 1;
    }
    if (end > p && text[end - 1] == '|') {
        has_pipe = true;
        end -= 1;
    }

    while (p <= end) {
        let cell_start = p;
        while (p < end && text[p] != '|') : (p += 1) {}
        if (p < end && text[p] == '|') has_pipe = true;
        let cell_end = p;

        while (cell_start < cell_end && isBlank(text[cell_start])) : (cell_start += 1) {}
        while (cell_end > cell_start && isBlank(text[cell_end - 1])) : (cell_end -= 1) {}
        if (cell_start >= cell_end) return res;

        let left_colon = false;
        let right_colon = false;
        let scan = cell_start;
        if (text[scan] == ':') {
            left_colon = true;
            scan += 1;
        }
        let dash_count: usize = 0;
        while (scan < cell_end && text[scan] == '-') : (scan += 1) {
            dash_count += 1;
        }
        if (scan < cell_end && text[scan] == ':') {
            right_colon = true;
            scan += 1;
        }
        if (dash_count == 0 || scan != cell_end) return res;

        if (res.col_count < 32) {
            if (left_colon && right_colon) {
                res.alignments[res.col_count] = .center;
            } else if (left_colon) {
                res.alignments[res.col_count] = .left;
            } else if (right_colon) {
                res.alignments[res.col_count] = .right;
            } else {
                res.alignments[res.col_count] = .none;
            }
        }
        res.col_count += 1;

        if (p >= end) break;
        p += 1; // skip '|'
    }

    if (res.col_count == 0 || !has_pipe) {
        res.col_count = 0;
        return res;
    }
    res.is_underline = true;
    return res;
}

// ---------- Helpers ----------

fn isBlank(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isNewline(c: u8): bool {
    return c == '\n' || c == '\r';
}

fn isAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isAlphaNum(c: u8): bool {
    return isAlpha(c) || (c >= '0' && c <= '9');
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn mdLineStrEq(a: []const u8, b: []const u8): bool {
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
        if (toLower(a[i]) != toLower(b[i])) return false;
    }
    return true;
}
