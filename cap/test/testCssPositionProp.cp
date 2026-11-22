// cap/test/testCssPositionProp.cp
// Production-grade deep tests for cap:css positionProp.cp (CSS Positioning L3 `position` property)

import { test, assert, assertEq } from "cap:test";
import {
    CSS_STICKY_PREFIX_NONE,
    CSS_STICKY_PREFIX_WEBKIT,
    CSS_STICKY_PREFIX_BOTH,
    CssPositionPropError,
    CssPositionKind,
    CssPositionProp,
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

test("CssPositionProp: parse all 6 keywords case-insensitively", () => {
    const p_static = CssPositionProp.parse("  STATIC ") catch unreachable;
    assert(p_static.kind == CssPositionKind.static_pos);
    assert(!p_static.isPositioned());
    assert(!p_static.isOutOfFlow());
    assert(!p_static.createsContainingBlockForAbsolute());

    const p_rel = CssPositionProp.parse("relative") catch unreachable;
    assert(p_rel.kind == CssPositionKind.relative);
    assert(p_rel.isPositioned());
    assert(!p_rel.isOutOfFlow());
    assert(p_rel.createsContainingBlockForAbsolute());

    const p_abs = CssPositionProp.parse("Absolute") catch unreachable;
    assert(p_abs.kind == CssPositionKind.absolute);
    assert(p_abs.isPositioned());
    assert(p_abs.isOutOfFlow());

    const p_fix = CssPositionProp.parse("FIXED") catch unreachable;
    assert(p_fix.kind == CssPositionKind.fixed);
    assert(p_fix.isOutOfFlow());

    const p_stk = CssPositionProp.parse("sticky") catch unreachable;
    assert(p_stk.isSticky());
    assertEq(p_stk.sticky_prefixes, CSS_STICKY_PREFIX_NONE);

    const p_wk = CssPositionProp.parse("-WEBKIT-STICKY") catch unreachable;
    assert(p_wk.isSticky());
    assertEq(p_wk.sticky_prefixes, CSS_STICKY_PREFIX_WEBKIT);
    assert(!p_stk.eql(p_wk));
});

test("CssPositionProp: Safari vendor-prefix lowering emits fallback + standard decls", () => {
    let buf: [64]u8 = [0; 64];

    const s = CssPositionProp.parse("sticky") catch unreachable;
    const expanded = s.withSafariCompat(true);
    assertEq(expanded.sticky_prefixes, CSS_STICKY_PREFIX_BOTH);

    const n_min = expanded.toCssDecls(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_min], "position:-webkit-sticky;position:sticky"));

    const n_pretty = expanded.toCssDecls(false, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_pretty], "position: -webkit-sticky; position: sticky"));

    // Non-sticky values are unchanged by withSafariCompat(true)
    const rel = CssPositionProp.parse("relative") catch unreachable;
    const rel2 = rel.withSafariCompat(true);
    assert(rel.eql(rel2));
    const n_rel = rel2.toCssDecls(true, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_rel], "position:relative"));
});

test("CssPositionProp: single-value toCss formatting for all variants", () => {
    let buf: [32]u8 = [0; 32];

    const n0 = CssPositionProp.default().toCss(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n0], "static"));

    const n1 = (CssPositionProp.parse("absolute") catch unreachable).toCss(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n1], "absolute"));

    const n2 = (CssPositionProp.parse("fixed") catch unreachable).toCss(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n2], "fixed"));

    const n3 = (CssPositionProp.parse("-webkit-sticky") catch unreachable).toCss(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n3], "-webkit-sticky"));

    const n4 = CssPositionProp.sticky(0).toCss(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n4], "sticky"));
});

test("CssPositionProp: equality and default helpers", () => {
    const d = CssPositionProp.default();
    const s = CssPositionProp.parse("static") catch unreachable;
    assert(d.eql(s));

    const w1 = CssPositionProp.sticky(CSS_STICKY_PREFIX_WEBKIT);
    const w2 = CssPositionProp.parse("-webkit-sticky") catch unreachable;
    assert(w1.eql(w2));

    const no_expand = (CssPositionProp.parse("sticky") catch unreachable).withSafariCompat(false);
    assertEq(no_expand.sticky_prefixes, CSS_STICKY_PREFIX_NONE);
});

test("CssPositionProp: malformed input and tiny buffer guards", () => {
    let empty_err = false;
    * = CssPositionProp.parse("  \t\n ") catch |e| {
        empty_err = (e == CssPositionPropError.EmptyInput);
    };
    assert(empty_err);

    let bad_err = false;
    * = CssPositionProp.parse("center") catch |e| {
        bad_err = (e == CssPositionPropError.InvalidKeyword);
    };
    assert(bad_err);

    let moz_err = false;
    * = CssPositionProp.parse("-moz-sticky") catch |e| {
        moz_err = (e == CssPositionPropError.InvalidKeyword);
    };
    assert(moz_err);

    let tiny: [5]u8 = [0; 5];
    let buf_err1 = false;
    * = CssPositionProp.default().toCss(tiny[0..]) catch |e| {
        buf_err1 = (e == CssPositionPropError.BufferTooSmall);
    };
    assert(buf_err1);

    let tiny_decl: [20]u8 = [0; 20];
    let buf_err2 = false;
    const both = CssPositionProp.sticky(CSS_STICKY_PREFIX_BOTH);
    * = both.toCssDecls(true, tiny_decl[0..]) catch |e| {
        buf_err2 = (e == CssPositionPropError.BufferTooSmall);
    };
    assert(buf_err2);
});
