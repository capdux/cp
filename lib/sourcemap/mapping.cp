// Source: Absorbed from /data/dev/bun/src/sourcemap/Mapping.cp | Synced: 2026-11-24T04:35:24Z
// ============================================================================
// lib/sourcemap/mapping.cp — SourceMap v3 映射表定义、排序与二分位置反查
// 吸收自 Bun src/sourcemap/Mapping.cp & ParsedSourceMap.cp
// ============================================================================

import { VLQ } from "./vlq.cp";
const std = @import("std");

pub struct LineColumnOffset {
    line: i32 = 0,
    column: i32 = 0,
}

/// 单条 SourceMap v3 位置映射项（全部字段 0-based，-1 代表缺失）
pub struct SourceMapping {
    generated_line: i32 = 0,
    generated_column: i32 = 0,
    source_index: i32 = -1,
    original_line: i32 = -1,
    original_column: i32 = -1,
    name_index: i32 = -1,

    pub fn hasOriginal(): bool {
        return _.source_index >= 0 && _.original_line >= 0 && _.original_column >= 0;
    }

    pub fn hasName(): bool {
        return _.name_index >= 0;
    }

    pub static fn lessThan(a: SourceMapping, b: SourceMapping): bool {
        if (a.generated_line != b.generated_line) {
            return a.generated_line < b.generated_line;
        }
        return a.generated_column < b.generated_column;
    }
}

pub struct MappingWithoutName {
    generated: LineColumnOffset,
    original: LineColumnOffset,
    source_index: i32,

    pub fn toNamed(): SourceMapping {
        return SourceMapping{
            generated_line: _.generated.line,
            generated_column: _.generated.column,
            source_index: _.source_index,
            original_line: _.original.line,
            original_column: _.original.column,
            name_index: -1,
        };
    }
}

pub fn binarySearchMappings(items: []const SourceMapping, line: i32, column: i32): usize? {
    if (items.len == 0) { return null; }

    let low: usize = 0;
    let count: usize = items.len;

    while (count > 0) {
        let step = count / 2;
        let mid = low + step;
        let item = items[mid];

        let is_less_or_equal = (item.generated_line < line) ||
            (item.generated_line == line && item.generated_column <= column);

        if (is_less_or_equal) {
            low = mid + 1;
            count = count - step - 1;
        } else {
            count = step;
        }
    }

    if (low == 0) { return null; }
    return low - 1;
}

/// 稳定原地归并/插入排序映射数组（按 generated_line, generated_column 升序）
pub fn sortMappingsSlice(items: []SourceMapping) {
    if (items.len <= 1) return;
    let i: usize = 1;
    while (i < items.len) {
        let key = items[i];
        let j: usize = i;
        while (j > 0 && SourceMapping.lessThan(key, items[j - 1])) {
            items[j] = items[j - 1];
            j -= 1;
        }
        items[j] = key;
        i += 1;
    }
}

/// 编码映射列表为 SourceMap v3 标准 VLQ mappings 字符串
pub fn encodeMappingsList(allocator: std.mem.Allocator, items: []const SourceMapping): []u8! {
    let buf: std.ArrayListUnmanaged(u8) = .empty;
    errdefer buf.deinit(allocator);

    let last_gen_line: i32 = 0;
    let last_gen_col: i32 = 0;
    let last_src_idx: i32 = 0;
    let last_orig_line: i32 = 0;
    let last_orig_col: i32 = 0;
    let last_name_idx: i32 = 0;

    let is_first_in_line = true;

    for (m in items) {
        if (m.generated_line > last_gen_line) {
            while (last_gen_line < m.generated_line) {
                try buf.append(allocator, 59); // ';'
                last_gen_line += 1;
            }
            last_gen_col = 0;
            is_first_in_line = true;
        } else if (!is_first_in_line) {
            try buf.append(allocator, 44); // ','
        }

        const d_gen_col = m.generated_column - last_gen_col;
        let vlq_gen_col = VLQ.encode(d_gen_col);
        for (b in vlq_gen_col.slice()) {
            try buf.append(allocator, b);
        }
        last_gen_col = m.generated_column;

        if (m.hasOriginal()) {
            const d_src = m.source_index - last_src_idx;
            let vlq_src = VLQ.encode(d_src);
            for (b in vlq_src.slice()) {
                try buf.append(allocator, b);
            }
            last_src_idx = m.source_index;

            const d_orig_line = m.original_line - last_orig_line;
            let vlq_orig_line = VLQ.encode(d_orig_line);
            for (b in vlq_orig_line.slice()) {
                try buf.append(allocator, b);
            }
            last_orig_line = m.original_line;

            const d_orig_col = m.original_column - last_orig_col;
            let vlq_orig_col = VLQ.encode(d_orig_col);
            for (b in vlq_orig_col.slice()) {
                try buf.append(allocator, b);
            }
            last_orig_col = m.original_column;

            if (m.hasName()) {
                const d_name = m.name_index - last_name_idx;
                let vlq_name = VLQ.encode(d_name);
                for (b in vlq_name.slice()) {
                    try buf.append(allocator, b);
                }
                last_name_idx = m.name_index;
            }
        }

        is_first_in_line = false;
    }

    return try buf.toOwnedSlice(allocator);
}

/// 动态可扩展映射表（对标 Bun Mapping.List：支持 append, sort, 二分查找 find/findIndex 与 VLQ 导出）
pub struct MappingList {
    items: std.ArrayListUnmanaged(SourceMapping) = .empty,
    is_sorted: bool = true,

    pub static fn init(): MappingList {
        return MappingList{
            items: .empty,
            is_sorted: true,
        };
    }

    pub fn len(): usize {
        return _.items.items.len;
    }

    pub fn get(index: usize): SourceMapping? {
        if (index >= _.items.items.len) return null;
        return _.items.items[index];
    }

    pub fn slice(): []const SourceMapping {
        return _.items.items;
    }

    pub fn append(*_, allocator: std.mem.Allocator, m: SourceMapping): !void {
        if (_.items.items.len > 0) {
            let last = _.items.items[_.items.items.len - 1];
            if (SourceMapping.lessThan(m, last)) {
                _.is_sorted = false;
            }
        }
        try _.items.append(allocator, m);
    }

    pub fn appendParts(
        *_,
        allocator: std.mem.Allocator,
        gen_line: i32,
        gen_col: i32,
        orig_line: i32,
        orig_col: i32,
        src_idx: i32,
        name_idx: i32,
    ): !void {
        try _.append(allocator, SourceMapping{
            generated_line: gen_line,
            generated_column: gen_col,
            source_index: src_idx,
            original_line: orig_line,
            original_column: orig_col,
            name_index: name_idx,
        });
    }

    pub fn sort(*_) {
        if (!_.is_sorted) {
            sortMappingsSlice(_.items.items);
            _.is_sorted = true;
        }
    }

    pub fn findIndex(*_, generated_line: i32, generated_col: i32): usize? {
        if (!_.is_sorted) {
            _.sort();
        }
        return binarySearchMappings(_.items.items, generated_line, generated_col);
    }

    pub fn find(*_, generated_line: i32, generated_col: i32): SourceMapping? {
        if (idx := _.findIndex(generated_line, generated_col)) {
            return _.items.items[idx];
        }
        return null;
    }

    /// 精确匹配同一生成行上的最近左侧映射（严格要求 generated_line 一致）
    pub fn findOnLine(*_, generated_line: i32, generated_col: i32): SourceMapping? {
        if (idx := _.findIndex(generated_line, generated_col)) {
            let m = _.items.items[idx];
            if (m.generated_line == generated_line) {
                return m;
            }
        }
        return null;
    }

    pub static fn parseMappings(allocator: std.mem.Allocator, mappings_str: []const u8): MappingList! {
        let list = MappingList.init();
        errdefer list.deinit(allocator);

        let gen_line: i32 = 0;
        let gen_col: i32 = 0;
        let src_idx: i32 = 0;
        let orig_line: i32 = 0;
        let orig_col: i32 = 0;
        let name_idx: i32 = 0;

        let pos: usize = 0;
        while (pos < mappings_str.len) {
            let b = mappings_str[pos];
            if (b == 59) {
                gen_line += 1;
                gen_col = 0;
                pos += 1;
            } else if (b == 44) {
                pos += 1;
            } else {
                let r1 = VLQ.decode(mappings_str, pos);
                if (!r1.valid) { break; }
                gen_col += r1.value;
                pos = r1.next_offset;

                let entry = SourceMapping{
                    generated_line: gen_line,
                    generated_column: gen_col,
                    source_index: -1,
                    original_line: -1,
                    original_column: -1,
                    name_index: -1,
                };

                if (pos < mappings_str.len && mappings_str[pos] != 44 && mappings_str[pos] != 59) {
                    let r2 = VLQ.decode(mappings_str, pos);
                    let r3 = VLQ.decode(mappings_str, r2.next_offset);
                    let r4 = VLQ.decode(mappings_str, r3.next_offset);
                    if (r2.valid && r3.valid && r4.valid) {
                        src_idx += r2.value;
                        orig_line += r3.value;
                        orig_col += r4.value;
                        pos = r4.next_offset;

                        entry.source_index = src_idx;
                        entry.original_line = orig_line;
                        entry.original_column = orig_col;

                        if (pos < mappings_str.len && mappings_str[pos] != 44 && mappings_str[pos] != 59) {
                            let r5 = VLQ.decode(mappings_str, pos);
                            if (r5.valid) {
                                name_idx += r5.value;
                                pos = r5.next_offset;
                                entry.name_index = name_idx;
                            }
                        }
                    }
                }

                try list.append(allocator, entry);
            }
        }

        return list;
    }

    pub fn toMappingsString(*_, allocator: std.mem.Allocator): []u8! {
        if (!_.is_sorted) {
            _.sort();
        }
        return encodeMappingsList(allocator, _.items.items);
    }

    pub fn clear(*_) {
        _.items.clearRetainingCapacity();
        _.is_sorted = true;
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.items.deinit(allocator);
        _.is_sorted = true;
    }
}

/// 编译期定容 SourceMap v3 映射表（支持解析 `"mappings"` VLQ 字符串、排序与二分反查）
pub struct ParsedSourceMap<const CAP: usize> {
    items: [CAP]SourceMapping = undefined,
    count: usize = 0,

    pub static fn init(): ParsedSourceMap<CAP> {
        return ParsedSourceMap<CAP>{
            items: undefined,
            count: 0,
        };
    }

    /// 解析 SourceMap v3 标准 `"mappings"` 字符串（`;` 分隔行，`,` 分隔段，每段 1/4/5 个相对 VLQ 值）
    pub static fn parseMappings(mappings_str: []const u8): ParsedSourceMap<CAP> {
        let table = ParsedSourceMap<CAP>.init();
        let gen_line: i32 = 0;
        let gen_col: i32 = 0;
        let src_idx: i32 = 0;
        let orig_line: i32 = 0;
        let orig_col: i32 = 0;
        let name_idx: i32 = 0;

        let pos: usize = 0;
        while (pos < mappings_str.len) {
            let b = mappings_str[pos];
            if (b == 59) { // ';' -> 换行，generated_column 归零
                gen_line += 1;
                gen_col = 0;
                pos += 1;
            } else if (b == 44) { // ',' -> 同行下一段
                pos += 1;
            } else {
                // 第 1 个 VLQ：generated_column 相对增量
                let r1 = VLQ.decode(mappings_str, pos);
                if (!r1.valid) { break; }
                gen_col += r1.value;
                pos = r1.next_offset;

                let entry = SourceMapping{
                    generated_line: gen_line,
                    generated_column: gen_col,
                    source_index: -1,
                    original_line: -1,
                    original_column: -1,
                    name_index: -1,
                };

                // 若后续紧跟更多 Base64 VLQ 字符（非 ',' 且非 ';'），依次读取第 2, 3, 4, (5) 个字段
                if (pos < mappings_str.len && mappings_str[pos] != 44 && mappings_str[pos] != 59) {
                    let r2 = VLQ.decode(mappings_str, pos);
                    let r3 = VLQ.decode(mappings_str, r2.next_offset);
                    let r4 = VLQ.decode(mappings_str, r3.next_offset);
                    if (r2.valid && r3.valid && r4.valid) {
                        src_idx += r2.value;
                        orig_line += r3.value;
                        orig_col += r4.value;
                        pos = r4.next_offset;

                        entry.source_index = src_idx;
                        entry.original_line = orig_line;
                        entry.original_column = orig_col;

                        if (pos < mappings_str.len && mappings_str[pos] != 44 && mappings_str[pos] != 59) {
                            let r5 = VLQ.decode(mappings_str, pos);
                            if (r5.valid) {
                                name_idx += r5.value;
                                pos = r5.next_offset;
                                entry.name_index = name_idx;
                            }
                        }
                    }
                }

                if (table.count < CAP) {
                    table.items[table.count] = entry;
                    table.count += 1;
                }
            }
        }

        return table;
    }

    pub fn sort(*_) {
        sortMappingsSlice(_.items[0.._.count]);
    }

    /// 二分查找给定 (line, column) 对应的最近左侧映射项
    pub fn findMapping(line: i32, column: i32): SourceMapping? {
        if (idx := binarySearchMappings(_.items[0.._.count], line, column)) {
            return _.items[idx];
        }
        return null;
    }

    pub fn find(line: i32, column: i32): SourceMapping? {
        return _.findMapping(line, column);
    }
}
