// Source: Absorbed from /data/dev/bun/src/md/containers.cp | Synced: 2026-11-22T12:23:32Z
// lib/md/containers.cp
// MD4C Container Stack Management, Loose List Patching & Block Dispatch Loop

const std = @import("std");
import { isListBullet } from "./autolinks.cp";
import { processLeafBlock } from "./inlines.cp";
import { enterBlock, leaveBlock, processCodeBlock, processHtmlBlock, processTableBlock } from "./renderBlocks.cp";
import {
    BLOCK_CONTAINER_CLOSER,
    BLOCK_CONTAINER_OPENER,
    BLOCK_LOOSE_LIST,
    BLOCK_REF_DEF_ONLY,
    BlockHeader,
    BlockType,
    Container,
    Md4cParserState,
    VerbatimLine,
} from "./types.cp";

pub fn pushContainer(p: *Md4cParserState, cont: *const Container): ! {
    if ((p.n_containers as usize) >= p.containers.items.len) {
        p.containers.append(p.allocator, unsafe { cont.* })?;
    } else {
        p.containers.items[p.n_containers] = unsafe { cont.* };
    }

    const block_off: u32 = @intCast(p.block_bytes.items.len);
    p.containers.items[p.n_containers].block_byte_off = block_off;
    p.n_containers += 1;
}

pub fn pushContainerBytes(p: *Md4cParserState, block_type: BlockType, data: u32, flags: u32): ! {
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
            flags: flags,
            data: data,
            n_lines: 0,
        };
    }
}

pub fn enterChildContainers(p: *Md4cParserState, count: u32): ! {
    let i: u32 = p.n_containers - count;
    while (i < p.n_containers) : (i += 1) {
        const c = p.containers.items[i];
        const task_data: u32 = if (c.is_task) (p.text[c.task_mark_off] as u32) else 0;
        if (c.ch == '>') {
            pushContainerBytes(p, .quote, 0, BLOCK_CONTAINER_OPENER)?;
        } else if (c.ch == '-' || c.ch == '+' || c.ch == '*') {
            const align_mask_: usize = @alignOf(BlockHeader) - 1;
            p.containers.items[i].block_byte_off = @intCast((p.block_bytes.items.len + align_mask_) & ~align_mask_);
            pushContainerBytes(p, .ul, 0, BLOCK_CONTAINER_OPENER)?;
            pushContainerBytes(p, .li, task_data, BLOCK_CONTAINER_OPENER)?;
        } else if (c.ch == '.' || c.ch == ')') {
            const align_mask_: usize = @alignOf(BlockHeader) - 1;
            p.containers.items[i].block_byte_off = @intCast((p.block_bytes.items.len + align_mask_) & ~align_mask_);
            pushContainerBytes(p, .ol, c.start, BLOCK_CONTAINER_OPENER)?;
            pushContainerBytes(p, .li, task_data, BLOCK_CONTAINER_OPENER)?;
        }
    }
}

pub fn leaveChildContainers(p: *Md4cParserState, keep: u32): ! {
    while (p.n_containers > keep) {
        p.n_containers -= 1;
        const c = p.containers.items[p.n_containers];
        const loose_flag: u32 = if (c.is_loose) BLOCK_LOOSE_LIST else 0;
        const task_data: u32 = if (c.is_task) (p.text[c.task_mark_off] as u32) else 0;

        if (c.ch == '>') {
            pushContainerBytes(p, .quote, 0, BLOCK_CONTAINER_CLOSER)?;
        } else if (c.ch == '-' || c.ch == '+' || c.ch == '*') {
            if (c.is_loose && (c.block_byte_off as usize) < p.block_bytes.items.len) {
                p.addBlockFlagsAt(c.block_byte_off as usize, BLOCK_LOOSE_LIST);
            }
            pushContainerBytes(p, .li, task_data, BLOCK_CONTAINER_CLOSER)?;
            pushContainerBytes(p, .ul, 0, BLOCK_CONTAINER_CLOSER | loose_flag)?;
        } else if (c.ch == '.' || c.ch == ')') {
            if (c.is_loose && (c.block_byte_off as usize) < p.block_bytes.items.len) {
                p.addBlockFlagsAt(c.block_byte_off as usize, BLOCK_LOOSE_LIST);
            }
            pushContainerBytes(p, .li, task_data, BLOCK_CONTAINER_CLOSER)?;
            pushContainerBytes(p, .ol, c.start, BLOCK_CONTAINER_CLOSER | loose_flag)?;
        }
    }
}

pub fn isContainerCompatible(existing: *const Container, new_c: *const Container): bool {
    if (existing.ch == '>' && new_c.ch == '>') return true;
    if (existing.ch == new_c.ch) return true;
    if (isListBullet(existing.ch) && isListBullet(new_c.ch)) return false;
    return false;
}

pub fn processAllBlocks(p: *Md4cParserState): ! {
    let off: usize = 0;
    const bytes = p.block_bytes.items;
    p.n_containers = 0;

    while (off < bytes.len) {
        const align_mask: usize = @alignOf(BlockHeader) - 1;
        off = (off + align_mask) & ~align_mask;
        if (off + @sizeOf(BlockHeader) > bytes.len) break;

        const hdr: *const BlockHeader = unsafe { @ptrCast(@alignCast(bytes.ptr + off)) };
        off += @sizeOf(BlockHeader);

        const block_type = hdr.block_type;
        const n_lines = hdr.n_lines;
        const data = hdr.data;
        const flags = hdr.flags;

        const lines_size = (n_lines as usize) * @sizeOf(VerbatimLine);
        if (off + lines_size > bytes.len) break;
        const line_data: [*]const VerbatimLine = unsafe { @ptrCast(@alignCast(bytes.ptr + off)) };
        const block_lines = unsafe { line_data[0..(n_lines as usize)] };
        off += lines_size;

        if ((flags & BLOCK_CONTAINER_OPENER) != 0) {
            enterBlock(p, block_type, data, flags)?;
            if (block_type == .ul || block_type == .ol) {
                if ((p.n_containers as usize) < p.containers.items.len) {
                    p.containers.items[p.n_containers].is_loose = ((flags & BLOCK_LOOSE_LIST) != 0);
                    p.n_containers += 1;
                }
            } else if (block_type == .quote) {
                if ((p.n_containers as usize) < p.containers.items.len) {
                    p.containers.items[p.n_containers].is_loose = true;
                    p.n_containers += 1;
                }
            }
            continue;
        }
        if ((flags & BLOCK_CONTAINER_CLOSER) != 0) {
            if (block_type == .ul || block_type == .ol || block_type == .quote) {
                if (p.n_containers > 0) p.n_containers -= 1;
            }
            leaveBlock(p, block_type, data)?;
            continue;
        }

        if ((flags & BLOCK_REF_DEF_ONLY) != 0) continue;

        const is_in_tight_list = p.n_containers > 0 && !p.containers.items[p.n_containers - 1].is_loose;

        if (!is_in_tight_list || block_type != .p) {
            enterBlock(p, block_type, data, flags)?;
        }
        switch (block_type) {
            .hr => {},
            .code => processCodeBlock(p, block_lines, data, flags)?,
            .html => processHtmlBlock(p, block_lines)?,
            .table => processTableBlock(p, block_lines, data)?,
            .p => processLeafBlock(p, block_lines, true)?,
            .h => processLeafBlock(p, block_lines, true)?,
            else => processLeafBlock(p, block_lines, false)?,
        }
        if (!is_in_tight_list || block_type != .p) {
            leaveBlock(p, block_type, data)?;
        }
    }
}
