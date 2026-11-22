// Source: cp/cap/test/test_kdl_production_deep.cp (Production deep tests for cap:kdl)

import { test, expect_eq } from "cap:test";
import {
    KdlTokenKind,
    KdlToken,
    KdlIter,
    KdlNodeSummary,
    KdlNodeIter,
    validateKdl,
} from "cap:conf";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("1. KdlIter: BOM, nested block comments, raw strings, numbers & line continuations") {
    const src =
        "\xEF\xBB\xBF/* outer /* inner */ end */\n" ++
        "server \"host\\u{20}1\" r#\"raw \"quote\"\"# \\\n" ++
        "  1_000 0xff_00 0b1010_0101 0o755 true false null key=3.14e-2 // trailing\n";

    let it = KdlIter.init(src);
    expect_eq(it.next().?.kind == .newline, true);

    const n = it.next().?;
    expect_eq(n.kind == .ident, true);
    expect_eq(strEq(n.lexeme, "server"), true);

    const s1 = it.next().?;
    expect_eq(s1.kind == KdlTokenKind.string, true);

    const s2 = it.next().?;
    expect_eq(s2.kind == KdlTokenKind.string, true);
    expect_eq(strEq(s2.lexeme, "r#\"raw \"quote\"\"#"), true);

    const num1 = it.next().?;
    expect_eq(num1.kind == .number, true);
    expect_eq(strEq(num1.lexeme, "1_000"), true);

    const num_hex = it.next().?;
    expect_eq(num_hex.kind == .number, true);
    expect_eq(strEq(num_hex.lexeme, "0xff_00"), true);

    const num_bin = it.next().?;
    expect_eq(num_bin.kind == .number, true);
    expect_eq(strEq(num_bin.lexeme, "0b1010_0101"), true);

    const num_oct = it.next().?;
    expect_eq(num_oct.kind == .number, true);
    expect_eq(strEq(num_oct.lexeme, "0o755"), true);

    expect_eq(it.next().?.kind == .bool_lit, true);
    expect_eq(it.next().?.kind == .bool_lit, true);
    expect_eq(it.next().?.kind == .null_lit, true);
    expect_eq(it.next().?.kind == .ident, true); // key
    expect_eq(it.next().?.kind == .equals, true);
    expect_eq(it.next().?.kind == .number, true); // 3.14e-2
    expect_eq(it.next().?.kind == .newline, true);
    expect_eq(it.next() == null, true);
}

test("2. KdlNodeIter: nested hierarchy, arg/prop counting and semicolon-separated nodes") {
    const doc =
        "package \"cap\" version=\"1.0.0\" edition=2026 {\n" ++
        "  dependencies {\n" ++
        "    dep \"json5\" optional=true; dep \"kdl\" 2\n" ++
        "  }\n" ++
        "  scripts build=\"cap build\"\n" ++
        "}\n";

    expect_eq(validateKdl(doc), true);

    let it = KdlNodeIter.init(doc);

    const n0 = it.next().?;
    expect_eq(strEq(n0.name, "package"), true);
    expect_eq(n0.arg_count, @as(u16, 1));
    expect_eq(n0.prop_count, @as(u16, 2));
    expect_eq(n0.has_children, true);
    expect_eq(n0.depth, @as(u16, 0));

    const n1 = it.next().?;
    expect_eq(strEq(n1.name, "dependencies"), true);
    expect_eq(n1.arg_count, @as(u16, 0));
    expect_eq(n1.prop_count, @as(u16, 0));
    expect_eq(n1.has_children, true);
    expect_eq(n1.depth, @as(u16, 1));

    const n2 = it.next().?;
    expect_eq(strEq(n2.name, "dep"), true);
    expect_eq(n2.arg_count, @as(u16, 1));
    expect_eq(n2.prop_count, @as(u16, 1));
    expect_eq(n2.has_children, false);
    expect_eq(n2.depth, @as(u16, 2));

    const n3 = it.next().?;
    expect_eq(strEq(n3.name, "dep"), true);
    expect_eq(n3.arg_count, @as(u16, 2));
    expect_eq(n3.prop_count, @as(u16, 0));
    expect_eq(n3.depth, @as(u16, 2));

    const n4 = it.next().?;
    expect_eq(strEq(n4.name, "scripts"), true);
    expect_eq(n4.arg_count, @as(u16, 0));
    expect_eq(n4.prop_count, @as(u16, 1));
    expect_eq(n4.depth, @as(u16, 1));

    expect_eq(it.next() == null, true);
}

test("3. KdlNodeIter: slashdash /- skips nodes, args, props and child blocks") {
    const doc =
        "/- commented-node \"ignored\" {\n" ++
        "  hidden-child 1\n" ++
        "}\n" ++
        "active \"kept\" /- \"skipped-arg\" k1=1 /- k2=2 /- {\n" ++
        "  skipped-child\n" ++
        "}\n" ++
        "tail 42\n";

    expect_eq(validateKdl(doc), true);

    let it = KdlNodeIter.init(doc);
    const n0 = it.next().?;
    expect_eq(strEq(n0.name, "active"), true);
    expect_eq(n0.arg_count, @as(u16, 1));
    expect_eq(n0.prop_count, @as(u16, 1));
    expect_eq(n0.has_children, false);
    expect_eq(n0.depth, @as(u16, 0));

    const n1 = it.next().?;
    expect_eq(strEq(n1.name, "tail"), true);
    expect_eq(n1.arg_count, @as(u16, 1));
    expect_eq(n1.depth, @as(u16, 0));

    expect_eq(it.next() == null, true);
}

test("4. Quoted node names, empty documents, and comment-only documents") {
    expect_eq(validateKdl(""), true);
    expect_eq(validateKdl("   \n\t// only comment\n/* block */"), true);

    let empty_it = KdlNodeIter.init("  // nothing here\n");
    expect_eq(empty_it.next() == null, true);

    const quoted_doc = "\"my node\" \"arg\" \"prop key\"=\"val\"\n";
    expect_eq(validateKdl(quoted_doc), true);

    let it = KdlNodeIter.init(quoted_doc);
    const n = it.next().?;
    expect_eq(strEq(n.name, "\"my node\""), true);
    expect_eq(n.arg_count, @as(u16, 1));
    expect_eq(n.prop_count, @as(u16, 1));
    expect_eq(it.next() == null, true);
}

test("5. Malformed KDL rejection: unbalanced braces, unclosed comments/strings, bad numbers & deep nesting") {
    // Unbalanced braces
    expect_eq(validateKdl("node { child 1"), false);
    expect_eq(validateKdl("node 1 }"), false);
    expect_eq(validateKdl("{ orphan }"), false);

    // Unclosed nested block comment & strings
    expect_eq(validateKdl("/* outer /* inner */"), false);
    expect_eq(validateKdl("node \"unterminated"), false);
    expect_eq(validateKdl("node r##\"unterminated\"#"), false);
    expect_eq(validateKdl("node \"bad\\qescape\""), false);

    // Malformed numbers & line continuations & dangling operators
    expect_eq(validateKdl("node 0x"), false);
    expect_eq(validateKdl("node 0b102"), false);
    expect_eq(validateKdl("node 1."), false);
    expect_eq(validateKdl("node 1e"), false);
    expect_eq(validateKdl("node \\ trailing"), false);
    expect_eq(validateKdl("node key="), false);
    expect_eq(validateKdl("=val"), false);
    expect_eq(validateKdl("node /-"), false);

    // Deep nesting guard (> 64)
    let deep: [520]u8 = [0; 520];
    let pos: usize = 0;
    let i: usize = 0;
    while (i < 66) : (i += 1) {
        deep[pos] = 'a';
        deep[pos + 1] = ' ';
        deep[pos + 2] = '{';
        deep[pos + 3] = '\n';
        pos += 4;
    }
    i = 0;
    while (i < 66) : (i += 1) {
        deep[pos] = '}';
        deep[pos + 1] = '\n';
        pos += 2;
    }
    expect_eq(validateKdl(deep[0..pos]), false);
}
