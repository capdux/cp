// Production Deep Tests for cap:css @custom-media Rule & Expander (lib/css/custom_media.cp)
import { test, expect_eq } from "cap:test";
import {
    CUSTOM_MEDIA_MAX_RULES,
    CustomMediaError,
    CustomMediaValueKind,
    CustomMediaRule,
    CustomMediaRegistry,
} from "cap:css";

test("css custom_media: parse and toCss roundtrip for standard and case-insensitive rules", fn() {
    expect_eq(CUSTOM_MEDIA_MAX_RULES, @as(usize, 64));

    const r1 = CustomMediaRule.parse("@custom-media --narrow-window (max-width: 30em);") catch unreachable;
    expect_eq(r1.name, "--narrow-window");
    expect_eq(r1.kind, CustomMediaValueKind.query);
    expect_eq(r1.query, "(max-width: 30em)");

    let out: [128]u8 = [0; 128];
    const n1 = r1.toCss(out[0..]) catch unreachable;
    expect_eq(out[0..n1], "@custom-media --narrow-window (max-width: 30em);");

    // Case-insensitive @CUSTOM-MEDIA without trailing semicolon
    const r2 = CustomMediaRule.parse("  @CUSTOM-MEDIA   --dark   (prefers-color-scheme: dark)  ") catch unreachable;
    expect_eq(r2.name, "--dark");
    expect_eq(r2.query, "(prefers-color-scheme: dark)");
    const n2 = r2.toCss(out[0..]) catch unreachable;
    expect_eq(out[0..n2], "@custom-media --dark (prefers-color-scheme: dark);");
});

test("css custom_media: boolean true/false media rules", fn() {
    const rt = CustomMediaRule.parse("@custom-media --modern TRUE;") catch unreachable;
    expect_eq(rt.kind, CustomMediaValueKind.always_true);
    expect_eq(rt.query, "");

    const rf = CustomMediaRule.parse("@custom-media --legacy False;") catch unreachable;
    expect_eq(rf.kind, CustomMediaValueKind.always_false);

    let out: [64]u8 = [0; 64];
    const nt = rt.toCss(out[0..]) catch unreachable;
    expect_eq(out[0..nt], "@custom-media --modern true;");

    const nf = rf.toCss(out[0..]) catch unreachable;
    expect_eq(out[0..nf], "@custom-media --legacy false;");
});

test("css custom_media: CustomMediaRegistry cascade override and lookup", fn() {
    let reg = CustomMediaRegistry.init();

    const s0 = reg.register(CustomMediaRule.parse("@custom-media --bp (max-width: 600px);") catch unreachable) catch unreachable;
    const s1 = reg.register(CustomMediaRule.parse("@custom-media --dark (prefers-color-scheme: dark);") catch unreachable) catch unreachable;
    expect_eq(s0, @as(usize, 0));
    expect_eq(s1, @as(usize, 1));
    expect_eq(reg.count, @as(usize, 2));

    // Later declaration overrides earlier one in-place
    const s_override = reg.register(CustomMediaRule.parse("@custom-media --bp (max-width: 768px);") catch unreachable) catch unreachable;
    expect_eq(s_override, @as(usize, 0));
    expect_eq(reg.count, @as(usize, 2));

    const found = reg.lookup("--bp") ?? unreachable;
    expect_eq(found.query, "(max-width: 768px)");
    expect_eq(reg.lookup("--missing") == null, true);
});

test("css custom_media: resolveMediaQuery expands queries, booleans and preserves unknown names", fn() {
    let reg = CustomMediaRegistry.init();
    * = reg.register(CustomMediaRule.parse("@custom-media --narrow (max-width: 30em);") catch unreachable) catch unreachable;
    * = reg.register(CustomMediaRule.parse("@custom-media --dark (prefers-color-scheme: dark);") catch unreachable) catch unreachable;
    * = reg.register(CustomMediaRule.parse("@custom-media --on true;") catch unreachable) catch unreachable;
    * = reg.register(CustomMediaRule.parse("@custom-media --off false;") catch unreachable) catch unreachable;

    let out: [160]u8 = [0; 160];

    const n1 = reg.resolveMediaQuery("screen and (--narrow) and (--dark)", out[0..]) catch unreachable;
    expect_eq(out[0..n1], "screen and (max-width: 30em) and (prefers-color-scheme: dark)");

    const n2 = reg.resolveMediaQuery("(--on), (--off), (--unknown)", out[0..]) catch unreachable;
    expect_eq(out[0..n2], "all, not all, (--unknown)");
});

test("css custom_media: InvalidPrelude, InvalidDashedIdent, EmptyMediaQuery and BufferTooSmall guards", fn() {
    // 1. Wrong at-rule prelude
    let bad_pre = false;
    * = CustomMediaRule.parse("@media --foo (min-width: 1px);") catch |e| {
        bad_pre = (e == CustomMediaError.InvalidPrelude);
        CustomMediaRule{}
    };
    expect_eq(bad_pre, true);

    // 2. Name without '--' prefix
    let bad_ident = false;
    * = CustomMediaRule.parse("@custom-media narrow (max-width: 30em);") catch |e| {
        bad_ident = (e == CustomMediaError.InvalidDashedIdent);
        CustomMediaRule{}
    };
    expect_eq(bad_ident, true);

    // 3. Bare '--' too short
    let bare_dash = false;
    * = CustomMediaRule.parse("@custom-media -- (max-width: 30em);") catch |e| {
        bare_dash = (e == CustomMediaError.InvalidDashedIdent);
        CustomMediaRule{}
    };
    expect_eq(bare_dash, true);

    // 4. Empty media query body
    let empty_q = false;
    * = CustomMediaRule.parse("@custom-media --empty ;") catch |e| {
        empty_q = (e == CustomMediaError.EmptyMediaQuery);
        CustomMediaRule{}
    };
    expect_eq(empty_q, true);

    // 5. BufferTooSmall on toCss
    const r = CustomMediaRule.parse("@custom-media --narrow (max-width: 30em);") catch unreachable;
    let tiny: [10]u8 = [0; 10];
    let buf_small = false;
    * = r.toCss(tiny[0..]) catch |e| {
        buf_small = (e == CustomMediaError.BufferTooSmall);
        @as(usize, 0)
    };
    expect_eq(buf_small, true);
});
