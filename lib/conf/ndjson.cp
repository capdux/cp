// Source: cp/lib/ndjson/ndjson.cp (NDJSON / JSON Lines Streaming Framing, Validation & Writer Engine)
// Pure CP zero-C-dependency NDJSON (RFC 8259 / JSON Lines) streaming parser and frame-safe writer built on std.json.

const std = @import("std");

/// Unified error set for `cap:ndjson` streaming parser, writer, and batch operations.
pub err NdjsonError {
    InvalidJsonLine,
    MaxLineLengthExceeded,
    UnterminatedFinalLine,
    InvalidLimit,
    OutOfMemory,
}

/// A single parsed NDJSON record holding its 1-based physical line number,
/// owned raw line slice, and parsed `std.json.Value` tree.
pub struct NdjsonRecord {
    line_number: usize,
    raw_line: []const u8,
    parsed: std.json.Parsed(std.json.Value),
    alloc: std.mem.Allocator,

    /// Releases the parsed JSON arena and the owned `raw_line` buffer.
    pub fn deinit(*_) {
        _.parsed.deinit();
        _.alloc.free(_.raw_line);
    }
}

fn isWhitespaceOnly(bytes: []const u8): bool {
    for (b of bytes) {
        if (b != ' ' && b != '\t' && b != '\r') {
            return false;
        }
    }
    return true;
}

fn trimTrailingCr(bytes: []const u8): []const u8 {
    if (bytes.len > 0 && bytes[bytes.len - 1] == '\r') {
        return bytes[0 .. bytes.len - 1];
    }
    return bytes;
}

fn parseJsonLine(allocator: std.mem.Allocator, line_bytes: []const u8): std.json.Parsed(std.json.Value) ! NdjsonError {
    return std.json.parseFromSlice(std.json.Value, allocator, line_bytes, .{}) catch e => {
        if (e == error.OutOfMemory) {
            return NdjsonError.OutOfMemory;
        }
        return NdjsonError.InvalidJsonLine;
    };
}

/// Incremental chunked NDJSON / JSON Lines streaming parser.
/// Supports arbitrary chunk boundaries, `\n` and `\r\n` line endings,
/// skipping whitespace-only lines, per-line length DoS limits, and strict JSON validation.
pub struct NdjsonStreamParser {
    alloc: std.mem.Allocator,
    max_line_bytes: usize,
    line_buf: std.ArrayList(u8) = .empty,
    records: std.ArrayList(NdjsonRecord) = .empty,
    read_cursor: usize = 0,
    current_line_number: usize = 1,
    parsed_count: usize = 0,
    skipped_empty_lines: usize = 0,
    last_error_line: usize = 0,

    /// Initializes a streaming NDJSON parser with a strict per-line byte limit.
    /// Returns `NdjsonError.InvalidLimit` if `max_line_bytes == 0`.
    pub static fn init(allocator: std.mem.Allocator, max_line_bytes: usize): NdjsonStreamParser ! NdjsonError {
        if (max_line_bytes == 0) {
            return NdjsonError.InvalidLimit;
        }
        return NdjsonStreamParser{
            alloc: allocator,
            max_line_bytes: max_line_bytes,
            line_buf: .empty,
            records: .empty,
            read_cursor: 0,
            current_line_number: 1,
            parsed_count: 0,
            skipped_empty_lines: 0,
            last_error_line: 0,
        };
    }

    /// Releases all buffered bytes and all parsed `NdjsonRecord` entries owned by this parser.
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.records.items.len) : (i += 1) {
            _.records.items[i].deinit();
        }
        _.records.deinit(_.alloc);
        _.line_buf.deinit(_.alloc);
    }

    fn processLineSlice(*_, raw_slice: []const u8): void ! NdjsonError {
        const line_num = _.current_line_number;
        _.current_line_number += 1;

        if (isWhitespaceOnly(raw_slice)) {
            _.skipped_empty_lines += 1;
            return;
        }

        const owned_raw = _.alloc.dupe(u8, raw_slice) catch return NdjsonError.OutOfMemory;
        let parsed = parseJsonLine(_.alloc, owned_raw) catch e => {
            _.alloc.free(owned_raw);
            if (e == NdjsonError.InvalidJsonLine) {
                _.last_error_line = line_num;
            }
            return e;
        };

        _.records.append(_.alloc, NdjsonRecord{
            line_number: line_num,
            raw_line: owned_raw,
            parsed: parsed,
            alloc: _.alloc,
        }) catch {
            parsed.deinit();
            _.alloc.free(owned_raw);
            return NdjsonError.OutOfMemory;
        };

        _.parsed_count += 1;
    }

    /// Feeds an arbitrary chunk of bytes into the parser.
    /// Returns a read-only slice of the newly parsed `NdjsonRecord`s produced by this chunk.
    /// Records remain owned by the parser until `deinit()`, and can also be pulled via `nextRecord()`.
    pub fn feed(*_, chunk: []const u8): ([]const NdjsonRecord) ! NdjsonError {
        const start_idx = _.records.items.len;
        let pos: usize = 0;

        while (pos < chunk.len) {
            if (nl_idx := std.mem.indexOfScalarPos(u8, chunk, pos, '\n')) {
                const seg = chunk[pos..nl_idx];
                if (_.line_buf.items.len == 0) {
                    const clean = trimTrailingCr(seg);
                    if (clean.len > _.max_line_bytes) {
                        _.last_error_line = _.current_line_number;
                        return NdjsonError.MaxLineLengthExceeded;
                    }
                    _.processLineSlice(clean)?;
                } else {
                    _.line_buf.appendSlice(_.alloc, seg) catch return NdjsonError.OutOfMemory;
                    const clean = trimTrailingCr(_.line_buf.items);
                    if (clean.len > _.max_line_bytes) {
                        _.line_buf.clearRetainingCapacity();
                        _.last_error_line = _.current_line_number;
                        return NdjsonError.MaxLineLengthExceeded;
                    }
                    _.processLineSlice(clean) catch e => {
                        _.line_buf.clearRetainingCapacity();
                        return e;
                    };
                    _.line_buf.clearRetainingCapacity();
                }
                pos = nl_idx + 1;
            } else {
                const rem = chunk[pos..];
                if (_.line_buf.items.len + rem.len > _.max_line_bytes) {
                    // Allow 1 extra byte if it is a trailing '\r' waiting for '\n' in next chunk
                    const possible_cr = rem.len > 0 && rem[rem.len - 1] == '\r';
                    if (!possible_cr || _.line_buf.items.len + rem.len - 1 > _.max_line_bytes) {
                        _.line_buf.clearRetainingCapacity();
                        _.last_error_line = _.current_line_number;
                        return NdjsonError.MaxLineLengthExceeded;
                    }
                }
                _.line_buf.appendSlice(_.alloc, rem) catch return NdjsonError.OutOfMemory;
                break;
            }
        }

        return _.records.items[start_idx..];
    }

    /// Pulls the next unconsumed `NdjsonRecord` pointer owned by this parser, or `null` if caught up.
    pub fn nextRecord(*_): (*const NdjsonRecord)? {
        if (_.read_cursor >= _.records.items.len) {
            return null;
        }
        const ptr = &_.records.items[_.read_cursor];
        _.read_cursor += 1;
        return ptr;
    }

    /// Finalizes the stream after all chunks have been fed.
    /// If `allow_trailing_without_newline` is `false` and non-whitespace bytes remain without `\n`,
    /// returns `NdjsonError.UnterminatedFinalLine`.
    pub fn finish(*_, allow_trailing_without_newline: bool): ([]const NdjsonRecord) ! NdjsonError {
        const start_idx = _.records.items.len;
        if (_.line_buf.items.len == 0) {
            return _.records.items[start_idx..];
        }

        if (isWhitespaceOnly(_.line_buf.items)) {
            _.skipped_empty_lines += 1;
            _.line_buf.clearRetainingCapacity();
            return _.records.items[start_idx..];
        }

        if (!allow_trailing_without_newline) {
            _.last_error_line = _.current_line_number;
            return NdjsonError.UnterminatedFinalLine;
        }

        const clean = trimTrailingCr(_.line_buf.items);
        if (clean.len > _.max_line_bytes) {
            _.line_buf.clearRetainingCapacity();
            _.last_error_line = _.current_line_number;
            return NdjsonError.MaxLineLengthExceeded;
        }

        _.processLineSlice(clean) catch e => {
            _.line_buf.clearRetainingCapacity();
            return e;
        };
        _.line_buf.clearRetainingCapacity();
        return _.records.items[start_idx..];
    }

    /// Returns the total number of valid JSON records parsed so far.
    pub fn parsedCount(*_): usize {
        return _.parsed_count;
    }

    /// Returns the total number of blank/whitespace-only lines skipped so far.
    pub fn skippedEmptyLines(*_): usize {
        return _.skipped_empty_lines;
    }

    /// Returns the 1-based line number of the most recent framing or JSON validation error.
    pub fn lastErrorLine(*_): usize {
        return _.last_error_line;
    }
}

/// Frame-safe NDJSON writer that guarantees every record occupies a single line
/// and rejects raw `\n` / `\r` characters that would corrupt NDJSON framing.
pub struct NdjsonWriter {
    alloc: std.mem.Allocator,
    buf: std.ArrayList(u8) = .empty,

    /// Initializes an empty `NdjsonWriter`.
    pub static fn init(allocator: std.mem.Allocator): NdjsonWriter {
        return NdjsonWriter{
            alloc: allocator,
            buf: .empty,
        };
    }

    /// Releases the internal byte buffer.
    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    /// Serializes a `std.json.Value` as a compact single-line JSON record followed by `\n`.
    pub fn writeValue(*_, val: std.json.Value): void ! NdjsonError {
        const serialized = std.json.Stringify.valueAlloc(_.alloc, val, .{}) catch return NdjsonError.OutOfMemory;
        defer _.alloc.free(serialized);

        for (c of serialized) {
            if (c == '\n' || c == '\r') {
                return NdjsonError.InvalidJsonLine;
            }
        }

        _.buf.appendSlice(_.alloc, serialized) catch return NdjsonError.OutOfMemory;
        _.buf.append(_.alloc, '\n') catch return NdjsonError.OutOfMemory;
    }

    /// Validates and writes a pre-serialized JSON line followed by `\n`.
    /// Rejects any embedded unescaped `\n` or `\r`, empty/whitespace lines, and malformed JSON.
    pub fn writeRawJson(*_, line: []const u8): void ! NdjsonError {
        if (line.len == 0 || isWhitespaceOnly(line)) {
            return NdjsonError.InvalidJsonLine;
        }
        for (c of line) {
            if (c == '\n' || c == '\r') {
                return NdjsonError.InvalidJsonLine;
            }
        }

        const parsed = parseJsonLine(_.alloc, line)?;
        parsed.deinit();

        _.buf.appendSlice(_.alloc, line) catch return NdjsonError.OutOfMemory;
        _.buf.append(_.alloc, '\n') catch return NdjsonError.OutOfMemory;
    }

    /// Returns a read-only view of the accumulated NDJSON output bytes.
    pub fn bytes(*_): []const u8 {
        return _.buf.items;
    }
}

/// Convenience one-shot parser for complete NDJSON input.
/// Caller owns the returned slice and must call `.deinit()` on each record and `allocator.free(records)`.
pub fn parseAll(allocator: std.mem.Allocator, input: []const u8): ([]NdjsonRecord) ! NdjsonError {
    let parser = NdjsonStreamParser.init(allocator, 64 * 1024 * 1024)?;
    errdefer parser.deinit();

    * = parser.feed(input)?;
    * = parser.finish(true)?;

    const owned = parser.records.toOwnedSlice(allocator) catch return NdjsonError.OutOfMemory;
    parser.line_buf.deinit(allocator);
    return owned;
}

/// Convenience one-shot serializer for a slice of `std.json.Value` items.
/// Caller owns the returned byte slice and must free it with `allocator.free(...)`.
pub fn stringifyAll(allocator: std.mem.Allocator, values: []const std.json.Value): ([]u8) ! NdjsonError {
    let writer = NdjsonWriter.init(allocator);
    errdefer writer.deinit();

    for (v of values) {
        writer.writeValue(v)?;
    }
    return writer.buf.toOwnedSlice(allocator) catch return NdjsonError.OutOfMemory;
}
