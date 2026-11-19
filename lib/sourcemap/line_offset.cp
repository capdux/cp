// Source: /data/dev/bun/src/sourcemap/LineOffsetTable.cp
// ============================================================================
// lib/sourcemap/line_offset.cp — 源码行起始字节偏移表与二分行号/列号定位器
// 吸收自 Bun src/sourcemap/LineOffsetTable.cp
// ============================================================================

const std = @import("std");

pub struct SourceLocation {
    line: u32 = 0,      // 0-based line index
    column: u32 = 0,    // 0-based byte column within line
}

/// 零堆分配编译期定容行偏移查找表（支持 O(log N) 二分字节偏移定位）
pub struct LineOffsetTable<const MAX_LINES: usize> {
    offsets: [MAX_LINES]u32 = undefined,
    line_count: usize = 0,

    pub static fn build(contents: []const u8): LineOffsetTable<MAX_LINES> {
        let table = LineOffsetTable<MAX_LINES>{
            offsets: undefined,
            line_count: 0,
        };
        if (MAX_LINES == 0) {
            return table;
        }
        table.offsets[0] = 0;
        table.line_count = 1;

        let i: usize = 0;
        while (i < contents.len) {
            if (contents[i] == 10) { // '\n'
                if (table.line_count < MAX_LINES && i + 1 <= contents.len) {
                    table.offsets[table.line_count] = (i + 1) as u32;
                    table.line_count += 1;
                }
            }
            i += 1;
        }
        return table;
    }

    /// 二分查找给定字节偏移对应的 0-based 行号
    pub fn findLine(byte_offset: u32): u32 {
        if (_.line_count == 0) { return 0; }
        let low: usize = 0;
        let count: usize = _.line_count;
        while (count > 0) {
            let step = count / 2;
            let mid = low + step;
            if (_.offsets[mid] <= byte_offset) {
                low = mid + 1;
                count = count - step - 1;
            } else {
                count = step;
            }
        }
        if (low == 0) { return 0; }
        return (low - 1) as u32;
    }

    /// 将字节偏移转换为 0-based (line, column)
    pub fn resolveLocation(byte_offset: u32): SourceLocation {
        let line_idx = _.findLine(byte_offset);
        let line_start = _.offsets[line_idx as usize];
        let col: u32 = if (byte_offset >= line_start) (byte_offset - line_start) else (0 as u32);
        return SourceLocation{
            line: line_idx,
            column: col,
        };
    }

    /// 将 0-based (line, column) 还原为字节偏移
    pub fn resolveByteOffset(line: u32, column: u32): u32 {
        if (line as usize >= _.line_count) return 0;
        return _.offsets[line as usize] + column;
    }
}

/// 动态堆分配行偏移查找表（适用于无界/超大源码文件）
pub struct DynamicLineOffsetTable {
    offsets: []u32 = [],
    line_count: usize = 0,

    pub static fn build(allocator: std.mem.Allocator, contents: []const u8): DynamicLineOffsetTable! {
        let list: std.ArrayListUnmanaged(u32) = .empty;
        errdefer list.deinit(allocator);

        try list.append(allocator, 0);

        let i: usize = 0;
        while (i < contents.len) {
            if (contents[i] == 10) { // '\n'
                if (i + 1 <= contents.len) {
                    try list.append(allocator, (i + 1) as u32);
                }
            }
            i += 1;
        }

        const slice = try list.toOwnedSlice(allocator);
        return DynamicLineOffsetTable{
            offsets: slice,
            line_count: slice.len,
        };
    }

    pub fn findLine(byte_offset: u32): u32 {
        if (_.line_count == 0) { return 0; }
        let low: usize = 0;
        let count: usize = _.line_count;
        while (count > 0) {
            let step = count / 2;
            let mid = low + step;
            if (_.offsets[mid] <= byte_offset) {
                low = mid + 1;
                count = count - step - 1;
            } else {
                count = step;
            }
        }
        if (low == 0) { return 0; }
        return (low - 1) as u32;
    }

    pub fn resolveLocation(byte_offset: u32): SourceLocation {
        let line_idx = _.findLine(byte_offset);
        let line_start = _.offsets[line_idx as usize];
        let col: u32 = if (byte_offset >= line_start) (byte_offset - line_start) else (0 as u32);
        return SourceLocation{
            line: line_idx,
            column: col,
        };
    }

    pub fn resolveByteOffset(line: u32, column: u32): u32 {
        if (line as usize >= _.line_count) return 0;
        return _.offsets[line as usize] + column;
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.offsets.len > 0) {
            allocator.free(_.offsets);
            _.offsets = [];
            _.line_count = 0;
        }
    }
}
