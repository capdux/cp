// Source: bun/src/sourcemap/Chunk.cp
// ============================================================================
// lib/sourcemap/chunk.cp — 构建产物分块（Chunk）与 SourceMap v3 拼接引擎
// 吸收自 Bun src/sourcemap/Chunk.cp
// ============================================================================

import { SourceMapping } from "./mapping.cp";
import { VLQ } from "./vlq.cp";
const std = @import("std");

pub struct Chunk {
    mappings_count: usize = 0,
    final_generated_line: i32 = 0,
    final_generated_column: i32 = 0,
    should_ignore: bool = false,

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
            sources: .empty,
            sources_content: .empty,
            names: .empty,
            mappings: .empty,
        };
    }

    pub fn addSource(*_, allocator: std.mem.Allocator, source_path: string, content: string): i32! {
        let idx = 0;
        for (s in _.sources.items) {
            if (std.mem.eql(u8, s, source_path)) {
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
        let idx = 0;
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

    /// 生成符合 SourceMap v3 规范的标准 mappings 字符串
    pub fn generateMappingsString(*_, allocator: std.mem.Allocator): []u8! {
        let buf: std.ArrayListUnmanaged(u8) = .empty;
        errdefer buf.deinit(allocator);

        let last_gen_line: i32 = 0;
        let last_gen_col: i32 = 0;
        let last_src_idx: i32 = 0;
        let last_orig_line: i32 = 0;
        let last_orig_col: i32 = 0;
        let last_name_idx: i32 = 0;

        let is_first_in_line = true;

        for (m in _.mappings.items) {
            // 换行处理
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

            // 1. generated column 增量
            const d_gen_col = m.generated_column - last_gen_col;
            let vlq_gen_col = VLQ.encode(d_gen_col);
            for (b in vlq_gen_col.slice()) {
                try buf.append(allocator, b);
            }
            last_gen_col = m.generated_column;

            if (m.hasOriginal()) {
                // 2. source index 增量
                const d_src = m.source_index - last_src_idx;
                let vlq_src = VLQ.encode(d_src);
                for (b in vlq_src.slice()) {
                    try buf.append(allocator, b);
                }
                last_src_idx = m.source_index;

                // 3. original line 增量
                const d_orig_line = m.original_line - last_orig_line;
                let vlq_orig_line = VLQ.encode(d_orig_line);
                for (b in vlq_orig_line.slice()) {
                    try buf.append(allocator, b);
                }
                last_orig_line = m.original_line;

                // 4. original column 增量
                const d_orig_col = m.original_column - last_orig_col;
                let vlq_orig_col = VLQ.encode(d_orig_col);
                for (b in vlq_orig_col.slice()) {
                    try buf.append(allocator, b);
                }
                last_orig_col = m.original_column;

                // 5. name index 增量（可选）
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

    /// 渲染完整的 SourceMap v3 JSON 字符串
    pub fn printSourceMapJSON(*_, allocator: std.mem.Allocator, file_name: string, include_contents: bool): []u8! {
        const mappings_str = try _.generateMappingsString(allocator);
        defer allocator.free(mappings_str);

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);

        // header
        try appendString(&out, allocator, "{\n  \"version\": 3,\n  \"file\": \"");
        try appendString(&out, allocator, file_name);
        try appendString(&out, allocator, "\",\n  \"sources\": [");

        // sources
        let i = 0;
        for (s in _.sources.items) {
            if (i > 0) try appendString(&out, allocator, ", ");
            try appendString(&out, allocator, "\"");
            try appendString(&out, allocator, s);
            try appendString(&out, allocator, "\"");
            i += 1;
        }
        try appendString(&out, allocator, "]");

        // sourcesContent
        if (include_contents && _.sources_content.items.len > 0) {
            try appendString(&out, allocator, ",\n  \"sourcesContent\": [");
            let j = 0;
            for (sc in _.sources_content.items) {
                if (j > 0) try appendString(&out, allocator, ", ");
                try appendString(&out, allocator, "\"");
                // Escape JSON special chars if any
                for (c in sc) {
                    if (c == '"') {
                        try appendString(&out, allocator, "\\\"");
                    } else if (c == '\\') {
                        try appendString(&out, allocator, "\\\\");
                    } else if (c == '\n') {
                        try appendString(&out, allocator, "\\n");
                    } else if (c == '\r') {
                        try appendString(&out, allocator, "\\r");
                    } else if (c == '\t') {
                        try appendString(&out, allocator, "\\t");
                    } else {
                        try out.append(allocator, c);
                    }
                }
                try appendString(&out, allocator, "\"");
                j += 1;
            }
            try appendString(&out, allocator, "]");
        }

        // names
        try appendString(&out, allocator, ",\n  \"names\": [");
        let k = 0;
        for (n in _.names.items) {
            if (k > 0) try appendString(&out, allocator, ", ");
            try appendString(&out, allocator, "\"");
            try appendString(&out, allocator, n);
            try appendString(&out, allocator, "\"");
            k += 1;
        }
        try appendString(&out, allocator, "],\n  \"mappings\": \"");
        try appendString(&out, allocator, mappings_str);
        try appendString(&out, allocator, "\"\n}\n");

        return try out.toOwnedSlice(allocator);
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        for (s in _.sources.items) {
            allocator.free(s);
        }
        _.sources.deinit(allocator);

        for (sc in _.sources_content.items) {
            if (sc.len > 0) allocator.free(sc);
        }
        _.sources_content.deinit(allocator);

        for (n in _.names.items) {
            allocator.free(n);
        }
        _.names.deinit(allocator);

        _.mappings.deinit(allocator);
        _.mappings_count = 0;
    }
}

fn appendString(list: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, s: []const u8): !void {
    for (b in s) {
        try list.append(allocator, b);
    }
}
