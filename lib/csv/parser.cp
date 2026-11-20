// Source: cp/lib/csv/parser.cp (RFC 4180 CSV & TSV Streaming Parser)
// RFC 4180 compliant CSV & TSV parser with streaming chunked reader, UTF-8 BOM stripping,
// comment/blank-line skipping, column count validation & DoS record-size limits.

const std = @import("std");

pub err CsvParseError {
    UnterminatedQuote,
    FieldCountMismatch,
    RecordTooLarge,
}

pub struct CsvOptions {
    delimiter: u8 = ',',
    quote: u8 = '"',
    has_header: bool = true,
    trim_spaces: bool = false,
    comment_char: u8? = null,
    allow_flexible_columns: bool = false,
    max_record_bytes: usize = 1048576,

    pub static fn defaults(): CsvOptions {
        return CsvOptions{};
    }

    pub static fn tsv(): CsvOptions {
        return CsvOptions{
            delimiter: '\t',
            quote: '"',
            has_header: true,
            trim_spaces: false,
            comment_char: null,
            allow_flexible_columns: false,
            max_record_bytes: 1048576,
        };
    }
}

pub struct CsvRow {
    allocator: std.mem.Allocator = undefined,
    fields: [][]const u8 = [],
    line_number: usize = 0,

    pub fn get(col: usize): ([]const u8)? {
        if (col >= _.fields.len) {
            return null;
        }
        return _.fields[col];
    }

    pub fn len(): usize {
        return _.fields.len;
    }

    pub fn deinit(*_) {
        for (f of _.fields) {
            if (f.len > 0) {
                _.allocator.free(f);
            }
        }
        if (_.fields.len > 0) {
            _.allocator.free(_.fields);
        }
        _.fields = [];
    }

    pub static fn freeRows(allocator: std.mem.Allocator, rows: []CsvRow) {
        let i: usize = 0;
        while (i < rows.len) : (i += 1) {
            let r = rows[i];
            r.deinit();
        }
        if (rows.len > 0) {
            allocator.free(rows);
        }
    }
}

enum CsvScanState {
    FieldStart,
    InUnquoted,
    InQuoted,
    QuotedEndOrEscape,
    InComment,
}

pub struct CsvParser {
    static fn isTrimSpace(c: u8, delimiter: u8): bool {
        if (c == ' ') return true;
        if (c == '\t' && delimiter != '\t') return true;
        return false;
    }

    static fn trimWhitespace(s: []const u8, delimiter: u8): []const u8 {
        let start: usize = 0;
        let end: usize = s.len;
        while (start < end && CsvParser.isTrimSpace(s[start], delimiter)) {
            start += 1;
        }
        while (end > start && CsvParser.isTrimSpace(s[end - 1], delimiter)) {
            end -= 1;
        }
        return s[start..end];
    }

    static fn isBlankOrComment(raw_line: []const u8, opts: CsvOptions): bool {
        let line = raw_line;
        if (line.len > 0 && line[line.len - 1] == '\r') {
            line = line[0 .. line.len - 1];
        }
        if (line.len == 0) {
            return true;
        }
        let i: usize = 0;
        while (i < line.len && CsvParser.isTrimSpace(line[i], opts.delimiter)) {
            i += 1;
        }
        if (i == line.len) {
            return true;
        }
        if (cc := opts.comment_char) {
            if (line[i] == cc) {
                return true;
            }
        }
        return false;
    }

    static fn dupeField(allocator: std.mem.Allocator, s: []const u8): ([]const u8)! {
        if (s.len == 0) {
            return "";
        }
        return allocator.dupe(u8, s);
    }

    pub static fn parseRecordLine(
        allocator: std.mem.Allocator,
        raw_line: []const u8,
        opts: CsvOptions,
        line_number: usize,
    ): CsvRow! {
        let line = raw_line;
        if (line.len > 0 && line[line.len - 1] == '\r') {
            line = line[0 .. line.len - 1];
        }

        let fields: std.ArrayList(string) = .empty;
        errdefer {
            for (f of fields.items) {
                if (f.len > 0) allocator.free(f);
            }
            fields.deinit(allocator);
        }

        let pos: usize = 0;
        let expect_field = true;

        while (expect_field) {
            expect_field = false;

            if (opts.trim_spaces) {
                while (pos < line.len && CsvParser.isTrimSpace(line[pos], opts.delimiter)) {
                    pos += 1;
                }
            }

            if (pos < line.len && line[pos] == opts.quote) {
                // Quoted field
                pos += 1;
                let fbuf: std.ArrayList(u8) = .empty;
                errdefer fbuf.deinit(allocator);

                let closed = false;
                while (pos < line.len) {
                    const ch = line[pos];
                    if (ch == opts.quote) {
                        if (pos + 1 < line.len && line[pos + 1] == opts.quote) {
                            fbuf.append(allocator, opts.quote)?;
                            pos += 2;
                        } else {
                            pos += 1;
                            closed = true;
                            break;
                        }
                    } else {
                        fbuf.append(allocator, ch)?;
                        pos += 1;
                    }
                }

                if (!closed) {
                    return err.UnterminatedQuote;
                }

                // Tolerate whitespace between closing quote and delimiter/EOL
                while (pos < line.len && CsvParser.isTrimSpace(line[pos], opts.delimiter)) {
                    pos += 1;
                }

                // Append any unexpected trailing chars up to delimiter if malformed
                while (pos < line.len && line[pos] != opts.delimiter) {
                    fbuf.append(allocator, line[pos])?;
                    pos += 1;
                }

                let owned: []const u8 = "";
                if (fbuf.items.len > 0) {
                    owned = fbuf.toOwnedSlice(allocator)?;
                } else {
                    fbuf.deinit(allocator);
                }
                fields.append(allocator, owned)?;
            } else {
                // Unquoted field
                const start = pos;
                while (pos < line.len && line[pos] != opts.delimiter) {
                    pos += 1;
                }
                let raw_field = line[start..pos];
                if (opts.trim_spaces) {
                    raw_field = CsvParser.trimWhitespace(raw_field, opts.delimiter);
                }
                const owned = CsvParser.dupeField(allocator, raw_field)?;
                fields.append(allocator, owned)?;
            }

            if (pos < line.len && line[pos] == opts.delimiter) {
                pos += 1;
                expect_field = true;
            }
        }

        const owned_fields = fields.toOwnedSlice(allocator)?;
        return CsvRow{
            allocator: allocator,
            fields: owned_fields,
            line_number: line_number,
        };
    }

    pub static fn parseAll(
        allocator: std.mem.Allocator,
        input: []const u8,
        opts: CsvOptions,
    ): ([]CsvRow)! {
        let reader = StreamingCsvReader.init(allocator, opts);
        defer reader.deinit();

        let all: std.ArrayList(CsvRow) = .empty;
        errdefer {
            for (*r of all.items) {
                r.deinit();
            }
            all.deinit(allocator);
        }

        const chunk_rows = reader.feed(input)?;
        all.appendSlice(allocator, chunk_rows) catch |e| {
            CsvRow.freeRows(allocator, chunk_rows);
            return e;
        };
        if (chunk_rows.len > 0) {
            allocator.free(chunk_rows);
        }

        const tail_rows = reader.finish()?;
        all.appendSlice(allocator, tail_rows) catch |e| {
            CsvRow.freeRows(allocator, tail_rows);
            return e;
        };
        if (tail_rows.len > 0) {
            allocator.free(tail_rows);
        }

        return all.toOwnedSlice(allocator);
    }
}

pub struct StreamingCsvReader {
    allocator: std.mem.Allocator = undefined,
    opts: CsvOptions = CsvOptions{},
    buf: std.ArrayList(u8) = .empty,
    state: CsvScanState = .FieldStart,
    bom_done: bool = false,
    bom_buf: [3]u8 = [0, 0, 0],
    bom_len: u8 = 0,
    expected_cols: usize? = null,
    line_number: usize = 0,
    record_count: usize = 0,

    pub static fn init(allocator: std.mem.Allocator, opts: CsvOptions): StreamingCsvReader {
        return StreamingCsvReader{
            allocator: allocator,
            opts: opts,
            buf: .empty,
            state: .FieldStart,
            bom_done: false,
            bom_buf: [0, 0, 0],
            bom_len: 0,
            expected_cols: null,
            line_number: 0,
            record_count: 0,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.allocator);
    }

    pub fn totalRecords(): usize {
        return _.record_count;
    }

    pub fn currentLine(): usize {
        return _.line_number;
    }

    pub fn bufferedBytes(): usize {
        return _.buf.items.len;
    }

    fn onlySpacesInBuf(): bool {
        for (c of _.buf.items) {
            if (!CsvParser.isTrimSpace(c, _.opts.delimiter)) {
                return false;
            }
        }
        return true;
    }

    fn flushBufferedRecord(*_, rows: *std.ArrayList(CsvRow)): ! {
        if (_.buf.items.len > 0 && _.buf.items[_.buf.items.len - 1] == '\r') {
            _.buf.items.len -= 1;
        }
        _.line_number += 1;

        if (!CsvParser.isBlankOrComment(_.buf.items, _.opts)) {
            let row = CsvParser.parseRecordLine(_.allocator, _.buf.items, _.opts, _.line_number)?;
            errdefer row.deinit();

            if (!_.opts.allow_flexible_columns) {
                if (exp := _.expected_cols) {
                    if (row.fields.len != exp) {
                        return err.FieldCountMismatch;
                    }
                } else {
                    _.expected_cols = row.fields.len;
                }
            }

            rows.append(_.allocator, row)?;
            _.record_count += 1;
        }

        _.buf.clearRetainingCapacity();
        _.state = .FieldStart;
    }

    fn pushByte(*_, b: u8, rows: *std.ArrayList(CsvRow)): ! {
        if (_.state == .InComment) {
            if (b == '\n') {
                _.line_number += 1;
                _.buf.clearRetainingCapacity();
                _.state = .FieldStart;
            }
            return;
        }

        if (_.state == .FieldStart) {
            if (cc := _.opts.comment_char) {
                if (b == cc && _.onlySpacesInBuf()) {
                    _.buf.clearRetainingCapacity();
                    _.state = .InComment;
                    return;
                }
            }

            if (b == '\n') {
                _.flushBufferedRecord(rows)?;
                return;
            }

            _.buf.append(_.allocator, b)?;
            if (_.buf.items.len > _.opts.max_record_bytes) {
                return err.RecordTooLarge;
            }

            if (b == _.opts.quote && _.onlySpacesBeforeLast()) {
                _.state = .InQuoted;
            } else if (b == _.opts.delimiter) {
                _.state = .FieldStart;
            } else if (_.opts.trim_spaces && CsvParser.isTrimSpace(b, _.opts.delimiter)) {
                _.state = .FieldStart;
            } else {
                _.state = .InUnquoted;
            }
            return;
        }

        if (_.state == .InUnquoted) {
            if (b == '\n') {
                _.flushBufferedRecord(rows)?;
                return;
            }
            _.buf.append(_.allocator, b)?;
            if (_.buf.items.len > _.opts.max_record_bytes) {
                return err.RecordTooLarge;
            }
            if (b == _.opts.delimiter) {
                _.state = .FieldStart;
            }
            return;
        }

        if (_.state == .InQuoted) {
            _.buf.append(_.allocator, b)?;
            if (_.buf.items.len > _.opts.max_record_bytes) {
                return err.RecordTooLarge;
            }
            if (b == _.opts.quote) {
                _.state = .QuotedEndOrEscape;
            }
            return;
        }

        // QuotedEndOrEscape
        if (b == '\n') {
            _.flushBufferedRecord(rows)?;
            return;
        }
        _.buf.append(_.allocator, b)?;
        if (_.buf.items.len > _.opts.max_record_bytes) {
            return err.RecordTooLarge;
        }
        if (b == _.opts.quote) {
            _.state = .InQuoted;
        } else if (b == _.opts.delimiter) {
            _.state = .FieldStart;
        } else if (!CsvParser.isTrimSpace(b, _.opts.delimiter) && b != '\r') {
            _.state = .InUnquoted;
        }
    }

    fn onlySpacesBeforeLast(): bool {
        if (_.buf.items.len <= 1) return true;
        // Walk backwards until start of current field (start of buf or delimiter)
        let idx = _.buf.items.len - 1;
        while (idx > 0) {
            idx -= 1;
            const c = _.buf.items[idx];
            if (c == _.opts.delimiter) {
                return true;
            }
            if (!CsvParser.isTrimSpace(c, _.opts.delimiter)) {
                return false;
            }
        }
        return true;
    }

    pub fn feed(*_, chunk: []const u8): ([]CsvRow)! {
        let rows: std.ArrayList(CsvRow) = .empty;
        errdefer {
            for (*r of rows.items) {
                r.deinit();
            }
            rows.deinit(_.allocator);
        }

        let pos: usize = 0;
        const bom_expected: [3]u8 = [0xEF, 0xBB, 0xBF];

        while (pos < chunk.len && !_.bom_done) {
            const b = chunk[pos];
            if (b == bom_expected[_.bom_len]) {
                _.bom_buf[_.bom_len] = b;
                _.bom_len += 1;
                pos += 1;
                if (_.bom_len == 3) {
                    _.bom_done = true;
                    _.bom_len = 0;
                }
            } else {
                _.bom_done = true;
                let bi: u8 = 0;
                while (bi < _.bom_len) : (bi += 1) {
                    _.pushByte(_.bom_buf[bi], &rows)?;
                }
                _.bom_len = 0;
            }
        }

        while (pos < chunk.len) : (pos += 1) {
            _.pushByte(chunk[pos], &rows)?;
        }

        return rows.toOwnedSlice(_.allocator);
    }

    pub fn finish(*_): ([]CsvRow)! {
        let rows: std.ArrayList(CsvRow) = .empty;
        errdefer {
            for (*r of rows.items) {
                r.deinit();
            }
            rows.deinit(_.allocator);
        }

        if (!_.bom_done) {
            _.bom_done = true;
            let bi: u8 = 0;
            while (bi < _.bom_len) : (bi += 1) {
                _.pushByte(_.bom_buf[bi], &rows)?;
            }
            _.bom_len = 0;
        }

        if (_.state == .InQuoted) {
            return err.UnterminatedQuote;
        }

        if (_.state == .InComment) {
            _.line_number += 1;
            _.buf.clearRetainingCapacity();
            _.state = .FieldStart;
            return rows.toOwnedSlice(_.allocator);
        }

        if (_.buf.items.len > 0) {
            _.flushBufferedRecord(&rows)?;
        }

        return rows.toOwnedSlice(_.allocator);
    }
}
