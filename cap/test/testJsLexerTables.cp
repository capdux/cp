// cap/test/testJsLexerTables.cp
// Production-grade deep tests for cap:js lexerTables.cp
// (JS/TS Lexer Token Taxonomy, Keyword/Strict-Mode Tables & JSX Named Entities)

import { test, assert, assertEq } from "cap:test";
import {
    JsToken,
    lookupJsKeyword,
    isStrictModeReserved,
    remapStrictModeReserved,
    JsPropertyModifier,
    isTsAccessibilityModifier,
    lookupJsxEntity,
} from "cap:js";

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("JsToken: isAssign, isReservedWord, isString, isCloseBraceOrEOF & asStr", () => {
    assert(JsToken.t_end_of_file.isCloseBraceOrEOF());
    assert(JsToken.t_close_brace.isCloseBraceOrEOF());
    assert(!JsToken.t_open_brace.isCloseBraceOrEOF());

    // Assignment range boundaries
    assert(JsToken.t_ampersand_ampersand_equals.isAssign());
    assert(JsToken.t_equals.isAssign());
    assert(JsToken.t_question_question_equals.isAssign());
    assert(JsToken.t_slash_equals.isAssign());
    assert(!JsToken.t_equals_equals.isAssign());
    assert(!JsToken.t_equals_greater_than.isAssign());

    // String & template tokens
    assert(JsToken.t_string_literal.isString());
    assert(JsToken.t_no_substitution_template_literal.isString());
    assert(JsToken.t_template_head.isString());
    assert(JsToken.t_template_middle.isString());
    assert(JsToken.t_template_tail.isString());
    assert(!JsToken.t_numeric_literal.isString());

    assert(eqStr(JsToken.t_question_dot.asStr(), "?."));
    assert(eqStr(JsToken.t_dot_dot_dot.asStr(), "..."));
    assert(eqStr(JsToken.t_end_of_file.asStr(), "end of file"));
});

test("lookupJsKeyword: exact case-sensitive match & non-keyword rejection", () => {
    if (t1 := lookupJsKeyword("break")) {
        assert(t1 == JsToken.t_break);
        assert(t1.isReservedWord());
    } else {
        assert(false);
    }

    if (t2 := lookupJsKeyword("instanceof")) {
        assert(t2 == JsToken.t_instanceof);
        assert(eqStr(t2.asStr(), "instanceof"));
    } else {
        assert(false);
    }

    if (t3 := lookupJsKeyword("with")) {
        assert(t3 == JsToken.t_with);
        assert(t3.isReservedWord());
    } else {
        assert(false);
    }

    // Case sensitivity and non-keywords
    assert(lookupJsKeyword("Break") == null);
    assert(lookupJsKeyword("let") == null); // strict-mode reserved, not unconditional keyword
    assert(lookupJsKeyword("async") == null);
    assert(lookupJsKeyword("") == null);
    assert(lookupJsKeyword("veryLongIdentifier") == null);
});

test("isStrictModeReserved & remapStrictModeReserved: all 9 strict-mode reserved words", () => {
    assert(isStrictModeReserved("implements"));
    assert(isStrictModeReserved("interface"));
    assert(isStrictModeReserved("let"));
    assert(isStrictModeReserved("package"));
    assert(isStrictModeReserved("private"));
    assert(isStrictModeReserved("protected"));
    assert(isStrictModeReserved("public"));
    assert(isStrictModeReserved("static"));
    assert(isStrictModeReserved("yield"));
    assert(!isStrictModeReserved("const"));
    assert(!isStrictModeReserved("foo"));

    if (r1 := remapStrictModeReserved("implements")) {
        assert(eqStr(r1, "_implements"));
    } else {
        assert(false);
    }
    if (r2 := remapStrictModeReserved("yield")) {
        assert(eqStr(r2, "_yield"));
    } else {
        assert(false);
    }
    assert(remapStrictModeReserved("normalIdent") == null);
});

test("JsPropertyModifier & isTsAccessibilityModifier: class member modifier classification", () => {
    if (m1 := JsPropertyModifier.parse("readonly")) {
        assert(m1 == JsPropertyModifier.p_readonly);
        assert(m1.isTsAccessibility());
    } else {
        assert(false);
    }

    if (m2 := JsPropertyModifier.parse("accessor")) {
        assert(m2 == JsPropertyModifier.p_accessor);
        assert(!m2.isTsAccessibility());
    } else {
        assert(false);
    }

    assert(isTsAccessibilityModifier("override"));
    assert(isTsAccessibilityModifier("private"));
    assert(isTsAccessibilityModifier("protected"));
    assert(isTsAccessibilityModifier("public"));
    assert(isTsAccessibilityModifier("readonly"));
    assert(!isTsAccessibilityModifier("static"));
    assert(!isTsAccessibilityModifier("async"));
    assert(!isTsAccessibilityModifier("unknown"));
});

test("lookupJsxEntity: XML 5 entities, typographic symbols & unknown entity rejection", () => {
    assertEq(lookupJsxEntity("amp") ?? 0, @as(u32, 0x0026));
    assertEq(lookupJsxEntity("lt") ?? 0, @as(u32, 0x003C));
    assertEq(lookupJsxEntity("gt") ?? 0, @as(u32, 0x003E));
    assertEq(lookupJsxEntity("quot") ?? 0, @as(u32, 0x0022));
    assertEq(lookupJsxEntity("apos") ?? 0, @as(u32, 0x0027));
    assertEq(lookupJsxEntity("nbsp") ?? 0, @as(u32, 0x00A0));
    assertEq(lookupJsxEntity("mdash") ?? 0, @as(u32, 0x2014));
    assertEq(lookupJsxEntity("hellip") ?? 0, @as(u32, 0x2026));
    assertEq(lookupJsxEntity("rarr") ?? 0, @as(u32, 0x2192));
    assertEq(lookupJsxEntity("euro") ?? 0, @as(u32, 0x20AC));

    assert(lookupJsxEntity("AMP") == null);
    assert(lookupJsxEntity("nonexistent") == null);
    assert(lookupJsxEntity("") == null);
});
