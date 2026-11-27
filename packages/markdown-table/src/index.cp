// @cap/markdown-table — Zero-heap GFM Markdown table renderer
// Clean-room CP implementation inspired by npm:markdown-table (wooorm/markdown-table)

const MDTB_MAX_COLS: usize = 64;

pub enum TableAlign {
    None,
    Left,
    Center,
    Right,
}

fn mdtbWriteByte(out: []u8, pos: usize, ch: u8): usize {
    if (pos < out.len) {
        out[pos] = ch;
        return pos + 1;
    }
    return pos;
}

fn mdtbWriteStr(out: []u8, pos: usize, s: string): usize {
    if (pos >= out.len || s.len == 0) {
        return pos;
    }
    const avail = out.len - pos;
    const n = if (s.len < avail) s.len else avail;
    @memcpy(out[pos..pos + n], s[0..n]);
    return pos + n;
}

fn mdtbWriteRepeat(out: []u8, pos: usize, ch: u8, count: usize): usize {
    if (pos >= out.len || count == 0) {
        return pos;
    }
    const avail = out.len - pos;
    const n = if (count < avail) count else avail;
    @memset(out[pos..pos + n], ch);
    return pos + n;
}

fn mdtbGetAlign(aligns: []const TableAlign, col: usize): TableAlign {
    if (col < aligns.len) {
        return aligns[col];
    }
    return TableAlign.None;
}

fn mdtbMinWidth(al: TableAlign): usize {
    if (al == TableAlign.Center) {
        return 5;
    }
    if (al == TableAlign.Left || al == TableAlign.Right) {
        return 4;
    }
    return 3;
}

fn mdtbWriteCell(out: []u8, pos: usize, text: string, width: usize, al: TableAlign): usize {
    let p = pos;
    const pad = if (width > text.len) width - text.len else 0;
    if (al == TableAlign.Right) {
        p = mdtbWriteRepeat(out, p, ' ', pad);
        p = mdtbWriteStr(out, p, text);
    } else if (al == TableAlign.Center) {
        const left_pad = pad / 2;
        const right_pad = pad - left_pad;
        p = mdtbWriteRepeat(out, p, ' ', left_pad);
        p = mdtbWriteStr(out, p, text);
        p = mdtbWriteRepeat(out, p, ' ', right_pad);
    } else {
        p = mdtbWriteStr(out, p, text);
        p = mdtbWriteRepeat(out, p, ' ', pad);
    }
    return p;
}

fn mdtbWriteDelim(out: []u8, pos: usize, width: usize, al: TableAlign): usize {
    let p = pos;
    if (al == TableAlign.Left) {
        p = mdtbWriteByte(out, p, ':');
        const dashes = if (width > 1) width - 1 else 1;
        p = mdtbWriteRepeat(out, p, '-', dashes);
    } else if (al == TableAlign.Right) {
        const dashes = if (width > 1) width - 1 else 1;
        p = mdtbWriteRepeat(out, p, '-', dashes);
        p = mdtbWriteByte(out, p, ':');
    } else if (al == TableAlign.Center) {
        p = mdtbWriteByte(out, p, ':');
        const dashes = if (width > 2) width - 2 else 1;
        p = mdtbWriteRepeat(out, p, '-', dashes);
        p = mdtbWriteByte(out, p, ':');
    } else {
        p = mdtbWriteRepeat(out, p, '-', width);
    }
    return p;
}

fn mdtbColWidth(
    headers: []const string,
    rows: []const string,
    num_cols: usize,
    num_rows: usize,
    col: usize,
    al: TableAlign,
): usize {
    let w = mdtbMinWidth(al);
    if (col < headers.len && headers[col].len > w) {
        w = headers[col].len;
    }
    let r: usize = 0;
    while (r < num_rows) : (r += 1) {
        const idx = r * num_cols + col;
        if (idx < rows.len && rows[idx].len > w) {
            w = rows[idx].len;
        }
    }
    return w;
}

/// Render a GFM Markdown table into `out`, returning the number of bytes written.
/// `rows` is row-major flattened with `num_rows * num_cols` cells.
pub fn renderMarkdownTable(
    headers: []const string,
    rows: []const string,
    num_cols: usize,
    num_rows: usize,
    aligns: []const TableAlign,
    out: []u8,
): usize {
    if (num_cols == 0 || out.len == 0) {
        return 0;
    }

    let widths: [64]usize = undefined;
    const cached_cols = if (num_cols < MDTB_MAX_COLS) num_cols else MDTB_MAX_COLS;
    let c: usize = 0;
    while (c < cached_cols) : (c += 1) {
        const al = mdtbGetAlign(aligns, c);
        widths[c] = mdtbColWidth(headers, rows, num_cols, num_rows, c, al);
    }

    let pos: usize = 0;

    // 1. Header row
    pos = mdtbWriteByte(out, pos, '|');
    c = 0;
    while (c < num_cols) : (c += 1) {
        const al = mdtbGetAlign(aligns, c);
        const w = if (c < cached_cols) widths[c] else mdtbColWidth(headers, rows, num_cols, num_rows, c, al);
        const h = if (c < headers.len) headers[c] else "";
        pos = mdtbWriteByte(out, pos, ' ');
        pos = mdtbWriteCell(out, pos, h, w, al);
        pos = mdtbWriteStr(out, pos, " |");
    }

    // 2. Alignment delimiter row
    pos = mdtbWriteByte(out, pos, '\n');
    pos = mdtbWriteByte(out, pos, '|');
    c = 0;
    while (c < num_cols) : (c += 1) {
        const al = mdtbGetAlign(aligns, c);
        const w = if (c < cached_cols) widths[c] else mdtbColWidth(headers, rows, num_cols, num_rows, c, al);
        pos = mdtbWriteByte(out, pos, ' ');
        pos = mdtbWriteDelim(out, pos, w, al);
        pos = mdtbWriteStr(out, pos, " |");
    }

    // 3. Data rows
    let r: usize = 0;
    while (r < num_rows) : (r += 1) {
        pos = mdtbWriteByte(out, pos, '\n');
        pos = mdtbWriteByte(out, pos, '|');
        c = 0;
        while (c < num_cols) : (c += 1) {
            const al = mdtbGetAlign(aligns, c);
            const w = if (c < cached_cols) widths[c] else mdtbColWidth(headers, rows, num_cols, num_rows, c, al);
            const idx = r * num_cols + c;
            const cell = if (idx < rows.len) rows[idx] else "";
            pos = mdtbWriteByte(out, pos, ' ');
            pos = mdtbWriteCell(out, pos, cell, w, al);
            pos = mdtbWriteStr(out, pos, " |");
        }
    }

    return pos;
}
