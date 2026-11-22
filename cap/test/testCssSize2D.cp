import { test, assert } from "cap:test";
import {
    CssSize2DError,
    CssNumberPair,
    CssLengthPctUnit,
    CssSizeComponent,
    CssLengthPctPair,
    CssCornerRadii,
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

fn approx(a: f64, b: f64, eps: f64): bool {
    const d = if (a > b) (a - b) else (b - a);
    return d <= eps;
}

test("CssNumberPair parse, arithmetic, and single-value collapse", () => {
    let buf: [64]u8 = [0; 64];

    const p1 = CssNumberPair.parse("1.5") catch unreachable;
    assert(p1.isUniform());
    assert(p1.a == 1.5 && p1.b == 1.5);
    const n1 = p1.toCss(buf[0..]) catch unreachable;
    assert(strEql(buf[0..n1], "1.5"));

    const p2 = CssNumberPair.parse("  0.5   -0.25 ") catch unreachable;
    assert(!p2.isUniform());
    const n2 = p2.toCss(buf[0..]) catch unreachable;
    assert(strEql(buf[0..n2], ".5 -.25"));

    // Equal two-token input collapses to single token on toCss
    const p3 = CssNumberPair.parse("2.0 2") catch unreachable;
    assert(p3.isUniform());
    const n3 = p3.toCss(buf[0..]) catch unreachable;
    assert(strEql(buf[0..n3], "2"));

    const scaled = p2.scale(2.0) catch unreachable;
    assert(scaled.a == 1.0 && scaled.b == -0.5);

    const sum = p1.add(scaled) catch unreachable;
    assert(sum.a == 2.5 && sum.b == 1.0);
});

test("CssSizeComponent units, em/ex disambiguation, and resolvePx", () => {
    // `1.5em` and `2ex` must not be misread as scientific notation!
    const em_c = CssSizeComponent.parse("1.5em") catch unreachable;
    assert(em_c.unit == CssLengthPctUnit.Em);
    assert(em_c.value == 1.5);
    if (px := em_c.resolvePx(200.0, 20.0, 16.0, 1000.0, 800.0)) {
        assert(approx(px, 30.0, 1e-9));
    } else {
        assert(false);
    }

    const ex_c = CssSizeComponent.parse("2ex") catch unreachable;
    assert(ex_c.unit == CssLengthPctUnit.Ex);
    assert(ex_c.resolvePx(200.0, 20.0, 16.0, 1000.0, 800.0) == null);

    // Scientific notation with unit: `1e2px` == 100px
    const sci_px = CssSizeComponent.parse("1e2px") catch unreachable;
    assert(sci_px.unit == CssLengthPctUnit.Px);
    assert(sci_px.value == 100.0);

    // Percentage, rem, vw, vmin, in
    const pct_c = CssSizeComponent.parse("50%") catch unreachable;
    assert(pct_c.isPercentage());
    if (px := pct_c.resolvePx(400.0, 16.0, 16.0, 1000.0, 800.0)) {
        assert(approx(px, 200.0, 1e-9));
    } else {
        assert(false);
    }

    const vmin_c = CssSizeComponent.parse("10vmin") catch unreachable;
    if (px := vmin_c.resolvePx(400.0, 16.0, 16.0, 1000.0, 800.0)) {
        assert(approx(px, 80.0, 1e-9));
    } else {
        assert(false);
    }

    // 0% stays distinct from 0px / 0
    const z_pct = CssSizeComponent.parse("0%") catch unreachable;
    const z_px = CssSizeComponent.parse("0px") catch unreachable;
    const z_bare = CssSizeComponent.parse("0") catch unreachable;
    assert(z_px.eql(z_bare));
    assert(!z_pct.eql(z_px));
});

test("CssLengthPctPair parse, collapse, minify, and scale", () => {
    let buf: [64]u8 = [0; 64];

    const pair1 = CssLengthPctPair.parse("12px 12px") catch unreachable;
    assert(pair1.isUniform());
    const n1 = pair1.toCss(buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n1], "12px"));

    const pair2 = CssLengthPctPair.parse("10px 50%") catch unreachable;
    assert(!pair2.isUniform());
    const n2 = pair2.toCss(buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n2], "10px 50%"));

    // Minifying `0px 0px` -> `0`, while `0% 0%` preserves `0%`
    const zp = CssLengthPctPair.parse("0px 0px") catch unreachable;
    assert(zp.isZero());
    const n3 = zp.toCss(buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n3], "0"));

    const zpct = CssLengthPctPair.parse("0% 0%") catch unreachable;
    const n4 = zpct.toCss(buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n4], "0%"));

    const doubled = pair2.scale(2.0) catch unreachable;
    const n5 = doubled.toCss(buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n5], "20px 100%"));
});

test("CssCornerRadii 1/2/3/4 circular & elliptical border-radius shorthand", () => {
    let buf: [128]u8 = [0; 128];

    // Circular 1-value
    const c1 = CssCornerRadii.parse("8px") catch unreachable;
    assert(c1.isCircular());
    assert(!c1.isZero());
    const n1 = c1.toCss(buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n1], "8px"));

    // Circular 4-value collapsing to 2-value
    const c2 = CssCornerRadii.parse("8px 16px 8px 16px") catch unreachable;
    assert(c2.isCircular());
    const n2 = c2.toCss(buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n2], "8px 16px"));

    // Circular 3-value
    const c3 = CssCornerRadii.parse("4px 8px 12px") catch unreachable;
    assert(c3.bottom_left.a.value == 8.0);
    const n3 = c3.toCss(buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n3], "4px 8px 12px"));

    // Elliptical shorthand with optimal collapse on both sides
    const e1 = CssCornerRadii.parse("8px 16px 8px 16px / 4px 8px 4px 8px") catch unreachable;
    assert(!e1.isCircular());
    const n4 = e1.toCss(buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n4], "8px 16px/4px 8px"));

    const n5 = e1.toCss(buf[0..], false) catch unreachable;
    assert(strEql(buf[0..n5], "8px 16px / 4px 8px"));

    // Elliptical where vertical equals horizontal collapses back to circular!
    const e2 = CssCornerRadii.parse("10px / 10px 10px 10px 10px") catch unreachable;
    assert(e2.isCircular());
    const n6 = e2.toCss(buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n6], "10px"));
});

test("CssSize2D malformed inputs, negative radii, and BufferTooSmall guards", () => {
    // Empty input
    let empty_err = false;
    * = CssNumberPair.parse("   ") catch |e| { empty_err = (e == CssSize2DError.EmptyInput); };
    assert(empty_err);

    // Too many components (> 2 for pair, > 4 for border-radius)
    let too_many_pair = false;
    * = CssLengthPctPair.parse("1px 2px 3px") catch |e| { too_many_pair = (e == CssSize2DError.TooManyComponents); };
    assert(too_many_pair);

    let too_many_rad = false;
    * = CssCornerRadii.parse("1px 2px 3px 4px 5px") catch |e| { too_many_rad = (e == CssSize2DError.TooManyComponents); };
    assert(too_many_rad);

    // Unitless non-zero length rejected
    let unitless_err = false;
    * = CssLengthPctPair.parse("10 20px") catch |e| { unitless_err = (e == CssSize2DError.InvalidSyntax); };
    assert(unitless_err);

    // Unknown unit rejected
    let unknown_u = false;
    * = CssLengthPctPair.parse("10foo") catch |e| { unknown_u = (e == CssSize2DError.UnknownUnit); };
    assert(unknown_u);

    // Negative border-radius rejected
    let neg_rad = false;
    * = CssCornerRadii.parse("-4px") catch |e| { neg_rad = (e == CssSize2DError.NegativeNotAllowed); };
    assert(neg_rad);

    // Multiple slashes in border-radius rejected
    let dbl_slash = false;
    * = CssCornerRadii.parse("4px / 8px / 12px") catch |e| { dbl_slash = (e == CssSize2DError.InvalidSyntax); };
    assert(dbl_slash);

    // BufferTooSmall guard
    let tiny: [3]u8 = [0; 3];
    let buf_err = false;
    const p = CssLengthPctPair.parse("10px 20px") catch unreachable;
    * = p.toCss(tiny[0..], true) catch |e| { buf_err = (e == CssSize2DError.BufferTooSmall); };
    assert(buf_err);
});
