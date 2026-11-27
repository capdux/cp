import { test, expect_eq } from "cap:test";
import { CSS_STYLE_CAP, CssProp, CssStyleDecl } from "../../packages/cssstyle/src/index.cp";

fn main() {
    test("1. @cap/cssstyle — Parse basic inline declarations and query values/priorities", () => {
        let style: CssStyleDecl = CssStyleDecl.new();
        expect_eq(style.len(), 0);
        style.parse("color: red; background-color: #fff; font-size: 16px !important;");
        expect_eq(style.len(), 3);
        expect_eq(style.getPropertyValue("color"), "red");
        expect_eq(style.getPropertyPriority("color"), "");
        expect_eq(style.getPropertyValue("background-color"), "#fff");
        expect_eq(style.getPropertyValue("font-size"), "16px");
        expect_eq(style.getPropertyPriority("font-size"), "important");
        expect_eq(style.getPropertyValue("missing"), "");
        expect_eq(style.getPropertyPriority("missing"), "");
    });

    test("2. @cap/cssstyle — Ignore /* ... */ comments and handle quoted semicolons", () => {
        let style: CssStyleDecl = CssStyleDecl.new();
        style.parse("/* leading */ color /* mid */ : blue /* trailing */ !IMPORTANT; content: \"a;b:c\"; /* end */");
        expect_eq(style.len(), 2);
        expect_eq(style.getPropertyValue("color"), "blue");
        expect_eq(style.getPropertyPriority("color"), "important");
        expect_eq(style.getPropertyValue("content"), "\"a;b:c\"");
    });

    test("3. @cap/cssstyle — Case-insensitive lookup and duplicate property overwrite", () => {
        let style: CssStyleDecl = CssStyleDecl.new();
        style.parse("Color: red; MARGIN-TOP: 10px; color: green !important;");
        expect_eq(style.len(), 2);
        expect_eq(style.getPropertyValue("COLOR"), "green");
        expect_eq(style.getPropertyPriority("Color"), "important");
        expect_eq(style.getPropertyValue("margin-top"), "10px");
        expect_eq(style.getPropertyPriority("MARGIN-TOP"), "");
    });

    test("4. @cap/cssstyle — setProperty() update/insert and removeProperty() order preservation", () => {
        let style: CssStyleDecl = CssStyleDecl.new();
        expect_eq(style.setProperty("display", "flex", false), true);
        expect_eq(style.setProperty("gap", "8px", true), true);
        expect_eq(style.setProperty("align-items", "center", false), true);
        expect_eq(style.len(), 3);

        // Overwrite existing property case-insensitively
        expect_eq(style.setProperty("GAP", "12px", false), true);
        expect_eq(style.len(), 3);
        expect_eq(style.getPropertyValue("gap"), "12px");
        expect_eq(style.getPropertyPriority("gap"), "");

        // Remove middle property
        expect_eq(style.removeProperty("GAP"), true);
        expect_eq(style.removeProperty("gap"), false);
        expect_eq(style.len(), 2);
        expect_eq(style.getPropertyValue("display"), "flex");
        expect_eq(style.getPropertyValue("align-items"), "center");

        // Invalid empty name fails
        expect_eq(style.setProperty("   ", "10px", false), false);
    });

    test("5. @cap/cssstyle — toCssText() serialization with !important and lowercase names", () => {
        let style: CssStyleDecl = CssStyleDecl.new();
        style.parse("COLOR: crimson; Font-Weight: 700 !important; padding: 4px 8px");
        let buf: [256]u8 = undefined;
        const n: usize = style.toCssText(buf[0..]);
        expect_eq(buf[0..n], "color: crimson; font-weight: 700 !important; padding: 4px 8px;");

        // Truncation safety with small output buffer
        let small: [10]u8 = undefined;
        const m: usize = style.toCssText(small[0..]);
        expect_eq(m, 10);
        expect_eq(small[0..m], "color: cri");
    });

    test("6. @cap/cssstyle — Empty value removes property and CssProp/capacity bounds", () => {
        let style: CssStyleDecl = CssStyleDecl.new();
        const p: CssProp = CssProp { name: "z-index", value: "10", important: true };
        expect_eq(style.setProperty(p.name, p.value, p.important), true);
        expect_eq(style.getPropertyValue("z-index"), "10");
        expect_eq(style.getPropertyPriority("z-index"), "important");

        // Setting empty value removes property
        expect_eq(style.setProperty("z-index", "   ", false), true);
        expect_eq(style.len(), 0);
        expect_eq(CSS_STYLE_CAP, 64);

        let buf: [32]u8 = undefined;
        expect_eq(style.toCssText(buf[0..]), 0);
    });
}
