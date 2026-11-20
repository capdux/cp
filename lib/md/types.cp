// Source: /data/dev/bun/src/md/types.cp
// lib/md/types.cp
// Core MD4C-compatible AST, Block, Span, Renderer VTable, Flags, Marks & Parser State

const std = @import("std");
import { BitSet256 } from "../col/bit_set.cp";

pub type OFF = u32;
pub type SZ = u32;

pub err MdError {
    OutOfMemory,
    StackOverflow,
    RenderError,
    InvalidInput,
}

pub enum BlockType: u8 {
    doc = 0,
    quote = 1,
    ul = 2,
    ol = 3,
    li = 4,
    hr = 5,
    h = 6,
    code = 7,
    html = 8,
    p = 9,
    table = 10,
    thead = 11,
    tbody = 12,
    tr = 13,
    th = 14,
    td = 15,
}

pub enum SpanType: u8 {
    em = 0,
    strong = 1,
    a = 2,
    img = 3,
    code = 4,
    del = 5,
    latexmath = 6,
    latexmath_display = 7,
    wikilink = 8,
    u = 9,
}

pub enum TextType: u8 {
    normal = 0,
    null_char = 1,
    br = 2,
    softbr = 3,
    entity = 4,
    code = 5,
    html = 6,
    latexmath = 7,
}

pub enum Align: u8 {
    default = 0,
    left = 1,
    center = 2,
    right = 3,
}

pub struct SubstrOffset {
    beg: OFF = 0,
    end: OFF = 0,
}

pub enum SubstrType: u8 {
    normal = 0,
    entity = 1,
}

pub struct Attribute {
    substr_offsets: []const SubstrOffset = [],
    substr_types: []const SubstrType = [],

    pub fn text(src: []const u8): []const u8 {
        if (_.substr_offsets.len == 0) return "";
        const first = _.substr_offsets[0].beg;
        const last = _.substr_offsets[_.substr_offsets.len - 1].end;
        return src[first..last];
    }
}

pub struct UlDetail {
    is_tight: bool = true,
    mark: u8 = '-',
}

pub struct OlDetail {
    start: u32 = 1,
    is_tight: bool = true,
    mark_delimiter: u8 = '.',
}

pub struct LiDetail {
    is_task: bool = false,
    task_mark: u8 = 0,
    task_mark_offset: OFF = 0,
}

pub struct HDetail {
    level: u8 = 1,
}

pub struct CodeDetail {
    info: Attribute = Attribute{},
    lang: Attribute = Attribute{},
    fence_char: u8 = '`',
}

pub struct TableDetail {
    col_count: u32 = 0,
    head_row_count: u32 = 0,
    body_row_count: u32 = 0,
}

pub struct TdDetail {
    alignment: Align = .default,
}

pub struct ADetail {
    href: Attribute = Attribute{},
    title: Attribute = Attribute{},
}

pub struct ImgDetail {
    src: Attribute = Attribute{},
    title: Attribute = Attribute{},
}

pub struct WikilinkDetail {
    target: Attribute = Attribute{},
}

pub struct SpanDetail {
    href: []const u8 = "",
    title: []const u8 = "",
    autolink: bool = false,
    autolink_email: bool = false,
    permissive_autolink: bool = false,
    autolink_www: bool = false,
}

pub struct RendererVTable {
    enterBlock: fn (*anyopaque, BlockType, u32, u32) !,
    leaveBlock: fn (*anyopaque, BlockType, u32) !,
    enterSpan: fn (*anyopaque, SpanType, SpanDetail) !,
    leaveSpan: fn (*anyopaque, SpanType) !,
    text: fn (*anyopaque, TextType, []const u8) !,
}

pub struct Renderer {
    ptr: *anyopaque,
    vtable: *const RendererVTable,

    pub fn enterBlock(block_type: BlockType, data: u32, flags: u32): ! {
        return _.vtable.enterBlock(_.ptr, block_type, data, flags);
    }

    pub fn leaveBlock(block_type: BlockType, data: u32): ! {
        return _.vtable.leaveBlock(_.ptr, block_type, data);
    }

    pub fn enterSpan(span_type: SpanType, detail: SpanDetail): ! {
        return _.vtable.enterSpan(_.ptr, span_type, detail);
    }

    pub fn leaveSpan(span_type: SpanType): ! {
        return _.vtable.leaveSpan(_.ptr, span_type);
    }

    pub fn text(text_type: TextType, content: []const u8): ! {
        return _.vtable.text(_.ptr, text_type, content);
    }
}

pub enum LineType: u8 {
    blank = 0,
    hr = 1,
    atxheader = 2,
    setextunderline = 3,
    setextheader = 4,
    indentedcode = 5,
    fencedcode = 6,
    html = 7,
    text = 8,
    table = 9,
    tableunderline = 10,
}

pub struct Line {
    kind: LineType = .blank,
    beg: OFF = 0,
    end: OFF = 0,
    indent: u32 = 0,
    data: u32 = 0,
    enforce_new_block: bool = false,
}

pub @extern struct VerbatimLine {
    beg: OFF = 0,
    end: OFF = 0,
    indent: u32 = 0,
}

pub struct Container {
    ch: u8 = 0,
    is_loose: bool = false,
    is_task: bool = false,
    task_mark_off: OFF = 0,
    start: u32 = 0,
    mark_indent: u32 = 0,
    contents_indent: u32 = 0,
    block_byte_off: u32 = 0,
}

pub const BLOCK_CONTAINER_CLOSER: u32 = 0x01;
pub const BLOCK_CONTAINER_OPENER: u32 = 0x02;
pub const BLOCK_LOOSE_LIST: u32 = 0x04;
pub const BLOCK_SETEXT_HEADER: u32 = 0x08;
pub const BLOCK_FENCED_CODE: u32 = 0x10;
pub const BLOCK_REF_DEF_ONLY: u32 = 0x20;

pub @extern struct BlockHeader {
    block_type: BlockType = .doc,
    _pad: [3]u8 = [0, 0, 0],
    flags: u32 = 0,
    data: u32 = 0,
    n_lines: u32 = 0,
}

pub struct MarkFlags {
    pub const POTENTIAL_OPENER: u16 = 0x01;
    pub const POTENTIAL_CLOSER: u16 = 0x02;
    pub const OPENER: u16 = 0x04;
    pub const CLOSER: u16 = 0x08;
    pub const RESOLVED: u16 = 0x10;
    pub const EMPH_INTRAWORD: u16 = 0x20;
    pub const EMPH_MOD3_0: u16 = 0x40;
    pub const EMPH_MOD3_1: u16 = 0x80;
    pub const EMPH_MOD3_2: u16 = 0x100;
    pub const EMPH_OC: u16 = POTENTIAL_OPENER | POTENTIAL_CLOSER;
}

pub struct Mark {
    beg: OFF = 0,
    end: OFF = 0,
    prev: i32 = -1,
    next: i32 = -1,
    ch: u8 = 0,
    flags: u16 = 0,
}

pub struct Flags {
    collapse_whitespace: bool = false,
    permissive_atx_headers: bool = false,
    permissive_url_autolinks: bool = false,
    permissive_www_autolinks: bool = false,
    permissive_email_autolinks: bool = false,
    no_indented_code_blocks: bool = false,
    no_html_blocks: bool = false,
    no_html_spans: bool = false,
    tables: bool = true,
    strikethrough: bool = true,
    tasklists: bool = true,
    latex_math: bool = false,
    wiki_links: bool = false,
    underline: bool = false,
    hard_soft_breaks: bool = false,

    pub static fn commonmark(): Flags {
        return Flags{
            tables: false,
            strikethrough: false,
            tasklists: false,
        };
    }

    pub static fn github(): Flags {
        return Flags{
            tables: true,
            strikethrough: true,
            tasklists: true,
            permissive_url_autolinks: true,
            permissive_www_autolinks: true,
            permissive_email_autolinks: true,
        };
    }

    pub fn permissiveAutolinks(): bool {
        return _.permissive_url_autolinks || _.permissive_www_autolinks || _.permissive_email_autolinks;
    }
}

pub const NUM_OPENER_STACKS: usize = 16;

pub const ASTERISK_OPENERS_OO_0: usize = 0;
pub const ASTERISK_OPENERS_OO_1: usize = 1;
pub const ASTERISK_OPENERS_OO_2: usize = 2;
pub const ASTERISK_OPENERS_OC_0: usize = 3;
pub const ASTERISK_OPENERS_OC_1: usize = 4;
pub const ASTERISK_OPENERS_OC_2: usize = 5;
pub const UNDERSCORE_OPENERS_OO_0: usize = 6;
pub const UNDERSCORE_OPENERS_OO_1: usize = 7;
pub const UNDERSCORE_OPENERS_OO_2: usize = 8;
pub const UNDERSCORE_OPENERS_OC_0: usize = 9;
pub const UNDERSCORE_OPENERS_OC_1: usize = 10;
pub const UNDERSCORE_OPENERS_OC_2: usize = 11;
pub const TILDE_OPENERS_1: usize = 12;
pub const TILDE_OPENERS_2: usize = 13;
pub const BRACKET_OPENERS: usize = 14;
pub const DOLLAR_OPENERS: usize = 15;

pub struct OpenerStack {
    top: i32 = -1,
}

pub const CODESPAN_MARK_MAXLEN: u32 = 255;
pub const TABLE_MAXCOLCOUNT: u32 = 128;
pub const MAX_EMPH_MATCHES: usize = 6;

pub struct EmphDelim {
    pos: usize = 0,
    count: usize = 0,
    emph_char: u8 = '*',
    can_open: bool = false,
    can_close: bool = false,
    remaining: usize = 0,
    open_count: usize = 0,
    close_count: usize = 0,
    open_sizes: [MAX_EMPH_MATCHES]u2 = [0, 0, 0, 0, 0, 0],
    open_num: u4 = 0,
    close_sizes: [MAX_EMPH_MATCHES]u2 = [0, 0, 0, 0, 0, 0],
    close_num: u4 = 0,
    active: bool = true,
}

pub struct MdRefDef {
    label: []const u8 = "",
    dest: []const u8 = "",
    title: []const u8 = "",
    owns_label: bool = false,
    owns_dest: bool = false,
    owns_title: bool = false,
}

pub struct RenderOptions {
    tag_filter: bool = false,
    heading_ids: bool = false,
    autolink_headings: bool = false,
    sanitize_html: bool = false,
}

pub fn alignmentFromData(data: u32): Align {
    return @enumFromInt(@as(u8, @intCast(data & 0x3)));
}

pub fn alignmentName(alignment: Align): ([]const u8)? {
    return switch (alignment) {
        .left => "left",
        .center => "center",
        .right => "right",
        .default => null,
    };
}

pub fn taskMarkFromData(data: u32): u8 {
    return @intCast(data & 0xFF);
}

pub fn isTaskChecked(task_mark: u8): bool {
    return task_mark != 0 && task_mark != ' ';
}

pub struct Md4cParserState {
    allocator: std.mem.Allocator = std.heap.page_allocator,
    text: []const u8 = "",
    size: OFF = 0,
    flags: Flags = Flags{},
    renderer: Renderer = undefined,
    image_nesting_level: u32 = 0,
    link_nesting_level: u32 = 0,
    code_indent_offset: u32 = 4,
    doc_ends_with_newline: bool = false,
    mark_char_map: BitSet256 = BitSet256.initEmpty(),
    marks: std.ArrayListUnmanaged(Mark) = .empty,
    containers: std.ArrayListUnmanaged(Container) = .empty,
    block_bytes: std.ArrayListAlignedUnmanaged(u8, std.mem.Alignment.fromByteUnits(4)) = .empty,
    buffer: std.ArrayListUnmanaged(u8) = .empty,
    emph_delims: std.ArrayListUnmanaged(EmphDelim) = .empty,
    n_containers: u32 = 0,
    current_block: usize? = null,
    current_block_lines: std.ArrayListUnmanaged(VerbatimLine) = .empty,
    opener_stacks: [NUM_OPENER_STACKS]OpenerStack = undefined,
    unresolved_link_head: i32 = -1,
    unresolved_link_tail: i32 = -1,
    table_cell_boundaries_head: i32 = -1,
    table_cell_boundaries_tail: i32 = -1,
    html_block_type: u8 = 0,
    fence_indent: u32 = 0,
    table_col_count: u32 = 0,
    table_alignments: [TABLE_MAXCOLCOUNT]Align = undefined,
    ref_defs: std.ArrayListUnmanaged(MdRefDef) = .empty,
    last_line_has_list_loosening_effect: bool = false,
    last_list_item_starts_with_two_blank_lines: bool = false,
    max_ref_def_output: u64 = 0,
    recursion_depth: u32 = 0,
    max_recursion_depth: u32 = 256,
    process_inline_ptr: usize = 0,

    pub static fn init(allocator: std.mem.Allocator, text: []const u8, flags: Flags, rend: Renderer): Md4cParserState {
        const size: OFF = @intCast(text.len);
        const ends_nl = if (size > 0) (text[size - 1] == '\n' || text[size - 1] == '\r') else false;
        const max_ref = @min(@min(16 * (size as u64), 1024 * 1024), 0xFFFFFFFF as u64);
        let p = Md4cParserState{
            allocator: allocator,
            text: text,
            size: size,
            flags: flags,
            renderer: rend,
            code_indent_offset: if (flags.no_indented_code_blocks) 0xFFFFFFFF else 4,
            doc_ends_with_newline: ends_nl,
            max_ref_def_output: max_ref,
        };
        let si: usize = 0;
        while (si < NUM_OPENER_STACKS) : (si += 1) {
            p.opener_stacks[si] = OpenerStack{};
        }
        let ti: usize = 0;
        while (ti < (TABLE_MAXCOLCOUNT as usize)) : (ti += 1) {
            p.table_alignments[ti] = Align.default;
        }
        p.buildMarkCharMap();
        return p;
    }

    pub fn deinit(*_) {
        _.marks.deinit(_.allocator);
        _.containers.deinit(_.allocator);
        _.block_bytes.deinit(_.allocator);
        _.buffer.deinit(_.allocator);
        _.current_block_lines.deinit(_.allocator);
        for (rd of _.ref_defs.items) {
            if (rd.owns_label && rd.label.len > 0) _.allocator.free(rd.label);
            if (rd.owns_dest && rd.dest.len > 0) _.allocator.free(rd.dest);
            if (rd.owns_title && rd.title.len > 0) _.allocator.free(rd.title);
        }
        _.ref_defs.deinit(_.allocator);
        _.emph_delims.deinit(_.allocator);
    }

    pub fn ch(off: OFF): u8 {
        if (off >= _.size) return 0;
        return _.text[off];
    }

    pub fn buildMarkCharMap(*_) {
        _.mark_char_map.set('\\');
        _.mark_char_map.set('*');
        _.mark_char_map.set('_');
        _.mark_char_map.set('`');
        _.mark_char_map.set('&');
        _.mark_char_map.set(';');
        _.mark_char_map.set('[');
        _.mark_char_map.set('!');
        _.mark_char_map.set(']');
        _.mark_char_map.set(0);
        _.mark_char_map.set('\n');
        if (!_.flags.no_html_spans) {
            _.mark_char_map.set('<');
            _.mark_char_map.set('>');
        }
        if (_.flags.strikethrough) _.mark_char_map.set('~');
        if (_.flags.latex_math) _.mark_char_map.set('$');
        if (_.flags.permissive_email_autolinks || _.flags.permissive_url_autolinks) {
            _.mark_char_map.set(':');
        }
        if (_.flags.permissive_email_autolinks) _.mark_char_map.set('@');
        if (_.flags.permissive_www_autolinks) _.mark_char_map.set('.');
        if (_.flags.collapse_whitespace) {
            _.mark_char_map.set(' ');
            _.mark_char_map.set('\t');
            _.mark_char_map.set('\r');
        }
    }

    pub fn getBlockHeaderAt(*_, off: usize): *BlockHeader {
        return unsafe { @ptrCast(@alignCast(_.block_bytes.items.ptr + off)) };
    }

    pub fn getBlockAt(*_, off: usize): *BlockHeader {
        return _.getBlockHeaderAt(off);
    }

    pub fn addBlockFlagsAt(*_, off: usize, flag: u32) {
        let hdr = _.getBlockHeaderAt(off);
        hdr.flags |= flag;
    }

    pub fn getBlockHeaderValAt(off: usize): BlockHeader {
        const hdr: *const BlockHeader = unsafe { @ptrCast(@alignCast(_.block_bytes.items.ptr + off)) };
        return unsafe { hdr.* };
    }

    pub fn setBlockHeaderValAt(*_, off: usize, val: BlockHeader) {
        let hdr = _.getBlockHeaderAt(off);
        unsafe { hdr.* = val; }
    }
}
