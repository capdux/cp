// Origin: Pure-CP Stdlib Test Suite (lib/js/foldString.cp) | Created: 2026-11-22
// Production Deep Verification Suite for JS/TS Compile-Time String & Template Addition Folding:
//   1. String + String & side-effect-free primitive coercion (number/bool/null/undefined/MIN_I64)
//   2. NestedLeft associativity guard: rejects `1 + "y"` in `(a + 1) + "y"`, allows `"x" + "y"`
//   3. InlinedEnumString unwrap without mutating shared enum literals
//   4. Template literal folding (string+tpl, tpl+string, tpl+number, tpl+tpl part concatenation)
//   5. Tagged template guard (never folds tagged templates) & simple template-to-string lowering
//   6. Empty-string identity elimination (known-string only) & fixed-buffer overflow protection

import { test, expect_eq, expect_true } from "cap:test";
import {
    FOLD_MAX_STR_LEN,
    FoldStringKind,
    JsPrimitiveType,
    JsFoldExprKind,
    JsFoldExpr,
    formatJsInt,
    foldStringAddition,
} from "../../lib/js/foldString.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

// ============================================================================
// Test 1: String + String & side-effect-free primitive coercion
// ============================================================================
test("foldString: string concat & primitive coercion (int, bool, null, undefined)", () => {
    const s_foo = JsFoldExpr.stringLit("foo") ?? JsFoldExpr.undefinedLit();
    const s_bar = JsFoldExpr.stringLit("bar") ?? JsFoldExpr.undefinedLit();

    if (res := foldStringAddition(s_foo, s_bar, FoldStringKind.Normal)) {
        expect_true(res.kind == JsFoldExprKind.StringLit);
        expect_true(strEq(res.strSlice(), "foobar"));
    } else {
        expect_true(false);
    }

    // "x" + 123 => "x123"
    const s_x = JsFoldExpr.stringLit("x") ?? JsFoldExpr.undefinedLit();
    if (res := foldStringAddition(s_x, JsFoldExpr.numberInt(123), FoldStringKind.Normal)) {
        expect_true(strEq(res.strSlice(), "x123"));
    } else {
        expect_true(false);
    }

    // -42 + "px" => "-42px"
    const s_px = JsFoldExpr.stringLit("px") ?? JsFoldExpr.undefinedLit();
    if (res := foldStringAddition(JsFoldExpr.numberInt(-42), s_px, FoldStringKind.Normal)) {
        expect_true(strEq(res.strSlice(), "-42px"));
    } else {
        expect_true(false);
    }

    // true + "!" => "true!", null + "?" => "null?", undefined + "!" => "undefined!"
    const s_bang = JsFoldExpr.stringLit("!") ?? JsFoldExpr.undefinedLit();
    if (res := foldStringAddition(JsFoldExpr.boolLit(true), s_bang, FoldStringKind.Normal)) {
        expect_true(strEq(res.strSlice(), "true!"));
    } else {
        expect_true(false);
    }
    if (res := foldStringAddition(JsFoldExpr.nullLit(), s_bang, FoldStringKind.Normal)) {
        expect_true(strEq(res.strSlice(), "null!"));
    } else {
        expect_true(false);
    }
    if (res := foldStringAddition(JsFoldExpr.undefinedLit(), s_bang, FoldStringKind.Normal)) {
        expect_true(strEq(res.strSlice(), "undefined!"));
    } else {
        expect_true(false);
    }

    // formatJsInt edge cases: 0 and i64 minimum (-9223372036854775808)
    let buf: [32]u8 = [0; 32];
    if (z := formatJsInt(0, buf[0..])) {
        expect_true(strEq(z, "0"));
    } else {
        expect_true(false);
    }
    const min_i64: i64 = -9223372036854775807 - 1;
    if (m := formatJsInt(min_i64, buf[0..])) {
        expect_true(strEq(m, "-9223372036854775808"));
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 2: NestedLeft associativity guard
// ============================================================================
test("foldString: NestedLeft prevents folding (a + 1) + 'y' while allowing (a + 'x') + 'y'", () => {
    const s_y = JsFoldExpr.stringLit("y") ?? JsFoldExpr.undefinedLit();
    const s_x = JsFoldExpr.stringLit("x") ?? JsFoldExpr.undefinedLit();

    // Normal mode: 1 + "y" => "1y"
    if (res := foldStringAddition(JsFoldExpr.numberInt(1), s_y, FoldStringKind.Normal)) {
        expect_true(strEq(res.strSlice(), "1y"));
    } else {
        expect_true(false);
    }

    // NestedLeft mode: 1 + "y" MUST NOT fold (could be numeric `(a + 1) + "y"`)
    expect_true(foldStringAddition(JsFoldExpr.numberInt(1), s_y, FoldStringKind.NestedLeft) == null);

    // NestedLeft mode: "x" + "y" IS safe to fold into "xy"
    if (res := foldStringAddition(s_x, s_y, FoldStringKind.NestedLeft)) {
        expect_true(strEq(res.strSlice(), "xy"));
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 3: InlinedEnumString non-mutating unwrap
// ============================================================================
test("foldString: InlinedEnumString unwraps cleanly without mutating shared enum", () => {
    const enum_b = JsFoldExpr.inlinedEnumString("ab") ?? JsFoldExpr.undefinedLit();
    const s_d = JsFoldExpr.stringLit("d") ?? JsFoldExpr.undefinedLit();
    const s_p = JsFoldExpr.stringLit("0") ?? JsFoldExpr.undefinedLit();

    if (r1 := foldStringAddition(enum_b, s_d, FoldStringKind.Normal)) {
        expect_true(strEq(r1.strSlice(), "abd"));
    } else {
        expect_true(false);
    }
    if (r2 := foldStringAddition(s_p, enum_b, FoldStringKind.Normal)) {
        expect_true(strEq(r2.strSlice(), "0ab"));
    } else {
        expect_true(false);
    }
    // Original enum literal remains intact
    expect_true(enum_b.kind == JsFoldExprKind.InlinedEnumString);
    expect_true(strEq(enum_b.strSlice(), "ab"));
});

// ============================================================================
// Test 4: Template literal folding (string+tpl, tpl+string, tpl+int, tpl+tpl)
// ============================================================================
test("foldString: template literal merging across string, int, and template operands", () => {
    // "bar" + `baz${1}` => `barbaz${1}end`
    let t1 = JsFoldExpr.templateLit("baz", false) ?? JsFoldExpr.undefinedLit();
    expect_true(t1.addTemplatePart(1, "end"));
    const s_bar = JsFoldExpr.stringLit("bar") ?? JsFoldExpr.undefinedLit();

    if (merged1 := foldStringAddition(s_bar, t1, FoldStringKind.Normal)) {
        expect_true(merged1.kind == JsFoldExprKind.TemplateLit);
        expect_true(strEq(merged1.templateHead(), "barbaz"));
        expect_eq(merged1.templatePartCount(), 1);
        if (p0 := merged1.templatePart(0)) {
            expect_eq(p0.expr_id, 1);
            expect_true(strEq(p0.tailSlice(), "end"));
        } else {
            expect_true(false);
        }
    } else {
        expect_true(false);
    }

    // `foo${1}` + 0 => `foo${1}0`
    let t2 = JsFoldExpr.templateLit("foo", false) ?? JsFoldExpr.undefinedLit();
    expect_true(t2.addTemplatePart(1, ""));
    if (merged2 := foldStringAddition(t2, JsFoldExpr.numberInt(0), FoldStringKind.Normal)) {
        expect_true(strEq(merged2.templateHead(), "foo"));
        if (p0 := merged2.templatePart(0)) {
            expect_true(strEq(p0.tailSlice(), "0"));
        } else {
            expect_true(false);
        }
    } else {
        expect_true(false);
    }

    // `foo${1}x` + `a${2}b` => `foo${1}xa${2}b`
    let t_left = JsFoldExpr.templateLit("foo", false) ?? JsFoldExpr.undefinedLit();
    expect_true(t_left.addTemplatePart(1, "x"));
    let t_right = JsFoldExpr.templateLit("a", false) ?? JsFoldExpr.undefinedLit();
    expect_true(t_right.addTemplatePart(2, "b"));

    if (merged3 := foldStringAddition(t_left, t_right, FoldStringKind.Normal)) {
        expect_true(strEq(merged3.templateHead(), "foo"));
        expect_eq(merged3.templatePartCount(), 2);
        if (p0 := merged3.templatePart(0)) {
            expect_eq(p0.expr_id, 1);
            expect_true(strEq(p0.tailSlice(), "xa"));
        } else {
            expect_true(false);
        }
        if (p1 := merged3.templatePart(1)) {
            expect_eq(p1.expr_id, 2);
            expect_true(strEq(p1.tailSlice(), "b"));
        } else {
            expect_true(false);
        }
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 5: Tagged template guard & simple template-to-string lowering
// ============================================================================
test("foldString: tagged templates never fold & simple template simplifies to string", () => {
    let tagged = JsFoldExpr.templateLit("raw", true) ?? JsFoldExpr.undefinedLit();
    expect_true(tagged.addTemplatePart(7, "!"));
    const s_pre = JsFoldExpr.stringLit("pre") ?? JsFoldExpr.undefinedLit();

    // Neither side may fold with a tagged template
    expect_true(foldStringAddition(s_pre, tagged, FoldStringKind.Normal) == null);
    expect_true(foldStringAddition(tagged, s_pre, FoldStringKind.Normal) == null);

    // Simple untagged template with 0 parts simplifies to StringLit
    const simple_tpl = JsFoldExpr.templateLit("Hello World", false) ?? JsFoldExpr.undefinedLit();
    if (lowered := simple_tpl.simplifyTemplateToString()) {
        expect_true(lowered.kind == JsFoldExprKind.StringLit);
        expect_true(strEq(lowered.strSlice(), "Hello World"));
    } else {
        expect_true(false);
    }

    // Tagged template must NOT simplify
    const tagged_no_parts = JsFoldExpr.templateLit("Hello", true) ?? JsFoldExpr.undefinedLit();
    expect_true(tagged_no_parts.simplifyTemplateToString() == null);
});

// ============================================================================
// Test 6: Empty-string identity elimination & buffer overflow guard
// ============================================================================
test("foldString: empty-string identity elimination (known string only) & overflow guard", () => {
    const empty = JsFoldExpr.stringLit("") ?? JsFoldExpr.undefinedLit();
    const dyn_str = JsFoldExpr.dynamicExpr(42, JsPrimitiveType.String);
    const dyn_unk = JsFoldExpr.dynamicExpr(99, JsPrimitiveType.Unknown);

    // "" + dyn_str => dyn_str, dyn_str + "" => dyn_str
    if (r1 := foldStringAddition(empty, dyn_str, FoldStringKind.Normal)) {
        expect_true(r1.kind == JsFoldExprKind.DynamicExpr);
        expect_eq(r1.expr_id, 42);
    } else {
        expect_true(false);
    }
    if (r2 := foldStringAddition(dyn_str, empty, FoldStringKind.Normal)) {
        expect_true(r2.kind == JsFoldExprKind.DynamicExpr);
        expect_eq(r2.expr_id, 42);
    } else {
        expect_true(false);
    }

    // "" + dyn_unk MUST NOT eliminate (would drop ToString coercion on numbers/objects)
    expect_true(foldStringAddition(empty, dyn_unk, FoldStringKind.Normal) == null);
    expect_true(foldStringAddition(dyn_unk, empty, FoldStringKind.Normal) == null);

    // Fixed-buffer overflow protection: 300 + 300 > FOLD_MAX_STR_LEN (512) returns null safely
    let big_buf: [300]u8 = ['a'; 300];
    const big1 = JsFoldExpr.stringLit(big_buf[0..]) ?? JsFoldExpr.undefinedLit();
    const big2 = JsFoldExpr.stringLit(big_buf[0..]) ?? JsFoldExpr.undefinedLit();
    expect_true(FOLD_MAX_STR_LEN == 512);
    expect_true(foldStringAddition(big1, big2, FoldStringKind.Normal) == null);
});
