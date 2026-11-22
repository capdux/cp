// Production Deep Tests for cap:css BoxRect & Size2D (lib/css/box_rect.cp)
import { test, expect_eq } from "cap:test";
import {
    CssBoxRectError,
    CssBoxRect,
    CssSize2D,
} from "cap:css";

test("css box_rect: 1, 2, 3, and 4-component parsing and equality", fn() {
    const r1 = CssBoxRect.parse("16px") catch unreachable;
    expect_eq(r1.top, "16px");
    expect_eq(r1.right, "16px");
    expect_eq(r1.bottom, "16px");
    expect_eq(r1.left, "16px");
    expect_eq(r1.isUniform(), true);
    expect_eq(r1.isSymmetric(), true);
    expect_eq(r1.minimalComponentCount(), @as(usize, 1));
    expect_eq(r1.eql(CssBoxRect.all("16px")), true);

    const r2 = CssBoxRect.parse("8px 16px") catch unreachable;
    expect_eq(r2.top, "8px");
    expect_eq(r2.right, "16px");
    expect_eq(r2.bottom, "8px");
    expect_eq(r2.left, "16px");
    expect_eq(r2.isUniform(), false);
    expect_eq(r2.isSymmetric(), true);
    expect_eq(r2.minimalComponentCount(), @as(usize, 2));
    expect_eq(r2.eql(CssBoxRect.symmetric("8px", "16px")), true);

    const r3 = CssBoxRect.parse("4px 12px 8px") catch unreachable;
    expect_eq(r3.top, "4px");
    expect_eq(r3.right, "12px");
    expect_eq(r3.bottom, "8px");
    expect_eq(r3.left, "12px");
    expect_eq(r3.isSymmetric(), false);
    expect_eq(r3.minimalComponentCount(), @as(usize, 3));

    const r4 = CssBoxRect.parse("1px 2px 3px 4px") catch unreachable;
    expect_eq(r4.top, "1px");
    expect_eq(r4.right, "2px");
    expect_eq(r4.bottom, "3px");
    expect_eq(r4.left, "4px");
    expect_eq(r4.minimalComponentCount(), @as(usize, 4));
});

test("css box_rect: minimal-component collapsing toCss vs toCssExpanded", fn() {
    let buf: [128]u8 = [0; 128];

    // 4 identical -> 1 value
    const r1 = CssBoxRect.parse("10px 10px 10px 10px") catch unreachable;
    const n1 = r1.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n1], "10px");
    const n1_exp = r1.toCssExpanded(buf[0..]) catch unreachable;
    expect_eq(buf[0..n1_exp], "10px 10px 10px 10px");

    // top==bottom && right==left -> 2 values
    const r2 = CssBoxRect.parse("10px 20px 10px 20px") catch unreachable;
    const n2 = r2.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n2], "10px 20px");

    // right==left, top!=bottom -> 3 values
    const r3 = CssBoxRect.parse("10px 20px 30px 20px") catch unreachable;
    const n3 = r3.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n3], "10px 20px 30px");

    // All distinct -> 4 values
    const r4 = CssBoxRect.parse("10px 20px 30px 40px") catch unreachable;
    const n4 = r4.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n4], "10px 20px 30px 40px");
});

test("css box_rect: calc(), var(), and nested function values with internal whitespace", fn() {
    const r = CssBoxRect.parse(
        "  calc(10px + 2rem)   var(--pad-x, 16px)   min(4vh, 24px)  "
    ) catch unreachable;
    expect_eq(r.top, "calc(10px + 2rem)");
    expect_eq(r.right, "var(--pad-x, 16px)");
    expect_eq(r.bottom, "min(4vh, 24px)");
    expect_eq(r.left, "var(--pad-x, 16px)");
    expect_eq(r.minimalComponentCount(), @as(usize, 3));

    let buf: [128]u8 = [0; 128];
    const n = r.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n], "calc(10px + 2rem) var(--pad-x, 16px) min(4vh, 24px)");
});

test("css box_rect: CssSize2D 1/2-component parsing and minimal serialization", fn() {
    let buf: [64]u8 = [0; 64];

    const s1 = CssSize2D.parse("12px") catch unreachable;
    expect_eq(s1.isUniform(), true);
    expect_eq(s1.eql(CssSize2D.both("12px")), true);
    const n1 = s1.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n1], "12px");

    // Two identical values collapse to 1
    const s2 = CssSize2D.parse("1rem 1rem") catch unreachable;
    expect_eq(s2.isUniform(), true);
    const n2 = s2.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n2], "1rem");

    // Distinct values preserve both
    const s3 = CssSize2D.parse("calc(1rem + 4px) 2rem") catch unreachable;
    expect_eq(s3.isUniform(), false);
    const n3 = s3.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n3], "calc(1rem + 4px) 2rem");
});

test("css box_rect: malformed inputs and BufferTooSmall protection", fn() {
    // Empty input
    let e_empty = false;
    * = CssBoxRect.parse("   \t\n  ") catch |e| blk: {
        expect_eq(e, CssBoxRectError.EmptyRect);
        e_empty = true;
        break :blk CssBoxRect{};
    };
    expect_eq(e_empty, true);

    // 5 values -> TooManyValues
    let e_many = false;
    * = CssBoxRect.parse("1px 2px 3px 4px 5px") catch |e| blk: {
        expect_eq(e, CssBoxRectError.TooManyValues);
        e_many = true;
        break :blk CssBoxRect{};
    };
    expect_eq(e_many, true);

    // 3 values on CssSize2D -> TooManyValues
    let e_many2 = false;
    * = CssSize2D.parse("1px 2px 3px") catch |e| blk: {
        expect_eq(e, CssBoxRectError.TooManyValues);
        e_many2 = true;
        break :blk CssSize2D{};
    };
    expect_eq(e_many2, true);

    // Unclosed parenthesis
    let e_unclosed = false;
    * = CssBoxRect.parse("calc(10px + 2px 8px") catch |e| blk: {
        expect_eq(e, CssBoxRectError.UnclosedDelimiter);
        e_unclosed = true;
        break :blk CssBoxRect{};
    };
    expect_eq(e_unclosed, true);

    // BufferTooSmall
    const r = CssBoxRect.parse("10px 20px 30px 40px") catch unreachable;
    let tiny: [5]u8 = [0; 5];
    let e_small = false;
    * = r.toCss(tiny[0..]) catch |e| blk: {
        expect_eq(e, CssBoxRectError.BufferTooSmall);
        e_small = true;
        break :blk 0;
    };
    expect_eq(e_small, true);
});
