// Source: Absorbed from /data/dev/bun/src/cli/cli.cp | Synced: 2026-11-22T17:17:12Z
// lib/cli/table.cp
// Terminal Table Formatter with Unicode/CJK/Emoji Display Width Alignment (cap:string) & Ascii/Box/Markdown Borders

import { stringWidth } from "cap:string";

pub enum CliTableAlign: u8 {
    Left = 0,
    Center = 1,
    Right = 2,
}

pub enum CliTableBorder: u8 {
    Ascii = 0,
    Box = 1,
    Markdown = 2,
}

pub struct CliTable {
    headers: [8][]const u8 = undefined;
    aligns: [8]CliTableAlign = undefined;
    widths: [8]usize = undefined;
    rows: [32][8][]const u8 = undefined;
    col_count: usize = 0;
    row_count: usize = 0;
    border: CliTableBorder = CliTableBorder.Ascii;

    pub static fn init(col_count: usize): CliTable {
        const clamped_cols = if (col_count > 8) (8 as usize) else col_count;
        let tbl = CliTable {
            headers: undefined,
            aligns: undefined,
            widths: undefined,
            rows: undefined,
            col_count: clamped_cols,
            row_count: 0,
            border: CliTableBorder.Ascii,
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

    pub fn setBorder(*_, border_style: CliTableBorder) {
        _.border = border_style;
    }

    pub static fn cellWidth(text: []const u8): usize {
        return stringWidth(text, false);
    }

    pub fn setHeader(*_, col: usize, title: []const u8, align_mode: CliTableAlign) {
        if (col >= _.col_count) {
            return;
        }
        _.headers[col] = title;
        _.aligns[col] = align_mode;
        const w = CliTable.cellWidth(title);
        if (w > _.widths[col]) {
            _.widths[col] = w;
        }
    }

    pub fn addRow(*_, cells: []const []const u8): bool {
        if (_.row_count >= 32) {
            return false;
        }
        const r = _.row_count;
        let k: usize = 0;
        while (k < 8) {
            _.rows[r][k] = "";
            k += 1;
        }
        let c: usize = 0;
        while (c < _.col_count && c < cells.len) {
            _.rows[r][c] = cells[c];
            const w = CliTable.cellWidth(cells[c]);
            if (w > _.widths[c]) {
                _.widths[c] = w;
            }
            c += 1;
        }
        _.row_count += 1;
        return true;
    }

    pub fn addRow1(*_, c0: []const u8): bool {
        let cells: [1][]const u8 = [c0];
        return _.addRow(&cells);
    }

    pub fn addRow2(*_, c0: []const u8, c1: []const u8): bool {
        let cells: [2][]const u8 = [c0, c1];
        return _.addRow(&cells);
    }

    pub fn addRow3(*_, c0: []const u8, c1: []const u8, c2: []const u8): bool {
        let cells: [3][]const u8 = [c0, c1, c2];
        return _.addRow(&cells);
    }

    pub fn addRow4(*_, c0: []const u8, c1: []const u8, c2: []const u8, c3: []const u8): bool {
        let cells: [4][]const u8 = [c0, c1, c2, c3];
        return _.addRow(&cells);
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

    pub static fn appendRepeatSlice(out: []u8, pos: usize, s: []const u8, count: usize): usize {
        let p = pos;
        let i: usize = 0;
        while (i < count) {
            p = CliTable.appendSlice(out, p, s);
            i += 1;
        }
        return p;
    }

    pub static fn appendAlignedCell(
        out: []u8,
        pos: usize,
        text: []const u8,
        width: usize,
        align_mode: CliTableAlign
    ): usize {
        let p = pos;
        const vis_w = CliTable.cellWidth(text);
        const pad_total = if (width > vis_w) (width - vis_w) else (0 as usize);
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

    fn renderBoxRule(out: []u8, pos: usize, left: []const u8, mid: []const u8, right: []const u8): usize {
        let p = pos;
        p = CliTable.appendSlice(out, p, left);
        let c: usize = 0;
        while (c < _.col_count) {
            p = CliTable.appendRepeatSlice(out, p, "─", _.widths[c] + 2);
            p = CliTable.appendSlice(out, p, if (c + 1 < _.col_count) mid else right);
            c += 1;
        }
        p = CliTable.appendChar(out, p, '\n', 1);
        return p;
    }

    fn renderMarkdownSeparator(out: []u8, pos: usize): usize {
        let p = pos;
        p = CliTable.appendChar(out, p, '|', 1);
        let c: usize = 0;
        while (c < _.col_count) {
            const col_w = if (_.widths[c] < 3) (3 as usize) else _.widths[c];
            p = CliTable.appendChar(out, p, ' ', 1);
            if (_.aligns[c] == CliTableAlign.Left) {
                p = CliTable.appendChar(out, p, ':', 1);
                p = CliTable.appendChar(out, p, '-', col_w - 1);
            } else if (_.aligns[c] == CliTableAlign.Center) {
                p = CliTable.appendChar(out, p, ':', 1);
                p = CliTable.appendChar(out, p, '-', if (col_w > 2) (col_w - 2) else 1);
                p = CliTable.appendChar(out, p, ':', 1);
            } else {
                p = CliTable.appendChar(out, p, '-', col_w - 1);
                p = CliTable.appendChar(out, p, ':', 1);
            }
            p = CliTable.appendSlice(out, p, " |");
            c += 1;
        }
        p = CliTable.appendChar(out, p, '\n', 1);
        return p;
    }

    fn renderDataRow(out: []u8, pos: usize, cells: []const []const u8, bar: []const u8): usize {
        let p = pos;
        p = CliTable.appendSlice(out, p, bar);
        let c: usize = 0;
        while (c < _.col_count) {
            const cell = if (c < cells.len) cells[c] else "";
            p = CliTable.appendAlignedCell(out, p, cell, _.widths[c], _.aligns[c]);
            p = CliTable.appendSlice(out, p, bar);
            c += 1;
        }
        p = CliTable.appendChar(out, p, '\n', 1);
        return p;
    }

    /// Renders the table in the configured `border` style (`Ascii`, `Box`, or `Markdown`) into `out`.
    pub fn renderToBuf(out: []u8): usize {
        if (_.col_count == 0) {
            return 0;
        }
        let pos: usize = 0;
        const bar: []const u8 = if (_.border == CliTableBorder.Box) "│" else "|";

        if (_.border == CliTableBorder.Ascii) {
            pos = _.renderSeparator(out, pos);
        } else if (_.border == CliTableBorder.Box) {
            pos = _.renderBoxRule(out, pos, "┌", "┬", "┐");
        }

        // Header row
        pos = _.renderDataRow(out, pos, _.headers[0.._.col_count], bar);

        if (_.border == CliTableBorder.Ascii) {
            pos = _.renderSeparator(out, pos);
        } else if (_.border == CliTableBorder.Box) {
            pos = _.renderBoxRule(out, pos, "├", "┼", "┤");
        } else {
            pos = _.renderMarkdownSeparator(out, pos);
        }

        // Data rows
        let r: usize = 0;
        while (r < _.row_count) {
            pos = _.renderDataRow(out, pos, _.rows[r][0.._.col_count], bar);
            r += 1;
        }

        if (_.border == CliTableBorder.Ascii) {
            pos = _.renderSeparator(out, pos);
        } else if (_.border == CliTableBorder.Box) {
            pos = _.renderBoxRule(out, pos, "└", "┴", "┘");
        }
        return pos;
    }
}
