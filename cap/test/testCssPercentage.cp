// cap/test/testCssPercentage.cp
// Production-grade deep tests for cap:css percentage.cp
// (CSS Values L4 <percentage>, <number-or-percentage>, and zero-alloc <length-percentage>)

import { test, assert, assertEq } from "cap:test";
import {
    CssPercentageError,
    CssPercentage,
    CssNumberOrPercentageKind,
    CssNumberOrPercentage,
    CssLengthUnitTag,
    cssLengthUnitName,
    CssLengthOrPercentageKind,
    CssLengthOrPercentage,
} from "cap:css";

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn near(a: f64, b: f64): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= 0.000000001;
}

test("CssPercentage: parse, unit_value normalization, scientific notation & arithmetic", () => {
    const p50 = CssPercentage.parse("  50%  ") catch unreachable;
    assert(near(p50.unit_value, 0.5));
    assert(near(p50.toPercent(), 50.0));
    assert(!p50.isZero());
    assert(p50.isSignPositive());
    assert(!p50.isSignNegative());
    assert(near(p50.sign(), 1.0));
    assert(near(p50.resolve(240.0), 120.0));

    const p_sci = CssPercentage.parse("1.25e2%") catch unreachable;
    assert(near(p_sci.unit_value, 1.25));
    assert(near(p_sci.toPercent(), 125.0));

    const p_neg = CssPercentage.parse("-.5%") catch unreachable;
    assert(near(p_neg.unit_value, -0.005));
    assert(p_neg.isSignNegative());
    assert(near(p_neg.sign(), -1.0));

    const sum = p50.add(CssPercentage.fromPercent(25.0));
    assert(sum.eql(CssPercentage.fromPercent(75.0)));
    assertEq(p50.partialCmp(sum), -1);
    assertEq(sum.partialCmp(p50), 1);
    assertEq(sum.partialCmp(CssPercentage.fromUnitValue(0.75)), 0);

    const diff = p50.sub(CssPercentage.fromPercent(80.0));
    assert(diff.eql(CssPercentage.fromPercent(-30.0)));
    assert(diff.clampZeroOne().isZero());
    assert(p_sci.clampZeroOne().eql(CssPercentage.fromUnitValue(1.0)));

    const scaled = p50.mulF64(1.5);
    assert(scaled.eql(CssPercentage.fromPercent(75.0)));
});

test("CssPercentage: Bun-aligned minimal CSS serialization (-0, fractional leading zero)", () => {
    let buf: [64]u8 = [0; 64];

    // -0% normalizes to 0%
    const neg_zero = CssPercentage.parse("-0.00%") catch unreachable;
    assert(neg_zero.isZero());
    const n0 = neg_zero.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n0], "0%"));

    // 0 < |pct| < 1 omits leading 0 when minify=true, keeps 0 when minify=false
    const small_pos = CssPercentage.parse("0.5%") catch unreachable;
    const n1 = small_pos.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n1], ".5%"));
    const n2 = small_pos.toCss(false, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n2], "0.5%"));

    const small_neg = CssPercentage.parse("-0.2500%") catch unreachable;
    const n3 = small_neg.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n3], "-.25%"));
    const n4 = small_neg.toCss(false, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n4], "-0.25%"));

    // Whole numbers strip trailing fractional zeros
    const whole = CssPercentage.parse("+100.000%") catch unreachable;
    const n5 = whole.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n5], "100%"));
});

test("CssNumberOrPercentage: parse, intoF64 equivalence, clamping & serialization", () => {
    let buf: [64]u8 = [0; 64];

    const num = CssNumberOrPercentage.parse("0.5") catch unreachable;
    assert(num.isNumber());
    assert(!num.isPercentage());
    assert(near(num.intoF64(), 0.5));

    const pct = CssNumberOrPercentage.parse("50%") catch unreachable;
    assert(pct.isPercentage());
    assert(!pct.isNumber());
    assert(near(pct.intoF64(), 0.5));
    assert(!num.eql(pct)); // distinct syntactic kinds

    const n_num = num.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_num], ".5"));
    const n_pct = pct.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_pct], "50%"));

    const over = CssNumberOrPercentage.parse("150%") catch unreachable;
    const clamped = over.clampZeroOne();
    assert(clamped.isPercentage());
    assert(near(clamped.intoF64(), 1.0));
    const n_clamped = clamped.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_clamped], "100%"));
});

test("CssLengthOrPercentage: parse lengths, unitless zero & static calc folding", () => {
    let buf: [64]u8 = [0; 64];

    const z = CssLengthOrPercentage.parse("0") catch unreachable;
    assert(z.isZero());
    const nz = z.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..nz], "0"));

    // Ensure 'em' unit isn't misread as scientific exponent 'e'
    const em_val = CssLengthOrPercentage.parse("2.5em") catch unreachable;
    assert(em_val.kind == CssLengthOrPercentageKind.length);
    assert(em_val.length_unit == CssLengthUnitTag.em);
    assert(eqStr(cssLengthUnitName(em_val.length_unit), "em"));
    const nem = em_val.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..nem], "2.5em"));

    // Same-unit length addition folds directly without calc()
    const px1 = CssLengthOrPercentage.parse("10px") catch unreachable;
    const px2 = CssLengthOrPercentage.parse("14px") catch unreachable;
    if (px_sum := px1.tryAdd(px2)) {
        assert(px_sum.kind == CssLengthOrPercentageKind.length);
        const n = px_sum.toCss(true, buf[0..]) catch unreachable;
        assert(eqStr(buf[0..n], "24px"));
    } else {
        assert(false);
    }

    // Length + percentage folds into calc(), placing positive term first when length < 0 and pct > 0
    const neg_px = CssLengthOrPercentage.parse("-16px") catch unreachable;
    const full_pct = CssLengthOrPercentage.parse("100%") catch unreachable;
    if (mixed := neg_px.tryAdd(full_pct)) {
        assert(mixed.kind == CssLengthOrPercentageKind.calc_sum);
        const n = mixed.toCss(true, buf[0..]) catch unreachable;
        assert(eqStr(buf[0..n], "calc(100% - 16px)"));
    } else {
        assert(false);
    }

    // Incompatible length units return null
    const rem_val = CssLengthOrPercentage.parse("2rem") catch unreachable;
    if (bad := px1.tryAdd(rem_val)) {
        * = bad;
        assert(false);
    }
});

test("CssLengthOrPercentage: scaling, sign analysis, partialCmp & cancellation", () => {
    let buf: [64]u8 = [0; 64];

    const a = CssLengthOrPercentage.fromCalcSum(20.0, CssLengthUnitTag.px, CssPercentage.fromPercent(50.0));
    const doubled = a.mulF64(2.0);
    const nd = doubled.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..nd], "calc(40px + 100%)"));

    if (s := doubled.trySign()) {
        assert(near(s, 1.0));
    } else {
        assert(false);
    }

    // Mixed-sign calc_sum has indeterminate static sign
    const mixed = CssLengthOrPercentage.fromCalcSum(20.0, CssLengthUnitTag.px, CssPercentage.fromPercent(-50.0));
    if (s2 := mixed.trySign()) {
        * = s2;
        assert(false);
    }

    // Cancelling the length term collapses calc_sum back to pure percentage
    const cancel = CssLengthOrPercentage.fromLength(-20.0, CssLengthUnitTag.px);
    if (reduced := a.tryAdd(cancel)) {
        assert(reduced.kind == CssLengthOrPercentageKind.percentage);
        const nr = reduced.toCss(true, buf[0..]) catch unreachable;
        assert(eqStr(buf[0..nr], "50%"));
    } else {
        assert(false);
    }

    // partialCmp on same-unit lengths vs incompatible types
    const l1 = CssLengthOrPercentage.fromLength(10.0, CssLengthUnitTag.px);
    const l2 = CssLengthOrPercentage.fromLength(25.0, CssLengthUnitTag.px);
    if (ord := l1.partialCmp(l2)) {
        assertEq(ord, -1);
    } else {
        assert(false);
    }
    const p1 = CssLengthOrPercentage.fromPercentage(CssPercentage.fromPercent(10.0));
    if (ord_none := l1.partialCmp(p1)) {
        * = ord_none;
        assert(false);
    }
});

test("CssPercentageError: malformed inputs, missing percent sign, unknown units & BufferTooSmall", () => {
    let empty_err = false;
    const r1 = CssPercentage.parse("   ") catch |e| {
        if (e == CssPercentageError.EmptyInput) empty_err = true;
        CssPercentage.zero()
    };
    * = r1;
    assert(empty_err);

    let missing_pct = false;
    const r2 = CssPercentage.parse("50") catch |e| {
        if (e == CssPercentageError.MissingPercentSign) missing_pct = true;
        CssPercentage.zero()
    };
    * = r2;
    assert(missing_pct);

    let bad_syntax = false;
    const r3 = CssPercentage.parse("50 %") catch |e| {
        if (e == CssPercentageError.InvalidSyntax) bad_syntax = true;
        CssPercentage.zero()
    };
    * = r3;
    assert(bad_syntax);

    let unknown_unit = false;
    const r4 = CssLengthOrPercentage.parse("12foo") catch |e| {
        if (e == CssPercentageError.UnknownUnit) unknown_unit = true;
        CssLengthOrPercentage.zero()
    };
    * = r4;
    assert(unknown_unit);

    let unitless_nonzero = false;
    const r5 = CssLengthOrPercentage.parse("12") catch |e| {
        if (e == CssPercentageError.UnknownUnit) unitless_nonzero = true;
        CssLengthOrPercentage.zero()
    };
    * = r5;
    assert(unitless_nonzero);

    let small_buf: [2]u8 = [0; 2];
    let buf_err = false;
    const p = CssPercentage.fromPercent(50.0);
    const r6 = p.toCss(true, small_buf[0..]) catch |e| {
        if (e == CssPercentageError.BufferTooSmall) buf_err = true;
        @as(usize, 0)
    };
    * = r6;
    assert(buf_err);
});
