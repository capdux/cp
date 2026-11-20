// Source: cp/cap/test/test_csv_production_deep.cp
// Deep Production Test Suite for cap:csv
// Covers:
//   1. RFC 4180 embedded newlines/commas, "" quote escaping, UTF-8 BOM, comments & FieldCountMismatch
//   2. Incremental cross-chunk StreamingCsvReader, RecordTooLarge & UnterminatedQuote
//   3. TSV preset, CsvWriter round-trip & formula injection protection (escape_formula)
//   4. CsvTable header-indexed queries, row filtering & NDJSON toJsonLines export
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.csv`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CsvParseError,
    CsvOptions,
    CsvRow,
    CsvParser,
    StreamingCsvReader,
    CsvWriteOptions,
    CsvWriter,
    CsvTable,
} from "cap:csv";
import cap from "cap";

fn isScoreHigh(row: *const CsvRow, headers: []const []const u8): bool {
    * = headers;
    const score = row.get(2) ?? "";
    return std.mem.eql(u8, score, "98") || std.mem.eql(u8, score, "95");
}

fn testRfc4180AndBom()! {
    const alloc = std.heap.page_allocator;

    const csv_data =
        "\xEF\xBB\xBF# header comment\r\n" ++
        "\r\n" ++
        "id,name,bio,tags\r\n" ++
        "1,\"Doe, John\",\"Line 1\r\nLine 2\nLine 3\",\"\"\"alpha\"\",beta\"\r\n" ++
        "2,Alice,,\r\n" ++
        "# trailing comment\n";

    let opts = CsvOptions.defaults();
    opts.comment_char = '#';

    const rows = CsvParser.parseAll(alloc, csv_data, opts) catch unreachable;
    defer CsvRow.freeRows(alloc, rows);

    expect_eq(rows.len, 3 as usize);
    expect_eq(std.mem.eql(u8, rows[0].get(0).?, "id"), true);
    expect_eq(std.mem.eql(u8, rows[0].get(3).?, "tags"), true);

    expect_eq(std.mem.eql(u8, rows[1].get(0).?, "1"), true);
    expect_eq(std.mem.eql(u8, rows[1].get(1).?, "Doe, John"), true);
    expect_eq(std.mem.eql(u8, rows[1].get(2).?, "Line 1\r\nLine 2\nLine 3"), true);
    expect_eq(std.mem.eql(u8, rows[1].get(3).?, "\"alpha\",beta"), true);

    expect_eq(rows[2].len(), 4 as usize);
    expect_eq(std.mem.eql(u8, rows[2].get(1).?, "Alice"), true);
    expect_eq(std.mem.eql(u8, rows[2].get(2).?, ""), true);
    expect_eq(std.mem.eql(u8, rows[2].get(3).?, ""), true);
    expect_eq(rows[2].get(4) == null, true);

    // Trim spaces mode
    let trim_opts = CsvOptions.defaults();
    trim_opts.trim_spaces = true;
    const trimmed_rows = CsvParser.parseAll(alloc, "  a ,  \" b \" , c  \n", trim_opts) catch unreachable;
    defer CsvRow.freeRows(alloc, trimmed_rows);
    expect_eq(trimmed_rows.len, 1 as usize);
    expect_eq(std.mem.eql(u8, trimmed_rows[0].get(0).?, "a"), true);
    expect_eq(std.mem.eql(u8, trimmed_rows[0].get(1).?, " b "), true);
    expect_eq(std.mem.eql(u8, trimmed_rows[0].get(2).?, "c"), true);

    // FieldCountMismatch strict check vs allow_flexible_columns
    let mismatch_hit = false;
    * = CsvParser.parseAll(alloc, "a,b,c\n1,2\n", CsvOptions.defaults()) catch |e| {
        if (e == @as(anyerror, err.FieldCountMismatch)) {
            mismatch_hit = true;
        }
    };
    expect_eq(mismatch_hit, true);

    let flex_opts = CsvOptions.defaults();
    flex_opts.allow_flexible_columns = true;
    const flex_rows = CsvParser.parseAll(alloc, "a,b,c\n1,2\n", flex_opts) catch unreachable;
    defer CsvRow.freeRows(alloc, flex_rows);
    expect_eq(flex_rows.len, 2 as usize);
    expect_eq(flex_rows[1].len(), 2 as usize);
}

fn testStreamingAndErrors()! {
    const alloc = std.heap.page_allocator;

    let reader = StreamingCsvReader.init(alloc, CsvOptions.defaults());
    defer reader.deinit();

    // Split UTF-8 BOM, quotes, "" escape, and CRLF across tiny arbitrary chunks
    const c1 = reader.feed("\xEF\xBB") catch unreachable;
    defer CsvRow.freeRows(alloc, c1);
    expect_eq(c1.len, 0 as usize);

    const c2 = reader.feed("\xBFk,v\r") catch unreachable;
    defer CsvRow.freeRows(alloc, c2);
    expect_eq(c2.len, 0 as usize);

    const c3 = reader.feed("\n\"a\"\"b\",\"hello\n") catch unreachable;
    defer CsvRow.freeRows(alloc, c3);
    expect_eq(c3.len, 1 as usize);
    expect_eq(std.mem.eql(u8, c3[0].get(0).?, "k"), true);
    expect_eq(std.mem.eql(u8, c3[0].get(1).?, "v"), true);

    const c4 = reader.feed("world\"\r\nlast,") catch unreachable;
    defer CsvRow.freeRows(alloc, c4);
    expect_eq(c4.len, 1 as usize);
    expect_eq(std.mem.eql(u8, c4[0].get(0).?, "a\"b"), true);
    expect_eq(std.mem.eql(u8, c4[0].get(1).?, "hello\nworld"), true);
    expect_eq(reader.bufferedBytes() > 0, true);

    const c5 = reader.feed("tail") catch unreachable;
    defer CsvRow.freeRows(alloc, c5);
    expect_eq(c5.len, 0 as usize);

    const c6 = reader.finish() catch unreachable;
    defer CsvRow.freeRows(alloc, c6);
    expect_eq(c6.len, 1 as usize);
    expect_eq(std.mem.eql(u8, c6[0].get(0).?, "last"), true);
    expect_eq(std.mem.eql(u8, c6[0].get(1).?, "tail"), true);
    expect_eq(reader.totalRecords(), 3 as usize);

    // RecordTooLarge guard
    let small_opts = CsvOptions.defaults();
    small_opts.max_record_bytes = 16;
    let large_err = false;
    * = CsvParser.parseAll(alloc, "a,b\n12345678901234567890,ok\n", small_opts) catch |e| {
        if (e == @as(anyerror, err.RecordTooLarge)) {
            large_err = true;
        }
    };
    expect_eq(large_err, true);

    // UnterminatedQuote guard
    let unterm_err = false;
    * = CsvParser.parseAll(alloc, "a,b\n1,\"unclosed\nstill open", CsvOptions.defaults()) catch |e| {
        if (e == @as(anyerror, err.UnterminatedQuote)) {
            unterm_err = true;
        }
    };
    expect_eq(unterm_err, true);
}

fn testTsvWriterAndFormulaProtection()! {
    const alloc = std.heap.page_allocator;

    // TSV round-trip
    let tw = CsvWriter.init(alloc, CsvWriteOptions.tsv());
    defer tw.deinit();

    let th: [3][]const u8 = ["key", "val", "note"];
    let tr1: [3][]const u8 = ["k1", "tab\there", "quote\"here"];
    tw.writeRow(&th) catch unreachable;
    tw.writeRow(&tr1) catch unreachable;
    const tsv_out = tw.toOwnedSlice() catch unreachable;
    defer alloc.free(tsv_out);

    const tsv_rows = CsvParser.parseAll(alloc, tsv_out, CsvOptions.tsv()) catch unreachable;
    defer CsvRow.freeRows(alloc, tsv_rows);
    expect_eq(tsv_rows.len, 2 as usize);
    expect_eq(std.mem.eql(u8, tsv_rows[1].get(0).?, "k1"), true);
    expect_eq(std.mem.eql(u8, tsv_rows[1].get(1).?, "tab\there"), true);
    expect_eq(std.mem.eql(u8, tsv_rows[1].get(2).?, "quote\"here"), true);

    // CSV round-trip + formula injection protection
    let w_opts = CsvWriteOptions.defaults();
    w_opts.escape_formula = true;
    let w = CsvWriter.init(alloc, w_opts);
    defer w.deinit();

    let h: [4][]const u8 = ["f1", "f2", "f3", "f4"];
    let f_row: [4][]const u8 = ["=CMD()", "+SUM(A1)", "-1+1", "@IMPORT"];
    let q_row: [4][]const u8 = ["=1,2", "a\"b", "x\ny", "safe"];
    w.writeRow(&h) catch unreachable;
    w.writeRow(&f_row) catch unreachable;
    w.writeRow(&q_row) catch unreachable;

    const csv_out = w.toOwnedSlice() catch unreachable;
    defer alloc.free(csv_out);

    const reparsed = CsvParser.parseAll(alloc, csv_out, CsvOptions.defaults()) catch unreachable;
    defer CsvRow.freeRows(alloc, reparsed);
    expect_eq(reparsed.len, 3 as usize);
    expect_eq(std.mem.eql(u8, reparsed[1].get(0).?, "'=CMD()"), true);
    expect_eq(std.mem.eql(u8, reparsed[1].get(1).?, "'+SUM(A1)"), true);
    expect_eq(std.mem.eql(u8, reparsed[1].get(2).?, "'-1+1"), true);
    expect_eq(std.mem.eql(u8, reparsed[1].get(3).?, "'@IMPORT"), true);
    expect_eq(std.mem.eql(u8, reparsed[2].get(0).?, "'=1,2"), true);
    expect_eq(std.mem.eql(u8, reparsed[2].get(1).?, "a\"b"), true);
    expect_eq(std.mem.eql(u8, reparsed[2].get(2).?, "x\ny"), true);
    expect_eq(std.mem.eql(u8, reparsed[2].get(3).?, "safe"), true);
}

fn testCsvTableAndNdjson()! {
    const alloc = std.heap.page_allocator;

    const raw =
        "id,role,score,comment\r\n" ++
        "u1,admin,98,\"hello \"\"world\"\"\"\r\n" ++
        "u2,user,75,\"line1\nline2\"\r\n" ++
        "u3,admin,95,ok\r\n";

    let table = CsvTable.parse(alloc, raw, CsvOptions.defaults()) catch unreachable;
    defer table.deinit();

    expect_eq(table.rowCount(), 3 as usize);
    expect_eq(table.colCount(), 4 as usize);
    expect_eq(table.columnIndex("role").?, 1 as usize);
    expect_eq(table.columnIndex("missing") == null, true);

    expect_eq(std.mem.eql(u8, table.getCell(0, "comment").?, "hello \"world\""), true);
    expect_eq(std.mem.eql(u8, table.getCell(1, "role").?, "user"), true);
    expect_eq(std.mem.eql(u8, table.getCellAt(2, 2).?, "95"), true);
    expect_eq(table.getCell(5, "role") == null, true);

    const admin_rows = table.filterRows(alloc, "role", "admin") catch unreachable;
    defer { if (admin_rows.len > 0) alloc.free(admin_rows); }
    expect_eq(admin_rows.len, 2 as usize);
    expect_eq(admin_rows[0], 0 as usize);
    expect_eq(admin_rows[1], 2 as usize);

    const high_rows = table.filterRowsWhere(alloc, isScoreHigh) catch unreachable;
    defer { if (high_rows.len > 0) alloc.free(high_rows); }
    expect_eq(high_rows.len, 2 as usize);
    expect_eq(high_rows[0], 0 as usize);
    expect_eq(high_rows[1], 2 as usize);

    const ndjson = table.toJsonLines(alloc) catch unreachable;
    defer alloc.free(ndjson);
    const expected_ndjson =
        "{\"id\":\"u1\",\"role\":\"admin\",\"score\":\"98\",\"comment\":\"hello \\\"world\\\"\"}\n" ++
        "{\"id\":\"u2\",\"role\":\"user\",\"score\":\"75\",\"comment\":\"line1\\nline2\"}\n" ++
        "{\"id\":\"u3\",\"role\":\"admin\",\"score\":\"95\",\"comment\":\"ok\"}\n";
    expect_eq(std.mem.eql(u8, ndjson, expected_ndjson), true);

    const rt_csv = table.toCsv(alloc, CsvWriteOptions.defaults()) catch unreachable;
    defer alloc.free(rt_csv);
    expect_eq(rt_csv.len > 0, true);
}

fn testCapCsvLazyMount()! {
    const alloc = std.heap.page_allocator;

    const input = "model,tokens\nclaude,1024\ngemini,2048\n";
    let tbl = cap.csv.CsvTable.parse(alloc, input, cap.csv.CsvOptions.defaults()) catch unreachable;
    defer tbl.deinit();

    expect_eq(tbl.rowCount(), 2 as usize);
    expect_eq(std.mem.eql(u8, tbl.getCell(1, "model").?, "gemini"), true);
    expect_eq(std.mem.eql(u8, tbl.getCell(1, "tokens").?, "2048"), true);

    let w = cap.csv.CsvWriter.init(alloc, cap.csv.CsvWriteOptions.tsv());
    defer w.deinit();
    let r: [2][]const u8 = ["a", "b"];
    w.writeRow(&r) catch unreachable;
    const out = w.toOwnedSlice() catch unreachable;
    defer alloc.free(out);
    expect_eq(std.mem.eql(u8, out, "a\tb\n"), true);
}

fn main() {
    test("1. RFC 4180 Quotes, Multiline Fields, Escapes, UTF-8 BOM & Column Consistency", () => {
        testRfc4180AndBom()?;
    });

    test("2. StreamingCsvReader Cross-Chunk Parsing, RecordTooLarge & UnterminatedQuote", () => {
        testStreamingAndErrors()?;
    });

    test("3. TSV Preset, CsvWriter Round-Trip & Formula Injection Protection", () => {
        testTsvWriterAndFormulaProtection()?;
    });

    test("4. CsvTable Header-Indexed Query, Filtering & NDJSON toJsonLines Export", () => {
        testCsvTableAndNdjson()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.csv`", () => {
        testCapCsvLazyMount()?;
    });
}
