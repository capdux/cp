// Source: Absorbed from /data/dev/bun/src/sourcemap/ParsedSourceMap.cp | Synced: 2026-11-20T11:34:17Z
// ============================================================================
// lib/sourcemap/parsed_source_map.cp — SourceMap v3 JSON 解析、序列化与完整映射反查
// 吸收自 Bun src/sourcemap/ParsedSourceMap.cp
// ============================================================================

import { SourceMapping, MappingList, binarySearchMappings, encodeMappingsList } from "./mapping.cp";
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
    source_root: string = "",
    sources: []string = [],
    sources_content: []string = [],
    names: []string = [],
    mappings: []SourceMapping = [],

    pub static fn parseMappingsOnly(allocator: std.mem.Allocator, mappings_str: string): ![]SourceMapping {
        let mlist = try MappingList.parseMappings(allocator, mappings_str);
        return try mlist.items.toOwnedSlice(allocator);
    }

    /// 解析完整 SourceMap v3 JSON 报文（支持转义字符串、null sourcesContent、sourceRoot、version）
    pub static fn parse(allocator: std.mem.Allocator, json_content: string): !ParsedSourceMapRecord {
        let record = ParsedSourceMapRecord{
            version: 3,
            file: "",
            source_root: "",
            sources: [],
            sources_content: [],
            names: [],
            mappings: [],
        };
        errdefer record.deinit(allocator);

        let pos: usize = skipWs(json_content, 0);
        if (pos >= json_content.len || json_content[pos] != '{') {
            return record;
        }
        pos += 1;

        let key_buf: [128]u8 = undefined;
        while (pos < json_content.len) {
            pos = skipWs(json_content, pos);
            if (pos >= json_content.len || json_content[pos] == '}') break;
            if (json_content[pos] == ',') {
                pos += 1;
                continue;
            }
            if (json_content[pos] != '"') {
                pos += 1;
                continue;
            }

            let key_res = readJsonStringRaw(json_content, pos);
            if (!key_res.valid) break;
            let key_len = decodeJsonStringInto(json_content[key_res.start..key_res.end], &key_buf);
            let key = key_buf[0..key_len];
            pos = skipWs(json_content, key_res.next_pos);

            if (pos >= json_content.len || json_content[pos] != ':') break;
            pos = skipWs(json_content, pos + 1);
            if (pos >= json_content.len) break;

            if (std.mem.eql(u8, key, "version")) {
                let num_res = readJsonInt(json_content, pos);
                if (num_res.valid) {
                    record.version = num_res.value;
                    pos = num_res.next_pos;
                } else {
                    pos = skipJsonValue(json_content, pos);
                }
            } else if (std.mem.eql(u8, key, "file")) {
                if (json_content[pos] == '"') {
                    let val_res = readJsonStringRaw(json_content, pos);
                    if (val_res.valid) {
                        if (record.file.len > 0) allocator.free(record.file);
                        record.file = try unescapeJsonStringAlloc(allocator, json_content[val_res.start..val_res.end]);
                        pos = val_res.next_pos;
                    } else {
                        pos = skipJsonValue(json_content, pos);
                    }
                } else {
                    pos = skipJsonValue(json_content, pos);
                }
            } else if (std.mem.eql(u8, key, "sourceRoot")) {
                if (json_content[pos] == '"') {
                    let val_res = readJsonStringRaw(json_content, pos);
                    if (val_res.valid) {
                        if (record.source_root.len > 0) allocator.free(record.source_root);
                        record.source_root = try unescapeJsonStringAlloc(allocator, json_content[val_res.start..val_res.end]);
                        pos = val_res.next_pos;
                    } else {
                        pos = skipJsonValue(json_content, pos);
                    }
                } else {
                    pos = skipJsonValue(json_content, pos);
                }
            } else if (std.mem.eql(u8, key, "mappings")) {
                if (json_content[pos] == '"') {
                    let val_res = readJsonStringRaw(json_content, pos);
                    if (val_res.valid) {
                        if (record.mappings.len > 0) allocator.free(record.mappings);
                        record.mappings = try parseMappingsOnly(allocator, json_content[val_res.start..val_res.end]);
                        pos = val_res.next_pos;
                    } else {
                        pos = skipJsonValue(json_content, pos);
                    }
                } else {
                    pos = skipJsonValue(json_content, pos);
                }
            } else if (std.mem.eql(u8, key, "sources")) {
                let arr_res = try readJsonStringArray(allocator, json_content, pos);
                record.sources = arr_res.items;
                pos = arr_res.next_pos;
            } else if (std.mem.eql(u8, key, "sourcesContent")) {
                let arr_res = try readJsonStringArray(allocator, json_content, pos);
                record.sources_content = arr_res.items;
                pos = arr_res.next_pos;
            } else if (std.mem.eql(u8, key, "names")) {
                let arr_res = try readJsonStringArray(allocator, json_content, pos);
                record.names = arr_res.items;
                pos = arr_res.next_pos;
            } else {
                pos = skipJsonValue(json_content, pos);
            }
        }

        return record;
    }

    /// 序列化为标准 SourceMap v3 JSON 字符串
    pub fn toJSON(*_, allocator: std.mem.Allocator, include_contents: bool): []u8! {
        const mappings_str = try encodeMappingsList(allocator, _.mappings);
        defer allocator.free(mappings_str);

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);

        try appendBytes(&out, allocator, "{\n  \"version\": 3,\n  \"file\": ");
        try appendJsonQuoted(&out, allocator, _.file);
        if (_.source_root.len > 0) {
            try appendBytes(&out, allocator, ",\n  \"sourceRoot\": ");
            try appendJsonQuoted(&out, allocator, _.source_root);
        }
        try appendBytes(&out, allocator, ",\n  \"sources\": [");
        let i: usize = 0;
        for (s in _.sources) {
            if (i > 0) try appendBytes(&out, allocator, ", ");
            try appendJsonQuoted(&out, allocator, s);
            i += 1;
        }
        try appendBytes(&out, allocator, "]");

        if (include_contents && _.sources_content.len > 0) {
            try appendBytes(&out, allocator, ",\n  \"sourcesContent\": [");
            let j: usize = 0;
            for (sc in _.sources_content) {
                if (j > 0) try appendBytes(&out, allocator, ", ");
                try appendJsonQuoted(&out, allocator, sc);
                j += 1;
            }
            try appendBytes(&out, allocator, "]");
        }

        try appendBytes(&out, allocator, ",\n  \"names\": [");
        let k: usize = 0;
        for (n in _.names) {
            if (k > 0) try appendBytes(&out, allocator, ", ");
            try appendJsonQuoted(&out, allocator, n);
            k += 1;
        }
        try appendBytes(&out, allocator, "],\n  \"mappings\": ");
        try appendJsonQuoted(&out, allocator, mappings_str);
        try appendBytes(&out, allocator, "\n}\n");

        return try out.toOwnedSlice(allocator);
    }

    /// 二分查找给定 (line, column) 对应的映射项
    pub fn findMapping(line: i32, column: i32): SourceMapping? {
        if (idx := binarySearchMappings(_.mappings, line, column)) {
            return _.mappings[idx];
        }
        return null;
    }

    pub fn find(line: i32, column: i32): SourceMapping? {
        return _.findMapping(line, column);
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
        if (_.source_root.len > 0) {
            allocator.free(_.source_root);
            _.source_root = "";
        }
        for (s in _.sources) {
            if (s.len > 0) allocator.free(s);
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
            if (n.len > 0) allocator.free(n);
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

struct JsonRawSlice {
    start: usize = 0,
    end: usize = 0,
    next_pos: usize = 0,
    valid: bool = false,
}

struct JsonIntResult {
    value: i32 = 0,
    next_pos: usize = 0,
    valid: bool = false,
}

struct JsonArrayResult {
    items: []string = [],
    next_pos: usize = 0,
}

fn skipWs(json: string, start: usize): usize {
    let pos = start;
    while (pos < json.len && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
        pos += 1;
    }
    return pos;
}

fn readJsonStringRaw(json: string, start: usize): JsonRawSlice {
    if (start >= json.len || json[start] != '"') return JsonRawSlice{};
    let pos = start + 1;
    let content_start = pos;
    while (pos < json.len) {
        if (json[pos] == '\\') {
            pos += 2;
            continue;
        }
        if (json[pos] == '"') {
            return JsonRawSlice{
                start: content_start,
                end: pos,
                next_pos: pos + 1,
                valid: true,
            };
        }
        pos += 1;
    }
    return JsonRawSlice{};
}

fn readJsonInt(json: string, start: usize): JsonIntResult {
    let pos = start;
    let neg = false;
    if (pos < json.len && json[pos] == '-') {
        neg = true;
        pos += 1;
    }
    let val: i32 = 0;
    let digits: usize = 0;
    while (pos < json.len && json[pos] >= '0' && json[pos] <= '9') {
        val = val * 10 + ((json[pos] - '0') as i32);
        pos += 1;
        digits += 1;
    }
    if (digits == 0) return JsonIntResult{};
    return JsonIntResult{
        value: if (neg) (-val) else val,
        next_pos: pos,
        valid: true,
    };
}

fn skipJsonValue(json: string, start: usize): usize {
    let pos = skipWs(json, start);
    if (pos >= json.len) return pos;
    let ch = json[pos];
    if (ch == '"') {
        let s = readJsonStringRaw(json, pos);
        return if (s.valid) s.next_pos else json.len;
    }
    if (ch == '{' || ch == '[') {
        let depth: usize = 0;
        while (pos < json.len) {
            let c = json[pos];
            if (c == '"') {
                let s = readJsonStringRaw(json, pos);
                if (!s.valid) return json.len;
                pos = s.next_pos;
                continue;
            }
            if (c == '{' || c == '[') {
                depth += 1;
            } else if (c == '}' || c == ']') {
                if (depth > 0) depth -= 1;
                if (depth == 0) return pos + 1;
            }
            pos += 1;
        }
        return json.len;
    }
    while (pos < json.len && json[pos] != ',' && json[pos] != '}' && json[pos] != ']' &&
           json[pos] != ' ' && json[pos] != '\t' && json[pos] != '\r' && json[pos] != '\n')
    {
        pos += 1;
    }
    return pos;
}

fn hexDigit(c: u8): u16? {
    if (c >= '0' && c <= '9') return (c - '0') as u16;
    if (c >= 'a' && c <= 'f') return (c - 'a' + 10) as u16;
    if (c >= 'A' && c <= 'F') return (c - 'A' + 10) as u16;
    return null;
}

fn decodeJsonStringInto(raw: string, out: []u8): usize {
    let i: usize = 0;
    let o: usize = 0;
    while (i < raw.len && o < out.len) {
        if (raw[i] == '\\' && i + 1 < raw.len) {
            let esc = raw[i + 1];
            if (esc == '"') { out[o] = '"'; o += 1; i += 2; continue; }
            if (esc == '\\') { out[o] = '\\'; o += 1; i += 2; continue; }
            if (esc == '/') { out[o] = '/'; o += 1; i += 2; continue; }
            if (esc == 'n') { out[o] = '\n'; o += 1; i += 2; continue; }
            if (esc == 'r') { out[o] = '\r'; o += 1; i += 2; continue; }
            if (esc == 't') { out[o] = '\t'; o += 1; i += 2; continue; }
            if (esc == 'b') { out[o] = 8; o += 1; i += 2; continue; }
            if (esc == 'f') { out[o] = 12; o += 1; i += 2; continue; }
        }
        out[o] = raw[i];
        o += 1;
        i += 1;
    }
    return o;
}

fn unescapeJsonStringAlloc(allocator: std.mem.Allocator, raw: string): []u8! {
    let buf: std.ArrayListUnmanaged(u8) = .empty;
    errdefer buf.deinit(allocator);
    let i: usize = 0;
    while (i < raw.len) {
        if (raw[i] == '\\' && i + 1 < raw.len) {
            let esc = raw[i + 1];
            if (esc == '"') { try buf.append(allocator, '"'); i += 2; continue; }
            if (esc == '\\') { try buf.append(allocator, '\\'); i += 2; continue; }
            if (esc == '/') { try buf.append(allocator, '/'); i += 2; continue; }
            if (esc == 'n') { try buf.append(allocator, '\n'); i += 2; continue; }
            if (esc == 'r') { try buf.append(allocator, '\r'); i += 2; continue; }
            if (esc == 't') { try buf.append(allocator, '\t'); i += 2; continue; }
            if (esc == 'b') { try buf.append(allocator, 8); i += 2; continue; }
            if (esc == 'f') { try buf.append(allocator, 12); i += 2; continue; }
            if (esc == 'u' && i + 5 < raw.len) {
                let d0 = hexDigit(raw[i + 2]);
                let d1 = hexDigit(raw[i + 3]);
                let d2 = hexDigit(raw[i + 4]);
                let d3 = hexDigit(raw[i + 5]);
                if (d0 != null && d1 != null && d2 != null && d3 != null) {
                    let v0: u32 = (d0 ?? 0) as u32;
                    let v1: u32 = (d1 ?? 0) as u32;
                    let v2: u32 = (d2 ?? 0) as u32;
                    let v3: u32 = (d3 ?? 0) as u32;
                    let codepoint_val: u32 = (v0 << 12) | (v1 << 8) | (v2 << 4) | v3;
                    let cp: u21 = @intCast(codepoint_val);
                    let utf8_buf: [4]u8 = undefined;
                    let utf8_len = std.unicode.utf8Encode(cp, &utf8_buf) catch 0;
                    for (b in utf8_buf[0..utf8_len]) {
                        try buf.append(allocator, b);
                    }
                    i += 6;
                    continue;
                }
            }
        }
        try buf.append(allocator, raw[i]);
        i += 1;
    }
    return try buf.toOwnedSlice(allocator);
}

fn readJsonStringArray(allocator: std.mem.Allocator, json: string, start: usize): !JsonArrayResult {
    let pos = skipWs(json, start);
    if (pos >= json.len || json[pos] != '[') {
        return JsonArrayResult{ items: [], next_pos: skipJsonValue(json, pos) };
    }
    pos += 1;

    let list: std.ArrayListUnmanaged(string) = .empty;
    errdefer {
        for (item in list.items) {
            if (item.len > 0) allocator.free(item);
        }
        list.deinit(allocator);
    }

    while (pos < json.len) {
        pos = skipWs(json, pos);
        if (pos >= json.len) break;
        if (json[pos] == ']') {
            pos += 1;
            break;
        }
        if (json[pos] == ',') {
            pos += 1;
            continue;
        }
        if (json[pos] == '"') {
            let str_res = readJsonStringRaw(json, pos);
            if (!str_res.valid) break;
            let unesc = try unescapeJsonStringAlloc(allocator, json[str_res.start..str_res.end]);
            try list.append(allocator, unesc);
            pos = str_res.next_pos;
        } else if (pos + 4 <= json.len && std.mem.eql(u8, json[pos .. pos + 4], "null")) {
            try list.append(allocator, "");
            pos += 4;
        } else {
            pos = skipJsonValue(json, pos);
        }
    }

    return JsonArrayResult{
        items: try list.toOwnedSlice(allocator),
        next_pos: pos,
    };
}

pub fn appendBytes(list: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, s: []const u8): !void {
    for (b in s) {
        try list.append(allocator, b);
    }
}

pub fn appendJsonQuoted(list: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, s: []const u8): !void {
    try list.append(allocator, '"');
    for (c in s) {
        if (c == '"') {
            try appendBytes(list, allocator, "\\\"");
        } else if (c == '\\') {
            try appendBytes(list, allocator, "\\\\");
        } else if (c == '\n') {
            try appendBytes(list, allocator, "\\n");
        } else if (c == '\r') {
            try appendBytes(list, allocator, "\\r");
        } else if (c == '\t') {
            try appendBytes(list, allocator, "\\t");
        } else if (c == 8) {
            try appendBytes(list, allocator, "\\b");
        } else if (c == 12) {
            try appendBytes(list, allocator, "\\f");
        } else {
            try list.append(allocator, c);
        }
    }
    try list.append(allocator, '"');
}
