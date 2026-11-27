// @cap/marked-md — Zero-heap CommonMark/GFM Markdown -> HTML compiler in pure CP
// Clean-room MIT implementation inspired by npm `marked` v14.
// Reuses:
//   - `escapeHtml` from `@cap/escape-html` for XSS-safe text & attribute escaping.

import { isDigit, isAlnum, toLowerByte } from "cap:ascii";
import { findByteInBytes } from "cap:bin";

type MkdVec16 = @Vector(16, u8);

fn mkdBuildPunctEscapeLut(): [256]u8 {
    let lut: [256]u8 = [0; 256];
    lut[92] = 1; lut[96] = 1; lut[42] = 1; lut[95] = 1; lut[123] = 1; lut[125] = 1;
    lut[91] = 1; lut[93] = 1; lut[40] = 1; lut[41] = 1; lut[35] = 1; lut[43] = 1;
    lut[45] = 1; lut[46] = 1; lut[33] = 1; lut[126] = 1; lut[62] = 1; lut[124] = 1;
    return lut;
}

fn mkdBuildInlineSpecialLut(): [256]u8 {
    let lut: [256]u8 = [0; 256];
    lut[92] = 1; lut[96] = 1; lut[42] = 1; lut[95] = 1; lut[126] = 1;
    lut[33] = 1; lut[91] = 1; lut[10] = 1; lut[32] = 1;
    return lut;
}

const MKD_PUNCT_ESCAPE_LUT: [256]u8 = mkdBuildPunctEscapeLut();
const MKD_INLINE_SPECIAL_LUT: [256]u8 = mkdBuildInlineSpecialLut();

pub struct MarkedOptions {
    pub gfm: bool,
    pub sanitize_links: bool,
    pub breaks: bool,
}

pub fn defaultMarkedOptions(): MarkedOptions {
    return MarkedOptions {
        gfm: true,
        sanitize_links: true,
        breaks: false,
    };
}

// ============================================================================
// Low-level buffer & ASCII helpers (all private symbols prefixed with `mkd`)
// ============================================================================

fn mkdWriteByte(out: []u8, pos: *usize, b: u8) {
    if (pos.* < out.len) {
        out[pos.*] = b;
        pos.* = pos.* + 1;
    }
}

fn mkdWriteStr(out: []u8, pos: *usize, s: string) {
    if (pos.* >= out.len || s.len == 0) { return; }
    const avail = out.len - pos.*;
    const n = if (s.len < avail) s.len else avail;
    @memcpy(out[pos.* .. pos.* + n], s[0..n]);
    pos.* = pos.* + n;
}

fn mkdFindHtmlSpecial(s: string, start: usize): usize {
    let i: usize = start;
    const v_amp: MkdVec16 = @splat(38 as u8);
    const v_lt: MkdVec16 = @splat(60 as u8);
    const v_gt: MkdVec16 = @splat(62 as u8);
    const v_quot: MkdVec16 = @splat(34 as u8);
    const v_apos: MkdVec16 = @splat(39 as u8);
    while (i + 16 <= s.len) {
        const v: MkdVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_amp) || @reduce(.Or, v == v_lt) || @reduce(.Or, v == v_gt) ||
            @reduce(.Or, v == v_quot) || @reduce(.Or, v == v_apos)) {
            let j: usize = 0;
            while (j < 16) {
                const b = s[i + j];
                if (b == 38 || b == 60 || b == 62 || b == 34 || b == 39) {
                    return i + j;
                }
                j = j + 1;
            }
        }
        i = i + 16;
    }
    while (i < s.len) {
        const b = s[i];
        if (b == 38 || b == 60 || b == 62 || b == 34 || b == 39) {
            return i;
        }
        i = i + 1;
    }
    return s.len;
}

fn mkdWriteEscaped(out: []u8, pos: *usize, s: string) {
    if (s.len == 0 || pos.* >= out.len) {
        return;
    }
    let i: usize = 0;
    while (i < s.len && pos.* < out.len) {
        const next_sp = mkdFindHtmlSpecial(s, i);
        if (next_sp > i) {
            mkdWriteStr(out, pos, s[i..next_sp]);
            i = next_sp;
            if (i >= s.len || pos.* >= out.len) { break; }
        }
        mkdWriteEscapedByte(out, pos, s[i]);
        i = i + 1;
    }
}

fn mkdWriteEscapedByte(out: []u8, pos: *usize, b: u8) {
    if (b == 38) {
        mkdWriteStr(out, pos, "&amp;");
    } else if (b == 60) {
        mkdWriteStr(out, pos, "&lt;");
    } else if (b == 62) {
        mkdWriteStr(out, pos, "&gt;");
    } else if (b == 34) {
        mkdWriteStr(out, pos, "&quot;");
    } else if (b == 39) {
        mkdWriteStr(out, pos, "&#39;");
    } else {
        mkdWriteByte(out, pos, b);
    }
}

fn mkdIsSpace(c: u8): bool {
    return c == 32 || c == 9;
}

fn mkdIsPunctEscape(c: u8): bool {
    return MKD_PUNCT_ESCAPE_LUT[c as usize] != 0;
}

// ============================================================================
// URL XSS Sanitization
// ============================================================================

fn mkdStartsWithIgnoreCaseClean(url: string, prefix: string): bool {
    let ui: usize = 0;
    // Skip leading whitespace and ASCII control chars
    while (ui < url.len && url[ui] <= 32) {
        ui = ui + 1;
    }
    let pi: usize = 0;
    while (pi < prefix.len) {
        // Ignore embedded control characters / tabs / newlines inside scheme
        while (ui < url.len && url[ui] < 32) {
            ui = ui + 1;
        }
        if (ui >= url.len) {
            return false;
        }
        if (toLowerByte(url[ui]) != toLowerByte(prefix[pi])) {
            return false;
        }
        ui = ui + 1;
        pi = pi + 1;
    }
    return true;
}

fn mkdIsSafeUrl(url: string): bool {
    if (mkdStartsWithIgnoreCaseClean(url, "javascript:")) {
        return false;
    }
    if (mkdStartsWithIgnoreCaseClean(url, "vbscript:")) {
        return false;
    }
    if (mkdStartsWithIgnoreCaseClean(url, "data:text/html")) {
        return false;
    }
    return true;
}

// ============================================================================
// Line Navigation Helpers
// ============================================================================

fn mkdFindLineEnd(s: string, start: usize): usize {
    return findByteInBytes(s, 10, start) ?? s.len;
}

fn mkdTrimCr(s: string, start: usize, end: usize): usize {
    if (end > start && s[end - 1] == 13) {
        return end - 1;
    }
    return end;
}

fn mkdSkipIndent(s: string, start: usize, end: usize, max_spaces: usize): usize {
    let i: usize = start;
    let count: usize = 0;
    while (i < end && count < max_spaces && s[i] == 32) {
        i = i + 1;
        count = count + 1;
    }
    return i;
}

fn mkdIsBlankLine(s: string, start: usize, end: usize): bool {
    let i: usize = start;
    while (i < end) {
        if (!mkdIsSpace(s[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

// ============================================================================
// Block Detection Helpers
// ============================================================================

// Returns heading level 1..6, or 0 if not an ATX heading.
// Sets content_start and content_end via pointers when > 0.
fn mkdMatchHeading(s: string, line_start: usize, line_end: usize, content_start: *usize, content_end: *usize): usize {
    let i: usize = mkdSkipIndent(s, line_start, line_end, 3);
    let level: usize = 0;
    while (i < line_end && s[i] == 35 && level < 7) {
        level = level + 1;
        i = i + 1;
    }
    if (level == 0 || level > 6) {
        return 0;
    }
    if (i < line_end && !mkdIsSpace(s[i])) {
        return 0;
    }
    while (i < line_end && mkdIsSpace(s[i])) {
        i = i + 1;
    }
    let e: usize = line_end;
    while (e > i && mkdIsSpace(s[e - 1])) {
        e = e - 1;
    }
    // Strip optional closing sequence of `#` preceded by space/tab (or all `#`)
    let hash_end: usize = e;
    while (hash_end > i && s[hash_end - 1] == 35) {
        hash_end = hash_end - 1;
    }
    if (hash_end < e) {
        if (hash_end == i) {
            e = i;
        } else if (mkdIsSpace(s[hash_end - 1])) {
            e = hash_end;
            while (e > i && mkdIsSpace(s[e - 1])) {
                e = e - 1;
            }
        }
    }
    content_start.* = i;
    content_end.* = e;
    return level;
}

// Returns true if line is a thematic break (---, ***, ___).
fn mkdIsHorizontalRule(s: string, line_start: usize, line_end: usize): bool {
    let i: usize = mkdSkipIndent(s, line_start, line_end, 3);
    if (i >= line_end) {
        return false;
    }
    const marker: u8 = s[i];
    if (marker != 45 && marker != 42 && marker != 95) {
        return false;
    }
    let count: usize = 0;
    while (i < line_end) {
        if (s[i] == marker) {
            count = count + 1;
        } else if (!mkdIsSpace(s[i])) {
            return false;
        }
        i = i + 1;
    }
    return count >= 3;
}

// Returns fence length (>= 3) if line opens a fenced code block (``` or ~~~), else 0.
fn mkdMatchFenceOpen(
    s: string,
    line_start: usize,
    line_end: usize,
    fence_char: *u8,
    lang_start: *usize,
    lang_end: *usize,
): usize {
    let i: usize = mkdSkipIndent(s, line_start, line_end, 3);
    if (i >= line_end) {
        return 0;
    }
    const ch: u8 = s[i];
    if (ch != 96 && ch != 126) {
        return 0;
    }
    let count: usize = 0;
    while (i < line_end && s[i] == ch) {
        count = count + 1;
        i = i + 1;
    }
    if (count < 3) {
        return 0;
    }
    // Backtick fence info string cannot contain backticks
    if (ch == 96) {
        if (findByteInBytes(s[i..line_end], 96, 0) != null) {
            return 0;
        }
    }
    while (i < line_end && mkdIsSpace(s[i])) {
        i = i + 1;
    }
    const ls: usize = i;
    while (i < line_end && !mkdIsSpace(s[i])) {
        i = i + 1;
    }
    fence_char.* = ch;
    lang_start.* = ls;
    lang_end.* = i;
    return count;
}

fn mkdIsFenceClose(s: string, line_start: usize, line_end: usize, fence_char: u8, min_len: usize): bool {
    let i: usize = mkdSkipIndent(s, line_start, line_end, 3);
    let count: usize = 0;
    while (i < line_end && s[i] == fence_char) {
        count = count + 1;
        i = i + 1;
    }
    if (count < min_len) {
        return false;
    }
    while (i < line_end) {
        if (!mkdIsSpace(s[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn mkdIsBlockquoteLine(s: string, line_start: usize, line_end: usize): bool {
    const i: usize = mkdSkipIndent(s, line_start, line_end, 3);
    return i < line_end && s[i] == 62;
}

// Returns content start index if line is an unordered list item (`- `, `* `, `+ `), else 0.
fn mkdMatchUnorderedItem(s: string, line_start: usize, line_end: usize): usize {
    if (mkdIsHorizontalRule(s, line_start, line_end)) {
        return 0;
    }
    const i: usize = mkdSkipIndent(s, line_start, line_end, 3);
    if (i >= line_end) {
        return 0;
    }
    const c: u8 = s[i];
    if (c != 45 && c != 42 && c != 43) {
        return 0;
    }
    if (i + 1 >= line_end) {
        return 0;
    }
    if (!mkdIsSpace(s[i + 1])) {
        return 0;
    }
    let content: usize = i + 2;
    while (content < line_end && mkdIsSpace(s[content])) {
        content = content + 1;
    }
    return content;
}

// Returns content start index if line is an ordered list item (`1. ` or `1) `), else 0.
fn mkdMatchOrderedItem(s: string, line_start: usize, line_end: usize, num_out: *usize): usize {
    let i: usize = mkdSkipIndent(s, line_start, line_end, 3);
    let digits: usize = 0;
    let num: usize = 0;
    while (i < line_end && isDigit(s[i]) && digits < 9) {
        num = num * 10 + ((s[i] - 48) as usize);
        digits = digits + 1;
        i = i + 1;
    }
    if (digits == 0 || i + 1 >= line_end) {
        return 0;
    }
    if (s[i] != 46 && s[i] != 41) {
        return 0;
    }
    if (!mkdIsSpace(s[i + 1])) {
        return 0;
    }
    let content: usize = i + 2;
    while (content < line_end && mkdIsSpace(s[content])) {
        content = content + 1;
    }
    num_out.* = num;
    return content;
}

fn mkdStartsNewBlock(s: string, line_start: usize, line_end: usize): bool {
    let cs: usize = 0;
    let ce: usize = 0;
    if (mkdMatchHeading(s, line_start, line_end, &cs, &ce) > 0) {
        return true;
    }
    if (mkdIsHorizontalRule(s, line_start, line_end)) {
        return true;
    }
    let fc: u8 = 0;
    let ls: usize = 0;
    let le: usize = 0;
    if (mkdMatchFenceOpen(s, line_start, line_end, &fc, &ls, &le) > 0) {
        return true;
    }
    if (mkdIsBlockquoteLine(s, line_start, line_end)) {
        return true;
    }
    if (mkdMatchUnorderedItem(s, line_start, line_end) > 0) {
        return true;
    }
    let ord_num: usize = 0;
    if (mkdMatchOrderedItem(s, line_start, line_end, &ord_num) > 0) {
        return true;
    }
    return false;
}

// ============================================================================
// Inline Parsing Helpers
// ============================================================================

fn mkdFindClosingDouble(s: string, start: usize, ch: u8): i64 {
    let i: usize = start;
    while (i + 1 < s.len) {
        if (s[i] == 92 && i + 1 < s.len) {
            i = i + 2;
            continue;
        }
        if (s[i] == 96) {
            // Skip inline code span
            let tick_len: usize = 0;
            while (i < s.len && s[i] == 96) {
                tick_len = tick_len + 1;
                i = i + 1;
            }
            let close_found: bool = false;
            while (i < s.len) {
                const tick_pos = findByteInBytes(s, 96, i) ?? s.len;
                if (tick_pos >= s.len) {
                    i = s.len;
                    break;
                }
                i = tick_pos;
                let cl: usize = 0;
                while (i < s.len && s[i] == 96) {
                    cl = cl + 1;
                    i = i + 1;
                }
                if (cl == tick_len) {
                    close_found = true;
                    break;
                }
            }
            if (!close_found) {
                break;
            }
            continue;
        }
        if (s[i] == ch && s[i + 1] == ch) {
            // For `***bold+italic***`, advance to the last two of a triple run
            if (i + 2 < s.len && s[i + 2] == ch) {
                return (i + 1) as i64;
            }
            if (i > start && !mkdIsSpace(s[i - 1])) {
                return i as i64;
            }
        }
        i = i + 1;
    }
    return -1;
}

fn mkdFindClosingSingle(s: string, start: usize, ch: u8): i64 {
    let i: usize = start;
    while (i < s.len) {
        if (s[i] == 92 && i + 1 < s.len) {
            i = i + 2;
            continue;
        }
        if (s[i] == 96) {
            let tick_len: usize = 0;
            while (i < s.len && s[i] == 96) {
                tick_len = tick_len + 1;
                i = i + 1;
            }
            while (i < s.len) {
                const tick_pos = findByteInBytes(s, 96, i) ?? s.len;
                if (tick_pos >= s.len) {
                    i = s.len;
                    break;
                }
                i = tick_pos;
                let cl: usize = 0;
                while (i < s.len && s[i] == 96) {
                    cl = cl + 1;
                    i = i + 1;
                }
                if (cl == tick_len) {
                    break;
                }
            }
            continue;
        }
        if (s[i] == ch) {
            // Do not match `**` as single `*`
            if (i + 1 < s.len && s[i + 1] == ch) {
                i = i + 2;
                continue;
            }
            if (i > start && !mkdIsSpace(s[i - 1])) {
                // For `_`, require non-alnum right after closing `_`
                if (ch == 95 && i + 1 < s.len && isAlnum(s[i + 1])) {
                    i = i + 1;
                    continue;
                }
                return i as i64;
            }
        }
        i = i + 1;
    }
    return -1;
}

// Parse `[label](dest)` starting at index of `[` (with `open_bracket`).
// Returns index right after `)` on success, or 0 on failure.
fn mkdMatchLinkOrImage(
    s: string,
    open_bracket: usize,
    label_start: *usize,
    label_end: *usize,
    url_start: *usize,
    url_end: *usize,
): usize {
    if (open_bracket >= s.len || s[open_bracket] != 91) {
        return 0;
    }
    let i: usize = open_bracket + 1;
    let depth: usize = 1;
    while (i < s.len && depth > 0) {
        if (s[i] == 92 && i + 1 < s.len) {
            i = i + 2;
            continue;
        }
        if (s[i] == 91) {
            depth = depth + 1;
        } else if (s[i] == 93) {
            depth = depth - 1;
            if (depth == 0) {
                break;
            }
        }
        i = i + 1;
    }
    if (depth != 0 || i + 1 >= s.len || s[i + 1] != 40) {
        return 0;
    }
    const close_bracket: usize = i;
    let u: usize = close_bracket + 2;
    while (u < s.len && mkdIsSpace(s[u])) {
        u = u + 1;
    }
    const us: usize = u;
    let paren_depth: usize = 1;
    let ue: usize = u;
    let in_title: bool = false;
    while (u < s.len && paren_depth > 0) {
        if (s[u] == 92 && u + 1 < s.len) {
            u = u + 2;
            if (!in_title) {
                ue = u;
            }
            continue;
        }
        if (s[u] == 40) {
            paren_depth = paren_depth + 1;
        } else if (s[u] == 41) {
            paren_depth = paren_depth - 1;
            if (paren_depth == 0) {
                break;
            }
        } else if (!in_title && mkdIsSpace(s[u])) {
            // Optional title after whitespace; stop URL slice here
            in_title = true;
        }
        u = u + 1;
        if (!in_title) {
            ue = u;
        }
    }
    if (paren_depth != 0) {
        return 0;
    }
    // Strip optional `<...>` angle brackets around destination URL
    let final_us: usize = us;
    let final_ue: usize = ue;
    if (final_ue > final_us + 1 && s[final_us] == 60 && s[final_ue - 1] == 62) {
        final_us = final_us + 1;
        final_ue = final_ue - 1;
    }
    label_start.* = open_bracket + 1;
    label_end.* = close_bracket;
    url_start.* = final_us;
    url_end.* = final_ue;
    return u + 1;
}

fn mkdIsInlineSpecial(c: u8): bool {
    return MKD_INLINE_SPECIAL_LUT[c as usize] != 0;
}

fn mkdParseInlineRec(src: string, opts: MarkedOptions, out: []u8, pos: *usize, depth: usize) {
    if (depth >= 16) {
        mkdWriteEscaped(out, pos, src);
        return;
    }
    let i: usize = 0;
    while (i < src.len && pos.* < out.len) {
        const ch: u8 = src[i];

        // 1. Backslash escape or backslash hard line break
        if (ch == 92 && i + 1 < src.len) {
            const next: u8 = src[i + 1];
            if (next == 10) {
                mkdWriteStr(out, pos, "<br>\n");
                i = i + 2;
                continue;
            }
            if (mkdIsPunctEscape(next)) {
                mkdWriteEscapedByte(out, pos, next);
                i = i + 2;
                continue;
            }
        }

        // 2. Trailing two+ spaces before '\n' -> hard line break
        if (ch == 32) {
            let sp: usize = i;
            while (sp < src.len && src[sp] == 32) {
                sp = sp + 1;
            }
            if (sp - i >= 2 && sp < src.len && src[sp] == 10) {
                mkdWriteStr(out, pos, "<br>\n");
                i = sp + 1;
                continue;
            }
            mkdWriteByte(out, pos, 32);
            i = i + 1;
            continue;
        }

        // 3. Newline: `<br>\n` when opts.breaks is true, else `\n`
        if (ch == 10) {
            if (opts.breaks) {
                mkdWriteStr(out, pos, "<br>\n");
            } else {
                mkdWriteByte(out, pos, 10);
            }
            i = i + 1;
            continue;
        }

        // 4. Inline code span (`code` or ``code``)
        if (ch == 96) {
            let tick_len: usize = 0;
            while (i + tick_len < src.len && src[i + tick_len] == 96) {
                tick_len = tick_len + 1;
            }
            let j: usize = i + tick_len;
            let found: bool = false;
            let close_start: usize = 0;
            while (j < src.len) {
                const tick_pos = findByteInBytes(src, 96, j) ?? src.len;
                if (tick_pos >= src.len) {
                    break;
                }
                j = tick_pos;
                let cl: usize = 0;
                while (j + cl < src.len && src[j + cl] == 96) {
                    cl = cl + 1;
                }
                if (cl == tick_len) {
                    found = true;
                    close_start = j;
                    break;
                }
                j = j + cl;
            }
            if (found) {
                let cs: usize = i + tick_len;
                let ce: usize = close_start;
                // CommonMark: strip one leading & one trailing space if both present and not all spaces
                if (ce >= cs + 2 && src[cs] == 32 && src[ce - 1] == 32) {
                    let all_space: bool = true;
                    let k: usize = cs;
                    while (k < ce) {
                        if (src[k] != 32) {
                            all_space = false;
                            break;
                        }
                        k = k + 1;
                    }
                    if (!all_space) {
                        cs = cs + 1;
                        ce = ce - 1;
                    }
                }
                mkdWriteStr(out, pos, "<code>");
                mkdWriteEscaped(out, pos, src[cs..ce]);
                mkdWriteStr(out, pos, "</code>");
                i = close_start + tick_len;
                continue;
            }
            // No matching closing ticks: emit literally
            mkdWriteStr(out, pos, src[i..(i + tick_len)]);
            i = i + tick_len;
            continue;
        }

        // 5. Image `![alt](src)`
        if (ch == 33 && i + 1 < src.len && src[i + 1] == 91) {
            let ls: usize = 0;
            let le: usize = 0;
            let us: usize = 0;
            let ue: usize = 0;
            const after: usize = mkdMatchLinkOrImage(src, i + 1, &ls, &le, &us, &ue);
            if (after > 0) {
                const url: string = src[us..ue];
                const alt: string = src[ls..le];
                mkdWriteStr(out, pos, "<img src=\"");
                if (!opts.sanitize_links || mkdIsSafeUrl(url)) {
                    mkdWriteEscaped(out, pos, url);
                }
                mkdWriteStr(out, pos, "\" alt=\"");
                mkdWriteEscaped(out, pos, alt);
                mkdWriteStr(out, pos, "\">");
                i = after;
                continue;
            }
        }

        // 6. Link `[text](url)`
        if (ch == 91) {
            let ls: usize = 0;
            let le: usize = 0;
            let us: usize = 0;
            let ue: usize = 0;
            const after: usize = mkdMatchLinkOrImage(src, i, &ls, &le, &us, &ue);
            if (after > 0) {
                const url: string = src[us..ue];
                const label: string = src[ls..le];
                mkdWriteStr(out, pos, "<a href=\"");
                if (!opts.sanitize_links || mkdIsSafeUrl(url)) {
                    mkdWriteEscaped(out, pos, url);
                }
                mkdWriteStr(out, pos, "\">");
                mkdParseInlineRec(label, opts, out, pos, depth + 1);
                mkdWriteStr(out, pos, "</a>");
                i = after;
                continue;
            }
        }

        // 7. Strikethrough `~~...~~` (GFM)
        if (opts.gfm && ch == 126 && i + 1 < src.len && src[i + 1] == 126) {
            if (i + 2 < src.len && !mkdIsSpace(src[i + 2])) {
                const close_idx: i64 = mkdFindClosingDouble(src, i + 2, 126);
                if (close_idx > 0) {
                    const ci: usize = close_idx as usize;
                    mkdWriteStr(out, pos, "<del>");
                    mkdParseInlineRec(src[(i + 2)..ci], opts, out, pos, depth + 1);
                    mkdWriteStr(out, pos, "</del>");
                    i = ci + 2;
                    continue;
                }
            }
        }

        // 8. Bold `**...**` or `__...__`
        if ((ch == 42 || ch == 95) && i + 1 < src.len && src[i + 1] == ch) {
            const left_ok: bool = if (ch == 95) { i == 0 || !isAlnum(src[i - 1]) } else { true };
            if (left_ok && i + 2 < src.len && !mkdIsSpace(src[i + 2])) {
                const close_idx: i64 = mkdFindClosingDouble(src, i + 2, ch);
                if (close_idx > 0) {
                    const ci: usize = close_idx as usize;
                    mkdWriteStr(out, pos, "<strong>");
                    mkdParseInlineRec(src[(i + 2)..ci], opts, out, pos, depth + 1);
                    mkdWriteStr(out, pos, "</strong>");
                    i = ci + 2;
                    continue;
                }
            }
        }

        // 9. Italic `*...*` or `_..._`
        if (ch == 42 || ch == 95) {
            const left_ok: bool = if (ch == 95) { i == 0 || !isAlnum(src[i - 1]) } else { true };
            if (left_ok && i + 1 < src.len && !mkdIsSpace(src[i + 1])) {
                const close_idx: i64 = mkdFindClosingSingle(src, i + 1, ch);
                if (close_idx > 0) {
                    const ci: usize = close_idx as usize;
                    mkdWriteStr(out, pos, "<em>");
                    mkdParseInlineRec(src[(i + 1)..ci], opts, out, pos, depth + 1);
                    mkdWriteStr(out, pos, "</em>");
                    i = ci + 1;
                    continue;
                }
            }
        }

        // 10. Plain text run (batch-escaped via @cap/escape-html)
        const text_start: usize = i;
        i = i + 1;
        while (i < src.len && !mkdIsInlineSpecial(src[i])) {
            i = i + 1;
        }
        mkdWriteEscaped(out, pos, src[text_start..i]);
    }
}

// ============================================================================
// Block Parsing Engine
// ============================================================================

fn mkdStripBlockquoteLines(md: string, start: usize, end: usize, dst: []u8): usize {
    let w: usize = 0;
    let cur: usize = start;
    while (cur < end) {
        const raw_end: usize = mkdFindLineEnd(md, cur);
        const le: usize = if (raw_end < end) { raw_end } else { end };
        const clean_end: usize = mkdTrimCr(md, cur, le);
        let p: usize = mkdSkipIndent(md, cur, clean_end, 3);
        if (p < clean_end && md[p] == 62) {
            p = p + 1;
            if (p < clean_end && md[p] == 32) {
                p = p + 1;
            }
        }
        if (p < clean_end && w < dst.len) {
            const avail = dst.len - w;
            const rem = clean_end - p;
            const n = if (rem < avail) rem else avail;
            @memcpy(dst[w .. w + n], md[p .. p + n]);
            w = w + n;
        }
        if (le < end && w < dst.len) {
            dst[w] = 10;
            w = w + 1;
        }
        if (raw_end >= end) {
            break;
        }
        cur = raw_end + 1;
    }
    return w;
}

fn mkdParseBlocksRec(md: string, opts: MarkedOptions, out: []u8, pos: *usize, depth: usize) {
    if (depth >= 12) {
        mkdWriteEscaped(out, pos, md);
        return;
    }
    let cur: usize = 0;
    while (cur < md.len && pos.* < out.len) {
        const raw_end: usize = mkdFindLineEnd(md, cur);
        const line_end: usize = mkdTrimCr(md, cur, raw_end);
        const next_cur: usize = if (raw_end < md.len) { raw_end + 1 } else { md.len };

        // 1. Skip blank lines
        if (mkdIsBlankLine(md, cur, line_end)) {
            cur = next_cur;
            continue;
        }

        // 2. ATX Heading (`#` .. `######`)
        let h_start: usize = 0;
        let h_end: usize = 0;
        const h_level: usize = mkdMatchHeading(md, cur, line_end, &h_start, &h_end);
        if (h_level > 0) {
            const lvl_char: u8 = (48 + h_level) as u8;
            mkdWriteStr(out, pos, "<h");
            mkdWriteByte(out, pos, lvl_char);
            mkdWriteByte(out, pos, 62);
            mkdParseInlineRec(md[h_start..h_end], opts, out, pos, 0);
            mkdWriteStr(out, pos, "</h");
            mkdWriteByte(out, pos, lvl_char);
            mkdWriteStr(out, pos, ">\n");
            cur = next_cur;
            continue;
        }

        // 3. Fenced Code Block (``` or ~~~)
        let fence_ch: u8 = 0;
        let lang_start: usize = 0;
        let lang_end: usize = 0;
        const fence_len: usize = mkdMatchFenceOpen(md, cur, line_end, &fence_ch, &lang_start, &lang_end);
        if (fence_len > 0) {
            mkdWriteStr(out, pos, "<pre><code");
            if (lang_end > lang_start) {
                mkdWriteStr(out, pos, " class=\"language-");
                mkdWriteEscaped(out, pos, md[lang_start..lang_end]);
                mkdWriteByte(out, pos, 34);
            }
            mkdWriteByte(out, pos, 62);
            cur = next_cur;
            while (cur < md.len) {
                const c_raw: usize = mkdFindLineEnd(md, cur);
                const c_end: usize = mkdTrimCr(md, cur, c_raw);
                const c_next: usize = if (c_raw < md.len) { c_raw + 1 } else { md.len };
                if (mkdIsFenceClose(md, cur, c_end, fence_ch, fence_len)) {
                    cur = c_next;
                    break;
                }
                mkdWriteEscaped(out, pos, md[cur..c_end]);
                mkdWriteByte(out, pos, 10);
                cur = c_next;
            }
            mkdWriteStr(out, pos, "</code></pre>\n");
            continue;
        }

        // 4. Horizontal Rule (`---`, `***`, `___`)
        if (mkdIsHorizontalRule(md, cur, line_end)) {
            mkdWriteStr(out, pos, "<hr>\n");
            cur = next_cur;
            continue;
        }

        // 5. Blockquote (`> ...`)
        if (mkdIsBlockquoteLine(md, cur, line_end)) {
            const bq_start: usize = cur;
            let bq_end: usize = line_end;
            cur = next_cur;
            while (cur < md.len) {
                const q_raw: usize = mkdFindLineEnd(md, cur);
                const q_end: usize = mkdTrimCr(md, cur, q_raw);
                const q_next: usize = if (q_raw < md.len) { q_raw + 1 } else { md.len };
                if (!mkdIsBlockquoteLine(md, cur, q_end)) {
                    break;
                }
                bq_end = q_end;
                cur = q_next;
            }
            mkdWriteStr(out, pos, "<blockquote>\n");
            let bq_buf: [2048]u8 = undefined;
            const inner_len: usize = mkdStripBlockquoteLines(md, bq_start, bq_end, &bq_buf);
            mkdParseBlocksRec(bq_buf[0..inner_len], opts, out, pos, depth + 1);
            mkdWriteStr(out, pos, "</blockquote>\n");
            continue;
        }

        // 6. Unordered List (`- `, `* `, `+ `)
        const ul_item: usize = mkdMatchUnorderedItem(md, cur, line_end);
        if (ul_item > 0) {
            mkdWriteStr(out, pos, "<ul>\n");
            while (cur < md.len) {
                const l_raw: usize = mkdFindLineEnd(md, cur);
                const l_end: usize = mkdTrimCr(md, cur, l_raw);
                const l_next: usize = if (l_raw < md.len) { l_raw + 1 } else { md.len };
                const item_start: usize = mkdMatchUnorderedItem(md, cur, l_end);
                if (item_start == 0) {
                    break;
                }
                mkdWriteStr(out, pos, "<li>");
                // Optional GFM task list item: `[ ] ` or `[x] ` / `[X] `
                let content_start: usize = item_start;
                if (opts.gfm && content_start + 3 <= l_end && md[content_start] == 91 && md[content_start + 2] == 93) {
                    const mark: u8 = md[content_start + 1];
                    if (mark == 32 && (content_start + 3 == l_end || md[content_start + 3] == 32)) {
                        mkdWriteStr(out, pos, "<input disabled=\"\" type=\"checkbox\"> ");
                        content_start = if (content_start + 4 <= l_end) { content_start + 4 } else { l_end };
                    } else if ((mark == 120 || mark == 88) && (content_start + 3 == l_end || md[content_start + 3] == 32)) {
                        mkdWriteStr(out, pos, "<input checked=\"\" disabled=\"\" type=\"checkbox\"> ");
                        content_start = if (content_start + 4 <= l_end) { content_start + 4 } else { l_end };
                    }
                }
                mkdParseInlineRec(md[content_start..l_end], opts, out, pos, 0);
                mkdWriteStr(out, pos, "</li>\n");
                cur = l_next;
            }
            mkdWriteStr(out, pos, "</ul>\n");
            continue;
        }

        // 7. Ordered List (`1. `, `2) `)
        let ord_start_num: usize = 1;
        const ol_item: usize = mkdMatchOrderedItem(md, cur, line_end, &ord_start_num);
        if (ol_item > 0) {
            if (ord_start_num != 1) {
                mkdWriteStr(out, pos, "<ol start=\"");
                let nbuf: [24]u8 = undefined;
                const nslice = std.fmt.bufPrint(nbuf[0..24], "{d}", .{ord_start_num}) catch "";
                mkdWriteStr(out, pos, nslice);
                mkdWriteStr(out, pos, "\">\n");
            } else {
                mkdWriteStr(out, pos, "<ol>\n");
            }
            while (cur < md.len) {
                const l_raw: usize = mkdFindLineEnd(md, cur);
                const l_end: usize = mkdTrimCr(md, cur, l_raw);
                const l_next: usize = if (l_raw < md.len) { l_raw + 1 } else { md.len };
                let dummy_num: usize = 0;
                const item_start: usize = mkdMatchOrderedItem(md, cur, l_end, &dummy_num);
                if (item_start == 0) {
                    break;
                }
                mkdWriteStr(out, pos, "<li>");
                mkdParseInlineRec(md[item_start..l_end], opts, out, pos, 0);
                mkdWriteStr(out, pos, "</li>\n");
                cur = l_next;
            }
            mkdWriteStr(out, pos, "</ol>\n");
            continue;
        }

        // 8. Paragraph (collect contiguous lines until blank line or new block)
        mkdWriteStr(out, pos, "<p>");
        let first_line: bool = true;
        while (cur < md.len) {
            const p_raw: usize = mkdFindLineEnd(md, cur);
            const p_end: usize = mkdTrimCr(md, cur, p_raw);
            const p_next: usize = if (p_raw < md.len) { p_raw + 1 } else { md.len };
            if (!first_line) {
                if (mkdIsBlankLine(md, cur, p_end) || mkdStartsNewBlock(md, cur, p_end)) {
                    break;
                }
            }
            const trimmed_start: usize = mkdSkipIndent(md, cur, p_end, 3);
            // Detect hard line break (2+ trailing spaces or trailing `\`) before joining with next line
            let has_hard_break: bool = false;
            let content_end: usize = p_end;
            if (p_end >= trimmed_start + 2 && md[p_end - 1] == 32 && md[p_end - 2] == 32) {
                has_hard_break = true;
                while (content_end > trimmed_start && md[content_end - 1] == 32) {
                    content_end = content_end - 1;
                }
            } else if (p_end > trimmed_start && md[p_end - 1] == 92) {
                has_hard_break = true;
                content_end = p_end - 1;
            }
            if (!first_line) {
                // Previous line ended without hard break -> emit newline or `<br>\n`
                if (opts.breaks) {
                    mkdWriteStr(out, pos, "<br>\n");
                } else {
                    mkdWriteByte(out, pos, 10);
                }
            }
            mkdParseInlineRec(md[trimmed_start..content_end], opts, out, pos, 0);
            cur = p_next;
            first_line = false;
            // If this line had a hard break and there is another paragraph line, emit `<br>\n`
            if (has_hard_break && cur < md.len) {
                const n_raw: usize = mkdFindLineEnd(md, cur);
                const n_end: usize = mkdTrimCr(md, cur, n_raw);
                if (!mkdIsBlankLine(md, cur, n_end) && !mkdStartsNewBlock(md, cur, n_end)) {
                    mkdWriteStr(out, pos, "<br>\n");
                    // Prevent double separator on next iteration
                    first_line = true;
                }
            }
        }
        mkdWriteStr(out, pos, "</p>\n");
    }
}

// ============================================================================
// Plain-Text Stripper (`stripMarkdown`)
// ============================================================================

fn mkdStripInlineRec(src: string, out: []u8, pos: *usize, depth: usize) {
    if (depth >= 16) {
        mkdWriteStr(out, pos, src);
        return;
    }
    let i: usize = 0;
    while (i < src.len && pos.* < out.len) {
        const ch: u8 = src[i];
        if (ch == 92 && i + 1 < src.len && mkdIsPunctEscape(src[i + 1])) {
            mkdWriteByte(out, pos, src[i + 1]);
            i = i + 2;
            continue;
        }
        if (ch == 96) {
            let tick_len: usize = 0;
            while (i + tick_len < src.len && src[i + tick_len] == 96) {
                tick_len = tick_len + 1;
            }
            let j: usize = i + tick_len;
            let found: bool = false;
            let close_start: usize = 0;
            while (j < src.len) {
                const tick_pos = findByteInBytes(src, 96, j) ?? src.len;
                if (tick_pos >= src.len) {
                    break;
                }
                j = tick_pos;
                let cl: usize = 0;
                while (j + cl < src.len && src[j + cl] == 96) {
                    cl = cl + 1;
                }
                if (cl == tick_len) {
                    found = true;
                    close_start = j;
                    break;
                }
                j = j + cl;
            }
            if (found) {
                mkdWriteStr(out, pos, src[(i + tick_len)..close_start]);
                i = close_start + tick_len;
                continue;
            }
        }
        if (ch == 33 && i + 1 < src.len && src[i + 1] == 91) {
            let ls: usize = 0;
            let le: usize = 0;
            let us: usize = 0;
            let ue: usize = 0;
            const after: usize = mkdMatchLinkOrImage(src, i + 1, &ls, &le, &us, &ue);
            if (after > 0) {
                mkdWriteStr(out, pos, src[ls..le]);
                i = after;
                continue;
            }
        }
        if (ch == 91) {
            let ls: usize = 0;
            let le: usize = 0;
            let us: usize = 0;
            let ue: usize = 0;
            const after: usize = mkdMatchLinkOrImage(src, i, &ls, &le, &us, &ue);
            if (after > 0) {
                mkdStripInlineRec(src[ls..le], out, pos, depth + 1);
                i = after;
                continue;
            }
        }
        if ((ch == 42 || ch == 95 || ch == 126) && i + 1 < src.len && src[i + 1] == ch) {
            const close_idx: i64 = mkdFindClosingDouble(src, i + 2, ch);
            if (close_idx > 0) {
                const ci: usize = close_idx as usize;
                mkdStripInlineRec(src[(i + 2)..ci], out, pos, depth + 1);
                i = ci + 2;
                continue;
            }
        }
        if (ch == 42 || ch == 95) {
            const left_ok: bool = if (ch == 95) { i == 0 || !isAlnum(src[i - 1]) } else { true };
            if (left_ok && i + 1 < src.len && !mkdIsSpace(src[i + 1])) {
                const close_idx: i64 = mkdFindClosingSingle(src, i + 1, ch);
                if (close_idx > 0) {
                    const ci: usize = close_idx as usize;
                    mkdStripInlineRec(src[(i + 1)..ci], out, pos, depth + 1);
                    i = ci + 1;
                    continue;
                }
            }
        }
        const text_start: usize = i;
        i = i + 1;
        while (i < src.len && !mkdIsInlineSpecial(src[i])) {
            i = i + 1;
        }
        mkdWriteStr(out, pos, src[text_start..i]);
    }
}

// ============================================================================
// Public API
// ============================================================================

/// Compile CommonMark/GFM Markdown `md` into HTML in caller-provided `out`.
/// Uses `defaultMarkedOptions()` (`gfm: true`, `sanitize_links: true`, `breaks: false`).
/// Returns the number of bytes written to `out`.
pub fn parseMarkdown(md: string, out: []u8): usize {
    return parseMarkdownWithOptions(md, defaultMarkedOptions(), out);
}

/// Compile CommonMark/GFM Markdown `md` into HTML with explicit `MarkedOptions`.
/// Returns the number of bytes written to `out`.
pub fn parseMarkdownWithOptions(md: string, opts: MarkedOptions, out: []u8): usize {
    if (md.len == 0 || out.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    mkdParseBlocksRec(md, opts, out, &pos, 0);
    return pos;
}

/// Compile inline-only Markdown `src` (no `<p>` wrapper) into HTML in `out`.
/// Returns the number of bytes written to `out`.
pub fn parseInlineMarkdown(src: string, out: []u8): usize {
    if (src.len == 0 || out.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    mkdParseInlineRec(src, defaultMarkedOptions(), out, &pos, 0);
    return pos;
}

/// Strip Markdown formatting from `md` and write plain text into `out`.
/// Returns the number of bytes written to `out`.
pub fn stripMarkdown(md: string, out: []u8): usize {
    if (md.len == 0 || out.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    let cur: usize = 0;
    let in_fence: bool = false;
    let fence_ch: u8 = 0;
    let fence_len: usize = 0;
    let wrote_any: bool = false;

    while (cur < md.len && pos < out.len) {
        const raw_end: usize = mkdFindLineEnd(md, cur);
        const line_end: usize = mkdTrimCr(md, cur, raw_end);
        const next_cur: usize = if (raw_end < md.len) { raw_end + 1 } else { md.len };

        if (in_fence) {
            if (mkdIsFenceClose(md, cur, line_end, fence_ch, fence_len)) {
                in_fence = false;
                cur = next_cur;
                continue;
            }
            if (wrote_any) {
                mkdWriteByte(out, &pos, 10);
            }
            mkdWriteStr(out, &pos, md[cur..line_end]);
            wrote_any = true;
            cur = next_cur;
            continue;
        }

        if (mkdIsBlankLine(md, cur, line_end)) {
            cur = next_cur;
            continue;
        }

        let fc: u8 = 0;
        let ls: usize = 0;
        let le: usize = 0;
        const fl: usize = mkdMatchFenceOpen(md, cur, line_end, &fc, &ls, &le);
        if (fl > 0) {
            in_fence = true;
            fence_ch = fc;
            fence_len = fl;
            cur = next_cur;
            continue;
        }

        if (mkdIsHorizontalRule(md, cur, line_end)) {
            cur = next_cur;
            continue;
        }

        let cs: usize = cur;
        let ce: usize = line_end;

        let hs: usize = 0;
        let he: usize = 0;
        if (mkdMatchHeading(md, cur, line_end, &hs, &he) > 0) {
            cs = hs;
            ce = he;
        } else {
            // Strip leading `> ` prefixes
            let p: usize = mkdSkipIndent(md, cur, line_end, 3);
            while (p < line_end && md[p] == 62) {
                p = p + 1;
                if (p < line_end && md[p] == 32) {
                    p = p + 1;
                }
            }
            cs = p;
            const ul_s: usize = mkdMatchUnorderedItem(md, cs, line_end);
            if (ul_s > 0) {
                cs = ul_s;
            } else {
                let ord_n: usize = 0;
                const ol_s: usize = mkdMatchOrderedItem(md, cs, line_end, &ord_n);
                if (ol_s > 0) {
                    cs = ol_s;
                }
            }
        }

        if (wrote_any) {
            mkdWriteByte(out, &pos, 10);
        }
        mkdStripInlineRec(md[cs..ce], out, &pos, 0);
        wrote_any = true;
        cur = next_cur;
    }
    return pos;
}
