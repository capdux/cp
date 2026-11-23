// Source: cp/cap/test/test_dotenv_parse_production_deep.cp (Production deep tests for cap:dotenv_parse)

import { test, expect_eq } from "cap:test";
import {
    DotEnvQuote,
    DotEnvEntry,
    DotEnvIterator,
    unescapeDoubleQuoted,
    expandVars,
} from "cap:cli";

fn mockLookup(name: []const u8): ([]const u8)? {
    if (std.mem.eql(u8, name, "HOST")) { return "127.0.0.1"; }
    if (std.mem.eql(u8, name, "PORT")) { return "8080"; }
    if (std.mem.eql(u8, name, "EMPTY")) { return ""; }
    return null;
}

test("dotenv_parse: UTF-8 BOM, export prefix, comments and unquoted inline comment stripping") {
    const src =
        "\xEF\xBB\xBF" ++
        "# top comment\n" ++
        "\n" ++
        "export PORT = 8080  # listening port\n" ++
        "export_dir=/var/data\n" ++
        "ACCENT=#ff00ff\n" ++
        "EMPTY_VAL=\n";

    let it = DotEnvIterator.init(src);

    const e1 = it.next().?;
    expect_eq(std.mem.eql(u8, e1.key, "PORT"), true);
    expect_eq(std.mem.eql(u8, e1.raw_value, "8080"), true);
    expect_eq(e1.quote == .unquoted, true);

    const e2 = it.next().?;
    expect_eq(std.mem.eql(u8, e2.key, "export_dir"), true);
    expect_eq(std.mem.eql(u8, e2.raw_value, "/var/data"), true);

    const e3 = it.next().?;
    expect_eq(std.mem.eql(u8, e3.key, "ACCENT"), true);
    expect_eq(std.mem.eql(u8, e3.raw_value, "#ff00ff"), true);

    const e4 = it.next().?;
    expect_eq(std.mem.eql(u8, e4.key, "EMPTY_VAL"), true);
    expect_eq(e4.raw_value.len, 0);

    expect_eq(it.next() == null, true);
}

test("dotenv_parse: all 4 quote modes including multiline double-quoted and backtick") {
    const src =
        "PLAIN=hello world\n" ++
        "SINGLE='literal \\n $HOST # not comment'\n" ++
        "DOUBLE=\"line1\nline2 \\\"quoted\\\"\"\n" ++
        "TICK=`multi\n'both' \"quotes\"`\n";

    let it = DotEnvIterator.init(src);

    const e1 = it.next().?;
    expect_eq(e1.quote == .unquoted, true);
    expect_eq(std.mem.eql(u8, e1.raw_value, "hello world"), true);

    const e2 = it.next().?;
    expect_eq(e2.quote == .single, true);
    expect_eq(std.mem.eql(u8, e2.raw_value, "literal \\n $HOST # not comment"), true);

    const e3 = it.next().?;
    expect_eq(e3.quote == .double, true);
    expect_eq(std.mem.eql(u8, e3.raw_value, "line1\nline2 \\\"quoted\\\""), true);

    const e4 = it.next().?;
    expect_eq(e4.quote == .backtick, true);
    expect_eq(std.mem.eql(u8, e4.raw_value, "multi\n'both' \"quotes\""), true);
    expect_eq(std.mem.eql(u8, e4.quote.asStr(), "backtick"), true);

    expect_eq(it.next() == null, true);
}

test("dotenv_parse: unescapeDoubleQuoted handles \\n, \\r, \\t, \\\", \\\\, \\$") {
    let buf: [128]u8 = [0; 128];
    const out = unescapeDoubleQuoted("a\\nb\\rc\\td\\\"e\\\\f\\$g", buf[0..]).?;
    expect_eq(std.mem.eql(u8, out, "a\nb\rc\td\"e\\f$g"), true);

    // 缓冲区溢出保护
    let tiny: [3]u8 = [0; 3];
    expect_eq(unescapeDoubleQuoted("hello", tiny[0..]) == null, true);
}

test("dotenv_parse: expandVars supports $VAR, ${VAR}, ${VAR:-fallback} and \\$ escape") {
    let buf: [256]u8 = [0; 256];

    const s1 = expandVars("http://$HOST:${PORT}/api", mockLookup, buf[0..]).?;
    expect_eq(std.mem.eql(u8, s1, "http://127.0.0.1:8080/api"), true);

    // 未定义与空值均触发 :- 默认值
    const s2 = expandVars("${MISSING:-default_val}|${EMPTY:-fallback}|${HOST:-unused}", mockLookup, buf[0..]).?;
    expect_eq(std.mem.eql(u8, s2, "default_val|fallback|127.0.0.1"), true);

    // \$ 逃逸保留字面量 $
    const s3 = expandVars("price=\\$100 host=\\$HOST real=$HOST", mockLookup, buf[0..]).?;
    expect_eq(std.mem.eql(u8, s3, "price=$100 host=$HOST real=127.0.0.1"), true);

    // 未定义无默认值展开为空串
    const s4 = expandVars("pre-$UNKNOWN-post", mockLookup, buf[0..]).?;
    expect_eq(std.mem.eql(u8, s4, "pre--post"), true);

    // 缓冲区溢出保护
    let tiny: [4]u8 = [0; 4];
    expect_eq(expandVars("http://$HOST", mockLookup, tiny[0..]) == null, true);
}

test("dotenv_parse: malformed lines without '=' or empty key are safely skipped") {
    const src =
        "MALFORMED_NO_EQUALS\n" ++
        "   =missing_key\n" ++
        "VALID_KEY=ok\n" ++
        "TRAILING_GARBAGE";

    let it = DotEnvIterator.init(src);
    const e1 = it.next().?;
    expect_eq(std.mem.eql(u8, e1.key, "VALID_KEY"), true);
    expect_eq(std.mem.eql(u8, e1.raw_value, "ok"), true);
    expect_eq(it.next() == null, true);
}
