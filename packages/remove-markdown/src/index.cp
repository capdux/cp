// @cap/markdown-strip — Zero-allocation Markdown to plain-text & excerpt engine for CP
// Clean-room MIT implementation inspired by npm `remove-markdown` and `strip-markdown`.
//
// Features:
// - Strips ATX headings (#..######), Setext underlines (=== / ---), and horizontal rules (--- / *** / ___).
// - Strips inline emphasis (**bold**, __bold__, *italic*, _italic_, ~~strike~~, `code`) while preserving
//   identifier underscores (`snake_case`) and spaced operators (`a < b`, `2 * 3`).
// - Strips links [text](url), reference links [text][ref], reference defs [ref]: url, and images ![alt](url).
// - Strips fenced code block delimiters (``` / ~~~) with optional `keep_code_block_text`.
// - Strips blockquotes (> ), unordered/ordered lists (- / * / + / 1. / 2)), task lists (- [x] ), and HTML tags.
// - Provides `stripMarkdown`, `stripMarkdownWith`, `stripToSingleLine`, and UTF-8 safe `excerpt`.
// - Zero heap allocation: writes directly into caller-provided `out: []u8`.

import { isSpace, isAlpha, isAlnum, isDigit, trim, findByte } from "cap:ascii";

pub struct StripOptions {
    pub keep_img_alt: bool,
    pub keep_code_block_text: bool,
    pub strip_html: bool,
    pub collapse_whitespace: bool,

    pub static fn defaults() -> StripOptions {
        return StripOptions{
            keep_img_alt: true,
            keep_code_block_text: true,
            strip_html: true,
            collapse_whitespace: false,
        };
    }
}

fn isAsciiPunct(c: u8) -> bool {
    return (c >= 33 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) || (c >= 123 && c <= 126);
}

fn pushByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pushTextByte(out: []u8, pos: usize, b: u8, collapse_ws: bool) -> usize {
    if (collapse_ws && isSpace(b)) {
        if (pos == 0 || out[pos - 1] == ' ') {
            return pos;
        }
        return pushByte(out, pos, ' ');
    }
    return pushByte(out, pos, b);
}

fn isRuleLine(trimmed: string) -> bool {
    if (trimmed.len < 3) {
        return false;
    }
    const ch = trimmed[0];
    if (ch != '-' && ch != '*' && ch != '_' && ch != '=') {
        return false;
    }
    let count: usize = 0;
    let i: usize = 0;
    while (i < trimmed.len) {
        const c = trimmed[i];
        if (c == ch) {
            count += 1;
        } else if (c == ' ' || c == '\t') {
            if (ch == '=') {
                return false;
            }
        } else {
            return false;
        }
        i += 1;
    }
    return count >= 3;
}

/// Checks if a trimmed line is a reference link definition: `[label]: url ...`
fn isRefLinkDef(trimmed: string) -> bool {
    if (trimmed.len < 5 || trimmed[0] != '[') {
        return false;
    }
    let i: usize = 1;
    while (i < trimmed.len && trimmed[i] != ']') {
        if (trimmed[i] == '[') {
            return false;
        }
        i += 1;
    }
    if (i <= 1 || i + 1 >= trimmed.len) {
        return false;
    }
    return trimmed[i] == ']' && trimmed[i + 1] == ':';
}

struct FenceInfo {
    is_fence: bool,
    ch: u8,
    len: usize,
}

fn detectFence(line: string) -> FenceInfo {
    let i: usize = 0;
    while (i < line.len && i < 3 && line[i] == ' ') {
        i += 1;
    }
    if (i >= line.len) {
        return FenceInfo{ is_fence: false, ch: 0, len: 0 };
    }
    const ch = line[i];
    if (ch != '`' && ch != '~') {
        return FenceInfo{ is_fence: false, ch: 0, len: 0 };
    }
    let count: usize = 0;
    while (i < line.len && line[i] == ch) {
        count += 1;
        i += 1;
    }
    if (count >= 3) {
        return FenceInfo{ is_fence: true, ch: ch, len: count };
    }
    return FenceInfo{ is_fence: false, ch: 0, len: 0 };
}

fn isClosingFence(line: string, fence_char: u8, min_len: usize) -> bool {
    let i: usize = 0;
    while (i < line.len && i < 3 && line[i] == ' ') {
        i += 1;
    }
    let count: usize = 0;
    while (i < line.len && line[i] == fence_char) {
        count += 1;
        i += 1;
    }
    if (count < min_len) {
        return false;
    }
    while (i < line.len) {
        if (line[i] != ' ' && line[i] != '\t' && line[i] != '\r') {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Strips block-level prefixes: blockquotes (`>`), ATX headings (`#..######`),
/// unordered lists (`- ` / `* ` / `+ `), ordered lists (`1. ` / `2) `), and task checkboxes (`[x] ` / `[ ] `).
fn stripBlockPrefixes(raw_line: string) -> string {
    let s = trim(raw_line);

    // Strip nested blockquote prefixes `>`
    while (s.len > 0 && s[0] == '>') {
        s = s[1..s.len];
        if (s.len > 0 && s[0] == ' ') {
            s = s[1..s.len];
        }
        s = trim(s);
    }

    // Strip ATX heading prefix `#`..`######` followed by space or end
    if (s.len > 0 && s[0] == '#') {
        let h: usize = 0;
        while (h < s.len && h < 6 && s[h] == '#') {
            h += 1;
        }
        if (h == s.len || s[h] == ' ' || s[h] == '\t') {
            s = trim(s[h..s.len]);
            // Strip optional closing ` ###`
            let end = s.len;
            while (end > 0 && s[end - 1] == '#') {
                end -= 1;
            }
            if (end < s.len && (end == 0 || s[end - 1] == ' ' || s[end - 1] == '\t')) {
                s = trim(s[0..end]);
            }
            return s;
        }
    }

    // Strip unordered list marker: `- `, `* `, `+ `
    if (s.len >= 2 && (s[0] == '-' || s[0] == '*' || s[0] == '+') && (s[1] == ' ' || s[1] == '\t')) {
        s = trim(s[2..s.len]);
        // Strip task list checkbox: `[x] `, `[X] `, `[ ] `
        if (s.len >= 3 && s[0] == '[' && (s[1] == ' ' || s[1] == 'x' || s[1] == 'X') && s[2] == ']') {
            if (s.len == 3 || s[3] == ' ' || s[3] == '\t') {
                s = trim(s[3..s.len]);
            }
        }
        return s;
    }

    // Strip ordered list marker: `1. `, `2) `
    if (s.len >= 3 && isDigit(s[0])) {
        let d: usize = 0;
        while (d < s.len && isDigit(s[d])) {
            d += 1;
        }
        if (d + 1 < s.len && (s[d] == '.' || s[d] == ')') && (s[d + 1] == ' ' || s[d + 1] == '\t')) {
            s = trim(s[(d + 2)..s.len]);
            if (s.len >= 3 && s[0] == '[' && (s[1] == ' ' || s[1] == 'x' || s[1] == 'X') && s[2] == ']') {
                if (s.len == 3 || s[3] == ' ' || s[3] == '\t') {
                    s = trim(s[3..s.len]);
                }
            }
            return s;
        }
    }

    return s;
}

fn findMatchingBracket(s: string, open_idx: usize) -> usize {
    let depth: usize = 0;
    let i = open_idx;
    while (i < s.len) {
        if (s[i] == '\\' && i + 1 < s.len) {
            i += 2;
            continue;
        }
        if (s[i] == '[') {
            depth += 1;
        } else if (s[i] == ']') {
            if (depth <= 1) {
                return i;
            }
            depth -= 1;
        }
        i += 1;
    }
    return s.len;
}

fn findMatchingParen(s: string, open_idx: usize) -> usize {
    let depth: usize = 0;
    let i = open_idx;
    while (i < s.len) {
        if (s[i] == '\\' && i + 1 < s.len) {
            i += 2;
            continue;
        }
        if (s[i] == '(') {
            depth += 1;
        } else if (s[i] == ')') {
            if (depth <= 1) {
                return i;
            }
            depth -= 1;
        }
        i += 1;
    }
    return s.len;
}

fn stripInline(s: string, opts: StripOptions, out: []u8, start_pos: usize, depth: u8) -> usize {
    let pos = start_pos;
    let i: usize = 0;

    while (i < s.len) {
        const c = s[i];

        // Backslash escape for ASCII punctuation
        if (c == '\\' && i + 1 < s.len && isAsciiPunct(s[i + 1])) {
            pos = pushTextByte(out, pos, s[i + 1], opts.collapse_whitespace);
            i += 2;
            continue;
        }

        // Inline code span: `...` or ``...``
        if (c == '`') {
            let ticks: usize = 0;
            while (i + ticks < s.len && s[i + ticks] == '`') {
                ticks += 1;
            }
            let j = i + ticks;
            let found = false;
            while (j + ticks <= s.len) {
                if (s[j] == '`') {
                    let m: usize = 0;
                    while (m < ticks && s[j + m] == '`') {
                        m += 1;
                    }
                    if (m == ticks && (j + ticks == s.len || s[j + ticks] != '`')) {
                        found = true;
                        break;
                    }
                }
                j += 1;
            }
            if (found) {
                let k = i + ticks;
                while (k < j) {
                    pos = pushTextByte(out, pos, s[k], opts.collapse_whitespace);
                    k += 1;
                }
                i = j + ticks;
                continue;
            }
        }

        // Image: ![alt](url) or ![alt][ref]
        if (c == '!' && i + 1 < s.len && s[i + 1] == '[' && depth < 6) {
            const close_b = findMatchingBracket(s, i + 1);
            if (close_b < s.len && close_b + 1 < s.len) {
                const next_ch = s[close_b + 1];
                if (next_ch == '(') {
                    const close_p = findMatchingParen(s, close_b + 1);
                    if (close_p < s.len) {
                        if (opts.keep_img_alt) {
                            pos = stripInline(s[(i + 2)..close_b], opts, out, pos, depth + 1);
                        }
                        i = close_p + 1;
                        continue;
                    }
                } else if (next_ch == '[') {
                    const close_ref = findMatchingBracket(s, close_b + 1);
                    if (close_ref < s.len) {
                        if (opts.keep_img_alt) {
                            pos = stripInline(s[(i + 2)..close_b], opts, out, pos, depth + 1);
                        }
                        i = close_ref + 1;
                        continue;
                    }
                }
            }
        }

        // Link: [text](url "title") or [text][ref]
        if (c == '[' && depth < 6) {
            const close_b = findMatchingBracket(s, i);
            if (close_b < s.len && close_b + 1 < s.len) {
                const next_ch = s[close_b + 1];
                if (next_ch == '(') {
                    const close_p = findMatchingParen(s, close_b + 1);
                    if (close_p < s.len) {
                        pos = stripInline(s[(i + 1)..close_b], opts, out, pos, depth + 1);
                        i = close_p + 1;
                        continue;
                    }
                } else if (next_ch == '[') {
                    const close_ref = findMatchingBracket(s, close_b + 1);
                    if (close_ref < s.len) {
                        pos = stripInline(s[(i + 1)..close_b], opts, out, pos, depth + 1);
                        i = close_ref + 1;
                        continue;
                    }
                }
            }
        }

        // Inline HTML tags: <tag ...>, </tag>, <!-- ... -->
        if (opts.strip_html && c == '<' && i + 1 < s.len) {
            const n = s[i + 1];
            const is_tag_start = isAlpha(n) || n == '/' || n == '!';
            if (is_tag_start) {
                const gt = findByte(s, '>', i + 2) ?? s.len;
                if (gt < s.len) {
                    i = gt + 1;
                    continue;
                }
            }
        }

        // Double emphasis / strikethrough markers: **, __, ~~
        if ((c == '*' || c == '_' || c == '~') && i + 1 < s.len && s[i + 1] == c) {
            i += 2;
            continue;
        }

        // Single '*' emphasis (preserve spaced math operator `2 * 3`)
        if (c == '*') {
            const prev_sp = (i > 0 && s[i - 1] == ' ');
            const next_sp = (i + 1 < s.len && s[i + 1] == ' ');
            if (!(prev_sp && next_sp)) {
                i += 1;
                continue;
            }
        }

        // Single '_' emphasis only at word boundary (preserve `snake_case`)
        if (c == '_') {
            const prev_alnum = (i > 0 && isAlnum(s[i - 1]));
            const next_alnum = (i + 1 < s.len && isAlnum(s[i + 1]));
            if (!(prev_alnum && next_alnum)) {
                i += 1;
                continue;
            }
        }

        pos = pushTextByte(out, pos, c, opts.collapse_whitespace);
        i += 1;
    }

    return pos;
}

/// Strips Markdown formatting using `StripOptions.defaults()` into caller-provided `out`.
pub fn stripMarkdown(md: string, out: []u8) -> string {
    return stripMarkdownWith(md, StripOptions.defaults(), out);
}

/// Strips Markdown formatting with custom options into caller-provided `out` (zero heap allocation).
pub fn stripMarkdownWith(md: string, opts: StripOptions, out: []u8) -> string {
    let pos: usize = 0;
    let in_fence = false;
    let fence_char: u8 = 0;
    let fence_len: usize = 0;
    let has_emitted_line = false;
    let pending_blank = false;

    let cursor: usize = 0;
    while (cursor <= md.len) {
        if (cursor == md.len) {
            break;
        }
        const line_start = cursor;
        cursor = findByte(md, '\n', cursor) ?? md.len;
        let line_end = cursor;
        if (line_end > line_start && md[line_end - 1] == '\r') {
            line_end -= 1;
        }
        if (cursor < md.len && md[cursor] == '\n') {
            cursor += 1;
        }

        const line = md[line_start..line_end];

        // Handle fenced code blocks
        if (in_fence) {
            if (isClosingFence(line, fence_char, fence_len)) {
                in_fence = false;
                continue;
            }
            if (!opts.keep_code_block_text) {
                continue;
            }
            const trimmed_code = trim(line);
            if (trimmed_code.len == 0) {
                if (has_emitted_line) {
                    pending_blank = true;
                }
                continue;
            }
            if (has_emitted_line) {
                if (opts.collapse_whitespace) {
                    pos = pushTextByte(out, pos, ' ', true);
                } else {
                    pos = pushByte(out, pos, '\n');
                    if (pending_blank) {
                        pos = pushByte(out, pos, '\n');
                    }
                }
                pending_blank = false;
            }
            let k: usize = 0;
            while (k < line.len) {
                pos = pushTextByte(out, pos, line[k], opts.collapse_whitespace);
                k += 1;
            }
            has_emitted_line = true;
            continue;
        }

        const fence = detectFence(line);
        if (fence.is_fence) {
            in_fence = true;
            fence_char = fence.ch;
            fence_len = fence.len;
            continue;
        }

        const trimmed = trim(line);
        if (trimmed.len == 0) {
            if (has_emitted_line) {
                pending_blank = true;
            }
            continue;
        }

        // Skip horizontal rules, Setext underlines, and reference link definitions
        if (isRuleLine(trimmed) || isRefLinkDef(trimmed)) {
            continue;
        }

        const content = stripBlockPrefixes(line);
        if (content.len == 0) {
            continue;
        }

        const before_sep = pos;
        if (has_emitted_line) {
            if (opts.collapse_whitespace) {
                pos = pushTextByte(out, pos, ' ', true);
            } else {
                pos = pushByte(out, pos, '\n');
                if (pending_blank) {
                    pos = pushByte(out, pos, '\n');
                }
            }
        }

        const after_sep = pos;
        pos = stripInline(content, opts, out, pos, 0);

        if (pos == after_sep) {
            // Line produced no visible text (e.g. standalone HTML tag or stripped image)
            pos = before_sep;
        } else {
            has_emitted_line = true;
            pending_blank = false;
        }
    }

    // Trim trailing spaces if collapse_whitespace is active
    if (opts.collapse_whitespace) {
        while (pos > 0 && out[pos - 1] == ' ') {
            pos -= 1;
        }
    }

    return out[0..pos];
}

/// Strips Markdown into a compact single-line plain-text string (ideal for search indexing).
pub fn stripToSingleLine(md: string, out: []u8) -> string {
    let opts = StripOptions.defaults();
    opts.collapse_whitespace = true;
    return stripMarkdownWith(md, opts, out);
}

/// Produces a clean single-line plain-text excerpt truncated cleanly to at most `max_bytes` bytes
/// (UTF-8 boundary safe, appends `...` when truncated).
pub fn excerpt(md: string, max_bytes: usize, out: []u8) -> string {
    const full = stripToSingleLine(md, out);
    if (full.len <= max_bytes) {
        return full;
    }
    if (max_bytes <= 3) {
        let k: usize = 0;
        while (k < max_bytes && k < out.len) {
            out[k] = '.';
            k += 1;
        }
        return out[0..k];
    }

    let cut = max_bytes - 3;
    // Back up to avoid splitting a multi-byte UTF-8 sequence (continuation bytes have 10xxxxxx)
    while (cut > 0 && (out[cut] & 0xC0) == 0x80) {
        cut -= 1;
    }
    // Prefer breaking at a word boundary if one exists in the second half
    let space_cut = cut;
    while (space_cut > cut / 2 && out[space_cut] != ' ') {
        space_cut -= 1;
    }
    if (space_cut > cut / 2 && out[space_cut] == ' ') {
        cut = space_cut;
    }
    while (cut > 0 && out[cut - 1] == ' ') {
        cut -= 1;
    }

    if (cut + 3 <= out.len) {
        out[cut] = '.';
        out[cut + 1] = '.';
        out[cut + 2] = '.';
        return out[0..(cut + 3)];
    }
    return out[0..cut];
}
