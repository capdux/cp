// @cap/textwrap — Zero-allocation ANSI-aware text wrapping, width, indent/dedent & truncation
// Clean-room MIT implementation inspired by npm `wrap-ansi`, `word-wrap`, `strip-indent`,
// and Rust `textwrap`.

pub struct WrapOptions {
    pub width: usize,
    pub break_words: bool,
    pub preserve_newlines: bool,

    pub static fn defaults() -> WrapOptions {
        return WrapOptions{
            width: 80,
            break_words: true,
            preserve_newlines: true,
        };
    }

    pub static fn withWidth(width: usize) -> WrapOptions {
        return WrapOptions{
            width: width,
            break_words: true,
            preserve_newlines: true,
        };
    }
}

struct DecodedChar {
    cp: u32,
    len: usize,
}

struct WrapCursor {
    pos: usize,
    col: usize,
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendSlice(out: []u8, pos: usize, s: string, start: usize, end: usize) -> usize {
    let p = pos;
    let i = start;
    while (i < end && i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    return appendSlice(out, pos, s, 0 as usize, s.len);
}

fn decodeUtf8(s: string, pos: usize) -> DecodedChar {
    const b0 = s[pos];
    if (b0 < 0x80) {
        return DecodedChar{ cp: b0 as u32, len: 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && pos + 1 < s.len) {
        const b1 = s[pos + 1];
        if ((b1 & 0xC0) == 0x80) {
            const cp: u32 = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
            return DecodedChar{ cp: cp, len: 2 };
        }
    }
    if ((b0 & 0xF0) == 0xE0 && pos + 2 < s.len) {
        const b1 = s[pos + 1];
        const b2 = s[pos + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            const cp: u32 = (((b0 & 0x0F) as u32) << 12)
                | (((b1 & 0x3F) as u32) << 6)
                | ((b2 & 0x3F) as u32);
            return DecodedChar{ cp: cp, len: 3 };
        }
    }
    if ((b0 & 0xF8) == 0xF0 && pos + 3 < s.len) {
        const b1 = s[pos + 1];
        const b2 = s[pos + 2];
        const b3 = s[pos + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            const cp: u32 = (((b0 & 0x07) as u32) << 18)
                | (((b1 & 0x3F) as u32) << 12)
                | (((b2 & 0x3F) as u32) << 6)
                | ((b3 & 0x3F) as u32);
            return DecodedChar{ cp: cp, len: 4 };
        }
    }
    return DecodedChar{ cp: b0 as u32, len: 1 };
}

/// If `s[pos]` begins an ANSI escape sequence (CSI `\x1b[...`, OSC `\x1b]...`, or 2-byte Fe/Fp),
/// returns its byte length; otherwise returns 0.
fn ansiSeqLen(s: string, pos: usize) -> usize {
    if (pos >= s.len || s[pos] != 0x1B) {
        return 0;
    }
    if (pos + 1 >= s.len) {
        return 1;
    }
    const next = s[pos + 1];
    // CSI: ESC [ <params 0x30..0x3F>* <intermediates 0x20..0x2F>* <final 0x40..0x7E>
    if (next == '[') {
        let i = pos + 2;
        while (i < s.len && s[i] >= 0x30 && s[i] <= 0x3F) {
            i += 1;
        }
        while (i < s.len && s[i] >= 0x20 && s[i] <= 0x2F) {
            i += 1;
        }
        if (i < s.len && s[i] >= 0x40 && s[i] <= 0x7E) {
            i += 1;
        }
        return i - pos;
    }
    // OSC: ESC ] ... (BEL | ESC \)
    if (next == ']') {
        let i = pos + 2;
        while (i < s.len) {
            if (s[i] == 0x07) {
                return (i + 1) - pos;
            }
            if (s[i] == 0x1B && i + 1 < s.len && s[i + 1] == '\\') {
                return (i + 2) - pos;
            }
            i += 1;
        }
        return s.len - pos;
    }
    return 2;
}

fn isAnsiReset(s: string, pos: usize, seq_len: usize) -> bool {
    // Matches `\x1b[0m` or `\x1b[m`
    if (seq_len == 4 && s[pos] == 0x1B && s[pos + 1] == '[' && s[pos + 2] == '0' && s[pos + 3] == 'm') {
        return true;
    }
    if (seq_len == 3 && s[pos] == 0x1B && s[pos + 1] == '[' && s[pos + 2] == 'm') {
        return true;
    }
    return false;
}

/// Returns terminal display column width (0, 1, or 2) for a Unicode codepoint.
fn codepointWidth(cp: u32) -> usize {
    if (cp >= 0x20 && cp <= 0x7E) {
        return 1;
    }
    if (cp < 0x20 || (cp >= 0x7F && cp <= 0x9F)) {
        return 0;
    }
    // Common zero-width and combining marks
    if ((cp >= 0x0300 && cp <= 0x036F)
        || (cp >= 0x1AB0 && cp <= 0x1AFF)
        || (cp >= 0x1DC0 && cp <= 0x1DFF)
        || (cp >= 0x200B && cp <= 0x200F)
        || (cp >= 0x202A && cp <= 0x202E)
        || (cp >= 0x20D0 && cp <= 0x20FF)
        || (cp >= 0xFE00 && cp <= 0xFE0F)
        || (cp >= 0xFE20 && cp <= 0xFE2F)
        || cp == 0xFEFF)
    {
        return 0;
    }
    // East Asian Wide / Fullwidth / CJK / Hangul / Emoji blocks (width = 2)
    if ((cp >= 0x1100 && cp <= 0x115F)
        || cp == 0x2329
        || cp == 0x232A
        || (cp >= 0x2E80 && cp <= 0x303E)
        || (cp >= 0x3040 && cp <= 0x33FF)
        || (cp >= 0x3400 && cp <= 0x4DBF)
        || (cp >= 0x4E00 && cp <= 0x9FFF)
        || (cp >= 0xA000 && cp <= 0xA4CF)
        || (cp >= 0xAC00 && cp <= 0xD7A3)
        || (cp >= 0xF900 && cp <= 0xFAFF)
        || (cp >= 0xFE10 && cp <= 0xFE19)
        || (cp >= 0xFE30 && cp <= 0xFE6F)
        || (cp >= 0xFF01 && cp <= 0xFF60)
        || (cp >= 0xFFE0 && cp <= 0xFFE6)
        || (cp >= 0x1F300 && cp <= 0x1F9FF)
        || (cp >= 0x1FA00 && cp <= 0x1FAFF)
        || (cp >= 0x20000 && cp <= 0x2FFFD)
        || (cp >= 0x30000 && cp <= 0x3FFFD))
    {
        return 2;
    }
    return 1;
}

fn isSpaceByte(c: u8) -> bool {
    return c == ' ' || c == '\t';
}

fn isReflowSpaceByte(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// ---------------------------------------------------------------------------
// Public API: Display Width & ANSI Stripping
// ---------------------------------------------------------------------------

/// Computes the visual terminal column width of `s`.
/// Skips ANSI CSI/OSC escape sequences and counts CJK / fullwidth characters as 2 columns.
pub fn displayWidth(s: string) -> usize {
    let width: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const seq = ansiSeqLen(s, i);
        if (seq > 0) {
            i += seq;
            continue;
        }
        const ch = decodeUtf8(s, i);
        width += codepointWidth(ch.cp);
        i += ch.len;
    }
    return width;
}

/// Removes all ANSI escape sequences from `s`, writing plain text into `out`.
pub fn stripAnsi(s: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const seq = ansiSeqLen(s, i);
        if (seq > 0) {
            i += seq;
            continue;
        }
        pos = appendByte(out, pos, s[i]);
        i += 1;
    }
    return out[0..pos];
}

// ---------------------------------------------------------------------------
// Internal Wrapping Helpers
// ---------------------------------------------------------------------------

fn writeHardBrokenToken(
    text: string,
    tok_start: usize,
    tok_end: usize,
    width: usize,
    out: []u8,
    start_pos: usize,
    start_col: usize,
) -> WrapCursor {
    let pos = start_pos;
    let col = start_col;
    let i = tok_start;
    while (i < tok_end) {
        const seq = ansiSeqLen(text, i);
        if (seq > 0) {
            let seq_end = i + seq;
            if (seq_end > tok_end) {
                seq_end = tok_end;
            }
            pos = appendSlice(out, pos, text, i, seq_end);
            i = seq_end;
            continue;
        }
        const ch = decodeUtf8(text, i);
        const cw = codepointWidth(ch.cp);
        if (col > 0 && col + cw > width) {
            pos = appendByte(out, pos, '\n');
            col = 0;
        }
        let ch_end = i + ch.len;
        if (ch_end > tok_end) {
            ch_end = tok_end;
        }
        pos = appendSlice(out, pos, text, i, ch_end);
        col += cw;
        i = ch_end;
    }
    return WrapCursor{ pos: pos, col: col };
}

fn wrapSegment(
    text: string,
    seg_start: usize,
    seg_end: usize,
    opts: WrapOptions,
    out: []u8,
    start_pos: usize,
) -> usize {
    let pos = start_pos;
    let col: usize = 0;
    let wrapped_line = false;
    let has_pending_ws = false;
    let ws_start: usize = 0;
    let ws_end: usize = 0;
    let ws_width: usize = 0;

    let i = seg_start;
    while (i < seg_end) {
        const b = text[i];
        const is_ws = if (opts.preserve_newlines) isSpaceByte(b) else isReflowSpaceByte(b);
        if (is_ws) {
            const run_start = i;
            let run_w: usize = 0;
            while (i < seg_end) {
                const c = text[i];
                const c_ws = if (opts.preserve_newlines) isSpaceByte(c) else isReflowSpaceByte(c);
                if (!c_ws) {
                    break;
                }
                if (c == '\t') {
                    run_w += 1;
                } else if (c == ' ') {
                    run_w += 1;
                }
                i += 1;
            }
            if (col > 0) {
                if (!has_pending_ws) {
                    has_pending_ws = true;
                    ws_start = run_start;
                    ws_end = i;
                    ws_width = if (opts.preserve_newlines) run_w else (1 as usize);
                } else if (opts.preserve_newlines) {
                    ws_end = i;
                    ws_width += run_w;
                }
            } else if (!wrapped_line && opts.preserve_newlines) {
                // Preserve leading indentation on the original first line of a paragraph
                if (run_w <= opts.width) {
                    pos = appendSlice(out, pos, text, run_start, i);
                    col += run_w;
                }
            }
            continue;
        }

        // Scan next non-whitespace token (leading ANSI + either one wide CJK char or a word)
        const tok_start = i;
        while (i < seg_end) {
            const seq = ansiSeqLen(text, i);
            if (seq > 0) {
                let next_i = i + seq;
                if (next_i > seg_end) {
                    next_i = seg_end;
                }
                i = next_i;
            } else {
                break;
            }
        }

        // Trailing ANSI sequence(s) at end of segment: emit directly without breaking
        if (i >= seg_end) {
            pos = appendSlice(out, pos, text, tok_start, seg_end);
            break;
        }

        // If ANSI was immediately followed by whitespace, emit the ANSI directly
        const b_after_ansi = text[i];
        const after_is_ws = if (opts.preserve_newlines) isSpaceByte(b_after_ansi) else isReflowSpaceByte(b_after_ansi);
        if (after_is_ws) {
            pos = appendSlice(out, pos, text, tok_start, i);
            continue;
        }

        let tok_w: usize = 0;
        const first_ch = decodeUtf8(text, i);
        const first_w = codepointWidth(first_ch.cp);
        if (first_w == 2) {
            // CJK / fullwidth character: breakable at any character boundary
            tok_w = 2;
            i += first_ch.len;
            if (i > seg_end) {
                i = seg_end;
            }
            // Attach any immediately following zero-width combining marks
            while (i < seg_end) {
                if (ansiSeqLen(text, i) > 0) {
                    break;
                }
                const comb = decodeUtf8(text, i);
                if (codepointWidth(comb.cp) == 0 && comb.cp >= 0x0300) {
                    i += comb.len;
                    if (i > seg_end) {
                        i = seg_end;
                    }
                } else {
                    break;
                }
            }
        } else {
            // Regular word: consume until whitespace or wide CJK character
            while (i < seg_end) {
                const seq = ansiSeqLen(text, i);
                if (seq > 0) {
                    let next_i = i + seq;
                    if (next_i > seg_end) {
                        next_i = seg_end;
                    }
                    i = next_i;
                    continue;
                }
                const cur_b = text[i];
                const cur_ws = if (opts.preserve_newlines) isSpaceByte(cur_b) else isReflowSpaceByte(cur_b);
                if (cur_ws) {
                    break;
                }
                const ch = decodeUtf8(text, i);
                const cw = codepointWidth(ch.cp);
                if (cw == 2) {
                    break;
                }
                tok_w += cw;
                i += ch.len;
                if (i > seg_end) {
                    i = seg_end;
                }
            }
        }

        const tok_end = i;
        const space_w: usize = if (has_pending_ws) ws_width else (0 as usize);

        if (col > 0) {
            if (col + space_w + tok_w <= opts.width) {
                if (has_pending_ws) {
                    if (opts.preserve_newlines) {
                        pos = appendSlice(out, pos, text, ws_start, ws_end);
                    } else {
                        pos = appendByte(out, pos, ' ');
                    }
                    col += space_w;
                    has_pending_ws = false;
                }
                pos = appendSlice(out, pos, text, tok_start, tok_end);
                col += tok_w;
            } else {
                // Wrap to next line, discarding pending trailing whitespace
                has_pending_ws = false;
                pos = appendByte(out, pos, '\n');
                col = 0;
                wrapped_line = true;
                if (tok_w <= opts.width || !opts.break_words) {
                    pos = appendSlice(out, pos, text, tok_start, tok_end);
                    col = tok_w;
                } else {
                    const cur = writeHardBrokenToken(text, tok_start, tok_end, opts.width, out, pos, col);
                    pos = cur.pos;
                    col = cur.col;
                }
            }
        } else {
            has_pending_ws = false;
            if (tok_w <= opts.width || !opts.break_words) {
                pos = appendSlice(out, pos, text, tok_start, tok_end);
                col = tok_w;
            } else {
                const cur = writeHardBrokenToken(text, tok_start, tok_end, opts.width, out, pos, col);
                pos = cur.pos;
                col = cur.col;
            }
        }
    }

    return pos;
}

// ---------------------------------------------------------------------------
// Public API: Text Wrapping
// ---------------------------------------------------------------------------

/// Wraps `text` to `opts.width` display columns according to `opts`, writing into `out`.
pub fn wrapTextWith(text: string, opts: WrapOptions, out: []u8) -> string {
    if (text.len == 0) {
        return out[0..0];
    }
    if (opts.width == 0) {
        const n = appendStr(out, 0 as usize, text);
        return out[0..n];
    }
    if (!opts.preserve_newlines) {
        const n = wrapSegment(text, 0 as usize, text.len, opts, out, 0 as usize);
        return out[0..n];
    }

    let pos: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= text.len) {
        if (i == text.len || text[i] == '\n') {
            let line_end = i;
            if (line_end > line_start && text[line_end - 1] == '\r') {
                line_end -= 1;
            }
            pos = wrapSegment(text, line_start, line_end, opts, out, pos);
            if (i < text.len && text[i] == '\n') {
                pos = appendByte(out, pos, '\n');
            }
            line_start = i + 1;
        }
        i += 1;
    }
    return out[0..pos];
}

/// Wraps `text` to `width` display columns with default options (`break_words = true`, `preserve_newlines = true`).
pub fn wrapText(text: string, width: usize, out: []u8) -> string {
    return wrapTextWith(text, WrapOptions.withWidth(width), out);
}

// ---------------------------------------------------------------------------
// Public API: Indent & Dedent
// ---------------------------------------------------------------------------

fn lineHasContent(text: string, start: usize, end: usize) -> bool {
    let i = start;
    while (i < end) {
        const c = text[i];
        if (c != ' ' && c != '\t' && c != '\r') {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Prepends `prefix` to every non-blank line in `text`, writing into `out`.
pub fn indent(text: string, prefix: string, out: []u8) -> string {
    if (text.len == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= text.len) {
        if (i == text.len || text[i] == '\n') {
            if (lineHasContent(text, line_start, i)) {
                pos = appendStr(out, pos, prefix);
            }
            pos = appendSlice(out, pos, text, line_start, i);
            if (i < text.len && text[i] == '\n') {
                pos = appendByte(out, pos, '\n');
            }
            line_start = i + 1;
        }
        i += 1;
    }
    return out[0..pos];
}

/// Removes any common leading whitespace (spaces/tabs) from all non-blank lines in `text`.
/// Mirrors Python `textwrap.dedent` and npm `strip-indent`.
pub fn dedent(text: string, out: []u8) -> string {
    if (text.len == 0) {
        return out[0..0];
    }

    // Pass 1: compute minimum leading whitespace across all non-blank lines
    let has_content_line = false;
    let min_indent: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= text.len) {
        if (i == text.len || text[i] == '\n') {
            if (lineHasContent(text, line_start, i)) {
                let ws: usize = 0;
                while (line_start + ws < i && (text[line_start + ws] == ' ' || text[line_start + ws] == '\t')) {
                    ws += 1;
                }
                if (!has_content_line || ws < min_indent) {
                    min_indent = ws;
                    has_content_line = true;
                }
            }
            line_start = i + 1;
        }
        i += 1;
    }

    // Pass 2: strip `min_indent` leading whitespace bytes from non-blank lines
    let pos: usize = 0;
    let line_start2: usize = 0;
    let j: usize = 0;
    while (j <= text.len) {
        if (j == text.len || text[j] == '\n') {
            if (lineHasContent(text, line_start2, j)) {
                let skip: usize = min_indent;
                const line_len: usize = j - line_start2;
                if (skip > line_len) {
                    skip = line_len;
                }
                pos = appendSlice(out, pos, text, line_start2 + skip, j);
            }
            if (j < text.len && text[j] == '\n') {
                pos = appendByte(out, pos, '\n');
            }
            line_start2 = j + 1;
        }
        j += 1;
    }
    return out[0..pos];
}

// ---------------------------------------------------------------------------
// Public API: Display-Width Truncation
// ---------------------------------------------------------------------------

/// Truncates `text` to fit within `max_width` visual columns, appending `ellipsis` if truncated.
/// Preserves ANSI escape sequences and safely resets active ANSI styling before appending `ellipsis`.
pub fn truncate(text: string, max_width: usize, ellipsis: string, out: []u8) -> string {
    if (displayWidth(text) <= max_width) {
        const n = appendStr(out, 0 as usize, text);
        return out[0..n];
    }
    if (max_width == 0) {
        return out[0..0];
    }

    const ellipsis_w = displayWidth(ellipsis);
    let pos: usize = 0;
    if (ellipsis_w >= max_width) {
        // Fit as much of `ellipsis` itself as possible within `max_width`
        let col: usize = 0;
        let i: usize = 0;
        while (i < ellipsis.len) {
            const seq = ansiSeqLen(ellipsis, i);
            if (seq > 0) {
                pos = appendSlice(out, pos, ellipsis, i, i + seq);
                i += seq;
                continue;
            }
            const ch = decodeUtf8(ellipsis, i);
            const cw = codepointWidth(ch.cp);
            if (col + cw > max_width) {
                break;
            }
            pos = appendSlice(out, pos, ellipsis, i, i + ch.len);
            col += cw;
            i += ch.len;
        }
        return out[0..pos];
    }

    const budget = max_width - ellipsis_w;
    let col: usize = 0;
    let active_ansi = false;
    let i: usize = 0;
    while (i < text.len) {
        const seq = ansiSeqLen(text, i);
        if (seq > 0) {
            pos = appendSlice(out, pos, text, i, i + seq);
            active_ansi = !isAnsiReset(text, i, seq);
            i += seq;
            continue;
        }
        const ch = decodeUtf8(text, i);
        const cw = codepointWidth(ch.cp);
        if (col + cw > budget) {
            break;
        }
        pos = appendSlice(out, pos, text, i, i + ch.len);
        col += cw;
        i += ch.len;
    }

    if (active_ansi) {
        pos = appendStr(out, pos, "\x1b[0m");
    }
    pos = appendStr(out, pos, ellipsis);
    return out[0..pos];
}
