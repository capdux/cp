// Source: Absorbed from /data/dev/bun/src/md/ansi_renderer.cp | Synced: 2026-11-23T21:59:09Z
// lib/md/ansi_renderer.cp
// Industrial-Grade Markdown -> ANSI Terminal Renderer
// Supports colors, word-wrapping, East-Asian visible width, Unicode box tables with SGR/OSC8 state tracking,
// OSC 8 hyperlinks, Kitty Graphics Protocol (t=d / t=f), and code block syntax highlighting.

const std = @import("std");
import { decodeEntityToUtf8, decodeUtf8, utf8SeqLen } from "./helpers.cp";
import {
    Align,
    BLOCK_FENCED_CODE,
    BlockType,
    Renderer,
    RendererVTable,
    SpanDetail,
    SpanType,
    TextType,
    alignmentFromData,
    isTaskChecked,
    taskMarkFromData,
} from "./types.cp";

type SliceList = std.ArrayListUnmanaged(string);
type CellStateList = std.ArrayListUnmanaged(CellAnsiState);

pub struct AnsiTheme {
    light: bool = false,
    columns: u16 = 80,
    colors: bool = true,
    hyperlinks: bool = false,
    kitty_graphics: bool = false,
    remote_image_paths: (*const std.StringHashMapUnmanaged(string))? = null,
    image_base_dir: ([]const u8)? = null,
}

pub struct ImageUrlCollector {
    urls: std.ArrayListUnmanaged(string) = .empty,
    allocator: std.mem.Allocator,

    pub static fn init(allocator: std.mem.Allocator): ImageUrlCollector {
        return ImageUrlCollector{ allocator: allocator };
    }

    pub fn deinit(*_) {
        for (u of _.urls.items) _.allocator.free(u);
        _.urls.deinit(_.allocator);
    }

    pub fn renderer(*_): Renderer {
        return Renderer{ ptr: _, vtable: &collector_vtable };
    }

    pub static fn noopEnterBlock(p: *anyopaque, bt: BlockType, d: u32, fl: u32): ! {
        * = p; * = bt; * = d; * = fl;
    }
    pub static fn noopLeaveBlock(p: *anyopaque, bt: BlockType, d: u32): ! {
        * = p; * = bt; * = d;
    }
    pub static fn noopLeaveSpan(p: *anyopaque, st: SpanType): ! {
        * = p; * = st;
    }
    pub static fn noopText(p: *anyopaque, tt: TextType, c: []const u8): ! {
        * = p; * = tt; * = c;
    }

    pub static fn enterSpanImpl(ptr: *anyopaque, span_type: SpanType, detail: SpanDetail): ! {
        if (span_type != .img) return;
        const self_ptr: *ImageUrlCollector = unsafe { @ptrCast(@alignCast(ptr)) };
        if (detail.href.len == 0) return;
        const owned = self_ptr.allocator.dupe(u8, detail.href)?;
        errdefer self_ptr.allocator.free(owned);
        self_ptr.urls.append(self_ptr.allocator, owned)?;
    }
}

const collector_vtable = RendererVTable{
    enterBlock: ImageUrlCollector.noopEnterBlock,
    leaveBlock: ImageUrlCollector.noopLeaveBlock,
    enterSpan: ImageUrlCollector.enterSpanImpl,
    leaveSpan: ImageUrlCollector.noopLeaveSpan,
    text: ImageUrlCollector.noopText,
};

pub enum BlockContextKind: u8 {
    quote = 0,
    ul = 1,
    ol = 2,
    li = 3,
}

pub struct BlockContext {
    kind: BlockContextKind = .quote,
    data: u32 = 0,
    index: u32 = 0,
    indent: u32 = 0,
}

pub struct AnsiTableCell {
    content: []const u8 = "",
    alignment: Align = .default,
}

pub struct AnsiTableRow {
    cells: []AnsiTableCell = [],
    is_header: bool = false,
}

const SPAN_EM: u32 = 1 << 0;
const SPAN_STRONG: u32 = 1 << 1;
const SPAN_DEL: u32 = 1 << 2;
const SPAN_U: u32 = 1 << 3;
const SPAN_CODE: u32 = 1 << 4;

struct InlineStyle {
    flag: u32 = 0,
    on: []const u8 = "",
    off: []const u8 = "",

    static fn of(span_type: SpanType): InlineStyle? {
        return switch (span_type) {
            .em => InlineStyle{ flag: SPAN_EM, on: "\x1b[3m", off: "\x1b[23m" },
            .strong => InlineStyle{ flag: SPAN_STRONG, on: "\x1b[1m", off: "\x1b[22m" },
            .u => InlineStyle{ flag: SPAN_U, on: "\x1b[4m", off: "\x1b[24m" },
            .del => InlineStyle{ flag: SPAN_DEL, on: "\x1b[9m", off: "\x1b[29m" },
            else => null,
        };
    }
}

pub struct AnsiOutputBuffer {
    list: std.ArrayListUnmanaged(u8) = .empty,
    allocator: std.mem.Allocator,
    oom: bool = false,

    pub fn write(*_, data: []const u8) {
        if (_.oom) return;
        _.list.appendSlice(_.allocator, data) catch {
            _.oom = true;
        };
    }

    pub fn writeByte(*_, b: u8) {
        if (_.oom) return;
        _.list.append(_.allocator, b) catch {
            _.oom = true;
        };
    }
}

struct BoxChars {
    h: []const u8 = "",
    v: []const u8 = "",
    tl: []const u8 = "",
    tr: []const u8 = "",
    bl: []const u8 = "",
    br: []const u8 = "",
    t: []const u8 = "",
    b: []const u8 = "",
    ml: []const u8 = "",
    mr: []const u8 = "",
    x: []const u8 = "",
}

const CELL_BOLD: u8 = 1 << 0;
const CELL_ITALIC: u8 = 1 << 1;
const CELL_UNDERLINE: u8 = 1 << 2;
const CELL_STRIKE: u8 = 1 << 3;
const CELL_DIM: u8 = 1 << 4;

pub struct CellAnsiState {
    flags: u8 = 0,
    fg: ([]const u8)? = null,
    bg: ([]const u8)? = null,
    link: ([]const u8)? = null,

    pub fn hasAny(): bool {
        return _.flags != 0 || _.fg != null || _.bg != null || _.link != null;
    }

    pub fn emitOpens(out: *AnsiOutputBuffer) {
        if ((_.flags & CELL_BOLD) != 0) out.write("\x1b[1m");
        if ((_.flags & CELL_DIM) != 0) out.write("\x1b[2m");
        if ((_.flags & CELL_ITALIC) != 0) out.write("\x1b[3m");
        if ((_.flags & CELL_UNDERLINE) != 0) out.write("\x1b[4m");
        if ((_.flags & CELL_STRIKE) != 0) out.write("\x1b[9m");
        if (f := _.fg) out.write(f);
        if (b := _.bg) out.write(b);
        if (l := _.link) out.write(l);
    }

    pub fn emitCloses(out: *AnsiOutputBuffer) {
        if (_.hasAny()) out.write("\x1b[0m");
        if (_.link != null) out.write("\x1b]8;;\x1b\\");
    }

    pub fn scan(*_, bytes: []const u8) {
        let i: usize = 0;
        while (i < bytes.len) {
            if (bytes[i] != 0x1b) {
                i += 1;
                continue;
            }
            if (i + 1 >= bytes.len) return;
            if (bytes[i + 1] == '[') {
                const seq_start = i;
                let j = i + 2;
                while (j < bytes.len) : (j += 1) {
                    const c = bytes[j];
                    if (c >= 0x40 && c <= 0x7e) break;
                }
                if (j >= bytes.len) return;
                if (bytes[j] == 'm') {
                    const seq = bytes[seq_start .. j + 1];
                    const params = bytes[seq_start + 2 .. j];
                    _.applySgr(seq, params);
                }
                i = j + 1;
                continue;
            }
            if (bytes[i + 1] == ']') {
                const seq_start = i;
                let j = i + 2;
                while (j < bytes.len) : (j += 1) {
                    if (bytes[j] == 0x07) {
                        j += 1;
                        break;
                    }
                    if (bytes[j] == 0x1b && j + 1 < bytes.len && bytes[j + 1] == '\\') {
                        j += 2;
                        break;
                    }
                }
                const seq = bytes[seq_start..j];
                if (seq.len >= 5 && std.mem.startsWith(u8, seq, "\x1b]8;")) {
                    const body = seq[4..];
                    const body_end: usize = if (body.len >= 2 && body[body.len - 2] == 0x1b && body[body.len - 1] == '\\')
                        body.len - 2
                    else if (body.len >= 1 && body[body.len - 1] == 0x07)
                        body.len - 1
                    else
                        body.len;
                    const body_stripped = body[0..body_end];
                    if (semi := std.mem.indexOfScalar(u8, body_stripped, ';')) {
                        const url = body_stripped[semi + 1 ..];
                        if (url.len == 0) {
                            _.link = null;
                        } else {
                            _.link = seq;
                        }
                    }
                }
                i = j;
                continue;
            }
            i += 1;
        }
    }

    pub fn applySgr(*_, seq: []const u8, params: []const u8) {
        if (params.len == 0) {
            _.flags = 0;
            _.fg = null;
            _.bg = null;
            return;
        }
        let iter = std.mem.splitScalar(u8, params, ';');
        while (p := iter.next()) {
            const n = std.fmt.parseInt(u32, p, 10) catch continue;
            if (n == 0) {
                _.flags = 0;
                _.fg = null;
                _.bg = null;
            } else if (n == 1) {
                _.flags |= CELL_BOLD;
            } else if (n == 2) {
                _.flags |= CELL_DIM;
            } else if (n == 3) {
                _.flags |= CELL_ITALIC;
            } else if (n == 4) {
                _.flags |= CELL_UNDERLINE;
            } else if (n == 9) {
                _.flags |= CELL_STRIKE;
            } else if (n == 22) {
                _.flags &= ~(CELL_BOLD | CELL_DIM);
            } else if (n == 23) {
                _.flags &= ~CELL_ITALIC;
            } else if (n == 24) {
                _.flags &= ~CELL_UNDERLINE;
            } else if (n == 29) {
                _.flags &= ~CELL_STRIKE;
            } else if ((n >= 30 && n <= 37) || (n >= 90 && n <= 97)) {
                _.fg = seq;
            } else if (n == 38) {
                _.fg = seq;
                while (iter.next() != null) {}
                return;
            } else if (n == 39) {
                _.fg = null;
            } else if ((n >= 40 && n <= 47) || (n >= 100 && n <= 107)) {
                _.bg = seq;
            } else if (n == 48) {
                _.bg = seq;
                while (iter.next() != null) {}
                return;
            } else if (n == 49) {
                _.bg = null;
            }
        }
    }
}

pub struct AnsiRenderer {
    out: AnsiOutputBuffer,
    allocator: std.mem.Allocator,
    src_text: []const u8 = "",
    theme: AnsiTheme = AnsiTheme{},
    block_stack: std.ArrayListUnmanaged(BlockContext) = .empty,
    span_flags: u32 = 0,
    link_href: ([]const u8)? = null,
    link_depth: u32 = 0,
    image_depth: u32 = 0,
    image_alt: std.ArrayListUnmanaged(u8) = .empty,
    image_src: ([]const u8)? = null,
    image_title: ([]const u8)? = null,
    col: u32 = 0,
    in_code_block: bool = false,
    code_lang: []const u8 = "",
    code_fenced: bool = false,
    code_buf: std.ArrayListUnmanaged(u8) = .empty,
    heading_level: u8 = 0,
    heading_buf: std.ArrayListUnmanaged(u8) = .empty,
    table_cells: std.ArrayListUnmanaged(AnsiTableCell) = .empty,
    table_rows: std.ArrayListUnmanaged(AnsiTableRow) = .empty,
    table_cell_buf: std.ArrayListUnmanaged(u8) = .empty,
    in_thead: bool = false,
    in_cell: bool = false,
    cell_align: Align = .default,
    last_was_newline: bool = true,
    blank_emitted: bool = false,

    pub static fn init(allocator: std.mem.Allocator, src_text: []const u8, theme: AnsiTheme): AnsiRenderer {
        let r = AnsiRenderer{
            out: AnsiOutputBuffer{ list: .empty, allocator: allocator, oom: false },
            allocator: allocator,
            src_text: src_text,
            theme: theme,
        };
        r.out.list.ensureTotalCapacity(allocator, src_text.len + src_text.len / 2) catch {};
        return r;
    }

    pub fn deinit(*_) {
        _.out.list.deinit(_.allocator);
        _.block_stack.deinit(_.allocator);
        _.image_alt.deinit(_.allocator);
        _.code_buf.deinit(_.allocator);
        _.heading_buf.deinit(_.allocator);
        if (s := _.link_href) _.allocator.free(s);
        if (s := _.image_src) _.allocator.free(s);
        if (s := _.image_title) _.allocator.free(s);
        for (row of _.table_rows.items) {
            for (cell of row.cells) _.allocator.free(cell.content);
            _.allocator.free(row.cells);
        }
        _.table_rows.deinit(_.allocator);
        for (cell of _.table_cells.items) _.allocator.free(cell.content);
        _.table_cells.deinit(_.allocator);
        _.table_cell_buf.deinit(_.allocator);
    }

    pub fn toOwnedSlice(*_): ([]u8)! {
        if (_.out.oom) return error.OutOfMemory;
        return _.out.list.toOwnedSlice(_.allocator);
    }

    pub fn renderer(*_): Renderer {
        return Renderer{ ptr: _, vtable: &ansi_renderer_vtable };
    }

    pub static fn enterBlockImpl(ptr: *anyopaque, block_type: BlockType, data: u32, flags: u32): ! {
        const r: *AnsiRenderer = unsafe { @ptrCast(@alignCast(ptr)) };
        r.enterBlock(block_type, data, flags);
    }

    pub static fn leaveBlockImpl(ptr: *anyopaque, block_type: BlockType, data: u32): ! {
        const r: *AnsiRenderer = unsafe { @ptrCast(@alignCast(ptr)) };
        r.leaveBlock(block_type, data);
    }

    pub static fn enterSpanImpl(ptr: *anyopaque, span_type: SpanType, detail: SpanDetail): ! {
        const r: *AnsiRenderer = unsafe { @ptrCast(@alignCast(ptr)) };
        r.enterSpan(span_type, detail);
    }

    pub static fn leaveSpanImpl(ptr: *anyopaque, span_type: SpanType): ! {
        const r: *AnsiRenderer = unsafe { @ptrCast(@alignCast(ptr)) };
        r.leaveSpan(span_type);
    }

    pub static fn textImpl(ptr: *anyopaque, text_type: TextType, content: []const u8): ! {
        const r: *AnsiRenderer = unsafe { @ptrCast(@alignCast(ptr)) };
        r.text(text_type, content);
    }

    pub fn enterBlock(*_, block_type: BlockType, data: u32, flags: u32) {
        switch (block_type) {
            .doc => {},
            .quote => {
                _.ensureBlankLine();
                _.block_stack.append(_.allocator, BlockContext{ kind: .quote, indent: 2 }) catch {
                    _.out.oom = true;
                };
            },
            .ul => {
                _.ensureNewline();
                _.block_stack.append(_.allocator, BlockContext{ kind: .ul, data: data, indent: 2 }) catch {
                    _.out.oom = true;
                };
            },
            .ol => {
                _.ensureNewline();
                _.block_stack.append(_.allocator, BlockContext{ kind: .ol, data: data, indent: 3 }) catch {
                    _.out.oom = true;
                };
            },
            .li => {
                _.ensureNewline();
                _.writeIndent();
                let entry = BlockContext{ kind: .li };
                const parent_list = _.findParentList();
                const task_mark = taskMarkFromData(data);
                if (list := parent_list) {
                    entry.index = list.index;
                    list.index += 1;
                }
                let num_buf: [12]u8 = undefined;
                let glyph: []const u8 = if (_.theme.colors) "• " else "* ";
                let glyph_color: []const u8 = ansiColor(.cyan);
                if (task_mark != 0) {
                    const checked = isTaskChecked(task_mark);
                    glyph = if (_.theme.colors)
                        (if (checked) "☒ " else "☐ ")
                    else
                        (if (checked) "[x] " else "[ ] ");
                    glyph_color = if (checked) ansiColor(.green) else ansiColor(.dim);
                } else if (parent_list != null && parent_list.?.kind == .ol) {
                    const num = parent_list.?.data + entry.index;
                    glyph = std.fmt.bufPrint(&num_buf, "{d}. ", .{num}) catch "? ";
                }
                _.writeStyled(glyph_color, glyph);
                _.writeStyled("\x1b[0m", "");
                entry.indent = @intCast(visibleWidth(glyph));
                _.block_stack.append(_.allocator, entry) catch {
                    _.out.oom = true;
                };
            },
            .hr => {
                _.ensureBlankLine();
                _.writeIndent();
                const indent_cols = _.currentIndent();
                const base_w: u32 = if (_.theme.columns == 0) 60 else @min(_.theme.columns as u32, 60);
                const width: u32 = base_w -| indent_cols;
                let i: u32 = 0;
                const dash = if (_.theme.colors) "─" else "-";
                _.writeStyled(ansiColor(.dim), "");
                while (i < width) : (i += 1) _.writeRaw(dash);
                _.writeStyled("\x1b[0m", "");
                _.writeRaw("\n");
                _.last_was_newline = true;
                _.col = 0;
            },
            .h => {
                _.ensureBlankLine();
                _.heading_level = @intCast(data & 0xFF);
                _.heading_buf.clearRetainingCapacity();
            },
            .code => {
                _.ensureBlankLine();
                _.in_code_block = true;
                _.code_fenced = (flags & BLOCK_FENCED_CODE) != 0;
                _.code_buf.clearRetainingCapacity();
                if (_.code_fenced) {
                    _.code_lang = extractLanguage(_.src_text, data);
                } else {
                    _.code_lang = "";
                }
            },
            .html => _.ensureNewline(),
            .p => {
                let in_li = false;
                if (_.block_stack.items.len > 0) {
                    in_li = _.block_stack.items[_.block_stack.items.len - 1].kind == .li;
                }
                if (!(in_li && _.col > 0)) {
                    _.ensureBlankLine();
                    _.writeIndent();
                }
            },
            .table => {
                _.ensureBlankLine();
                _.in_thead = false;
                for (row of _.table_rows.items) {
                    for (cell of row.cells) _.allocator.free(cell.content);
                    _.allocator.free(row.cells);
                }
                _.table_rows.clearRetainingCapacity();
                _.table_cells.clearRetainingCapacity();
            },
            .thead => _.in_thead = true,
            .tbody => _.in_thead = false,
            .tr => _.table_cells.clearRetainingCapacity(),
            .th, .td => {
                _.in_cell = true;
                _.cell_align = alignmentFromData(data);
                _.table_cell_buf.clearRetainingCapacity();
            },
        }
    }

    pub fn leaveBlock(*_, block_type: BlockType, unused_data: u32) {
        * = unused_data;
        switch (block_type) {
            .doc => {},
            .quote, .ul, .ol, .li => {
                if (_.block_stack.items.len > 0) {
                    * = _.block_stack.pop();
                }
                _.ensureNewline();
            },
            .hr => {},
            .h => {
                _.flushHeading();
                _.heading_level = 0;
            },
            .code => {
                _.flushCodeBlock();
                _.in_code_block = false;
                _.code_lang = "";
            },
            .html => _.ensureNewline(),
            .p => {
                _.writeStyled("\x1b[0m", "");
                _.ensureNewline();
                _.col = 0;
            },
            .table => {
                _.flushTable();
                _.ensureNewline();
            },
            .thead, .tbody => {},
            .tr => {
                const cells = _.allocator.dupe(AnsiTableCell, _.table_cells.items) catch {
                    for (c of _.table_cells.items) _.allocator.free(c.content);
                    _.table_cells.clearRetainingCapacity();
                    _.out.oom = true;
                    return;
                };
                _.table_rows.append(_.allocator, AnsiTableRow{
                    cells: cells,
                    is_header: _.in_thead,
                }) catch {
                    for (c of cells) _.allocator.free(c.content);
                    _.allocator.free(cells);
                    _.table_cells.clearRetainingCapacity();
                    _.out.oom = true;
                    return;
                };
                _.table_cells.clearRetainingCapacity();
            },
            .th, .td => {
                _.in_cell = false;
                const owned = _.allocator.dupe(u8, _.table_cell_buf.items) catch {
                    _.out.oom = true;
                    return;
                };
                _.table_cells.append(_.allocator, AnsiTableCell{
                    content: owned,
                    alignment: _.cell_align,
                }) catch {
                    _.allocator.free(owned);
                    _.out.oom = true;
                };
            },
        }
    }

    pub fn enterSpan(*_, span_type: SpanType, detail: SpanDetail) {
        switch (span_type) {
            .em, .strong, .u, .del => {
                const s = InlineStyle.of(span_type).?;
                _.span_flags |= s.flag;
                _.writeStyled(s.on, "");
            },
            .code => {
                _.span_flags |= SPAN_CODE;
                _.writeStyled(codeSpanOpen(_.theme.light), "");
            },
            .a => {
                _.link_depth += 1;
                if (_.link_depth == 1) {
                    _.link_href = resolveHrefOpt(detail, _.allocator);
                    if (_.theme.colors && _.theme.hyperlinks) {
                        if (href := _.link_href) {
                            _.writeRawNoColor("\x1b]8;;");
                            _.writeRawNoColor(href);
                            _.writeRawNoColor("\x1b\\");
                        }
                    }
                    _.writeStyled(ansiColor(.blue), "");
                    _.writeStyled("\x1b[4m", "");
                }
            },
            .img => {
                _.image_depth += 1;
                if (_.image_depth == 1) {
                    _.image_src = _.allocator.dupe(u8, detail.href) catch null;
                    _.image_title = _.allocator.dupe(u8, detail.title) catch null;
                    _.image_alt.clearRetainingCapacity();
                }
            },
            .wikilink => _.writeStyled(ansiColor(.blue), "[["),
            .latexmath => _.writeStyled(ansiColor(.magenta), "$"),
            .latexmath_display => _.writeStyled(ansiColor(.magenta), "$$"),
        }
    }

    pub fn leaveSpan(*_, span_type: SpanType) {
        switch (span_type) {
            .em, .strong, .u, .del => {
                const s = InlineStyle.of(span_type).?;
                _.span_flags &= ~s.flag;
                _.writeStyled(s.off, "");
                if (_.heading_level > 0) _.reapplyStyles();
            },
            .code => {
                _.span_flags &= ~SPAN_CODE;
                _.writeStyled("\x1b[39m\x1b[49m", "");
                _.reapplyStyles();
            },
            .a => {
                if (_.link_depth == 1) {
                    _.link_depth = 0;
                    const had_href = _.link_href != null;
                    _.writeStyled("\x1b[24m\x1b[39m", "");
                    _.reapplyStyles();
                    if (_.theme.colors && _.theme.hyperlinks) {
                        if (had_href) _.writeRawNoColor("\x1b]8;;\x1b\\");
                    } else if (href := _.link_href) {
                        if (href.len > 0 && _.image_depth == 0) {
                            _.writeStyled(ansiColor(.dim), " (");
                            _.writeStyled("", href);
                            _.writeStyled(ansiColor(.dim), ")");
                            _.writeStyled("\x1b[39m\x1b[22m", "");
                            _.reapplyStyles();
                        }
                    }
                    if (href := _.link_href) _.allocator.free(href);
                    _.link_href = null;
                } else if (_.link_depth > 0) {
                    _.link_depth -= 1;
                }
            },
            .img => {
                if (_.image_depth == 1) {
                    _.emitImage();
                    if (src := _.image_src) _.allocator.free(src);
                    if (title := _.image_title) _.allocator.free(title);
                    _.image_src = null;
                    _.image_title = null;
                    _.image_alt.clearRetainingCapacity();
                }
                if (_.image_depth > 0) _.image_depth -= 1;
            },
            .wikilink, .latexmath, .latexmath_display => {
                const suffix = switch (span_type) {
                    .wikilink => "]]",
                    .latexmath => "$",
                    .latexmath_display => "$$",
                    else => "",
                };
                _.writeNoWrap(suffix);
                _.writeStyled("\x1b[39m", "");
                _.reapplyStyles();
            },
        }
    }

    pub fn text(*_, text_type: TextType, content: []const u8) {
        switch (text_type) {
            .null_char => _.writeContent("\xEF\xBF\xBD"),
            .br => _.writeContent("\n"),
            .softbr => _.writeContent(" "),
            .html => {
                _.writeStyled(ansiColor(.dim), "");
                _.writeContent(content);
                _.writeStyled("\x1b[22m", "");
                _.reapplyStyles();
            },
            .entity => {
                let buf: [8]u8 = undefined;
                const decoded = decodeEntityToUtf8(content, &buf) ?? content;
                _.writeContent(decoded);
            },
            .code => _.writeStyled("", content),
            .latexmath => _.writeStyled("", content),
            else => _.writeContent(content),
        }
    }

    fn writeContent(*_, data: []const u8) {
        if (_.image_depth > 0) {
            _.image_alt.appendSlice(_.allocator, data) catch {
                _.out.oom = true;
            };
            return;
        }
        if (_.in_code_block) {
            _.code_buf.appendSlice(_.allocator, data) catch {
                _.out.oom = true;
            };
            return;
        }
        if (_.heading_level > 0) {
            _.heading_buf.appendSlice(_.allocator, data) catch {
                _.out.oom = true;
            };
            return;
        }
        if (_.in_cell) {
            _.table_cell_buf.appendSlice(_.allocator, data) catch {
                _.out.oom = true;
            };
            return;
        }
        _.writeWrapped(data);
    }

    fn writeWrapped(*_, data: []const u8) {
        if (_.theme.columns == 0) {
            let start: usize = 0;
            let i: usize = 0;
            while (i < data.len) : (i += 1) {
                if (data[i] == '\n') {
                    _.writeRaw(data[start .. i + 1]);
                    _.col = 0;
                    _.last_was_newline = true;
                    _.writeIndent();
                    start = i + 1;
                }
            }
            if (start < data.len) {
                _.writeRaw(data[start..]);
                _.updateColFromText(data[start..]);
            }
            return;
        }
        const indent = _.currentIndent();
        const max = _.theme.columns as u32;
        let i: usize = 0;
        while (i < data.len) {
            const c = data[i];
            if (c == '\n') {
                _.writeRaw("\n");
                _.last_was_newline = true;
                _.col = 0;
                i += 1;
                _.writeIndent();
                continue;
            }
            if (c == ' ' && _.col >= max) {
                _.writeRaw("\n");
                _.last_was_newline = true;
                _.col = 0;
                _.writeIndent();
                i += 1;
                while (i < data.len && data[i] == ' ') i += 1;
                continue;
            }
            let j = i;
            while (j < data.len && data[j] != ' ' && data[j] != '\n') : (j += 1) {}
            const word = data[i..j];
            const word_width: u32 = @intCast(visibleWidth(word));
            const avail = max -| indent;
            if (avail > 0 && word_width > avail) {
                let rest = word;
                while (rest.len > 0) {
                    const r = (max -| _.col) as usize;
                    if (r == 0) {
                        _.wrapBreak();
                        continue;
                    }
                    let cut = visibleIndexAt(rest, r);
                    if (cut == 0) cut = @min(rest.len, utf8SeqLenSafe(rest[0]));
                    _.writeRaw(rest[0..cut]);
                    _.col += @as(u32, @intCast(visibleWidth(rest[0..cut])));
                    _.last_was_newline = false;
                    rest = rest[cut..];
                    if (rest.len > 0) _.wrapBreak();
                }
            } else {
                if (_.col != 0 && _.col + word_width > max && _.col > indent) {
                    _.wrapBreak();
                }
                _.writeRaw(word);
                _.col += word_width;
                _.last_was_newline = (word.len == 0);
            }
            i = j;
            if (i < data.len && data[i] == ' ') {
                let k = i;
                while (k < data.len && data[k] == ' ') k += 1;
                let m = k;
                while (m < data.len && data[m] != ' ' && data[m] != '\n') : (m += 1) {}
                const next_word_width: u32 = @intCast(visibleWidth(data[k..m]));
                const next_avail = max -| indent;
                if (_.col != 0 && _.col + 1 + next_word_width > max && _.col > indent && next_word_width <= next_avail) {
                    _.writeRaw("\n");
                    _.last_was_newline = true;
                    _.col = 0;
                    _.writeIndent();
                } else {
                    _.writeRaw(" ");
                    _.col += 1;
                }
                i = k;
            }
        }
    }

    fn emitInline(*_, bytes: []const u8) {
        if (bytes.len == 0) return;
        if (_.image_depth > 0) {
            let i: usize = 0;
            while (i < bytes.len) {
                if (bytes[i] == 0x1b) {
                    i += 1;
                    if (i < bytes.len && bytes[i] == '[') {
                        i += 1;
                        while (i < bytes.len && (bytes[i] < 0x40 || bytes[i] > 0x7e)) : (i += 1) {}
                        if (i < bytes.len) i += 1;
                    } else if (i < bytes.len && bytes[i] == ']') {
                        i += 1;
                        while (i < bytes.len) : (i += 1) {
                            if (bytes[i] == 0x07) {
                                i += 1;
                                break;
                            }
                            if (bytes[i] == 0x1b && i + 1 < bytes.len && bytes[i + 1] == '\\') {
                                i += 2;
                                break;
                            }
                        }
                    }
                    continue;
                }
                const start = i;
                while (i < bytes.len && bytes[i] != 0x1b) : (i += 1) {}
                _.image_alt.appendSlice(_.allocator, bytes[start..i]) catch {
                    _.out.oom = true;
                    return;
                };
            }
            return;
        }
        if (_.in_cell) {
            _.table_cell_buf.appendSlice(_.allocator, bytes) catch {
                _.out.oom = true;
            };
            return;
        }
        if (_.heading_level > 0) {
            _.heading_buf.appendSlice(_.allocator, bytes) catch {
                _.out.oom = true;
            };
            return;
        }
        _.out.write(bytes);
    }

    fn writeStyled(*_, prefix: []const u8, text_: []const u8) {
        const in_main_flow = !_.in_cell && _.heading_level == 0 && !_.in_code_block && _.image_depth == 0;

        if (in_main_flow && _.theme.columns > 0 && text_.len > 0) {
            const tw: u32 = @intCast(visibleWidth(text_));
            if (tw > 0) {
                const max = _.theme.columns as u32;
                const indent = _.currentIndent();
                if (_.col > indent && _.col + tw > max) {
                    _.wrapBreak();
                }
            }
        }

        if (_.theme.colors && prefix.len > 0) {
            _.emitInline(prefix);
        }
        if (text_.len == 0) return;

        if (!in_main_flow) {
            _.emitInline(text_);
            return;
        }

        const max = _.theme.columns as u32;
        if (max == 0) {
            _.emitInline(text_);
            _.col += @as(u32, @intCast(visibleWidth(text_)));
            _.last_was_newline = false;
            return;
        }

        let rest = text_;
        while (rest.len > 0) {
            const room = (max -| _.col) as usize;
            if (room == 0) {
                if (_.col <= _.currentIndent()) {
                    _.emitInline(rest);
                    _.col += @as(u32, @intCast(visibleWidth(rest)));
                    _.last_was_newline = false;
                    return;
                }
                _.wrapBreak();
                continue;
            }
            const cut = visibleIndexAt(rest, room);
            if (cut == rest.len) {
                _.emitInline(rest);
                _.col += @as(u32, @intCast(visibleWidth(rest)));
                _.last_was_newline = false;
                return;
            }
            if (cut == 0) {
                if (_.col <= _.currentIndent()) {
                    const adv = visibleIndexAt(rest, 2);
                    const one = if (adv == 0) @min(rest.len, utf8SeqLenSafe(rest[0])) else adv;
                    _.emitInline(rest[0..one]);
                    _.col += @as(u32, @intCast(visibleWidth(rest[0..one])));
                    _.last_was_newline = false;
                    rest = rest[one..];
                    if (rest.len > 0) _.wrapBreak();
                    continue;
                }
                _.wrapBreak();
                continue;
            }
            _.emitInline(rest[0..cut]);
            _.col += @as(u32, @intCast(visibleWidth(rest[0..cut])));
            _.last_was_newline = false;
            rest = rest[cut..];
            _.wrapBreak();
        }
    }

    fn wrapBreak(*_) {
        const has_style = _.span_flags != 0 || _.link_depth > 0;
        if (_.theme.colors && has_style) _.out.write("\x1b[39m\x1b[49m");
        _.out.writeByte('\n');
        _.last_was_newline = true;
        _.col = 0;
        _.writeIndent();
        if (has_style) _.reapplyStyles();
    }

    fn writeRaw(*_, data: []const u8) {
        if (data.len == 0) return;
        _.emitInline(data);
        _.last_was_newline = (data[data.len - 1] == '\n');
    }

    fn writeNoWrap(*_, text_: []const u8) {
        if (text_.len == 0) return;
        _.emitInline(text_);
        if (!_.in_cell && _.heading_level == 0 && !_.in_code_block && _.image_depth == 0) {
            _.col += @as(u32, @intCast(visibleWidth(text_)));
            _.last_was_newline = false;
        }
    }

    fn writeRawNoColor(*_, data: []const u8) {
        if (!_.theme.colors || data.len == 0 || _.image_depth > 0) return;
        if (_.in_cell) {
            _.table_cell_buf.appendSlice(_.allocator, data) catch {
                _.out.oom = true;
            };
            return;
        }
        if (_.heading_level > 0) {
            _.heading_buf.appendSlice(_.allocator, data) catch {
                _.out.oom = true;
            };
            return;
        }
        _.out.write(data);
    }

    fn reapplyStyles(*_) {
        if (!_.theme.colors) return;
        if (_.heading_level > 0) {
            _.emitInline("\x1b[1m");
            _.emitInline(headingColor(_.heading_level));
        }
        if ((_.span_flags & SPAN_STRONG) != 0) _.emitInline("\x1b[1m");
        if ((_.span_flags & SPAN_EM) != 0) _.emitInline("\x1b[3m");
        if ((_.span_flags & SPAN_U) != 0) _.emitInline("\x1b[4m");
        if ((_.span_flags & SPAN_DEL) != 0) _.emitInline("\x1b[9m");
        if ((_.span_flags & SPAN_CODE) != 0) _.emitInline(codeSpanOpen(_.theme.light));
        if (_.link_depth > 0) {
            _.emitInline(ansiColor(.blue));
            _.emitInline("\x1b[4m");
        }
    }

    fn writeIndent(*_) {
        _.blank_emitted = false;
        let quote_bars: u32 = 0;
        let other_indent: u32 = 0;
        for (entry of _.block_stack.items) {
            if (entry.kind == .quote) {
                quote_bars += 1;
            } else {
                other_indent += entry.indent;
            }
        }
        const bar = if (_.theme.colors) "│ " else "| ";
        if (_.theme.colors && quote_bars > 0) {
            _.out.write("\x1b[38;5;242m");
        }
        let i: u32 = 0;
        while (i < quote_bars) : (i += 1) {
            _.out.write(bar);
            _.col += 2;
        }
        if (_.theme.colors && quote_bars > 0) {
            _.out.write("\x1b[39m");
            _.reapplyStyles();
        }
        let j: u32 = 0;
        while (j < other_indent) : (j += 1) {
            _.out.write(" ");
            _.col += 1;
        }
    }

    fn currentIndent(*_): u32 {
        let total: u32 = 0;
        for (entry of _.block_stack.items) {
            total += if (entry.kind == .quote) (2 as u32) else entry.indent;
        }
        return total;
    }

    fn updateColFromText(*_, data: []const u8) {
        let start: usize = 0;
        let i: usize = 0;
        while (i < data.len) : (i += 1) {
            if (data[i] == '\n') {
                _.col = 0;
                _.last_was_newline = true;
                start = i + 1;
            }
        }
        if (start < data.len) {
            _.col += @as(u32, @intCast(visibleWidth(data[start..])));
            _.last_was_newline = false;
        }
    }

    fn writeQuoteBars(*_) {
        let quote_bars: u32 = 0;
        for (entry of _.block_stack.items) {
            if (entry.kind == .quote) quote_bars += 1;
        }
        if (quote_bars == 0) return;
        const bar = if (_.theme.colors) "│" else "|";
        if (_.theme.colors) _.out.write("\x1b[38;5;242m");
        let i: u32 = 0;
        while (i < quote_bars) : (i += 1) {
            _.out.write(bar);
            _.col += 1;
        }
        if (_.theme.colors) _.out.write("\x1b[39m");
    }

    fn ensureNewline(*_) {
        if (!_.last_was_newline) {
            _.out.writeByte('\n');
            _.col = 0;
            _.last_was_newline = true;
        }
    }

    fn ensureBlankLine(*_) {
        _.ensureNewline();
        if (_.blank_emitted) return;
        if (_.out.list.items.len > 0) {
            const items = _.out.list.items;
            if (items.len >= 2 && items[items.len - 1] == '\n' && items[items.len - 2] != '\n') {
                _.writeQuoteBars();
                _.out.writeByte('\n');
                _.col = 0;
                _.blank_emitted = true;
            } else if (items.len == 1 && items[0] == '\n') {
                // single newline
            } else if (items.len >= 1 && items[items.len - 1] != '\n') {
                _.writeQuoteBars();
                _.out.writeByte('\n');
                _.col = 0;
                _.blank_emitted = true;
            }
        }
    }

    fn findParentList(*_): (*BlockContext)? {
        const len = _.block_stack.items.len;
        if (len == 0) return null;
        let i: usize = len;
        while (i > 0) {
            i -= 1;
            const entry = &_.block_stack.items[i];
            if (entry.kind == .ul || entry.kind == .ol) return entry;
        }
        return null;
    }

    fn flushHeading(*_) {
        const level = _.heading_level;
        _.heading_level = 0;
        defer {
            _.heading_level = level;
        }
        const content = _.heading_buf.items;
        _.writeIndent();
        if (_.theme.colors) {
            _.out.write("\x1b[1m");
            _.out.write(headingColor(level));
        }
        _.out.write(content);
        if (_.theme.colors) _.out.write("\x1b[0m");
        _.out.writeByte('\n');
        _.last_was_newline = true;
        _.col = 0;
        if (level == 1 || level == 2) {
            _.writeIndent();
            const text_w = @max(visibleWidth(content), 3);
            const indent_cols = _.currentIndent() as usize;
            const width = if (_.theme.columns == 0)
                text_w
            else
                @min(text_w, (_.theme.columns as usize) -| indent_cols);
            if (_.theme.colors) _.out.write(ansiColor(.dim));
            const underline_ch = if (_.theme.colors)
                (if (level == 1) "═" else "─")
            else
                (if (level == 1) "=" else "-");
            let i: usize = 0;
            while (i < width) : (i += 1) _.out.write(underline_ch);
            if (_.theme.colors) _.out.write("\x1b[0m");
            _.out.writeByte('\n');
            _.last_was_newline = true;
            _.col = 0;
        }
    }

    static fn headingColor(level: u8): []const u8 {
        return switch (level) {
            1 => ansiColor(.magenta),
            2 => ansiColor(.cyan),
            3 => ansiColor(.yellow),
            4 => ansiColor(.green),
            5 => ansiColor(.blue),
            else => ansiColor(.white),
        };
    }

    fn flushCodeBlock(*_) {
        const src = _.code_buf.items;
        const body = if (src.len > 0 && src[src.len - 1] == '\n') src[0 .. src.len - 1] else src;

        const top_border = if (_.theme.colors) "┌─ " else "+- ";
        const top_bare = if (_.theme.colors) "┌─" else "+-";
        const side = if (_.theme.colors) "│ " else "| ";
        const bottom = if (_.theme.colors) "└─" else "+-";

        if (_.theme.colors) _.out.write(ansiColor(.dim));
        _.writeIndent();
        if (_.code_lang.len > 0) {
            _.out.write(top_border);
            if (_.theme.colors) _.out.write("\x1b[0m\x1b[2m\x1b[3m");
            _.out.write(_.code_lang);
            if (_.theme.colors) _.out.write("\x1b[0m");
        } else {
            if (_.theme.colors) _.out.write(ansiColor(.dim));
            _.out.write(top_bare);
            if (_.theme.colors) _.out.write("\x1b[0m");
        }
        _.out.writeByte('\n');
        _.last_was_newline = true;

        const highlight = isCodeHighlightLang(_.code_lang);
        let line_start: usize = 0;
        let i: usize = 0;
        while (i <= body.len) : (i += 1) {
            if (i == body.len || body[i] == '\n') {
                const line = body[line_start..i];
                _.writeIndent();
                if (_.theme.colors) _.out.write(ansiColor(.dim));
                _.out.write(side);
                if (_.theme.colors) _.out.write("\x1b[0m");
                if (highlight && _.theme.colors) {
                    _.writeHighlightedCodeLine(line);
                } else {
                    _.out.write(line);
                }
                _.out.writeByte('\n');
                _.last_was_newline = true;
                line_start = i + 1;
            }
        }
        _.writeIndent();
        if (_.theme.colors) _.out.write(ansiColor(.dim));
        _.out.write(bottom);
        if (_.theme.colors) _.out.write("\x1b[0m");
        _.out.writeByte('\n');
        _.col = 0;
        _.last_was_newline = true;
    }

    fn writeHighlightedCodeLine(*_, line: []const u8) {
        let i: usize = 0;
        while (i < line.len) {
            if (i + 1 < line.len && line[i] == '/' && line[i + 1] == '/') {
                _.out.write("\x1b[2m");
                _.out.write(line[i..]);
                _.out.write("\x1b[0m");
                return;
            }
            if (line[i] == '"' || line[i] == '\'' || line[i] == '`') {
                const q = line[i];
                let end = i + 1;
                while (end < line.len && line[end] != q) {
                    if (line[end] == '\\' && end + 1 < line.len) {
                        end += 2;
                    } else {
                        end += 1;
                    }
                }
                if (end < line.len) end += 1;
                _.out.write("\x1b[32m");
                _.out.write(line[i..end]);
                _.out.write("\x1b[0m");
                i = end;
                continue;
            }
            if (std.ascii.isDigit(line[i])) {
                let end = i;
                while (end < line.len && (std.ascii.isAlphanumeric(line[end]) || line[end] == '.' || line[end] == '_')) end += 1;
                _.out.write("\x1b[33m");
                _.out.write(line[i..end]);
                _.out.write("\x1b[0m");
                i = end;
                continue;
            }
            if (std.ascii.isAlphabetic(line[i]) || line[i] == '_') {
                let end = i;
                while (end < line.len && (std.ascii.isAlphanumeric(line[end]) || line[end] == '_')) end += 1;
                const tok = line[i..end];
                if (isHighlightKeyword(tok)) {
                    _.out.write("\x1b[35m");
                    _.out.write(tok);
                    _.out.write("\x1b[0m");
                } else {
                    _.out.write(tok);
                }
                i = end;
                continue;
            }
            _.out.writeByte(line[i]);
            i += 1;
        }
    }

    fn flushTable(*_) {
        if (_.table_rows.items.len == 0) return;

        let col_count: usize = 0;
        for (row of _.table_rows.items) col_count = @max(col_count, row.cells.len);
        if (col_count == 0) return;

        let widths = _.allocator.alloc(usize, col_count) catch {
            _.out.oom = true;
            return;
        };
        defer _.allocator.free(widths);
        @memset(widths, 3);

        let aligns = _.allocator.alloc(Align, col_count) catch {
            _.out.oom = true;
            return;
        };
        defer _.allocator.free(aligns);
        @memset(aligns, Align.default);

        for (row of _.table_rows.items) {
            let i: usize = 0;
            while (i < row.cells.len) : (i += 1) {
                const cell = row.cells[i];
                widths[i] = @max(widths[i], visibleWidth(cell.content));
                if (aligns[i] == .default) aligns[i] = cell.alignment;
            }
        }

        if (_.theme.columns > 0) {
            const indent = _.currentIndent() as usize;
            let total: usize = indent + 1;
            for (w of widths) total += w + 3;
            const budget = _.theme.columns as usize;
            while (total > budget) {
                let widest: usize = 0;
                let wi: usize = 0;
                while (wi < widths.len) : (wi += 1) {
                    if (widths[wi] > widths[widest]) widest = wi;
                }
                if (widths[widest] <= 3) break;
                widths[widest] -= 1;
                total -= 1;
            }
        }

        const chars = _.boxChars();

        _.writeIndent();
        if (_.theme.colors) _.out.write(ansiColor(.dim));
        _.out.write(chars.tl);
        let ti: usize = 0;
        while (ti < widths.len) : (ti += 1) {
            let j: usize = 0;
            while (j < widths[ti] + 2) : (j += 1) _.out.write(chars.h);
            _.out.write(if (ti == widths.len - 1) chars.tr else chars.t);
        }
        if (_.theme.colors) _.out.write("\x1b[0m");
        _.out.writeByte('\n');
        _.last_was_newline = true;

        let has_separated_header = false;
        for (row of _.table_rows.items) {
            _.writeRowCells(row, widths, aligns);
            if (row.is_header && !has_separated_header) {
                _.writeTableSeparator(widths);
                has_separated_header = true;
            }
        }

        _.writeIndent();
        if (_.theme.colors) _.out.write(ansiColor(.dim));
        _.out.write(chars.bl);
        let bi: usize = 0;
        while (bi < widths.len) : (bi += 1) {
            let j: usize = 0;
            while (j < widths[bi] + 2) : (j += 1) _.out.write(chars.h);
            _.out.write(if (bi == widths.len - 1) chars.br else chars.b);
        }
        if (_.theme.colors) _.out.write("\x1b[0m");
        _.out.writeByte('\n');
        _.last_was_newline = true;
        _.col = 0;

        for (row of _.table_rows.items) {
            for (cell of row.cells) _.allocator.free(cell.content);
            _.allocator.free(row.cells);
        }
        _.table_rows.clearRetainingCapacity();
    }

    static fn lastWordBreakOutsideEscapes(bytes: []const u8): usize? {
        let last: usize? = null;
        let i: usize = 0;
        while (i < bytes.len) {
            const c = bytes[i];
            if (c == 0x1b && i + 1 < bytes.len) {
                const next = bytes[i + 1];
                if (next == '[') {
                    i += 2;
                    while (i < bytes.len) : (i += 1) {
                        if (bytes[i] >= 0x40 && bytes[i] <= 0x7e) {
                            i += 1;
                            break;
                        }
                    }
                    continue;
                }
                if (next == ']') {
                    i += 2;
                    while (i < bytes.len) : (i += 1) {
                        if (bytes[i] == 0x07) {
                            i += 1;
                            break;
                        }
                        if (bytes[i] == 0x1b && i + 1 < bytes.len && bytes[i + 1] == '\\') {
                            i += 2;
                            break;
                        }
                    }
                    continue;
                }
                i += 2;
                continue;
            }
            if (c == ' ') last = i;
            i += 1;
        }
        return last;
    }

    fn writeRowCells(*_, row: AnsiTableRow, widths: []const usize, aligns: []const Align) {
        const chars = _.boxChars();

        let segments = _.allocator.alloc(SliceList, widths.len) catch {
            _.out.oom = true;
            return;
        };
        defer {
            let si: usize = 0;
            while (si < segments.len) : (si += 1) segments[si].deinit(_.allocator);
            _.allocator.free(segments);
        }
        @memset(segments, SliceList.empty);

        let state_at = _.allocator.alloc(CellStateList, widths.len) catch {
            _.out.oom = true;
            return;
        };
        defer {
            let sai: usize = 0;
            while (sai < state_at.len) : (sai += 1) state_at[sai].deinit(_.allocator);
            _.allocator.free(state_at);
        }
        @memset(state_at, CellStateList.empty);

        let lines: usize = 1;
        let ci: usize = 0;
        while (ci < widths.len) : (ci += 1) {
            const w = widths[ci];
            const i = ci;
            const content = if (i < row.cells.len) row.cells[i].content else "";
            let rest = content;
            let state = CellAnsiState{};
            while (rest.len > 0) {
                let cut = visibleIndexAt(rest, w);
                if (cut < rest.len) {
                    if (sp := lastWordBreakOutsideEscapes(rest[0..cut])) {
                        if (sp > 0) cut = sp;
                    }
                }
                if (cut == 0) cut = @min(rest.len, utf8SeqLenSafe(rest[0]));
                state_at[i].append(_.allocator, state) catch {
                    _.out.oom = true;
                    return;
                };
                segments[i].append(_.allocator, rest[0..cut]) catch {
                    _.out.oom = true;
                    return;
                };
                state.scan(rest[0..cut]);
                rest = rest[cut..];
                let skipped_start: usize = 0;
                while (skipped_start < rest.len && rest[skipped_start] == ' ') skipped_start += 1;
                if (skipped_start > 0) {
                    state.scan(rest[0..skipped_start]);
                    rest = rest[skipped_start..];
                }
            }
            lines = @max(lines, segments[i].items.len);
        }

        let line: usize = 0;
        while (line < lines) : (line += 1) {
            _.writeIndent();
            if (_.theme.colors) _.out.write(ansiColor(.dim));
            _.out.write(chars.v);
            if (_.theme.colors) _.out.write("\x1b[0m");
            let col_idx: usize = 0;
            while (col_idx < widths.len) : (col_idx += 1) {
                const w = widths[col_idx];
                const i = col_idx;
                const seg: []const u8 = if (line < segments[i].items.len) segments[i].items[line] else "";
                const opens: CellAnsiState = if (line < state_at[i].items.len) state_at[i].items[line] else CellAnsiState{};
                _.out.writeByte(' ');
                if (row.is_header && _.theme.colors) _.out.write("\x1b[1m");
                if (_.theme.colors && line > 0) opens.emitOpens(&_.out);
                const cw = visibleWidth(seg);
                const cell_align = if (i < row.cells.len) row.cells[i].alignment else Align.default;
                const alignment = if (cell_align != .default) cell_align else aligns[i];
                const pad = w -| cw;
                const left: usize = switch (alignment) {
                    .right => pad,
                    .center => pad / 2,
                    else => 0,
                };
                const right: usize = pad - left;
                _.writePadding(left);
                _.out.write(seg);
                if (_.theme.colors) {
                    let end_state = opens;
                    end_state.scan(seg);
                    end_state.emitCloses(&_.out);
                    if (row.is_header) _.out.write("\x1b[0m");
                }
                _.writePadding(right);
                _.out.writeByte(' ');
                if (_.theme.colors) _.out.write(ansiColor(.dim));
                _.out.write(chars.v);
                if (_.theme.colors) _.out.write("\x1b[0m");
            }
            _.out.writeByte('\n');
        }
        _.last_was_newline = true;
    }

    fn writeTableSeparator(*_, widths: []const usize) {
        const chars = _.boxChars();
        _.writeIndent();
        if (_.theme.colors) _.out.write(ansiColor(.dim));
        _.out.write(chars.ml);
        let mi: usize = 0;
        while (mi < widths.len) : (mi += 1) {
            let j: usize = 0;
            while (j < widths[mi] + 2) : (j += 1) _.out.write(chars.h);
            _.out.write(if (mi == widths.len - 1) chars.mr else chars.x);
        }
        if (_.theme.colors) _.out.write("\x1b[0m");
        _.out.writeByte('\n');
        _.last_was_newline = true;
    }

    fn boxChars(*_): BoxChars {
        return if (_.theme.colors)
            BoxChars{
                h: "─", v: "│", tl: "┌", tr: "┐", bl: "└", br: "┘",
                t: "┬", b: "┴", ml: "├", mr: "┤", x: "┼",
            }
        else
            BoxChars{
                h: "-", v: "|", tl: "+", tr: "+", bl: "+", br: "+",
                t: "+", b: "+", ml: "+", mr: "+", x: "+",
            };
    }

    fn writePadding(*_, n: usize) {
        let i: usize = 0;
        while (i < n) : (i += 1) _.out.writeByte(' ');
    }

    fn emitImage(*_) {
        const alt = _.image_alt.items;
        const src = _.image_src;
        const saved_depth = _.image_depth;
        _.image_depth = 0;
        defer {
            _.image_depth = saved_depth;
        }

        const has_src = src != null && src.?.len > 0;
        const kitty_allowed = !_.in_cell && _.heading_level == 0;
        if (kitty_allowed && _.theme.colors && _.theme.kitty_graphics && has_src) {
            if (payload := extractPngDataUrlBase64(src.?)) {
                _.emitKittyImageDirect(payload);
                return;
            }
            if (map := _.theme.remote_image_paths) {
                if (std.mem.startsWith(u8, src.?, "http://") || std.mem.startsWith(u8, src.?, "https://")) {
                    if (local_path := map.get(src.?)) {
                        _.emitKittyImageFile(local_path);
                        return;
                    }
                }
            }
        }

        const link_ok = _.theme.colors && _.theme.hyperlinks && has_src &&
            _.link_depth == 0 && !std.mem.startsWith(u8, src.?, "data:");
        if (link_ok) {
            _.writeRawNoColor("\x1b]8;;");
            _.writeRawNoColor(src.?);
            _.writeRawNoColor("\x1b\\");
        }
        const img_marker = if (_.theme.colors) "📷 " else "[img] ";
        _.writeStyled(ansiColor(.magenta), img_marker);
        if (alt.len > 0) {
            _.writeContent(alt);
        } else if (title := _.image_title) {
            if (title.len > 0) {
                _.writeContent(title);
            } else {
                _.writeContent("(image)");
            }
        } else {
            _.writeContent("(image)");
        }
        _.writeStyled("\x1b[0m", "");
        _.reapplyStyles();
        if (link_ok) {
            _.writeRawNoColor("\x1b]8;;\x1b\\");
        }
    }

    fn emitKittyImageFile(*_, path: []const u8) {
        const encoder = std.base64.standard.Encoder;
        const encoded_len = encoder.calcSize(path.len);
        const encoded = _.allocator.alloc(u8, encoded_len) catch {
            _.out.oom = true;
            return;
        };
        defer _.allocator.free(encoded);
        * = encoder.encode(encoded, path);
        _.writeRawNoColor("\x1b_Ga=T,t=f,f=100,q=2;");
        _.writeRawNoColor(encoded);
        _.writeRawNoColor("\x1b\\");
        _.writeRaw("\n");
        _.col = 0;
        _.last_was_newline = true;
        _.writeIndent();
    }

    fn emitKittyImageDirect(*_, base64_payload: []const u8) {
        _.writeRawNoColor("\x1b_Ga=T,t=d,f=100,q=2;");
        _.writeRawNoColor(base64_payload);
        _.writeRawNoColor("\x1b\\");
        _.writeRaw("\n");
        _.col = 0;
        _.last_was_newline = true;
        _.writeIndent();
    }
}

const ansi_renderer_vtable = RendererVTable{
    enterBlock: AnsiRenderer.enterBlockImpl,
    leaveBlock: AnsiRenderer.leaveBlockImpl,
    enterSpan: AnsiRenderer.enterSpanImpl,
    leaveSpan: AnsiRenderer.leaveSpanImpl,
    text: AnsiRenderer.textImpl,
};

enum AnsiColorKind: u8 {
    black = 0,
    red = 1,
    green = 2,
    yellow = 3,
    blue = 4,
    magenta = 5,
    cyan = 6,
    white = 7,
    dim = 8,
}

fn ansiColor(c: AnsiColorKind): []const u8 {
    return switch (c) {
        .black => "\x1b[30m",
        .red => "\x1b[31m",
        .green => "\x1b[32m",
        .yellow => "\x1b[33m",
        .blue => "\x1b[34m",
        .magenta => "\x1b[35m",
        .cyan => "\x1b[36m",
        .white => "\x1b[37m",
        .dim => "\x1b[2m",
    };
}

fn codeSpanOpen(light: bool): []const u8 {
    return if (light) "\x1b[48;5;254m\x1b[38;5;124m" else "\x1b[48;5;236m\x1b[38;5;215m";
}

fn utf8SeqLenSafe(b0: u8): usize {
    const l = utf8SeqLen(b0);
    return if (l == 0) 1 else (l as usize);
}

fn codepointDisplayWidth(cp: u21): usize {
    if (cp == 0 || cp < 0x20 || (cp >= 0x7F && cp < 0xA0)) return 0;
    if ((cp >= 0x0300 && cp <= 0x036F) ||
        (cp >= 0x1AB0 && cp <= 0x1AFF) ||
        (cp >= 0x1DC0 && cp <= 0x1DFF) ||
        (cp >= 0x20D0 && cp <= 0x20FF) ||
        (cp >= 0xFE00 && cp <= 0xFE0F) ||
        (cp >= 0xFE20 && cp <= 0xFE2F) ||
        cp == 0x200D)
    {
        return 0;
    }
    if ((cp >= 0x1100 && cp <= 0x115F) ||
        cp == 0x2329 || cp == 0x232A ||
        (cp >= 0x2E80 && cp <= 0x303E) ||
        (cp >= 0x3040 && cp <= 0xA4CF) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) ||
        (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0xFE10 && cp <= 0xFE19) ||
        (cp >= 0xFE30 && cp <= 0xFE6F) ||
        (cp >= 0xFF00 && cp <= 0xFF60) ||
        (cp >= 0xFFE0 && cp <= 0xFFE6) ||
        (cp >= 0x1F300 && cp <= 0x1FAFF) ||
        (cp >= 0x20000 && cp <= 0x3FFFD))
    {
        return 2;
    }
    return 1;
}

fn skipAnsiEscape(s: []const u8, i: usize): usize {
    if (i + 1 >= s.len) return i + 1;
    const next = s[i + 1];
    if (next == '[') {
        let j = i + 2;
        while (j < s.len) : (j += 1) {
            if (s[j] >= 0x40 && s[j] <= 0x7e) return j + 1;
        }
        return s.len;
    }
    if (next == ']') {
        let j = i + 2;
        while (j < s.len) : (j += 1) {
            if (s[j] == 0x07) return j + 1;
            if (s[j] == 0x1b && j + 1 < s.len && s[j + 1] == '\\') return j + 2;
        }
        return s.len;
    }
    return i + 2;
}

pub fn visibleWidth(s: []const u8): usize {
    let w: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0x1b) {
            i = skipAnsiEscape(s, i);
            continue;
        }
        const decoded = decodeUtf8(s, i);
        w += codepointDisplayWidth(decoded.codepoint);
        i += decoded.len as usize;
    }
    return w;
}

pub fn visibleIndexAt(s: []const u8, max_cols: usize): usize {
    let w: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0x1b) {
            i = skipAnsiEscape(s, i);
            continue;
        }
        const decoded = decodeUtf8(s, i);
        const cw = codepointDisplayWidth(decoded.codepoint);
        if (w + cw > max_cols) return i;
        w += cw;
        i += decoded.len as usize;
    }
    return s.len;
}

fn resolveHrefOpt(detail: SpanDetail, allocator: std.mem.Allocator): ([]u8)? {
    return resolveHref(detail, allocator) catch return null;
}

fn resolveHref(detail: SpanDetail, allocator: std.mem.Allocator): ([]u8)! {
    let buf: std.ArrayListUnmanaged(u8) = .empty;
    errdefer buf.deinit(allocator);
    if (detail.autolink_email) buf.appendSlice(allocator, "mailto:")?;
    if (detail.autolink_www) buf.appendSlice(allocator, "http://")?;
    buf.appendSlice(allocator, detail.href)?;
    return buf.toOwnedSlice(allocator);
}

fn isCodeHighlightLang(lang: []const u8): bool {
    const names: [12][]const u8 = [
        "js", "javascript", "jsx", "mjs", "cjs",
        "ts", "typescript", "tsx", "mts", "cts", "cp", "zig",
    ];
    for (n of names) {
        if (std.ascii.eqlIgnoreCase(lang, n)) return true;
    }
    return false;
}

fn isHighlightKeyword(tok: []const u8): bool {
    const kws: [20][]const u8 = [
        "const", "let", "fn", "pub", "struct", "enum", "return", "if", "else", "while",
        "for", "switch", "break", "continue", "import", "export", "async", "await", "try", "defer",
    ];
    for (k of kws) {
        if (std.mem.eql(u8, tok, k)) return true;
    }
    return false;
}

fn extractLanguage(src_text: []const u8, info_beg: u32): []const u8 {
    let lang_end: u32 = info_beg;
    while ((lang_end as usize) < src_text.len) {
        const c = src_text[lang_end];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') break;
        lang_end += 1;
    }
    if (lang_end > info_beg) return src_text[info_beg..lang_end];
    return "";
}

fn extractPngDataUrlBase64(src: []const u8): ([]const u8)? {
    if (!std.mem.startsWith(u8, src, "data:")) return null;
    const comma = std.mem.indexOfScalar(u8, src, ',') ?? return null;
    const header = src[0..comma];
    const payload = src[comma + 1 ..];
    if (!std.mem.endsWith(u8, header, ";base64")) return null;
    if (std.mem.indexOf(u8, header, "image/png") == null) return null;
    return payload;
}

pub fn detectLightBackground(): bool {
    if (raw_c := std.c.getenv("COLORFGBG")) {
        const value = std.mem.span(raw_c);
        let iter = std.mem.splitScalar(u8, value, ';');
        let last: []const u8 = "";
        while (part := iter.next()) last = part;
        if (last.len > 0) {
            const bg = std.fmt.parseInt(u8, last, 10) catch return false;
            return bg == 7 || bg == 15;
        }
    }
    return false;
}
