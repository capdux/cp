// Source: cp/lib/json/ndjson.cp (NDJSON / JSON Lines Streaming Reader & Writer)
// Incremental chunked NDJSON / JSONL parser with line limits & compact line-delimited writer

const std = @import("std");
import { JsonValue } from "./value";
import { JsonParser, JsonParseOptions } from "./parser";
import { JsonSerializer } from "./serializer";

pub err NdjsonError {
    LineTooLong,
}

pub struct NdjsonReader {
    allocator: std.mem.Allocator = undefined,
    buf: std.ArrayList(u8) = .empty,
    line_number: usize = 0,
    record_count: usize = 0,
    max_line_bytes: usize = 16 * 1024 * 1024,
    max_depth: usize = 512,

    /// Initializes a streaming NDJSON reader with a default 16 MiB per-line limit.
    pub static fn init(allocator: std.mem.Allocator): NdjsonReader {
        return NdjsonReader{
            allocator: allocator,
            buf: .empty,
            line_number: 0,
            record_count: 0,
            max_line_bytes: 16 * 1024 * 1024,
            max_depth: 512,
        };
    }

    /// Initializes a streaming NDJSON reader with custom line-length DoS guard and JSON max depth.
    pub static fn initWithOptions(
        allocator: std.mem.Allocator,
        max_line_bytes: usize,
        max_depth: usize,
    ): NdjsonReader {
        return NdjsonReader{
            allocator: allocator,
            buf: .empty,
            line_number: 0,
            record_count: 0,
            max_line_bytes: max_line_bytes,
            max_depth: max_depth,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.allocator);
    }

    pub fn currentLine(): usize {
        return _.line_number;
    }

    pub fn totalRecords(): usize {
        return _.record_count;
    }

    pub fn bufferedBytes(): usize {
        return _.buf.items.len;
    }

    static fn trimAsciiLine(raw: []const u8): []const u8 {
        let start: usize = 0;
        let end: usize = raw.len;
        if (end > 0 && raw[end - 1] == '\r') {
            end -= 1;
        }
        while (start < end && (raw[start] == ' ' || raw[start] == '\t' || raw[start] == '\r')) {
            start += 1;
        }
        while (end > start && (raw[end - 1] == ' ' || raw[end - 1] == '\t' || raw[end - 1] == '\r')) {
            end -= 1;
        }
        return raw[start..end];
    }

    fn parseSingleLine(*_, raw_line: []const u8): JsonValue?! {
        _.line_number += 1;
        const trimmed = NdjsonReader.trimAsciiLine(raw_line);
        if (trimmed.len == 0) {
            return null;
        }
        // Duplicate the line so zero-copy string slices inside JsonValue never alias reusable chunk/line buffers.
        const owned = _.allocator.dupe(u8, trimmed)?;
        const val = JsonParser.parseWithOptions(
            _.allocator,
            owned,
            JsonParseOptions{ max_depth: _.max_depth },
        )?;
        _.record_count += 1;
        return val;
    }

    /// Incrementally feeds an arbitrary byte chunk into the NDJSON stream.
    /// Handles `\n` and `\r\n` boundaries across chunks, ignores blank lines, and enforces `max_line_bytes`.
    pub fn feed(*_, chunk: []const u8): ([]JsonValue)! {
        let records: std.ArrayList(JsonValue) = .empty;
        errdefer records.deinit(_.allocator);

        let pos: usize = 0;
        while (pos < chunk.len) {
            const rem = chunk[pos..];
            const nl_opt = std.mem.indexOfScalar(u8, rem, '\n');
            if (nl_pos := nl_opt) {
                const seg = rem[0..nl_pos];
                if (_.buf.items.len + seg.len > _.max_line_bytes) {
                    return err.LineTooLong;
                }
                if (_.buf.items.len > 0) {
                    _.buf.appendSlice(_.allocator, seg)?;
                    const parsed_opt = _.parseSingleLine(_.buf.items)?;
                    _.buf.clearRetainingCapacity();
                    if (v := parsed_opt) {
                        records.append(_.allocator, v)?;
                    }
                } else {
                    const parsed_opt = _.parseSingleLine(seg)?;
                    if (v := parsed_opt) {
                        records.append(_.allocator, v)?;
                    }
                }
                pos += nl_pos + 1;
            } else {
                if (_.buf.items.len + rem.len > _.max_line_bytes) {
                    return err.LineTooLong;
                }
                _.buf.appendSlice(_.allocator, rem)?;
                break;
            }
        }

        return records.toOwnedSlice(_.allocator);
    }

    /// Flushes any trailing line remaining at EOF (without a trailing `\n`).
    pub fn finish(*_): ([]JsonValue)! {
        let records: std.ArrayList(JsonValue) = .empty;
        errdefer records.deinit(_.allocator);

        if (_.buf.items.len > 0) {
            const parsed_opt = _.parseSingleLine(_.buf.items)?;
            _.buf.clearRetainingCapacity();
            if (v := parsed_opt) {
                records.append(_.allocator, v)?;
            }
        }

        return records.toOwnedSlice(_.allocator);
    }

    /// Parses a complete NDJSON/JSONL payload in one shot.
    pub static fn parseAll(allocator: std.mem.Allocator, src: []const u8): ([]JsonValue)! {
        let reader = NdjsonReader.init(allocator);
        defer reader.deinit();

        let all: std.ArrayList(JsonValue) = .empty;
        errdefer all.deinit(allocator);

        const chunk_recs = reader.feed(src)?;
        defer {
            if (chunk_recs.len > 0) allocator.free(chunk_recs);
        }
        all.appendSlice(allocator, chunk_recs)?;

        const tail_recs = reader.finish()?;
        defer {
            if (tail_recs.len > 0) allocator.free(tail_recs);
        }
        all.appendSlice(allocator, tail_recs)?;

        return all.toOwnedSlice(allocator);
    }
}

pub struct NdjsonWriter {
    /// Appends a single `JsonValue` as compact single-line JSON followed by `\n` to `out`.
    pub static fn writeRecord(
        out: *std.ArrayList(u8),
        allocator: std.mem.Allocator,
        val: JsonValue,
    ): ! {
        const line = JsonSerializer.stringify(allocator, val)?;
        defer allocator.free(line);
        out.appendSlice(allocator, line)?;
        out.append(allocator, '\n')?;
    }

    /// Serializes a single `JsonValue` into a compact NDJSON line (`{...}\n`).
    pub static fn stringifyRecord(allocator: std.mem.Allocator, val: JsonValue): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        NdjsonWriter.writeRecord(&out, allocator, val)?;
        return out.toOwnedSlice(allocator);
    }

    /// Serializes a slice of `JsonValue` items into newline-delimited compact JSON (`\n` per record).
    pub static fn stringifyAll(allocator: std.mem.Allocator, values: []const JsonValue): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        for (v of values) {
            NdjsonWriter.writeRecord(&out, allocator, v)?;
        }
        return out.toOwnedSlice(allocator);
    }
}
