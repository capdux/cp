// Source: Absorbed from /data/dev/bun/src/md/blocks.cp | Synced: 2026-11-22T12:25:01Z
// lib/md/blocks.cp
// CommonMark & GFM Block-Level Parser (Two-Pass Line Analysis, Container Nesting, Setext/Tables)

const std = @import("std");
import { isListItemMark } from "./autolinks.cp";
import {
    enterChildContainers,
    isContainerCompatible,
    leaveChildContainers,
    processAllBlocks,
    pushContainer,
    pushContainerBytes,
} from "./containers.cp";
import { isBlank, isNewline, isWhitespace, lineIndentation } from "./helpers.cp";
import {
    countTableRowColumns,
    isAtxHeaderLine,
    isClosingCodeFence,
    isContainerMark,
    isHrLine,
    isHtmlBlockEndCondition,
    isHtmlBlockStartCondition,
    isOpeningCodeFence,
    isSetextUnderline,
    isTableUnderline,
} from "./lineAnalysis.cp";
import { buildRefDefHashtable, normalizeLabel, parseRefDef } from "./refDefs.cp";
import { enterBlock, leaveBlock } from "./renderBlocks.cp";
import {
    OFF,
    BLOCK_CONTAINER_CLOSER,
    BLOCK_CONTAINER_OPENER,
    BLOCK_FENCED_CODE,
    BLOCK_REF_DEF_ONLY,
    BLOCK_SETEXT_HEADER,
    BlockHeader,
    BlockType,
    Container,
    Line,
    LineType,
    Md4cParserState,
    MdRefDef,
    VerbatimLine,
} from "./types.cp";

pub fn processDoc(p: *Md4cParserState): ! {
    let pivot_line = Line{ kind: .blank };
    let line_buf: [2]Line = [Line{}, Line{}];
    let line_idx: u1 = 0;
    let off: u32 = 0;

    enterBlock(p, .doc, 0, 0)?;

    while (off < p.size) {
        const line = &line_buf[line_idx];
        analyzeLine(p, off, &off, &pivot_line, line)?;
        processLine(p, &pivot_line, line, &line_buf, &line_idx)?;
    }

    endCurrentBlock(p)?;
    buildRefDefHashtable(p)?;
    leaveChildContainers(p, 0)?;
    processAllBlocks(p)?;
    leaveBlock(p, .doc, 0)?;
}

pub fn analyzeLine(
    p: *Md4cParserState,
    off_start: u32,
    p_end: *u32,
    pivot_line: *const Line,
    line: *Line,
): ! {
    let off = off_start;
    let total_indent: u32 = 0;
    let n_parents: u32 = 0;
    let n_brothers: u32 = 0;
    let n_children: u32 = 0;
    let container = Container{};
    const prev_line_has_list_loosening_effect = p.last_line_has_list_loosening_effect;

    unsafe {
        line.* = Line{};
    }
    line.enforce_new_block = false;

    const indent_result = lineIndentation(p.text, total_indent, off);
    line.indent = indent_result.indent;
    total_indent += line.indent;
    off = indent_result.off;
    line.beg = off;

    let remaining_indent = total_indent;
    while (n_parents < p.n_containers) {
        const c = &p.containers.items[n_parents];
        if (c.ch == '>') {
            if (off < p.size && p.text[off] == '>' && line.indent < p.code_indent_offset) {
                off += 1;
                total_indent += 1;
                const r = lineIndentation(p.text, total_indent, off);
                line.indent = r.indent;
                total_indent += line.indent;
                off = r.off;
                if (line.indent > 0) line.indent -= 1;
                remaining_indent = line.indent;
                line.beg = off;
                n_parents += 1;
                continue;
            } else {
                break;
            }
        } else {
            if (remaining_indent >= c.contents_indent) {
                remaining_indent -= c.contents_indent;
                line.indent = remaining_indent;
                n_parents += 1;
                continue;
            } else {
                break;
            }
        }
    }

    p.last_line_has_list_loosening_effect = false;

    if (off >= p.size || isNewline(p.text[off])) {
        if (n_brothers + n_children == 0) {
            while (n_parents < p.n_containers && p.containers.items[n_parents].ch != '>') {
                n_parents += 1;
            }
        }
    }

    let effective_pivot_type = pivot_line.kind;

    while (true) {
        if (effective_pivot_type == .fencedcode) {
            line.beg = off;
            if (line.indent < p.code_indent_offset) {
                if (isClosingCodeFence(p, off, pivot_line.data)) {
                    line.kind = .blank;
                    p.last_line_has_list_loosening_effect = false;
                    break;
                }
            }
            if (n_parents == p.n_containers) {
                if (line.indent > p.fence_indent) {
                    line.indent -= p.fence_indent;
                } else {
                    line.indent = 0;
                }
                line.kind = .fencedcode;
                break;
            }
        }

        if (effective_pivot_type == .html && p.html_block_type > 0) {
            if (n_parents < p.n_containers) {
                p.html_block_type = 0;
            } else {
                if (isHtmlBlockEndCondition(p, off, p.html_block_type)) {
                    const ended_type = p.html_block_type;
                    p.html_block_type = 0;
                    if (ended_type == 6 || ended_type == 7) {
                        line.kind = .blank;
                        line.indent = 0;
                        break;
                    }
                }
                line.kind = .html;
                n_parents = p.n_containers;
                break;
            }
        }

        if (off >= p.size || isNewline(p.text[off])) {
            if (effective_pivot_type == .indentedcode && n_parents == p.n_containers) {
                line.kind = .indentedcode;
                if (line.indent > p.code_indent_offset) {
                    line.indent -= p.code_indent_offset;
                } else {
                    line.indent = 0;
                }
                p.last_line_has_list_loosening_effect = false;
            } else {
                line.kind = .blank;
                p.last_line_has_list_loosening_effect = (n_parents > 0 &&
                    n_brothers + n_children == 0 &&
                    p.containers.items[n_parents - 1].ch != '>');

                if (p.html_block_type >= 6) {
                    p.html_block_type = 0;
                }

                if (n_parents > 0 && p.containers.items[n_parents - 1].ch != '>' &&
                    n_brothers + n_children == 0 && p.current_block == null &&
                    p.block_bytes.items.len > @sizeOf(BlockHeader))
                {
                    const top_hdr: *const BlockHeader = unsafe {
                        @ptrCast(@alignCast(p.block_bytes.items.ptr + (p.block_bytes.items.len - @sizeOf(BlockHeader))))
                    };
                    if (top_hdr.block_type == .li) {
                        p.last_list_item_starts_with_two_blank_lines = true;
                    }
                }
            }
            break;
        } else {
            if (p.last_list_item_starts_with_two_blank_lines) {
                if (n_parents > 0 && n_parents == p.n_containers &&
                    p.containers.items[n_parents - 1].ch != '>' &&
                    n_brothers + n_children == 0 && p.current_block == null &&
                    p.block_bytes.items.len > @sizeOf(BlockHeader))
                {
                    const top_hdr: *const BlockHeader = unsafe {
                        @ptrCast(@alignCast(p.block_bytes.items.ptr + (p.block_bytes.items.len - @sizeOf(BlockHeader))))
                    };
                    if (top_hdr.block_type == .li) {
                        n_parents -= 1;
                        line.indent = total_indent;
                        if (n_parents > 0) {
                            line.indent -= @min(line.indent, p.containers.items[n_parents - 1].contents_indent);
                        }
                    }
                }
                p.last_list_item_starts_with_two_blank_lines = false;
            }
            p.last_line_has_list_loosening_effect = false;
        }

        if (effective_pivot_type == .indentedcode) {
            if (line.indent >= p.code_indent_offset) {
                line.kind = .indentedcode;
                line.indent -= p.code_indent_offset;
                line.data = 0;
                break;
            }
        }

        if (line.indent < p.code_indent_offset && effective_pivot_type == .text &&
            off < p.size && (p.text[off] == '=' || p.text[off] == '-') &&
            n_parents == p.n_containers)
        {
            const setext_result = isSetextUnderline(p, off);
            if (setext_result.is_setext) {
                line.kind = .setextunderline;
                line.data = setext_result.level;
                break;
            }
        }

        if (line.indent < p.code_indent_offset && off < p.size &&
            (p.text[off] == '-' || p.text[off] == '_' || p.text[off] == '*'))
        {
            if (isHrLine(p, off)) {
                line.kind = .hr;
                break;
            }
        }

        if (n_parents < p.n_containers && n_brothers + n_children == 0) {
            const cont_result = isContainerMark(p, line.indent, off);
            if (cont_result.is_container) {
                if (isContainerCompatible(&p.containers.items[n_parents], &cont_result.container)) {
                    effective_pivot_type = .blank;
                    container = cont_result.container;
                    off = cont_result.off;

                    total_indent += container.contents_indent - container.mark_indent;
                    const r = lineIndentation(p.text, total_indent, off);
                    line.indent = r.indent;
                    total_indent += line.indent;
                    off = r.off;
                    line.beg = off;

                    if (off >= p.size || isNewline(p.text[off])) {
                        container.contents_indent += 1;
                    } else if (line.indent <= p.code_indent_offset) {
                        container.contents_indent += line.indent;
                        line.indent = 0;
                    } else {
                        container.contents_indent += 1;
                        line.indent -= 1;
                    }

                    p.containers.items[n_parents].mark_indent = container.mark_indent;
                    p.containers.items[n_parents].contents_indent = container.contents_indent;
                    p.html_block_type = 0;
                    n_brothers += 1;
                    continue;
                }
            }
        }

        if (line.indent >= p.code_indent_offset && effective_pivot_type != .text) {
            line.kind = .indentedcode;
            line.indent -= p.code_indent_offset;
            line.data = 0;
            break;
        }

        if (line.indent < p.code_indent_offset) {
            const cont_result = isContainerMark(p, line.indent, off);
            if (cont_result.is_container) {
                container = cont_result.container;
                let allow_new_container = true;
                if (effective_pivot_type == .text && n_parents == p.n_containers) {
                    if ((cont_result.off >= p.size || isNewline(p.ch(cont_result.off))) && container.ch != '>') {
                        allow_new_container = false;
                    } else if ((container.ch == '.' || container.ch == ')') && container.start != 1) {
                        allow_new_container = false;
                    }
                }
                if (allow_new_container) {
                    off = cont_result.off;
                    total_indent += container.contents_indent - container.mark_indent;
                    const r = lineIndentation(p.text, total_indent, off);
                    line.indent = r.indent;
                    total_indent += line.indent;
                    off = r.off;
                    line.beg = off;
                    line.data = container.ch as u32;

                    if (off >= p.size || isNewline(p.text[off])) {
                        container.contents_indent += 1;
                    } else if (line.indent <= p.code_indent_offset) {
                        container.contents_indent += line.indent;
                        line.indent = 0;
                    } else {
                        container.contents_indent += 1;
                        line.indent -= 1;
                    }

                    if (n_brothers + n_children == 0) {
                        effective_pivot_type = .blank;
                    }
                    if (n_children == 0) {
                        endCurrentBlock(p)?;
                        leaveChildContainers(p, n_parents + n_brothers)?;
                    }
                    n_children += 1;
                    pushContainer(p, &container)?;
                    continue;
                }
            }
        }

        if (effective_pivot_type == .table && n_parents == p.n_containers) {
            line.kind = .table;
            break;
        }

        if (line.indent < p.code_indent_offset && off < p.size && p.text[off] == '#') {
            const atx_result = isAtxHeaderLine(p, off);
            if (atx_result.is_atx) {
                line.kind = .atxheader;
                line.data = atx_result.level;
                line.beg = atx_result.content_beg;
                break;
            }
        }

        if (line.indent < p.code_indent_offset && off < p.size && (p.text[off] == '`' || p.text[off] == '~')) {
            const fence_result = isOpeningCodeFence(p, off);
            if (fence_result.is_fence) {
                line.kind = .fencedcode;
                line.data = fence_result.fence_data;
                line.enforce_new_block = true;
                break;
            }
        }

        if (off < p.size && p.text[off] == '<' && !p.flags.no_html_blocks) {
            p.html_block_type = isHtmlBlockStartCondition(p, off);
            if (p.html_block_type == 7 && effective_pivot_type == .text) {
                p.html_block_type = 0;
            }
            if (p.html_block_type > 0) {
                if (isHtmlBlockEndCondition(p, off, p.html_block_type)) {
                    p.html_block_type = 0;
                }
                line.enforce_new_block = true;
                line.kind = .html;
                break;
            }
        }

        if (p.flags.tables && effective_pivot_type == .text &&
            off < p.size && (p.text[off] == '|' || p.text[off] == '-' || p.text[off] == ':') &&
            n_parents == p.n_containers)
        {
            const tbl_result = isTableUnderline(p, off);
            if (tbl_result.is_underline && p.current_block != null && p.current_block_lines.items.len >= 1) {
                const header_line = p.current_block_lines.items[p.current_block_lines.items.len - 1];
                const header_cols = countTableRowColumns(p, header_line.beg, header_line.end);
                if (header_cols == tbl_result.col_count) {
                    line.data = tbl_result.col_count;
                    line.kind = .tableunderline;
                    break;
                }
            }
        }

        line.kind = .text;
        if (effective_pivot_type == .text && n_brothers + n_children == 0) {
            n_parents = p.n_containers;
        }

        if (p.flags.tasklists && n_brothers + n_children > 0 &&
            p.n_containers > 0 &&
            isListItemMark(p.containers.items[p.n_containers - 1].ch))
        {
            let tmp = off;
            while (tmp < p.size && tmp < off + 3 && isBlank(p.text[tmp])) tmp += 1;
            if (tmp + 2 < p.size && p.text[tmp] == '[' &&
                (p.text[tmp + 1] == 'x' || p.text[tmp + 1] == 'X' || p.text[tmp + 1] == ' ') &&
                p.text[tmp + 2] == ']' &&
                (tmp + 3 == p.size || isBlank(p.text[tmp + 3]) || isNewline(p.text[tmp + 3])))
            {
                let task_container = if (n_children > 0) &p.containers.items[p.n_containers - 1] else &container;
                task_container.is_task = true;
                task_container.task_mark_off = tmp + 1;
                off = tmp + 3;
                while (off < p.size && isWhitespace(p.text[off])) off += 1;
                line.beg = off;
            }
        }

        break;
    }

    while (off < p.size && !isNewline(p.text[off])) off += 1;
    line.end = off;

    if (line.kind == .atxheader) {
        let tmp: u32 = @as(u32, line.end);
        while (tmp > line.beg && isBlank(p.text[tmp - 1])) tmp -= 1;
        while (tmp > line.beg && p.text[tmp - 1] == '#') tmp -= 1;
        if (tmp == line.beg || isBlank(p.text[tmp - 1]) || p.flags.permissive_atx_headers) {
            line.end = @as(OFF, tmp);
        }
    }

    if (line.kind != .indentedcode && line.kind != .fencedcode && line.kind != .html && line.kind != .text) {
        let end_u32: u32 = @as(u32, line.end);
        while (end_u32 > line.beg && isBlank(p.text[end_u32 - 1])) end_u32 -= 1;
        line.end = @as(OFF, end_u32);
    }

    if (off < p.size && p.text[off] == '\r') off += 1;
    if (off < p.size && p.text[off] == '\n') off += 1;

    unsafe {
        p_end.* = off;
    }

    if (prev_line_has_list_loosening_effect && line.kind != .blank && n_parents + n_brothers > 0) {
        const ci = (n_parents + n_brothers - 1) as usize;
        if (ci < p.containers.items.len && p.containers.items[ci].ch != '>') {
            p.containers.items[ci].is_loose = true;
        }
    }

    if ((n_children == 0 && n_parents + n_brothers < p.n_containers) || n_brothers > 0 || n_children > 0) {
        endCurrentBlock(p)?;
    }

    if (n_children == 0 && n_parents + n_brothers < p.n_containers) {
        leaveChildContainers(p, n_parents + n_brothers)?;
    }

    if (n_brothers > 0) {
        const old_task: u32 = if (p.containers.items[n_parents].is_task) (p.text[p.containers.items[n_parents].task_mark_off] as u32) else 0;
        const new_task: u32 = if (container.is_task) (p.text[container.task_mark_off] as u32) else 0;
        pushContainerBytes(p, .li, old_task, BLOCK_CONTAINER_CLOSER)?;
        pushContainerBytes(p, .li, new_task, BLOCK_CONTAINER_OPENER)?;
        p.containers.items[n_parents].is_task = container.is_task;
        p.containers.items[n_parents].task_mark_off = container.task_mark_off;
    }

    if (n_children > 0) {
        enterChildContainers(p, n_children)?;
    }
}

pub fn processLine(
    p: *Md4cParserState,
    pivot_line: *Line,
    line: *Line,
    line_buf: *[2]Line,
    line_idx: *u1,
): ! {
    * = line_buf;

    if (line.kind == .blank) {
        endCurrentBlock(p)?;
        unsafe {
            pivot_line.* = Line{ kind: .blank };
        }
        return;
    }

    if (line.kind == .fencedcode && line.enforce_new_block) {
        endCurrentBlock(p)?;
        startNewBlock(p, line)?;
        p.fence_indent = line.indent;

        if (cb_off := p.current_block) {
            const fence_count: u32 = line.data >> 8;
            let info_beg: u32 = @as(u32, line.beg) + fence_count;
            while (info_beg < line.end && isBlank(p.text[info_beg])) info_beg += 1;
            let hdr = p.getBlockHeaderValAt(cb_off);
            hdr.data = info_beg;
            hdr.flags |= BLOCK_FENCED_CODE;
            p.setBlockHeaderValAt(cb_off, hdr);
        }
        unsafe {
            pivot_line.* = line.*;
        }
        return;
    }

    if (line.enforce_new_block) {
        endCurrentBlock(p)?;
    }

    if (line.kind == .hr || line.kind == .atxheader) {
        endCurrentBlock(p)?;
        startNewBlock(p, line)?;
        addLineToCurrentBlock(p, line)?;
        endCurrentBlock(p)?;
        unsafe {
            pivot_line.* = Line{ kind: .blank };
        }
        return;
    }

    if (line.kind == .setextunderline) {
        if (cb_off := p.current_block) {
            let blk = p.getBlockHeaderValAt(cb_off);
            blk.block_type = .h;
            blk.data = line.data;
            blk.flags |= BLOCK_SETEXT_HEADER;
            p.setBlockHeaderValAt(cb_off, blk);
        }
        addLineToCurrentBlock(p, line)?;
        endCurrentBlock(p)?;
        if (p.current_block == null) {
            unsafe {
                pivot_line.* = Line{ kind: .blank };
            }
        } else {
            line.kind = .text;
            unsafe {
                pivot_line.* = line.*;
            }
        }
        return;
    }

    if (line.kind == .tableunderline) {
        if (cb_off := p.current_block) {
            if (p.current_block_lines.items.len > 1) {
                const last_line = p.current_block_lines.items[p.current_block_lines.items.len - 1];
                * = p.current_block_lines.pop();
                let hdr = p.getBlockHeaderValAt(cb_off);
                hdr.n_lines -= 1;
                p.setBlockHeaderValAt(cb_off, hdr);
                endCurrentBlock(p)?;
                let header_as_line = Line{
                    kind: .table,
                    beg: last_line.beg,
                    end: last_line.end,
                    indent: last_line.indent,
                    data: line.data,
                };
                startNewBlock(p, &header_as_line)?;
                addLineToCurrentBlock(p, &header_as_line)?;
            } else {
                let blk = p.getBlockHeaderValAt(cb_off);
                blk.block_type = .table;
                blk.data = line.data;
                p.setBlockHeaderValAt(cb_off, blk);
            }
        }
        pivot_line.kind = .table;
        addLineToCurrentBlock(p, line)?;
        return;
    }

    if (line.kind != pivot_line.kind) {
        endCurrentBlock(p)?;
    }

    if (p.current_block == null) {
        startNewBlock(p, line)?;
        unsafe {
            pivot_line.* = line.*;
        }
    }

    addLineToCurrentBlock(p, line)?;
    unsafe {
        line_idx.* ^= 1;
    }
}

pub fn startNewBlock(p: *Md4cParserState, line: *const Line): ! {
    const block_type: BlockType = switch (line.kind) {
        .hr => .hr,
        .atxheader => .h,
        .fencedcode, .indentedcode => .code,
        .html => .html,
        .table, .tableunderline => .table,
        else => .p,
    };

    const align_mask: usize = @alignOf(BlockHeader) - 1;
    const cur_len = p.block_bytes.items.len;
    const aligned = (cur_len + align_mask) & ~align_mask;
    const needed = aligned + @sizeOf(BlockHeader);
    p.block_bytes.ensureTotalCapacity(p.allocator, needed)?;
    while (p.block_bytes.items.len < aligned) {
        p.block_bytes.append(p.allocator, 0)?;
    }
    p.block_bytes.items.len = needed;

    let hdr = p.getBlockHeaderAt(aligned);
    unsafe {
        hdr.* = BlockHeader{
            block_type: block_type,
            flags: 0,
            data: line.data,
            n_lines: 0,
        };
    }

    p.current_block = aligned;
    p.current_block_lines.clearRetainingCapacity();
}

pub fn addLineToCurrentBlock(p: *Md4cParserState, line: *const Line): ! {
    if (cb_off := p.current_block) {
        let hdr = p.getBlockHeaderValAt(cb_off);
        hdr.n_lines += 1;
        p.setBlockHeaderValAt(cb_off, hdr);
        p.current_block_lines.append(p.allocator, VerbatimLine{
            beg: line.beg,
            end: line.end,
            indent: line.indent,
        })?;
    }
}

pub fn endCurrentBlock(p: *Md4cParserState): ! {
    if (cb_off := p.current_block) {
        let hdr = p.getBlockHeaderValAt(cb_off);

        const is_setext = hdr.block_type == .h && (hdr.flags & BLOCK_SETEXT_HEADER) != 0;
        if (is_setext && hdr.n_lines > 0 &&
            p.current_block_lines.items.len > 0 &&
            p.current_block_lines.items[0].beg < p.size &&
            p.text[p.current_block_lines.items[0].beg] == '[')
        {
            consumeRefDefsFromCurrentBlock(p);
            hdr = p.getBlockHeaderValAt(cb_off);
        }

        if (hdr.block_type == .h && (hdr.flags & BLOCK_SETEXT_HEADER) != 0) {
            if (hdr.n_lines > 1) {
                hdr.n_lines -= 1;
                p.setBlockHeaderValAt(cb_off, hdr);
                * = p.current_block_lines.pop();
            } else if (hdr.n_lines == 1) {
                hdr.block_type = .p;
                hdr.flags &= ~BLOCK_SETEXT_HEADER;
                p.setBlockHeaderValAt(cb_off, hdr);
                return;
            } else {
                hdr.flags |= BLOCK_REF_DEF_ONLY;
                p.setBlockHeaderValAt(cb_off, hdr);
            }
        }

        const line_bytes = std.mem.sliceAsBytes(p.current_block_lines.items);
        p.block_bytes.appendSlice(p.allocator, line_bytes)?;
        p.current_block = null;
    }
}

pub fn consumeRefDefsFromCurrentBlock(p: *Md4cParserState) {
    const items = p.current_block_lines.items;
    if (items.len == 0) return;

    p.buffer.clearRetainingCapacity();
    for (vline of items) {
        if (vline.beg > vline.end || vline.end > p.size) continue;
        if (p.buffer.items.len > 0) {
            p.buffer.append(p.allocator, '\n') catch {};
        }
        p.buffer.appendSlice(p.allocator, p.text[vline.beg..vline.end]) catch {};
    }

    const merged = p.buffer.items;
    let pos: usize = 0;
    let lines_consumed: u32 = 0;

    while (pos < merged.len) {
        const result = parseRefDef(merged, pos) ?? break;
        const norm_label = normalizeLabel(p, result.label);
        if (norm_label.len == 0) break;

        let already_exists = false;
        for (existing of p.ref_defs.items) {
            if (std.mem.eql(u8, existing.label, norm_label)) {
                already_exists = true;
                break;
            }
        }
        if (!already_exists) {
            const dest_dupe = p.allocator.dupe(u8, result.dest) catch {
                p.allocator.free(norm_label);
                return;
            };
            const title_dupe = p.allocator.dupe(u8, result.title) catch {
                p.allocator.free(norm_label);
                p.allocator.free(dest_dupe);
                return;
            };
            p.ref_defs.append(p.allocator, MdRefDef{
                label: norm_label,
                dest: dest_dupe,
                title: title_dupe,
                owns_label: true,
                owns_dest: true,
                owns_title: true,
            }) catch return;
        } else {
            p.allocator.free(norm_label);
        }

        let newlines: u32 = 0;
        for (mc of merged[pos..result.end_pos]) {
            if (mc == '\n') newlines += 1;
        }
        if (result.end_pos >= merged.len && (result.end_pos == pos || merged[result.end_pos - 1] != '\n')) {
            newlines += 1;
        }
        lines_consumed += newlines;
        pos = result.end_pos;
    }

    if (lines_consumed > 0) {
        if (cb_off := p.current_block) {
            let hdr = p.getBlockHeaderValAt(cb_off);
            if (lines_consumed >= hdr.n_lines) {
                p.current_block_lines.clearRetainingCapacity();
                hdr.n_lines = 0;
            } else {
                const remaining = p.current_block_lines.items.len - (lines_consumed as usize);
                std.mem.copyForwards(
                    VerbatimLine,
                    p.current_block_lines.items[0..remaining],
                    p.current_block_lines.items[(lines_consumed as usize)..],
                );
                p.current_block_lines.shrinkRetainingCapacity(remaining);
                hdr.n_lines -= lines_consumed;
            }
            p.setBlockHeaderValAt(cb_off, hdr);
        }
    }
}
