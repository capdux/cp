// Source: cp/cap/test/test_css_image_set_production_deep.cp (Production deep tests for cap:css_image_set)

import { test, expect_eq } from "cap:test";
import {
    ResolutionUnit,
    Resolution,
    ImageSetOption,
    ImageSet,
    CssImageKind,
    CssImage,
} from "cap:css_image_set";

fn approxEq(a: f32, b: f32, eps: f32): bool {
    const diff = if (a >= b) a - b else b - a;
    return diff <= eps;
}

fn supportsWebpOnly(mime: []const u8): bool {
    return std.mem.eql(u8, mime, "image/webp") || std.mem.eql(u8, mime, "image/png");
}

fn supportsNothing(mime: []const u8): bool {
    * = mime;
    return false;
}

test("css_image_set: Resolution unit parsing, dppx conversion and x/dppx normalization") {
    let buf: [64]u8 = [0; 64];

    const r_dpi = Resolution.parse("96dpi").?;
    expect_eq(r_dpi.unit == .dpi, true);
    expect_eq(approxEq(r_dpi.toDppx(), 1.0, 0.0001), true);
    expect_eq(std.mem.eql(u8, r_dpi.toCss(buf[0..]), "96dpi"), true);

    const r_dpi2 = Resolution.parse("192DPI").?;
    expect_eq(approxEq(r_dpi2.toDppx(), 2.0, 0.0001), true);

    const r_dppx = Resolution.parse("2.5dppx").?;
    expect_eq(r_dppx.unit == .dppx, true);
    expect_eq(approxEq(r_dppx.toDppx(), 2.5, 0.0001), true);
    expect_eq(r_dppx.normalized().unit == .x, true);
    expect_eq(std.mem.eql(u8, r_dppx.toCss(buf[0..]), "2.5x"), true);

    const r_x = Resolution.parse("2x").?;
    expect_eq(r_x.unit == .x, true);
    expect_eq(approxEq(r_x.toDppx(), 2.0, 0.0001), true);
    expect_eq(std.mem.eql(u8, r_x.toCss(buf[0..]), "2x"), true);

    // 1dpcm = 2.54 / 96 dppx => 37.795275 dpcm ≈ 1.0 dppx
    const r_dpcm = Resolution.parse("37.7953dpcm").?;
    expect_eq(r_dpcm.unit == .dpcm, true);
    expect_eq(approxEq(r_dpcm.toDppx(), 1.0, 0.001), true);
    expect_eq(std.mem.eql(u8, r_dpcm.unit.asStr(), "dpcm"), true);

    // 负数、未知单位与空输入应拒绝
    expect_eq(Resolution.parse("-2x") == null, true);
    expect_eq(Resolution.parse("100px") == null, true);
    expect_eq(Resolution.parse("") == null, true);
}

test("css_image_set: image-set() and -webkit-image-set() parsing with type() and order flexibility") {
    let buf: [256]u8 = [0; 256];

    const iset = ImageSet.parse(
        "image-set(\"hero.avif\" type(\"image/avif\") 2dppx, url(hero.webp) 192dpi type('image/webp'), 'hero.png')"
    ).?;
    expect_eq(iset.is_webkit_prefixed, false);
    expect_eq(iset.len, 3);

    expect_eq(std.mem.eql(u8, iset.options[0].url, "hero.avif"), true);
    expect_eq(approxEq(iset.options[0].resolution.toDppx(), 2.0, 0.0001), true);
    expect_eq(std.mem.eql(u8, iset.options[0].mime_type.?, "image/avif"), true);

    expect_eq(std.mem.eql(u8, iset.options[1].url, "hero.webp"), true);
    expect_eq(approxEq(iset.options[1].resolution.toDppx(), 2.0, 0.0001), true);
    expect_eq(std.mem.eql(u8, iset.options[1].mime_type.?, "image/webp"), true);

    // 缺省分辨率默认为 1x，无 mime_type
    expect_eq(std.mem.eql(u8, iset.options[2].url, "hero.png"), true);
    expect_eq(approxEq(iset.options[2].resolution.toDppx(), 1.0, 0.0001), true);
    expect_eq(iset.options[2].mime_type == null, true);

    const out = iset.toCss(buf[0..]);
    expect_eq(
        std.mem.eql(
            u8,
            out,
            "image-set(\"hero.avif\" 2x type(\"image/avif\"), \"hero.webp\" 192dpi type(\"image/webp\"), \"hero.png\" 1x)"
        ),
        true
    );

    // -webkit-image-set() 前缀保留与 url("...") 序列化
    const wset = ImageSet.parse("-webkit-image-set(url(\"a.png\") 1x, url(\"a@2x.png\") 2dppx)").?;
    expect_eq(wset.is_webkit_prefixed, true);
    expect_eq(wset.len, 2);
    const wout = wset.toCss(buf[0..]);
    expect_eq(
        std.mem.eql(u8, wout, "-webkit-image-set(url(\"a.png\") 1x, url(\"a@2x.png\") 2x)"),
        true
    );
}

test("css_image_set: selectBest DPR matching, MIME filtering and high-DPR fallback") {
    const iset = ImageSet.parse(
        "image-set(\"img-1x.png\" 1x type(\"image/png\"), \"img-2x.avif\" 2x type(\"image/avif\"), \"img-2x.webp\" 2x type(\"image/webp\"), \"img-3x.avif\" 3x type(\"image/avif\"))"
    ).?;

    // 1. DPR 1.0 -> 选 1x png
    const pick1 = iset.selectBest(1.0, supportsWebpOnly).?;
    expect_eq(std.mem.eql(u8, pick1.url, "img-1x.png"), true);

    // 2. DPR 1.5 -> 过滤掉 avif，满足 >= 1.5 的最小分辨率为 2x webp
    const pick15 = iset.selectBest(1.5, supportsWebpOnly).?;
    expect_eq(std.mem.eql(u8, pick15.url, "img-2x.webp"), true);

    // 3. DPR 3.0 且不支持 avif -> 无 >= 3.0 候选，回退至受支持的最大分辨率 2x webp
    const pick3_no_avif = iset.selectBest(3.0, supportsWebpOnly).?;
    expect_eq(std.mem.eql(u8, pick3_no_avif.url, "img-2x.webp"), true);

    // 4. DPR 3.0 且不限制 MIME（传 null）-> 直接命中 3x avif
    const pick3_all = iset.selectBest(3.0, null).?;
    expect_eq(std.mem.eql(u8, pick3_all.url, "img-3x.avif"), true);

    // 5. 所有 MIME 均不支持时返回 null
    expect_eq(iset.selectBest(2.0, supportsNothing) == null, true);
}

test("css_image_set: CssImage unified parsing and serialization for none/url/image-set/gradient") {
    let buf: [256]u8 = [0; 256];

    const img_none = CssImage.parse("  NONE ").?;
    expect_eq(img_none.kind == .none, true);
    expect_eq(std.mem.eql(u8, img_none.toCss(buf[0..]), "none"), true);

    const img_url = CssImage.parse("url( /assets/bg.png )").?;
    expect_eq(img_url.kind == .url, true);
    expect_eq(std.mem.eql(u8, img_url.url, "/assets/bg.png"), true);
    expect_eq(std.mem.eql(u8, img_url.toCss(buf[0..]), "url(\"/assets/bg.png\")"), true);

    const img_set = CssImage.parse("image-set(\"a.webp\" 1x, \"b.webp\" 2dppx)").?;
    expect_eq(img_set.kind == .image_set, true);
    expect_eq(img_set.image_set.len, 2);
    expect_eq(std.mem.eql(u8, img_set.toCss(buf[0..]), "image-set(\"a.webp\" 1x, \"b.webp\" 2x)"), true);

    const img_grad = CssImage.parse("linear-gradient(45deg, rgba(0, 0, 0, 0.5), #fff)").?;
    expect_eq(img_grad.kind == .gradient_raw, true);
    expect_eq(std.mem.eql(u8, img_grad.kind.asStr(), "gradient"), true);
    expect_eq(std.mem.eql(u8, img_grad.toCss(buf[0..]), "linear-gradient(45deg, rgba(0, 0, 0, 0.5), #fff)"), true);
}

test("css_image_set: malformed inputs, zero/duplicate resolution and syntax error guards") {
    // 空 image-set、尾随逗号、未闭合括号、0x 分辨率、重复分辨率、重复 type()
    expect_eq(ImageSet.parse("image-set()") == null, true);
    expect_eq(ImageSet.parse("image-set(\"a.png\" 1x,)") == null, true);
    expect_eq(ImageSet.parse("image-set(\"a.png\" 1x") == null, true);
    expect_eq(ImageSet.parse("image-set(\"a.png\" 0x)") == null, true);
    expect_eq(ImageSet.parse("image-set(\"a.png\" 1x 2x)") == null, true);
    expect_eq(ImageSet.parse("image-set(\"a.png\" type(\"image/png\") type(\"image/webp\"))") == null, true);
    expect_eq(ImageSet.parse("image-set(\"a.png\" type(\"\"))") == null, true);
    expect_eq(ImageSetOption.parse("not-a-url 2x") == null, true);

    // CssImage 非法值与括号不平衡的渐变
    expect_eq(CssImage.parse("") == null, true);
    expect_eq(CssImage.parse("url()") == null, true);
    expect_eq(CssImage.parse("url(\"a.png\") trailing") == null, true);
    expect_eq(CssImage.parse("linear-gradient(red, blue") == null, true);
    expect_eq(CssImage.parse("linear-gradient(red, blue))") == null, true);
    expect_eq(CssImage.parse("unknown-func(1px)") == null, true);
}
