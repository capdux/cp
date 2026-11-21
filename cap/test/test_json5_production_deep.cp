// Source: cp/cap/test/test_json5_production_deep.cp (Production deep tests for cap:json5)

import { test, expect_eq } from "cap:test";
import {
    Json5TokenKind,
    Json5Token,
    Json5Iter,
    toStandardJson,
    isValidJson5,
} from "cap:json5";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("1. Json5Iter tokenization: comments, identifiers, quotes, numbers, literals") {
    const src = "/* header */ { // line comment\n  $key_1: 'val', hex: 0x1A, num: +.5, ok: true, no: false, nil: null, inf: -Infinity, nan: NaN }";
    let it = Json5Iter.init(src);

    const t0 = it.next().?;
    expect_eq(t0.kind == .lbrace, true);

    const t1 = it.next().?;
    expect_eq(t1.kind == .ident, true);
    expect_eq(strEq(t1.lexeme, "$key_1"), true);

    expect_eq(it.next().?.kind == .colon, true);

    const t3 = it.next().?;
    expect_eq(t3.kind == Json5TokenKind.string, true);
    expect_eq(strEq(t3.lexeme, "'val'"), true);

    expect_eq(it.next().?.kind == .comma, true);
    expect_eq(it.next().?.kind == .ident, true); // hex
    expect_eq(it.next().?.kind == .colon, true);

    const hex_tok = it.next().?;
    expect_eq(hex_tok.kind == .number, true);
    expect_eq(strEq(hex_tok.lexeme, "0x1A"), true);

    // Drain remaining tokens and confirm no invalid tokens
    let count: usize = 0;
    while (tok := it.next()) {
        expect_eq(tok.kind != .invalid, true);
        count += 1;
    }
    expect_eq(count > 10, true);
}

test("2. toStandardJson: unquoted keys, trailing commas, comments, single quotes & escapes") {
    let buf: [512]u8 = [0; 512];

    const src =
        "// top comment\n" ++
        "{\n" ++
        "  unquoted: 'single \"quoted\" \\'text\\'',\n" ++
        "  /* block comment */\n" ++
        "  multi: 'line1\\\nline2\\\r\nline3',\n" ++
        "  esc: '\\x41\\v\\0\\u0042',\n" ++
        "  true: [1, 2, ],\n" ++
        "}\n";

    const out = toStandardJson(src, buf[0..]).?;
    const expected = "{\"unquoted\":\"single \\\"quoted\\\" 'text'\",\"multi\":\"line1line2line3\",\"esc\":\"\\u0041\\u000b\\u0000\\u0042\",\"true\":[1,2]}";
    expect_eq(strEq(out, expected), true);
    expect_eq(isValidJson5(src), true);
}

test("3. toStandardJson: hex, leading/trailing decimal points, plus sign, Infinity & NaN") {
    let buf: [512]u8 = [0; 512];

    const src = "{ hex: 0xFF, negHex: -0x10, posHex: +0X0A, lead: .5, negLead: -.25, trail: 5., expTrail: 5.e2, plus: +42, inf: Infinity, ninf: -Infinity, pinf: +Infinity, nan: NaN }";
    const out = toStandardJson(src, buf[0..]).?;
    const expected = "{\"hex\":255,\"negHex\":-16,\"posHex\":10,\"lead\":0.5,\"negLead\":-0.25,\"trail\":5.0,\"expTrail\":5.0e2,\"plus\":42,\"inf\":null,\"ninf\":null,\"pinf\":null,\"nan\":null}";
    expect_eq(strEq(out, expected), true);
    expect_eq(isValidJson5(src), true);
}

test("4. Nested objects, empty containers, UTF-8 BOM/NBSP whitespace and root primitives") {
    let buf: [256]u8 = [0; 256];

    // UTF-8 BOM + empty containers + nested arrays/objects
    const nested = "\xEF\xBB\xBF { a: {}, b: [], c: [{ d: [1, { e: 'ok', }, ], }, ], }";
    const out_nested = toStandardJson(nested, buf[0..]).?;
    expect_eq(strEq(out_nested, "{\"a\":{},\"b\":[],\"c\":[{\"d\":[1,{\"e\":\"ok\"}]}]}"), true);

    // Root primitives
    expect_eq(strEq(toStandardJson("  +0x2A  ", buf[0..]).?, "42"), true);
    expect_eq(strEq(toStandardJson("'hello'", buf[0..]).?, "\"hello\""), true);
    expect_eq(strEq(toStandardJson("true // ok", buf[0..]).?, "true"), true);
    expect_eq(strEq(toStandardJson("null", buf[0..]).?, "null"), true);
    expect_eq(isValidJson5("Infinity"), true);
    expect_eq(isValidJson5("-NaN"), true);
}

test("5. Malformed input rejection, buffer overflow protection & deep nesting guard") {
    let buf: [128]u8 = [0; 128];

    // Malformed syntax cases
    expect_eq(isValidJson5(""), false);
    expect_eq(isValidJson5("   // only comment"), false);
    expect_eq(isValidJson5("/* unclosed comment"), false);
    expect_eq(isValidJson5("'unterminated"), false);
    expect_eq(isValidJson5("'raw\nnewline'"), false);
    expect_eq(isValidJson5("{ a: 1,, }"), false);
    expect_eq(isValidJson5("[1, , 2]"), false);
    expect_eq(isValidJson5("[, 1]"), false);
    expect_eq(isValidJson5("{ , a: 1 }"), false);
    expect_eq(isValidJson5("{ a: 1 } extra"), false);
    expect_eq(isValidJson5("0123"), false);
    expect_eq(isValidJson5("0x"), false);
    expect_eq(isValidJson5("1e"), false);
    expect_eq(isValidJson5("."), false);
    expect_eq(isValidJson5("{ 123: 'bad key' }"), false);
    expect_eq(toStandardJson("{ a: 1,, }", buf[0..]) == null, true);

    // Output buffer too small -> returns null without panic
    let tiny: [5]u8 = [0; 5];
    expect_eq(toStandardJson("{ key: 'value' }", tiny[0..]) == null, true);

    // Deep nesting (> 64) rejected safely
    let deep: [140]u8 = [0; 140];
    let i: usize = 0;
    while (i < 70) : (i += 1) {
        deep[i] = '[';
        deep[139 - i] = ']';
    }
    expect_eq(isValidJson5(deep[0..]), false);
    expect_eq(toStandardJson(deep[0..], buf[0..]) == null, true);
}
