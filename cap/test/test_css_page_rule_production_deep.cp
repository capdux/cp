import { test, expect_eq } from "cap:test";
import {
    CssPageError,
    PagePseudoClass,
    PageMarginEdge,
    PageMarginBox,
    PageSelector,
    PagePosition as Position,
} from "cap:css";

test("1. All 16 PageMarginBox variants parse, CSS serialize & edge/corner classification", () => {
    const names: [16]string = [
        "top-left-corner",
        "top-left",
        "top-center",
        "top-right",
        "top-right-corner",
        "left-top",
        "left-middle",
        "left-bottom",
        "right-top",
        "right-middle",
        "right-bottom",
        "bottom-left-corner",
        "bottom-left",
        "bottom-center",
        "bottom-right",
        "bottom-right-corner",
    ];

    for (n of names) {
        const box = PageMarginBox.parse(n).?;
        expect_eq(std.mem.eql(u8, box.toCss(), n), true);
        const at_box = PageMarginBox.parse(box.toAtRuleCss()).?;
        expect_eq(at_box == box, true);
    }

    // Corner & edge predicates
    expect_eq(PageMarginBox.top_left_corner.isCorner(), true);
    expect_eq(PageMarginBox.top_left_corner.isTop(), true);
    expect_eq(PageMarginBox.top_left_corner.isLeft(), true);
    expect_eq(PageMarginBox.top_left_corner.edge() == PageMarginEdge.corner, true);

    expect_eq(PageMarginBox.top_center.isCorner(), false);
    expect_eq(PageMarginBox.top_center.edge() == PageMarginEdge.top, true);
    expect_eq(PageMarginBox.left_middle.edge() == PageMarginEdge.left, true);
    expect_eq(PageMarginBox.right_bottom.edge() == PageMarginEdge.right, true);
    expect_eq(PageMarginBox.bottom_center.edge() == PageMarginEdge.bottom, true);

    // Case-insensitive with leading '@'
    expect_eq(PageMarginBox.parse("@BOTTOM-RIGHT-CORNER").? == PageMarginBox.bottom_right_corner, true);
});

test("2. PagePseudoClass parsing & serialization (:left, :right, :first, :last, :blank)", () => {
    expect_eq(PagePseudoClass.parse(":left").? == PagePseudoClass.left, true);
    expect_eq(PagePseudoClass.parse("RIGHT").? == PagePseudoClass.right, true);
    expect_eq(PagePseudoClass.parse(":First").? == PagePseudoClass.first, true);
    expect_eq(PagePseudoClass.parse("last").? == PagePseudoClass.last, true);
    expect_eq(PagePseudoClass.parse(":BLANK").? == PagePseudoClass.blank, true);

    expect_eq(std.mem.eql(u8, PagePseudoClass.first.toCss(), "first"), true);
    expect_eq(std.mem.eql(u8, PagePseudoClass.blank.toSelectorCss(), ":blank"), true);
    expect_eq(PagePseudoClass.parse(":hover") == null, true);
});

test("3. PageSelector combined name + pseudo-classes parse & CSS serialization", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    // Named page + multiple pseudo classes
    const s1 = PageSelector.parse("chapter:first:right") catch unreachable;
    expect_eq(s1.isAnonymous(), false);
    expect_eq(std.mem.eql(u8, s1.name.?, "chapter"), true);
    expect_eq(s1.pseudo_count, usize(2));
    expect_eq(s1.hasPseudo(PagePseudoClass.first), true);
    expect_eq(s1.hasPseudo(PagePseudoClass.right), true);
    expect_eq(s1.hasPseudo(PagePseudoClass.blank), false);

    const n1 = s1.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "chapter:first:right"), true);

    const n_rule = s1.formatAtPageRule(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n_rule], "@page chapter:first:right"), true);

    // Anonymous pseudo-only selector
    const s2 = PageSelector.parse(":left:blank") catch unreachable;
    expect_eq(s2.isAnonymous(), true);
    expect_eq(s2.pseudo_count, usize(2));
    const n2 = s2.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n2], ":left:blank"), true);

    // Name-only selector
    const s3 = PageSelector.parse("landscape-page") catch unreachable;
    expect_eq(s3.pseudo_count, usize(0));
    expect_eq(std.mem.eql(u8, s3.name.?, "landscape-page"), true);
});

test("4. Position parsing (static/relative/absolute/fixed/sticky/-webkit-sticky) & layout predicates", () => {
    const p_static = Position.parse("static").?;
    expect_eq(p_static == Position.default(), true);
    expect_eq(p_static.isPositioned(), false);
    expect_eq(p_static.isOutOfFlow(), false);
    expect_eq(p_static.createsStackingContext(), false);
    expect_eq(std.mem.eql(u8, p_static.toCss(), "static"), true);

    const p_rel = Position.parse("RELATIVE").?;
    expect_eq(p_rel.isPositioned(), true);
    expect_eq(p_rel.isOutOfFlow(), false);

    const p_abs = Position.parse("absolute").?;
    expect_eq(p_abs.isOutOfFlow(), true);
    expect_eq(p_abs.isPositioned(), true);

    const p_fix = Position.parse("fixed").?;
    expect_eq(p_fix.isOutOfFlow(), true);
    expect_eq(p_fix.createsStackingContext(), true);

    const p_sticky = Position.parse("sticky").?;
    expect_eq(p_sticky.isSticky(), true);
    expect_eq(p_sticky.isWebKitPrefixed(), false);
    expect_eq(p_sticky.createsStackingContext(), true);

    const p_wk = Position.parse("-WebKit-Sticky").?;
    expect_eq(p_wk.isSticky(), true);
    expect_eq(p_wk.isWebKitPrefixed(), true);
    expect_eq(std.mem.eql(u8, p_wk.toCss(), "-webkit-sticky"), true);
});

test("5. Malformed input & buffer boundary guards", () => {
    // Invalid PageSelector cases
    std.testing.expectError(CssPageError.InvalidPageSelector, PageSelector.parse("")) catch unreachable;
    std.testing.expectError(CssPageError.InvalidPageSelector, PageSelector.parse("123invalid")) catch unreachable;
    std.testing.expectError(CssPageError.InvalidPageSelector, PageSelector.parse("name :left")) catch unreachable;
    std.testing.expectError(CssPageError.InvalidPseudoClass, PageSelector.parse(":")) catch unreachable;
    std.testing.expectError(CssPageError.InvalidPseudoClass, PageSelector.parse("toc:unknown")) catch unreachable;
    std.testing.expectError(CssPageError.InvalidPseudoClass, PageSelector.parse(":left::right")) catch unreachable;

    // BufferTooSmall guard on toCss / formatAtPageRule
    const sel = PageSelector.parse("appendix:first") catch unreachable;
    let tiny: [4]u8 = [0, 0, 0, 0];
    std.testing.expectError(CssPageError.BufferTooSmall, sel.toCss(tiny[0..])) catch unreachable;
    std.testing.expectError(CssPageError.BufferTooSmall, sel.formatAtPageRule(tiny[0..])) catch unreachable;

    // Invalid MarginBox & Position
    expect_eq(PageMarginBox.parse("") == null, true);
    expect_eq(PageMarginBox.parse("top-middle") == null, true);
    expect_eq(Position.parse("") == null, true);
    expect_eq(Position.parse("center") == null, true);
});
