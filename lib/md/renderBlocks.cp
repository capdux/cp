// Source: Absorbed from /data/dev/bun/src/md/render_blocks.cp | Synced: 2026-11-22T12:49:17Z
// lib/md/render_blocks.cp
// MD4C Block-Level Rendering Dispatch (Code, HTML, Tables & Leaf Blocks)

const std = @import("std");
import { isBlank } from "./helpers.cp";
import { emitText } from "./inlineCore.cp";
import { processInlineContent } from "./inlines.cp";
import {
    BLOCK_FENCED_CODE,
    BlockType,
    Md4cParserState,
    OFF,
    TABLE_MAXCOLCOUNT,
    VerbatimLine,
} from "./types.cp";

pub fn enterBlock(p: *Md4cParserState, block_type: BlockType, data: u32, flags: u32): ! {
    if (p.image_nesting_level > 0) return;
    p.renderer.enterBlock(block_type, data, flags)?;
}

pub fn leaveBlock(p: *Md4cParserState, block_type: BlockType, data: u32): ! {
    if (p.image_nesting_level > 0) return;
    p.renderer.leaveBlock(block_type, data)?;
}

pub fn processCodeBlock(p: *Md4cParserState, block_lines: []const VerbatimLine, data: u32, flags: u32): ! {
    * = data;
    let count = block_lines.len;

    if ((flags & BLOCK_FENCED_CODE) == 0) {
        while (count > 0 && block_lines[count - 1].beg >= block_lines[count - 1].end) {
            count -= 1;
        }
    }

    for (vline of block_lines[0..count]) {
        for (idx of 0..vline.indent) {
            * = idx;
            emitText(p, .normal, " ")?;
        }
        const content = p.text[vline.beg..vline.end];
        emitText(p, .normal, content)?;
        emitText(p, .normal, "\n")?;
    }
}

pub fn processHtmlBlock(p: *Md4cParserState, block_lines: []const VerbatimLine): ! {
    let i: usize = 0;
    for (vline of block_lines) {
        if (i > 0) emitText(p, .html, "\n")?;
        for (idx of 0..vline.indent) {
            * = idx;
            emitText(p, .html, " ")?;
        }
        emitText(p, .html, p.text[vline.beg..vline.end])?;
        i += 1;
    }
    emitText(p, .html, "\n")?;
}

pub fn processTableRow(p: *Md4cParserState, vline: VerbatimLine, is_header: bool, col_count: u32): ! {
    const row_text = p.text[vline.beg..vline.end];
    let start: usize = 0;
    let cell_index: u32 = 0;

    if (start < row_text.len && row_text[start] == '|') start += 1;

    while (start < row_text.len && cell_index < col_count) {
        let end = start;
        while (end < row_text.len && row_text[end] != '|') {
            if (row_text[end] == '\\' && end + 1 < row_text.len) {
                end += 2;
            } else {
                end += 1;
            }
        }

        if (end == row_text.len && start == end) break;

        let cell_beg = start;
        let cell_end = end;
        while (cell_beg < cell_end && isBlank(row_text[cell_beg])) cell_beg += 1;
        while (cell_end > cell_beg && isBlank(row_text[cell_end - 1])) cell_end -= 1;

        const cell_type: BlockType = if (is_header) BlockType.th else BlockType.td;
        const align_data: u32 = if (cell_index < TABLE_MAXCOLCOUNT) (@intFromEnum(p.table_alignments[cell_index]) as u32) else 0;
        enterBlock(p, cell_type, align_data, 0)?;
        if (cell_beg < cell_end) {
            const cell_content = row_text[cell_beg..cell_end];
            if (std.mem.indexOf(u8, cell_content, "\\|") != null) {
                let buf: std.ArrayListUnmanaged(u8) = .empty;
                defer buf.deinit(p.allocator);
                let unescaped = cell_content;
                let cap_ok = true;
                buf.ensureTotalCapacity(p.allocator, cell_content.len) catch {
                    cap_ok = false;
                };
                if (cap_ok) {
                    let ci: usize = 0;
                    while (ci < cell_content.len) {
                        if (cell_content[ci] == '\\' && ci + 1 < cell_content.len && cell_content[ci + 1] == '|') {
                            buf.appendAssumeCapacity('|');
                            ci += 2;
                        } else {
                            buf.appendAssumeCapacity(cell_content[ci]);
                            ci += 1;
                        }
                    }
                    unescaped = buf.items;
                }
                processInlineContent(p, unescaped, vline.beg + @as(OFF, @intCast(cell_beg)))?;
            } else {
                processInlineContent(p, cell_content, vline.beg + @as(OFF, @intCast(cell_beg)))?;
            }
        }
        leaveBlock(p, cell_type, 0)?;
        cell_index += 1;

        if (end < row_text.len) {
            start = end + 1;
        } else {
            break;
        }
    }

    const cell_type: BlockType = if (is_header) BlockType.th else BlockType.td;
    while (cell_index < col_count) {
        const align_data: u32 = if (cell_index < TABLE_MAXCOLCOUNT) (@intFromEnum(p.table_alignments[cell_index]) as u32) else 0;
        enterBlock(p, cell_type, align_data, 0)?;
        leaveBlock(p, cell_type, 0)?;
        cell_index += 1;
    }
}

pub fn processTableBlock(p: *Md4cParserState, block_lines: []const VerbatimLine, col_count: u32): ! {
    if (block_lines.len < 2) return;

    enterBlock(p, .thead, 0, 0)?;
    enterBlock(p, .tr, 0, 0)?;
    processTableRow(p, block_lines[0], true, col_count)?;
    leaveBlock(p, .tr, 0)?;
    leaveBlock(p, .thead, 0)?;

    if (block_lines.len > 2) {
        enterBlock(p, .tbody, 0, 0)?;
        for (vline of block_lines[2..]) {
            enterBlock(p, .tr, 0, 0)?;
            processTableRow(p, vline, false, col_count)?;
            leaveBlock(p, .tr, 0)?;
        }
        leaveBlock(p, .tbody, 0)?;
    }
}
