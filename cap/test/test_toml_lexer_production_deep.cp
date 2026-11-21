import { test, expect_eq } from "cap:test";
import { TomlTokenKind, TomlLexer } from "cap:toml_lexer";

test("1. [package], [[bin]], dotted keys, inline table, comments & line/col tracking", () => {
    const src =
        "# header comment\n"
        ++ "[package]\n"
        ++ "a.b = { ok = true }\n"
        ++ "[[bin]]\n";

    let lex = TomlLexer.init(src);
    expect_eq(lex.next().kind == TomlTokenKind.newline, true);

    const t_lb = lex.next();
    expect_eq(t_lb.kind == TomlTokenKind.lbracket, true);
    expect_eq(t_lb.line, u32(2));
    expect_eq(t_lb.col, u32(1));

    const t_pkg = lex.next();
    expect_eq(t_pkg.kind == TomlTokenKind.bare_key, true);
    expect_eq(std.mem.eql(u8, t_pkg.raw, "package"), true);
    expect_eq(lex.next().kind == TomlTokenKind.rbracket, true);
    expect_eq(lex.next().kind == TomlTokenKind.newline, true);

    // a.b = { ok = true }
    expect_eq(std.mem.eql(u8, lex.next().raw, "a"), true);
    expect_eq(lex.next().kind == TomlTokenKind.dot, true);
    expect_eq(std.mem.eql(u8, lex.next().raw, "b"), true);
    expect_eq(lex.next().kind == TomlTokenKind.equals, true);
    expect_eq(lex.next().kind == TomlTokenKind.lbrace, true);
    expect_eq(std.mem.eql(u8, lex.next().raw, "ok"), true);
    expect_eq(lex.next().kind == TomlTokenKind.equals, true);
    const t_bool = lex.next();
    expect_eq(t_bool.kind == TomlTokenKind.boolean, true);
    expect_eq(t_bool.bool_val, true);
    expect_eq(lex.next().kind == TomlTokenKind.rbrace, true);
    expect_eq(lex.next().kind == TomlTokenKind.newline, true);

    // [[bin]]
    expect_eq(lex.next().kind == TomlTokenKind.double_lbracket, true);
    expect_eq(std.mem.eql(u8, lex.next().raw, "bin"), true);
    expect_eq(lex.next().kind == TomlTokenKind.double_rbracket, true);
    expect_eq(lex.next().kind == TomlTokenKind.newline, true);
    expect_eq(lex.next().kind == TomlTokenKind.eof, true);
});

test("2. All 4 TOML string forms (basic, multiline basic, literal, multiline literal)", () => {
    const src =
        "\"hello \\\"world\\\"\"\n"
        ++ "\"\"\"multi\nline\"\"\"\n"
        ++ "'C:\\Users\\raw'\n"
        ++ "'''raw\nmulti'''\n";

    let lex = TomlLexer.init(src);
    const s1 = lex.next();
    expect_eq(s1.kind == TomlTokenKind.basic_string, true);
    expect_eq(lex.next().kind == TomlTokenKind.newline, true);

    const s2 = lex.next();
    expect_eq(s2.kind == TomlTokenKind.multiline_basic_string, true);
    expect_eq(lex.next().kind == TomlTokenKind.newline, true);

    const s3 = lex.next();
    expect_eq(s3.kind == TomlTokenKind.literal_string, true);
    expect_eq(lex.next().kind == TomlTokenKind.newline, true);

    const s4 = lex.next();
    expect_eq(s4.kind == TomlTokenKind.multiline_literal_string, true);
});

test("3. Decimal, hex (0x), octal (0o), binary (0b) integers with underscore separators", () => {
    const src = "1_000_000 -42 0xde_ad 0o7_55 0b1010_0101";
    let lex = TomlLexer.init(src);

    const n1 = lex.next();
    expect_eq(n1.kind == TomlTokenKind.integer, true);
    expect_eq(n1.int_val, i64(1000000));

    const n2 = lex.next();
    expect_eq(n2.kind == TomlTokenKind.integer, true);
    expect_eq(n2.int_val, i64(-42));

    const n3 = lex.next();
    expect_eq(n3.kind == TomlTokenKind.integer, true);
    expect_eq(n3.int_val, i64(0xdead));

    const n4 = lex.next();
    expect_eq(n4.kind == TomlTokenKind.integer, true);
    expect_eq(n4.int_val, i64(493));

    const n5 = lex.next();
    expect_eq(n5.kind == TomlTokenKind.integer, true);
    expect_eq(n5.int_val, i64(165));
});

test("4. Floats (exponent, inf, -inf, nan) & ISO 8601 datetimes", () => {
    const src = "3.14_15 1e-5 inf -inf nan 2026-11-22T01:00:00Z 2026-11-22 08:30:00";
    let lex = TomlLexer.init(src);

    expect_eq(lex.next().kind == TomlTokenKind.float_val, true);
    expect_eq(lex.next().kind == TomlTokenKind.float_val, true);
    expect_eq(lex.next().kind == TomlTokenKind.float_val, true);
    expect_eq(lex.next().kind == TomlTokenKind.float_val, true);
    expect_eq(lex.next().kind == TomlTokenKind.float_val, true);

    const dt1 = lex.next();
    expect_eq(dt1.kind == TomlTokenKind.datetime, true);
    expect_eq(std.mem.eql(u8, dt1.raw, "2026-11-22T01:00:00Z"), true);

    const dt2 = lex.next();
    expect_eq(dt2.kind == TomlTokenKind.datetime, true);
    expect_eq(std.mem.eql(u8, dt2.raw, "2026-11-22"), true);

    const dt3 = lex.next();
    expect_eq(dt3.kind == TomlTokenKind.datetime, true);
    expect_eq(std.mem.eql(u8, dt3.raw, "08:30:00"), true);
});

test("5. Unclosed strings & invalid radix/underscore numbers yield .invalid safely", () => {
    let l1 = TomlLexer.init("\"unclosed");
    expect_eq(l1.next().kind == TomlTokenKind.invalid, true);

    let l2 = TomlLexer.init("'unclosed\nnext");
    expect_eq(l2.next().kind == TomlTokenKind.invalid, true);

    let l3 = TomlLexer.init("0xGG 0b102 1__0");
    expect_eq(l3.next().kind == TomlTokenKind.invalid, true);
    expect_eq(l3.next().kind == TomlTokenKind.invalid, true);
    expect_eq(l3.next().kind == TomlTokenKind.invalid, true);
});
