// Source: cp/cap/test/test_css_svg_production_deep.cp (Production deep tests for cap:css_svg)

import { test, expect_eq } from "cap:test";
import {
    SVGPaint,
    SVGPaintKind,
    SVGPaintFallbackKind,
    StrokeLinecap,
    StrokeLinejoin,
    StrokeDasharray,
    PaintOrder,
    PaintOrderItem,
    ColorInterpolation,
    ColorRendering,
    ShapeRendering,
    TextRendering,
    ImageRendering,
} from "cap:css_svg";

test("SVGPaint all 5 kinds including url(#id) fallback and context-fill/stroke") {
    let buf: [64]u8 = [0; 64];

    const p_none = SVGPaint.parse("none").?;
    expect_eq(p_none.kind == .none, true);
    expect_eq(std.mem.eql(u8, p_none.toCss(buf[0..64]), "none"), true);

    const p_cf = SVGPaint.parse("context-fill").?;
    expect_eq(p_cf.kind == .context_fill, true);
    expect_eq(std.mem.eql(u8, p_cf.toCss(buf[0..64]), "context-fill"), true);

    const p_cs = SVGPaint.parse("context-stroke").?;
    expect_eq(p_cs.kind == .context_stroke, true);

    const p_col = SVGPaint.parse("rgb(255, 0, 128)").?;
    expect_eq(p_col.kind == .color, true);
    expect_eq(std.mem.eql(u8, p_col.toCss(buf[0..64]), "rgb(255, 0, 128)"), true);

    const p_url_fb = SVGPaint.parse("url(\"#grad1\") red").?;
    expect_eq(p_url_fb.kind == .url, true);
    expect_eq(p_url_fb.url_ref, "#grad1");
    expect_eq(p_url_fb.fallback_kind == .color, true);
    expect_eq(p_url_fb.fallback_color, "red");
    expect_eq(std.mem.eql(u8, p_url_fb.toCss(buf[0..64]), "url(#grad1) red"), true);

    const p_url_none = SVGPaint.parse("url(#pat) none").?;
    expect_eq(p_url_none.fallback_kind == .none, true);
    expect_eq(std.mem.eql(u8, p_url_none.toCss(buf[0..64]), "url(#pat) none"), true);
}

test("StrokeDasharray none and mixed comma/space numeric/px/percent sequences") {
    let buf: [64]u8 = [0; 64];

    const d_none = StrokeDasharray.parse("none").?;
    expect_eq(d_none.is_none, true);
    expect_eq(d_none.slice().len, @as(usize, 0));
    expect_eq(std.mem.eql(u8, d_none.toCss(buf[0..64]), "none"), true);

    const d_mix = StrokeDasharray.parse("5px, 10%, 3 2.5px").?;
    expect_eq(d_mix.is_none, false);
    expect_eq(d_mix.slice().len, @as(usize, 4));
    expect_eq(std.mem.eql(u8, d_mix.toCss(buf[0..64]), "5px 10% 3 2.5px"), true);
}

test("PaintOrder all 6 permutations, auto-completion and canonical shortest toCss") {
    let buf: [64]u8 = [0; 64];

    const po_norm = PaintOrder.parse("fill stroke markers").?;
    expect_eq(po_norm.isNormal(), true);
    expect_eq(po_norm.eql(PaintOrder.parse("normal").?), true);
    expect_eq(std.mem.eql(u8, po_norm.toCss(buf[0..64]), "normal"), true);

    // stroke -> [stroke, fill, markers] -> 压缩为 "stroke"
    const po_s1 = PaintOrder.parse("stroke").?;
    const po_s2 = PaintOrder.parse("stroke fill markers").?;
    expect_eq(po_s1.eql(po_s2), true);
    expect_eq(po_s1.order[0] == .stroke && po_s1.order[1] == .fill && po_s1.order[2] == .markers, true);
    expect_eq(std.mem.eql(u8, po_s2.toCss(buf[0..64]), "stroke"), true);

    // markers -> [markers, fill, stroke] -> 压缩为 "markers"
    const po_m = PaintOrder.parse("markers fill stroke").?;
    expect_eq(std.mem.eql(u8, po_m.toCss(buf[0..64]), "markers"), true);

    // 非默认尾序压缩为前 2 项
    const po_sm = PaintOrder.parse("stroke markers fill").?;
    expect_eq(std.mem.eql(u8, po_sm.toCss(buf[0..64]), "stroke markers"), true);

    const po_fm = PaintOrder.parse("fill markers stroke").?;
    expect_eq(std.mem.eql(u8, po_fm.toCss(buf[0..64]), "fill markers"), true);

    const po_ms = PaintOrder.parse("markers stroke fill").?;
    expect_eq(std.mem.eql(u8, po_ms.toCss(buf[0..64]), "markers stroke"), true);
}

test("StrokeLinecap and StrokeLinejoin roundtrip") {
    expect_eq(StrokeLinecap.fromStr("butt").?.asStr(), "butt");
    expect_eq(StrokeLinecap.fromStr("ROUND").?.asStr(), "round");
    expect_eq(StrokeLinecap.fromStr("square").?.asStr(), "square");
    expect_eq(StrokeLinecap.fromStr("flat") == null, true);

    expect_eq(StrokeLinejoin.fromStr("miter").?.asStr(), "miter");
    expect_eq(StrokeLinejoin.fromStr("round").?.asStr(), "round");
    expect_eq(StrokeLinejoin.fromStr("bevel").?.asStr(), "bevel");
    expect_eq(StrokeLinejoin.fromStr("miter-clip").?.asStr(), "miter-clip");
    expect_eq(StrokeLinejoin.fromStr("arcs").?.asStr(), "arcs");
    expect_eq(StrokeLinejoin.fromStr("sharp") == null, true);
}

test("All 5 SVG rendering hint enums parsing and canonical casing") {
    expect_eq(ColorInterpolation.fromStr("srgb").?.asStr(), "sRGB");
    expect_eq(ColorInterpolation.fromStr("linearrgb").?.asStr(), "linearRGB");
    expect_eq(ColorInterpolation.fromStr("auto").?.asStr(), "auto");

    expect_eq(ColorRendering.fromStr("optimizespeed").?.asStr(), "optimizeSpeed");
    expect_eq(ColorRendering.fromStr("optimizequality").?.asStr(), "optimizeQuality");

    expect_eq(ShapeRendering.fromStr("crispedges").?.asStr(), "crispEdges");
    expect_eq(ShapeRendering.fromStr("geometricprecision").?.asStr(), "geometricPrecision");

    expect_eq(TextRendering.fromStr("optimizelegibility").?.asStr(), "optimizeLegibility");
    expect_eq(TextRendering.fromStr("geometricprecision").?.asStr(), "geometricPrecision");

    expect_eq(ImageRendering.fromStr("smooth").?.asStr(), "smooth");
    expect_eq(ImageRendering.fromStr("high-quality").?.asStr(), "high-quality");
    expect_eq(ImageRendering.fromStr("crisp-edges").?.asStr(), "crisp-edges");
    expect_eq(ImageRendering.fromStr("pixelated").?.asStr(), "pixelated");
}

test("Malformed SVG property inputs and buffer bounds guards") {
    expect_eq(SVGPaint.parse("") == null, true);
    expect_eq(SVGPaint.parse("url()") == null, true);
    expect_eq(SVGPaint.parse("url(#a) red blue") == null, true);
    expect_eq(SVGPaint.parse("red blue") == null, true);

    expect_eq(StrokeDasharray.parse("") == null, true);
    expect_eq(StrokeDasharray.parse("-4px") == null, true);
    expect_eq(StrokeDasharray.parse("4px,,2px") == null, true);
    expect_eq(StrokeDasharray.parse(",4px") == null, true);
    expect_eq(StrokeDasharray.parse("4px,") == null, true);

    expect_eq(PaintOrder.parse("") == null, true);
    expect_eq(PaintOrder.parse("fill fill") == null, true);
    expect_eq(PaintOrder.parse("fill stroke markers fill") == null, true);
    expect_eq(PaintOrder.parse("normal fill") == null, true);
    expect_eq(PaintOrder.parse("unknown") == null, true);

    // 缓冲过小安全返回空切片
    const p = SVGPaint.parse("url(#grad) red").?;
    let tiny: [3]u8 = [0; 3];
    expect_eq(p.toCss(tiny[0..3]).len, @as(usize, 0));
}
