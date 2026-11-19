// Source: /data/dev/bun/src/sourcemap/ParsedSourceMap.cp
// ============================================================================
// lib/sourcemap/parsed_source_map.cp — SourceMap v3 JSON 解析与完整映射反查
// 吸收自 Bun src/sourcemap/ParsedSourceMap.cp
// ============================================================================

import { SourceMapping } from "./mapping.cp";
import { VLQ } from "./vlq.cp";
const std = @import("std");

pub struct OriginalPosition {
    source: string = "",
    source_index: i32 = -1,
    line: i32 = -1,
    column: i32 = -1,
    name: string = "",
    name_index: i32 = -1,
}

pub struct ParsedSourceMapRecord {
    version: i32 = 3,
    file: string = "",
    sources: []string = [],
    sources_content: []string = [],
    names: []string = [],
    mappings: []SourceMapping = [],

    pub static fn parseMappingsOnly(allocator: std.mem.Allocator, mappings_str: string): ![]SourceMapping {
        let list: std.ArrayListUnmanaged(SourceMapping) = .empty;
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
            if (b == 59) { // ';' -> 下一行
                gen_line += 1;
                gen_col = 0;
                pos += 1;
            } else if (b == 44) { // ',' -> 同行下一段
                pos += 1;
            } else {
                // 1. generated_column
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

                // 2, 3, 4. source_index, original_line, original_column
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

                        // 5. name_index
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

        return try list.toOwnedSlice(allocator);
    }

    /// 解析完整 SourceMap v3 JSON 报文
    pub static fn parse(allocator: std.mem.Allocator, json_content: string): !ParsedSourceMapRecord {
        let record = ParsedSourceMapRecord{
            version: 3,
            file: "",
            sources: [],
            sources_content: [],
            names: [],
            mappings: [],
        };

        // 提取 mappings
        if (mappings_val := extractJsonStringField(json_content, "mappings")) {
            record.mappings = try parseMappingsOnly(allocator, mappings_val);
        }

        // 提取 file
        if (file_val := extractJsonStringField(json_content, "file")) {
            record.file = try allocator.dupe(u8, file_val);
        }

        // 提取 sources 数组
        if (sources_list := extractJsonStringArray(allocator, json_content, "sources")) {
            record.sources = sources_list;
        }

        // 提取 sourcesContent 数组
        if (contents_list := extractJsonStringArray(allocator, json_content, "sourcesContent")) {
            record.sources_content = contents_list;
        }

        // 提取 names 数组
        if (names_list := extractJsonStringArray(allocator, json_content, "names")) {
            record.names = names_list;
        }

        return record;
    }

    /// 二分查找给定 (line, column) 对应的映射项
    pub fn findMapping(line: i32, column: i32): SourceMapping? {
        if (_.mappings.len == 0) return null;

        let low: usize = 0;
        let count: usize = _.mappings.len;

        while (count > 0) {
            let step = count / 2;
            let mid = low + step;
            const item = _.mappings[mid];

            let is_less_or_equal = (item.generated_line < line) ||
                (item.generated_line == line && item.generated_column <= column);

            if (is_less_or_equal) {
                low = mid + 1;
                count = count - step - 1;
            } else {
                count = step;
            }
        }

        if (low == 0) return null;
        return _.mappings[low - 1];
    }

    /// 查询原始源码位置与源码文件名
    pub fn findOriginalPosition(line: i32, column: i32): OriginalPosition? {
        if (m_opt := _.findMapping(line, column)) {
            let m = m_opt;
            if (m.hasOriginal()) {
                let src_name: string = "";
                if (m.source_index >= 0 && (m.source_index as usize) < _.sources.len) {
                    src_name = _.sources[m.source_index as usize];
                }

                let id_name: string = "";
                if (m.name_index >= 0 && (m.name_index as usize) < _.names.len) {
                    id_name = _.names[m.name_index as usize];
                }

                return OriginalPosition{
                    source: src_name,
                    source_index: m.source_index,
                    line: m.original_line,
                    column: m.original_column,
                    name: id_name,
                    name_index: m.name_index,
                };
            }
        }
        return null;
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.file.len > 0) {
            allocator.free(_.file);
            _.file = "";
        }
        for (s in _.sources) {
            allocator.free(s);
        }
        if (_.sources.len > 0) {
            allocator.free(_.sources);
            _.sources = [];
        }
        for (sc in _.sources_content) {
            if (sc.len > 0) allocator.free(sc);
        }
        if (_.sources_content.len > 0) {
            allocator.free(_.sources_content);
            _.sources_content = [];
        }
        for (n in _.names) {
            allocator.free(n);
        }
        if (_.names.len > 0) {
            allocator.free(_.names);
            _.names = [];
        }
        if (_.mappings.len > 0) {
            allocator.free(_.mappings);
            _.mappings = [];
        }
    }
}

fn extractJsonStringField(json: string, key: string): string? {
    let key_buf: [64]u8 = undefined;
    if (key.len + 3 > 64) return null;
    key_buf[0] = '"';
    @memcpy(key_buf[1 .. 1 + key.len], key);
    key_buf[1 + key.len] = '"';
    const key_str = key_buf[0 .. 2 + key.len];

    let idx_opt = std.mem.indexOf(u8, json, key_str);
    if (idx_opt == null) return null;
    let pos = (idx_opt ?? 0) + key_str.len;

    // Skip to colon
    while (pos < json.len && json[pos] != ':') {
        pos += 1;
    }
    if (pos >= json.len) return null;
    pos += 1;

    // Skip whitespace to quote
    while (pos < json.len && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
        pos += 1;
    }
    if (pos >= json.len || json[pos] != '"') return null;
    pos += 1;

    let start = pos;
    while (pos < json.len) {
        if (json[pos] == '\\') {
            pos += 2;
            continue;
        }
        if (json[pos] == '"') {
            return json[start..pos];
        }
        pos += 1;
    }
    return null;
}

fn extractJsonStringArray(allocator: std.mem.Allocator, json: string, key: string): ([]string)? {
    let key_buf: [64]u8 = undefined;
    if (key.len + 3 > 64) return null;
    key_buf[0] = '"';
    @memcpy(key_buf[1 .. 1 + key.len], key);
    key_buf[1 + key.len] = '"';
    const key_str = key_buf[0 .. 2 + key.len];

    let idx_opt = std.mem.indexOf(u8, json, key_str);
    if (idx_opt == null) return null;
    let pos = (idx_opt ?? 0) + key_str.len;

    while (pos < json.len && json[pos] != ':') {
        pos += 1;
    }
    if (pos >= json.len) return null;
    pos += 1;

    while (pos < json.len && json[pos] != '[') {
        if (json[pos] != ' ' && json[pos] != '\t' && json[pos] != '\r' && json[pos] != '\n') return null;
        pos += 1;
    }
    if (pos >= json.len) return null;
    pos += 1; // skip '['

    let list: std.ArrayListUnmanaged(string) = .empty;
    errdefer {
        for (item in list.items) allocator.free(item);
        list.deinit(allocator);
    }

    while (pos < json.len && json[pos] != ']') {
        while (pos < json.len && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n' || json[pos] == ',')) {
            pos += 1;
        }
        if (pos >= json.len || json[pos] == ']') break;
        if (json[pos] != '"') {
            pos += 1;
            continue;
        }
        pos += 1; // skip '"'
        let str_start = pos;
        while (pos < json.len) {
            if (json[pos] == '\\') {
                pos += 2;
                continue;
            }
            if (json[pos] == '"') {
                const s = json[str_start..pos];
                const dup = allocator.dupe(u8, s) catch return null;
                list.append(allocator, dup) catch return null;
                pos += 1;
                break;
            }
            pos += 1;
        }
    }

    return list.toOwnedSlice(allocator) catch null;
}
