// Production regression tests for cap:css CSSOM value validators,
// CSS Color Level 4 parser enhancements, CssProperty.parse comment/!important/custom-prop
// handling, and @supports custom-property & escape preservation.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CssColor,
    CssProperty,
    SupportsTree,
    isValidCssColorSyntax,
    isValidCssBorderWidthValue,
    isValidCssBorderStyleValue,
    isValidCssBorderRadiusShorthand,
    isValidCssBorderCornerRadius,
    isValidCssFilterValue,
    isValidCssBoxShadowValue,
    isValidCssFlexNumberValue,
    isValidCssOrderValue,
    isValidCssZIndexValue,
    isValidCssFlexBasisValue,
    isValidCssGapValue,
    isValidCssAnimIterCountValue,
    isValidCssWritingModeValue,
    isValidCssDirectionValue,
    isValidCssUnicodeBidiValue,
    isValidCssFloatValue,
    isValidCssClearValue,
    isValidCssObjectFitValue,
    isValidCssTransformValue,
    isValidCssRotateValue,
    isValidCssScaleValue,
    isValidCssTimeListValue,
    isValidCssPointerEventsValue,
    isValidCssUserSelectValue,
    isValidCssResizeValue,
    isValidCssGridAutoFlowValue,
    isValidCssMathSyntax,
    isValidCssFlexDirectionValue,
    isValidCssFlexWrapValue,
    isValidCssAlignSelfOrItemsValue,
    isValidCssContentDistributionValue,
    isValidCssMarginShorthand,
    isValidCssPaddingShorthand,
    isValidCssOutlineStyleValue,
    isValidCssContainValue,
    isValidCssContentVisibilityValue,
    isValidCssIsolationValue,
    isValidCssMixBlendModeValue,
    isValidCssInsetValue,
    isValidCssOverflowValue,
} from "cap:css";

// ============================================================================
// 1. Border & Border-Radius Validators
// ============================================================================

test("css validation: border-width and border-style", () => {
    expect_eq(isValidCssBorderWidthValue("thin", 1), true);
    expect_eq(isValidCssBorderWidthValue("2px 4px", 4), true);
    expect_eq(isValidCssBorderWidthValue("thin medium thick 1rem", 4), true);
    expect_eq(isValidCssBorderWidthValue("1px 2px", 1), false);
    expect_eq(isValidCssBorderWidthValue("-2px", 1), false);
    expect_eq(isValidCssBorderWidthValue("", 4), false);

    expect_eq(isValidCssBorderStyleValue("solid", 1), true);
    expect_eq(isValidCssBorderStyleValue("solid dashed dotted double", 4), true);
    expect_eq(isValidCssBorderStyleValue("solid dashed", 1), false);
    expect_eq(isValidCssBorderStyleValue("wavy", 1), false);
    expect_eq(isValidCssBorderStyleValue("", 1), false);
});

test("css validation: border-radius shorthand and corner radius", () => {
    expect_eq(isValidCssBorderRadiusShorthand("8px"), true);
    expect_eq(isValidCssBorderRadiusShorthand("10px 20% 30px 40%"), true);
    expect_eq(isValidCssBorderRadiusShorthand("10px 20px / 5px 15px"), true);
    expect_eq(isValidCssBorderRadiusShorthand("10px / 20px / 30px"), false);
    expect_eq(isValidCssBorderRadiusShorthand("-5px"), false);
    expect_eq(isValidCssBorderRadiusShorthand("1px 2px 3px 4px 5px"), false);
    expect_eq(isValidCssBorderRadiusShorthand(""), false);

    expect_eq(isValidCssBorderCornerRadius("12px"), true);
    expect_eq(isValidCssBorderCornerRadius("12px 50%"), true);
    expect_eq(isValidCssBorderCornerRadius("12px 24px 36px"), false);
    expect_eq(isValidCssBorderCornerRadius("-4px"), false);
});

// ============================================================================
// 2. Effects (filter & box-shadow) Validators
// ============================================================================

test("css validation: filter and box-shadow", () => {
    expect_eq(isValidCssFilterValue("none"), true);
    expect_eq(isValidCssFilterValue("blur(4px)"), true);
    expect_eq(isValidCssFilterValue("blur(4px) brightness(120%) contrast(0.8)"), true);
    expect_eq(isValidCssFilterValue("var(--my-filter)"), true);
    expect_eq(isValidCssFilterValue("unknownfn(10px)"), false);
    expect_eq(isValidCssFilterValue("blur(4px"), false);
    expect_eq(isValidCssFilterValue(""), false);

    expect_eq(isValidCssBoxShadowValue("none"), true);
    expect_eq(isValidCssBoxShadowValue("2px 4px 6px rgba(0, 0, 0, 0.25)"), true);
    expect_eq(isValidCssBoxShadowValue("inset 0 1px 2px #000, 0 4px 8px red"), true);
    expect_eq(isValidCssBoxShadowValue("var(--shadow-lg)"), true);
    expect_eq(isValidCssBoxShadowValue("2px"), false);
    expect_eq(isValidCssBoxShadowValue("2px 4px 6px 8px 10px red"), false);
    expect_eq(isValidCssBoxShadowValue(""), false);
});

// ============================================================================
// 3. Flex / Grid / Box-Align Validators
// ============================================================================

test("css validation: flex-grow/shrink, order, grid-auto-flow and box-align", () => {
    expect_eq(isValidCssFlexNumberValue("0"), true);
    expect_eq(isValidCssFlexNumberValue("1.5"), true);
    expect_eq(isValidCssFlexNumberValue("calc(1 + 2)"), true);
    expect_eq(isValidCssFlexNumberValue("var(--grow)"), true);
    expect_eq(isValidCssFlexNumberValue("-1"), false);
    expect_eq(isValidCssFlexNumberValue("10px"), false);
    expect_eq(isValidCssFlexNumberValue(""), false);

    expect_eq(isValidCssOrderValue("0"), true);
    expect_eq(isValidCssOrderValue("-3"), true);
    expect_eq(isValidCssOrderValue("+5"), true);
    expect_eq(isValidCssOrderValue("1.5"), false);
    expect_eq(isValidCssOrderValue("auto"), false);

    expect_eq(isValidCssGridAutoFlowValue("row"), true);
    expect_eq(isValidCssGridAutoFlowValue("column dense"), true);
    expect_eq(isValidCssGridAutoFlowValue("dense row"), true);
    expect_eq(isValidCssGridAutoFlowValue("row column"), false);
    expect_eq(isValidCssGridAutoFlowValue("row dense extra"), false);
    expect_eq(isValidCssGridAutoFlowValue(""), false);

    expect_eq(isValidCssFlexDirectionValue("row"), true);
    expect_eq(isValidCssFlexDirectionValue("column-reverse"), true);
    expect_eq(isValidCssFlexDirectionValue("diagonal"), false);

    expect_eq(isValidCssFlexWrapValue("nowrap"), true);
    expect_eq(isValidCssFlexWrapValue("wrap-reverse"), true);
    expect_eq(isValidCssFlexWrapValue("scroll"), false);

    expect_eq(isValidCssAlignSelfOrItemsValue("auto", true), true);
    expect_eq(isValidCssAlignSelfOrItemsValue("auto", false), false);
    expect_eq(isValidCssAlignSelfOrItemsValue("center", false), true);
    expect_eq(isValidCssAlignSelfOrItemsValue("safe center", false), true);
    expect_eq(isValidCssAlignSelfOrItemsValue("bogus-align", true), false);

    expect_eq(isValidCssContentDistributionValue("space-between", true), true);
    expect_eq(isValidCssContentDistributionValue("safe flex-end", false), true);
    expect_eq(isValidCssContentDistributionValue("not-a-distribution", true), false);
});

// ============================================================================
// 4. Layout Control Validators
// ============================================================================

test("css validation: layoutctrl z-index, flex-basis, gap, anim-iter-count and keywords", () => {
    expect_eq(isValidCssZIndexValue("auto"), true);
    expect_eq(isValidCssZIndexValue("-10"), true);
    expect_eq(isValidCssZIndexValue("999"), true);
    expect_eq(isValidCssZIndexValue("1.5"), false);
    expect_eq(isValidCssZIndexValue("10px"), false);

    expect_eq(isValidCssFlexBasisValue("auto"), true);
    expect_eq(isValidCssFlexBasisValue("content"), true);
    expect_eq(isValidCssFlexBasisValue("fit-content(200px)"), true);
    expect_eq(isValidCssFlexBasisValue("50%"), true);
    expect_eq(isValidCssFlexBasisValue("-10px"), false);
    expect_eq(isValidCssFlexBasisValue("anchor(top)"), false);

    expect_eq(isValidCssGapValue("normal", 1), true);
    expect_eq(isValidCssGapValue("8px 16px", 2), true);
    expect_eq(isValidCssGapValue("8px 16px", 1), false);
    expect_eq(isValidCssGapValue("-4px", 2), false);

    expect_eq(isValidCssAnimIterCountValue("infinite"), true);
    expect_eq(isValidCssAnimIterCountValue("1, 2.5, infinite"), true);
    expect_eq(isValidCssAnimIterCountValue("-1"), false);
    expect_eq(isValidCssAnimIterCountValue("2s"), false);

    expect_eq(isValidCssWritingModeValue("horizontal-tb"), true);
    expect_eq(isValidCssWritingModeValue("vertical-rl"), true);
    expect_eq(isValidCssWritingModeValue("diagonal-tb"), false);

    expect_eq(isValidCssDirectionValue("ltr"), true);
    expect_eq(isValidCssDirectionValue("rtl"), true);
    expect_eq(isValidCssDirectionValue("ttb"), false);

    expect_eq(isValidCssUnicodeBidiValue("isolate-override"), true);
    expect_eq(isValidCssUnicodeBidiValue("plaintext"), true);
    expect_eq(isValidCssUnicodeBidiValue("auto"), false);

    expect_eq(isValidCssFloatValue("left"), true);
    expect_eq(isValidCssFloatValue("inline-start"), true);
    expect_eq(isValidCssFloatValue("center"), false);

    expect_eq(isValidCssClearValue("both"), true);
    expect_eq(isValidCssClearValue("inline-end"), true);
    expect_eq(isValidCssClearValue("top"), false);

    expect_eq(isValidCssObjectFitValue("cover"), true);
    expect_eq(isValidCssObjectFitValue("contain scale-down"), true);
    expect_eq(isValidCssObjectFitValue("contain cover"), false);
    expect_eq(isValidCssObjectFitValue("stretch"), false);
});

// ============================================================================
// 5. Transform, Rotate, Scale & Transition Time List Validators
// ============================================================================

test("css validation: transform, rotate, scale and time-list", () => {
    expect_eq(isValidCssTransformValue("none"), true);
    expect_eq(isValidCssTransformValue("translateX(10px) rotate(45deg) scale(1.2)"), true);
    expect_eq(isValidCssTransformValue("perspective(500px) rotate3d(1, 0, 0, 30deg)"), true);
    expect_eq(isValidCssTransformValue("perspective(500)"), false);
    expect_eq(isValidCssTransformValue("unknownTransform(10px)"), false);
    expect_eq(isValidCssTransformValue("rotate()"), false);

    expect_eq(isValidCssRotateValue("none"), true);
    expect_eq(isValidCssRotateValue("45deg"), true);
    expect_eq(isValidCssRotateValue("x 90deg"), true);
    expect_eq(isValidCssRotateValue("1 1 0 60deg"), true);
    expect_eq(isValidCssRotateValue("45px"), false);
    expect_eq(isValidCssRotateValue("1 2 45deg"), false);

    expect_eq(isValidCssScaleValue("none"), true);
    expect_eq(isValidCssScaleValue("1.5"), true);
    expect_eq(isValidCssScaleValue("120% 0.8 2"), true);
    expect_eq(isValidCssScaleValue("1 2 3 4"), false);
    expect_eq(isValidCssScaleValue("10px"), false);

    expect_eq(isValidCssTimeListValue("200ms", false, false), true);
    expect_eq(isValidCssTimeListValue("0.3s, 500ms", false, false), true);
    expect_eq(isValidCssTimeListValue("-150ms", true, false), true);
    expect_eq(isValidCssTimeListValue("-150ms", false, false), false);
    expect_eq(isValidCssTimeListValue("auto", false, true), true);
    expect_eq(isValidCssTimeListValue("auto", false, false), false);
    expect_eq(isValidCssTimeListValue("10px", false, false), false);
});

// ============================================================================
// 6. UI, MathFunc, Margin/Padding, Outline, Contain, Position & Overflow
// ============================================================================

test("css validation: ui, mathFunc, margin/padding, outline, contain, inset and overflow", () => {
    expect_eq(isValidCssPointerEventsValue("none"), true);
    expect_eq(isValidCssPointerEventsValue("visiblePainted"), true);
    expect_eq(isValidCssPointerEventsValue("clickable"), false);

    expect_eq(isValidCssUserSelectValue("none"), true);
    expect_eq(isValidCssUserSelectValue("text"), true);
    expect_eq(isValidCssUserSelectValue("invalid-select"), false);

    expect_eq(isValidCssResizeValue("both"), true);
    expect_eq(isValidCssResizeValue("horizontal"), true);
    expect_eq(isValidCssResizeValue("diagonal"), false);

    expect_eq(isValidCssMathSyntax("calc(100% - 20px)"), true);
    expect_eq(isValidCssMathSyntax("clamp(1rem, 2vw + 1rem, 3rem)"), true);
    expect_eq(isValidCssMathSyntax("min(10px, max(5px, 1em))"), true);
    expect_eq(isValidCssMathSyntax("calc(100% - )"), false);
    expect_eq(isValidCssMathSyntax("notamath(10px)"), false);
    expect_eq(isValidCssMathSyntax("10px"), false);

    expect_eq(isValidCssMarginShorthand("10px auto -5px 2rem"), true);
    expect_eq(isValidCssMarginShorthand("1px 2px 3px 4px 5px"), false);
    expect_eq(isValidCssPaddingShorthand("8px 16px"), true);
    expect_eq(isValidCssPaddingShorthand("-8px"), false);
    expect_eq(isValidCssPaddingShorthand("auto"), false);

    expect_eq(isValidCssOutlineStyleValue("auto"), true);
    expect_eq(isValidCssOutlineStyleValue("dashed"), true);
    expect_eq(isValidCssOutlineStyleValue("hidden"), false);

    expect_eq(isValidCssContainValue("strict"), true);
    expect_eq(isValidCssContainValue("layout style paint"), true);
    expect_eq(isValidCssContainValue("size inline-size"), false);
    expect_eq(isValidCssContainValue("layout layout"), false);

    expect_eq(isValidCssContentVisibilityValue("auto"), true);
    expect_eq(isValidCssContentVisibilityValue("hidden"), true);
    expect_eq(isValidCssContentVisibilityValue("collapse"), false);

    expect_eq(isValidCssIsolationValue("isolate"), true);
    expect_eq(isValidCssIsolationValue("auto"), true);
    expect_eq(isValidCssIsolationValue("none"), false);

    expect_eq(isValidCssMixBlendModeValue("multiply"), true);
    expect_eq(isValidCssMixBlendModeValue("difference"), true);
    expect_eq(isValidCssMixBlendModeValue("unknown-blend"), false);

    expect_eq(isValidCssInsetValue("auto", 1), true);
    expect_eq(isValidCssInsetValue("-10px 5% 0 auto", 4), true);
    expect_eq(isValidCssInsetValue("10px 20px", 1), false);
    expect_eq(isValidCssInsetValue("bogus", 4), false);

    expect_eq(isValidCssOverflowValue("hidden", 1), true);
    expect_eq(isValidCssOverflowValue("hidden scroll", 2), true);
    expect_eq(isValidCssOverflowValue("hidden scroll", 1), false);
    expect_eq(isValidCssOverflowValue("wrap", 2), false);

    expect_eq(isValidCssColorSyntax("rgb(/* c */ 255, 0, 0)"), true);
    expect_eq(isValidCssColorSyntax("not-a-color-xyz"), false);
});

// ============================================================================
// 7. CssColor.parse: CSS Color Level 4 (Comments, calc(), none)
// ============================================================================

test("css color4: comment stripping in CssColor.parse", () => {
    const c1 = CssColor.parse("rgb(/* r */ 255, /* g */ 0, /* b */ 128)") ?? return;
    expect_eq(c1.r, 255);
    expect_eq(c1.g, 0);
    expect_eq(c1.b, 128);
    expect_eq(c1.a, 255);

    const c2 = CssColor.parse("/* leading */ #0f8 /* trailing */") ?? return;
    expect_eq(c2.r, 0);
    expect_eq(c2.g, 255);
    expect_eq(c2.b, 136);

    const c3 = CssColor.parse("hsl(/* h */ 120deg /* s */ 100% /* l */ 50%)") ?? return;
    expect_eq(c3.r, 0);
    expect_eq(c3.g, 255);
    expect_eq(c3.b, 0);

    // Unclosed comment must fail cleanly
    expect_eq(CssColor.parse("rgb(/* unclosed 255, 0, 0)") == null, true);
});

test("css color4: channel calc() expressions in CssColor.parse", () => {
    const c1 = CssColor.parse("rgb(calc(100 + 155), calc(50% * 2), calc(200 - 200))") ?? return;
    expect_eq(c1.r, 255);
    expect_eq(c1.g, 255);
    expect_eq(c1.b, 0);
    expect_eq(c1.a, 255);

    const c2 = CssColor.parse("rgb(255 0 0 / calc(25% + 25%))") ?? return;
    expect_eq(c2.r, 255);
    expect_eq(c2.g, 0);
    expect_eq(c2.b, 0);
    expect_eq(c2.a, 128);

    const c3 = CssColor.parse("hsl(calc(60deg * 2) 100% 50%)") ?? return;
    expect_eq(c3.r, 0);
    expect_eq(c3.g, 255);
    expect_eq(c3.b, 0);

    // Incompatible unit in RGB channel calc() must fail
    expect_eq(CssColor.parse("rgb(calc(10deg) 0 0)") == null, true);
});

test("css color4: none keyword in CssColor.parse", () => {
    const c1 = CssColor.parse("rgb(none 128 255)") ?? return;
    expect_eq(c1.r, 0);
    expect_eq(c1.g, 128);
    expect_eq(c1.b, 255);
    expect_eq(c1.a, 255);

    const c2 = CssColor.parse("rgb(255 none none / none)") ?? return;
    expect_eq(c2.r, 255);
    expect_eq(c2.g, 0);
    expect_eq(c2.b, 0);
    expect_eq(c2.a, 0);

    const c3 = CssColor.parse("hsl(none 100% 50%)") ?? return;
    expect_eq(c3.r, 255);
    expect_eq(c3.g, 0);
    expect_eq(c3.b, 0);

    const c4 = CssColor.parse("oklch(0.7 0.1 none)") ?? return;
    expect_eq(c4.a, 255);
});

// ============================================================================
// 8. CssProperty.parse: Comments, Duplicate !important, Whitespace Custom Prop
// ============================================================================

test("css property parse: comment stripping around name, value and !important", () => {
    const p1 = CssProperty.parse("/* lead */ color /* prop */ : red /* tail */") ?? return;
    expect_eq(std.mem.eql(u8, p1.name, "color"), true);
    expect_eq(std.mem.eql(u8, p1.value, "red"), true);
    expect_eq(p1.important, false);

    const p2 = CssProperty.parse("color: blue /* before */ !important /* after */") ?? return;
    expect_eq(std.mem.eql(u8, p2.name, "color"), true);
    expect_eq(std.mem.eql(u8, p2.value, "blue"), true);
    expect_eq(p2.important, true);

    // Unclosed leading comment must be rejected
    expect_eq(CssProperty.parse("/* unclosed color: red") == null, true);
});

test("css property parse: reject duplicate !important and preserve whitespace custom prop", () => {
    expect_eq(CssProperty.parse("color: red !important !important") == null, true);
    expect_eq(CssProperty.parse("color: var(--a) !important /* c */ !important") == null, true);

    const custom_ws = CssProperty.parse("--custom: ") ?? return;
    expect_eq(std.mem.eql(u8, custom_ws.name, "--custom"), true);
    expect_eq(std.mem.eql(u8, custom_ws.value, " "), true);
    expect_eq(custom_ws.isCustomProperty(), true);

    // Custom property with no whitespace after colon is empty and rejected
    expect_eq(CssProperty.parse("--custom:") == null, true);
    // Non-custom property with only whitespace is rejected
    expect_eq(CssProperty.parse("color:   ") == null, true);
});

// ============================================================================
// 9. @supports: Custom Property Raw Value & Property Name Escape Characters
// ============================================================================

test("css supports: custom property raw value preservation and escaped property names", () => {
    const t1 = SupportsTree.parse("(--foo:  bar )") ?? return;
    const n1 = t1.nodes[t1.root];
    expect_eq(n1.kind == .declaration, true);
    expect_eq(std.mem.eql(u8, n1.prop, "--foo"), true);
    expect_eq(std.mem.eql(u8, n1.value, "  bar "), true);

    const t2 = SupportsTree.parse("(--empty: )") ?? return;
    const n2 = t2.nodes[t2.root];
    expect_eq(n2.kind == .declaration, true);
    expect_eq(std.mem.eql(u8, n2.prop, "--empty"), true);
    expect_eq(std.mem.eql(u8, n2.value, " "), true);

    // Escaped character inside property name
    const t3 = SupportsTree.parse("(foo\\-bar: 1)") ?? return;
    const n3 = t3.nodes[t3.root];
    expect_eq(n3.kind == .declaration, true);
    expect_eq(std.mem.eql(u8, n3.prop, "foo\\-bar"), true);
    expect_eq(std.mem.eql(u8, n3.value, "1"), true);

    // Trailing lone backslash in property name is not a valid declaration
    const t4 = SupportsTree.parse("(foo\\: 1)") ?? return;
    const n4 = t4.nodes[t4.root];
    expect_eq(n4.kind == .unknown, true);
});
