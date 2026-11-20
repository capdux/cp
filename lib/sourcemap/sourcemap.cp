// Source: /data/dev/bun/src/sourcemap/sourcemap.cp
// ============================================================================
// lib/sourcemap/sourcemap.cp — SourceMap v3 综合门面、生成器与消费者
// 吸收自 Bun src/sourcemap/sourcemap.cp
// ============================================================================

import { VLQ } from "./vlq.cp";
import { LineOffsetTable, DynamicLineOffsetTable, SourceLocation } from "./line_offset.cp";
import { SourceMapping, MappingList, ParsedSourceMap } from "./mapping.cp";
import { InternalSourceMap } from "./internal_source_map.cp";
import { Chunk, SourceMapChunk } from "./chunk.cp";
import { ParsedSourceMapRecord, OriginalPosition } from "./parsed_source_map.cp";
const std = @import("std");

pub struct SourceMapState {
    generated_line: i32 = 0,
    generated_column: i32 = 0,
    source_index: i32 = 0,
    original_line: i32 = 0,
    original_column: i32 = 0,
    name_index: i32 = 0,
}

/// SourceMap v3 生成器
pub struct SourceMapGenerator {
    allocator: std.mem.Allocator,
    chunk: Chunk,
    file_name: string,

    pub static fn init(allocator: std.mem.Allocator, file_name: string): SourceMapGenerator {
        return SourceMapGenerator{
            allocator: allocator,
            chunk: Chunk.init(),
            file_name: file_name,
        };
    }

    pub fn setSourceRoot(*_, root: string): !void {
        try _.chunk.setSourceRoot(_.allocator, root);
    }

    pub fn addSource(*_, source_path: string): i32! {
        return _.chunk.addSource(_.allocator, source_path, "");
    }

    pub fn addSourceWithContent(*_, source_path: string, content: string): i32! {
        return _.chunk.addSource(_.allocator, source_path, content);
    }

    pub fn addName(*_, name: string): i32! {
        return _.chunk.addName(_.allocator, name);
    }

    pub fn addMapping(*_, gen_line: i32, gen_col: i32, orig_line: i32, orig_col: i32, src_idx: i32, name_idx: i32): !void {
        const m = SourceMapping{
            generated_line: gen_line,
            generated_column: gen_col,
            source_index: src_idx,
            original_line: orig_line,
            original_column: orig_col,
            name_index: name_idx,
        };
        try _.chunk.addMapping(_.allocator, m);
    }

    pub fn appendChunk(*_, other: &Chunk, line_offset: i32, col_offset: i32): !void {
        try _.chunk.appendChunk(_.allocator, other, line_offset, col_offset);
    }

    pub fn sortMappings(*_) {
        _.chunk.sortMappings();
    }

    pub fn generateMappings(*_, allocator: std.mem.Allocator): []u8! {
        return _.chunk.generateMappingsString(allocator);
    }

    pub fn generateJSON(*_, allocator: std.mem.Allocator, include_contents: bool): []u8! {
        return _.chunk.printSourceMapJSON(allocator, _.file_name, include_contents);
    }

    pub fn deinit(*_) {
        _.chunk.deinit(_.allocator);
    }
}

/// SourceMap v3 消费者（支持对完整 SourceMap JSON 进行反查）
pub struct SourceMapConsumer {
    allocator: std.mem.Allocator,
    record: ParsedSourceMapRecord,

    pub static fn init(allocator: std.mem.Allocator, json_content: string): SourceMapConsumer! {
        const rec = try ParsedSourceMapRecord.parse(allocator, json_content);
        return SourceMapConsumer{
            allocator: allocator,
            record: rec,
        };
    }

    pub fn originalPositionFor(*_, gen_line: i32, gen_col: i32): OriginalPosition? {
        return _.record.findOriginalPosition(gen_line, gen_col);
    }

    pub fn mappingFor(*_, gen_line: i32, gen_col: i32): SourceMapping? {
        return _.record.findMapping(gen_line, gen_col);
    }

    pub fn sourceContentFor(*_, source_path: string): string? {
        let i: usize = 0;
        while (i < _.record.sources.len) {
            if (std.mem.eql(u8, _.record.sources[i], source_path)) {
                if (i < _.record.sources_content.len && _.record.sources_content[i].len > 0) {
                    return _.record.sources_content[i];
                }
                return null;
            }
            i += 1;
        }
        return null;
    }

    pub fn deinit(*_) {
        _.record.deinit(_.allocator);
    }
}

pub struct SourceMap {
    pub const Generator = SourceMapGenerator;
    pub const Consumer = SourceMapConsumer;
    pub const Chunk = SourceMapChunk;
    pub const List = MappingList;
}
