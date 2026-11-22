// cap/test/testCssOverflowProp.cp
// Production-grade deep tests for cap:css overflowProp.cp (CSS Overflow L3 `overflow` & `text-overflow`)

import { test, assert, assertEq } from "cap:test";
import {
    CssOverflowPropError,
    CssOverflowAxis,
    CssOverflowProp,
    CssTextOverflowProp,
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

test("CssOverflowAxis: parse all 5 keywords and scroll/clip predicates", () => {
    const vis = CssOverflowAxis.parse("VISIBLE") catch unreachable;
    assert(vis == CssOverflowAxis.visible);
    assert(!vis.allowsProgrammaticScroll());
    assert(!vis.isClipped());

    const hid = CssOverflowAxis.parse("  hidden ") catch unreachable;
    assert(hid == CssOverflowAxis.hidden);
    assert(hid.allowsProgrammaticScroll());
    assert(hid.isClipped());

    const clp = CssOverflowAxis.parse("Clip") catch unreachable;
    assert(clp == CssOverflowAxis.clip);
    assert(!clp.allowsProgrammaticScroll()); // clip forbids programmatic scroll
    assert(clp.isClipped());

    const scr = CssOverflowAxis.parse("scroll") catch unreachable;
    assert(scr.allowsProgrammaticScroll());
    assert(!scr.isClipped());

    const aut = CssOverflowAxis.parse("AUTO") catch unreachable;
    assert(aut == CssOverflowAxis.auto_mode);
    assert(aut.allowsProgrammaticScroll());
    assert(eqStr(aut.asStr(), "auto"));
});

test("CssOverflowProp: 1-value expansion and symmetric collapse minification", () => {
    let buf: [64]u8 = [0; 64];

    const single = CssOverflowProp.parse("hidden") catch unreachable;
    assert(single.x == CssOverflowAxis.hidden);
    assert(single.y == CssOverflowAxis.hidden);
    assert(single.isSymmetric());
    assert(!single.needsTwoValueLowering());

    // Redundant 2-value shorthand collapses to 1 value
    const redundant = CssOverflowProp.parse("  scroll   SCROLL  ") catch unreachable;
    assert(redundant.isSymmetric());
    assert(redundant.eql(CssOverflowProp.single(CssOverflowAxis.scroll)));
    const n1 = redundant.toCss(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n1], "scroll"));

    const n_decl = redundant.toCssDecls(false, true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_decl], "overflow:scroll"));
});

test("CssOverflowProp: 2-value shorthand and legacy longhand lowering", () => {
    let buf: [64]u8 = [0; 64];

    const asym = CssOverflowProp.parse("hidden auto") catch unreachable;
    assert(asym.x == CssOverflowAxis.hidden);
    assert(asym.y == CssOverflowAxis.auto_mode);
    assert(!asym.isSymmetric());
    assert(asym.needsTwoValueLowering());

    // Modern browser supports 2-value shorthand
    const n_mod_min = asym.toCssDecls(true, true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_mod_min], "overflow:hidden auto"));

    const n_mod_pretty = asym.toCssDecls(true, false, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_mod_pretty], "overflow: hidden auto"));

    // Legacy browser lowers to overflow-x + overflow-y
    const n_leg_min = asym.toCssDecls(false, true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_leg_min], "overflow-x:hidden;overflow-y:auto"));

    const n_leg_pretty = asym.toCssDecls(false, false, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_leg_pretty], "overflow-x: hidden; overflow-y: auto"));
});

test("CssTextOverflowProp: parse and serialize clip/ellipsis", () => {
    let buf: [32]u8 = [0; 32];

    const d = CssTextOverflowProp.default();
    assert(d == CssTextOverflowProp.clip);
    const n1 = d.toCss(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n1], "clip"));

    const e = CssTextOverflowProp.parse("  ELLIPSIS ") catch unreachable;
    assert(e == CssTextOverflowProp.ellipsis);
    const n2 = e.toCss(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n2], "ellipsis"));
});

test("CssOverflowProp: malformed syntax, trailing tokens and tiny buffer guards", () => {
    let empty_err = false;
    * = CssOverflowProp.parse("   ") catch |e| {
        empty_err = (e == CssOverflowPropError.EmptyInput);
    };
    assert(empty_err);

    let bad_kw = false;
    * = CssOverflowProp.parse("overlay") catch |e| {
        bad_kw = (e == CssOverflowPropError.InvalidKeyword);
    };
    assert(bad_kw);

    let trailing = false;
    * = CssOverflowProp.parse("hidden auto scroll") catch |e| {
        trailing = (e == CssOverflowPropError.TrailingTokens);
    };
    assert(trailing);

    let bad_text = false;
    * = CssTextOverflowProp.parse("fade") catch |e| {
        bad_text = (e == CssOverflowPropError.InvalidKeyword);
    };
    assert(bad_text);

    let tiny: [5]u8 = [0; 5];
    let buf_err = false;
    * = CssOverflowProp.default().toCss(tiny[0..]) catch |e| {
        buf_err = (e == CssOverflowPropError.BufferTooSmall);
    };
    assert(buf_err);
});

test("CssOverflowProp: axis serialization and equality checks", () => {
    let buf: [16]u8 = [0; 16];
    const n = CssOverflowAxis.clip.toCss(buf[0..]) catch unreachable;
    assertEq(n, 4);
    assert(eqStr(buf[0..n], "clip"));

    const a = CssOverflowProp.parse("clip visible") catch unreachable;
    const b = CssOverflowProp.parse("visible clip") catch unreachable;
    assert(!a.eql(b));
    assert(CssOverflowProp.default().eql(CssOverflowProp.single(CssOverflowAxis.visible)));
});
