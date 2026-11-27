// Source: cp/lib/csv/table.cp (Header-Indexed CsvTable Query & JSON Export)
// In-memory tabular CSV view with O(N) header-indexed cell lookup,
// row filtering & compact NDJSON (JSON Lines) export for AI data pipelines.

const std = @import("std");
import { CsvOptions, CsvRow, CsvParser } from "./parser.cp";
import { CsvWriteOptions, CsvWriter } from "./writer.cp";

type CsvTableVec16u8 = @Vector(16, u8);

fn simdFindJsonEscapeByte(s: []const u8, start: usize): usize {
    let i: usize = start;
    const n = s.len;
    const v_quote: CsvTableVec16u8 = @splat('"');
    const v_bs: CsvTableVec16u8 = @splat('\\');
    const v_ctrl: CsvTableVec16u8 = @splat(0x20);
    while (i + 16 <= n) {
        const chunk: CsvTableVec16u8 = s[i..][0..16].*;
        const m_q: u16 = @bitCast(chunk == v_quote);
        const m_b: u16 = @bitCast(chunk == v_bs);
        const m_c: u16 = @bitCast(chunk < v_ctrl);
        const mask: u16 = m_q | m_b | m_c;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const c = s[i];
        if (c == '"' || c == '\\' || c < 0x20) return i;
    }
    return n;
}

pub struct CsvTable {
    allocator: std.mem.Allocator = undefined,
    headers: [][]const u8 = [],
    rows: []CsvRow = [],

    pub static fn parse(
        allocator: std.mem.Allocator,
        input: []const u8,
        opts: CsvOptions,
    ): CsvTable! {
        const parsed = CsvParser.parseAll(allocator, input, opts)?;
        errdefer CsvRow.freeRows(allocator, parsed);

        if (parsed.len == 0) {
            return CsvTable{
                allocator: allocator,
                headers: [],
                rows: [],
            };
        }

        if (!opts.has_header) {
            return CsvTable{
                allocator: allocator,
                headers: [],
                rows: parsed,
            };
        }

        const headers = parsed[0].fields;
        const data_len = parsed.len - 1;
        if (data_len == 0) {
            allocator.free(parsed);
            return CsvTable{
                allocator: allocator,
                headers: headers,
                rows: [],
            };
        }

        let data_rows = allocator.alloc(CsvRow, data_len)?;
        let i: usize = 0;
        while (i < data_len) : (i += 1) {
            data_rows[i] = parsed[i + 1];
        }
        allocator.free(parsed);

        return CsvTable{
            allocator: allocator,
            headers: headers,
            rows: data_rows,
        };
    }

    pub fn deinit(*_) {
        for (h of _.headers) {
            if (h.len > 0) {
                _.allocator.free(h);
            }
        }
        if (_.headers.len > 0) {
            _.allocator.free(_.headers);
        }
        for (*r of _.rows) {
            r.deinit();
        }
        if (_.rows.len > 0) {
            _.allocator.free(_.rows);
        }
        _.headers = [];
        _.rows = [];
    }

    pub fn rowCount(): usize {
        return _.rows.len;
    }

    pub fn colCount(): usize {
        if (_.headers.len > 0) {
            return _.headers.len;
        }
        if (_.rows.len > 0) {
            return _.rows[0].fields.len;
        }
        return 0;
    }

    pub fn columnIndex(name: []const u8): usize? {
        let i: usize = 0;
        while (i < _.headers.len) : (i += 1) {
            if (std.mem.eql(u8, _.headers[i], name)) {
                return i;
            }
        }
        return null;
    }

    pub fn getCell(row: usize, col_name: []const u8): ([]const u8)? {
        if (row >= _.rows.len) {
            return null;
        }
        const col = _.columnIndex(col_name) ?? return null;
        return _.rows[row].get(col);
    }

    pub fn getCellAt(row: usize, col: usize): ([]const u8)? {
        if (row >= _.rows.len) {
            return null;
        }
        return _.rows[row].get(col);
    }

    pub fn getRow(row: usize): ([]const []const u8)? {
        if (row >= _.rows.len) {
            return null;
        }
        return _.rows[row].fields;
    }

    pub fn filterRows(
        allocator: std.mem.Allocator,
        col_name: []const u8,
        expected: []const u8,
    ): ([]usize)! {
        let matched: std.ArrayList(usize) = .empty;
        errdefer matched.deinit(allocator);

        const col_opt = _.columnIndex(col_name);
        if (col := col_opt) {
            let r: usize = 0;
            while (r < _.rows.len) : (r += 1) {
                if (val := _.rows[r].get(col)) {
                    if (std.mem.eql(u8, val, expected)) {
                        matched.append(allocator, r)?;
                    }
                }
            }
        }

        return matched.toOwnedSlice(allocator);
    }

    pub fn filterRowsWhere(
        allocator: std.mem.Allocator,
        pred: fn (row: *const CsvRow, headers: []const []const u8) bool,
    ): ([]usize)! {
        let matched: std.ArrayList(usize) = .empty;
        errdefer matched.deinit(allocator);

        let r: usize = 0;
        while (r < _.rows.len) : (r += 1) {
            if (pred(&_.rows[r], _.headers)) {
                matched.append(allocator, r)?;
            }
        }

        return matched.toOwnedSlice(allocator);
    }

    static fn appendJsonString(
        out: *std.ArrayList(u8),
        allocator: std.mem.Allocator,
        s: []const u8,
    ): ! {
        out.append(allocator, '"')?;
        let i: usize = 0;
        while (i < s.len) {
            const hit = simdFindJsonEscapeByte(s, i);
            if (hit > i) {
                out.appendSlice(allocator, s[i..hit])?;
                i = hit;
            }
            if (i >= s.len) break;
            const c = s[i];
            switch (c) {
                '"' => out.appendSlice(allocator, "\\\"")?,
                '\\' => out.appendSlice(allocator, "\\\\")?,
                0x08 => out.appendSlice(allocator, "\\b")?,
                0x0C => out.appendSlice(allocator, "\\f")?,
                '\n' => out.appendSlice(allocator, "\\n")?,
                '\r' => out.appendSlice(allocator, "\\r")?,
                '\t' => out.appendSlice(allocator, "\\t")?,
                else => {
                    const hex_digits = "0123456789abcdef";
                    let seq: [6]u8 = ['\\', 'u', '0', '0', hex_digits[(c >> 4) & 0xF], hex_digits[c & 0xF]];
                    out.appendSlice(allocator, seq[0..6])?;
                },
            }
            i += 1;
        }
        out.append(allocator, '"')?;
    }

    pub fn toJsonLines(allocator: std.mem.Allocator): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

        let key_buf: [32]u8 = undefined;
        for (row of _.rows) {
            out.append(allocator, '{')?;
            let c: usize = 0;
            while (c < row.fields.len) : (c += 1) {
                if (c > 0) {
                    out.append(allocator, ',')?;
                }
                if (c < _.headers.len) {
                    CsvTable.appendJsonString(&out, allocator, _.headers[c])?;
                } else {
                    const auto_key = std.fmt.bufPrint(&key_buf, "col_{d}", .{c}) catch "col";
                    CsvTable.appendJsonString(&out, allocator, auto_key)?;
                }
                out.append(allocator, ':')?;
                CsvTable.appendJsonString(&out, allocator, row.fields[c])?;
            }
            out.appendSlice(allocator, "}\n")?;
        }

        return out.toOwnedSlice(allocator);
    }

    pub fn toCsv(allocator: std.mem.Allocator, opts: CsvWriteOptions): ([]u8)! {
        let w = CsvWriter.init(allocator, opts);
        errdefer w.deinit();

        if (_.headers.len > 0) {
            w.writeRow(_.headers)?;
        }
        for (row of _.rows) {
            w.writeRow(row.fields)?;
        }
        return w.toOwnedSlice();
    }
}
