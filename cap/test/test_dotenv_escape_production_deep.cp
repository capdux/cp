// Source: cp/cap/test/test_dotenv_escape_production_deep.cp (Production deep tests for cap:dotenv_escape)

import { test, expect_eq } from "cap:test";
import {
    isValidEnvKey,
    needsDotenvQuotes,
    formatDotenvValue,
    formatDotenvLine,
    unescapeDotenvDouble,
} from "cap:dotenv";

test("dotenv_escape: isValidEnvKey POSIX identifier validation") {
    expect_eq(isValidEnvKey("PORT"), true);
    expect_eq(isValidEnvKey("_PRIVATE_KEY_2"), true);
    expect_eq(isValidEnvKey("a"), true);

    expect_eq(isValidEnvKey(""), false);
    expect_eq(isValidEnvKey("2FAST"), false);
    expect_eq(isValidEnvKey("MY-VAR"), false);
    expect_eq(isValidEnvKey("MY.VAR"), false);
    expect_eq(isValidEnvKey("A B"), false);
    expect_eq(isValidEnvKey("A=B"), false);
}

test("dotenv_escape: needsDotenvQuotes detects empty, whitespace and special chars") {
    expect_eq(needsDotenvQuotes("8080"), false);
    expect_eq(needsDotenvQuotes("https://api.example.com/v1"), false);
    expect_eq(needsDotenvQuotes("simple_value-1.2"), false);

    expect_eq(needsDotenvQuotes(""), true);
    expect_eq(needsDotenvQuotes("hello world"), true);
    expect_eq(needsDotenvQuotes("val#comment"), true);
    expect_eq(needsDotenvQuotes("a\"b"), true);
    expect_eq(needsDotenvQuotes("a'b"), true);
    expect_eq(needsDotenvQuotes("a\\b"), true);
    expect_eq(needsDotenvQuotes("$HOME"), true);
    expect_eq(needsDotenvQuotes("line1\nline2"), true);
    expect_eq(needsDotenvQuotes("tab\there"), true);
}

test("dotenv_escape: formatDotenvValue and unescapeDotenvDouble round-trip") {
    let out1: [128]u8 = [0; 128];
    let out2: [128]u8 = [0; 128];

    // 安全值原样输出
    expect_eq(std.mem.eql(u8, formatDotenvValue("localhost:8080", out1[0..]).?, "localhost:8080"), true);

    // 含特殊字符双引号包裹并转义，且防 $VAR 误展开
    const raw = "line1\n\r\t\"q\"\\$VAR #end";
    const formatted = formatDotenvValue(raw, out1[0..]).?;
    expect_eq(std.mem.eql(u8, formatted, "\"line1\\n\\r\\t\\\"q\\\"\\\\\\$VAR #end\""), true);

    const restored = unescapeDotenvDouble(formatted, out2[0..]).?;
    expect_eq(std.mem.eql(u8, restored, raw), true);

    // 空串输出 ""
    const empty_fmt = formatDotenvValue("", out1[0..]).?;
    expect_eq(std.mem.eql(u8, empty_fmt, "\"\""), true);
    expect_eq(unescapeDotenvDouble(empty_fmt, out2[0..]).?.len, 0);
}

test("dotenv_escape: formatDotenvLine validates key and formats KEY=VALUE") {
    let buf: [128]u8 = [0; 128];

    const l1 = formatDotenvLine("PORT", "3000", buf[0..]).?;
    expect_eq(std.mem.eql(u8, l1, "PORT=3000"), true);

    const l2 = formatDotenvLine("SECRET", "p@ss$1 #ok", buf[0..]).?;
    expect_eq(std.mem.eql(u8, l2, "SECRET=\"p@ss\\$1 #ok\""), true);

    // 非法 key 必须拒绝
    expect_eq(formatDotenvLine("BAD-KEY", "123", buf[0..]) == null, true);
    expect_eq(formatDotenvLine("", "123", buf[0..]) == null, true);
}

test("dotenv_escape: unterminated escapes and small output buffer guards") {
    let buf: [64]u8 = [0; 64];
    expect_eq(unescapeDotenvDouble("\"unterminated", buf[0..]) == null, true);
    expect_eq(unescapeDotenvDouble("dangling\\", buf[0..]) == null, true);

    let tiny: [4]u8 = [0; 4];
    expect_eq(formatDotenvValue("hello", tiny[0..]) == null, true);
    expect_eq(formatDotenvValue("a b", tiny[0..]) == null, true);
    expect_eq(formatDotenvLine("A", "hello", tiny[0..]) == null, true);
    expect_eq(unescapeDotenvDouble("abcdef", tiny[0..]) == null, true);
}
