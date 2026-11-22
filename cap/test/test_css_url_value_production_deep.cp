// Production Deep Tests for cap:css url(...) Value (lib/css/url_value.cp)
import { test, expect_eq } from "cap:test";
import {
    CssUrlError,
    CssUrlKind,
    CssUrlValue,
} from "cap:css";

test("css url_value: parse unquoted, double-quoted, single-quoted, empty, and uppercase URL()", fn() {
    const u1 = CssUrlValue.parse("url(images/hero.png)") catch unreachable;
    expect_eq(u1.raw, "images/hero.png");
    expect_eq(u1.was_quoted, false);
    expect_eq(u1.needsQuoting(), false);

    const u2 = CssUrlValue.parse("  URL( \"images/hero.png\" )  ") catch unreachable;
    expect_eq(u2.raw, "images/hero.png");
    expect_eq(u2.was_quoted, true);
    expect_eq(u1.eql(u2), true);

    const u3 = CssUrlValue.parse("url('my folder/pic (1).png')") catch unreachable;
    expect_eq(u3.raw, "my folder/pic (1).png");
    expect_eq(u3.was_quoted, true);
    expect_eq(u3.needsQuoting(), true);

    const u_empty = CssUrlValue.parse("url()") catch unreachable;
    expect_eq(u_empty.raw, "");
    expect_eq(u_empty.isRelative(), true);
});

test("css url_value: RFC 3986 / WHATWG classification and isAbsolute()", fn() {
    const rel_dot = CssUrlValue.parse("url(./a/b.png)") catch unreachable;
    expect_eq(rel_dot.kind(), CssUrlKind.Relative);
    expect_eq(rel_dot.isRelative(), true);
    expect_eq(rel_dot.isAbsolute(), false);

    const rel_bare = CssUrlValue.parse("url(fonts/inter.woff2)") catch unreachable;
    expect_eq(rel_bare.kind(), CssUrlKind.Relative);
    expect_eq(rel_bare.isAbsolute(), false);

    const root_rel = CssUrlValue.parse("url(/static/logo.svg)") catch unreachable;
    expect_eq(root_rel.kind(), CssUrlKind.RootRelative);
    expect_eq(root_rel.isAbsolute(), true);

    const frag = CssUrlValue.parse("url(#clip-mask)") catch unreachable;
    expect_eq(frag.kind(), CssUrlKind.Fragment);
    expect_eq(frag.isFragment(), true);
    expect_eq(frag.isAbsolute(), true);

    const data_u = CssUrlValue.parse("url(\"DATA:image/svg+xml,<svg/>\")") catch unreachable;
    expect_eq(data_u.kind(), CssUrlKind.Data);
    expect_eq(data_u.isDataUrl(), true);
    expect_eq(data_u.isAbsolute(), true);

    const https_u = CssUrlValue.parse("url(https://cdn.example.com/a.png)") catch unreachable;
    expect_eq(https_u.kind(), CssUrlKind.AbsoluteScheme);
    expect_eq(https_u.isAbsolute(), true);

    const custom_scheme = CssUrlValue.parse("url(my+app-1.0://asset/icon)") catch unreachable;
    expect_eq(custom_scheme.kind(), CssUrlKind.AbsoluteScheme);
    expect_eq(custom_scheme.isAbsolute(), true);

    // Invalid scheme start (digit before ':') -> Relative
    const bad_scheme = CssUrlValue.parse("url(1abc:def)") catch unreachable;
    expect_eq(bad_scheme.kind(), CssUrlKind.Relative);
    expect_eq(bad_scheme.isAbsolute(), false);
});

test("css url_value: toCss vs shortest-form toCssMinified selection", fn() {
    let buf: [128]u8 = [0; 128];

    // Simple path: minified drops quotes (`url(a.png)` vs `url("a.png")`)
    const simple = CssUrlValue.parse("url(\"./a.png\")") catch unreachable;
    const n_std = simple.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n_std], "url(\"./a.png\")");
    const n_min = simple.toCssMinified(buf[0..]) catch unreachable;
    expect_eq(buf[0..n_min], "url(./a.png)");

    // 1 space: unquoted `url(a\ b)` (9 bytes) < quoted `url("a b")` (10 bytes) -> picks unquoted
    const one_space = CssUrlValue.parse("url(\"a b\")") catch unreachable;
    const n_one = one_space.toCssMinified(buf[0..]) catch unreachable;
    expect_eq(buf[0..n_one], "url(a\\ b)");

    // 3 spaces: unquoted `url(a\ b\ c\ d)` (15 bytes) > quoted `url("a b c d")` (14 bytes) -> picks quoted!
    const many_spaces = CssUrlValue.parse("url('a b c d')") catch unreachable;
    const n_many = many_spaces.toCssMinified(buf[0..]) catch unreachable;
    expect_eq(buf[0..n_many], "url(\"a b c d\")");
});

test("css url_value: malformed syntax error handling", fn() {
    // Missing url( prefix
    let e1 = false;
    * = CssUrlValue.parse("http://example.com") catch |e| blk: {
        expect_eq(e, CssUrlError.InvalidUrl);
        e1 = true;
        break :blk CssUrlValue{};
    };
    expect_eq(e1, true);

    // Missing closing paren
    let e2 = false;
    * = CssUrlValue.parse("url(foo.png") catch |e| blk: {
        expect_eq(e, CssUrlError.UnclosedParen);
        e2 = true;
        break :blk CssUrlValue{};
    };
    expect_eq(e2, true);

    // Unclosed quote
    let e3 = false;
    * = CssUrlValue.parse("url(\"foo.png)") catch |e| blk: {
        expect_eq(e, CssUrlError.UnclosedQuote);
        e3 = true;
        break :blk CssUrlValue{};
    };
    expect_eq(e3, true);

    // Unescaped space in unquoted URL
    let e4 = false;
    * = CssUrlValue.parse("url(foo bar.png)") catch |e| blk: {
        expect_eq(e, CssUrlError.UnescapedSpecialChar);
        e4 = true;
        break :blk CssUrlValue{};
    };
    expect_eq(e4, true);

    // Unescaped nested paren in unquoted URL
    let e5 = false;
    * = CssUrlValue.parse("url(foo(1).png)") catch |e| blk: {
        expect_eq(e, CssUrlError.UnescapedSpecialChar);
        e5 = true;
        break :blk CssUrlValue{};
    };
    expect_eq(e5, true);
});

test("css url_value: BufferTooSmall protection on toCss and toCssMinified", fn() {
    const u = CssUrlValue.parse("url(\"https://example.com/image.png\")") catch unreachable;
    let tiny: [8]u8 = [0; 8];

    let got_e1 = false;
    * = u.toCss(tiny[0..]) catch |e| blk: {
        expect_eq(e, CssUrlError.BufferTooSmall);
        got_e1 = true;
        break :blk 0;
    };
    expect_eq(got_e1, true);

    let got_e2 = false;
    * = u.toCssMinified(tiny[0..]) catch |e| blk: {
        expect_eq(e, CssUrlError.BufferTooSmall);
        got_e2 = true;
        break :blk 0;
    };
    expect_eq(got_e2, true);
});
