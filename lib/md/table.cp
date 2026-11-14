// lib/md/table.cp
// GFM Markdown Table Alignment Parser, Row Cell Splitter & HTML <table> Renderer
// Absorbed from Bun src/md/

const std = @import("std");

pub enum MdTableAlign: u8 {
    None = 0,
    Left = 1,
    Center = 2,
    Right = 3,
}

pub struct MdTable {
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

    /// Parses a GFM delimiter cell (`---`, `:---`, `:---:`, `---:`) into `MdTableAlign`.
    /// Returns `null` if the cell is not a valid GFM table delimiter specification.
    pub static fn parseAlignSpec(spec: []const u8): MdTableAlign? {
        const trimmed = std.mem.trim(u8, spec, " \t\r");
        if (trimmed.len == 0) {
            return null;
        }
        const left_colon = trimmed[0] == ':';
        const right_colon = trimmed.len > 1 && trimmed[trimmed.len - 1] == ':';
        const dash_start: usize = if (left_colon) (1 as usize) else (0 as usize);
        const dash_end: usize = if (right_colon) (trimmed.len - 1) else trimmed.len;

        if (dash_start >= dash_end) {
            return null;
        }
        let i: usize = dash_start;
        while (i < dash_end) {
            if (trimmed[i] != '-') {
                return null;
            }
            i += 1;
        }

        if (left_colon && right_colon) {
            return MdTableAlign.Center;
        }
        if (left_colon) {
            return MdTableAlign.Left;
        }
        if (right_colon) {
            return MdTableAlign.Right;
        }
        return MdTableAlign.None;
    }

    /// Splits a pipe-delimited Markdown table row (`| a | b | c |`) into trimmed cells inside `out_cells`.
    /// Returns the number of cells extracted.
    pub static fn splitRow(line: []const u8, out_cells: [][]const u8): usize {
        let trimmed = std.mem.trim(u8, line, " \t\r");
        if (trimmed.len == 0) {
            return 0;
        }
        if (trimmed[0] == '|') {
            trimmed = trimmed[1..];
        }
        if (trimmed.len > 0 && trimmed[trimmed.len - 1] == '|') {
            trimmed = trimmed[0 .. trimmed.len - 1];
        }

        let count: usize = 0;
        let iter = std.mem.splitScalar(u8, trimmed, '|');
        while (raw_cell := iter.next()) {
            if (count >= out_cells.len) {
                break;
            }
            out_cells[count] = std.mem.trim(u8, raw_cell, " \t");
            count += 1;
        }
        return count;
    }

    /// Parses a GFM delimiter row (`| :--- | :---: | ---: |`) into `out_aligns`.
    /// Returns the column count if all cells are valid alignment specs, or `0` if invalid.
    pub static fn parseDelimiterRow(line: []const u8, out_aligns: []MdTableAlign): usize {
        let cells: [16][]const u8 = undefined;
        const max_cols = if (out_aligns.len < 16) out_aligns.len else 16;
        const count = MdTable.splitRow(line, cells[0..max_cols]);
        if (count == 0) {
            return 0;
        }
        let i: usize = 0;
        while (i < count) {
            if (align_val := MdTable.parseAlignSpec(cells[i])) {
                out_aligns[i] = align_val;
            } else {
                return 0;
            }
            i += 1;
        }
        return count;
    }

    pub static fn alignAttr(align_val: MdTableAlign): []const u8 {
        if (align_val == MdTableAlign.Left) {
            return " align=\"left\"";
        }
        if (align_val == MdTableAlign.Center) {
            return " align=\"center\"";
        }
        if (align_val == MdTableAlign.Right) {
            return " align=\"right\"";
        }
        return "";
    }

    /// Renders a multi-line GFM Markdown table into an HTML `<table><thead>...</thead><tbody>...</tbody></table>`.
    pub static fn renderHtml(md_table: []const u8, out_buf: []u8): []const u8 {
        let line_iter = std.mem.splitScalar(u8, md_table, '\n');
        let header_line: []const u8 = "";
        let delim_line: []const u8 = "";

        while (raw := line_iter.next()) {
            const trimmed = std.mem.trim(u8, raw, " \t\r");
            if (trimmed.len > 0) {
                header_line = trimmed;
                break;
            }
        }
        while (raw := line_iter.next()) {
            const trimmed = std.mem.trim(u8, raw, " \t\r");
            if (trimmed.len > 0) {
                delim_line = trimmed;
                break;
            }
        }
        if (header_line.len == 0 || delim_line.len == 0) {
            return out_buf[0..0];
        }

        let aligns: [16]MdTableAlign = undefined;
        const col_count = MdTable.parseDelimiterRow(delim_line, aligns[0..16]);
        if (col_count == 0) {
            return out_buf[0..0];
        }

        let header_cells: [16][]const u8 = undefined;
        const hdr_count = MdTable.splitRow(header_line, header_cells[0..16]);

        let pos: usize = 0;
        pos = MdTable.appendSlice(out_buf, pos, "<table><thead><tr>");
        let c: usize = 0;
        while (c < col_count) {
            pos = MdTable.appendSlice(out_buf, pos, "<th");
            pos = MdTable.appendSlice(out_buf, pos, MdTable.alignAttr(aligns[c]));
            pos = MdTable.appendSlice(out_buf, pos, ">");
            if (c < hdr_count) {
                pos = MdTable.appendSlice(out_buf, pos, header_cells[c]);
            }
            pos = MdTable.appendSlice(out_buf, pos, "</th>");
            c += 1;
        }
        pos = MdTable.appendSlice(out_buf, pos, "</tr></thead><tbody>");

        while (raw_row := line_iter.next()) {
            const row_trimmed = std.mem.trim(u8, raw_row, " \t\r");
            if (row_trimmed.len == 0) {
                continue;
            }
            let row_cells: [16][]const u8 = undefined;
            const cell_count = MdTable.splitRow(row_trimmed, row_cells[0..16]);
            pos = MdTable.appendSlice(out_buf, pos, "<tr>");
            let k: usize = 0;
            while (k < col_count) {
                pos = MdTable.appendSlice(out_buf, pos, "<td");
                pos = MdTable.appendSlice(out_buf, pos, MdTable.alignAttr(aligns[k]));
                pos = MdTable.appendSlice(out_buf, pos, ">");
                if (k < cell_count) {
                    pos = MdTable.appendSlice(out_buf, pos, row_cells[k]);
                }
                pos = MdTable.appendSlice(out_buf, pos, "</td>");
                k += 1;
            }
            pos = MdTable.appendSlice(out_buf, pos, "</tr>");
        }

        pos = MdTable.appendSlice(out_buf, pos, "</tbody></table>");
        return out_buf[0..pos];
    }
}
