// Source: cp/lib/csv/writer.cp (RFC 4180 CSV & TSV Writer)
// RFC 4180 compliant CSV & TSV serializer with automatic minimal quoting,
// embedded quote escaping, configurable CRLF/LF line endings & formula injection defense.

const std = @import("std");
import { CsvRow } from "./parser.cp";

pub struct CsvWriteOptions {
    delimiter: u8 = ',',
    quote: u8 = '"',
    use_crlf: bool = true,
    always_quote: bool = false,
    escape_formula: bool = false,

    pub static fn defaults(): CsvWriteOptions {
        return CsvWriteOptions{};
    }

    pub static fn tsv(): CsvWriteOptions {
        return CsvWriteOptions{
            delimiter: '\t',
            quote: '"',
            use_crlf: false,
            always_quote: false,
            escape_formula: false,
        };
    }
}

pub struct CsvWriter {
    allocator: std.mem.Allocator = undefined,
    buf: std.ArrayList(u8) = .empty,
    opts: CsvWriteOptions = CsvWriteOptions{},
    row_count: usize = 0,

    pub static fn init(allocator: std.mem.Allocator, opts: CsvWriteOptions): CsvWriter {
        return CsvWriter{
            allocator: allocator,
            buf: .empty,
            opts: opts,
            row_count: 0,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.allocator);
    }

    pub fn count(): usize {
        return _.row_count;
    }

    pub fn slice(): []const u8 {
        return _.buf.items;
    }

    pub fn clear(*_) {
        _.buf.clearRetainingCapacity();
        _.row_count = 0;
    }

    pub fn toOwnedSlice(*_): ([]u8)! {
        _.row_count = 0;
        return _.buf.toOwnedSlice(_.allocator);
    }

    static fn isFormulaTrigger(c: u8): bool {
        return c == '=' || c == '+' || c == '-' || c == '@';
    }

    fn fieldNeedsQuote(field: []const u8): bool {
        if (_.opts.always_quote) {
            return true;
        }
        for (c of field) {
            if (c == _.opts.delimiter || c == _.opts.quote || c == '\n' || c == '\r') {
                return true;
            }
        }
        return false;
    }

    pub fn writeField(*_, field: []const u8): ! {
        const prefix_formula = _.opts.escape_formula && field.len > 0 && CsvWriter.isFormulaTrigger(field[0]);
        const quote_it = _.fieldNeedsQuote(field);

        if (!quote_it) {
            if (prefix_formula) {
                _.buf.append(_.allocator, '\'')?;
            }
            _.buf.appendSlice(_.allocator, field)?;
            return;
        }

        const q = _.opts.quote;
        _.buf.append(_.allocator, q)?;
        if (prefix_formula) {
            _.buf.append(_.allocator, '\'')?;
        }

        let i: usize = 0;
        let flush_start: usize = 0;
        while (i < field.len) : (i += 1) {
            if (field[i] == q) {
                if (i > flush_start) {
                    _.buf.appendSlice(_.allocator, field[flush_start..i])?;
                }
                _.buf.append(_.allocator, q)?;
                _.buf.append(_.allocator, q)?;
                flush_start = i + 1;
            }
        }
        if (i > flush_start) {
            _.buf.appendSlice(_.allocator, field[flush_start..i])?;
        }
        _.buf.append(_.allocator, q)?;
    }

    pub fn writeRow(*_, fields: []const []const u8): ! {
        let i: usize = 0;
        while (i < fields.len) : (i += 1) {
            if (i > 0) {
                _.buf.append(_.allocator, _.opts.delimiter)?;
            }
            _.writeField(fields[i])?;
        }
        if (_.opts.use_crlf) {
            _.buf.appendSlice(_.allocator, "\r\n")?;
        } else {
            _.buf.append(_.allocator, '\n')?;
        }
        _.row_count += 1;
    }

    pub fn writeCsvRow(*_, row: *const CsvRow): ! {
        _.writeRow(row.fields)?;
    }

    pub static fn stringifyRows(
        allocator: std.mem.Allocator,
        rows: []const CsvRow,
        opts: CsvWriteOptions,
    ): ([]u8)! {
        let w = CsvWriter.init(allocator, opts);
        errdefer w.deinit();
        for (r of rows) {
            w.writeRow(r.fields)?;
        }
        return w.toOwnedSlice();
    }
}
