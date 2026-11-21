import { test, expect_eq } from "cap:test";
import {
    CssTextError,
    TextTransformCase,
    TextTransform,
    WhiteSpace,
    WordBreak,
    LineBreak,
    Hyphens,
    OverflowWrap,
    TextAlign,
    TextDecorationLine,
    TextDecorationStyle,
    TextDecorationThicknessKind,
    TextDecoration,
} from "cap:css_text";

test("1. TextTransform combined case + full-width + full-size-kana parse & serialize", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const tt_none = TextTransform.parse("none") catch unreachable;
    expect_eq(tt_none.isDefault(), true);
    const n0 = tt_none.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n0], "none"), true);

    const tt = TextTransform.parse("full-width UPPERCASE full-size-kana") catch unreachable;
    expect_eq(tt.case_mode == TextTransformCase.uppercase, true);
    expect_eq(tt.hasFullWidth(), true);
    expect_eq(tt.hasFullSizeKana(), true);
    const n1 = tt.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "uppercase full-width full-size-kana"), true);
});

test("2. WhiteSpace, WordBreak, LineBreak, Hyphens, OverflowWrap & TextAlign enums", () => {
    expect_eq(WhiteSpace.pre_wrap.preservesNewlines(), true);
    expect_eq(WhiteSpace.pre_wrap.allowsWrapping(), true);
    expect_eq(WhiteSpace.nowrap.allowsWrapping(), false);
    expect_eq(WhiteSpace.parse("break-spaces").? == WhiteSpace.break_spaces, true);

    expect_eq(WordBreak.parse("keep-all").? == WordBreak.keep_all, true);
    expect_eq(LineBreak.parse("strict").? == LineBreak.strict, true);
    expect_eq(Hyphens.default() == Hyphens.manual, true);
    expect_eq(Hyphens.parse("auto").? == Hyphens.auto, true);
    expect_eq(OverflowWrap.parse("anywhere").? == OverflowWrap.anywhere, true);
    expect_eq(TextAlign.parse("justify-all").? == TextAlign.justify_all, true);
    expect_eq(std.mem.eql(u8, TextAlign.match_parent.toCss(), "match-parent"), true);
});

test("3. TextDecorationLine multi-bit flags parse & canonical serialize", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const dl = TextDecorationLine.parse("line-through underline overline") catch unreachable;
    expect_eq(dl.isNone(), false);
    expect_eq(dl.hasUnderline(), true);
    expect_eq(dl.hasOverline(), true);
    expect_eq(dl.hasLineThrough(), true);
    expect_eq(dl.hasBlink(), false);

    const n = dl.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n], "underline overline line-through"), true);
});

test("4. TextDecoration shorthand order-independent parse & default omission", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const td = TextDecoration.parse("2px wavy #ff0000 underline line-through") catch unreachable;
    expect_eq(td.line.hasUnderline(), true);
    expect_eq(td.line.hasLineThrough(), true);
    expect_eq(td.style == TextDecorationStyle.wavy, true);
    expect_eq(std.mem.eql(u8, td.color.?, "#ff0000"), true);
    expect_eq(td.thickness.kind == TextDecorationThicknessKind.px, true);
    expect_eq(td.thickness.value, 2.0);

    const n1 = td.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "underline line-through wavy #ff0000 2px"), true);

    // Defaults (solid, currentcolor, auto) omitted
    const td_def = TextDecoration.parse("underline solid currentcolor auto") catch unreachable;
    const n2 = td_def.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n2], "underline"), true);
});

test("5. Invalid inputs & duplicate token guards", () => {
    std.testing.expectError(CssTextError.InvalidTextTransform, TextTransform.parse("none uppercase")) catch unreachable;
    std.testing.expectError(CssTextError.InvalidTextTransform, TextTransform.parse("uppercase lowercase")) catch unreachable;
    std.testing.expectError(CssTextError.InvalidDecorationLine, TextDecorationLine.parse("underline underline")) catch unreachable;
    std.testing.expectError(CssTextError.InvalidTextDecoration, TextDecoration.parse("none underline")) catch unreachable;
    std.testing.expectError(CssTextError.InvalidTextDecoration, TextDecoration.parse("solid wavy underline")) catch unreachable;
    std.testing.expectError(CssTextError.InvalidTextDecoration, TextDecoration.parse("")) catch unreachable;
});
