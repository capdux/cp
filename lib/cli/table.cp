// Source: Bun src/cli/
// lib/cli/table.cp
// Terminal ASCII Table Formatter with Auto Column Width, Alignment & Header Separator
// Absorbed from Bun src/cli/

pub enum CliTableAlign: u8 {
    Left = 0,
    Center = 1,
    Right = 2,
}

pub struct CliTable {
    headers: [8][]const u8 = undefined;
    aligns: [8]CliTableAlign = undefined;
    widths: [8]usize = undefined;
    rows: [32][8][]const u8 = undefined;
    col_count: usize = 0;
    row_count: usize = 0;

    pub static fn init(col_count: usize): CliTable {
        const clamped_cols = if (col_count > 8) (8 as usize) else col_count;
        let tbl = CliTable {
            headers: undefined,
            aligns: undefined,
            widths: undefined,
            rows: undefined,
            col_count: clamped_cols,
            row_count: 0,
        };
        let c: usize = 0;
        while (c < 8) {
            tbl.headers[c] = "";
            tbl.aligns[c] = CliTableAlign.Left;
            tbl.widths[c] = 0;
            c += 1;
        }
        return tbl;
    }

    pub fn setHeader(*_, col: usize, title: []const u8, align_mode: CliTableAlign) {
        if (col >= _.col_count) {
            return;
        }
        _.headers[col] = title;
        _.aligns[col] = align_mode;
        if (title.len > _.widths[col]) {
            _.widths[col] = title.len;
        }
    }

    pub fn addRow2(*_, c0: []const u8, c1: []const u8): bool {
        if (_.row_count >= 32) {
            return false;
        }
        let r = _.row_count;
        let k: usize = 0;
        while (k < 8) {
            _.rows[r][k] = "";
            k += 1;
        }
        if (_.col_count > 0) {
            _.rows[r][0] = c0;
            if (c0.len > _.widths[0]) { _.widths[0] = c0.len; }
        }
        if (_.col_count > 1) {
            _.rows[r][1] = c1;
            if (c1.len > _.widths[1]) { _.widths[1] = c1.len; }
        }
        _.row_count += 1;
        return true;
    }

    pub fn addRow3(*_, c0: []const u8, c1: []const u8, c2: []const u8): bool {
        if (!_.addRow2(c0, c1)) {
            return false;
        }
        if (_.col_count > 2) {
            const r = _.row_count - 1;
            _.rows[r][2] = c2;
            if (c2.len > _.widths[2]) { _.widths[2] = c2.len; }
        }
        return true;
    }

    pub static fn appendChar(out: []u8, pos: usize, ch: u8, count: usize): usize {
        let p = pos;
        let i: usize = 0;
        while (i < count && p < out.len) {
            out[p] = ch;
            p += 1;
            i += 1;
        }
        return p;
    }

    pub static fn appendSlice(out: []u8, pos: usize, s: []const u8): usize {
        if (pos + s.len > out.len) {
            return pos;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[pos + i] = s[i];
            i += 1;
        }
        return pos + s.len;
    }

    pub static fn appendAlignedCell(
        out: []u8,
        pos: usize,
        text: []const u8,
        width: usize,
        align_mode: CliTableAlign
    ): usize {
        let p = pos;
        const pad_total = if (width > text.len) (width - text.len) else (0 as usize);
        let pad_left: usize = 0;
        let pad_right: usize = pad_total;
        if (align_mode == CliTableAlign.Right) {
            pad_left = pad_total;
            pad_right = 0;
        } else if (align_mode == CliTableAlign.Center) {
            pad_left = pad_total / 2;
            pad_right = pad_total - pad_left;
        }

        p = CliTable.appendChar(out, p, ' ', 1 + pad_left);
        p = CliTable.appendSlice(out, p, text);
        p = CliTable.appendChar(out, p, ' ', pad_right + 1);
        return p;
    }

    pub fn renderSeparator(out: []u8, pos: usize): usize {
        let p = pos;
        p = CliTable.appendChar(out, p, '+', 1);
        let c: usize = 0;
        while (c < _.col_count) {
            p = CliTable.appendChar(out, p, '-', _.widths[c] + 2);
            p = CliTable.appendChar(out, p, '+', 1);
            c += 1;
        }
        p = CliTable.appendChar(out, p, '\n', 1);
        return p;
    }

    /// Renders the complete ASCII box table into `out` and returns the number of bytes written.
    pub fn renderToBuf(out: []u8): usize {
        if (_.col_count == 0) {
            return 0;
        }
        let pos: usize = 0;
        pos = _.renderSeparator(out, pos);

        // Header row
        pos = CliTable.appendChar(out, pos, '|', 1);
        let c: usize = 0;
        while (c < _.col_count) {
            pos = CliTable.appendAlignedCell(out, pos, _.headers[c], _.widths[c], _.aligns[c]);
            pos = CliTable.appendChar(out, pos, '|', 1);
            c += 1;
        }
        pos = CliTable.appendChar(out, pos, '\n', 1);
        pos = _.renderSeparator(out, pos);

        // Data rows
        let r: usize = 0;
        while (r < _.row_count) {
            pos = CliTable.appendChar(out, pos, '|', 1);
            let col: usize = 0;
            while (col < _.col_count) {
                pos = CliTable.appendAlignedCell(out, pos, _.rows[r][col], _.widths[col], _.aligns[col]);
                pos = CliTable.appendChar(out, pos, '|', 1);
                col += 1;
            }
            pos = CliTable.appendChar(out, pos, '\n', 1);
            r += 1;
        }

        pos = _.renderSeparator(out, pos);
        return pos;
    }
}
