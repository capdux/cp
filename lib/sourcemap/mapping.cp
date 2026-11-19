// Source: /data/dev/bun/src/sourcemap/Mapping.cp
// ============================================================================
// lib/sourcemap/mapping.cp — SourceMap v3 映射表定义与位置反查
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

/// 编译期定容 SourceMap v3 映射表（支持解析 `"mappings"` VLQ 字符串与二分反查）
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

    /// 二分查找给定 (line, column) 对应的最近左侧映射项
    pub fn findMapping(line: i32, column: i32): SourceMapping? {
        if (_.count == 0) { return null; }

        let low: usize = 0;
        let count: usize = _.count;

        while (count > 0) {
            let step = count / 2;
            let mid = low + step;
            let item = _.items[mid];

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
        return _.items[low - 1];
    }
}
