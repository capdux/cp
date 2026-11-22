// Production Deep Tests for cap:css Values Ident (lib/css/ident.cp)
import { test, expect_eq } from "cap:test";
import {
    CssIdentError,
    isCssWideKeyword,
    serializeCssIdentifier,
    CssIdent,
    CustomIdent,
    DashedIdent,
    CssIdentFromKind,
    DashedIdentReference,
} from "cap:css";

test("css ident: CssIdent parsing, equality, and invalid syntax rejection", fn() {
    const id1 = CssIdent.parse("  grid-area_1  ") catch unreachable;
    const id2 = CssIdent.parse("grid-area_1") catch unreachable;
    const id3 = CssIdent.parse("-webkit-box") catch unreachable;
    const id4 = CssIdent.parse("--custom") catch unreachable;
    expect_eq(id1.eql(id2), true);
    expect_eq(id1.eql(id3), false);
    expect_eq(id4.value, "--custom");

    // Empty -> EmptyIdent
    let err_empty = false;
    * = CssIdent.parse("   ") catch |e| blk: {
        expect_eq(e, CssIdentError.EmptyIdent);
        err_empty = true;
        break :blk CssIdent{};
    };
    expect_eq(err_empty, true);

    // Leading digit, single hyphen, or hyphen + digit -> InvalidIdent
    let err_digit = false;
    * = CssIdent.parse("123abc") catch |e| blk: {
        expect_eq(e, CssIdentError.InvalidIdent);
        err_digit = true;
        break :blk CssIdent{};
    };
    expect_eq(err_digit, true);

    let err_dash = false;
    * = CssIdent.parse("-") catch |e| blk: {
        expect_eq(e, CssIdentError.InvalidIdent);
        err_dash = true;
        break :blk CssIdent{};
    };
    expect_eq(err_dash, true);

    let err_dash_digit = false;
    * = CssIdent.parse("-9px") catch |e| blk: {
        expect_eq(e, CssIdentError.InvalidIdent);
        err_dash_digit = true;
        break :blk CssIdent{};
    };
    expect_eq(err_dash_digit, true);
});

test("css ident: CustomIdent rejects all 6 CSS-wide keywords case-insensitively", fn() {
    expect_eq(isCssWideKeyword("initial"), true);
    expect_eq(isCssWideKeyword("INHERIT"), true);
    expect_eq(isCssWideKeyword("Unset"), true);
    expect_eq(isCssWideKeyword("default"), true);
    expect_eq(isCssWideKeyword("revert"), true);
    expect_eq(isCssWideKeyword("REVERT-LAYER"), true);
    expect_eq(isCssWideKeyword("auto"), false);
    expect_eq(isCssWideKeyword("slide-in"), false);

    const ok = CustomIdent.parse("fadeIn") catch unreachable;
    expect_eq(ok.value, "fadeIn");

    let got_reserved = false;
    * = CustomIdent.parse("Revert-Layer") catch |e| blk: {
        expect_eq(e, CssIdentError.ReservedKeyword);
        got_reserved = true;
        break :blk CustomIdent{};
    };
    expect_eq(got_reserved, true);

    let got_default = false;
    * = CustomIdent.parse("DEFAULT") catch |e| blk: {
        expect_eq(e, CssIdentError.ReservedKeyword);
        got_default = true;
        break :blk CustomIdent{};
    };
    expect_eq(got_default, true);
});

test("css ident: DashedIdent parsing, nameWithoutPrefix, and validation", fn() {
    const d = DashedIdent.parse("--brand-accent-500") catch unreachable;
    expect_eq(d.value, "--brand-accent-500");
    expect_eq(d.nameWithoutPrefix(), "brand-accent-500");

    let buf: [64]u8 = [0; 64];
    const n = d.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n], "--brand-accent-500");

    // Missing `--` prefix -> MissingDoubleDash
    let err_prefix = false;
    * = DashedIdent.parse("-single-dash") catch |e| blk: {
        expect_eq(e, CssIdentError.MissingDoubleDash);
        err_prefix = true;
        break :blk DashedIdent{};
    };
    expect_eq(err_prefix, true);

    // Bare `--` -> InvalidIdent
    let err_bare = false;
    * = DashedIdent.parse("--") catch |e| blk: {
        expect_eq(e, CssIdentError.InvalidIdent);
        err_bare = true;
        break :blk DashedIdent{};
    };
    expect_eq(err_bare, true);
});

test("css ident: DashedIdentReference with plain, from global, and from file", fn() {
    let buf: [128]u8 = [0; 128];

    const r_plain = DashedIdentReference.parse("--primary") catch unreachable;
    expect_eq(r_plain.hasFrom(), false);
    expect_eq(r_plain.isFromGlobal(), false);
    expect_eq(r_plain.from_kind, CssIdentFromKind.None);
    const n0 = r_plain.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n0], "--primary");

    const r_global = DashedIdentReference.parse("--theme-bg FROM global") catch unreachable;
    expect_eq(r_global.hasFrom(), true);
    expect_eq(r_global.isFromGlobal(), true);
    expect_eq(r_global.from_kind, CssIdentFromKind.Global);
    const n1 = r_global.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n1], "--theme-bg from global");

    const r_file = DashedIdentReference.parse("--accent from './tokens.css'") catch unreachable;
    expect_eq(r_file.hasFrom(), true);
    expect_eq(r_file.isFromGlobal(), false);
    expect_eq(r_file.from_kind, CssIdentFromKind.File);
    expect_eq(r_file.from_file, "./tokens.css");
    const n2 = r_file.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n2], "--accent from \"./tokens.css\"");

    const r_file2 = DashedIdentReference.parse("--accent from \"./tokens.css\"") catch unreachable;
    expect_eq(r_file.eql(r_file2), true);
    expect_eq(r_file.eql(r_global), false);

    // Invalid from clause
    let err_from = false;
    * = DashedIdentReference.parse("--accent from \"\"") catch |e| blk: {
        expect_eq(e, CssIdentError.InvalidFromClause);
        err_from = true;
        break :blk DashedIdentReference{};
    };
    expect_eq(err_from, true);

    let err_kw = false;
    * = DashedIdentReference.parse("--accent in global") catch |e| blk: {
        expect_eq(e, CssIdentError.InvalidFromClause);
        err_kw = true;
        break :blk DashedIdentReference{};
    };
    expect_eq(err_kw, true);
});

test("css ident: W3C CSSOM serializeCssIdentifier escaping and BufferTooSmall", fn() {
    let buf: [64]u8 = [0; 64];

    // Leading digit
    const n1 = serializeCssIdentifier("1a", buf[0..]) catch unreachable;
    expect_eq(buf[0..n1], "\\31 a");

    // Leading hyphen + digit
    const n2 = serializeCssIdentifier("-2b", buf[0..]) catch unreachable;
    expect_eq(buf[0..n2], "-\\32 b");

    // Single hyphen
    const n3 = serializeCssIdentifier("-", buf[0..]) catch unreachable;
    expect_eq(buf[0..n3], "\\-");

    // Tailwind-style punctuation escaping
    const n4 = serializeCssIdentifier("sm:w-1/2", buf[0..]) catch unreachable;
    expect_eq(buf[0..n4], "sm\\:w-1\\/2");

    // Control byte (0x07) -> \7 + space
    const ctrl: [2]u8 = [0x07, 'x'];
    const n5 = serializeCssIdentifier(ctrl[0..], buf[0..]) catch unreachable;
    expect_eq(buf[0..n5], "\\7 x");

    // NUL byte -> UTF-8 U+FFFD (EF BF BD)
    const nul_in: [1]u8 = [0x00];
    const n6 = serializeCssIdentifier(nul_in[0..], buf[0..]) catch unreachable;
    expect_eq(n6, @as(usize, 3));
    expect_eq(buf[0], @as(u8, 0xEF));
    expect_eq(buf[1], @as(u8, 0xBF));
    expect_eq(buf[2], @as(u8, 0xBD));

    // BufferTooSmall protection
    let tiny: [3]u8 = [0; 3];
    let got_small = false;
    * = serializeCssIdentifier("12345", tiny[0..]) catch |e| blk: {
        expect_eq(e, CssIdentError.BufferTooSmall);
        got_small = true;
        break :blk 0;
    };
    expect_eq(got_small, true);
});
