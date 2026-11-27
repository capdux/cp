import { test, expect_eq } from "cap:test";
import {
    isPcenChar,
    isReservedCustomElementName,
    isPotentialCustomElementName,
} from "../../packages/is-potential-custom-element-name/src/index.cp";

fn main() {
    test("1. @cap/is-potential-custom-element-name — accepts standard valid custom element names", () => {
        expect_eq(isPotentialCustomElementName("x-foo"), true);
        expect_eq(isPotentialCustomElementName("my-element-123"), true);
        expect_eq(isPotentialCustomElementName("a-"), true);
        expect_eq(isPotentialCustomElementName("foo-bar.baz_qux"), true);
    });

    test("2. @cap/is-potential-custom-element-name — accepts Unicode PCENChar names", () => {
        expect_eq(isPotentialCustomElementName("a-组件"), true);
        expect_eq(isPotentialCustomElementName("x-café"), true);
        expect_eq(isPotentialCustomElementName("a-🚀"), true);
    });

    test("3. @cap/is-potential-custom-element-name — rejects missing hyphen or uppercase ASCII", () => {
        expect_eq(isPotentialCustomElementName("div"), false);
        expect_eq(isPotentialCustomElementName("X-foo"), false);
        expect_eq(isPotentialCustomElementName("x-Foo"), false);
        expect_eq(isPotentialCustomElementName("-foo"), false);
        expect_eq(isPotentialCustomElementName("1-foo"), false);
        expect_eq(isPotentialCustomElementName(""), false);
    });

    test("4. @cap/is-potential-custom-element-name — rejects all 8 reserved SVG/MathML names", () => {
        expect_eq(isPotentialCustomElementName("annotation-xml"), false);
        expect_eq(isPotentialCustomElementName("color-profile"), false);
        expect_eq(isPotentialCustomElementName("font-face"), false);
        expect_eq(isPotentialCustomElementName("font-face-src"), false);
        expect_eq(isPotentialCustomElementName("font-face-uri"), false);
        expect_eq(isPotentialCustomElementName("font-face-format"), false);
        expect_eq(isPotentialCustomElementName("font-face-name"), false);
        expect_eq(isPotentialCustomElementName("missing-glyph"), false);
        expect_eq(isReservedCustomElementName("font-face"), true);
    });

    test("5. @cap/is-potential-custom-element-name — rejects whitespace and forbidden punctuation", () => {
        expect_eq(isPotentialCustomElementName("x-foo bar"), false);
        expect_eq(isPotentialCustomElementName("x-foo:bar"), false);
        expect_eq(isPotentialCustomElementName("x-foo/bar"), false);
    });

    test("6. @cap/is-potential-custom-element-name — isPcenChar boundary checks", () => {
        expect_eq(isPcenChar(0x2D), true);  // '-'
        expect_eq(isPcenChar(0x41), false); // 'A'
        expect_eq(isPcenChar(0xD7), false); // multiplication sign excluded
        expect_eq(isPcenChar(0xF7), false); // division sign excluded
        expect_eq(isPcenChar(0xB7), true);  // middle dot
    });
}
