// @cap/wrap-ansi — Zero-allocation ANSI-preserving text wrapper & column slicer for CP
// Clean-room MIT implementation inspired by npm `wrap-ansi` (615M/wk) and `slice-ansi` (550M/wk).
//
// Features:
// - `visibleWidth`: accurate terminal column width (ANSI CSI sequences = 0, CJK/fullwidth/emoji = 2, ASCII = 1).
// - `wrapAnsi`: word-boundary or hard character-boundary wrapping that tracks active ANSI SGR styles,
//   closes them with `\x1b[0m` before every line break, and re-injects them on the next line.
// - `sliceAnsi`: slices by visible column range `[start_col, end_col)` while preserving active ANSI SGR
//   styles and closing them cleanly with `\x1b[0m`.
// - Zero heap allocation: writes directly into caller-provided `out: []u8`.

const MAX_ACTIVE_SGR: usize = 8;

struct Utf8Step {
    cp: u32,
    width: usize,
}

fn decodeUtf8At(s: string, i: usize) -> Utf8Step {
    const b0 = s[i];
    if (b0 < 0x80) {
        return Utf8Step{ cp: b0 as u32, width: 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && i + 1 < s.len) {
        const b1 = s[i + 1];
        if ((b1 & 0xC0) == 0x80) {
            return Utf8Step{
                cp: (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32),
                width: 2,
            };
        }
    } else if ((b0 & 0xF0) == 0xE0 && i + 2 < s.len) {
        const b1 = s[i + 1];
        const b2 = s[i + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            return Utf8Step{
                cp: (((b0 & 0x0F) as u32) << 12) | (((b1 & 0x3F) as u32) << 6) | ((b2 & 0x3F) as u32),
                width: 3,
            };
        }
    } else if ((b0 & 0xF8) == 0xF0 && i + 3 < s.len) {
        const b1 = s[i + 1];
        const b2 = s[i + 2];
        const b3 = s[i + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            return Utf8Step{
                cp: (((b0 & 0x07) as u32) << 18) | (((b1 & 0x3F) as u32) << 12) | (((b2 & 0x3F) as u32) << 6) | ((b3 & 0x3F) as u32),
                width: 4,
            };
        }
    }
    return Utf8Step{ cp: 0xFFFD, width: 1 };
}

fn codepointWidth(cp: u32) -> usize {
    if (cp < 0x20 || cp == 0x7F) {
        return 0;
    }
    if (cp >= 0x0300 && cp <= 0x036F) {
        return 0;
    }
    if ((cp >= 0x1100 && cp <= 0x115F) ||
        (cp >= 0x2E80 && cp <= 0x303E) ||
        (cp >= 0x3041 && cp <= 0x33FF) ||
        (cp >= 0x3400 && cp <= 0x4DBF) ||
        (cp >= 0x4E00 && cp <= 0x9FFF) ||
        (cp >= 0xA000 && cp <= 0xA4CF) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) ||
        (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0xFE10 && cp <= 0xFE6F) ||
        (cp >= 0xFF01 && cp <= 0xFF60) ||
        (cp >= 0xFFE0 && cp <= 0xFFE6) ||
        (cp >= 0x1F300 && cp <= 0x1F9FF) ||
        (cp >= 0x1FA70 && cp <= 0x1FAFF) ||
        (cp >= 0x20000 && cp <= 0x3FFFF)) {
        return 2;
    }
    return 1;
}

/// Returns the end index of an ANSI CSI sequence starting at `i` (`\x1b[...`), or `i` if none.
fn scanAnsiCsi(s: string, i: usize) -> usize {
    if (i + 1 >= s.len || s[i] != 0x1B || s[i + 1] != '[') {
        return i;
    }
    let j = i + 2;
    while (j < s.len) {
        const c = s[j];
        j += 1;
        if (c >= 0x40 && c <= 0x7E) {
            return j;
        }
    }
    return j;
}

fn isResetSgr(seq: string) -> bool {
    if (seq.len < 3 || seq[0] != 0x1B || seq[1] != '[' || seq[seq.len - 1] != 'm') {
        return false;
    }
    // `\x1b[m`, `\x1b[0m`, `\x1b[00m`
    if (seq.len == 3) {
        return true;
    }
    let i: usize = 2;
    while (i + 1 < seq.len) {
        if (seq[i] != '0') {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWithZeroReset(seq: string) -> bool {
    // e.g. `\x1b[0;31m`
    return seq.len >= 5 && seq[0] == 0x1B && seq[1] == '[' && seq[2] == '0' && seq[3] == ';' && seq[seq.len - 1] == 'm';
}

fn pushByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pushStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

struct AnsiStyleState {
    seqs: [MAX_ACTIVE_SGR]string,
    count: usize,

    static fn empty() -> AnsiStyleState {
        let arr: [MAX_ACTIVE_SGR]string = undefined;
        return AnsiStyleState{ seqs: arr, count: 0 };
    }

    fn hasActive() -> bool {
        return _.count > 0;
    }

    fn applySeq(*_, seq: string) {
        if (seq.len < 3 || seq[seq.len - 1] != 'm') {
            return;
        }
        if (isResetSgr(seq)) {
            _.count = 0;
            return;
        }
        if (startsWithZeroReset(seq)) {
            _.count = 0;
        }
        if (_.count < MAX_ACTIVE_SGR) {
            _.seqs[_.count] = seq;
            _.count += 1;
        }
    }

    fn emitActive(out: []u8, pos: usize) -> usize {
        let p = pos;
        let i: usize = 0;
        while (i < _.count) {
            p = pushStr(out, p, _.seqs[i]);
            i += 1;
        }
        return p;
    }
}

/// Computes visible terminal column width, ignoring ANSI CSI sequences (`\x1b[...m`)
/// and counting CJK / fullwidth / emoji codepoints as width 2.
pub fn visibleWidth(s: string) -> usize {
    let w: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const ansi_end = scanAnsiCsi(s, i);
        if (ansi_end > i) {
            i = ansi_end;
            continue;
        }
        const step = decodeUtf8At(s, i);
        w += codepointWidth(step.cp);
        i += step.width;
    }
    return w;
}

fn emitLineBreak(out: []u8, pos: usize, style: AnsiStyleState) -> usize {
    let p = pos;
    if (style.hasActive()) {
        p = pushStr(out, p, "\x1b[0m");
    }
    p = pushByte(out, p, '\n');
    if (style.hasActive()) {
        p = style.emitActive(out, p);
    }
    return p;
}

/// Wraps `text` to `max_cols` visible columns into caller-provided `out` (zero heap allocation).
/// Tracks active ANSI SGR styles across line breaks: closes with `\x1b[0m` before `\n` and
/// re-injects active styles at the start of the wrapped line.
pub fn wrapAnsi(text: string, max_cols: usize, hard_wrap: bool, out: []u8) -> string {
    if (text.len == 0) {
        return out[0..0];
    }
    if (max_cols == 0) {
        const end = pushStr(out, 0, text);
        return out[0..end];
    }

    let pos: usize = 0;
    let style = AnsiStyleState.empty();
    let cur: usize = 0;

    while (cur < text.len) {
        const ls = cur;
        while (cur < text.len && text[cur] != '\n') {
            cur += 1;
        }
        let le = cur;
        if (le > ls && text[le - 1] == '\r') {
            le -= 1;
        }
        const has_next_line = (cur < text.len && text[cur] == '\n');
        if (has_next_line) {
            cur += 1;
        }

        const line = text[ls..le];
        let i: usize = 0;
        let col: usize = 0;
        let just_wrapped = false;

        while (i < line.len) {
            // 1. ANSI escape sequence
            const ansi_end = scanAnsiCsi(line, i);
            if (ansi_end > i) {
                const seq = line[i..ansi_end];
                pos = pushStr(out, pos, seq);
                style.applySeq(seq);
                i = ansi_end;
                continue;
            }

            // 2. Space separator
            if (line[i] == ' ' || line[i] == '\t') {
                i += 1;
                if (just_wrapped && col == 0) {
                    continue;
                }
                if (col + 1 <= max_cols) {
                    pos = pushByte(out, pos, ' ');
                    col += 1;
                } else {
                    pos = emitLineBreak(out, pos, style);
                    col = 0;
                    just_wrapped = true;
                }
                continue;
            }

            // 3. Scan whole word (may contain embedded ANSI escape sequences)
            const w_start = i;
            let w_cols: usize = 0;
            while (i < line.len) {
                const ae = scanAnsiCsi(line, i);
                if (ae > i) {
                    i = ae;
                    continue;
                }
                if (line[i] == ' ' || line[i] == '\t') {
                    break;
                }
                const st = decodeUtf8At(line, i);
                w_cols += codepointWidth(st.cp);
                i += st.width;
            }
            const word = line[w_start..i];
            just_wrapped = false;

            if (col + w_cols <= max_cols) {
                // Fits on current line
                let k: usize = 0;
                while (k < word.len) {
                    const ae = scanAnsiCsi(word, k);
                    if (ae > k) {
                        const seq = word[k..ae];
                        pos = pushStr(out, pos, seq);
                        style.applySeq(seq);
                        k = ae;
                        continue;
                    }
                    const st = decodeUtf8At(word, k);
                    pos = pushStr(out, pos, word[k..(k + st.width)]);
                    k += st.width;
                }
                col += w_cols;
            } else if (w_cols <= max_cols || !hard_wrap) {
                // Move whole word onto next line (trim trailing space on previous line)
                if (col > 0) {
                    while (pos > 0 && out[pos - 1] == ' ') {
                        pos -= 1;
                    }
                    pos = emitLineBreak(out, pos, style);
                    col = 0;
                }
                let k: usize = 0;
                while (k < word.len) {
                    const ae = scanAnsiCsi(word, k);
                    if (ae > k) {
                        const seq = word[k..ae];
                        pos = pushStr(out, pos, seq);
                        style.applySeq(seq);
                        k = ae;
                        continue;
                    }
                    const st = decodeUtf8At(word, k);
                    pos = pushStr(out, pos, word[k..(k + st.width)]);
                    k += st.width;
                }
                col = w_cols;
            } else {
                // hard_wrap == true and word itself exceeds max_cols: break at character boundary
                let k: usize = 0;
                while (k < word.len) {
                    const ae = scanAnsiCsi(word, k);
                    if (ae > k) {
                        const seq = word[k..ae];
                        pos = pushStr(out, pos, seq);
                        style.applySeq(seq);
                        k = ae;
                        continue;
                    }
                    const st = decodeUtf8At(word, k);
                    const cw = codepointWidth(st.cp);
                    if (col > 0 && col + cw > max_cols) {
                        while (pos > 0 && out[pos - 1] == ' ') {
                            pos -= 1;
                        }
                        pos = emitLineBreak(out, pos, style);
                        col = 0;
                    }
                    pos = pushStr(out, pos, word[k..(k + st.width)]);
                    col += cw;
                    k += st.width;
                }
            }
        }

        if (has_next_line) {
            pos = emitLineBreak(out, pos, style);
        }
    }

    return out[0..pos];
}

/// Slices `text` by visible terminal column range `[start_col, end_col)` into `out` (zero heap allocation).
/// Preserves active ANSI SGR styles entering the slice and closes them with `\x1b[0m` if still active at end.
pub fn sliceAnsi(text: string, start_col: usize, end_col: usize, out: []u8) -> string {
    if (start_col >= end_col || text.len == 0) {
        return out[0..0];
    }

    let pos: usize = 0;
    let col: usize = 0;
    let style = AnsiStyleState.empty();
    let started = false;
    let i: usize = 0;

    while (i < text.len) {
        const ansi_end = scanAnsiCsi(text, i);
        if (ansi_end > i) {
            const seq = text[i..ansi_end];
            style.applySeq(seq);
            if (started) {
                pos = pushStr(out, pos, seq);
            }
            i = ansi_end;
            continue;
        }

        if (col >= end_col) {
            break;
        }

        const st = decodeUtf8At(text, i);
        const cw = codepointWidth(st.cp);
        if (col >= start_col && col + cw <= end_col) {
            if (!started) {
                started = true;
                if (style.hasActive()) {
                    pos = style.emitActive(out, pos);
                }
            }
            pos = pushStr(out, pos, text[i..(i + st.width)]);
        }
        col += cw;
        i += st.width;
    }

    if (started && style.hasActive()) {
        pos = pushStr(out, pos, "\x1b[0m");
    }

    return out[0..pos];
}
