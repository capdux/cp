// Source: Absorbed from /data/dev/bun/src/sourcemap/LineOffsetTable.cp | Synced: 2026-11-20T04:56:21Z
// ============================================================================
// lib/sourcemap/line_offset.cp — 源码行起始字节偏移表与二分行号/列号定位器
// 吸收自 Bun src/sourcemap/LineOffsetTable.cp
// ============================================================================

const std = @import("std");

pub struct SourceLocation {
    line: u32 = 0,      // 0-based line index
    column: u32 = 0,    // 0-based byte column within line
}

pub struct LineRange {
    start: u32 = 0,
    end: u32 = 0,
}

fn findLineBinary(offsets: []const u32, byte_offset: u32): u32 {
    if (offsets.len == 0) { return 0; }
    let low: usize = 0;
    let count: usize = offsets.len;
    while (count > 0) {
        let step = count / 2;
        let mid = low + step;
        if (offsets[mid] <= byte_offset) {
            low = mid + 1;
            count = count - step - 1;
        } else {
            count = step;
        }
    }
    if (low == 0) { return 0; }
    return (low - 1) as u32;
}

/// 将行内 UTF-8 字节列号转换为 SourceMap v3 标准 UTF-16 码元列号
pub fn utf8ToUtf16Column(line_bytes: []const u8, byte_col: u32): u32 {
    let limit = if ((byte_col as usize) <= line_bytes.len) (byte_col as usize) else line_bytes.len;
    let i: usize = 0;
    let utf16_col: u32 = 0;
    while (i < limit) {
        let b = line_bytes[i];
        if (b < 0x80) {
            i += 1;
            utf16_col += 1;
        } else if ((b & 0xE0) == 0xC0) {
            i += 2;
            utf16_col += 1;
        } else if ((b & 0xF0) == 0xE0) {
            i += 3;
            utf16_col += 1;
        } else if ((b & 0xF8) == 0xF0) {
            i += 4;
            utf16_col += 2; // surrogate pair
        } else {
            i += 1;
            utf16_col += 1;
        }
    }
    return utf16_col;
}

/// 将 UTF-16 码元列号转换为行内 UTF-8 字节列号
pub fn utf16ToUtf8Column(line_bytes: []const u8, utf16_col: u32): u32 {
    let i: usize = 0;
    let cur_u16: u32 = 0;
    while (i < line_bytes.len && cur_u16 < utf16_col) {
        let b = line_bytes[i];
        if (b < 0x80) {
            i += 1;
            cur_u16 += 1;
        } else if ((b & 0xE0) == 0xC0) {
            i += 2;
            cur_u16 += 1;
        } else if ((b & 0xF0) == 0xE0) {
            i += 3;
            cur_u16 += 1;
        } else if ((b & 0xF8) == 0xF0) {
            i += 4;
            cur_u16 += 2;
        } else {
            i += 1;
            cur_u16 += 1;
        }
    }
    if (i > line_bytes.len) return line_bytes.len as u32;
    return i as u32;
}

/// 零堆分配编译期定容行偏移查找表（支持 O(log N) 二分字节偏移定位）
pub struct LineOffsetTable<const MAX_LINES: usize> {
    offsets: [MAX_LINES]u32 = undefined,
    line_count: usize = 0,
    total_bytes: u32 = 0,

    pub static fn build(contents: []const u8): LineOffsetTable<MAX_LINES> {
        let table = LineOffsetTable<MAX_LINES>{
            offsets: undefined,
            line_count: 0,
            total_bytes: contents.len as u32,
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

    pub fn lineCount(): usize {
        return _.line_count;
    }

    /// 二分查找给定字节偏移对应的 0-based 行号
    pub fn findLine(byte_offset: u32): u32 {
        return findLineBinary(_.offsets[0.._.line_count], byte_offset);
    }

    /// 获取给定行的起始与结束字节偏移（不含换行符）
    pub fn lineRange(line: u32): LineRange? {
        let idx = line as usize;
        if (idx >= _.line_count) return null;
        let start = _.offsets[idx];
        let end = if (idx + 1 < _.line_count) (_.offsets[idx + 1] - 1) else _.total_bytes;
        if (end < start) end = start;
        return LineRange{ start: start, end: end };
    }

    pub fn lineLength(line: u32): u32 {
        if (r := _.lineRange(line)) {
            return r.end - r.start;
        }
        return 0;
    }

    pub fn lineText(contents: []const u8, line: u32): []const u8 {
        if (r := _.lineRange(line)) {
            let s = if ((r.start as usize) <= contents.len) (r.start as usize) else contents.len;
            let e = if ((r.end as usize) <= contents.len) (r.end as usize) else contents.len;
            if (e > s && contents[e - 1] == '\r') {
                e -= 1;
            }
            return contents[s..e];
        }
        return "";
    }

    /// 将字节偏移转换为 0-based (line, column)
    pub fn resolveLocation(byte_offset: u32): SourceLocation {
        if (_.line_count == 0) return SourceLocation{};
        let line_idx = _.findLine(byte_offset);
        let line_start = _.offsets[line_idx as usize];
        let col: u32 = if (byte_offset >= line_start) (byte_offset - line_start) else (0 as u32);
        return SourceLocation{
            line: line_idx,
            column: col,
        };
    }

    /// 将字节偏移转换为 SourceMap v3 UTF-16 码元 (line, column)
    pub fn resolveUtf16Location(contents: []const u8, byte_offset: u32): SourceLocation {
        let loc = _.resolveLocation(byte_offset);
        let text = _.lineText(contents, loc.line);
        return SourceLocation{
            line: loc.line,
            column: utf8ToUtf16Column(text, loc.column),
        };
    }

    /// 将 0-based (line, column) 还原为字节偏移
    pub fn resolveByteOffset(line: u32, column: u32): u32 {
        if (line as usize >= _.line_count) return 0;
        return _.offsets[line as usize] + column;
    }

    /// 将 0-based (line, column) 还原为字节偏移并钳位到该行合法范围
    pub fn resolveByteOffsetClamped(line: u32, column: u32): u32 {
        if (r := _.lineRange(line)) {
            let max_col = r.end - r.start;
            let clamped = if (column <= max_col) column else max_col;
            return r.start + clamped;
        }
        return _.total_bytes;
    }
}

/// 动态堆分配行偏移查找表（适用于无界/超大源码文件）
pub struct DynamicLineOffsetTable {
    offsets: []u32 = [],
    line_count: usize = 0,
    total_bytes: u32 = 0,

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
            total_bytes: contents.len as u32,
        };
    }

    pub fn lineCount(): usize {
        return _.line_count;
    }

    pub fn findLine(byte_offset: u32): u32 {
        return findLineBinary(_.offsets[0.._.line_count], byte_offset);
    }

    pub fn lineRange(line: u32): LineRange? {
        let idx = line as usize;
        if (idx >= _.line_count) return null;
        let start = _.offsets[idx];
        let end = if (idx + 1 < _.line_count) (_.offsets[idx + 1] - 1) else _.total_bytes;
        if (end < start) end = start;
        return LineRange{ start: start, end: end };
    }

    pub fn lineLength(line: u32): u32 {
        if (r := _.lineRange(line)) {
            return r.end - r.start;
        }
        return 0;
    }

    pub fn lineText(contents: []const u8, line: u32): []const u8 {
        if (r := _.lineRange(line)) {
            let s = if ((r.start as usize) <= contents.len) (r.start as usize) else contents.len;
            let e = if ((r.end as usize) <= contents.len) (r.end as usize) else contents.len;
            if (e > s && contents[e - 1] == '\r') {
                e -= 1;
            }
            return contents[s..e];
        }
        return "";
    }

    pub fn resolveLocation(byte_offset: u32): SourceLocation {
        if (_.line_count == 0) return SourceLocation{};
        let line_idx = _.findLine(byte_offset);
        let line_start = _.offsets[line_idx as usize];
        let col: u32 = if (byte_offset >= line_start) (byte_offset - line_start) else (0 as u32);
        return SourceLocation{
            line: line_idx,
            column: col,
        };
    }

    pub fn resolveUtf16Location(contents: []const u8, byte_offset: u32): SourceLocation {
        let loc = _.resolveLocation(byte_offset);
        let text = _.lineText(contents, loc.line);
        return SourceLocation{
            line: loc.line,
            column: utf8ToUtf16Column(text, loc.column),
        };
    }

    pub fn resolveByteOffset(line: u32, column: u32): u32 {
        if (line as usize >= _.line_count) return 0;
        return _.offsets[line as usize] + column;
    }

    pub fn resolveByteOffsetClamped(line: u32, column: u32): u32 {
        if (r := _.lineRange(line)) {
            let max_col = r.end - r.start;
            let clamped = if (column <= max_col) column else max_col;
            return r.start + clamped;
        }
        return _.total_bytes;
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.offsets.len > 0) {
            allocator.free(_.offsets);
            _.offsets = [];
            _.line_count = 0;
            _.total_bytes = 0;
        }
    }
}
