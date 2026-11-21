import std from "std";
import { test, expect_eq } from "cap:test";
import {
    needsCsvQuotes,
    escapeCsvField,
    unescapeCsvField,
    formatCsvRow,
    isFormulaInjectionRisk,
} from "cap:csv";

test("1. needsCsvQuotes and escapeCsvField handle CSV and TSV delimiters", () => {
    expect_eq(needsCsvQuotes("plain", ','), false);
    expect_eq(needsCsvQuotes("a,b", ','), true);
    expect_eq(needsCsvQuotes("a,b", '\t'), false);
    expect_eq(needsCsvQuotes("a\tb", '\t'), true);
    expect_eq(needsCsvQuotes("a\"b", ','), true);
    expect_eq(needsCsvQuotes("a\nb", ','), true);
    expect_eq(needsCsvQuotes("a\rb", ','), true);

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    expect_eq(std.mem.eql(u8, escapeCsvField("plain", ',', &buf) ?? unreachable, "plain"), true);
    expect_eq(std.mem.eql(u8, escapeCsvField("a, \"b\"", ',', &buf) ?? unreachable, "\"a, \"\"b\"\"\""), true);
    expect_eq(std.mem.eql(u8, escapeCsvField("x\ty", '\t', &buf) ?? unreachable, "\"x\ty\""), true);
});

test("2. unescapeCsvField round-trips quoted fields and rejects malformed quotes", () => {
    let esc_buf: [128]u8 = std.mem.zeroes([128]u8);
    let out_buf: [128]u8 = std.mem.zeroes([128]u8);

    const orig = "line1\n\"quoted\", end";
    const escaped = escapeCsvField(orig, ',', &esc_buf) ?? unreachable;
    const unesc = unescapeCsvField(escaped, &out_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, unesc, orig), true);

    // 非引号原样拷贝与空引号 "" -> ""
    expect_eq(std.mem.eql(u8, unescapeCsvField("plain", &out_buf) ?? unreachable, "plain"), true);
    expect_eq(std.mem.eql(u8, unescapeCsvField("\"\"", &out_buf) ?? unreachable, ""), true);

    // 未闭合引号与内部孤立引号拒绝
    expect_eq(unescapeCsvField("\"unclosed", &out_buf) == null, true);
    expect_eq(unescapeCsvField("\"", &out_buf) == null, true);
    expect_eq(unescapeCsvField("\"bad \" lone quote\"", &out_buf) == null, true);
});

test("3. formatCsvRow supports CRLF, LF, and TSV tab delimiter", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const cols: [3][]const u8 = ["id", "a,b", "c\"d"];

    const r_crlf = formatCsvRow(&cols, ',', true, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, r_crlf, "id,\"a,b\",\"c\"\"d\"\r\n"), true);

    const r_lf = formatCsvRow(&cols, '\t', false, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, r_lf, "id\ta,b\t\"c\"\"d\"\n"), true);
});

test("4. isFormulaInjectionRisk detects spreadsheet formula prefixes", () => {
    expect_eq(isFormulaInjectionRisk("=1+1"), true);
    expect_eq(isFormulaInjectionRisk("+cmd"), true);
    expect_eq(isFormulaInjectionRisk("-2+3"), true);
    expect_eq(isFormulaInjectionRisk("@SUM(A1)"), true);
    expect_eq(isFormulaInjectionRisk("\t=1"), true);
    expect_eq(isFormulaInjectionRisk("\r=1"), true);

    expect_eq(isFormulaInjectionRisk(""), false);
    expect_eq(isFormulaInjectionRisk("normal text"), false);
    expect_eq(isFormulaInjectionRisk("12345"), false);
    expect_eq(isFormulaInjectionRisk("a=b"), false);
});

test("5. buffer-too-small safety guards", () => {
    let tiny: [3]u8 = std.mem.zeroes([3]u8);
    expect_eq(escapeCsvField("hello", ',', &tiny) == null, true);
    expect_eq(escapeCsvField("a,b", ',', &tiny) == null, true);
    expect_eq(unescapeCsvField("\"abcd\"", &tiny) == null, true);

    const cols: [2][]const u8 = ["ab", "cd"];
    expect_eq(formatCsvRow(&cols, ',', true, &tiny) == null, true);
});
