// Production Deep Tests for cap:css @namespace Rule & Registry (lib/css/namespace_rule.cp)
import { test, expect_eq } from "cap:test";
import {
    CSS_NAMESPACE_MAX_RULES,
    CSS_HTML_NS,
    CSS_SVG_NS,
    CSS_MATHML_NS,
    CssNamespaceError,
    NamespaceRule,
    NamespaceRegistry,
} from "cap:css";

test("css namespace_rule: default namespace parsing across string and url() forms", fn() {
    expect_eq(CSS_NAMESPACE_MAX_RULES, @as(usize, 32));

    const r1 = NamespaceRule.parse("@namespace \"http://www.w3.org/1999/xhtml\";") catch unreachable;
    expect_eq(r1.isDefault(), true);
    expect_eq(r1.url, CSS_HTML_NS);

    const r2 = NamespaceRule.parse("@NAMESPACE url('http://www.w3.org/1999/xhtml')") catch unreachable;
    expect_eq(r2.isDefault(), true);
    expect_eq(r2.url, CSS_HTML_NS);

    const r3 = NamespaceRule.parse("@namespace url(http://www.w3.org/2000/svg);") catch unreachable;
    expect_eq(r3.isDefault(), true);
    expect_eq(r3.url, CSS_SVG_NS);

    let out: [128]u8 = [0; 128];
    const n = r1.toCss(out[0..]) catch unreachable;
    expect_eq(out[0..n], "@namespace \"http://www.w3.org/1999/xhtml\";");
});

test("css namespace_rule: prefixed namespace parsing and canonical toCss", fn() {
    const r_svg = NamespaceRule.parse("@namespace svg url(\"http://www.w3.org/2000/svg\");") catch unreachable;
    expect_eq(r_svg.isDefault(), false);
    expect_eq(r_svg.prefix ?? "", "svg");
    expect_eq(r_svg.url, CSS_SVG_NS);

    const r_math = NamespaceRule.parse("@namespace math 'http://www.w3.org/1998/Math/MathML';") catch unreachable;
    expect_eq(r_math.prefix ?? "", "math");
    expect_eq(r_math.url, CSS_MATHML_NS);

    let out: [128]u8 = [0; 128];
    const n = r_svg.toCss(out[0..]) catch unreachable;
    expect_eq(out[0..n], "@namespace svg \"http://www.w3.org/2000/svg\";");
});

test("css namespace_rule: NamespaceRegistry default/prefixed registration and override", fn() {
    let reg = NamespaceRegistry.init();
    expect_eq(reg.default_url == null, true);

    * = reg.register(NamespaceRule.parse("@namespace \"http://www.w3.org/1999/xhtml\";") catch unreachable) catch unreachable;
    expect_eq(reg.default_url ?? "", CSS_HTML_NS);

    const slot_svg = reg.register(NamespaceRule.parse("@namespace svg \"http://www.w3.org/2000/svg\";") catch unreachable) catch unreachable;
    expect_eq(slot_svg, @as(usize, 0));
    expect_eq(reg.count, @as(usize, 1));
    expect_eq(reg.lookupPrefix("svg") ?? "", CSS_SVG_NS);

    // Override existing prefix in-place
    const slot_override = reg.register(NamespaceRule.parse("@namespace svg \"http://example.com/svg2\";") catch unreachable) catch unreachable;
    expect_eq(slot_override, @as(usize, 0));
    expect_eq(reg.count, @as(usize, 1));
    expect_eq(reg.lookupPrefix("svg") ?? "", "http://example.com/svg2");
});

test("css namespace_rule: resolveSelectorPrefix for default, wildcard and named prefixes", fn() {
    let reg = NamespaceRegistry.init();
    * = reg.register(NamespaceRule.parse("@namespace \"http://www.w3.org/1999/xhtml\";") catch unreachable) catch unreachable;
    * = reg.register(NamespaceRule.parse("@namespace svg \"http://www.w3.org/2000/svg\";") catch unreachable) catch unreachable;

    expect_eq(reg.resolveSelectorPrefix("") ?? "", CSS_HTML_NS);
    expect_eq(reg.resolveSelectorPrefix("*") ?? "", "*");
    expect_eq(reg.resolveSelectorPrefix("svg") ?? "", CSS_SVG_NS);
    expect_eq(reg.resolveSelectorPrefix("unknown") == null, true);
});

test("css namespace_rule: InvalidPrelude, InvalidPrefix, InvalidUrl and BufferTooSmall guards", fn() {
    // 1. Wrong at-rule prelude
    let bad_pre = false;
    * = NamespaceRule.parse("@import \"a.css\";") catch |e| {
        bad_pre = (e == CssNamespaceError.InvalidPrelude);
        NamespaceRule{}
    };
    expect_eq(bad_pre, true);

    // 2. Digit-starting prefix -> InvalidPrefix
    let bad_prefix = false;
    * = NamespaceRule.parse("@namespace 9bad \"http://example.com\";") catch |e| {
        bad_prefix = (e == CssNamespaceError.InvalidPrefix);
        NamespaceRule{}
    };
    expect_eq(bad_prefix, true);

    // 3. Empty url() -> InvalidUrl
    let bad_url = false;
    * = NamespaceRule.parse("@namespace svg url();") catch |e| {
        bad_url = (e == CssNamespaceError.InvalidUrl);
        NamespaceRule{}
    };
    expect_eq(bad_url, true);

    // 4. Unquoted url with space -> InvalidUrl
    let space_url = false;
    * = NamespaceRule.parse("@namespace url(http://a b);") catch |e| {
        space_url = (e == CssNamespaceError.InvalidUrl);
        NamespaceRule{}
    };
    expect_eq(space_url, true);

    // 5. BufferTooSmall on toCss
    const r = NamespaceRule.parse("@namespace svg \"http://www.w3.org/2000/svg\";") catch unreachable;
    let tiny: [8]u8 = [0; 8];
    let buf_small = false;
    * = r.toCss(tiny[0..]) catch |e| {
        buf_small = (e == CssNamespaceError.BufferTooSmall);
        @as(usize, 0)
    };
    expect_eq(buf_small, true);
});
