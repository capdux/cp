// @cap/cli-truncate — Zero-allocation ANSI-aware CLI string truncator
// Clean-room MIT implementation in pure CP modeled after npm `cli-truncate` v4.
// Reuses @cap/string-width for visual column width calculation and
// @cap/slice-ansi for ANSI SGR-preserving column slicing & auto-reset.

import { stringWidth } from "../../string-width/src/index.cp";
import { sliceAnsi, sliceAnsiFrom } from "../../slice-ansi/src/index.cp";

/// Position where truncation and ellipsis insertion occur.
pub enum TruncatePosition: u8 {
    END = 0,
    START = 1,
    MIDDLE = 2,
}

/// Configuration options for `cliTruncateInto`.
pub struct TruncateOptions {
    position: TruncatePosition = TruncatePosition.END,
    prefer_truncation_on_space: bool = false,
    ellipsis: string = "...",

    /// Default options: truncate at `END`, space preference disabled, ellipsis `"..."`.
    pub static fn defaults(): TruncateOptions {
        return TruncateOptions{
            position: TruncatePosition.END,
            prefer_truncation_on_space: false,
            ellipsis: "...",
        };
    }
}

fn ctWriteBytes(out: []u8, pos: usize, src: string): usize {
    let w: usize = pos;
    let i: usize = 0;
    while (i < src.len && w < out.len) {
        out[w] = src[i];
        w += 1;
        i += 1;
    }
    return w;
}

/// Skip an ANSI escape sequence starting at `text[pos]` (where `text[pos] == 27`).
/// Returns the byte offset immediately after the sequence.
fn ctSkipAnsi(text: string, pos: usize): usize {
    if (pos + 1 >= text.len) {
        return pos + 1;
    }
    const next: u8 = text[pos + 1];
    // CSI: ESC [ ... <0x40..0x7E>
    if (next == 91) {
        let p: usize = pos + 2;
        while (p < text.len) {
            const b: u8 = text[p];
            p += 1;
            if (b >= 64 && b <= 126) {
                return p;
            }
        }
        return text.len;
    }
    // OSC: ESC ] ... (BEL | ESC \)
    if (next == 93) {
        let p: usize = pos + 2;
        while (p < text.len) {
            if (text[p] == 7) {
                return p + 1;
            }
            if (text[p] == 27 && p + 1 < text.len && text[p + 1] == 92) {
                return p + 2;
            }
            p += 1;
        }
        return text.len;
    }
    // Two-byte ESC sequence
    return pos + 2;
}

/// Return the UTF-8 byte length of the character starting at `text[pos]`.
fn ctUtf8Len(text: string, pos: usize): usize {
    const b0: u8 = text[pos];
    if (b0 < (0x80 as u8)) {
        return 1;
    }
    if ((b0 & (0xE0 as u8)) == (0xC0 as u8) && pos + 1 < text.len) {
        return 2;
    }
    if ((b0 & (0xF0 as u8)) == (0xE0 as u8) && pos + 2 < text.len) {
        return 3;
    }
    if ((b0 & (0xF8 as u8)) == (0xF0 as u8) && pos + 3 < text.len) {
        return 4;
    }
    return 1;
}

/// Find the column index of the last ASCII space (`' '`) at column `c` with `0 < c <= max_col`.
/// Returns `0` if no such space exists.
fn ctFindLastSpaceLe(text: string, max_col: usize): usize {
    let p: usize = 0;
    let col: usize = 0;
    let best: usize = 0;
    while (p < text.len && col <= max_col) {
        if (text[p] == 27) {
            p = ctSkipAnsi(text, p);
            continue;
        }
        const blen: usize = ctUtf8Len(text, p);
        let cw: usize = 0;
        if (blen == 1) {
            const b: u8 = text[p];
            if (b == 32 && col > 0 && col <= max_col) {
                best = col;
            }
            if (b >= 32 && b < 127) {
                cw = 1;
            }
        } else {
            cw = stringWidth(text[p..p + blen]);
        }
        col += cw;
        p += blen;
    }
    return best;
}

/// Check whether the visible character at column `target_col` is an ASCII space (`' '`).
fn ctIsSpaceAtCol(text: string, target_col: usize): bool {
    let p: usize = 0;
    let col: usize = 0;
    while (p < text.len && col <= target_col) {
        if (text[p] == 27) {
            p = ctSkipAnsi(text, p);
            continue;
        }
        const blen: usize = ctUtf8Len(text, p);
        let cw: usize = 0;
        if (blen == 1) {
            const b: u8 = text[p];
            if (col == target_col && b == 32) {
                return true;
            }
            if (b >= 32 && b < 127) {
                cw = 1;
            }
        } else {
            cw = stringWidth(text[p..p + blen]);
        }
        col += cw;
        p += blen;
    }
    return false;
}

/// Find the column index of the first ASCII space (`' '`) at column `c` with `min_col <= c < limit_col`.
/// Returns `limit_col` if no such space exists.
fn ctFindFirstSpaceGe(text: string, min_col: usize, limit_col: usize): usize {
    let p: usize = 0;
    let col: usize = 0;
    while (p < text.len && col < limit_col) {
        if (text[p] == 27) {
            p = ctSkipAnsi(text, p);
            continue;
        }
        const blen: usize = ctUtf8Len(text, p);
        let cw: usize = 0;
        if (blen == 1) {
            const b: u8 = text[p];
            if (b == 32 && col >= min_col) {
                return col;
            }
            if (b >= 32 && b < 127) {
                cw = 1;
            }
        } else {
            cw = stringWidth(text[p..p + blen]);
        }
        col += cw;
        p += blen;
    }
    return limit_col;
}

/// Truncate `text` to at most `columns` visible terminal display columns into `out`.
///
/// - When `columns == 0`, returns `0`.
/// - When `stringWidth(text) <= columns`, copies `text` unchanged into `out`.
/// - Supports `END`, `START`, and `MIDDLE` truncation positions.
/// - Preserves active ANSI SGR styles and automatically closes/resets them around the ellipsis.
/// - When `opts.prefer_truncation_on_space` is `true`, prefers breaking on nearby space boundaries.
/// - Returns the number of bytes written into `out`.
pub fn cliTruncateInto(text: string, columns: usize, opts: TruncateOptions, out: []u8): usize {
    if (columns == 0 || out.len == 0 || text.len == 0) {
        return 0;
    }

    const total_width: usize = stringWidth(text);
    if (total_width <= columns) {
        return ctWriteBytes(out, 0, text);
    }

    const ell: string = opts.ellipsis;
    const ell_width: usize = stringWidth(ell);

    if (columns <= ell_width) {
        if (columns == ell_width) {
            return ctWriteBytes(out, 0, ell);
        }
        return sliceAnsi(ell, 0, columns, out);
    }

    const available: usize = columns - ell_width;

    if (opts.position == TruncatePosition.END) {
        let cut_end: usize = available;
        if (opts.prefer_truncation_on_space) {
            const sp: usize = ctFindLastSpaceLe(text, available);
            if (sp > 0) {
                cut_end = sp;
            }
        }
        let w: usize = sliceAnsi(text, 0, cut_end, out);
        w = ctWriteBytes(out, w, ell);
        return w;
    }

    if (opts.position == TruncatePosition.START) {
        let cut_start: usize = total_width - available;
        if (opts.prefer_truncation_on_space) {
            if (!(cut_start > 0 && ctIsSpaceAtCol(text, cut_start - 1))) {
                const sp: usize = ctFindFirstSpaceGe(text, cut_start, total_width);
                if (sp + 1 < total_width) {
                    cut_start = sp + 1;
                }
            }
        }
        let w: usize = ctWriteBytes(out, 0, ell);
        if (w < out.len && cut_start < total_width) {
            w += sliceAnsiFrom(text, cut_start, out[w..out.len]);
        }
        return w;
    }

    // TruncatePosition.MIDDLE
    const right_cols: usize = available / 2;
    const left_cols: usize = available - right_cols;

    let cut_left: usize = left_cols;
    let cut_right: usize = total_width - right_cols;

    if (opts.prefer_truncation_on_space) {
        const sp_left: usize = ctFindLastSpaceLe(text, left_cols);
        if (sp_left > 0) {
            cut_left = sp_left;
        }
        if (right_cols > 0 && !(cut_right > 0 && ctIsSpaceAtCol(text, cut_right - 1))) {
            const sp_right: usize = ctFindFirstSpaceGe(text, cut_right, total_width);
            if (sp_right + 1 < total_width) {
                cut_right = sp_right + 1;
            }
        }
    }

    let w: usize = 0;
    if (cut_left > 0) {
        w += sliceAnsi(text, 0, cut_left, out);
    }
    w = ctWriteBytes(out, w, ell);
    if (w < out.len && cut_right < total_width) {
        w += sliceAnsiFrom(text, cut_right, out[w..out.len]);
    }
    return w;
}

/// Convenience helper: truncate `text` at the end with default ellipsis `"..."`.
pub fn cliTruncateEndInto(text: string, columns: usize, out: []u8): usize {
    return cliTruncateInto(text, columns, TruncateOptions.defaults(), out);
}

/// Convenience helper: truncate `text` at the start with default ellipsis `"..."`.
pub fn cliTruncateStartInto(text: string, columns: usize, out: []u8): usize {
    return cliTruncateInto(
        text,
        columns,
        TruncateOptions{
            position: TruncatePosition.START,
            prefer_truncation_on_space: false,
            ellipsis: "...",
        },
        out,
    );
}

/// Convenience helper: truncate `text` in the middle with default ellipsis `"..."`.
pub fn cliTruncateMiddleInto(text: string, columns: usize, out: []u8): usize {
    return cliTruncateInto(
        text,
        columns,
        TruncateOptions{
            position: TruncatePosition.MIDDLE,
            prefer_truncation_on_space: false,
            ellipsis: "...",
        },
        out,
    );
}

export {
    TruncatePosition,
    TruncateOptions,
    cliTruncateInto,
    cliTruncateEndInto,
    cliTruncateStartInto,
    cliTruncateMiddleInto,
};
