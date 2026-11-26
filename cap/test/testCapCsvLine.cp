// cap/test/testCapCsvLine.cp
// Comprehensive unit tests for @cap/csv-line (RFC 4180 CSV/TSV parity)

import { test, expect_eq } from "cap:test";
import {
    CsvFieldSpan,
    CsvParseResult,
    parseRecord,
    formatField,
    formatFieldSafe,
    formatRecord,
} from "../../packages/@cap/csv-line/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/csv-line — basic comma-separated fields including empty fields 'a,,c'", () => {
        let out_buf: [128]u8 = undefined;
        let spans: [8]CsvFieldSpan = undefined;
        const r: CsvParseResult = parseRecord("a,,c,", 0, ',' as u8, out_buf[0..128], spans[0..8]);

        expect_eq(r.ok, true);
        expect_eq(r.field_count, 4 as usize);
        expect_eq(sliceEq(out_buf[spans[0].start..(spans[0].start + spans[0].len)], "a"), true);
        expect_eq(spans[1].len, 0 as usize);
        expect_eq(sliceEq(out_buf[spans[2].start..(spans[2].start + spans[2].len)], "c"), true);
        expect_eq(spans[3].len, 0 as usize);
    });

    test("2. @cap/csv-line — quoted fields with embedded commas & escaped \"\" quotes", () => {
        const line = "id,\"hello, \"\"world\"\"\",ok";
        let out_buf: [128]u8 = undefined;
        let spans: [8]CsvFieldSpan = undefined;
        const r: CsvParseResult = parseRecord(line, 0, ',' as u8, out_buf[0..128], spans[0..8]);

        expect_eq(r.ok, true);
        expect_eq(r.field_count, 3 as usize);
        expect_eq(sliceEq(out_buf[spans[0].start..(spans[0].start + spans[0].len)], "id"), true);
        expect_eq(
            sliceEq(out_buf[spans[1].start..(spans[1].start + spans[1].len)], "hello, \"world\""),
            true,
        );
        expect_eq(sliceEq(out_buf[spans[2].start..(spans[2].start + spans[2].len)], "ok"), true);
    });

    test("3. @cap/csv-line — quoted embedded newline & multi-record next_pos iteration", () => {
        const doc = "1,\"line1\nline2\"\r\n2,plain\n";
        let out_buf: [128]u8 = undefined;
        let spans: [8]CsvFieldSpan = undefined;

        const r1: CsvParseResult = parseRecord(doc, 0, ',' as u8, out_buf[0..128], spans[0..8]);
        expect_eq(r1.ok, true);
        expect_eq(r1.field_count, 2 as usize);
        expect_eq(sliceEq(out_buf[spans[0].start..(spans[0].start + spans[0].len)], "1"), true);
        expect_eq(
            sliceEq(out_buf[spans[1].start..(spans[1].start + spans[1].len)], "line1\nline2"),
            true,
        );

        const r2: CsvParseResult = parseRecord(doc, r1.next_pos, ',' as u8, out_buf[0..128], spans[0..8]);
        expect_eq(r2.ok, true);
        expect_eq(r2.field_count, 2 as usize);
        expect_eq(sliceEq(out_buf[spans[0].start..(spans[0].start + spans[0].len)], "2"), true);
        expect_eq(sliceEq(out_buf[spans[1].start..(spans[1].start + spans[1].len)], "plain"), true);
        expect_eq(r2.next_pos, doc.len);
    });

    test("4. @cap/csv-line — custom tab delimiter (TSV)", () => {
        const tsv = "name\tage\t\"note\t1\"\n";
        let out_buf: [128]u8 = undefined;
        let spans: [8]CsvFieldSpan = undefined;
        const r: CsvParseResult = parseRecord(tsv, 0, '\t' as u8, out_buf[0..128], spans[0..8]);

        expect_eq(r.ok, true);
        expect_eq(r.field_count, 3 as usize);
        expect_eq(sliceEq(out_buf[spans[0].start..(spans[0].start + spans[0].len)], "name"), true);
        expect_eq(sliceEq(out_buf[spans[1].start..(spans[1].start + spans[1].len)], "age"), true);
        expect_eq(sliceEq(out_buf[spans[2].start..(spans[2].start + spans[2].len)], "note\t1"), true);
    });

    test("5. @cap/csv-line — formatRecord -> parseRecord round-trip & formula guard", () => {
        const fields: [3][]const u8 = ["plain", "a, \"b\"\nc", "42"];
        let encoded: [128]u8 = undefined;
        const n: i64 = formatRecord(fields[0..3], ',' as u8, encoded[0..128]);
        expect_eq(n > 0, true);

        let out_buf: [128]u8 = undefined;
        let spans: [8]CsvFieldSpan = undefined;
        const r: CsvParseResult = parseRecord(encoded[0..(n as usize)], 0, ',' as u8, out_buf[0..128], spans[0..8]);
        expect_eq(r.ok, true);
        expect_eq(r.field_count, 3 as usize);
        expect_eq(sliceEq(out_buf[spans[0].start..(spans[0].start + spans[0].len)], "plain"), true);
        expect_eq(sliceEq(out_buf[spans[1].start..(spans[1].start + spans[1].len)], "a, \"b\"\nc"), true);
        expect_eq(sliceEq(out_buf[spans[2].start..(spans[2].start + spans[2].len)], "42"), true);

        // Formula guard prefixes leading '=' with single quote
        let safe_dst: [32]u8 = undefined;
        const sn: i64 = formatFieldSafe("=SUM(A1:A2)", ',' as u8, safe_dst[0..32]);
        expect_eq(sliceEq(safe_dst[0..(sn as usize)], "'=SUM(A1:A2)"), true);
    });

    test("6. @cap/csv-line — unclosed quotes & tiny buffers fail cleanly", () => {
        let out_buf: [64]u8 = undefined;
        let spans: [8]CsvFieldSpan = undefined;
        // Unclosed quote
        expect_eq(parseRecord("a,\"unclosed", 0, ',' as u8, out_buf[0..64], spans[0..8]).ok, false);
        // Too few span slots
        let one_span: [1]CsvFieldSpan = undefined;
        expect_eq(parseRecord("a,b", 0, ',' as u8, out_buf[0..64], one_span[0..1]).ok, false);
        // Output buffer too small
        let tiny: [3]u8 = undefined;
        expect_eq(parseRecord("abcdef", 0, ',' as u8, tiny[0..3], spans[0..8]).ok, false);
        expect_eq(formatField("abcdef", ',' as u8, tiny[0..3]), -1 as i64);
        const f: [2][]const u8 = ["abc", "def"];
        expect_eq(formatRecord(f[0..2], ',' as u8, tiny[0..3]), -1 as i64);
    });
}
