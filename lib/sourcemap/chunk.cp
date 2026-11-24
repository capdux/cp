// Source: Absorbed from /data/dev/bun/src/sourcemap/Chunk.cp | Synced: 2026-11-24T04:35:11Z
// ============================================================================
// lib/sourcemap/chunk.cp — 构建产物分块（Chunk / SourceMapChunk）与 SourceMap v3 拼接引擎
// 吸收自 Bun src/sourcemap/Chunk.cp
// ============================================================================

import { SourceMapping, binarySearchMappings, sortMappingsSlice, encodeMappingsList } from "./mapping.cp";
import { ParsedSourceMapRecord, appendBytes, appendJsonQuoted } from "./parsedSourceMap.cp";
import { VLQ } from "./vlq.cp";
const std = @import("std");

pub struct Chunk {
    mappings_count: usize = 0,
    final_generated_line: i32 = 0,
    final_generated_column: i32 = 0,
    should_ignore: bool = false,
    source_root: string = "",

    sources: std.ArrayListUnmanaged(string) = .empty,
    sources_content: std.ArrayListUnmanaged(string) = .empty,
    names: std.ArrayListUnmanaged(string) = .empty,
    mappings: std.ArrayListUnmanaged(SourceMapping) = .empty,

    pub static fn init(): Chunk {
        return Chunk{
            mappings_count: 0,
            final_generated_line: 0,
            final_generated_column: 0,
            should_ignore: false,
            source_root: "",
            sources: .empty,
            sources_content: .empty,
            names: .empty,
            mappings: .empty,
        };
    }

    pub fn setSourceRoot(*_, allocator: std.mem.Allocator, root: string): !void {
        if (_.source_root.len > 0) {
            allocator.free(_.source_root);
            _.source_root = "";
        }
        if (root.len > 0) {
            _.source_root = try allocator.dupe(u8, root);
        }
    }

    pub fn addSource(*_, allocator: std.mem.Allocator, source_path: string, content: string): i32! {
        let idx: usize = 0;
        for (s in _.sources.items) {
            if (std.mem.eql(u8, s, source_path)) {
                if (content.len > 0 && idx < _.sources_content.items.len && _.sources_content.items[idx].len == 0) {
                    _.sources_content.items[idx] = try allocator.dupe(u8, content);
                }
                return idx as i32;
            }
            idx += 1;
        }

        const new_idx = _.sources.items.len as i32;
        const dup_path = try allocator.dupe(u8, source_path);
        try _.sources.append(allocator, dup_path);

        const dup_content = if (content.len > 0) try allocator.dupe(u8, content) else "";
        try _.sources_content.append(allocator, dup_content);

        return new_idx;
    }

    pub fn addName(*_, allocator: std.mem.Allocator, name: string): i32! {
        let idx: usize = 0;
        for (n in _.names.items) {
            if (std.mem.eql(u8, n, name)) {
                return idx as i32;
            }
            idx += 1;
        }

        const new_idx = _.names.items.len as i32;
        const dup_name = try allocator.dupe(u8, name);
        try _.names.append(allocator, dup_name);
        return new_idx;
    }

    pub fn addMapping(*_, allocator: std.mem.Allocator, m: SourceMapping): !void {
        try _.mappings.append(allocator, m);
        _.mappings_count += 1;
        _.final_generated_line = m.generated_line;
        _.final_generated_column = m.generated_column;
    }

    pub fn sortMappings(*_) {
        sortMappingsSlice(_.mappings.items);
        if (_.mappings.items.len > 0) {
            let last = _.mappings.items[_.mappings.items.len - 1];
            _.final_generated_line = last.generated_line;
            _.final_generated_column = last.generated_column;
        }
    }

    pub fn find(*_, generated_line: i32, generated_col: i32): SourceMapping? {
        if (idx := binarySearchMappings(_.mappings.items, generated_line, generated_col)) {
            return _.mappings.items[idx];
        }
        return null;
    }

    /// 将另一段 SourceMapChunk 拼接合并到当前 Chunk 中（自动去重合并 sources/sourcesContent/names 并重映射行列与索引）
    pub fn appendChunk(*_, allocator: std.mem.Allocator, other: &Chunk, line_offset: i32, col_offset: i32): !void {
        let src_remap: std.ArrayListUnmanaged(i32) = .empty;
        defer src_remap.deinit(allocator);
        let i: usize = 0;
        while (i < other.sources.items.len) {
            let content: string = if (i < other.sources_content.items.len) other.sources_content.items[i] else "";
            let new_src_idx = try _.addSource(allocator, other.sources.items[i], content);
            try src_remap.append(allocator, new_src_idx);
            i += 1;
        }

        let name_remap: std.ArrayListUnmanaged(i32) = .empty;
        defer name_remap.deinit(allocator);
        for (n in other.names.items) {
            let new_name_idx = try _.addName(allocator, n);
            try name_remap.append(allocator, new_name_idx);
        }

        for (m in other.mappings.items) {
            let remapped_src: i32 = -1;
            if (m.source_index >= 0 && (m.source_index as usize) < src_remap.items.len) {
                remapped_src = src_remap.items[m.source_index as usize];
            }
            let remapped_name: i32 = -1;
            if (m.name_index >= 0 && (m.name_index as usize) < name_remap.items.len) {
                remapped_name = name_remap.items[m.name_index as usize];
            }
            let adj_col: i32 = if (m.generated_line == 0) (m.generated_column + col_offset) else m.generated_column;
            try _.addMapping(allocator, SourceMapping{
                generated_line: m.generated_line + line_offset,
                generated_column: adj_col,
                source_index: remapped_src,
                original_line: m.original_line,
                original_column: m.original_column,
                name_index: remapped_name,
            });
        }
    }

    /// 将一个已解析的 ParsedSourceMapRecord 转换为可拼接的 SourceMapChunk
    pub static fn fromParsed(allocator: std.mem.Allocator, rec: &ParsedSourceMapRecord): Chunk! {
        let chunk = Chunk.init();
        errdefer chunk.deinit(allocator);

        if (rec.source_root.len > 0) {
            try chunk.setSourceRoot(allocator, rec.source_root);
        }
        let i: usize = 0;
        while (i < rec.sources.len) {
            let content: string = if (i < rec.sources_content.len) rec.sources_content[i] else "";
            * = try chunk.addSource(allocator, rec.sources[i], content);
            i += 1;
        }
        for (n in rec.names) {
            * = try chunk.addName(allocator, n);
        }
        for (m in rec.mappings) {
            try chunk.addMapping(allocator, m);
        }
        return chunk;
    }

    /// 合并多段 SourceMapChunk 及其对应的输出起始行偏移，生成统一的拼接 SourceMap
    pub static fn merge(allocator: std.mem.Allocator, chunks: []const Chunk, line_offsets: []const i32): Chunk! {
        let merged = Chunk.init();
        errdefer merged.deinit(allocator);

        let i: usize = 0;
        while (i < chunks.len) {
            let off: i32 = if (i < line_offsets.len) line_offsets[i] else 0;
            try merged.appendChunk(allocator, &chunks[i], off, 0);
            i += 1;
        }
        merged.sortMappings();
        return merged;
    }

    /// 生成符合 SourceMap v3 规范的标准 mappings 字符串
    pub fn generateMappingsString(*_, allocator: std.mem.Allocator): []u8! {
        return encodeMappingsList(allocator, _.mappings.items);
    }

    /// 渲染完整的 SourceMap v3 JSON 字符串
    pub fn printSourceMapJSON(*_, allocator: std.mem.Allocator, file_name: string, include_contents: bool): []u8! {
        const mappings_str = try _.generateMappingsString(allocator);
        defer allocator.free(mappings_str);

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);

        try appendBytes(&out, allocator, "{\n  \"version\": 3,\n  \"file\": ");
        try appendJsonQuoted(&out, allocator, file_name);
        if (_.source_root.len > 0) {
            try appendBytes(&out, allocator, ",\n  \"sourceRoot\": ");
            try appendJsonQuoted(&out, allocator, _.source_root);
        }
        try appendBytes(&out, allocator, ",\n  \"sources\": [");

        let i: usize = 0;
        for (s in _.sources.items) {
            if (i > 0) try appendBytes(&out, allocator, ", ");
            try appendJsonQuoted(&out, allocator, s);
            i += 1;
        }
        try appendBytes(&out, allocator, "]");

        if (include_contents && _.sources_content.items.len > 0) {
            try appendBytes(&out, allocator, ",\n  \"sourcesContent\": [");
            let j: usize = 0;
            for (sc in _.sources_content.items) {
                if (j > 0) try appendBytes(&out, allocator, ", ");
                try appendJsonQuoted(&out, allocator, sc);
                j += 1;
            }
            try appendBytes(&out, allocator, "]");
        }

        try appendBytes(&out, allocator, ",\n  \"names\": [");
        let k: usize = 0;
        for (n in _.names.items) {
            if (k > 0) try appendBytes(&out, allocator, ", ");
            try appendJsonQuoted(&out, allocator, n);
            k += 1;
        }
        try appendBytes(&out, allocator, "],\n  \"mappings\": ");
        try appendJsonQuoted(&out, allocator, mappings_str);
        try appendBytes(&out, allocator, "\n}\n");

        return try out.toOwnedSlice(allocator);
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.source_root.len > 0) {
            allocator.free(_.source_root);
            _.source_root = "";
        }
        for (s in _.sources.items) {
            if (s.len > 0) allocator.free(s);
        }
        _.sources.deinit(allocator);

        for (sc in _.sources_content.items) {
            if (sc.len > 0) allocator.free(sc);
        }
        _.sources_content.deinit(allocator);

        for (n in _.names.items) {
            if (n.len > 0) allocator.free(n);
        }
        _.names.deinit(allocator);

        _.mappings.deinit(allocator);
        _.mappings_count = 0;
    }
}

pub const SourceMapChunk = Chunk;
