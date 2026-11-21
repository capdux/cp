// Source: cp/cap/test/test_css_text_list_production_deep.cp (Production deep tests for cap:css textlist)

import { test, expect_eq } from "cap:test";
import {
    TextListTransformCase,
    TextTransformOther,
    TextListWhiteSpace,
    TextListWordBreak,
    TextListOverflowWrap,
    TextListTextAlign,
    TextListDecorationLine,
    Direction,
    TextListStyleType,
    TextListStylePosition,
    TextListFilterKind,
    BlendMode,
    BoxShadowFlags,
} from "cap:css";

test("TextTransformCase, TextTransformOther, TextDecorationLine and Direction") {
    const cases: [4]string = ["none", "uppercase", "lowercase", "capitalize"];
    let i: usize = 0;
    while (i < 4) {
        if (c := TextListTransformCase.fromStr(cases[i])) {
            expect_eq(c.asStr(), cases[i]);
            expect_eq(c.isActive(), i != 0);
        } else {
            expect_eq(true, false);
        }
        i += 1;
    }

    const other_empty = TextTransformOther{};
    expect_eq(other_empty.isEmpty(), true);
    expect_eq(other_empty.toU8(), @as(u8, 0));

    const other_full = TextTransformOther.fromU8(3);
    expect_eq(other_full.isEmpty(), false);
    expect_eq(other_full.full_width, true);
    expect_eq(other_full.full_size_kana, true);
    expect_eq(other_full.toU8(), @as(u8, 3));
    expect_eq(other_full.eql(TextTransformOther{ full_width: true, full_size_kana: true }), true);

    const deco_none = TextListDecorationLine{};
    expect_eq(deco_none.isNone(), true);
    expect_eq(deco_none.hasLine(), false);

    const deco = TextListDecorationLine{ underline: true, line_through: true, spelling_error: true };
    const deco_raw = deco.toU8();
    expect_eq(deco_raw, @as(u8, 1 | 4 | 16));
    expect_eq(TextListDecorationLine.fromU8(deco_raw).eql(deco), true);
    expect_eq(deco.isNone(), false);
    expect_eq(deco.hasLine(), true);

    const deco_only_blink = TextListDecorationLine{ blink: true };
    expect_eq(deco_only_blink.isNone(), false);
    expect_eq(deco_only_blink.hasLine(), false);

    if (d_ltr := Direction.fromStr("ltr")) {
        expect_eq(d_ltr.asStr(), "ltr");
        expect_eq(d_ltr.isRtl(), false);
    } else {
        expect_eq(true, false);
    }
    if (d_rtl := Direction.fromStr("rtl")) {
        expect_eq(d_rtl.asStr(), "rtl");
        expect_eq(d_rtl.isRtl(), true);
    } else {
        expect_eq(true, false);
    }
}

test("WhiteSpace, WordBreak, OverflowWrap and TextAlign semantic matrix") {
    const ws_names: [6]string = ["normal", "pre", "nowrap", "pre-wrap", "break-spaces", "pre-line"];
    const ws_spaces: [6]bool = [false, true, false, true, true, false];
    const ws_newlines: [6]bool = [false, true, false, true, true, true];
    const ws_wraps: [6]bool = [true, false, false, true, true, true];

    let i: usize = 0;
    while (i < 6) {
        if (ws := TextListWhiteSpace.fromStr(ws_names[i])) {
            expect_eq(ws.asStr(), ws_names[i]);
            expect_eq(ws.preservesSpaces(), ws_spaces[i]);
            expect_eq(ws.preservesNewlines(), ws_newlines[i]);
            expect_eq(ws.allowsWrap(), ws_wraps[i]);
        } else {
            expect_eq(true, false);
        }
        i += 1;
    }

    const wb_names: [4]string = ["normal", "keep-all", "break-all", "break-word"];
    const wb_any: [4]bool = [false, false, true, true];
    let j: usize = 0;
    while (j < 4) {
        if (wb := TextListWordBreak.fromStr(wb_names[j])) {
            expect_eq(wb.asStr(), wb_names[j]);
            expect_eq(wb.breaksAnywhere(), wb_any[j]);
        } else {
            expect_eq(true, false);
        }
        j += 1;
    }

    const ow_names: [3]string = ["normal", "break-word", "anywhere"];
    const ow_emg: [3]bool = [false, true, true];
    let k: usize = 0;
    while (k < 3) {
        if (ow := TextListOverflowWrap.fromStr(ow_names[k])) {
            expect_eq(ow.asStr(), ow_names[k]);
            expect_eq(ow.allowsEmergencyBreak(), ow_emg[k]);
        } else {
            expect_eq(true, false);
        }
        k += 1;
    }

    const ta_names: [8]string = ["start", "end", "left", "right", "center", "justify", "match-parent", "justify-all"];
    const ta_logical: [8]bool = [true, true, false, false, false, false, true, false];
    const ta_just: [8]bool = [false, false, false, false, false, true, false, true];
    let m: usize = 0;
    while (m < 8) {
        if (ta := TextListTextAlign.fromStr(ta_names[m])) {
            expect_eq(ta.asStr(), ta_names[m]);
            expect_eq(ta.isLogical(), ta_logical[m]);
            expect_eq(ta.isJustified(), ta_just[m]);
        } else {
            expect_eq(true, false);
        }
        m += 1;
    }
}

test("ListStyleType and ListStylePosition roundtrip and category classification") {
    const list_names: [14]string = [
        "none",
        "disc",
        "circle",
        "square",
        "decimal",
        "decimal-leading-zero",
        "lower-roman",
        "upper-roman",
        "lower-alpha",
        "upper-alpha",
        "lower-latin",
        "upper-latin",
        "lower-greek",
        "cjk-decimal",
    ];

    let sym_count: usize = 0;
    let num_count: usize = 0;
    let alpha_count: usize = 0;
    let roman_count: usize = 0;

    let i: usize = 0;
    while (i < 14) {
        if (lt := TextListStyleType.fromStr(list_names[i])) {
            expect_eq(lt.asStr(), list_names[i]);
            if (lt.isSymbolic()) { sym_count += 1; }
            if (lt.isNumeric()) { num_count += 1; }
            if (lt.isAlphabetic()) { alpha_count += 1; }
            if (lt.isRoman()) { roman_count += 1; }
        } else {
            expect_eq(true, false);
        }
        i += 1;
    }

    expect_eq(sym_count, @as(usize, 3));
    expect_eq(num_count, @as(usize, 3));
    expect_eq(alpha_count, @as(usize, 5));
    expect_eq(roman_count, @as(usize, 2));

    if (p_in := TextListStylePosition.fromStr("inside")) {
        expect_eq(p_in.asStr(), "inside");
        expect_eq(p_in.isInside(), true);
    } else {
        expect_eq(true, false);
    }
    if (p_out := TextListStylePosition.fromStr("outside")) {
        expect_eq(p_out.asStr(), "outside");
        expect_eq(p_out.isInside(), false);
    } else {
        expect_eq(true, false);
    }
}

test("FilterKind, BlendMode and BoxShadowFlags roundtrip and semantics") {
    const filters: [11]string = [
        "blur", "brightness", "contrast", "grayscale", "hue-rotate",
        "invert", "opacity", "saturate", "sepia", "drop-shadow", "url",
    ];
    let color_matrix_count: usize = 0;
    let i: usize = 0;
    while (i < 11) {
        if (fk := TextListFilterKind.fromStr(filters[i])) {
            expect_eq(fk.asStr(), filters[i]);
            if (fk.isColorMatrix()) { color_matrix_count += 1; }
        } else {
            expect_eq(true, false);
        }
        i += 1;
    }
    expect_eq(color_matrix_count, @as(usize, 8));

    const blends: [16]string = [
        "normal", "multiply", "screen", "overlay", "darken", "lighten",
        "color-dodge", "color-burn", "hard-light", "soft-light",
        "difference", "exclusion", "hue", "saturation", "color", "luminosity",
    ];
    let sep_count: usize = 0;
    let j: usize = 0;
    while (j < 16) {
        if (bm := BlendMode.fromStr(blends[j])) {
            expect_eq(bm.asStr(), blends[j]);
            if (bm.isSeparable()) { sep_count += 1; }
        } else {
            expect_eq(true, false);
        }
        j += 1;
    }
    expect_eq(sep_count, @as(usize, 12));

    // BoxShadowFlags 全部 16 种 4-bit 组合往返
    let mask: u8 = 0;
    while (mask < 16) {
        const f = BoxShadowFlags.fromU8(mask);
        expect_eq(f.toU8(), mask);
        expect_eq(f.isSimpleDrop(), !f.inset && !f.has_spread);
        mask += 1;
    }
}

test("Case sensitivity, whitespace and invalid keyword boundary guards") {
    expect_eq(TextListTransformCase.fromStr("") == null, true);
    expect_eq(TextListTransformCase.fromStr("UPPERCASE") == null, true);
    expect_eq(TextListTransformCase.fromStr("uppercase ") == null, true);

    expect_eq(TextListWhiteSpace.fromStr("pre_wrap") == null, true);
    expect_eq(TextListWhiteSpace.fromStr("Pre-Wrap") == null, true);
    expect_eq(TextListWhiteSpace.fromStr("collapse") == null, true);

    expect_eq(TextListWordBreak.fromStr("break_all") == null, true);
    expect_eq(TextListOverflowWrap.fromStr("break_word") == null, true);
    expect_eq(TextListTextAlign.fromStr("middle") == null, true);
    expect_eq(Direction.fromStr("LTR") == null, true);

    expect_eq(TextListStyleType.fromStr("lower_roman") == null, true);
    expect_eq(TextListStyleType.fromStr("bullet") == null, true);
    expect_eq(TextListStylePosition.fromStr("center") == null, true);

    expect_eq(TextListFilterKind.fromStr("hue_rotate") == null, true);
    expect_eq(TextListFilterKind.fromStr("drop_shadow") == null, true);
    expect_eq(BlendMode.fromStr("color_dodge") == null, true);
    expect_eq(BlendMode.fromStr("plus-lighter") == null, true);
}

test("Typography, list and visual effects style composition workflow") {
    const ws = TextListWhiteSpace.fromStr("pre-line").?;
    const ow = TextListOverflowWrap.fromStr("anywhere").?;
    const align = TextListTextAlign.fromStr("start").?;
    const dir = Direction.fromStr("rtl").?;

    // RTL 环境下 logical start 对齐 + 允许软折行与紧急断行
    expect_eq(ws.preservesNewlines() && ws.allowsWrap(), true);
    expect_eq(ow.allowsEmergencyBreak(), true);
    expect_eq(align.isLogical() && dir.isRtl(), true);

    const list_t = TextListStyleType.fromStr("cjk-decimal").?;
    const list_p = TextListStylePosition.fromStr("inside").?;
    expect_eq(list_t.isNumeric() && list_p.isInside(), true);

    const blend = BlendMode.fromStr("luminosity").?;
    const filter = TextListFilterKind.fromStr("drop-shadow").?;
    const shadow_flags = BoxShadowFlags{ has_blur: true, is_text_shadow: false };
    expect_eq(!blend.isSeparable(), true);
    expect_eq(!filter.isColorMatrix(), true);
    expect_eq(shadow_flags.isSimpleDrop(), true);
}
