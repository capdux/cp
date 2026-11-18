// Source: bun/src/sourcemap/InternalSourceMap.cp
// ============================================================================
// lib/sourcemap/internal_source_map.cp — 高性能紧凑二进制 SourceMap 存储格式
// 吸收自 Bun src/sourcemap/InternalSourceMap.cp
// ============================================================================

import { SourceMapping } from "./mapping.cp";
import { VLQ } from "./vlq.cp";
const std = @import("std");

pub const SYNC_INTERVAL: usize = 64;

pub struct SyncEntry {
    generated_line: i32 = 0,
    generated_column: i32 = 0,
    byte_offset: u32 = 0,
    original_line: i32 = 0,
    original_column: i32 = 0,
    source_index: i32 = 0,

    pub fn lessOrEqual(line: i32, col: i32): bool {
        return _.generated_line < line ||
            (_.generated_line == line && _.generated_column <= col);
    }
}

pub fn zigzagEncode(value: i32): u32 {
    if (value >= 0) {
        return (value as u32) << 1;
    } else {
        return (((-value) as u32) << 1) - 1;
    }
}

pub fn zigzagDecode(value: u32): i32 {
    let half = (value >> 1) as i32;
    if ((value & 1) == 0) {
        return half;
    } else {
        return -(half + 1);
    }
}

pub fn writeVarint(buf: []u8, offset: usize, value: i32): usize {
    let v = zigzagEncode(value);
    let i = offset;
    while (true) {
        let byte: u8 = (v & 0x7F) as u8;
        v = v >> 7;
        if (v != 0) {
            byte = byte | 0x80;
        }
        if (i < buf.len) {
            buf[i] = byte;
        }
        i += 1;
        if (v == 0) {
            return i - offset;
        }
    }
}

pub fn readVarint(bytes: []const u8, pos: *usize): i32 {
    let shift: u5 = 0;
    let result: u32 = 0;
    let i = pos.*;
    while (i < bytes.len) {
        let byte = bytes[i];
        i += 1;
        result = result | (((byte & 0x7F) as u32) << shift);
        if ((byte & 0x80) == 0) {
            pos.* = i;
            return zigzagDecode(result);
        }
        shift += 7;
    }
    pos.* = i;
    return zigzagDecode(result);
}

/// 紧凑二进制 SourceMap（支持 O(log N) 快速随机查找与增量解码）
pub struct InternalSourceMap {
    sync_entries: []SyncEntry = [],
    stream: []const u8 = [],
    total_mappings: usize = 0,

    /// 二分结合流式解码反查 (generated_line, generated_column)
    pub fn find(line: i32, column: i32): SourceMapping? {
        if (_.total_mappings == 0) return null;

        // 1. 在 sync_entries 中二分查找最近的同步检查点
        let sync_idx: usize = 0;
        if (_.sync_entries.len > 0) {
            let low: usize = 0;
            let count: usize = _.sync_entries.len;
            while (count > 0) {
                let step = count / 2;
                let mid = low + step;
                if (_.sync_entries[mid].lessOrEqual(line, column)) {
                    low = mid + 1;
                    count = count - step - 1;
                } else {
                    count = step;
                }
            }
            if (low > 0) {
                sync_idx = low - 1;
            }
        }

        // 2. 从该检查点开始重放流式增量
        let gen_line: i32 = 0;
        let gen_col: i32 = 0;
        let orig_line: i32 = 0;
        let orig_col: i32 = 0;
        let src_idx: i32 = 0;
        let pos: usize = 0;

        if (_.sync_entries.len > 0) {
            const sync = _.sync_entries[sync_idx];
            gen_line = sync.generated_line;
            gen_col = sync.generated_column;
            orig_line = sync.original_line;
            orig_col = sync.original_column;
            src_idx = sync.source_index;
            pos = sync.byte_offset as usize;
        }

        let best_match: SourceMapping? = null;

        // 检查点本身是否命中
        if (gen_line < line || (gen_line == line && gen_col <= column)) {
            best_match = SourceMapping{
                generated_line: gen_line,
                generated_column: gen_col,
                source_index: src_idx,
                original_line: orig_line,
                original_column: orig_col,
                name_index: -1,
            };
        }

        while (pos < _.stream.len) {
            let d_gen_line = readVarint(_.stream, &pos);
            let d_gen_col = readVarint(_.stream, &pos);
            let d_orig_line = readVarint(_.stream, &pos);
            let d_orig_col = readVarint(_.stream, &pos);
            let d_src_idx = readVarint(_.stream, &pos);

            gen_line += d_gen_line;
            gen_col += d_gen_col;
            orig_line += d_orig_line;
            orig_col += d_orig_col;
            src_idx += d_src_idx;

            if (gen_line > line || (gen_line == line && gen_col > column)) {
                break;
            }

            best_match = SourceMapping{
                generated_line: gen_line,
                generated_column: gen_col,
                source_index: src_idx,
                original_line: orig_line,
                original_column: orig_col,
                name_index: -1,
            };
        }

        return best_match;
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.sync_entries.len > 0) {
            allocator.free(_.sync_entries);
            _.sync_entries = [];
        }
        if (_.stream.len > 0) {
            allocator.free(_.stream);
            _.stream = [];
        }
        _.total_mappings = 0;
    }
}

/// 紧凑二进制构建器
pub struct InternalSourceMapBuilder {
    sync_list: std.ArrayListUnmanaged(SyncEntry) = .empty,
    stream_list: std.ArrayListUnmanaged(u8) = .empty,
    last_mapping: SourceMapping = SourceMapping{},
    count: usize = 0,

    pub static fn init(): InternalSourceMapBuilder {
        return InternalSourceMapBuilder{
            sync_list: .empty,
            stream_list: .empty,
            last_mapping: SourceMapping{},
            count: 0,
        };
    }

    pub fn addMapping(*_, allocator: std.mem.Allocator, m: SourceMapping): !void {
        if (_.count % SYNC_INTERVAL == 0) {
            const entry = SyncEntry{
                generated_line: m.generated_line,
                generated_column: m.generated_column,
                byte_offset: _.stream_list.items.len as u32,
                original_line: m.original_line,
                original_column: m.original_column,
                source_index: m.source_index,
            };
            try _.sync_list.append(allocator, entry);
        } else {
            // 写入与上一条 mapping 的相对增量
            let varint_buf: [32]u8 = undefined;
            const d_gl = m.generated_line - _.last_mapping.generated_line;
            const d_gc = m.generated_column - _.last_mapping.generated_column;
            const d_ol = m.original_line - _.last_mapping.original_line;
            const d_oc = m.original_column - _.last_mapping.original_column;
            const d_si = m.source_index - _.last_mapping.source_index;

            let n = writeVarint(&varint_buf, 0, d_gl);
            n += writeVarint(&varint_buf, n, d_gc);
            n += writeVarint(&varint_buf, n, d_ol);
            n += writeVarint(&varint_buf, n, d_oc);
            n += writeVarint(&varint_buf, n, d_si);

            for (b in varint_buf[0..n]) {
                try _.stream_list.append(allocator, b);
            }
        }

        _.last_mapping = m;
        _.count += 1;
    }

    pub fn build(*_, allocator: std.mem.Allocator): InternalSourceMap! {
        const syncs = try _.sync_list.toOwnedSlice(allocator);
        const stream = try _.stream_list.toOwnedSlice(allocator);
        return InternalSourceMap{
            sync_entries: syncs,
            stream: stream,
            total_mappings: _.count,
        };
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.sync_list.deinit(allocator);
        _.stream_list.deinit(allocator);
        _.count = 0;
    }
}
