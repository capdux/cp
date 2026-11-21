// Source: cp/cap/test/test_yaml_scalar_production_deep.cp (Production deep tests for cap:yaml_scalar)

import { test, expect_eq } from "cap:test";
import {
    YamlScalarKind,
    YamlScalar,
    parseCoreScalar,
    unescapeDoubleQuoted,
    unescapeSingleQuoted,
    needsQuoting,
    formatSafeScalar,
} from "cap:yaml";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a >= b) a - b else b - a;
    return diff <= eps;
}

test("yaml_scalar: YAML 1.2 Core Schema null, bool and yes/no/on/off remain strings") {
    expect_eq(parseCoreScalar("").kind == .null_val, true);
    expect_eq(parseCoreScalar("   ").kind == .null_val, true);
    expect_eq(parseCoreScalar("~").kind == .null_val, true);
    expect_eq(parseCoreScalar("null").kind == .null_val, true);
    expect_eq(parseCoreScalar("Null").kind == .null_val, true);
    expect_eq(parseCoreScalar("NULL").kind == .null_val, true);

    const b1 = parseCoreScalar("true");
    expect_eq(b1.kind == .bool_val && b1.bool_v == true, true);
    const b2 = parseCoreScalar("True");
    expect_eq(b2.kind == .bool_val && b2.bool_v == true, true);
    const b3 = parseCoreScalar("TRUE");
    expect_eq(b3.kind == .bool_val && b3.bool_v == true, true);
    const b4 = parseCoreScalar("false");
    expect_eq(b4.kind == .bool_val && b4.bool_v == false, true);
    const b5 = parseCoreScalar("False");
    expect_eq(b5.kind == .bool_val && b5.bool_v == false, true);
    const b6 = parseCoreScalar("FALSE");
    expect_eq(b6.kind == .bool_val && b6.bool_v == false, true);

    // YAML 1.2 Core Schema: yes/no/on/off 必须保持为 string_val，而非布尔
    expect_eq(parseCoreScalar("yes").kind == .string_val, true);
    expect_eq(parseCoreScalar("no").kind == .string_val, true);
    expect_eq(parseCoreScalar("on").kind == .string_val, true);
    expect_eq(parseCoreScalar("off").kind == .string_val, true);
    expect_eq(parseCoreScalar("tRuE").kind == .string_val, true);
}

test("yaml_scalar: decimal, octal 0o, hex 0x integers and floats including .inf/.nan") {
    const i_dec = parseCoreScalar("-42");
    expect_eq(i_dec.kind == .int_val && i_dec.int_v == -42, true);

    const i_plus = parseCoreScalar("+1024");
    expect_eq(i_plus.kind == .int_val && i_plus.int_v == 1024, true);

    const i_oct = parseCoreScalar("0o755");
    expect_eq(i_oct.kind == .int_val && i_oct.int_v == 493, true);

    const i_hex = parseCoreScalar("0x1F90");
    expect_eq(i_hex.kind == .int_val && i_hex.int_v == 8080, true);

    const f_std = parseCoreScalar("3.14159");
    expect_eq(f_std.kind == .float_val && approxEq(f_std.float_v, 3.14159, 0.00001), true);

    const f_sci = parseCoreScalar("-2.5e-2");
    expect_eq(f_sci.kind == .float_val && approxEq(f_sci.float_v, -0.025, 0.000001), true);

    const f_inf = parseCoreScalar(".inf");
    expect_eq(f_inf.kind == .float_val && std.math.isPositiveInf(f_inf.float_v), true);

    const f_ninf = parseCoreScalar("-.INF");
    expect_eq(f_ninf.kind == .float_val && std.math.isNegativeInf(f_ninf.float_v), true);

    const f_nan = parseCoreScalar(".NaN");
    expect_eq(f_nan.kind == .float_val && std.math.isNan(f_nan.float_v), true);

    // 非法八进制数字应降级为字符串
    expect_eq(parseCoreScalar("0o89").kind == .string_val, true);
}

test("yaml_scalar: unescapeDoubleQuoted and unescapeSingleQuoted") {
    let buf: [128]u8 = [0; 128];

    const dq = unescapeDoubleQuoted("\"a\\nb\\rc\\td\\\\e\\\"f\\/g\\0h\\x41\"", buf[0..]).?;
    expect_eq(dq.len, 16);
    expect_eq(dq[0], 'a');
    expect_eq(dq[1], '\n');
    expect_eq(dq[2], 'b');
    expect_eq(dq[3], '\r');
    expect_eq(dq[4], 'c');
    expect_eq(dq[5], '\t');
    expect_eq(dq[6], 'd');
    expect_eq(dq[7], '\\');
    expect_eq(dq[8], 'e');
    expect_eq(dq[9], '"');
    expect_eq(dq[10], 'f');
    expect_eq(dq[11], '/');
    expect_eq(dq[12], 'g');
    expect_eq(dq[13], 0);
    expect_eq(dq[14], 'h');
    expect_eq(dq[15], 'A');

    const sq = unescapeSingleQuoted("'it''s a ''quoted'' string'", buf[0..]).?;
    expect_eq(std.mem.eql(u8, sq, "it's a 'quoted' string"), true);

    // 非法转义与未闭合引号保护
    expect_eq(unescapeDoubleQuoted("\"unterminated", buf[0..]) == null, true);
    expect_eq(unescapeDoubleQuoted("\"bad\\xZZ\"", buf[0..]) == null, true);
    expect_eq(unescapeDoubleQuoted("\"bad\\q\"", buf[0..]) == null, true);
    expect_eq(unescapeSingleQuoted("'unterminated", buf[0..]) == null, true);
    expect_eq(unescapeSingleQuoted("'lone'quote'", buf[0..]) == null, true);
}

test("yaml_scalar: needsQuoting detection for keywords, numbers and YAML indicators") {
    // 安全纯字符串无需引号
    expect_eq(needsQuoting("hello_world"), false);
    expect_eq(needsQuoting("yes"), false);
    expect_eq(needsQuoting("cap-lang/cp"), false);
    expect_eq(needsQuoting("https://example.com/a:b"), false);

    // 关键字与数字必须加引号
    expect_eq(needsQuoting(""), true);
    expect_eq(needsQuoting("null"), true);
    expect_eq(needsQuoting("~"), true);
    expect_eq(needsQuoting("true"), true);
    expect_eq(needsQuoting("42"), true);
    expect_eq(needsQuoting("0x10"), true);
    expect_eq(needsQuoting(".inf"), true);

    // YAML 指示符、映射分隔符、注释与首尾空白必须加引号
    expect_eq(needsQuoting(" leading"), true);
    expect_eq(needsQuoting("trailing "), true);
    expect_eq(needsQuoting("key: value"), true);
    expect_eq(needsQuoting("val # comment"), true);
    expect_eq(needsQuoting("[1, 2]"), true);
    expect_eq(needsQuoting("{a: 1}"), true);
    expect_eq(needsQuoting("&anchor"), true);
    expect_eq(needsQuoting("*alias"), true);
    expect_eq(needsQuoting("#comment"), true);
    expect_eq(needsQuoting("line1\nline2"), true);
}

test("yaml_scalar: formatSafeScalar round-trip with unescapeDoubleQuoted and buffer guard") {
    let out1: [128]u8 = [0; 128];
    let out2: [128]u8 = [0; 128];

    // 安全字符串原样输出
    const plain = formatSafeScalar("simple-name", out1[0..]).?;
    expect_eq(std.mem.eql(u8, plain, "simple-name"), true);

    // 含特殊字符与关键字自动加引号并可精确往返解转义
    const orig = "line1\n\t\"quote\"\\end";
    const formatted = formatSafeScalar(orig, out1[0..]).?;
    expect_eq(std.mem.eql(u8, formatted, "\"line1\\n\\t\\\"quote\\\"\\\\end\""), true);

    const roundtrip = unescapeDoubleQuoted(formatted, out2[0..]).?;
    expect_eq(std.mem.eql(u8, roundtrip, orig), true);

    // 关键字加引号
    const kw = formatSafeScalar("true", out1[0..]).?;
    expect_eq(std.mem.eql(u8, kw, "\"true\""), true);

    // 小缓冲区溢出防护
    let tiny: [3]u8 = [0; 3];
    expect_eq(formatSafeScalar("hello", tiny[0..]) == null, true);
    expect_eq(formatSafeScalar("true", tiny[0..]) == null, true);
}
