// cap/test/testCssComposes.cp
// Production-grade deep tests for cap:css composes.cp (CSS Modules `composes` property)

import { test, assert, assertEq } from "cap:test";
import {
    CSS_COMPOSES_MAX_NAMES,
    CssComposesError,
    isValidComposesSelector,
    CssComposesFromKind,
    CssComposesSpecifier,
    CssComposesProp,
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

test("CssComposes: parse local single and multiple class names", () => {
    const c1 = CssComposesProp.parse("background") catch unreachable;
    assertEq(c1.count(), 1);
    assert(c1.isLocal());
    assert(!c1.isGlobal());
    assert(!c1.isExternalFile());
    assert(c1.contains("background"));
    assert(!c1.contains("Background")); // CSS class names are case-sensitive

    const c2 = CssComposesProp.parse("  btn-primary   elevated_card  _helper  ") catch unreachable;
    assertEq(c2.count(), 3);
    assert(c2.isLocal());
    if (n0 := c2.get(0)) {
        assert(eqStr(n0, "btn-primary"));
    } else {
        assert(false);
    }
    if (n2 := c2.get(2)) {
        assert(eqStr(n2, "_helper"));
    } else {
        assert(false);
    }
    assert(c2.get(3) == null);

    let buf: [96]u8 = [0; 96];
    const written = c2.toCss(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..written], "btn-primary elevated_card _helper"));
});

test("CssComposes: parse global composition case-insensitively", () => {
    const c = CssComposesProp.parse("clearfix reset-box FROM GLOBAL") catch unreachable;
    assertEq(c.count(), 2);
    assert(c.isGlobal());
    assert(c.from_spec.kind == CssComposesFromKind.global);
    assert(c.contains("clearfix"));
    assert(c.contains("reset-box"));

    let buf: [64]u8 = [0; 64];
    const n = c.toCss(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n], "clearfix reset-box from global"));
});

test("CssComposes: parse external file specifier via string and url()", () => {
    const c1 = CssComposesProp.parse("background from \"./background.module.css\"") catch unreachable;
    assertEq(c1.count(), 1);
    assert(c1.isExternalFile());
    assert(eqStr(c1.from_spec.file_path, "./background.module.css"));

    const c2 = CssComposesProp.parse("a b from './shared/base.module.css'") catch unreachable;
    assertEq(c2.count(), 2);
    assert(c2.isExternalFile());
    assert(eqStr(c2.from_spec.file_path, "./shared/base.module.css"));

    const c3 = CssComposesProp.parse("card from url(\"./theme.module.css\")") catch unreachable;
    assert(c3.isExternalFile());
    assert(eqStr(c3.from_spec.file_path, "./theme.module.css"));

    const c4 = CssComposesProp.parse("card from URL(./raw.module.css)") catch unreachable;
    assert(c4.isExternalFile());
    assert(eqStr(c4.from_spec.file_path, "./raw.module.css"));

    let buf: [96]u8 = [0; 96];
    const n = c2.toCss(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n], "a b from \"./shared/base.module.css\""));
});

test("CssComposes: equality and specifier construction", () => {
    const a = CssComposesProp.parse("foo bar from \"./x.css\"") catch unreachable;
    const b = CssComposesProp.parse("  foo   bar  FROM  './x.css' ") catch unreachable;
    const c = CssComposesProp.parse("foo bar from global") catch unreachable;
    assert(a.eql(b));
    assert(!a.eql(c));

    const s1 = CssComposesSpecifier.file("./a.css", 3);
    const s2 = CssComposesSpecifier.file("./a.css", 3);
    const s3 = CssComposesSpecifier.file("./a.css", 4);
    assert(s1.eql(s2));
    assert(!s1.eql(s3));
    assert(CssComposesSpecifier.local().isLocal());
    assert(CssComposesSpecifier.global().isGlobal());
});

test("CssComposes: Bun CSS Modules selector eligibility validation", () => {
    // Valid simple single-class selectors
    assert(isValidComposesSelector(".button"));
    assert(isValidComposesSelector("  .card-header_2  "));
    assert(isValidComposesSelector(".-webkit-like"));

    // Invalid selectors per Bun/CSS Modules rules
    assert(!isValidComposesSelector("#button"));
    assert(!isValidComposesSelector(".button, .button-secondary"));
    assert(!isValidComposesSelector(".parent .child"));
    assert(!isValidComposesSelector(".a.b"));
    assert(!isValidComposesSelector(".button:hover"));
    assert(!isValidComposesSelector("div.button"));
    assert(!isValidComposesSelector(".--custom"));
    assert(!isValidComposesSelector(""));
});

test("CssComposes: malformed input, reserved idents, overflow and buffer guards", () => {
    assertEq(CSS_COMPOSES_MAX_NAMES, 16);

    let empty_err = false;
    * = CssComposesProp.parse("   ") catch |e| {
        empty_err = (e == CssComposesError.EmptyInput);
    };
    assert(empty_err);

    // Missing class name before `from`
    let missing_name_err = false;
    * = CssComposesProp.parse("from global") catch |e| {
        missing_name_err = (e == CssComposesError.MissingClassName);
    };
    assert(missing_name_err);

    // Reserved CSS-wide keyword or `--` custom property rejected
    let reserved_err = false;
    * = CssComposesProp.parse("inherit") catch |e| {
        reserved_err = (e == CssComposesError.InvalidIdent);
    };
    assert(reserved_err);

    let custom_prop_err = false;
    * = CssComposesProp.parse("--token") catch |e| {
        custom_prop_err = (e == CssComposesError.InvalidIdent);
    };
    assert(custom_prop_err);

    // Missing specifier after `from`
    let missing_spec_err = false;
    * = CssComposesProp.parse("foo from   ") catch |e| {
        missing_spec_err = (e == CssComposesError.MissingSpecifier);
    };
    assert(missing_spec_err);

    // Invalid specifier (unquoted ident other than `global`, or unclosed string)
    let bad_spec_err = false;
    * = CssComposesProp.parse("foo from local") catch |e| {
        bad_spec_err = (e == CssComposesError.InvalidSpecifier);
    };
    assert(bad_spec_err);

    let unclosed_err = false;
    * = CssComposesProp.parse("foo from \"./unclosed.css") catch |e| {
        unclosed_err = (e == CssComposesError.InvalidSpecifier);
    };
    assert(unclosed_err);

    // Trailing tokens after specifier
    let trailing_err = false;
    * = CssComposesProp.parse("foo from global extra") catch |e| {
        trailing_err = (e == CssComposesError.TrailingTokens);
    };
    assert(trailing_err);

    // >16 class names rejected
    let too_many_err = false;
    * = CssComposesProp.parse("a1 a2 a3 a4 a5 a6 a7 a8 a9 a10 a11 a12 a13 a14 a15 a16 a17") catch |e| {
        too_many_err = (e == CssComposesError.TooManyNames);
    };
    assert(too_many_err);

    // Tiny output buffer rejected
    let tiny: [4]u8 = [0; 4];
    let buf_err = false;
    const valid = CssComposesProp.parse("alpha beta") catch unreachable;
    * = valid.toCss(tiny[0..]) catch |e| {
        buf_err = (e == CssComposesError.BufferTooSmall);
    };
    assert(buf_err);
});
