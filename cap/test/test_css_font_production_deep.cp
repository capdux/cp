import { test, expect_eq } from "cap:test";
import {
    CssFontError,
    FontWeightKind,
    FontWeight,
    FontStyleKind,
    FontStyle,
    FontStretch,
    AbsoluteFontSize,
    FontSizeKind,
    FontSize,
    LineHeightKind,
    LineHeight,
    FontVariantCaps,
    GenericFontFamily,
    Font,
} from "cap:css_font";

test("1. FontWeight (normal/bold/bolder/lighter/1..1000) & FontStyle (normal/italic/oblique deg)", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const w_norm = FontWeight.parse("400") catch unreachable;
    expect_eq(w_norm.isDefault(), true);
    expect_eq(w_norm.isBold(), false);
    const n1 = w_norm.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "normal"), true);

    const w_bold = FontWeight.parse("700") catch unreachable;
    expect_eq(w_bold.isBold(), true);
    const n2 = w_bold.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n2], "bold"), true);

    const w_600 = FontWeight.parse("600") catch unreachable;
    expect_eq(w_600.value, 600.0);
    expect_eq((FontWeight.parse("bolder") catch unreachable).kind == FontWeightKind.bolder, true);

    const s_ob = FontStyle.parse("oblique 20deg") catch unreachable;
    expect_eq(s_ob.kind == FontStyleKind.oblique, true);
    expect_eq(s_ob.angle_deg, 20.0);
    const n3 = s_ob.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n3], "oblique 20deg"), true);

    // Default oblique 14deg omits angle
    const s_ob_def = FontStyle.parse("oblique 14deg") catch unreachable;
    const n4 = s_ob_def.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n4], "oblique"), true);
});

test("2. FontStretch, FontSize, LineHeight, FontVariantCaps & all 13 GenericFontFamily variants", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const st = FontStretch.parse("75%") catch unreachable;
    const ns = st.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..ns], "condensed"), true);
    expect_eq((FontStretch.parse("ultra-expanded") catch unreachable).percent, 200.0);

    const sz = FontSize.parse("xx-large") catch unreachable;
    expect_eq(sz.kind == FontSizeKind.absolute, true);
    expect_eq(sz.abs == AbsoluteFontSize.xx_large, true);

    const lh = LineHeight.parse("1.5") catch unreachable;
    expect_eq(lh.kind == LineHeightKind.number, true);
    expect_eq(lh.value, 1.5);

    expect_eq(FontVariantCaps.parse("small-caps").? == FontVariantCaps.small_caps, true);
    expect_eq(std.mem.eql(u8, FontVariantCaps.titling_caps.toCss(), "titling-caps"), true);

    const generics: [13]string = [
        "serif", "sans-serif", "monospace", "cursive", "fantasy",
        "system-ui", "ui-serif", "ui-sans-serif", "ui-monospace", "ui-rounded",
        "emoji", "math", "fangsong",
    ];
    for (gname of generics) {
        const gf = GenericFontFamily.parse(gname).?;
        expect_eq(std.mem.eql(u8, gf.toCss(), gname), true);
    }
});

test("3. Full Font shorthand parse (italic bold 16px/1.5 \"Helvetica Neue\", sans-serif) & canonical toCss", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const f = Font.parse("italic bold 16px/1.5 \"Helvetica Neue\", sans-serif") catch unreachable;
    expect_eq(f.style.kind == FontStyleKind.italic, true);
    expect_eq(f.weight.isBold(), true);
    expect_eq(f.size.kind == FontSizeKind.px, true);
    expect_eq(f.size.value, 16.0);
    expect_eq(f.line_height.kind == LineHeightKind.number, true);
    expect_eq(f.line_height.value, 1.5);
    expect_eq(f.family_count, usize(2));
    expect_eq(std.mem.eql(u8, f.families[0].name, "Helvetica Neue"), true);
    expect_eq(f.families[1].generic.? == GenericFontFamily.sans_serif, true);

    const n = f.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n], "italic bold 16px/1.5 \"Helvetica Neue\", sans-serif"), true);
});

test("4. Font shorthand default omission & spaced slash syntax (14px / normal)", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const f = Font.parse("normal 400 14px / normal monospace") catch unreachable;
    expect_eq(f.style.isDefault(), true);
    expect_eq(f.weight.isDefault(), true);
    expect_eq(f.line_height.isDefault(), true);

    // Defaults omitted in canonical output
    const n = f.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n], "14px monospace"), true);
});

test("5. Numeric range & malformed Font shorthand boundary guards", () => {
    // FontWeight out of 1..1000
    std.testing.expectError(CssFontError.InvalidWeight, FontWeight.parse("0")) catch unreachable;
    std.testing.expectError(CssFontError.InvalidWeight, FontWeight.parse("1001")) catch unreachable;
    std.testing.expectError(CssFontError.InvalidWeight, FontWeight.parse("-100")) catch unreachable;

    // FontStyle oblique angle outside -90deg..90deg
    std.testing.expectError(CssFontError.InvalidStyle, FontStyle.parse("oblique 95deg")) catch unreachable;
    std.testing.expectError(CssFontError.InvalidStyle, FontStyle.parse("oblique -91deg")) catch unreachable;

    // Negative size / line-height / stretch
    std.testing.expectError(CssFontError.InvalidSize, FontSize.parse("-12px")) catch unreachable;
    std.testing.expectError(CssFontError.InvalidLineHeight, LineHeight.parse("-1.2")) catch unreachable;
    std.testing.expectError(CssFontError.InvalidStretch, FontStretch.parse("-50%")) catch unreachable;

    // Font shorthand missing family or size
    std.testing.expectError(CssFontError.InvalidFontShorthand, Font.parse("")) catch unreachable;
    std.testing.expectError(CssFontError.InvalidFontShorthand, Font.parse("italic bold sans-serif")) catch unreachable;
    std.testing.expectError(CssFontError.InvalidFontShorthand, Font.parse("16px")) catch unreachable;
});
