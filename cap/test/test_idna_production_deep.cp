// Source: cp/cap/test/test_idna_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:idna & cap.idna

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    IdnaError,
    punycodeEncode,
    punycodeDecode,
    toAscii,
    toUnicode,
    isValidDomain,
    normalizeDomain,
} from "cap:text";

fn emptyMutSlice(): []u8 {
    return std.heap.page_allocator.alloc(u8, 0) catch unreachable;
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // Empty input
    const empty_enc = punycodeEncode(alloc, "")?;
    defer alloc.free(empty_enc);
    expect_eq(empty_enc.len, 0 as usize);

    const empty_dec = punycodeDecode(alloc, "")?;
    defer alloc.free(empty_dec);
    expect_eq(empty_dec.len, 0 as usize);

    // RFC 3492 Section 7.1 (A) Arabic (Egyptian)
    const ar_u = "\u{0644}\u{064A}\u{0647}\u{0645}\u{0627}\u{0628}\u{062A}\u{0643}\u{0644}\u{0645}\u{0648}\u{0634}\u{0639}\u{0631}\u{0628}\u{064A}\u{061F}";
    const ar_p = "egbpdaj6bu4bxfgehfvwxn";
    const ar_enc = punycodeEncode(alloc, ar_u)?;
    defer alloc.free(ar_enc);
    expect_eq(std.mem.eql(u8, ar_enc, ar_p), true);
    const ar_dec = punycodeDecode(alloc, ar_p)?;
    defer alloc.free(ar_dec);
    expect_eq(std.mem.eql(u8, ar_dec, ar_u), true);

    // RFC 3492 Section 7.1 (B) Chinese (Simplified)
    const zh_s_u = "他们为什么不说中文";
    const zh_s_p = "ihqwcrb4cv8a8dqg056pqjye";
    const zh_s_enc = punycodeEncode(alloc, zh_s_u)?;
    defer alloc.free(zh_s_enc);
    expect_eq(std.mem.eql(u8, zh_s_enc, zh_s_p), true);
    const zh_s_dec = punycodeDecode(alloc, zh_s_p)?;
    defer alloc.free(zh_s_dec);
    expect_eq(std.mem.eql(u8, zh_s_dec, zh_s_u), true);

    // RFC 3492 Section 7.1 (C) Chinese (Traditional)
    const zh_t_u = "他們爲什麽不說中文";
    const zh_t_p = "ihqwctvzc91f659drss3x8bo0yb";
    const zh_t_enc = punycodeEncode(alloc, zh_t_u)?;
    defer alloc.free(zh_t_enc);
    expect_eq(std.mem.eql(u8, zh_t_enc, zh_t_p), true);
    const zh_t_dec = punycodeDecode(alloc, zh_t_p)?;
    defer alloc.free(zh_t_dec);
    expect_eq(std.mem.eql(u8, zh_t_dec, zh_t_u), true);

    // RFC 3492 Section 7.1 (E) Japanese
    const ja_u = "なぜみんな日本語を話してくれないのか";
    const ja_p = "n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa";
    const ja_enc = punycodeEncode(alloc, ja_u)?;
    defer alloc.free(ja_enc);
    expect_eq(std.mem.eql(u8, ja_enc, ja_p), true);
    const ja_dec = punycodeDecode(alloc, ja_p)?;
    defer alloc.free(ja_dec);
    expect_eq(std.mem.eql(u8, ja_dec, ja_u), true);

    // German: München -> xn--mnchen-3ya (toAscii) and Mnchen-3ya (raw punycodeEncode)
    const de_raw_enc = punycodeEncode(alloc, "München")?;
    defer alloc.free(de_raw_enc);
    expect_eq(std.mem.eql(u8, de_raw_enc, "Mnchen-3ya"), true);
    const de_raw_dec = punycodeDecode(alloc, "Mnchen-3ya")?;
    defer alloc.free(de_raw_dec);
    expect_eq(std.mem.eql(u8, de_raw_dec, "München"), true);

    const de_ascii = toAscii(alloc, "München")?;
    defer alloc.free(de_ascii);
    expect_eq(std.mem.eql(u8, de_ascii, "xn--mnchen-3ya"), true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    // Multi-label domain with ideographic full stop \u{3002}
    const ja_dom = "例え.テスト\u{3002}jp";
    const ja_ascii = toAscii(alloc, ja_dom)?;
    defer alloc.free(ja_ascii);
    expect_eq(std.mem.eql(u8, ja_ascii, "xn--r8jz45g.xn--zckzah.jp"), true);

    const ja_uni = toUnicode(alloc, ja_ascii)?;
    defer alloc.free(ja_uni);
    expect_eq(std.mem.eql(u8, ja_uni, "例え.テスト.jp"), true);

    // Fullwidth full stop \u{FF0E} and optional trailing root dot
    const de_dom = "München\u{FF0E}DE.";
    const de_ascii = toAscii(alloc, de_dom)?;
    defer alloc.free(de_ascii);
    expect_eq(std.mem.eql(u8, de_ascii, "xn--mnchen-3ya.de."), true);

    const de_uni = toUnicode(alloc, de_ascii)?;
    defer alloc.free(de_uni);
    expect_eq(std.mem.eql(u8, de_uni, "münchen.de."), true);

    // Halfwidth ideographic full stop \u{FF61} & case-insensitive XN--
    const mixed_ascii = toAscii(alloc, "sub\u{FF61}Bücher\u{3002}org")?;
    defer alloc.free(mixed_ascii);
    expect_eq(std.mem.eql(u8, mixed_ascii, "sub.xn--bcher-kva.org"), true);

    const upper_xn = toUnicode(alloc, "XN--MNCHEN-3YA.DE")?;
    defer alloc.free(upper_xn);
    expect_eq(std.mem.eql(u8, upper_xn, "münchen.de"), true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // 63-byte label valid; 64-byte label -> LabelTooLong
    const lbl63 = alloc.alloc(u8, 63) catch unreachable;
    defer alloc.free(lbl63);
    @memset(lbl63, 'a');
    const ok63 = toAscii(alloc, lbl63)?;
    defer alloc.free(ok63);
    expect_eq(ok63.len, 63 as usize);
    expect_eq(isValidDomain(lbl63), true);

    const lbl64 = alloc.alloc(u8, 64) catch unreachable;
    defer alloc.free(lbl64);
    @memset(lbl64, 'a');
    let got_lbl_too_long = false;
    * = toAscii(alloc, lbl64) catch e => errLbl: {
        got_lbl_too_long = (e == IdnaError.LabelTooLong);
        break :errLbl emptyMutSlice();
    };
    expect_eq(got_lbl_too_long, true);
    expect_eq(isValidDomain(lbl64), false);

    // 253-byte domain valid (even with trailing root dot = 254); 254-byte domain -> DomainTooLong
    // 63 + 1 + 63 + 1 + 63 + 1 + 61 = 253
    let dom253_dot = alloc.alloc(u8, 254) catch unreachable;
    defer alloc.free(dom253_dot);
    @memset(dom253_dot[0..63], 'a');
    dom253_dot[63] = '.';
    @memset(dom253_dot[64..127], 'b');
    dom253_dot[127] = '.';
    @memset(dom253_dot[128..191], 'c');
    dom253_dot[191] = '.';
    @memset(dom253_dot[192..253], 'd');
    dom253_dot[253] = '.';
    const dom253 = dom253_dot[0..253];
    expect_eq(isValidDomain(dom253), true);
    expect_eq(isValidDomain(dom253_dot), true);

    let dom254 = alloc.alloc(u8, 254) catch unreachable;
    defer alloc.free(dom254);
    @memcpy(dom254[0..253], dom253);
    dom254[253] = 'd';
    let got_dom_too_long = false;
    * = toAscii(alloc, dom254) catch e => errDom: {
        got_dom_too_long = (e == IdnaError.DomainTooLong);
        break :errDom emptyMutSlice();
    };
    expect_eq(got_dom_too_long, true);
    expect_eq(isValidDomain(dom254), false);

    // Leading/trailing hyphen
    let got_hyphen_lead = false;
    * = toAscii(alloc, "-bad.example.com") catch e => errHyL: {
        got_hyphen_lead = (e == IdnaError.LeadingOrTrailingHyphen);
        break :errHyL emptyMutSlice();
    };
    expect_eq(got_hyphen_lead, true);

    let got_hyphen_trail = false;
    * = toAscii(alloc, "bad-.example.com") catch e => errHyT: {
        got_hyphen_trail = (e == IdnaError.LeadingOrTrailingHyphen);
        break :errHyT emptyMutSlice();
    };
    expect_eq(got_hyphen_trail, true);

    // Empty label
    let got_empty = false;
    * = toAscii(alloc, "a..b") catch e => errEmp: {
        got_empty = (e == IdnaError.EmptyLabel);
        break :errEmp emptyMutSlice();
    };
    expect_eq(got_empty, true);

    // Invalid character
    let got_inv_char = false;
    * = toAscii(alloc, "bad_label.com") catch e => errChr: {
        got_inv_char = (e == IdnaError.InvalidChar);
        break :errChr emptyMutSlice();
    };
    expect_eq(got_inv_char, true);

    // Invalid / truncated variable-length integer Punycode
    let got_inv_puny = false;
    * = punycodeDecode(alloc, "9") catch e => errPuny: {
        got_inv_puny = (e == IdnaError.InvalidPunycode);
        break :errPuny emptyMutSlice();
    };
    expect_eq(got_inv_puny, true);

    // Empty xn-- label in toUnicode -> InvalidPunycode
    let got_xn_empty = false;
    * = toUnicode(alloc, "xn--.com") catch e => errXn: {
        got_xn_empty = (e == IdnaError.InvalidPunycode);
        break :errXn emptyMutSlice();
    };
    expect_eq(got_xn_empty, true);

    // Integer overflow protection in punycodeDecode
    let got_overflow = false;
    * = punycodeDecode(alloc, "99999999999") catch e => errOvf: {
        got_overflow = (e == IdnaError.Overflow);
        break :errOvf emptyMutSlice();
    };
    expect_eq(got_overflow, true);

    // Invalid UTF-8
    const bad_utf8: [2]u8 = [0xFF, 0xFE];
    let got_utf8 = false;
    * = punycodeEncode(alloc, &bad_utf8) catch e => errU8: {
        got_utf8 = (e == IdnaError.InvalidUtf8);
        break :errU8 emptyMutSlice();
    };
    expect_eq(got_utf8, true);

    // Disallowed bidirection control character
    let got_bidi = false;
    * = toAscii(alloc, "\u{202E}abc.com") catch e => errBidi: {
        got_bidi = (e == IdnaError.DisallowedBidirection);
        break :errBidi emptyMutSlice();
    };
    expect_eq(got_bidi, true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // Pure ASCII lowercase normalization and zero-conversion roundtrip
    const asc1 = toAscii(alloc, "Example.COM")?;
    defer alloc.free(asc1);
    expect_eq(std.mem.eql(u8, asc1, "example.com"), true);

    const uni1 = toUnicode(alloc, "API-V2.Sub.Example.Org.")?;
    defer alloc.free(uni1);
    expect_eq(std.mem.eql(u8, uni1, "api-v2.sub.example.org."), true);

    const norm1 = normalizeDomain(alloc, "Example\u{FF0E}COM")?;
    defer alloc.free(norm1);
    expect_eq(std.mem.eql(u8, norm1, "example.com"), true);

    const norm2 = normalizeDomain(alloc, "MÜNCHEN\u{3002}DE")?;
    defer alloc.free(norm2);
    expect_eq(std.mem.eql(u8, norm2, "münchen.de"), true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Dynamic mount via `cap.idna`
    const p_enc = cap.text.punycodeEncode(alloc, "他们为什么不说中文")?;
    defer alloc.free(p_enc);
    expect_eq(std.mem.eql(u8, p_enc, "ihqwcrb4cv8a8dqg056pqjye"), true);

    const p_dec = cap.text.punycodeDecode(alloc, p_enc)?;
    defer alloc.free(p_dec);
    expect_eq(std.mem.eql(u8, p_dec, "他们为什么不说中文"), true);

    const d_asc = cap.text.toAscii(alloc, "München\u{3002}DE")?;
    defer alloc.free(d_asc);
    expect_eq(std.mem.eql(u8, d_asc, "xn--mnchen-3ya.de"), true);

    const d_uni = cap.text.toUnicode(alloc, d_asc)?;
    defer alloc.free(d_uni);
    expect_eq(std.mem.eql(u8, d_uni, "münchen.de"), true);

    const d_norm = cap.text.normalizeDomain(alloc, "Sub\u{FF61}Example.ORG")?;
    defer alloc.free(d_norm);
    expect_eq(std.mem.eql(u8, d_norm, "sub.example.org"), true);

    expect_eq(cap.text.isValidDomain("xn--mnchen-3ya.de"), true);
    expect_eq(cap.text.isValidDomain("-invalid.de"), false);
}

test("1. RFC 3492 official Punycode vectors (Arabic, Chinese Simplified/Traditional, Japanese, German)", () => {
    runTest1() catch unreachable;
});

test("2. IDNA toAscii / toUnicode multi-label domains, CJK/fullwidth dot separators, and trailing root dot", () => {
    runTest2() catch unreachable;
});

test("3. RFC 1034/1123 63/253 length limits, hyphens, invalid Punycode, integer overflow, UTF-8, and bidi checks", () => {
    runTest3() catch unreachable;
});

test("4. Case normalization and pure ASCII zero-conversion domain handling", () => {
    runTest4() catch unreachable;
});

test("5. import cap from \"cap\"; cap.idna dynamic mount and std.testing.allocator zero leak", () => {
    runTest5() catch unreachable;
});
