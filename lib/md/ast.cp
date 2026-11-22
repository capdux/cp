// Source: Absorbed from /data/dev/bun/src/md/types.cp | Synced: 2026-11-22T07:47:14Z
// lib/md/ast.cp
// Markdown Options, Block AST Nodes, Link Reference Definitions & Custom Renderer Callbacks

const std = @import("std");
import { MdTableAlign } from "./table";

pub struct MarkdownOptions {
    gfm: bool = true,
    tables: bool = true,
    strikethrough: bool = true,
    tasklists: bool = true,
    autolinks: bool = true,
    sanitizeHtml: bool = false,
    headingIds: bool = false,
    tag_filter: bool = false,
    no_indented_code_blocks: bool = false,
    hard_soft_breaks: bool = false,

    pub static fn commonmark(): MarkdownOptions {
        return MarkdownOptions{
            gfm: false,
            tables: false,
            strikethrough: false,
            tasklists: false,
            autolinks: false,
            sanitizeHtml: false,
            headingIds: false,
            tag_filter: false,
        };
    }

    pub static fn github(): MarkdownOptions {
        return MarkdownOptions{
            gfm: true,
            tables: true,
            strikethrough: true,
            tasklists: true,
            autolinks: true,
            sanitizeHtml: true,
            headingIds: true,
            tag_filter: true,
        };
    }
}

pub enum MdBlockKind: u8 {
    Document = 0,
    Heading = 1,
    Paragraph = 2,
    CodeBlock = 3,
    BlockQuote = 4,
    BulletList = 5,
    OrderedList = 6,
    ListItem = 7,
    ThematicBreak = 8,
    Table = 9,
    TableHead = 10,
    TableBody = 11,
    TableRow = 12,
    TableHeaderCell = 13,
    TableCell = 14,
    HtmlBlock = 15,
}

pub struct MdLinkDef {
    label: []const u8 = "",
    url: []const u8 = "",
    title: []const u8 = "",
}

pub struct MdBlock {
    kind: MdBlockKind = .Document,
    level: u8 = 0,
    start_num: u32 = 1,
    tight: bool = true,
    is_task: bool = false,
    task_checked: bool = false,
    bullet_char: u8 = '-',
    fence_lang: []const u8 = "",
    fence_info: []const u8 = "",
    is_fenced: bool = false,
    text: []const u8 = "",
    heading_id: []const u8 = "",
    cell_align: MdTableAlign = .None,
    owns_text: bool = false,
    owns_heading_id: bool = false,
    children: []MdBlock = [],

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        for (*child of _.children) {
            child.deinit(allocator);
        }
        if (_.children.len > 0) {
            allocator.free(_.children);
            _.children = [];
        }
        if (_.owns_text && _.text.len > 0) {
            allocator.free(_.text);
            _.text = "";
            _.owns_text = false;
        }
        if (_.owns_heading_id && _.heading_id.len > 0) {
            allocator.free(_.heading_id);
            _.heading_id = "";
            _.owns_heading_id = false;
        }
    }
}

pub struct MdRenderer {
    enterBlock: (fn (MdBlock, (*anyopaque)?) void)? = null,
    leaveBlock: (fn (MdBlock, (*anyopaque)?) void)? = null,
    onText: (fn ([]const u8, bool, (*anyopaque)?) void)? = null,
    ctx: (*anyopaque)? = null,

    pub fn walkBlock(block: MdBlock) {
        if (enter := _.enterBlock) {
            enter(block, _.ctx);
        }
        if (block.text.len > 0) {
            if (txt_cb := _.onText) {
                txt_cb(block.text, block.kind == .CodeBlock, _.ctx);
            }
        }
        for (child of block.children) {
            _.walkBlock(child);
        }
        if (leave := _.leaveBlock) {
            leave(block, _.ctx);
        }
    }
}
