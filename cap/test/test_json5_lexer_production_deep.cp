// Source: cp/cap/test/test_json5_lexer_production_deep.cp (Production deep tests for cap:json5_lexer)

import { test, expect_eq } from "cap:test";
import {
    Json5TokenKind,
    Json5Token,
    Json5Lexer,
    toStandardJson,
} from "cap:json";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a >= b) a - b else b - a;
    return diff <= eps;
}

test("json5_lexer: full token kinds, structural symbols, comments and ES5 identifiers") {
    const src =
        "// header comment\n" ++
        "{ /* block */ $foo_1: 'bar', ok: true, off: false, empty: null, arr: [1, 2], }";

    let lex = Json5Lexer.init(src);

    expect_eq(lex.next().kind == .lbrace, true);

    const k1 = lex.next();
    expect_eq(k1.kind == .identifier, true);
    expect_eq(std.mem.eql(u8, k1.raw, "$foo_1"), true);

    expect_eq(lex.next().kind == .colon, true);

    const v1 = lex.next();
    expect_eq(v1.kind == Json5TokenKind.string, true);
    expect_eq(std.mem.eql(u8, v1.raw, "'bar'"), true);

    expect_eq(lex.next().kind == .comma, true);
    expect_eq(lex.next().kind == .identifier, true); // ok
    expect_eq(lex.next().kind == .colon, true);

    const b1 = lex.next();
    expect_eq(b1.kind == .boolean, true);
    expect_eq(b1.bool_val, true);

    expect_eq(lex.next().kind == .comma, true);
    expect_eq(lex.next().kind == .identifier, true); // off
    expect_eq(lex.next().kind == .colon, true);

    const b2 = lex.next();
    expect_eq(b2.kind == .boolean, true);
    expect_eq(b2.bool_val, false);

    expect_eq(lex.next().kind == .comma, true);
    expect_eq(lex.next().kind == .identifier, true); // empty
    expect_eq(lex.next().kind == .colon, true);
    expect_eq(lex.next().kind == .null_lit, true);

    expect_eq(lex.next().kind == .comma, true);
    expect_eq(lex.next().kind == .identifier, true); // arr
    expect_eq(lex.next().kind == .colon, true);
    expect_eq(lex.next().kind == .lbracket, true);
    expect_eq(lex.next().kind == .number, true); // 1
    expect_eq(lex.next().kind == .comma, true);
    expect_eq(lex.next().kind == .number, true); // 2
    expect_eq(lex.next().kind == .rbracket, true);
    expect_eq(lex.next().kind == .comma, true); // trailing comma
    expect_eq(lex.next().kind == .rbrace, true);
    expect_eq(lex.next().kind == .eof, true);
}

test("json5_lexer: hex numbers, leading/trailing dot, plus sign, scientific, Infinity and NaN") {
    const src = "0x1A -0xFF +42 .5 5. 1e-3 Infinity -Infinity NaN";
    let lex = Json5Lexer.init(src);

    const t_hex = lex.next();
    expect_eq(t_hex.kind == .number, true);
    expect_eq(approxEq(t_hex.num_val, 26.0, 0.000001), true);

    const t_nhex = lex.next();
    expect_eq(t_nhex.kind == .number, true);
    expect_eq(approxEq(t_nhex.num_val, -255.0, 0.000001), true);

    const t_plus = lex.next();
    expect_eq(t_plus.kind == .number, true);
    expect_eq(approxEq(t_plus.num_val, 42.0, 0.000001), true);

    const t_ldot = lex.next();
    expect_eq(t_ldot.kind == .number, true);
    expect_eq(approxEq(t_ldot.num_val, 0.5, 0.000001), true);

    const t_tdot = lex.next();
    expect_eq(t_tdot.kind == .number, true);
    expect_eq(approxEq(t_tdot.num_val, 5.0, 0.000001), true);

    const t_sci = lex.next();
    expect_eq(t_sci.kind == .number, true);
    expect_eq(approxEq(t_sci.num_val, 0.001, 0.0000001), true);

    const t_inf = lex.next();
    expect_eq(t_inf.kind == .number, true);
    expect_eq(std.math.isPositiveInf(t_inf.num_val), true);

    const t_ninf = lex.next();
    expect_eq(t_ninf.kind == .number, true);
    expect_eq(std.math.isNegativeInf(t_ninf.num_val), true);

    const t_nan = lex.next();
    expect_eq(t_nan.kind == .number, true);
    expect_eq(std.math.isNan(t_nan.num_val), true);

    expect_eq(lex.next().kind == .eof, true);
}

test("json5_lexer: single-quoted strings, escapes and line continuations") {
    const src = "'it\\'s \"quoted\"' \"line1\\\nline2\"";
    let lex = Json5Lexer.init(src);

    const s1 = lex.next();
    expect_eq(s1.kind == Json5TokenKind.string, true);
    expect_eq(s1.start, 0);

    const s2 = lex.next();
    expect_eq(s2.kind == Json5TokenKind.string, true);
    expect_eq(lex.next().kind == .eof, true);

    let out: [128]u8 = [0; 128];
    const j1 = toStandardJson("'it\\'s \"quoted\"'", out[0..]).?;
    expect_eq(std.mem.eql(u8, j1, "\"it's \\\"quoted\\\"\""), true);

    const j2 = toStandardJson("\"line1\\\nline2\"", out[0..]).?;
    expect_eq(std.mem.eql(u8, j2, "\"line1line2\""), true);
}

test("json5_lexer: toStandardJson full object/array with comments, trailing commas, hex and unquoted keys") {
    const src =
        "{\n" ++
        "  // app settings\n" ++
        "  $name: 'cap-app',\n" ++
        "  port: 0x1F90,\n" ++
        "  negHex: -0x10,\n" ++
        "  ratio: .5,\n" ++
        "  whole: 5.,\n" ++
        "  boost: +3,\n" ++
        "  /* nested list */\n" ++
        "  tags: ['a', 'b',],\n" ++
        "  nested: { ok: true, empty: null, },\n" ++
        "}\n";

    let out: [512]u8 = [0; 512];
    const res = toStandardJson(src, out[0..]).?;
    const expected = "{\"$name\":\"cap-app\",\"port\":8080,\"negHex\":-16,\"ratio\":0.5,\"whole\":5,\"boost\":3,\"tags\":[\"a\",\"b\"],\"nested\":{\"ok\":true,\"empty\":null}}";
    expect_eq(std.mem.eql(u8, res, expected), true);
}

test("json5_lexer: invalid token guards for unclosed comments/strings, bad hex and non-finite JSON") {
    // 1. 未闭合多行注释
    let l1 = Json5Lexer.init("/* unclosed comment");
    expect_eq(l1.next().kind == .invalid, true);

    // 2. 未闭合单引号/双引号字符串与裸换行
    let l2 = Json5Lexer.init("'unterminated");
    expect_eq(l2.next().kind == .invalid, true);

    let l3 = Json5Lexer.init("\"raw\nnewline\"");
    expect_eq(l3.next().kind == .invalid, true);

    // 3. 畸形十六进制与前导零
    let l4 = Json5Lexer.init("0x");
    expect_eq(l4.next().kind == .invalid, true);

    let l5 = Json5Lexer.init("0123");
    expect_eq(l5.next().kind == .invalid, true);

    // 4. 标准 JSON 拒绝 Infinity / NaN、尾随非法 token 与小缓冲区溢出
    let out: [64]u8 = [0; 64];
    expect_eq(toStandardJson("{a: Infinity}", out[0..]) == null, true);
    expect_eq(toStandardJson("{a: NaN}", out[0..]) == null, true);
    expect_eq(toStandardJson("{a: 1} extra", out[0..]) == null, true);
    expect_eq(toStandardJson("{a: 1,,}", out[0..]) == null, true);

    let tiny: [4]u8 = [0; 4];
    expect_eq(toStandardJson("{long_key: 'value'}", tiny[0..]) == null, true);
}
