import { test, expect_eq } from "cap:test";
import {
    GeometryBox,
    MaskOrigin,
    MaskClip,
    MaskMode,
    MaskType,
    MaskComposite,
    WebKitMaskComposite,
    WebKitMaskSourceType,
    MaskBorderMode,
    MaskBorderRepeat,
    FillRule,
    MASK_FLAG_IMAGE,
    MASK_FLAG_CLIP,
    MASK_FLAG_WEBKIT_PREFIX,
    MaskFeatureFlags,
    MaskConfig,
    MaskBorderConfig,
    getWebKitMaskPropertyName,
} from "cap:css_masking";

test("1. GeometryBox, MaskOrigin & MaskClip parsing, CSS serialization and SVG/CSS box classification", () => {
    const vb = GeometryBox.parse("VIEW-BOX").?;
    expect_eq(vb == GeometryBox.view_box, true);
    expect_eq(std.mem.eql(u8, vb.toCss(), "view-box"), true);
    expect_eq(vb.isSvgBox(), true);
    expect_eq(vb.isCssBox(), false);

    const mb = GeometryBox.parse("margin-box").?;
    expect_eq(mb.isCssBox(), true);
    expect_eq(mb.isSvgBox(), false);
    expect_eq(mb.intoMaskClip() == MaskClip.margin_box, true);

    // MaskOrigin excludes margin-box per CSS Masking 1
    expect_eq(MaskOrigin.parse("margin-box") == null, true);
    const pb = MaskOrigin.parse("Padding-Box").?;
    expect_eq(pb == MaskOrigin.padding_box, true);
    expect_eq(pb.toGeometryBox() == GeometryBox.padding_box, true);

    // MaskClip includes no-clip
    const nc = MaskClip.parse("no-clip").?;
    expect_eq(nc == MaskClip.no_clip, true);
    expect_eq(nc.isClipped(), false);
    expect_eq(std.mem.eql(u8, nc.toCss(), "no-clip"), true);
    expect_eq(MaskClip.default().isClipped(), true);
});

test("2. MaskMode, MaskType, MaskBorderMode, MaskBorderRepeat & FillRule round-trip", () => {
    expect_eq(MaskMode.default() == MaskMode.match_source, true);
    expect_eq(MaskMode.parse("Match-Source").? == MaskMode.match_source, true);
    expect_eq(std.mem.eql(u8, MaskMode.luminance.toCss(), "luminance"), true);

    expect_eq(MaskType.default() == MaskType.luminance, true);
    expect_eq(MaskType.parse("ALPHA").? == MaskType.alpha, true);
    expect_eq(std.mem.eql(u8, MaskType.alpha.toCss(), "alpha"), true);

    expect_eq(MaskBorderMode.default() == MaskBorderMode.alpha, true);
    expect_eq(MaskBorderMode.parse("luminance").? == MaskBorderMode.luminance, true);

    expect_eq(MaskBorderRepeat.default() == MaskBorderRepeat.stretch, true);
    expect_eq(MaskBorderRepeat.parse("ROUND").? == MaskBorderRepeat.round, true);
    expect_eq(std.mem.eql(u8, MaskBorderRepeat.space.toCss(), "space"), true);

    expect_eq(FillRule.default() == FillRule.nonzero, true);
    expect_eq(FillRule.parse("evenodd").? == FillRule.evenodd, true);
    expect_eq(std.mem.eql(u8, FillRule.evenodd.toCss(), "evenodd"), true);
});

test("3. MaskComposite <-> WebKitMaskComposite & MaskMode <-> WebKitMaskSourceType compatibility mapping", () => {
    // Standard -> WebKit -> Standard round-trip
    expect_eq(MaskComposite.add.toWebKit() == WebKitMaskComposite.source_over, true);
    expect_eq(MaskComposite.subtract.toWebKit() == WebKitMaskComposite.source_out, true);
    expect_eq(MaskComposite.intersect.toWebKit() == WebKitMaskComposite.source_in, true);
    expect_eq(MaskComposite.exclude.toWebKit() == WebKitMaskComposite.xor, true);

    expect_eq(WebKitMaskComposite.source_over.toStandard().? == MaskComposite.add, true);
    expect_eq(WebKitMaskComposite.source_in.toStandard().? == MaskComposite.intersect, true);
    expect_eq(WebKitMaskComposite.source_out.toStandard().? == MaskComposite.subtract, true);
    expect_eq(WebKitMaskComposite.xor.toStandard().? == MaskComposite.exclude, true);

    // WebKit-only operators have no standard MaskComposite equivalent
    expect_eq(WebKitMaskComposite.clear.toStandard() == null, true);
    expect_eq(WebKitMaskComposite.destination_atop.toStandard() == null, true);
    expect_eq(std.mem.eql(u8, WebKitMaskComposite.destination_atop.toCss(), "destination-atop"), true);

    // MaskMode <-> WebKitMaskSourceType
    expect_eq(MaskMode.match_source.toWebKitSourceType() == WebKitMaskSourceType.auto, true);
    expect_eq(WebKitMaskSourceType.auto.toStandard() == MaskMode.match_source, true);
    expect_eq(WebKitMaskSourceType.parse("AUTO").? == WebKitMaskSourceType.auto, true);
});

test("4. MaskFeatureFlags bit operations, MaskConfig defaults & WebKit property name mapping", () => {
    let flags = MaskFeatureFlags.empty();
    expect_eq(flags.isEmpty(), true);
    expect_eq(flags.needsWebKitFallback(), false);

    flags.add(MASK_FLAG_IMAGE);
    flags.add(MASK_FLAG_CLIP);
    expect_eq(flags.has(MASK_FLAG_IMAGE), true);
    expect_eq(flags.has(MASK_FLAG_CLIP), true);
    expect_eq(flags.has(MASK_FLAG_WEBKIT_PREFIX), false);
    expect_eq(flags.needsWebKitFallback(), true);

    flags.remove(MASK_FLAG_IMAGE);
    expect_eq(flags.has(MASK_FLAG_IMAGE), false);
    flags.remove(MASK_FLAG_CLIP);
    expect_eq(flags.isEmpty(), true);

    const cfg = MaskConfig.default();
    expect_eq(cfg.isDefault(), true);
    const custom_cfg = MaskConfig{ composite: MaskComposite.exclude };
    expect_eq(custom_cfg.isDefault(), false);

    const bcfg = MaskBorderConfig.default();
    expect_eq(bcfg.isDefault(), true);

    // WebKit property name translation
    expect_eq(std.mem.eql(u8, getWebKitMaskPropertyName("mask-border-slice") ?? "", "-webkit-mask-box-image-slice"), true);
    expect_eq(std.mem.eql(u8, getWebKitMaskPropertyName("MASK-MODE") ?? "", "-webkit-mask-source-type"), true);
    expect_eq(std.mem.eql(u8, getWebKitMaskPropertyName("clip-path") ?? "", "-webkit-clip-path"), true);
    expect_eq(getWebKitMaskPropertyName("background-color") == null, true);
});

test("5. Malformed, empty & whitespace-padded input boundary rejection", () => {
    const invalid_inputs: [5]string = ["", " ", "border_box", "border-box ", "unknown"];
    for (s of invalid_inputs) {
        expect_eq(GeometryBox.parse(s) == null, true);
        expect_eq(MaskOrigin.parse(s) == null, true);
        expect_eq(MaskClip.parse(s) == null, true);
        expect_eq(MaskMode.parse(s) == null, true);
        expect_eq(MaskType.parse(s) == null, true);
        expect_eq(MaskComposite.parse(s) == null, true);
        expect_eq(WebKitMaskComposite.parse(s) == null, true);
        expect_eq(WebKitMaskSourceType.parse(s) == null, true);
        expect_eq(MaskBorderMode.parse(s) == null, true);
        expect_eq(MaskBorderRepeat.parse(s) == null, true);
        expect_eq(FillRule.parse(s) == null, true);
        expect_eq(getWebKitMaskPropertyName(s) == null, true);
    }
});
