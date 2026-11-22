// cap/test/testCssResolution.cp
// Production-grade deep tests for cap:css resolution.cp (CSS Values Level 4 `<resolution>`)

import { test, assert, assertEq } from "cap:test";
import {
    CSS_DPI_PER_DPPX,
    CSS_CM_PER_INCH,
    CSS_DPCM_PER_DPPX,
    CssResolutionError,
    CssResolutionUnitKind,
    CssResolutionVal,
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

fn approx(a: f64, b: f64, eps: f64): bool {
    const d = if (a > b) (a - b) else (b - a);
    return d <= eps;
}

test("CssResolution: parse dpi/dpcm/dppx/x case-insensitively and scientific notation", () => {
    const r1 = CssResolutionVal.parse("96dpi") catch unreachable;
    assert(r1.unit == CssResolutionUnitKind.dpi);
    assert(approx(r1.value, 96.0, 1.0e-9));

    const r2 = CssResolutionVal.parse("  300.5DPI  ") catch unreachable;
    assert(r2.unit == CssResolutionUnitKind.dpi);
    assert(approx(r2.value, 300.5, 1.0e-9));

    const r3 = CssResolutionVal.parse("37.795dpcm") catch unreachable;
    assert(r3.unit == CssResolutionUnitKind.dpcm);
    assert(approx(r3.value, 37.795, 1.0e-9));

    const r4 = CssResolutionVal.parse("+2X") catch unreachable;
    assert(r4.unit == CssResolutionUnitKind.dppx);
    assert(approx(r4.value, 2.0, 1.0e-9));

    const r5 = CssResolutionVal.parse(".5DPPX") catch unreachable;
    assert(r5.unit == CssResolutionUnitKind.dppx);
    assert(approx(r5.value, 0.5, 1.0e-9));

    const r6 = CssResolutionVal.parse("1.5e2dpi") catch unreachable;
    assert(r6.unit == CssResolutionUnitKind.dpi);
    assert(approx(r6.value, 150.0, 1.0e-9));

    const r7 = CssResolutionVal.tryFromDimension(2.0, "x") catch unreachable;
    assert(r7.eql(CssResolutionVal.dppx(2.0)));
});

test("CssResolution: exact physical unit conversions (1dppx == 96dpi == 96/2.54 dpcm)", () => {
    assert(approx(CSS_DPI_PER_DPPX, 96.0, 1.0e-12));
    assert(approx(CSS_CM_PER_INCH, 2.54, 1.0e-12));
    assert(approx(CSS_DPCM_PER_DPPX, 96.0 / 2.54, 1.0e-12));

    const retina = CssResolutionVal.dppx(2.0);
    assert(approx(retina.toDpi(), 192.0, 1.0e-9));
    assert(approx(retina.toDpcm(), 192.0 / 2.54, 1.0e-9));
    assert(approx(retina.toDppx(), 2.0, 1.0e-9));

    const from_dpi = CssResolutionVal.dpi(192.0);
    assert(from_dpi.approxEqlPhysical(retina, 1.0e-9));
    assertEq(from_dpi.comparePhysical(retina), 0);
    assert(!from_dpi.eql(retina)); // Different units

    const to_dpcm = from_dpi.convertTo(CssResolutionUnitKind.dpcm);
    assert(to_dpcm.unit == CssResolutionUnitKind.dpcm);
    assert(approx(to_dpcm.toDppx(), 2.0, 1.0e-9));

    const back_to_dpi = to_dpcm.convertTo(CssResolutionUnitKind.dpi);
    assert(approx(back_to_dpi.value, 192.0, 1.0e-9));
});

test("CssResolution: minimal CSS serialization with x_resolution_unit target switch", () => {
    let buf: [32]u8 = [0; 32];

    // dppx with x_resolution_unit compatible vs legacy target
    const r2x = CssResolutionVal.dppx(2.0);
    const n1 = r2x.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n1], "2x"));

    const n2 = r2x.toCss(false, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n2], "2dppx"));

    // Fractional leading-zero omission + trailing-zero stripping
    const half = CssResolutionVal.parse("0.500dppx") catch unreachable;
    const n3 = half.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n3], ".5x"));

    const n4 = half.toCss(false, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n4], ".5dppx"));

    // dpi & dpcm unaffected by use_x_alias
    const dpi_val = CssResolutionVal.dpi(96.0);
    const n5 = dpi_val.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n5], "96dpi"));

    const dpcm_val = CssResolutionVal.dpcm(38.25);
    const n6 = dpcm_val.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n6], "38.25dpcm"));

    // Negative zero normalizes to "0dppx" / "0x"
    const neg_zero = CssResolutionVal.dppx(-0.0);
    const n7 = neg_zero.toCss(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n7], "0x"));
});

test("CssResolution: arithmetic and media query min/max-resolution range matching", () => {
    const base = CssResolutionVal.dpi(96.0);
    const bumped = base.addF64(48.0);
    assert(bumped.unit == CssResolutionUnitKind.dpi);
    assert(approx(bumped.value, 144.0, 1.0e-9));

    // Cross-unit addition keeps LHS unit
    const combined = base.add(CssResolutionVal.dppx(1.0));
    assert(combined.unit == CssResolutionUnitKind.dpi);
    assert(approx(combined.value, 192.0, 1.0e-9));

    const doubled = CssResolutionVal.dppx(1.5).scale(2.0);
    assert(approx(doubled.value, 3.0, 1.0e-9));

    // Media query range: @media (min-resolution: 1.5dppx) and (max-resolution: 288dpi)
    const min_r: CssResolutionVal? = CssResolutionVal.dppx(1.5);
    const max_r: CssResolutionVal? = CssResolutionVal.dpi(288.0); // 3dppx
    const none_r: CssResolutionVal? = null;

    assert(CssResolutionVal.dpi(192.0).matchesRange(min_r, max_r));  // 2dppx in [1.5, 3]
    assert(CssResolutionVal.dppx(1.5).matchesRange(min_r, max_r));   // inclusive lower bound
    assert(CssResolutionVal.dppx(3.0).matchesRange(min_r, max_r));   // inclusive upper bound
    assert(!CssResolutionVal.dpi(96.0).matchesRange(min_r, max_r));  // 1dppx < 1.5
    assert(!CssResolutionVal.dppx(4.0).matchesRange(min_r, max_r));  // 4dppx > 3
    assert(CssResolutionVal.dpi(96.0).matchesRange(none_r, max_r));
    assert(CssResolutionVal.dppx(4.0).matchesRange(min_r, none_r));
});

test("CssResolution: malformed syntax, unknown units, negative guard and tiny buffer", () => {
    let empty_err = false;
    * = CssResolutionVal.parse("   ") catch |e| {
        empty_err = (e == CssResolutionError.EmptyInput);
    };
    assert(empty_err);

    // Unitless number rejected
    let unitless_err = false;
    * = CssResolutionVal.parse("96") catch |e| {
        unitless_err = (e == CssResolutionError.InvalidSyntax);
    };
    assert(unitless_err);

    // Space between number and unit rejected
    let space_err = false;
    * = CssResolutionVal.parse("96 dpi") catch |e| {
        space_err = (e == CssResolutionError.InvalidSyntax);
    };
    assert(space_err);

    // Unknown unit rejected
    let unknown_err = false;
    * = CssResolutionVal.parse("96px") catch |e| {
        unknown_err = (e == CssResolutionError.UnknownUnit);
    };
    assert(unknown_err);

    // parse() accepts negative, parseNonNegative() rejects it
    const neg = CssResolutionVal.parse("-1.5dppx") catch unreachable;
    assert(neg.isNegative());
    let neg_err = false;
    * = CssResolutionVal.parseNonNegative("-1.5dppx") catch |e| {
        neg_err = (e == CssResolutionError.NegativeValue);
    };
    assert(neg_err);

    // Tiny output buffer rejected
    let tiny: [3]u8 = [0; 3];
    let buf_err = false;
    * = CssResolutionVal.dppx(2.0).toCss(false, tiny[0..]) catch |e| {
        buf_err = (e == CssResolutionError.BufferTooSmall);
    };
    assert(buf_err);
});

test("CssResolution: ordering across units and zero predicates", () => {
    const z = CssResolutionVal.parseNonNegative("0dpi") catch unreachable;
    assert(z.isZero());
    assert(!z.isNegative());

    const lo = CssResolutionVal.dpi(72.0);      // 0.75dppx
    const mid = CssResolutionVal.dppx(1.0);     // 96dpi
    const hi = CssResolutionVal.dpcm(50.0);     // ~1.3229dppx

    assertEq(lo.comparePhysical(mid), -1);
    assertEq(mid.comparePhysical(lo), 1);
    assertEq(mid.comparePhysical(hi), -1);
    assertEq(hi.comparePhysical(mid), 1);
});
