// @cap/table — Zero-Dependency CLI Table Renderer
// Clean-room pure-CP implementation modeled after npm `cli-table3` / `table`
// and Rust `comfy-table` / `tabled` (MIT License).
//
// Features:
// - 5 border presets: Unicode, Rounded, Ascii, Markdown (GFM), Compact
// - Column alignment: Left, Center, Right (including GFM alignment markers)
// - Display width calculation: skips ANSI CSI/OSC escape sequences, handles UTF-8 & CJK wide chars (= 2)
// - Ellipsis truncation (`truncateDisplay`) & cell padding (`padCell`) with ANSI reset safety
// - Dynamic multi-column `Table` builder supporting jagged rows, empty tables, and max column width

const std = @import("std");

/// Table border style preset.
pub enum BorderStyle: u8 {
    Unicode = 0,
    Rounded = 1,
    Ascii = 2,
    Markdown = 3,
    Compact = 4,
}

/// Column text alignment.
pub enum Align: u8 {
    Left = 0,
    Center = 1,
    Right = 2,
}

struct BorderChars {
    top_left: string,
    top_mid: string,
    top_right: string,
    top_horiz: string,
    mid_left: string,
    mid_mid: string,
    mid_right: string,
    mid_horiz: string,
    bot_left: string,
    bot_mid: string,
    bot_right: string,
    bot_horiz: string,
    left_vert: string,
    mid_vert: string,
    right_vert: string,
}

fn getBorderChars(style: BorderStyle): BorderChars {
    switch (style) {
        .Unicode => return BorderChars{
            top_left: "┌",
            top_mid: "┬",
            top_right: "┐",
            top_horiz: "─",
            mid_left: "├",
            mid_mid: "┼",
            mid_right: "┤",
            mid_horiz: "─",
            bot_left: "└",
            bot_mid: "┴",
            bot_right: "┘",
            bot_horiz: "─",
            left_vert: "│",
            mid_vert: "│",
            right_vert: "│",
        },
        .Rounded => return BorderChars{
            top_left: "╭",
            top_mid: "┬",
            top_right: "╮",
            top_horiz: "─",
            mid_left: "├",
            mid_mid: "┼",
            mid_right: "┤",
            mid_horiz: "─",
            bot_left: "╰",
            bot_mid: "┴",
            bot_right: "╯",
            bot_horiz: "─",
            left_vert: "│",
            mid_vert: "│",
            right_vert: "│",
        },
        .Ascii => return BorderChars{
            top_left: "+",
            top_mid: "+",
            top_right: "+",
            top_horiz: "-",
            mid_left: "+",
            mid_mid: "+",
            mid_right: "+",
            mid_horiz: "-",
            bot_left: "+",
            bot_mid: "+",
            bot_right: "+",
            bot_horiz: "-",
            left_vert: "|",
            mid_vert: "|",
            right_vert: "|",
        },
        .Markdown => return BorderChars{
            top_left: "|",
            top_mid: "|",
            top_right: "|",
            top_horiz: "-",
            mid_left: "|",
            mid_mid: "|",
            mid_right: "|",
            mid_horiz: "-",
            bot_left: "|",
            bot_mid: "|",
            bot_right: "|",
            bot_horiz: "-",
            left_vert: "|",
            mid_vert: "|",
            right_vert: "|",
        },
        .Compact => return BorderChars{
            top_left: "",
            top_mid: "",
            top_right: "",
            top_horiz: "",
            mid_left: "",
            mid_mid: "  ",
            mid_right: "",
            mid_horiz: "─",
            bot_left: "",
            bot_mid: "",
            bot_right: "",
            bot_horiz: "",
            left_vert: "",
            mid_vert: "  ",
            right_vert: "",
        },
    }
}

struct DecodedChar {
    cp: u32,
    len: usize,
}

/// Decodes a single UTF-8 codepoint starting at `s[pos]`.
fn decodeUtf8(s: string, pos: usize): DecodedChar {
    const b0 = s[pos];
    if (b0 < 0x80) {
        return DecodedChar{ cp: b0 as u32, len: 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && pos + 1 < s.len) {
        const b1 = s[pos + 1];
        if ((b1 & 0xC0) == 0x80) {
            const cp: u32 = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
            return DecodedChar{ cp: cp, len: 2 };
        }
    }
    if ((b0 & 0xF0) == 0xE0 && pos + 2 < s.len) {
        const b1 = s[pos + 1];
        const b2 = s[pos + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            const cp: u32 = (((b0 & 0x0F) as u32) << 12)
                | (((b1 & 0x3F) as u32) << 6)
                | ((b2 & 0x3F) as u32);
            return DecodedChar{ cp: cp, len: 3 };
        }
    }
    if ((b0 & 0xF8) == 0xF0 && pos + 3 < s.len) {
        const b1 = s[pos + 1];
        const b2 = s[pos + 2];
        const b3 = s[pos + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            const cp: u32 = (((b0 & 0x07) as u32) << 18)
                | (((b1 & 0x3F) as u32) << 12)
                | (((b2 & 0x3F) as u32) << 6)
                | ((b3 & 0x3F) as u32);
            return DecodedChar{ cp: cp, len: 4 };
        }
    }
    return DecodedChar{ cp: b0 as u32, len: 1 };
}

/// If `s[pos]` starts an ANSI escape sequence (CSI `\x1b[...` or OSC `\x1b]...`),
/// returns the byte length of the sequence; otherwise returns 0.
fn ansiSeqLen(s: string, pos: usize): usize {
    if (pos >= s.len || s[pos] != 0x1B) return 0;
    if (pos + 1 >= s.len) return 1;
    const next = s[pos + 1];
    // CSI sequence: ESC [ <params 0x30..0x3F>* <intermediates 0x20..0x2F>* <final 0x40..0x7E>
    if (next == '[') {
        let i = pos + 2;
        while (i < s.len && s[i] >= 0x30 && s[i] <= 0x3F) : (i += 1) {}
        while (i < s.len && s[i] >= 0x20 && s[i] <= 0x2F) : (i += 1) {}
        if (i < s.len && s[i] >= 0x40 && s[i] <= 0x7E) {
            i += 1;
        }
        return i - pos;
    }
    // OSC sequence: ESC ] ... (BEL | ESC \)
    if (next == ']') {
        let i = pos + 2;
        while (i < s.len) : (i += 1) {
            if (s[i] == 0x07) {
                return (i + 1) - pos;
            }
            if (s[i] == 0x1B && i + 1 < s.len && s[i + 1] == '\\') {
                return (i + 2) - pos;
            }
        }
        return s.len - pos;
    }
    // Two-byte Fe/Fp escape sequence
    return 2;
}

/// Returns the terminal column width (0, 1, or 2) of a Unicode codepoint.
fn codepointWidth(cp: u32): usize {
    // ASCII printable fast path
    if (cp >= 0x20 && cp <= 0x7E) return 1;
    // Control characters
    if (cp < 0x20 || (cp >= 0x7F && cp <= 0x9F)) return 0;
    // Common zero-width & combining marks
    if ((cp >= 0x0300 && cp <= 0x036F)
        || (cp >= 0x1AB0 && cp <= 0x1AFF)
        || (cp >= 0x1DC0 && cp <= 0x1DFF)
        || (cp >= 0x200B && cp <= 0x200F)
        || (cp >= 0x202A && cp <= 0x202E)
        || (cp >= 0x20D0 && cp <= 0x20FF)
        || (cp >= 0xFE00 && cp <= 0xFE0F)
        || (cp >= 0xFE20 && cp <= 0xFE2F)
        || cp == 0xFEFF)
    {
        return 0;
    }
    // East Asian Wide / Fullwidth & Emoji blocks (width = 2)
    if ((cp >= 0x1100 && cp <= 0x115F)
        || cp == 0x2329
        || cp == 0x232A
        || (cp >= 0x2E80 && cp <= 0x303E)
        || (cp >= 0x3040 && cp <= 0x33FF)
        || (cp >= 0x3400 && cp <= 0x4DBF)
        || (cp >= 0x4E00 && cp <= 0x9FFF)
        || (cp >= 0xA000 && cp <= 0xA4CF)
        || (cp >= 0xAC00 && cp <= 0xD7A3)
        || (cp >= 0xF900 && cp <= 0xFAFF)
        || (cp >= 0xFE10 && cp <= 0xFE19)
        || (cp >= 0xFE30 && cp <= 0xFE6F)
        || (cp >= 0xFF01 && cp <= 0xFF60)
        || (cp >= 0xFFE0 && cp <= 0xFFE6)
        || (cp >= 0x1F300 && cp <= 0x1F9FF)
        || (cp >= 0x1FA00 && cp <= 0x1FAFF)
        || (cp >= 0x20000 && cp <= 0x2FFFD)
        || (cp >= 0x30000 && cp <= 0x3FFFD))
    {
        return 2;
    }
    return 1;
}

/// Computes the visual terminal column width of `s`.
/// Skips ANSI CSI/OSC escape sequences (e.g. `\x1b[31m`) and counts CJK / fullwidth characters as width 2.
pub fn displayWidth(s: string): usize {
    let width: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const esc_len = ansiSeqLen(s, i);
        if (esc_len > 0) {
            i += esc_len;
            continue;
        }
        const decoded = decodeUtf8(s, i);
        width += codepointWidth(decoded.cp);
        i += decoded.len;
    }
    return width;
}

/// Truncates `s` to at most `max_width` visual columns, appending `"…"` (U+2026) when truncated.
/// Preserves ANSI escape sequences before the cut point and appends `\x1b[0m` if ANSI styling was present.
/// Caller owns the returned slice and must free it with `alloc.free(...)`.
pub fn truncateDisplay(
    alloc: std.mem.Allocator,
    s: string,
    max_width: usize,
): ([]u8) ! std.mem.Allocator.Error {
    const cur_total = displayWidth(s);
    if (cur_total <= max_width) {
        return alloc.dupe(u8, s);
    }
    if (max_width == 0) {
        return alloc.dupe(u8, "");
    }

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    const target_w: usize = max_width - 1; // Reserve 1 column for "…"
    let used_w: usize = 0;
    let saw_ansi = false;
    let i: usize = 0;
    while (i < s.len) {
        const esc_len = ansiSeqLen(s, i);
        if (esc_len > 0) {
            out.appendSlice(alloc, s[i..(i + esc_len)])?;
            saw_ansi = true;
            i += esc_len;
            continue;
        }
        const decoded = decodeUtf8(s, i);
        const cw = codepointWidth(decoded.cp);
        if (used_w + cw > target_w) {
            break;
        }
        out.appendSlice(alloc, s[i..(i + decoded.len)])?;
        used_w += cw;
        i += decoded.len;
    }

    out.appendSlice(alloc, "…")?;
    if (saw_ansi) {
        out.appendSlice(alloc, "\x1b[0m")?;
    }
    return out.toOwnedSlice(alloc);
}

fn appendRepeat(
    alloc: std.mem.Allocator,
    out: *std.ArrayList(u8),
    chunk: string,
    count: usize,
): void ! std.mem.Allocator.Error {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        out.appendSlice(alloc, chunk)?;
    }
}

/// Pads `s` with ASCII spaces to reach `width` visual columns using `alignment` (`Left`, `Center`, or `Right`).
/// If `displayWidth(s) >= width`, returns a duplicate of `s` unchanged.
/// Caller owns the returned slice and must free it with `alloc.free(...)`.
pub fn padCell(
    alloc: std.mem.Allocator,
    s: string,
    width: usize,
    alignment: Align,
): ([]u8) ! std.mem.Allocator.Error {
    const w = displayWidth(s);
    if (w >= width) {
        return alloc.dupe(u8, s);
    }
    const pad = width - w;
    let left_pad: usize = 0;
    let right_pad: usize = 0;
    switch (alignment) {
        .Left => {
            left_pad = 0;
            right_pad = pad;
        },
        .Right => {
            left_pad = pad;
            right_pad = 0;
        },
        .Center => {
            left_pad = pad / 2;
            right_pad = pad - left_pad;
        },
    }

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);
    appendRepeat(alloc, *out, " ", left_pad)?;
    out.appendSlice(alloc, s)?;
    appendRepeat(alloc, *out, " ", right_pad)?;
    return out.toOwnedSlice(alloc);
}

/// Dynamic CLI table builder supporting headers, jagged multi-column rows,
/// per-column alignment, 5 border presets, and max column width truncation.
pub struct Table {
    alloc: std.mem.Allocator,
    headers: std.ArrayList(string) = .empty,
    rows: std.ArrayList([]string) = .empty,
    aligns: std.ArrayList(Align) = .empty,
    style: BorderStyle = BorderStyle.Unicode,
    max_col_width: usize = 0,

    /// Initializes a new empty table backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): Table {
        return Table{
            alloc: alloc,
            headers: .empty,
            rows: .empty,
            aligns: .empty,
            style: BorderStyle.Unicode,
            max_col_width: 0,
        };
    }

    /// Releases all internal storage allocated by the table.
    pub fn deinit(*_) {
        _.headers.deinit(_.alloc);
        let i: usize = 0;
        while (i < _.rows.items.len) : (i += 1) {
            _.alloc.free(_.rows.items[i]);
        }
        _.rows.deinit(_.alloc);
        _.aligns.deinit(_.alloc);
    }

    /// Sets (or replaces) the header cells for the table.
    pub fn setHeaders(*_, headers: []const string): void ! std.mem.Allocator.Error {
        _.headers.clearRetainingCapacity();
        _.headers.appendSlice(_.alloc, headers)?;
    }

    /// Appends a data row. Rows may have fewer or more columns than headers.
    pub fn addRow(*_, row: []const string): void ! std.mem.Allocator.Error {
        const duped = _.alloc.dupe(string, row)?;
        errdefer _.alloc.free(duped);
        _.rows.append(_.alloc, duped)?;
    }

    /// Sets the horizontal alignment for 0-based column `col`.
    pub fn setAlign(*_, col: usize, alignment: Align): void ! std.mem.Allocator.Error {
        while (_.aligns.items.len <= col) {
            _.aligns.append(_.alloc, Align.Left)?;
        }
        _.aligns.items[col] = alignment;
    }

    /// Selects one of the 5 built-in border styles.
    pub fn setStyle(*_, style: BorderStyle) {
        _.style = style;
    }

    /// Sets the maximum visual width per column (`0` = unlimited).
    pub fn setMaxColWidth(*_, max_width: usize) {
        _.max_col_width = max_width;
    }

    fn colAlign(col: usize): Align {
        if (col < _.aligns.items.len) return _.aligns.items[col];
        return Align.Left;
    }

    fn computeColCount(): usize {
        let cols: usize = _.headers.items.len;
        let r: usize = 0;
        while (r < _.rows.items.len) : (r += 1) {
            if (_.rows.items[r].len > cols) {
                cols = _.rows.items[r].len;
            }
        }
        return cols;
    }

    static fn renderHorizBorder(
        alloc: std.mem.Allocator,
        out: *std.ArrayList(u8),
        left: string,
        mid: string,
        right: string,
        horiz: string,
        widths: []const usize,
        padded: bool,
    ): void ! std.mem.Allocator.Error {
        out.appendSlice(alloc, left)?;
        let c: usize = 0;
        while (c < widths.len) : (c += 1) {
            if (c > 0) {
                out.appendSlice(alloc, mid)?;
            }
            const span = if (padded) widths[c] + 2 else widths[c];
            appendRepeat(alloc, out, horiz, span)?;
        }
        out.appendSlice(alloc, right)?;
    }

    fn renderMarkdownSep(
        alloc: std.mem.Allocator,
        out: *std.ArrayList(u8),
        widths: []const usize,
    ): void ! std.mem.Allocator.Error {
        out.appendSlice(alloc, "|")?;
        let c: usize = 0;
        while (c < widths.len) : (c += 1) {
            const w = if (widths[c] < 3) (3 as usize) else widths[c];
            const al = _.colAlign(c);
            out.appendSlice(alloc, " ")?;
            switch (al) {
                .Left => {
                    out.appendSlice(alloc, ":")?;
                    appendRepeat(alloc, out, "-", w - 1)?;
                },
                .Center => {
                    out.appendSlice(alloc, ":")?;
                    appendRepeat(alloc, out, "-", w - 2)?;
                    out.appendSlice(alloc, ":")?;
                },
                .Right => {
                    appendRepeat(alloc, out, "-", w - 1)?;
                    out.appendSlice(alloc, ":")?;
                },
            }
            out.appendSlice(alloc, " |")?;
        }
    }

    fn renderDataRow(
        alloc: std.mem.Allocator,
        out: *std.ArrayList(u8),
        bc: BorderChars,
        cells: []const string,
        widths: []const usize,
        is_compact: bool,
    ): void ! std.mem.Allocator.Error {
        if (!is_compact) {
            out.appendSlice(alloc, bc.left_vert)?;
        }
        let c: usize = 0;
        while (c < widths.len) : (c += 1) {
            if (c > 0) {
                out.appendSlice(alloc, bc.mid_vert)?;
            }
            const raw: string = if (c < cells.len) cells[c] else "";
            const al = _.colAlign(c);

            const truncated = if (_.max_col_width > 0)
                truncateDisplay(alloc, raw, widths[c])?
            else
                alloc.dupe(u8, raw)?;
            defer alloc.free(truncated);

            const padded = padCell(alloc, truncated, widths[c], al)?;
            defer alloc.free(padded);

            if (!is_compact) {
                out.appendSlice(alloc, " ")?;
                out.appendSlice(alloc, padded)?;
                out.appendSlice(alloc, " ")?;
            } else {
                out.appendSlice(alloc, padded)?;
            }
        }
        if (!is_compact) {
            out.appendSlice(alloc, bc.right_vert)?;
        }
    }

    /// Renders the complete formatted table as a newly allocated UTF-8 string owned by `alloc`.
    /// Returns an empty string `""` when the table has no columns (no headers and no rows).
    pub fn render(alloc: std.mem.Allocator): ([]u8) ! std.mem.Allocator.Error {
        const num_cols = _.computeColCount();
        if (num_cols == 0) {
            return alloc.dupe(u8, "");
        }

        let widths = alloc.alloc(usize, num_cols)?;
        defer alloc.free(widths);
        let c: usize = 0;
        while (c < num_cols) : (c += 1) {
            widths[c] = 0;
        }

        // Measure header widths
        let hc: usize = 0;
        while (hc < _.headers.items.len) : (hc += 1) {
            const w = displayWidth(_.headers.items[hc]);
            if (w > widths[hc]) widths[hc] = w;
        }

        // Measure row cell widths
        let r: usize = 0;
        while (r < _.rows.items.len) : (r += 1) {
            const row = _.rows.items[r];
            let rc: usize = 0;
            while (rc < row.len) : (rc += 1) {
                const w = displayWidth(row[rc]);
                if (w > widths[rc]) widths[rc] = w;
            }
        }

        // Clamp to max_col_width and enforce GFM minimum width of 3
        let ci: usize = 0;
        while (ci < num_cols) : (ci += 1) {
            if (_.max_col_width > 0 && widths[ci] > _.max_col_width) {
                widths[ci] = _.max_col_width;
            }
            if (_.style == BorderStyle.Markdown && widths[ci] < 3) {
                widths[ci] = 3;
            }
        }

        const bc = getBorderChars(_.style);
        const is_box = (_.style == BorderStyle.Unicode
            || _.style == BorderStyle.Rounded
            || _.style == BorderStyle.Ascii);
        const is_markdown = _.style == BorderStyle.Markdown;
        const is_compact = _.style == BorderStyle.Compact;

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let has_output = false;

        // Top border (box styles only)
        if (is_box) {
            Table.renderHorizBorder(alloc, *out, bc.top_left, bc.top_mid, bc.top_right, bc.top_horiz, widths, true)?;
            has_output = true;
        }

        // Header row & header separator
        if (_.headers.items.len > 0) {
            if (has_output) out.append(alloc, '\n')?;
            _.renderDataRow(alloc, *out, bc, _.headers.items, widths, is_compact)?;
            has_output = true;

            if (is_box) {
                if (_.rows.items.len > 0) {
                    out.append(alloc, '\n')?;
                    Table.renderHorizBorder(alloc, *out, bc.mid_left, bc.mid_mid, bc.mid_right, bc.mid_horiz, widths, true)?;
                }
            } else if (is_markdown) {
                out.append(alloc, '\n')?;
                _.renderMarkdownSep(alloc, *out, widths)?;
            } else if (is_compact) {
                if (_.rows.items.len > 0) {
                    out.append(alloc, '\n')?;
                    Table.renderHorizBorder(alloc, *out, bc.mid_left, bc.mid_mid, bc.mid_right, bc.mid_horiz, widths, false)?;
                }
            }
        }

        // Data rows
        let ri: usize = 0;
        while (ri < _.rows.items.len) : (ri += 1) {
            if (has_output) out.append(alloc, '\n')?;
            _.renderDataRow(alloc, *out, bc, _.rows.items[ri], widths, is_compact)?;
            has_output = true;
        }

        // Bottom border (box styles only)
        if (is_box) {
            out.append(alloc, '\n')?;
            Table.renderHorizBorder(alloc, *out, bc.bot_left, bc.bot_mid, bc.bot_right, bc.bot_horiz, widths, true)?;
        }

        return out.toOwnedSlice(alloc);
    }
}
