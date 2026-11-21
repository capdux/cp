// Source: Absorbed from /data/dev/bun/src/cli/ | Synced: 2026-11-21T16:39:27Z (Zero-Alloc CLI Table Renderer)
// Pure-CP zero-allocation CLI terminal table border, column-width, and alignment renderer.

/// Terminal table border drawing style.
pub enum TableBorderStyle {
    ascii,
    compact,
    unicode_light,
}

/// Horizontal text alignment inside a table cell.
pub enum CellAlign {
    left,
    right,
    center,
}

/// Skip a single ANSI CSI/OSC escape sequence starting at `src[pos] == 0x1B`.
fn skipAnsiAt(src: []const u8, pos: usize): usize {
    if (pos + 1 >= src.len) return src.len;
    const next = src[pos + 1];
    let i = pos + 2;
    if (next == '[') {
        while (i < src.len && src[i] >= 0x20 && src[i] <= 0x3F) i += 1;
        if (i < src.len && src[i] >= 0x40 && src[i] <= 0x7E) i += 1;
        return i;
    }
    if (next == ']') {
        while (i < src.len) {
            if (src[i] == 0x07) return i + 1;
            if (src[i] == 0x1B && i + 1 < src.len && src[i + 1] == '\\') return i + 2;
            i += 1;
        }
        return i;
    }
    return i;
}

/// Check if a Unicode codepoint occupies 2 terminal columns (East Asian Wide / Fullwidth).
fn isWideCodepoint(cp: u32): bool {
    return (cp >= 0x1100 && cp <= 0x115F) ||
        (cp >= 0x2E80 && cp <= 0x9FFF) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) ||
        (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0xFE10 && cp <= 0xFE6F) ||
        (cp >= 0xFF00 && cp <= 0xFF60) ||
        (cp >= 0xFFE0 && cp <= 0xFFE6) ||
        (cp >= 0x20000 && cp <= 0x2FFFF);
}

/// Compute visible terminal display width of a cell string (ignoring ANSI escape codes).
fn cellDisplayWidth(s: []const u8): usize {
    let i: usize = 0;
    let width: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b == 0x1B) {
            i = skipAnsiAt(s, i);
            continue;
        }
        if (b < 0x80) {
            if (b >= 0x20 && b != 0x7F) width += 1;
            i += 1;
            continue;
        }
        // Decode UTF-8 multi-byte codepoint
        if ((b & 0xE0) == 0xC0 && i + 1 < s.len) {
            const cp: u32 = (((b & 0x1F) as u32) << 6) | ((s[i + 1] & 0x3F) as u32);
            width += if (isWideCodepoint(cp)) 2 else 1;
            i += 2;
        } else if ((b & 0xF0) == 0xE0 && i + 2 < s.len) {
            const cp: u32 = (((b & 0x0F) as u32) << 12) |
                (((s[i + 1] & 0x3F) as u32) << 6) |
                ((s[i + 2] & 0x3F) as u32);
            width += if (isWideCodepoint(cp)) 2 else 1;
            i += 3;
        } else if ((b & 0xF8) == 0xF0 && i + 3 < s.len) {
            const cp: u32 = (((b & 0x07) as u32) << 18) |
                (((s[i + 1] & 0x3F) as u32) << 12) |
                (((s[i + 2] & 0x3F) as u32) << 6) |
                ((s[i + 3] & 0x3F) as u32);
            width += if (isWideCodepoint(cp)) 2 else 1;
            i += 4;
        } else {
            width += 1;
            i += 1;
        }
    }
    return width;
}

/// Append a byte slice into `out` at `offset`, returning new offset or `null` on overflow.
fn pushSlice(out: []u8, offset: usize, src: []const u8): usize? {
    if (offset + src.len > out.len) return null;
    let i: usize = 0;
    while (i < src.len) {
        out[offset + i] = src[i];
        i += 1;
    }
    return offset + src.len;
}

/// Append `count` repetitions of `unit` into `out` at `offset`.
fn pushRepeat(out: []u8, offset: usize, unit: []const u8, count: usize): usize? {
    let w = offset;
    let i: usize = 0;
    while (i < count) {
        w = pushSlice(out, w, unit) orelse return null;
        i += 1;
    }
    return w;
}

/// Append `count` ASCII space characters into `out` at `offset`.
fn pushSpaces(out: []u8, offset: usize, count: usize): usize? {
    if (offset + count > out.len) return null;
    let i: usize = 0;
    while (i < count) {
        out[offset + i] = ' ';
        i += 1;
    }
    return offset + count;
}

/// Compute maximum display width per column across `rows`, writing into `widths_out`.
/// Returns the number of columns recorded (clamped to `widths_out.len`).
pub fn computeColumnWidths(rows: []const []const []const u8, widths_out: []usize): usize {
    let i: usize = 0;
    while (i < widths_out.len) {
        widths_out[i] = 0;
        i += 1;
    }

    let max_cols: usize = 0;
    let r: usize = 0;
    while (r < rows.len) {
        const row = rows[r];
        const cols = if (row.len < widths_out.len) row.len else widths_out.len;
        if (cols > max_cols) max_cols = cols;
        let c: usize = 0;
        while (c < cols) {
            const w = cellDisplayWidth(row[c]);
            if (w > widths_out[c]) widths_out[c] = w;
            c += 1;
        }
        r += 1;
    }
    return max_cols;
}

/// Render a horizontal border line for `widths`.
/// `pos`: `0` = top, `1` = middle separator, `2` = bottom.
/// Returns `null` if `pos > 2` or if `out` is too small.
/// For `TableBorderStyle.compact`, top (`0`) and bottom (`2`) return an empty slice.
pub fn renderBorderLine(widths: []const usize, style: TableBorderStyle, pos: u8, out: []u8): ([]const u8)? {
    if (pos > 2) return null;
    if (widths.len == 0) return out[0..0];

    if (style == TableBorderStyle.compact) {
        if (pos == 0 || pos == 2) return out[0..0];
        let w: usize = 0;
        let c: usize = 0;
        while (c < widths.len) {
            if (c > 0) {
                w = pushSpaces(out, w, 2) orelse return null;
            }
            const seg_w = if (widths[c] > 0) widths[c] else 1;
            w = pushRepeat(out, w, "-", seg_w) orelse return null;
            c += 1;
        }
        return out[0..w];
    }

    let left_ch: []const u8 = "+";
    let mid_ch: []const u8 = "+";
    let right_ch: []const u8 = "+";
    let horiz_ch: []const u8 = "-";

    if (style == TableBorderStyle.unicode_light) {
        horiz_ch = "─";
        if (pos == 0) {
            left_ch = "┌";
            mid_ch = "┬";
            right_ch = "┐";
        } else if (pos == 1) {
            left_ch = "├";
            mid_ch = "┼";
            right_ch = "┤";
        } else {
            left_ch = "└";
            mid_ch = "┴";
            right_ch = "┘";
        }
    }

    let w: usize = 0;
    w = pushSlice(out, w, left_ch) orelse return null;
    let c: usize = 0;
    while (c < widths.len) {
        if (c > 0) {
            w = pushSlice(out, w, mid_ch) orelse return null;
        }
        w = pushRepeat(out, w, horiz_ch, widths[c] + 2) orelse return null;
        c += 1;
    }
    w = pushSlice(out, w, right_ch) orelse return null;
    return out[0..w];
}

/// Render a single data or header row aligned according to `widths` and `col_aligns`.
/// Missing cells default to `""`; missing alignments default to `CellAlign.left`.
/// Returns `null` if `out` is too small.
pub fn renderTableRow(
    cells: []const []const u8,
    widths: []const usize,
    col_aligns: []const CellAlign,
    style: TableBorderStyle,
    out: []u8,
): ([]const u8)? {
    if (widths.len == 0) return out[0..0];

    const is_compact = (style == TableBorderStyle.compact);
    const bar: []const u8 = if (style == TableBorderStyle.unicode_light) "│" else "|";

    let w: usize = 0;
    if (!is_compact) {
        w = pushSlice(out, w, bar) orelse return null;
    }

    let c: usize = 0;
    while (c < widths.len) {
        if (c > 0) {
            if (is_compact) {
                w = pushSpaces(out, w, 2) orelse return null;
            } else {
                w = pushSlice(out, w, bar) orelse return null;
            }
        }

        const cell: []const u8 = if (c < cells.len) cells[c] else "";
        const col_align: CellAlign = if (c < col_aligns.len) col_aligns[c] else CellAlign.left;
        const vis_w = cellDisplayWidth(cell);
        const pad = if (widths[c] > vis_w) (widths[c] - vis_w) else 0;

        let left_pad: usize = 0;
        let right_pad: usize = pad;
        if (col_align == CellAlign.right) {
            left_pad = pad;
            right_pad = 0;
        } else if (col_align == CellAlign.center) {
            left_pad = pad / 2;
            right_pad = pad - left_pad;
        }

        if (!is_compact) {
            w = pushSpaces(out, w, 1) orelse return null;
        }
        w = pushSpaces(out, w, left_pad) orelse return null;
        w = pushSlice(out, w, cell) orelse return null;
        w = pushSpaces(out, w, right_pad) orelse return null;
        if (!is_compact) {
            w = pushSpaces(out, w, 1) orelse return null;
        }
        c += 1;
    }

    if (!is_compact) {
        w = pushSlice(out, w, bar) orelse return null;
    }
    return out[0..w];
}

/// Render a complete table (top border + header + separator + rows + bottom border) into `out`.
/// Supports up to 64 columns on the stack with zero heap allocations.
/// Returns `null` if `out` is too small.
pub fn renderTable(
    headers: []const []const u8,
    rows: []const []const []const u8,
    col_aligns: []const CellAlign,
    style: TableBorderStyle,
    out: []u8,
): ([]const u8)? {
    let widths_buf: [64]usize = [0; 64];
    let col_count = computeColumnWidths(rows, widths_buf[0..]);

    const h_cols = if (headers.len < 64) headers.len else 64;
    if (h_cols > col_count) col_count = h_cols;
    let hc: usize = 0;
    while (hc < h_cols) {
        const hw = cellDisplayWidth(headers[hc]);
        if (hw > widths_buf[hc]) widths_buf[hc] = hw;
        hc += 1;
    }

    if (col_count == 0) return out[0..0];
    const widths = widths_buf[0..col_count];

    let w: usize = 0;
    let has_line = false;

    // 1. Top border
    const top = renderBorderLine(widths, style, 0, out[w..]) orelse return null;
    if (top.len > 0) {
        w += top.len;
        has_line = true;
    }

    // 2. Header row + middle separator
    if (headers.len > 0) {
        if (has_line) {
            w = pushSlice(out, w, "\n") orelse return null;
        }
        const h_row = renderTableRow(headers, widths, col_aligns, style, out[w..]) orelse return null;
        w += h_row.len;
        has_line = true;

        w = pushSlice(out, w, "\n") orelse return null;
        const sep = renderBorderLine(widths, style, 1, out[w..]) orelse return null;
        w += sep.len;
    }

    // 3. Data rows
    let r: usize = 0;
    while (r < rows.len) {
        if (has_line) {
            w = pushSlice(out, w, "\n") orelse return null;
        }
        const d_row = renderTableRow(rows[r], widths, col_aligns, style, out[w..]) orelse return null;
        w += d_row.len;
        has_line = true;
        r += 1;
    }

    // 4. Bottom border
    if (style != TableBorderStyle.compact) {
        if (has_line) {
            w = pushSlice(out, w, "\n") orelse return null;
        }
        const bot = renderBorderLine(widths, style, 2, out[w..]) orelse return null;
        w += bot.len;
    }

    return out[0..w];
}
