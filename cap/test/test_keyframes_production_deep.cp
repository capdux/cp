// Source: cp/cap/test/test_keyframes_production_deep.cp (Production deep tests for cap:keyframes)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    VendorPrefix,
    KeyframesNameKind,
    KeyframesName,
    KeyframeSelector,
    KeyframeStop,
    isReservedCssKeyword,
    parseKeyframesRule,
    sortStopsByOffset,
    sampleKeyframeSegment,
} from "cap:keyframes";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

test("1. KeyframesName parsing, reserved CSS keyword rejection & minification quoting rules", () => {
    expect_eq(isReservedCssKeyword("none"), true);
    expect_eq(isReservedCssKeyword("REVERT-LAYER"), true);
    expect_eq(isReservedCssKeyword("fade-in"), false);

    // Unquoted reserved keywords must be rejected
    expect_eq(KeyframesName.parse("none") == null, true);
    expect_eq(KeyframesName.parse("initial") == null, true);
    expect_eq(KeyframesName.parse("revert-layer") == null, true);

    let buf: [32]u8 = [0; 32];
    let out: []u8 = buf[0..];

    // Quoted reserved keyword is valid and MUST retain quotes even when minified
    if (kn_res := KeyframesName.parse("\"none\"")) {
        expect_eq(kn_res.kind, KeyframesNameKind.string_lit);
        if (s := kn_res.format(true, *out)) {
            expect_eq(std.mem.eql(u8, s, "\"none\""), true);
        } else { expect_eq(true, false); }
    } else { expect_eq(true, false); }

    // Quoted regular identifier drops quotes when minified
    if (kn_str := KeyframesName.parse("'fade-in'")) {
        if (s_min := kn_str.format(true, *out)) {
            expect_eq(std.mem.eql(u8, s_min, "fade-in"), true);
        } else { expect_eq(true, false); }
        if (s_pretty := kn_str.format(false, *out)) {
            expect_eq(std.mem.eql(u8, s_pretty, "\"fade-in\""), true);
        } else { expect_eq(true, false); }
    } else { expect_eq(true, false); }
});

test("2. KeyframeSelector parsing, offset normalization & Bun minification (from->0%, 100%->to)", () => {
    let buf: [16]u8 = [0; 16];
    let out: []u8 = buf[0..];

    if (s_from := KeyframeSelector.parse("FROM")) {
        expect_eq(s_from.offset(), 0.0);
        if (m := s_from.format(true, *out)) {
            expect_eq(std.mem.eql(u8, m, "0%"), true);
        } else { expect_eq(true, false); }
        if (p := s_from.format(false, *out)) {
            expect_eq(std.mem.eql(u8, p, "from"), true);
        } else { expect_eq(true, false); }
    } else { expect_eq(true, false); }

    if (s_100 := KeyframeSelector.parse("100%")) {
        expect_eq(s_100.offset(), 1.0);
        if (m := s_100.format(true, *out)) {
            expect_eq(std.mem.eql(u8, m, "to"), true);
        } else { expect_eq(true, false); }
    } else { expect_eq(true, false); }

    if (s_dec := KeyframeSelector.parse("33.5%")) {
        expect_eq(absF64(s_dec.offset() - 0.335) < 1.0e-12, true);
        if (m := s_dec.format(true, *out)) {
            expect_eq(std.mem.eql(u8, m, "33.5%"), true);
        } else { expect_eq(true, false); }
    } else { expect_eq(true, false); }

    // Out-of-range or malformed percentages
    expect_eq(KeyframeSelector.parse("-1%") == null, true);
    expect_eq(KeyframeSelector.parse("101%") == null, true);
    expect_eq(KeyframeSelector.parse("50") == null, true);
});

test("3. parseKeyframesRule with comments, vendor prefix & comma-separated multi-selectors", () => {
    const css = "/* bounce */ @-webkit-keyframes bounce { 0%, 100% { transform: translateY(0); } 50% { transform: translateY(-20px); } }";
    let stops_buf: [8]KeyframeStop = [{ offset: 0.0, selector_raw: "", body_raw: "" }; 8];
    let stops: []KeyframeStop = stops_buf[0..];

    if (res := parseKeyframesRule(css, *stops)) {
        expect_eq(res.vendor, VendorPrefix.webkit);
        expect_eq(std.mem.eql(u8, res.vendor.atRuleStr(), "@-webkit-keyframes"), true);
        expect_eq(std.mem.eql(u8, res.name.value, "bounce"), true);
        expect_eq(res.stop_count, 3);

        expect_eq(stops[0].offset, 0.0);
        expect_eq(stops[1].offset, 1.0);
        expect_eq(stops[2].offset, 0.5);
        expect_eq(std.mem.eql(u8, stops[0].body_raw, "transform: translateY(0);"), true);
        expect_eq(std.mem.eql(u8, stops[1].body_raw, "transform: translateY(0);"), true);
        expect_eq(std.mem.eql(u8, stops[2].body_raw, "transform: translateY(-20px);"), true);
    } else { expect_eq(true, false); }
});

test("4. sortStopsByOffset & sampleKeyframeSegment timeline interpolation", () => {
    const css = "@keyframes pulse { to { opacity: 1; } 25% { opacity: 0.5; } from { opacity: 0; } }";
    let stops_buf: [4]KeyframeStop = [{ offset: 0.0, selector_raw: "", body_raw: "" }; 4];
    let stops: []KeyframeStop = stops_buf[0..];

    if (res := parseKeyframesRule(css, *stops)) {
        expect_eq(res.stop_count, 3);
        let active = stops[0..res.stop_count];
        sortStopsByOffset(*active);

        expect_eq(active[0].offset, 0.0);
        expect_eq(active[1].offset, 0.25);
        expect_eq(active[2].offset, 1.0);

        if (seg1 := sampleKeyframeSegment(active, 0.125)) {
            expect_eq(seg1.prev_idx, 0);
            expect_eq(seg1.next_idx, 1);
            expect_eq(absF64(seg1.local_t - 0.5) < 1.0e-12, true);
        } else { expect_eq(true, false); }

        if (seg2 := sampleKeyframeSegment(active, 0.625)) {
            expect_eq(seg2.prev_idx, 1);
            expect_eq(seg2.next_idx, 2);
            expect_eq(absF64(seg2.local_t - 0.5) < 1.0e-12, true);
        } else { expect_eq(true, false); }

        if (seg_end := sampleKeyframeSegment(active, 1.0)) {
            expect_eq(seg_end.prev_idx, 2);
            expect_eq(seg_end.next_idx, 2);
        } else { expect_eq(true, false); }
    } else { expect_eq(true, false); }
});

test("5. Boundary defense: unclosed comment/braces, unknown at-rule, short buffer & NaN t", () => {
    let stops_buf: [2]KeyframeStop = [{ offset: 0.0, selector_raw: "", body_raw: "" }; 2];
    let stops: []KeyframeStop = stops_buf[0..];

    expect_eq(parseKeyframesRule("/* unclosed @keyframes a {}", *stops) == null, true);
    expect_eq(parseKeyframesRule("@media screen {}", *stops) == null, true);
    expect_eq(parseKeyframesRule("@keyframes none { from {} }", *stops) == null, true);
    expect_eq(parseKeyframesRule("@keyframes a { from { opacity: 0; }", *stops) == null, true);
    expect_eq(parseKeyframesRule("@keyframes a { 150% { opacity: 0; } }", *stops) == null, true);
    // Capacity 2 overflow (3 stops)
    expect_eq(parseKeyframesRule("@keyframes a { from {} 50% {} to {} }", *stops) == null, true);

    // sampleKeyframeSegment invalid t / empty slice
    expect_eq(sampleKeyframeSegment(stops[0..0], 0.5) == null, true);
    expect_eq(sampleKeyframeSegment(stops[0..1], -0.1) == null, true);
    expect_eq(sampleKeyframeSegment(stops[0..1], 1.1) == null, true);
    expect_eq(sampleKeyframeSegment(stops[0..1], std.math.nan(f64)) == null, true);
});
