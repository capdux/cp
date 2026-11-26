// packages/@cap/textwrap-crate/src/index.cp
// Zero-allocation smart word wrapping, indentation, dedentation, and unfilling engine.
// Clean-room MIT parity with crates.io `textwrap` v0.16.
// Composes `@cap/unicode-width` for Unicode 15.1 / CJK / Emoji terminal display column width.

import {
    strWidth,
} from "../../unicode-width/src/index.cp";

fn twpIsInlineSpace(b: u8): bool {
    return b == (0x20 as u8) || b == (0x09 as u8);
}

fn twpWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn twpWriteSlice(out: []u8, pos: usize, src: string): usize {
    let cur = pos;
    let i: usize = 0;
    while (i < src.len && cur < out.len) {
        out[cur] = src[i];
        cur += 1;
        i += 1;
    }
    return cur;
}

fn twpLineContentEnd(text: string, start: usize, nl_pos: usize): usize {
    if (nl_pos > start && text[nl_pos - 1] == (0x0D as u8)) {
        return nl_pos - 1;
    }
    return nl_pos;
}

fn twpFindNewline(text: string, start: usize): usize {
    let i = start;
    while (i < text.len) {
        if (text[i] == (0x0A as u8)) {
            return i;
        }
        i += 1;
    }
    return text.len;
}

fn twpSkipInlineSpaces(text: string, start: usize, end: usize): usize {
    let i = start;
    while (i < end && twpIsInlineSpace(text[i])) {
        i += 1;
    }
    return i;
}

fn twpTrimRightInlineSpaces(text: string, start: usize, end: usize): usize {
    let i = end;
    while (i > start && twpIsInlineSpace(text[i - 1])) {
        i -= 1;
    }
    return i;
}

fn twpIsBlankLine(text: string, start: usize, end: usize): bool {
    return twpSkipInlineSpaces(text, start, end) >= end;
}

/// Compute terminal display column width of a line using `@cap/unicode-width`.
pub fn lineDisplayWidth(line: string): usize {
    return strWidth(line);
}

/// Wrap `text` to at most `width` display columns per line, prepending `initial_indent`
/// to the first line and `subsequent_indent` to all subsequent lines.
/// Preserves explicit line breaks and empty paragraph lines. Writes into `out` and
/// returns the number of bytes written.
pub fn fillWithIndent(
    text: string,
    width: usize,
    initial_indent: string,
    subsequent_indent: string,
    out: []u8,
): usize {
    if (text.len == 0) {
        return 0;
    }

    const init_w = strWidth(initial_indent);
    const sub_w = strWidth(subsequent_indent);

    let out_pos: usize = 0;
    let pos: usize = 0;
    let is_first_output_line = true;
    let is_first_input_line = true;

    while (pos <= text.len) {
        if (pos == text.len) {
            if (!is_first_input_line && text[text.len - 1] == (0x0A as u8)) {
                out_pos = twpWriteByte(out, out_pos, 0x0A as u8);
            }
            break;
        }

        const nl = twpFindNewline(text, pos);
        const line_end = twpLineContentEnd(text, pos, nl);

        if (!is_first_input_line) {
            out_pos = twpWriteByte(out, out_pos, 0x0A as u8);
        }
        is_first_input_line = false;

        const trimmed_end = twpTrimRightInlineSpaces(text, pos, line_end);
        let cur = twpSkipInlineSpaces(text, pos, trimmed_end);

        if (cur >= trimmed_end) {
            // Blank line: preserve empty line without trailing whitespace.
            is_first_output_line = false;
        } else {
            let col: usize = 0;
            if (is_first_output_line) {
                out_pos = twpWriteSlice(out, out_pos, initial_indent);
                col = init_w;
            } else {
                out_pos = twpWriteSlice(out, out_pos, subsequent_indent);
                col = sub_w;
            }
            is_first_output_line = false;

            let words_on_line: usize = 0;
            while (cur < trimmed_end) {
                const word_start = cur;
                while (cur < trimmed_end && !twpIsInlineSpace(text[cur])) {
                    cur += 1;
                }
                const word = text[word_start..cur];
                const word_w = strWidth(word);

                if (words_on_line == 0) {
                    out_pos = twpWriteSlice(out, out_pos, word);
                    col += word_w;
                    words_on_line = 1;
                } else if (col + 1 + word_w <= width) {
                    out_pos = twpWriteByte(out, out_pos, 0x20 as u8);
                    out_pos = twpWriteSlice(out, out_pos, word);
                    col += 1 + word_w;
                    words_on_line += 1;
                } else {
                    out_pos = twpWriteByte(out, out_pos, 0x0A as u8);
                    out_pos = twpWriteSlice(out, out_pos, subsequent_indent);
                    out_pos = twpWriteSlice(out, out_pos, word);
                    col = sub_w + word_w;
                    words_on_line = 1;
                }

                cur = twpSkipInlineSpaces(text, cur, trimmed_end);
            }
        }

        if (nl >= text.len) {
            break;
        }
        pos = nl + 1;
    }

    return out_pos;
}

/// Wrap `text` to at most `width` display columns per line at word boundaries,
/// preserving paragraph blank lines. Writes into `out` and returns bytes written.
pub fn fill(text: string, width: usize, out: []u8): usize {
    return fillWithIndent(text, width, "", "", out);
}

/// Prepend `prefix` to every non-empty line in `text`, skipping blank lines.
/// Preserves original line endings. Writes into `out` and returns bytes written.
pub fn indent(text: string, prefix: string, out: []u8): usize {
    if (text.len == 0) {
        return 0;
    }

    let out_pos: usize = 0;
    let pos: usize = 0;

    while (pos < text.len) {
        const nl = twpFindNewline(text, pos);
        const line_end = twpLineContentEnd(text, pos, nl);

        if (!twpIsBlankLine(text, pos, line_end)) {
            out_pos = twpWriteSlice(out, out_pos, prefix);
        }

        out_pos = twpWriteSlice(out, out_pos, text[pos..line_end]);

        if (nl < text.len) {
            if (line_end < nl) {
                out_pos = twpWriteByte(out, out_pos, 0x0D as u8);
            }
            out_pos = twpWriteByte(out, out_pos, 0x0A as u8);
            pos = nl + 1;
        } else {
            break;
        }
    }

    return out_pos;
}

/// Remove common leading whitespace from every non-blank line in `text`
/// (parity with Python `textwrap.dedent` and Rust `textwrap::dedent`).
/// Writes into `out` and returns bytes written.
pub fn dedent(text: string, out: []u8): usize {
    if (text.len == 0) {
        return 0;
    }

    // Pass 1: find longest common leading whitespace byte prefix across all non-blank lines.
    let has_prefix = false;
    let prefix_start: usize = 0;
    let prefix_len: usize = 0;

    let pos: usize = 0;
    while (pos < text.len) {
        const nl = twpFindNewline(text, pos);
        const line_end = twpLineContentEnd(text, pos, nl);
        const content_start = twpSkipInlineSpaces(text, pos, line_end);

        if (content_start < line_end) {
            const ws_len = content_start - pos;
            if (!has_prefix) {
                has_prefix = true;
                prefix_start = pos;
                prefix_len = ws_len;
            } else {
                let common: usize = 0;
                const limit = if (prefix_len < ws_len) prefix_len else ws_len;
                while (common < limit && text[prefix_start + common] == text[pos + common]) {
                    common += 1;
                }
                prefix_len = common;
            }
        }

        if (nl < text.len) {
            pos = nl + 1;
        } else {
            break;
        }
    }

    // Pass 2: strip common prefix from non-blank lines, normalize whitespace-only lines to empty.
    let out_pos: usize = 0;
    pos = 0;
    while (pos < text.len) {
        const nl = twpFindNewline(text, pos);
        const line_end = twpLineContentEnd(text, pos, nl);

        if (!twpIsBlankLine(text, pos, line_end)) {
            const strip = if (has_prefix && prefix_len <= line_end - pos) prefix_len else 0;
            out_pos = twpWriteSlice(out, out_pos, text[(pos + strip)..line_end]);
        }

        if (nl < text.len) {
            if (line_end < nl) {
                out_pos = twpWriteByte(out, out_pos, 0x0D as u8);
            }
            out_pos = twpWriteByte(out, out_pos, 0x0A as u8);
            pos = nl + 1;
        } else {
            break;
        }
    }

    return out_pos;
}

/// Reflow soft-wrapped paragraphs back into single lines while preserving
/// blank-line (`\n\n`) paragraph boundaries. Writes into `out` and returns bytes written.
pub fn unfill(text: string, out: []u8): usize {
    if (text.len == 0) {
        return 0;
    }

    let out_pos: usize = 0;
    let pos: usize = 0;
    let in_paragraph = false;
    let pending_para_break = false;

    while (pos < text.len) {
        const nl = twpFindNewline(text, pos);
        const line_end = twpLineContentEnd(text, pos, nl);

        const trimmed_start = twpSkipInlineSpaces(text, pos, line_end);
        const trimmed_end = twpTrimRightInlineSpaces(text, trimmed_start, line_end);

        if (trimmed_start >= trimmed_end) {
            if (in_paragraph) {
                pending_para_break = true;
            }
        } else {
            if (pending_para_break) {
                out_pos = twpWriteByte(out, out_pos, 0x0A as u8);
                out_pos = twpWriteByte(out, out_pos, 0x0A as u8);
                pending_para_break = false;
            } else if (in_paragraph) {
                out_pos = twpWriteByte(out, out_pos, 0x20 as u8);
            }
            out_pos = twpWriteSlice(out, out_pos, text[trimmed_start..trimmed_end]);
            in_paragraph = true;
        }

        if (nl < text.len) {
            pos = nl + 1;
        } else {
            break;
        }
    }

    return out_pos;
}

export {
    fill,
    fillWithIndent,
    indent,
    dedent,
    unfill,
    lineDisplayWidth,
};
