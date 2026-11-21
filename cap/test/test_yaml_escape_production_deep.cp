import std from "std";
import { test, expect_eq } from "cap:test";
import {
    needsYamlQuotes,
    quoteYamlSingle,
    quoteYamlDouble,
    formatYamlScalar,
    unescapeYamlDouble,
} from "cap:yaml_escape";

test("1. needsYamlQuotes catches keywords, numbers, indicators, whitespace, and control chars", () => {
    expect_eq(needsYamlQuotes("hello-world"), false);
    expect_eq(needsYamlQuotes("v1.2.3"), false);
    expect_eq(needsYamlQuotes("path/to/file.cp"), false);

    // 空串、首尾空白、保留字
    expect_eq(needsYamlQuotes(""), true);
    expect_eq(needsYamlQuotes(" lead"), true);
    expect_eq(needsYamlQuotes("trail "), true);
    expect_eq(needsYamlQuotes("true"), true);
    expect_eq(needsYamlQuotes("FALSE"), true);
    expect_eq(needsYamlQuotes("null"), true);
    expect_eq(needsYamlQuotes("~"), true);
    expect_eq(needsYamlQuotes("yes"), true);
    expect_eq(needsYamlQuotes("off"), true);

    // 数字外观（整数/小数/十六进制/八进制）
    expect_eq(needsYamlQuotes("42"), true);
    expect_eq(needsYamlQuotes("-3.14"), true);
    expect_eq(needsYamlQuotes("0xFF"), true);
    expect_eq(needsYamlQuotes("0o755"), true);

    // 指示符
    expect_eq(needsYamlQuotes("a: b"), true);
    expect_eq(needsYamlQuotes("a #comment"), true);
    expect_eq(needsYamlQuotes("*anchor"), true);
    expect_eq(needsYamlQuotes("&ref"), true);
    expect_eq(needsYamlQuotes("[1,2]"), true);
});

test("2. quoteYamlSingle escapes single quotes and rejects control chars", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    expect_eq(std.mem.eql(u8, quoteYamlSingle("it's ok", &buf) ?? unreachable, "'it''s ok'"), true);
    expect_eq(quoteYamlSingle("line1\nline2", &buf) == null, true);
    expect_eq(quoteYamlSingle("tab\there", &buf) == null, true);
});

test("3. quoteYamlDouble and unescapeYamlDouble round-trip all YAML 1.2 control escapes", () => {
    const raw: [11]u8 = ['"', '\\', 0x00, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x1B];
    let esc_buf: [128]u8 = std.mem.zeroes([128]u8);
    let out_buf: [128]u8 = std.mem.zeroes([128]u8);

    const quoted = quoteYamlDouble(&raw, &esc_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, quoted, "\"\\\"\\\\\\0\\a\\b\\t\\n\\v\\f\\r\\e\""), true);

    const back = unescapeYamlDouble(quoted, &out_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, back, &raw), true);

    // \xHH 与 \uXXXX / \UXXXXXXXX 还原
    const uni = unescapeYamlDouble("\"\\x41\\u4E2D\\U0001F680\"", &out_buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, uni, "A中🚀"), true);
});

test("4. formatYamlScalar emits plain safe scalars and double-quotes unsafe ones", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    expect_eq(std.mem.eql(u8, formatYamlScalar("cp-lang", &buf) ?? unreachable, "cp-lang"), true);
    expect_eq(std.mem.eql(u8, formatYamlScalar("null", &buf) ?? unreachable, "\"null\""), true);
    expect_eq(std.mem.eql(u8, formatYamlScalar("a: b", &buf) ?? unreachable, "\"a: b\""), true);
});

test("5. unescapeYamlDouble malformed escapes and buffer-too-small guards", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    expect_eq(unescapeYamlDouble("\"\\q\"", &buf) == null, true);
    expect_eq(unescapeYamlDouble("\"\\x1\"", &buf) == null, true);
    expect_eq(unescapeYamlDouble("\"\\uD800\"", &buf) == null, true);
    expect_eq(unescapeYamlDouble("trailing\\", &buf) == null, true);

    let tiny: [3]u8 = std.mem.zeroes([3]u8);
    expect_eq(quoteYamlSingle("abc", &tiny) == null, true);
    expect_eq(quoteYamlDouble("abc", &tiny) == null, true);
    expect_eq(formatYamlScalar("hello", &tiny) == null, true);
});
