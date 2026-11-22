import { test, assert, assertEq } from "cap:test";
import {
    CssRatioError,
    CssRatioVal,
    CssAspectRatioKind,
    CssAspectRatioVal,
    matchCssMediaAspectRatio,
} from "cap:css";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn expectParseErr(raw: []const u8, expected: CssRatioError): bool {
    let matched = false;
    * = CssRatioVal.parse(raw) catch |e| {
        matched = (e == expected);
    };
    return matched;
}

test("CssRatioVal.parse & parseRequired basic forms and whitespace", () => {
    const r1 = CssRatioVal.parse("16 / 9") catch unreachable;
    assert(r1.numerator == 16.0);
    assert(r1.denominator == 9.0);
    assert(!r1.isDegenerate());

    // Compact without spaces around '/'
    const r2 = CssRatioVal.parse("4/3") catch unreachable;
    assert(r2.numerator == 4.0);
    assert(r2.denominator == 3.0);

    // Single operand defaults denominator to 1.0
    const r3 = CssRatioVal.parse("  1.5  ") catch unreachable;
    assert(r3.numerator == 1.5);
    assert(r3.denominator == 1.0);

    // Leading dot and scientific notation
    const r4 = CssRatioVal.parse(".5 / 2.5e-1") catch unreachable;
    assert(r4.numerator == 0.5);
    assert(r4.denominator == 0.25);
    if (v := r4.value()) {
        assert(v == 2.0);
    } else {
        assert(false);
    }

    // parseRequired accepts explicit slash
    const r5 = CssRatioVal.parseRequired("\t21 / 9\n") catch unreachable;
    assert(r5.numerator == 21.0);
    assert(r5.denominator == 9.0);

    // parseRequired rejects single operand without '/'
    let miss_err = false;
    * = CssRatioVal.parseRequired("16") catch |e| {
        miss_err = (e == CssRatioError.MissingDelimiter);
    };
    assert(miss_err);
});

test("CssRatioVal serialization omits /1 and aligns with Bun formatting", () => {
    let buf: [64]u8 = [0; 64];

    const r1 = CssRatioVal.parse("16 / 9") catch unreachable;
    const n1 = r1.toCss(buf[0..], false) catch unreachable;
    assert(strEql(buf[0..n1], "16 / 9"));

    const n2 = r1.toCss(buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n2], "16/9"));

    // Denominator == 1 omits "/ 1"
    const r2 = CssRatioVal.parse("2 / 1") catch unreachable;
    const n3 = r2.toCss(buf[0..], false) catch unreachable;
    assert(strEql(buf[0..n3], "2"));

    // Fractional < 1 omits leading zero
    const r3 = CssRatioVal.parse("0.5 / 0.25") catch unreachable;
    const n4 = r3.toCss(buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n4], ".5/.25"));

    // BufferTooSmall guard
    let tiny: [3]u8 = [0; 3];
    let buf_err = false;
    * = r1.toCss(tiny[0..], true) catch |e| {
        buf_err = (e == CssRatioError.BufferTooSmall);
    };
    assert(buf_err);
});

test("CssRatioVal degenerate ratios and safe dimension resolution", () => {
    const z_num = CssRatioVal.parse("0 / 9") catch unreachable;
    assert(z_num.isDegenerate());
    assert(z_num.value() == null);
    assert(z_num.resolveHeight(100.0) == null);
    assert(z_num.resolveWidth(100.0) == null);

    const z_den = CssRatioVal.parse("16 / 0") catch unreachable;
    assert(z_den.isDegenerate());
    assert(z_den.value() == null);

    const z_both = CssRatioVal.parse("-0 / +0") catch unreachable;
    assert(z_both.isDegenerate());
    assert(z_both.numerator == 0.0);
    assert(z_both.denominator == 0.0);

    // Non-degenerate dimension resolution
    const hd = CssRatioVal.parse("16 / 9") catch unreachable;
    if (h := hd.resolveHeight(1920.0)) {
        assert(h == 1080.0);
    } else {
        assert(false);
    }
    if (w := hd.resolveWidth(1080.0)) {
        assert(w == 1920.0);
    } else {
        assert(false);
    }

    // Negative input dimension returns null safely
    assert(hd.resolveHeight(-10.0) == null);
    assert(hd.resolveWidth(-10.0) == null);
});

test("CssRatioVal GCD reduction, reciprocal, addNumerator, and comparison", () => {
    const full_hd = CssRatioVal.fromParts(1920.0, 1080.0) catch unreachable;
    const reduced = full_hd.reduce();
    assert(reduced.numerator == 16.0);
    assert(reduced.denominator == 9.0);
    assert(!full_hd.eql(reduced));
    assert(full_hd.equivalent(reduced, 1e-12));
    assertEq(full_hd.compare(reduced), 0);

    // Non-integer ratio stays unchanged under reduce()
    const frac = CssRatioVal.fromParts(1.5, 2.5) catch unreachable;
    const frac_r = frac.reduce();
    assert(frac_r.eql(frac));

    // Reciprocal swaps numerator and denominator
    const portrait = reduced.reciprocal();
    assert(portrait.numerator == 9.0);
    assert(portrait.denominator == 16.0);
    assertEq(portrait.compare(reduced), -1);
    assertEq(reduced.compare(portrait), 1);

    // addNumerator mirrors Bun addF32 with non-negative guard
    const bumped = reduced.addNumerator(2.0) catch unreachable;
    assert(bumped.numerator == 18.0);
    assert(bumped.denominator == 9.0);

    let neg_bump_err = false;
    * = reduced.addNumerator(-20.0) catch |e| {
        neg_bump_err = (e == CssRatioError.NegativeValue);
    };
    assert(neg_bump_err);
});

test("CssAspectRatioVal parse, normalization, effectiveRatio, and toCss", () => {
    let buf: [64]u8 = [0; 64];

    const a1 = CssAspectRatioVal.parse("AUTO") catch unreachable;
    assert(a1.kind == CssAspectRatioKind.Auto);
    assert(a1.isAuto());
    assert(!a1.hasRatio());
    assert(a1.effectiveRatio() == null);
    const n1 = a1.toCss(buf[0..], false) catch unreachable;
    assert(strEql(buf[0..n1], "auto"));

    const a2 = CssAspectRatioVal.parse("16 / 9") catch unreachable;
    assert(a2.kind == CssAspectRatioKind.Ratio);
    assert(!a2.isAuto());
    assert(a2.hasRatio());
    if (eff := a2.effectiveRatio()) {
        assert(eff.numerator == 16.0 && eff.denominator == 9.0);
    } else {
        assert(false);
    }

    // Both `auto <ratio>` and `<ratio> auto` serialize canonically as `auto <ratio>`
    const a3 = CssAspectRatioVal.parse("auto 4 / 3") catch unreachable;
    const a4 = CssAspectRatioVal.parse("4 / 3 auto") catch unreachable;
    assert(a3.kind == CssAspectRatioKind.AutoRatio);
    assert(a3.eql(a4));
    const n3 = a3.toCss(buf[0..], false) catch unreachable;
    assert(strEql(buf[0..n3], "auto 4 / 3"));
    const n4 = a4.toCss(buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n4], "auto 4/3"));

    // Degenerate ratio inside aspect-ratio yields null effectiveRatio
    const a5 = CssAspectRatioVal.parse("auto 0 / 1") catch unreachable;
    assert(a5.hasRatio());
    assert(a5.effectiveRatio() == null);
});

test("matchCssMediaAspectRatio exact, min, max, and degenerate protection", () => {
    const target = CssRatioVal.parse("16 / 9") catch unreachable;

    // Exact match: 1920x1080 == 16/9
    assert(matchCssMediaAspectRatio(1920.0, 1080.0, target, false, false));
    assert(!matchCssMediaAspectRatio(1280.0, 800.0, target, false, false));

    // min-aspect-ratio: viewport >= target
    assert(matchCssMediaAspectRatio(2560.0, 1080.0, target, true, false));
    assert(matchCssMediaAspectRatio(1920.0, 1080.0, target, true, false));
    assert(!matchCssMediaAspectRatio(1024.0, 768.0, target, true, false));

    // max-aspect-ratio: viewport <= target
    assert(matchCssMediaAspectRatio(1024.0, 768.0, target, false, true));
    assert(matchCssMediaAspectRatio(1920.0, 1080.0, target, false, true));
    assert(!matchCssMediaAspectRatio(2560.0, 1080.0, target, false, true));

    // Invalid viewport or degenerate target never matches
    assert(!matchCssMediaAspectRatio(0.0, 1080.0, target, true, false));
    assert(!matchCssMediaAspectRatio(1920.0, -1.0, target, false, true));
    const degen = CssRatioVal.parse("16 / 0") catch unreachable;
    assert(!matchCssMediaAspectRatio(1920.0, 1080.0, degen, false, false));
});

test("CssRatioVal malformed & boundary input guards", () => {
    // Empty & whitespace
    assert(expectParseErr("", CssRatioError.EmptyInput));
    assert(expectParseErr("   \t\n ", CssRatioError.EmptyInput));

    // Negative values rejected
    assert(expectParseErr("-16 / 9", CssRatioError.NegativeValue));
    assert(expectParseErr("16 / -9", CssRatioError.NegativeValue));
    let neg_parts_err = false;
    * = CssRatioVal.fromParts(-1.0, 2.0) catch |e| {
        neg_parts_err = (e == CssRatioError.NegativeValue);
    };
    assert(neg_parts_err);

    // Malformed syntax
    assert(expectParseErr("16 /", CssRatioError.InvalidSyntax));
    assert(expectParseErr("/ 9", CssRatioError.InvalidSyntax));
    assert(expectParseErr("16 / 9 / 2", CssRatioError.InvalidSyntax));
    assert(expectParseErr("16. / 9", CssRatioError.InvalidSyntax));
    assert(expectParseErr("16px / 9px", CssRatioError.InvalidSyntax));

    // Overflow exponent
    assert(expectParseErr("1e400 / 1", CssRatioError.Overflow));
});
