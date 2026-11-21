import { test, expect_eq } from "cap:test";
import {
    CssBackgroundError,
    BackgroundRepeatKeyword,
    BackgroundRepeat,
    BackgroundSizeKind,
    BackgroundSize,
    BackgroundAttachment,
    BackgroundOrigin,
    BackgroundClip,
    BackgroundPosition,
    BackgroundLayer,
    Background,
} from "cap:css";

test("1. BackgroundRepeat (repeat-x/repeat-y/dual-axis) & BackgroundSize (cover/contain/explicit)", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const rx = BackgroundRepeat.parse("repeat-x") catch unreachable;
    expect_eq(rx.x == BackgroundRepeatKeyword.repeat, true);
    expect_eq(rx.y == BackgroundRepeatKeyword.no_repeat, true);
    const n1 = rx.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "repeat-x"), true);

    const dual = BackgroundRepeat.parse("no-repeat repeat") catch unreachable;
    const n2 = dual.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n2], "repeat-y"), true);

    const same = BackgroundRepeat.parse("round round") catch unreachable;
    const n3 = same.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n3], "round"), true);

    // BackgroundSize
    const s_cov = BackgroundSize.parse("COVER") catch unreachable;
    expect_eq(s_cov.kind == BackgroundSizeKind.cover, true);
    const s_exp = BackgroundSize.parse("120px auto") catch unreachable;
    const ns = s_exp.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..ns], "120px"), true);
});

test("2. BackgroundAttachment, BackgroundOrigin, BackgroundClip & BackgroundPosition", () => {
    expect_eq(BackgroundAttachment.default() == BackgroundAttachment.scroll, true);
    expect_eq(BackgroundAttachment.parse("local").? == BackgroundAttachment.local, true);

    expect_eq(BackgroundOrigin.default() == BackgroundOrigin.padding_box, true);
    expect_eq(BackgroundOrigin.parse("content-box").? == BackgroundOrigin.content_box, true);

    expect_eq(BackgroundClip.default() == BackgroundClip.border_box, true);
    expect_eq(BackgroundClip.parse("text").? == BackgroundClip.text, true);
    expect_eq(BackgroundClip.parse("border-area").? == BackgroundClip.border_area, true);
    expect_eq(BackgroundClip.text.isBackgroundBox(), false);
    expect_eq(BackgroundClip.content_box.eqlOrigin(BackgroundOrigin.content_box), true);

    // Vertical keyword first: `top right` -> x=100%, y=0%
    const pos = BackgroundPosition.parse("top right") catch unreachable;
    expect_eq(pos.x.value, 100.0);
    expect_eq(pos.y.value, 0.0);
});

test("3. BackgroundLayer shorthand with position/size slash & dual-box (origin/clip) rules", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    // Single box sets both origin & clip to content-box
    const l1 = BackgroundLayer.parse("url(hero.png) center / cover no-repeat fixed content-box", true) catch unreachable;
    expect_eq(std.mem.eql(u8, l1.image, "url(hero.png)"), true);
    expect_eq(l1.size.kind == BackgroundSizeKind.cover, true);
    expect_eq(l1.repeat.x == BackgroundRepeatKeyword.no_repeat, true);
    expect_eq(l1.attachment == BackgroundAttachment.fixed, true);
    expect_eq(l1.origin == BackgroundOrigin.content_box, true);
    expect_eq(l1.clip == BackgroundClip.content_box, true);

    const n1 = l1.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "url(hero.png) 50% 50% / cover no-repeat fixed content-box"), true);

    // Two boxes: first is origin, second is clip
    const l2 = BackgroundLayer.parse("border-box padding-box", true) catch unreachable;
    expect_eq(l2.origin == BackgroundOrigin.border_box, true);
    expect_eq(l2.clip == BackgroundClip.padding_box, true);
    const n2 = l2.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n2], "border-box padding-box"), true);

    // Default layer serializes cleanly to "none"
    const def_l = BackgroundLayer.default();
    expect_eq(def_l.isDefault(), true);
    const nd = def_l.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..nd], "none"), true);
});

test("4. Multi-layer Background shorthand (commas inside functions preserved, color only on final layer)", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);

    const bg = Background.parse("url(a.png) no-repeat, linear-gradient(red, blue) #ffffff") catch unreachable;
    expect_eq(bg.len, usize(2));
    expect_eq(bg.layers[0].color == null, true);
    expect_eq(std.mem.eql(u8, bg.layers[1].color.?, "#ffffff"), true);
    expect_eq(std.mem.eql(u8, bg.layers[1].image, "linear-gradient(red, blue)"), true);

    const n = bg.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n], "url(a.png) no-repeat, #ffffff linear-gradient(red, blue)"), true);
});

test("5. Invalid syntax, slash without position, non-final layer color & negative size guards", () => {
    // Color on non-final layer forbidden by CSS Backgrounds Level 3
    std.testing.expectError(
        CssBackgroundError.InvalidBackground,
        Background.parse("#fff url(a.png), url(b.png)"),
    ) catch unreachable;

    // Slash without preceding position
    std.testing.expectError(
        CssBackgroundError.InvalidBackground,
        BackgroundLayer.parse("/ cover", true),
    ) catch unreachable;

    // Empty / trailing comma / unbalanced parens
    std.testing.expectError(CssBackgroundError.InvalidBackground, Background.parse("")) catch unreachable;
    std.testing.expectError(CssBackgroundError.InvalidBackground, Background.parse("url(a.png), ")) catch unreachable;
    std.testing.expectError(CssBackgroundError.InvalidBackground, Background.parse("url(a.png")) catch unreachable;

    // Negative size & invalid repeat
    std.testing.expectError(CssBackgroundError.InvalidSize, BackgroundSize.parse("-10px")) catch unreachable;
    std.testing.expectError(CssBackgroundError.InvalidRepeat, BackgroundRepeat.parse("repeat-z")) catch unreachable;
});
