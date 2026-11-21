// Source: cp/cap/test/test_css_border_image_production_deep.cp (Production deep tests for cap:css_border_image)

import { test, expect_eq } from "cap:test";
import {
    BorderImage,
    BorderImageSlice,
    SliceOffset,
    BorderImageWidth,
    BorderImageSideWidth,
    BorderImageOutset,
    BorderImageRepeat,
    BorderImageRepeatKeyword,
} from "cap:css";

test("BorderImageSlice fill before/after and 1..4 value compression") {
    let buf: [64]u8 = [0; 64];

    const def = BorderImageSlice.defaultSlice();
    expect_eq(def.isDefault(), true);
    expect_eq(std.mem.eql(u8, def.toCss(buf[0..64]), "100%"), true);

    // fill 前置与后置等价
    const s_before = BorderImageSlice.parse("fill 30").?;
    const s_after = BorderImageSlice.parse("30 fill").?;
    expect_eq(s_before.eql(s_after), true);
    expect_eq(s_before.fill, true);
    expect_eq(std.mem.eql(u8, s_before.toCss(buf[0..64]), "30 fill"), true);

    // 4 值压缩：10% 20% 10% 20% -> 10% 20%
    const s_sym = BorderImageSlice.parse("10% 20% 10% 20%").?;
    expect_eq(std.mem.eql(u8, s_sym.toCss(buf[0..64]), "10% 20%"), true);

    // 3 值保留：10 20 30 -> 10 20 30
    const s3 = BorderImageSlice.parse("10 20 30").?;
    expect_eq(s3.left.eql(SliceOffset.number(20.0)), true);
    expect_eq(std.mem.eql(u8, s3.toCss(buf[0..64]), "10 20 30"), true);
}

test("BorderImageWidth and BorderImageOutset 1..4 side parsing and compression") {
    let buf: [64]u8 = [0; 64];

    const w1 = BorderImageWidth.parse("10px 20px 10px 20px").?;
    expect_eq(w1.isDefault(), false);
    expect_eq(std.mem.eql(u8, w1.toCss(buf[0..64]), "10px 20px"), true);

    const w_auto = BorderImageWidth.parse("auto 2 50% 4px").?;
    expect_eq(w_auto.top.kind == .auto_kw, true);
    expect_eq(std.mem.eql(u8, w_auto.toCss(buf[0..64]), "auto 2 50% 4px"), true);

    const o1 = BorderImageOutset.parse("5px 10px 5px 10px").?;
    expect_eq(o1.isDefault(), false);
    expect_eq(std.mem.eql(u8, o1.toCss(buf[0..64]), "5px 10px"), true);

    const o_zero = BorderImageOutset.parse("0px").?;
    expect_eq(o_zero.isDefault(), true);
}

test("BorderImageRepeat keywords and single/dual axis normalization") {
    let buf: [64]u8 = [0; 64];

    const r_def = BorderImageRepeat.parse("stretch stretch").?;
    expect_eq(r_def.isDefault(), true);
    expect_eq(std.mem.eql(u8, r_def.toCss(buf[0..64]), "stretch"), true);

    const r_same = BorderImageRepeat.parse("round round").?;
    expect_eq(std.mem.eql(u8, r_same.toCss(buf[0..64]), "round"), true);

    const r_diff = BorderImageRepeat.parse("round space").?;
    expect_eq(r_diff.horizontal == .round, true);
    expect_eq(r_diff.vertical == .space, true);
    expect_eq(std.mem.eql(u8, r_diff.toCss(buf[0..64]), "round space"), true);

    expect_eq(BorderImageRepeatKeyword.fromStr("invalid") == null, true);
}

test("BorderImage full shorthand with double slash, gradient source and arbitrary order") {
    let buf: [160]u8 = [0; 160];

    const bi = BorderImage.parse("url(border.png) 30 fill / 10px 20px / 5px round space").?;
    expect_eq(bi.source, "url(border.png)");
    expect_eq(bi.slice.fill, true);
    expect_eq(bi.repeat.horizontal == .round, true);
    expect_eq(bi.repeat.vertical == .space, true);
    expect_eq(
        std.mem.eql(u8, bi.toCss(buf[0..160]), "url(border.png) 30 fill / 10px 20px / 5px round space"),
        true,
    );

    // 任意顶层顺序（repeat 在前、gradient source 在中、slice 在后）
    const reordered = BorderImage.parse("round linear-gradient(45deg, red, blue) fill 20%").?;
    expect_eq(reordered.source, "linear-gradient(45deg, red, blue)");
    expect_eq(reordered.slice.fill, true);
    expect_eq(reordered.repeat.horizontal == .round, true);
    expect_eq(
        std.mem.eql(u8, reordered.toCss(buf[0..160]), "linear-gradient(45deg, red, blue) 20% fill round"),
        true,
    );
}

test("BorderImage double slash omitted width and default value omission") {
    let buf: [128]u8 = [0; 128];

    // 省略 width 仅指定 outset: 30 / / 5px
    const bi_outset_only = BorderImage.parse("url(a.png) 30 / / 5px").?;
    expect_eq(bi_outset_only.width.isDefault(), true);
    expect_eq(bi_outset_only.outset.isDefault(), false);
    expect_eq(
        std.mem.eql(u8, bi_outset_only.toCss(buf[0..128]), "url(a.png) 30 / / 5px"),
        true,
    );

    // 全默认分量压缩为 "none"
    const bi_all_def = BorderImage.parse("none 100% / 1 / 0 stretch").?;
    expect_eq(std.mem.eql(u8, bi_all_def.toCss(buf[0..128]), "none"), true);

    // 仅 source 非默认时省略默认 slice/width/outset/repeat
    const bi_src_only = BorderImage.parse("url(frame.svg) 100% stretch").?;
    expect_eq(std.mem.eql(u8, bi_src_only.toCss(buf[0..128]), "url(frame.svg)"), true);
}

test("Malformed border-image inputs and small buffer guards") {
    expect_eq(BorderImage.parse("") == null, true);
    expect_eq(BorderImageSlice.parse("-10%") == null, true);
    expect_eq(BorderImageSlice.parse("fill 10 fill") == null, true);
    expect_eq(BorderImageWidth.parse("-5px") == null, true);
    expect_eq(BorderImageOutset.parse("-2") == null, true);
    expect_eq(BorderImage.parse("url(a.png) 30 /") == null, true);
    expect_eq(BorderImage.parse("url(a.png) 30 / 10px /") == null, true);
    expect_eq(BorderImage.parse("url(a.png) url(b.png)") == null, true);
    expect_eq(BorderImage.parse("url(unclosed") == null, true);

    const bi = BorderImage.parse("url(border.png) 30 round").?;
    let tiny: [5]u8 = [0; 5];
    expect_eq(bi.toCss(tiny[0..5]).len, @as(usize, 0));
}
