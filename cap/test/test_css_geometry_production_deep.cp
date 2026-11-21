// Source: cp/cap/test/test_css_geometry_production_deep.cp (Production deep tests for cap:css_geometry)

import { test, expect_eq } from "cap:test";
import {
    Ratio,
    Rect,
    OverflowKeyword,
    Overflow,
    TextOverflow,
} from "cap:css_geometry";

test("Ratio simplify, asF64, invert, eql and equivalent") {
    const hd = Ratio.new(1920.0, 1080.0);
    expect_eq(hd.isDegenerate(), false);

    const simplified = hd.simplify();
    expect_eq(simplified.eql(Ratio.new(16.0, 9.0)), true);
    expect_eq(hd.equivalent(simplified), true);
    expect_eq(hd.eql(simplified), false);

    const square = Ratio.new(256.0, 256.0).simplify();
    expect_eq(square.eql(Ratio.fromSingle(1.0)), true);

    // 小数比例精确化简：1.5 / 0.5 -> 3 / 1，2.5 / 1.0 -> 5 / 2
    const dec1 = Ratio.new(1.5, 0.5).simplify();
    expect_eq(dec1.eql(Ratio.new(3.0, 1.0)), true);
    const dec2 = Ratio.new(2.5, 1.0).simplify();
    expect_eq(dec2.eql(Ratio.new(5.0, 2.0)), true);

    // asF64 与 invert 验证
    if (val := Ratio.new(3.0, 2.0).asF64()) {
        expect_eq(val, @as(f64, 1.5));
    } else {
        expect_eq(true, false);
    }

    const inv = Ratio.new(16.0, 9.0).invert();
    expect_eq(inv.eql(Ratio.new(9.0, 16.0)), true);
}

test("Ratio zero-denominator and degenerate ratio boundary guards") {
    // 零分母防护
    const zero_den = Ratio.new(16.0, 0.0);
    expect_eq(zero_den.isDegenerate(), true);
    expect_eq(zero_den.asF64() == null, true);
    expect_eq(zero_den.simplify().eql(zero_den), true);

    // 零分子与 0/0 防护
    const zero_num = Ratio.new(0.0, 9.0);
    expect_eq(zero_num.isDegenerate(), true);
    if (z := zero_num.asF64()) {
        expect_eq(z, @as(f64, 0.0));
    } else {
        expect_eq(true, false);
    }
    expect_eq(zero_num.simplify().eql(zero_num), true);

    const zero_both = Ratio.new(0.0, 0.0);
    expect_eq(zero_both.isDegenerate(), true);
    expect_eq(zero_both.asF64() == null, true);

    // 负数、NaN、Inf 防护
    const neg_num = Ratio.new(-4.0, 3.0);
    const neg_den = Ratio.new(4.0, -3.0);
    expect_eq(neg_num.isDegenerate(), true);
    expect_eq(neg_den.isDegenerate(), true);
    expect_eq(neg_num.equivalent(neg_den), false);

    const nan_r = Ratio.new(std.math.nan(f64), 1.0);
    const inf_r = Ratio.new(1.0, std.math.inf(f64));
    expect_eq(nan_r.isDegenerate(), true);
    expect_eq(nan_r.asF64() == null, true);
    expect_eq(inf_r.isDegenerate(), true);
    expect_eq(inf_r.asF64() == null, true);
}

test("Rect(T) generic constructors, uniformity, sums and equality") {
    const RectF = Rect(f64);
    const uniform = RectF.all(16.0);
    expect_eq(uniform.isUniform(), true);
    expect_eq(uniform.horizontalSum(), @as(f64, 32.0));
    expect_eq(uniform.verticalSum(), @as(f64, 32.0));
    expect_eq(uniform.eql(RectF.symmetric(16.0, 16.0)), true);

    const sym = RectF.symmetric(10.0, 24.0);
    expect_eq(sym.top, @as(f64, 10.0));
    expect_eq(sym.bottom, @as(f64, 10.0));
    expect_eq(sym.left, @as(f64, 24.0));
    expect_eq(sym.right, @as(f64, 24.0));
    expect_eq(sym.isUniform(), false);
    expect_eq(sym.horizontalSum(), @as(f64, 48.0));
    expect_eq(sym.verticalSum(), @as(f64, 20.0));

    const three = RectF.fromThree(4.0, 8.0, 12.0);
    expect_eq(three.top, @as(f64, 4.0));
    expect_eq(three.right, @as(f64, 8.0));
    expect_eq(three.bottom, @as(f64, 12.0));
    expect_eq(three.left, @as(f64, 8.0));
    expect_eq(three.horizontalSum(), @as(f64, 16.0));
    expect_eq(three.verticalSum(), @as(f64, 16.0));
    expect_eq(three.isUniform(), false);

    // 整型 Rect(i32) 实例化（含负 margin）
    const RectI = Rect(i32);
    const margin = RectI{ top: -4, right: 12, bottom: 8, left: -2 };
    expect_eq(margin.horizontalSum(), @as(i32, 10));
    expect_eq(margin.verticalSum(), @as(i32, 4));
    expect_eq(margin.isUniform(), false);
    expect_eq(margin.eql(RectI{ top: -4, right: 12, bottom: 8, left: -2 }), true);
    expect_eq(margin.eql(RectI.all(0)), false);
}

test("OverflowKeyword parsing, serialization, clipping and unknown input guards") {
    const names: [5]string = ["visible", "hidden", "clip", "scroll", "auto"];
    const clips: [5]bool = [false, true, true, true, true];
    const scrolls: [5]bool = [false, true, false, true, true];

    let i: usize = 0;
    while (i < 5) {
        if (kw := OverflowKeyword.fromStr(names[i])) {
            expect_eq(kw.asStr(), names[i]);
            expect_eq(kw.clipsContent(), clips[i]);
            expect_eq(kw.allowsScroll(), scrolls[i]);
        } else {
            expect_eq(true, false);
        }
        i += 1;
    }

    // 未知、空串、前后空白、大小写错误输入必须被拦截返回 null
    const bad_inputs: [7]string = ["", " ", "overlay", "none", "VISIBLE", "hidden ", " scroll"];
    let j: usize = 0;
    while (j < 7) {
        expect_eq(OverflowKeyword.fromStr(bad_inputs[j]) == null, true);
        j += 1;
    }
}

test("Overflow dual-axis shorthand parsing and TextOverflow") {
    const ov_default = Overflow{};
    expect_eq(ov_default.eql(Overflow.all(.visible)), true);
    expect_eq(ov_default.isUniform(), true);
    expect_eq(ov_default.clipsAny(), false);

    // 单值简写
    if (ov1 := Overflow.parseShorthand("clip")) {
        expect_eq(ov1.x == .clip, true);
        expect_eq(ov1.y == .clip, true);
        expect_eq(ov1.isUniform(), true);
        expect_eq(ov1.clipsAny(), true);
    } else {
        expect_eq(true, false);
    }

    // 双值简写（含多空格/制表符）
    if (ov2 := Overflow.parseShorthand("hidden   scroll")) {
        expect_eq(ov2.x == .hidden, true);
        expect_eq(ov2.y == .scroll, true);
        expect_eq(ov2.isUniform(), false);
        expect_eq(ov2.clipsAny(), true);
    } else {
        expect_eq(true, false);
    }

    if (ov3 := Overflow.parseShorthand("visible\tauto")) {
        expect_eq(ov3.eql(Overflow.new(.visible, .auto)), true);
        expect_eq(ov3.clipsAny(), true);
    } else {
        expect_eq(true, false);
    }

    // 畸形与未知简写拦截
    expect_eq(Overflow.parseShorthand("") == null, true);
    expect_eq(Overflow.parseShorthand("hidden ") == null, true);
    expect_eq(Overflow.parseShorthand("hidden auto clip") == null, true);
    expect_eq(Overflow.parseShorthand("hidden unknown") == null, true);

    // TextOverflow 验证
    if (t1 := TextOverflow.fromStr("clip")) {
        expect_eq(t1.asStr(), "clip");
    } else {
        expect_eq(true, false);
    }
    if (t2 := TextOverflow.fromStr("ellipsis")) {
        expect_eq(t2.asStr(), "ellipsis");
    } else {
        expect_eq(true, false);
    }
    expect_eq(TextOverflow.fromStr("fade") == null, true);
}

test("CSS box layout integration with Ratio, Rect and Overflow") {
    const viewport_w: f64 = 1280.0;
    const aspect = Ratio.new(16.0, 9.0);
    let viewport_h: f64 = 0.0;
    if (r := aspect.asF64()) {
        viewport_h = viewport_w / r;
    }
    expect_eq(viewport_h, @as(f64, 720.0));

    const padding = Rect(f64).symmetric(20.0, 40.0);
    const border = Rect(f64).all(2.0);
    const inner_w = viewport_w - padding.horizontalSum() - border.horizontalSum();
    const inner_h = viewport_h - padding.verticalSum() - border.verticalSum();
    expect_eq(inner_w, @as(f64, 1196.0));
    expect_eq(inner_h, @as(f64, 676.0));

    const container_ov = Overflow.new(.hidden, .auto);
    expect_eq(container_ov.clipsAny(), true);
    expect_eq(container_ov.x.clipsContent(), true);
    expect_eq(container_ov.y.allowsScroll(), true);
}
