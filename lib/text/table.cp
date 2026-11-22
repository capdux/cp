// Source: cp/lib/table/table.cp (ANSI & CJK-Aware Table Rendering Engine)
// Pure CP table builder & renderer supporting ASCII, Unicode Box, Rounded,
// Markdown GFM, and Borderless styles, column alignment, wrapping, sorting & footer.

const std = @import("std");
import { TextAlign, TableError, DisplayWidth } from "./tableWidth.cp";

type ByteSlice = []u8;
type LineList = [][]u8;

pub enum TableStyle {
    Ascii,
    Box,
    Rounded,
    Markdown,
    Borderless,
}

pub struct ColumnSpec {
    title: string = "",
    alignment: TextAlign = .Left,
    max_width: usize = 0,
    min_width: usize = 0,
}

struct BorderChars {
    tl: string,
    tr: string,
    bl: string,
    br: string,
    h: string,
    v: string,
    tm: string,
    bm: string,
    ml: string,
    mm: string,
    mr: string,
}

pub struct Table {
    allocator: std.mem.Allocator = undefined,
    style: TableStyle = .Box,
    padding: usize = 1,
    columns: std.ArrayList(ColumnSpec) = .empty,
    rows: std.ArrayList([][]u8) = .empty,
    footer: ([][]u8)? = null,

    pub static fn init(alloc: std.mem.Allocator): Table {
        return Table{
            allocator: alloc,
            style: .Box,
            padding: 1,
            columns: .empty,
            rows: .empty,
            footer: null,
        };
    }

    fn freeStringSlice(alloc: std.mem.Allocator, items: [][]u8) {
        for (item of items) {
            alloc.free(item);
        }
        if (items.len > 0) {
            alloc.free(items);
        }
    }

    pub fn deinit(*_) {
        for (col of _.columns.items) {
            _.allocator.free(col.title);
        }
        _.columns.deinit(_.allocator);

        for (r of _.rows.items) {
            Table.freeStringSlice(_.allocator, r);
        }
        _.rows.deinit(_.allocator);

        if (f := _.footer) {
            Table.freeStringSlice(_.allocator, f);
            _.footer = null;
        }
    }

    pub fn setStyle(*_, style: TableStyle) {
        _.style = style;
    }

    pub fn setPadding(*_, left_right: usize) {
        _.padding = left_right;
    }

    pub fn setColumns(*_, cols: []const ColumnSpec): ! {
        for (col of _.columns.items) {
            _.allocator.free(col.title);
        }
        _.columns.clearRetainingCapacity();

        for (c of cols) {
            const title_copy = _.allocator.dupe(u8, c.title)?;
            errdefer _.allocator.free(title_copy);
            _.columns.append(_.allocator, ColumnSpec{
                title: title_copy,
                alignment: c.alignment,
                max_width: c.max_width,
                min_width: c.min_width,
            })?;
        }
    }

    pub fn addColumn(*_, title: string, alignment: TextAlign): ! {
        const title_copy = _.allocator.dupe(u8, title)?;
        errdefer _.allocator.free(title_copy);
        _.columns.append(_.allocator, ColumnSpec{
            title: title_copy,
            alignment: alignment,
            max_width: 0,
            min_width: 0,
        })?;
    }

    fn dupeCells(alloc: std.mem.Allocator, cells: []const string): ([][]u8)! {
        let out = alloc.alloc(ByteSlice, cells.len)?;
        let filled: usize = 0;
        errdefer {
            let k: usize = 0;
            while (k < filled) : (k += 1) {
                alloc.free(out[k]);
            }
            alloc.free(out);
        }
        while (filled < cells.len) : (filled += 1) {
            out[filled] = alloc.dupe(u8, cells[filled])?;
        }
        return out;
    }

    pub fn addRow(*_, cells: []const string): ! {
        if (_.columns.items.len == 0 || cells.len != _.columns.items.len) {
            return TableError.ColumnMismatch;
        }
        const row_copy = Table.dupeCells(_.allocator, cells)?;
        errdefer Table.freeStringSlice(_.allocator, row_copy);
        _.rows.append(_.allocator, row_copy)?;
    }

    pub fn setFooter(*_, cells: []const string): ! {
        if (_.columns.items.len == 0 || cells.len != _.columns.items.len) {
            return TableError.ColumnMismatch;
        }
        const footer_copy = Table.dupeCells(_.allocator, cells)?;
        if (old := _.footer) {
            Table.freeStringSlice(_.allocator, old);
        }
        _.footer = footer_copy;
    }

    static fn compareStringsIgnoringAnsi(alloc: std.mem.Allocator, a: string, b: string): i32 {
        const pa = DisplayWidth.stripAnsi(alloc, a) catch return 0;
        defer alloc.free(pa);
        const pb = DisplayWidth.stripAnsi(alloc, b) catch return 0;
        defer alloc.free(pb);

        switch (std.mem.order(u8, pa, pb)) {
            .lt => return -1,
            .gt => return 1,
            .eq => return 0,
        }
    }

    static fn parseNumericCell(alloc: std.mem.Allocator, s: string): f64? {
        const plain = DisplayWidth.stripAnsi(alloc, s) catch return null;
        defer alloc.free(plain);
        const trimmed = std.mem.trim(u8, plain, " \t\r\n");
        if (trimmed.len == 0) {
            return null;
        }
        return std.fmt.parseFloat(f64, trimmed) catch null;
    }

    pub fn sortByColumn(*_, col_idx: usize, ascending: bool) {
        if (col_idx >= _.columns.items.len || _.rows.items.len < 2) {
            return;
        }
        const n = _.rows.items.len;
        let i: usize = 1;
        while (i < n) : (i += 1) {
            const key = _.rows.items[i];
            let j: usize = i;
            while (j > 0) {
                const cmp = Table.compareStringsIgnoringAnsi(
                    _.allocator,
                    _.rows.items[j - 1][col_idx],
                    key[col_idx],
                );
                const should_swap = if (ascending) cmp > 0 else cmp < 0;
                if (!should_swap) {
                    break;
                }
                _.rows.items[j] = _.rows.items[j - 1];
                j -= 1;
            }
            _.rows.items[j] = key;
        }
    }

    pub fn sortNumericByColumn(*_, col_idx: usize, ascending: bool) {
        if (col_idx >= _.columns.items.len || _.rows.items.len < 2) {
            return;
        }
        const n = _.rows.items.len;
        let i: usize = 1;
        while (i < n) : (i += 1) {
            const key = _.rows.items[i];
            const key_num = Table.parseNumericCell(_.allocator, key[col_idx]);
            let j: usize = i;
            while (j > 0) {
                const prev_num = Table.parseNumericCell(_.allocator, _.rows.items[j - 1][col_idx]);
                let should_swap = false;
                if (prev_num != null && key_num != null) {
                    should_swap = if (ascending) prev_num.? > key_num.? else prev_num.? < key_num.?;
                } else if (prev_num == null && key_num != null) {
                    should_swap = true;
                } else if (prev_num == null && key_num == null) {
                    const cmp = Table.compareStringsIgnoringAnsi(
                        _.allocator,
                        _.rows.items[j - 1][col_idx],
                        key[col_idx],
                    );
                    should_swap = if (ascending) cmp > 0 else cmp < 0;
                }
                if (!should_swap) {
                    break;
                }
                _.rows.items[j] = _.rows.items[j - 1];
                j -= 1;
            }
            _.rows.items[j] = key;
        }
    }

    static fn maxLineVisibleWidth(s: string): usize {
        let best: usize = 0;
        let start: usize = 0;
        let i: usize = 0;
        while (i <= s.len) {
            if (i == s.len || s[i] == '\n') {
                let end = i;
                if (end > start && s[end - 1] == '\r') {
                    end -= 1;
                }
                const w = DisplayWidth.visibleWidth(s[start..end]);
                if (w > best) {
                    best = w;
                }
                start = i + 1;
            }
            i += 1;
        }
        return best;
    }

    static fn bordersForStyle(style: TableStyle): BorderChars {
        switch (style) {
            .Ascii => return BorderChars{
                tl: "+", tr: "+", bl: "+", br: "+",
                h: "-", v: "|",
                tm: "+", bm: "+", ml: "+", mm: "+", mr: "+",
            },
            .Box => return BorderChars{
                tl: "┌", tr: "┐", bl: "└", br: "┘",
                h: "─", v: "│",
                tm: "┬", bm: "┴", ml: "├", mm: "┼", mr: "┤",
            },
            .Rounded => return BorderChars{
                tl: "╭", tr: "╮", bl: "╰", br: "╯",
                h: "─", v: "│",
                tm: "┬", bm: "┴", ml: "├", mm: "┼", mr: "┤",
            },
            .Markdown => return BorderChars{
                tl: "|", tr: "|", bl: "|", br: "|",
                h: "-", v: "|",
                tm: "|", bm: "|", ml: "|", mm: "|", mr: "|",
            },
            .Borderless => return BorderChars{
                tl: "", tr: "", bl: "", br: "",
                h: "─", v: "",
                tm: "", bm: "", ml: "", mm: "", mr: "",
            },
        }
    }

    static fn appendRepeat(
        alloc: std.mem.Allocator,
        out: *std.ArrayList(u8),
        s: string,
        count: usize,
    ): ! {
        let i: usize = 0;
        while (i < count) : (i += 1) {
            out.appendSlice(alloc, s)?;
        }
    }

    fn appendHorizontalRule(
        out: *std.ArrayList(u8),
        widths: []const usize,
        left: string,
        mid: string,
        right: string,
        fill: string,
    ): ! {
        out.appendSlice(_.allocator, left)?;
        let c: usize = 0;
        while (c < widths.len) : (c += 1) {
            if (c > 0) {
                out.appendSlice(_.allocator, mid)?;
            }
            Table.appendRepeat(_.allocator, out, fill, widths[c] + _.padding * 2)?;
        }
        out.appendSlice(_.allocator, right)?;
        out.append(_.allocator, '\n')?;
    }

    fn appendMarkdownSeparator(
        out: *std.ArrayList(u8),
        widths: []const usize,
    ): ! {
        out.appendSlice(_.allocator, "|")?;
        let c: usize = 0;
        while (c < widths.len) : (c += 1) {
            const w = widths[c];
            Table.appendRepeat(_.allocator, out, " ", _.padding)?;
            switch (_.columns.items[c].alignment) {
                .Left => {
                    out.append(_.allocator, ':')?;
                    const dashes = if (w > 1) w - 1 else 3;
                    Table.appendRepeat(_.allocator, out, "-", dashes)?;
                },
                .Right => {
                    const dashes = if (w > 1) w - 1 else 3;
                    Table.appendRepeat(_.allocator, out, "-", dashes)?;
                    out.append(_.allocator, ':')?;
                },
                .Center => {
                    out.append(_.allocator, ':')?;
                    const dashes = if (w > 2) w - 2 else 3;
                    Table.appendRepeat(_.allocator, out, "-", dashes)?;
                    out.append(_.allocator, ':')?;
                },
            }
            Table.appendRepeat(_.allocator, out, " ", _.padding)?;
            out.appendSlice(_.allocator, "|")?;
        }
        out.append(_.allocator, '\n')?;
    }

    fn appendRenderedRow(
        out: *std.ArrayList(u8),
        cells: []const string,
        widths: []const usize,
        borders: BorderChars,
    ): ! {
        const ncols = widths.len;
        let wrapped_cols = _.allocator.alloc(LineList, ncols)?;
        let init_count: usize = 0;
        defer {
            let k: usize = 0;
            while (k < init_count) : (k += 1) {
                DisplayWidth.freeLines(_.allocator, wrapped_cols[k]);
            }
            _.allocator.free(wrapped_cols);
        }

        let max_lines: usize = 1;
        while (init_count < ncols) : (init_count += 1) {
            const w = if (widths[init_count] > 0) widths[init_count] else 1;
            const lines = DisplayWidth.wrap(_.allocator, cells[init_count], w)?;
            wrapped_cols[init_count] = lines;
            if (lines.len > max_lines) {
                max_lines = lines.len;
            }
        }

        let line_idx: usize = 0;
        while (line_idx < max_lines) : (line_idx += 1) {
            if (_.style != .Borderless) {
                out.appendSlice(_.allocator, borders.v)?;
            }
            let c: usize = 0;
            while (c < ncols) : (c += 1) {
                if (c > 0) {
                    if (_.style == .Borderless) {
                        Table.appendRepeat(_.allocator, out, " ", if (_.padding > 0) _.padding * 2 else 2)?;
                    } else {
                        out.appendSlice(_.allocator, borders.v)?;
                    }
                }
                const cell_line = if (line_idx < wrapped_cols[c].len) wrapped_cols[c][line_idx] else "";
                const padded = DisplayWidth.pad(
                    _.allocator,
                    cell_line,
                    widths[c],
                    _.columns.items[c].alignment,
                )?;
                defer _.allocator.free(padded);

                if (_.style != .Borderless) {
                    Table.appendRepeat(_.allocator, out, " ", _.padding)?;
                }
                out.appendSlice(_.allocator, padded)?;
                if (_.style != .Borderless) {
                    Table.appendRepeat(_.allocator, out, " ", _.padding)?;
                }
            }
            if (_.style != .Borderless) {
                out.appendSlice(_.allocator, borders.v)?;
            }
            out.append(_.allocator, '\n')?;
        }
    }

    pub fn render(): ([]u8)! {
        const ncols = _.columns.items.len;
        if (ncols == 0) {
            return _.allocator.dupe(u8, "");
        }

        let widths = _.allocator.alloc(usize, ncols)?;
        defer _.allocator.free(widths);

        let c: usize = 0;
        while (c < ncols) : (c += 1) {
            widths[c] = Table.maxLineVisibleWidth(_.columns.items[c].title);
        }

        for (r of _.rows.items) {
            c = 0;
            while (c < ncols) : (c += 1) {
                const w = Table.maxLineVisibleWidth(r[c]);
                if (w > widths[c]) {
                    widths[c] = w;
                }
            }
        }

        if (f := _.footer) {
            c = 0;
            while (c < ncols) : (c += 1) {
                const w = Table.maxLineVisibleWidth(f[c]);
                if (w > widths[c]) {
                    widths[c] = w;
                }
            }
        }

        c = 0;
        while (c < ncols) : (c += 1) {
            const spec = _.columns.items[c];
            if (_.style == .Markdown) {
                const md_min: usize = if (spec.alignment == .Center) 5 else 4;
                if (widths[c] < md_min) {
                    widths[c] = md_min;
                }
            }
            if (spec.min_width > 0 && widths[c] < spec.min_width) {
                widths[c] = spec.min_width;
            }
            if (spec.max_width > 0 && widths[c] > spec.max_width) {
                widths[c] = spec.max_width;
            }
        }

        let header_cells = _.allocator.alloc(string, ncols)?;
        defer _.allocator.free(header_cells);
        c = 0;
        while (c < ncols) : (c += 1) {
            header_cells[c] = _.columns.items[c].title;
        }

        const borders = Table.bordersForStyle(_.style);
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.allocator);

        if (_.style == .Ascii || _.style == .Box || _.style == .Rounded) {
            _.appendHorizontalRule(&out, widths, borders.tl, borders.tm, borders.tr, borders.h)?;
        }

        _.appendRenderedRow(&out, header_cells, widths, borders)?;

        if (_.style == .Markdown) {
            _.appendMarkdownSeparator(&out, widths)?;
        } else if (_.style == .Borderless) {
            c = 0;
            while (c < ncols) : (c += 1) {
                if (c > 0) {
                    Table.appendRepeat(_.allocator, &out, " ", if (_.padding > 0) _.padding * 2 else 2)?;
                }
                Table.appendRepeat(_.allocator, &out, borders.h, widths[c])?;
            }
            out.append(_.allocator, '\n')?;
        } else {
            _.appendHorizontalRule(&out, widths, borders.ml, borders.mm, borders.mr, borders.h)?;
        }

        let row_cells = _.allocator.alloc(string, ncols)?;
        defer _.allocator.free(row_cells);

        for (r of _.rows.items) {
            c = 0;
            while (c < ncols) : (c += 1) {
                row_cells[c] = r[c];
            }
            _.appendRenderedRow(&out, row_cells, widths, borders)?;
        }

        if (f := _.footer) {
            if (_.style == .Ascii || _.style == .Box || _.style == .Rounded) {
                _.appendHorizontalRule(&out, widths, borders.ml, borders.mm, borders.mr, borders.h)?;
            } else if (_.style == .Borderless) {
                c = 0;
                while (c < ncols) : (c += 1) {
                    if (c > 0) {
                        Table.appendRepeat(_.allocator, &out, " ", if (_.padding > 0) _.padding * 2 else 2)?;
                    }
                    Table.appendRepeat(_.allocator, &out, borders.h, widths[c])?;
                }
                out.append(_.allocator, '\n')?;
            }
            c = 0;
            while (c < ncols) : (c += 1) {
                row_cells[c] = f[c];
            }
            _.appendRenderedRow(&out, row_cells, widths, borders)?;
        }

        if (_.style == .Ascii || _.style == .Box || _.style == .Rounded) {
            _.appendHorizontalRule(&out, widths, borders.bl, borders.bm, borders.br, borders.h)?;
        }

        return out.toOwnedSlice(_.allocator);
    }
}
