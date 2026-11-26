// @cap/cliui — Zero-allocation multi-column terminal layout engine in pure CP
// Clean-room MIT implementation inspired by npm `cliui` (yargs core layout engine).
// Reuses @cap/wrap-ansi for ANSI-preserving text wrapping, @cap/ansi-strip for ANSI stripping,
// and @cap/unicode-width for terminal display column width calculation.

import { wrapAnsi, visibleWidth } from "../../wrap-ansi/src/index.cp";
import { hasAnsi, stripAnsi } from "../../ansi-strip/src/index.cp";
import { strWidth } from "../../unicode-width/src/index.cp";

pub const CU_MAX_COLS: usize = 8;
pub const CU_WRAP_CAP: usize = 512;
pub const CU_BUF_CAP: usize = 8192;

pub const ALIGN_LEFT: u8 = 0;
pub const ALIGN_RIGHT: u8 = 1;
pub const ALIGN_CENTER: u8 = 2;

pub enum Align {
    Left,
    Right,
    Center,
}

pub fn alignToU8(a: Align): u8 {
    if (a == Align.Right) {
        return ALIGN_RIGHT;
    }
    if (a == Align.Center) {
        return ALIGN_CENTER;
    }
    return ALIGN_LEFT;
}

pub struct Column {
    pub text: string,
    pub width: usize,
    pub alignment: u8,
    pub pad_left: usize,
    pub pad_right: usize,
    pub border: bool,

    pub static fn plain(text: string): Column {
        return Column {
            text: text,
            width: 0,
            alignment: ALIGN_LEFT,
            pad_left: 0,
            pad_right: 0,
            border: false,
        };
    }

    pub static fn sized(text: string, width: usize): Column {
        return Column {
            text: text,
            width: width,
            alignment: ALIGN_LEFT,
            pad_left: 0,
            pad_right: 0,
            border: false,
        };
    }
}

fn cuWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn cuWriteBytes(out: []u8, pos: usize, s: string): usize {
    let w: usize = pos;
    let i: usize = 0;
    while (i < s.len && w < out.len) {
        out[w] = s[i];
        w += 1;
        i += 1;
    }
    return w;
}

fn cuWriteSpaces(out: []u8, pos: usize, count: usize): usize {
    let w: usize = pos;
    let i: usize = 0;
    while (i < count && w < out.len) {
        out[w] = 32; // ' '
        w += 1;
        i += 1;
    }
    return w;
}

/// Compute terminal display width of `s`, reusing `@cap/ansi-strip` and `@cap/unicode-width`.
fn cuVisibleWidth(s: string): usize {
    if (s.len == 0) {
        return 0;
    }
    if (!hasAnsi(s)) {
        return strWidth(s);
    }
    if (s.len <= 512) {
        let stripped: [512]u8 = undefined;
        const n: usize = stripAnsi(s, &stripped);
        return strWidth(stripped[0..n]);
    }
    return visibleWidth(s);
}

fn cuCountLines(s: string): usize {
    if (s.len == 0) {
        return 1;
    }
    let lines: usize = 1;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 10) { // '\n'
            lines += 1;
        }
        i += 1;
    }
    return lines;
}

fn cuGetLine(s: string, target_line: usize): string {
    let cur: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 10) { // '\n'
            if (cur == target_line) {
                return s[start..i];
            }
            cur += 1;
            start = i + 1;
        }
        i += 1;
    }
    if (cur == target_line) {
        return s[start..s.len];
    }
    return "";
}

fn cuResolveWidths(total_width: usize, cols: []const Column, n_cols: usize, widths: []usize) {
    let used: usize = 0;
    let auto_count: usize = 0;
    let i: usize = 0;
    while (i < n_cols) {
        if (cols[i].width > 0) {
            widths[i] = cols[i].width;
            used += cols[i].width;
        } else {
            widths[i] = 0;
            auto_count += 1;
        }
        i += 1;
    }
    if (auto_count > 0) {
        let rem: usize = 0;
        if (total_width > used) {
            rem = total_width - used;
        }
        let base: usize = rem / auto_count;
        let extra: usize = rem % auto_count;
        if (base == 0) {
            base = 1;
            extra = 0;
        }
        let j: usize = 0;
        while (j < n_cols) {
            if (widths[j] == 0) {
                let w: usize = base;
                if (extra > 0) {
                    w += 1;
                    extra -= 1;
                }
                widths[j] = w;
            }
            j += 1;
        }
    }
}

pub struct CliUi {
    pub width: usize,
    pub len: usize,
    pub row_count: usize,
    pub in_span: bool,
    buf: [CU_BUF_CAP]u8,

    pub static fn new(width: usize): CliUi {
        let b: [CU_BUF_CAP]u8 = undefined;
        let w: usize = width;
        if (w == 0) {
            w = 80;
        }
        return CliUi {
            width: w,
            len: 0,
            row_count: 0,
            in_span: false,
            buf: b,
        };
    }

    pub fn resetOutput(*_) {
        _.len = 0;
        _.row_count = 0;
        _.in_span = false;
    }

    /// Add a multi-column block (div) that starts on a new line if output already exists.
    pub fn div(*_, cols: []const Column) {
        if (cols.len == 0) {
            return;
        }
        let n_cols: usize = cols.len;
        if (n_cols > CU_MAX_COLS) {
            n_cols = CU_MAX_COLS;
        }
        if (_.len > 0) {
            _.len = cuWriteByte(&_.buf, _.len, 10); // '\n'
        }
        _.in_span = false;

        let col_widths: [8]usize = [0, 0, 0, 0, 0, 0, 0, 0];
        cuResolveWidths(_.width, cols, n_cols, &col_widths);

        let inner_widths: [8]usize = [0, 0, 0, 0, 0, 0, 0, 0];
        let wrap_bufs: [8][512]u8 = undefined;
        let wrap_lens: [8]usize = [0, 0, 0, 0, 0, 0, 0, 0];
        let line_counts: [8]usize = [0, 0, 0, 0, 0, 0, 0, 0];
        let max_lines: usize = 1;

        let c: usize = 0;
        while (c < n_cols) {
            let overhead: usize = cols[c].pad_left + cols[c].pad_right;
            if (cols[c].border) {
                overhead += 2;
            }
            let iw: usize = 1;
            if (col_widths[c] > overhead) {
                iw = col_widths[c] - overhead;
            }
            inner_widths[c] = iw;

            const wrapped: string = wrapAnsi(cols[c].text, iw, true, &wrap_bufs[c]);
            wrap_lens[c] = wrapped.len;
            const lc: usize = cuCountLines(wrapped);
            line_counts[c] = lc;
            if (lc > max_lines) {
                max_lines = lc;
            }
            c += 1;
        }

        let r: usize = 0;
        while (r < max_lines) {
            if (r > 0) {
                _.len = cuWriteByte(&_.buf, _.len, 10); // '\n'
            }
            let ci: usize = 0;
            while (ci < n_cols) {
                const col = cols[ci];
                const iw: usize = inner_widths[ci];
                const wrapped_str: string = wrap_bufs[ci][0..wrap_lens[ci]];
                let seg: string = "";
                if (r < line_counts[ci]) {
                    seg = cuGetLine(wrapped_str, r);
                }
                const vis_w: usize = cuVisibleWidth(seg);
                let pad_sp: usize = 0;
                if (iw > vis_w) {
                    pad_sp = iw - vis_w;
                }

                let left_sp: usize = 0;
                let right_sp: usize = pad_sp;
                if (col.alignment == ALIGN_RIGHT) {
                    left_sp = pad_sp;
                    right_sp = 0;
                } else if (col.alignment == ALIGN_CENTER) {
                    left_sp = pad_sp / 2;
                    right_sp = pad_sp - left_sp;
                }

                const is_last: bool = (ci + 1 == n_cols);
                if (col.border) {
                    _.len = cuWriteByte(&_.buf, _.len, 124); // '|'
                }
                _.len = cuWriteSpaces(&_.buf, _.len, col.pad_left);
                _.len = cuWriteSpaces(&_.buf, _.len, left_sp);
                _.len = cuWriteBytes(&_.buf, _.len, seg);
                if (!is_last || col.border || col.pad_right > 0 || col.alignment == ALIGN_CENTER) {
                    _.len = cuWriteSpaces(&_.buf, _.len, right_sp);
                }
                _.len = cuWriteSpaces(&_.buf, _.len, col.pad_right);
                if (col.border) {
                    _.len = cuWriteByte(&_.buf, _.len, 124); // '|'
                }
                ci += 1;
            }
            r += 1;
        }
        _.row_count += 1;
    }

    /// Append columns horizontally on the current span line without forcing a line break
    /// between consecutive `span` calls.
    pub fn span(*_, cols: []const Column) {
        if (cols.len == 0) {
            return;
        }
        let n_cols: usize = cols.len;
        if (n_cols > CU_MAX_COLS) {
            n_cols = CU_MAX_COLS;
        }
        if (!_.in_span && _.len > 0) {
            _.len = cuWriteByte(&_.buf, _.len, 10); // '\n'
        }
        _.in_span = true;

        let ci: usize = 0;
        while (ci < n_cols) {
            const col = cols[ci];
            const vis_w: usize = cuVisibleWidth(col.text);
            let overhead: usize = col.pad_left + col.pad_right;
            if (col.border) {
                overhead += 2;
            }
            let iw: usize = vis_w;
            if (col.width > overhead) {
                iw = col.width - overhead;
            }
            let pad_sp: usize = 0;
            if (iw > vis_w) {
                pad_sp = iw - vis_w;
            }
            let left_sp: usize = 0;
            let right_sp: usize = pad_sp;
            if (col.alignment == ALIGN_RIGHT) {
                left_sp = pad_sp;
                right_sp = 0;
            } else if (col.alignment == ALIGN_CENTER) {
                left_sp = pad_sp / 2;
                right_sp = pad_sp - left_sp;
            }

            if (col.border) {
                _.len = cuWriteByte(&_.buf, _.len, 124);
            }
            _.len = cuWriteSpaces(&_.buf, _.len, col.pad_left);
            _.len = cuWriteSpaces(&_.buf, _.len, left_sp);
            _.len = cuWriteBytes(&_.buf, _.len, col.text);
            if (col.width > 0 || col.border || col.pad_right > 0) {
                _.len = cuWriteSpaces(&_.buf, _.len, right_sp);
            }
            _.len = cuWriteSpaces(&_.buf, _.len, col.pad_right);
            if (col.border) {
                _.len = cuWriteByte(&_.buf, _.len, 124);
            }
            ci += 1;
        }
    }

    /// Copy the rendered multi-column layout into `out` and return bytes written.
    pub fn render(out: []u8): usize {
        return cuWriteBytes(out, 0, _.buf[0.._.len]);
    }
}

/// Convenience helper: format a two-column CLI help row (`left` fixed to `left_width`,
/// `right` automatically occupying remaining width within `total_width`).
pub fn formatTwoCol(left: string, right: string, left_width: usize, total_width: usize, out: []u8): usize {
    let ui: CliUi = CliUi.new(total_width);
    let cols: [2]Column = [
        Column.sized(left, left_width),
        Column.plain(right),
    ];
    ui.div(&cols);
    return ui.render(out);
}

export {
    CU_MAX_COLS,
    CU_WRAP_CAP,
    CU_BUF_CAP,
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER,
    Align,
    alignToU8,
    Column,
    CliUi,
    formatTwoCol,
};
