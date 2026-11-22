// Source: cp/cap/test/test_hjson_production_deep.cp (Production deep tests for cap:hjson)

import { test, expect_eq } from "cap:test";
import {
    HjsonTokenKind,
    HjsonToken,
    hjsonToJson,
    isValidHjson,
    stripHjsonComments,
} from "cap:conf";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("1. hjsonToJson: optional commas, unquoted keys, quoteless values & all 3 comment styles") {
    let buf: [512]u8 = [0; 512];

    const src =
        "# hash header\n" ++
        "{\n" ++
        "  // line comment\n" ++
        "  name: Hello, Hjson World!   \n" ++
        "  /* block comment */\n" ++
        "  port: 8080\n" ++
        "  ratio: -3.14e2\n" ++
        "  enabled: true\n" ++
        "  extra: null\n" ++
        "  tags: [\n" ++
        "    alpha\n" ++
        "    beta\n" ++
        "    42\n" ++
        "  ]\n" ++
        "}\n";

    expect_eq(isValidHjson(src), true);
    const json = hjsonToJson(src, buf[0..]).?;
    const expected = "{\"name\":\"Hello, Hjson World!\",\"port\":8080,\"ratio\":-3.14e2,\"enabled\":true,\"extra\":null,\"tags\":[\"alpha\",\"beta\",42]}";
    expect_eq(strEq(json, expected), true);
}

test("2. hjsonToJson: single/double quoted strings, trailing commas & UTF-8 BOM") {
    let buf: [512]u8 = [0; 512];

    const src =
        "\xEF\xBB\xBF{\n" ++
        "  'single key': 'value with \"quotes\" and \\'apos\\'',\n" ++
        "  \"double key\": \"line1\\nline2\",\n" ++
        "  arr: [1, 2, 3, ],\n" ++
        "  emptyObj: {},\n" ++
        "  emptyArr: [],\n" ++
        "}\n";

    expect_eq(isValidHjson(src), true);
    const json = hjsonToJson(src, buf[0..]).?;
    const expected = "{\"single key\":\"value with \\\"quotes\\\" and 'apos'\",\"double key\":\"line1\\nline2\",\"arr\":[1,2,3],\"emptyObj\":{},\"emptyArr\":[]}";
    expect_eq(strEq(json, expected), true);
}

test("3. hjsonToJson: triple-quoted '''...''' multiline strings with indentation stripping") {
    let buf: [512]u8 = [0; 512];

    const src =
        "{\n" ++
        "  desc:\n" ++
        "    '''\n" ++
        "    First line\n" ++
        "      Indented second line\n" ++
        "    Third \"quoted\" line\n" ++
        "    '''\n" ++
        "}\n";

    expect_eq(isValidHjson(src), true);
    const json = hjsonToJson(src, buf[0..]).?;
    const expected = "{\"desc\":\"First line\\n  Indented second line\\nThird \\\"quoted\\\" line\"}";
    expect_eq(strEq(json, expected), true);
}

test("4. stripHjsonComments: preserves # // /* inside single, double and triple-quoted strings") {
    let buf: [512]u8 = [0; 512];

    const src =
        "# top comment\n" ++
        "{\n" ++
        "  url: \"https://example.com/#frag /* keep */\" // drop me\n" ++
        "  single: 'keep # and //' # drop hash\n" ++
        "  multi: '''# keep\n// also keep''' /* block\ncomment */\n" ++
        "}\n";

    const stripped = stripHjsonComments(src, buf[0..]).?;
    const expected =
        "\n" ++
        "{\n" ++
        "  url: \"https://example.com/#frag /* keep */\" \n" ++
        "  single: 'keep # and //' \n" ++
        "  multi: '''# keep\n// also keep''' \n\n" ++
        "}\n";
    expect_eq(strEq(stripped, expected), true);

    // Unclosed block comment & tiny buffer rejected
    expect_eq(stripHjsonComments("a /* unclosed", buf[0..]) == null, true);
    let tiny: [4]u8 = [0; 4];
    expect_eq(stripHjsonComments("{ a: 1 }", tiny[0..]) == null, true);
}

test("5. Malformed Hjson rejection, buffer overflow protection & deep nesting guard") {
    let buf: [128]u8 = [0; 128];

    // Verify token enum variants compile and compare cleanly
    const tok = HjsonToken{ kind: HjsonTokenKind.quoteless_string, lexeme: "hello" };
    expect_eq(tok.kind == HjsonTokenKind.quoteless_string, true);

    // Malformed structures
    expect_eq(isValidHjson(""), false);
    expect_eq(isValidHjson("  # only comment"), false);
    expect_eq(isValidHjson("/* unclosed block"), false);
    expect_eq(isValidHjson("{ a: \"unterminated }"), false);
    expect_eq(isValidHjson("{ a: '''unterminated }"), false);
    expect_eq(isValidHjson("{ a: 1 b: 2 }"), false);
    expect_eq(isValidHjson("[1 2]"), false);
    expect_eq(isValidHjson("[1,,2]"), false);
    expect_eq(isValidHjson("{ a: 1,, }"), false);
    expect_eq(isValidHjson("{ missing_colon 1 }"), false);
    expect_eq(isValidHjson("{ a: 1 } trailing"), false);

    // Buffer too small
    let tiny: [6]u8 = [0; 6];
    expect_eq(hjsonToJson("{ key: value }", tiny[0..]) == null, true);

    // Deep nesting (> 64) rejected safely
    let deep: [140]u8 = [0; 140];
    let i: usize = 0;
    while (i < 70) : (i += 1) {
        deep[i] = '[';
        deep[139 - i] = ']';
    }
    expect_eq(isValidHjson(deep[0..]), false);
    expect_eq(hjsonToJson(deep[0..], buf[0..]) == null, true);
}
