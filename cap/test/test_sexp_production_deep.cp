// Source: cp/cap/test/test_sexp_production_deep.cp (Production deep tests for cap:sexp)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SexpTokenKind,
    SexpToken,
    SexpIter,
    validateSexp,
    maxSexpDepth,
    formatSexpCompact,
} from "cap:sexp";

test("1. SexpIter tokenizes parens, atoms, quoted strings with escapes, and skips comments", () => {
    const src = " ; header comment\n (func $add (param \"a \\\"quoted\\\" b\") 42) ; tail";
    let it = SexpIter.init(src);

    const t0 = it.next() ?? unreachable;
    expect_eq(t0.kind == SexpTokenKind.lparen, true);
    expect_eq(std.mem.eql(u8, t0.lexeme, "("), true);

    const t1 = it.next() ?? unreachable;
    expect_eq(t1.kind == SexpTokenKind.atom, true);
    expect_eq(std.mem.eql(u8, t1.lexeme, "func"), true);

    const t2 = it.next() ?? unreachable;
    expect_eq(t2.kind == SexpTokenKind.atom, true);
    expect_eq(std.mem.eql(u8, t2.lexeme, "$add"), true);

    const t3 = it.next() ?? unreachable;
    expect_eq(t3.kind == SexpTokenKind.lparen, true);

    const t4 = it.next() ?? unreachable;
    expect_eq(t4.kind == SexpTokenKind.atom, true);
    expect_eq(std.mem.eql(u8, t4.lexeme, "param"), true);

    const t5 = it.next() ?? unreachable;
    expect_eq(t5.kind == SexpTokenKind.string, true);
    expect_eq(std.mem.eql(u8, t5.lexeme, "\"a \\\"quoted\\\" b\""), true);

    const t6 = it.next() ?? unreachable;
    expect_eq(t6.kind == SexpTokenKind.rparen, true);

    const t7 = it.next() ?? unreachable;
    expect_eq(t7.kind == SexpTokenKind.atom, true);
    expect_eq(std.mem.eql(u8, t7.lexeme, "42"), true);

    const t8 = it.next() ?? unreachable;
    expect_eq(t8.kind == SexpTokenKind.rparen, true);

    expect_eq(it.next() == null, true);
});

test("2. validateSexp & maxSexpDepth handle bare atoms, nested WAT, and sibling lists", () => {
    expect_eq(validateSexp("42"), true);
    expect_eq((maxSexpDepth("42") ?? 99) as i64, 0);

    expect_eq(validateSexp("()"), true);
    expect_eq((maxSexpDepth("()") ?? 99) as i64, 1);

    const wat = "(module (func $f (param i32) (result i32) (i32.add (local.get 0) (i32.const 1))))";
    expect_eq(validateSexp(wat), true);
    expect_eq((maxSexpDepth(wat) ?? 99) as i64, 4);

    // Multiple top-level S-expressions
    expect_eq(validateSexp("(a (b)) (c (d (e)))"), true);
    expect_eq((maxSexpDepth("(a (b)) (c (d (e)))") ?? 99) as i64, 3);
});

test("3. Malformed inputs: empty, comments-only, unbalanced parens, unclosed strings", () => {
    expect_eq(validateSexp(""), false);
    expect_eq(validateSexp("   \t\r\n  "), false);
    expect_eq(validateSexp("; only a comment\n; second comment"), false);
    expect_eq(maxSexpDepth("") == null, true);

    // Unbalanced parentheses
    expect_eq(validateSexp("(a b"), false);
    expect_eq(validateSexp("a b)"), false);
    expect_eq(validateSexp("((a)))"), false);
    expect_eq(validateSexp(")("), false);
    expect_eq(maxSexpDepth("(a (b)") == null, true);

    // Unclosed string and trailing backslash escape at EOF
    expect_eq(validateSexp("(msg \"unterminated)"), false);
    expect_eq(validateSexp("(msg \"trailing\\"), false);

    let it = SexpIter.init("\"unclosed");
    const bad = it.next() ?? unreachable;
    expect_eq(bad.kind == SexpTokenKind.invalid, true);
    expect_eq(it.next() == null, true);
});

test("4. formatSexpCompact strips comments/whitespace and preserves canonical spacing", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);

    const src = " ; WAT example\n ( module \n  ( func   $add  ( param  i32  i32 ) ( result  i32 ) )\n  ( export \"add\" ( func $add ) ) ) ";
    const out = formatSexpCompact(src, buf[0..]) ?? unreachable;
    expect_eq(
        std.mem.eql(u8, out, "(module (func $add (param i32 i32) (result i32)) (export \"add\" (func $add)))"),
        true,
    );

    // Adjacent nested parens have no inner space, sibling lists have single space
    const sib = "( ( a )   ( b ) )";
    const sib_out = formatSexpCompact(sib, buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, sib_out, "((a) (b))"), true);

    // Escaped spaces and parens inside string literals are preserved verbatim
    const str_src = "( log   \"(a   b)\"  \"x\\\\y\" )";
    const str_out = formatSexpCompact(str_src, buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, str_out, "(log \"(a   b)\" \"x\\\\y\")"), true);
});

test("5. formatSexpCompact buffer boundary & invalid input protection", () => {
    let small: [5]u8 = std.mem.zeroes([5]u8);
    // Exact fit: "(a b)" is 5 bytes
    const exact = formatSexpCompact(" ( a   b ) ", small[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, exact, "(a b)"), true);

    // 1 byte too small -> null
    expect_eq(formatSexpCompact(" ( a   b ) ", small[0..4]) == null, true);
    expect_eq(formatSexpCompact("(a b c)", small[0..]) == null, true);

    // Invalid S-expression returns null even with large buffer
    let big: [64]u8 = std.mem.zeroes([64]u8);
    expect_eq(formatSexpCompact("(a b", big[0..]) == null, true);
    expect_eq(formatSexpCompact("(a \"bad)", big[0..]) == null, true);
    expect_eq(formatSexpCompact("; comment only", big[0..]) == null, true);
});
