import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SpdxTokenKind,
    SpdxLexer,
    isValidSpdxExpression,
    SpdxIdIter,
    isCommonPermissive,
} from "cap:pkg";

test("1. SpdxLexer distinguishes strict uppercase operators, + suffixes, and LicenseRef", () => {
    let lex = SpdxLexer.init("(GPL-2.0+ OR Apache-2.0 WITH LLVM-exception) AND LicenseRef-Custom-1");

    expect_eq((lex.next() ?? unreachable).kind == SpdxTokenKind.lparen, true);
    const t1 = lex.next() ?? unreachable;
    expect_eq(t1.kind == SpdxTokenKind.ident, true);
    expect_eq(std.mem.eql(u8, t1.text, "GPL-2.0+"), true);
    expect_eq((lex.next() ?? unreachable).kind == SpdxTokenKind.or_op, true);
    expect_eq(std.mem.eql(u8, (lex.next() ?? unreachable).text, "Apache-2.0"), true);
    expect_eq((lex.next() ?? unreachable).kind == SpdxTokenKind.with_op, true);
    expect_eq(std.mem.eql(u8, (lex.next() ?? unreachable).text, "LLVM-exception"), true);
    expect_eq((lex.next() ?? unreachable).kind == SpdxTokenKind.rparen, true);
    expect_eq((lex.next() ?? unreachable).kind == SpdxTokenKind.and_op, true);
    const t_ref = lex.next() ?? unreachable;
    expect_eq(t_ref.kind == SpdxTokenKind.ident, true);
    expect_eq(std.mem.eql(u8, t_ref.text, "LicenseRef-Custom-1"), true);
    expect_eq(lex.next() == null, true);
});

test("2. isValidSpdxExpression accepts valid simple, compound, nested, WITH, and DocumentRef expressions", () => {
    expect_eq(isValidSpdxExpression("MIT"), true);
    expect_eq(isValidSpdxExpression("GPL-2.0+"), true);
    expect_eq(isValidSpdxExpression("MIT OR Apache-2.0"), true);
    expect_eq(isValidSpdxExpression("(MIT AND (BSD-3-Clause OR ISC))"), true);
    expect_eq(isValidSpdxExpression("Apache-2.0 WITH LLVM-exception"), true);
    expect_eq(isValidSpdxExpression("(Apache-2.0 WITH LLVM-exception) OR MIT"), true);
    expect_eq(isValidSpdxExpression("DocumentRef-spdx-tool-1.2:LicenseRef-MIT-Style-2"), true);
});

test("3. isValidSpdxExpression rejects malformed syntax, lowercase operators, and invalid WITH placement", () => {
    expect_eq(isValidSpdxExpression(""), false);
    expect_eq(isValidSpdxExpression("   "), false);
    expect_eq(isValidSpdxExpression("()"), false);
    expect_eq(isValidSpdxExpression("(MIT"), false);
    expect_eq(isValidSpdxExpression("MIT)"), false);
    expect_eq(isValidSpdxExpression("MIT AND"), false);
    expect_eq(isValidSpdxExpression("OR MIT"), false);
    expect_eq(isValidSpdxExpression("MIT AND OR ISC"), false);
    // 小写 and/or 不是运算符，导致相邻两个 ident 语法非法
    expect_eq(isValidSpdxExpression("MIT or Apache-2.0"), false);
    expect_eq(isValidSpdxExpression("MIT and ISC"), false);
    // WITH 左侧不能是括号复合表达式，右侧不能带 +
    expect_eq(isValidSpdxExpression("(MIT OR Apache-2.0) WITH LLVM-exception"), false);
    expect_eq(isValidSpdxExpression("Apache-2.0 WITH LLVM-exception+"), false);
    expect_eq(isValidSpdxExpression("Apache-2.0 WITH"), false);
    // 非法字符
    expect_eq(isValidSpdxExpression("MIT & ISC"), false);
    expect_eq(isValidSpdxExpression("MIT / ISC"), false);
});

test("4. SpdxIdIter extracts all license IDs while skipping operators and exception IDs", () => {
    const expr = "(MIT OR Apache-2.0 WITH LLVM-exception) AND GPL-3.0-only WITH Classpath-exception-2.0";
    let it = SpdxIdIter.init(expr);

    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "MIT"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "Apache-2.0"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "GPL-3.0-only"), true);
    expect_eq(it.next() == null, true);
});

test("5. isCommonPermissive identifies all 10 permissive licenses and excludes copyleft", () => {
    const permissive: [10][]const u8 = [
        "MIT",
        "ISC",
        "BSD-2-Clause",
        "BSD-3-Clause",
        "Apache-2.0",
        "0BSD",
        "Unlicense",
        "CC0-1.0",
        "Zlib",
        "BlueOak-1.0.0",
    ];
    for (id of permissive) {
        expect_eq(isCommonPermissive(id), true);
    }

    expect_eq(isCommonPermissive("GPL-3.0-only"), false);
    expect_eq(isCommonPermissive("AGPL-3.0-or-later"), false);
    expect_eq(isCommonPermissive("LGPL-2.1"), false);
    expect_eq(isCommonPermissive("MPL-2.0"), false);
    expect_eq(isCommonPermissive("mit"), false);
});
