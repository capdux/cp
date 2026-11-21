import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CsvReader,
    unescapeField,
    needsCsvQuoting,
    formatCsvField,
    formatCsvRow,
} from "cap:csv_stream";

test("1. multi-row CSV with embedded commas, newlines, and escaped double-quotes", () => {
    const src =
        "id,name,bio\r\n"
        ++ "1,\"Alice, A.\",\"line1\nline2\"\r\n"
        ++ "2,\"Bob \"\"The Builder\"\"\",plain\n";

    let reader = CsvReader.init(src, ',');
    let tmp: [64]u8 = std.mem.zeroes([64]u8);

    const r0 = reader.nextRow() ?? unreachable;
    expect_eq(r0.len, usize(3));
    expect_eq(std.mem.eql(u8, r0.fields[0].raw, "id"), true);
    expect_eq(std.mem.eql(u8, r0.fields[2].raw, "bio"), true);

    const r1 = reader.nextRow() ?? unreachable;
    expect_eq(r1.len, usize(3));
    expect_eq(r1.fields[1].was_quoted, true);
    expect_eq(r1.fields[1].has_escaped_quotes, false);
    expect_eq(std.mem.eql(u8, r1.fields[1].raw, "Alice, A."), true);
    expect_eq(std.mem.eql(u8, r1.fields[2].raw, "line1\nline2"), true);

    const r2 = reader.nextRow() ?? unreachable;
    expect_eq(r2.len, usize(3));
    expect_eq(r2.fields[1].was_quoted, true);
    expect_eq(r2.fields[1].has_escaped_quotes, true);
    const unesc = CsvReader.unescapeField(r2.fields[1], &tmp) ?? unreachable;
    expect_eq(std.mem.eql(u8, unesc, "Bob \"The Builder\""), true);
    // 未转义字段零拷贝返回
    const plain = unescapeField(r2.fields[2], &tmp) ?? unreachable;
    expect_eq(std.mem.eql(u8, plain, "plain"), true);

    expect_eq(reader.nextRow() == null, true);
    expect_eq(reader.had_error, false);
});

test("2. empty fields (a,,c), trailing delimiter, and mixed CRLF/LF", () => {
    const src = "a,,c\r\n,mid,\nx,y";
    let reader = CsvReader.init(src, ',');

    const r0 = reader.nextRow() ?? unreachable;
    expect_eq(r0.len, usize(3));
    expect_eq(std.mem.eql(u8, r0.fields[0].raw, "a"), true);
    expect_eq(std.mem.eql(u8, r0.fields[1].raw, ""), true);
    expect_eq(std.mem.eql(u8, r0.fields[2].raw, "c"), true);

    const r1 = reader.nextRow() ?? unreachable;
    expect_eq(r1.len, usize(3));
    expect_eq(std.mem.eql(u8, r1.fields[0].raw, ""), true);
    expect_eq(std.mem.eql(u8, r1.fields[1].raw, "mid"), true);
    expect_eq(std.mem.eql(u8, r1.fields[2].raw, ""), true);

    const r2 = reader.nextRow() ?? unreachable;
    expect_eq(r2.len, usize(2));
    expect_eq(std.mem.eql(u8, r2.fields[0].raw, "x"), true);
    expect_eq(std.mem.eql(u8, r2.fields[1].raw, "y"), true);

    expect_eq(reader.nextRow() == null, true);
});

test("3. TSV tab-delimited and semicolon-delimited streaming", () => {
    const tsv = "col1\tcol2\tcol3\n10\t\"has\ttab\"\t30";
    let reader = CsvReader.init(tsv, '\t');

    const h = reader.nextRow() ?? unreachable;
    expect_eq(h.len, usize(3));
    expect_eq(std.mem.eql(u8, h.fields[1].raw, "col2"), true);

    const d = reader.nextRow() ?? unreachable;
    expect_eq(d.len, usize(3));
    expect_eq(std.mem.eql(u8, d.fields[1].raw, "has\ttab"), true);
    expect_eq(d.fields[1].was_quoted, true);

    const semi = "k;v\n\"a;b\";42";
    let sr = CsvReader.init(semi, ';');
    * = sr.nextRow();
    const row = sr.nextRow() ?? unreachable;
    expect_eq(std.mem.eql(u8, row.fields[0].raw, "a;b"), true);
    expect_eq(std.mem.eql(u8, row.fields[1].raw, "42"), true);
});

test("4. formatCsvField and formatCsvRow round-trip with CsvReader", () => {
    expect_eq(needsCsvQuoting("simple", ','), false);
    expect_eq(needsCsvQuoting("a,b", ','), true);
    expect_eq(needsCsvQuoting("a\"b", ','), true);
    expect_eq(needsCsvQuoting("a\nb", ','), true);

    let out: [256]u8 = std.mem.zeroes([256]u8);
    const f = formatCsvField("say \"hi\", ok", ',', &out) ?? unreachable;
    expect_eq(std.mem.eql(u8, f, "\"say \"\"hi\"\", ok\""), true);

    const cols: [4][]const u8 = ["plain", "with,comma", "quote\"here", "multi\nline"];
    const encoded = formatCsvRow(&cols, ',', &out) ?? unreachable;

    let reader = CsvReader.init(encoded, ',');
    const parsed = reader.nextRow() ?? unreachable;
    expect_eq(parsed.len, usize(4));

    let tmp: [64]u8 = std.mem.zeroes([64]u8);
    let i: usize = 0;
    while (i < 4) {
        const val = CsvReader.unescapeField(parsed.fields[i], &tmp) ?? unreachable;
        expect_eq(std.mem.eql(u8, val, cols[i]), true);
        i += 1;
    }
});

test("5. unclosed quote detection and buffer-too-small safety guards", () => {
    let bad = CsvReader.init("a,\"unclosed quote", ',');
    expect_eq(bad.nextRow() == null, true);
    expect_eq(bad.had_error, true);

    // 编码缓冲区不足安全返回 null
    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(formatCsvField("hello,world", ',', &tiny) == null, true);
    const cols: [2][]const u8 = ["abc", "def"];
    expect_eq(formatCsvRow(&cols, ',', &tiny) == null, true);

    // unescapeField 缓冲区不足返回 null
    let r = CsvReader.init("\"a\"\"b\"\"c\"", ',');
    const f = r.nextField() ?? unreachable;
    let small: [2]u8 = std.mem.zeroes([2]u8);
    expect_eq(unescapeField(f, &small) == null, true);
});
