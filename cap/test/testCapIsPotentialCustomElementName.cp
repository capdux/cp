import { test, expect_eq } from "cap:test";
import { strEq } from "cap:ascii";
import {
    RESERVED_CUSTOM_ELEMENT_NAMES_COUNT,
    PcenError,
    isValidCustomElementStart,
    isPcenChar,
    reservedCustomElementNameAt,
    isReservedCustomElementName,
    validateCustomElementName,
    isPotentialCustomElementName,
    sanitizeCustomElementName,
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

    test("4. @cap/is-potential-custom-element-name — rejects & enumerates all 8 reserved SVG/MathML names", () => {
        expect_eq(RESERVED_CUSTOM_ELEMENT_NAMES_COUNT, 8 as usize);
        let idx: usize = 0;
        while (idx < RESERVED_CUSTOM_ELEMENT_NAMES_COUNT) : (idx += 1) {
            const name = reservedCustomElementNameAt(idx).?;
            expect_eq(isReservedCustomElementName(name), true);
            expect_eq(isPotentialCustomElementName(name), false);
        }
        expect_eq(reservedCustomElementNameAt(8) == null, true);
    });

    test("5. @cap/is-potential-custom-element-name — rejects whitespace, forbidden punctuation & invalid UTF-8", () => {
        expect_eq(isPotentialCustomElementName("x-foo bar"), false);
        expect_eq(isPotentialCustomElementName("x-foo:bar"), false);
        expect_eq(isPotentialCustomElementName("x-foo/bar"), false);

        const bad_utf8: [4]u8 = ['a', '-', 0xFF, 0xFE];
        const r = validateCustomElementName(bad_utf8[0..4] as string);
        expect_eq(r.valid, false);
        expect_eq(r.err == PcenError.InvalidUtf8, true);
        expect_eq(r.error_offset, 2 as usize);
    });

    test("6. @cap/is-potential-custom-element-name — isPcenChar & isValidCustomElementStart boundary checks", () => {
        expect_eq(isValidCustomElementStart('a'), true);
        expect_eq(isValidCustomElementStart('z'), true);
        expect_eq(isValidCustomElementStart('A'), false);
        expect_eq(isValidCustomElementStart('-'), false);

        expect_eq(isPcenChar(0x2D), true);  // '-'
        expect_eq(isPcenChar(0x41), false); // 'A'
        expect_eq(isPcenChar(0xD7), false); // multiplication sign excluded
        expect_eq(isPcenChar(0xF7), false); // division sign excluded
        expect_eq(isPcenChar(0xB7), true);  // middle dot
        expect_eq(isPcenChar(0x37E), false); // Greek question mark excluded
        expect_eq(isPcenChar(0x200C), true); // ZWNJ
        expect_eq(isPcenChar(0xF0000), false); // above 0xEFFFF
    });

    test("7. @cap/is-potential-custom-element-name — validateCustomElementName structured diagnostics", () => {
        const r_empty = validateCustomElementName("");
        expect_eq(r_empty.err == PcenError.Empty, true);

        const r_short = validateCustomElementName("a");
        expect_eq(r_short.err == PcenError.TooShort, true);

        const r_start = validateCustomElementName("A-foo");
        expect_eq(r_start.err == PcenError.InvalidStartChar, true);

        const r_res = validateCustomElementName("font-face");
        expect_eq(r_res.err == PcenError.ReservedName, true);

        const r_no_hyp = validateCustomElementName("custom");
        expect_eq(r_no_hyp.err == PcenError.MissingHyphen, true);

        const r_bad_ch = validateCustomElementName("a-bad:char");
        expect_eq(r_bad_ch.err == PcenError.InvalidChar, true);
        expect_eq(r_bad_ch.error_offset, 5 as usize);

        const r_ok = validateCustomElementName("my-widget");
        expect_eq(r_ok.valid, true);
        expect_eq(r_ok.err == PcenError.Ok, true);
    });

    test("8. @cap/is-potential-custom-element-name — sanitizeCustomElementName produces valid names", () => {
        let buf: [64]u8 = undefined;
        const n1 = sanitizeCustomElementName("My Widget!", buf[0..64]);
        expect_eq(isPotentialCustomElementName(buf[0..n1] as string), true);
        expect_eq(strEq(buf[0..n1] as string, "my-widget-"), true);

        const n2 = sanitizeCustomElementName("123bad", buf[0..64]);
        expect_eq(isPotentialCustomElementName(buf[0..n2] as string), true);
        expect_eq(strEq(buf[0..n2] as string, "x-123bad"), true);

        const n3 = sanitizeCustomElementName("font-face", buf[0..64]);
        expect_eq(isPotentialCustomElementName(buf[0..n3] as string), true);
        expect_eq(strEq(buf[0..n3] as string, "font-face-el"), true);
    });
}
