import { test, expect_eq } from "cap:test";
import {
    A11yNodeInput,
    collapseWhitespace,
    computeAccessibleName,
    computeAccessibleDescription,
} from "../../packages/dom-accessibility-api/src/index.cp";

fn main() {
    test("1. @cap/dom-accessibility-name — collapseWhitespace trims and collapses runs", () => {
        let buf: [128]u8 = [0; 128];
        const n = collapseWhitespace("  \t Hello\n\r   World  \t ", buf[0..128]);
        expect_eq(buf[0..n] as string, "Hello World");

        const n_empty = collapseWhitespace("   \t\n  ", buf[0..128]);
        expect_eq(n_empty, 0 as usize);
    });

    test("2. @cap/dom-accessibility-name — aria_hidden suppresses accessible name", () => {
        const node = A11yNodeInput{
            tag: "button",
            aria_label: "Close",
            labelled_by_text: "Dialog Close",
            inner_text: "X",
            alt: "",
            title: "Close dialog",
            placeholder: "",
            aria_hidden: true,
        };
        let buf: [128]u8 = [0; 128];
        expect_eq(computeAccessibleName(node, buf[0..128]), 0 as usize);
    });

    test("3. @cap/dom-accessibility-name — labelled_by_text takes precedence over aria_label and inner_text", () => {
        const node = A11yNodeInput{
            tag: "button",
            aria_label: "Fallback Label",
            labelled_by_text: "  Submit \n Form  ",
            inner_text: "Go",
            alt: "",
            title: "Tooltip",
            placeholder: "",
            aria_hidden: false,
        };
        let buf: [128]u8 = [0; 128];
        const n = computeAccessibleName(node, buf[0..128]);
        expect_eq(buf[0..n] as string, "Submit Form");
    });

    test("4. @cap/dom-accessibility-name — img/area uses alt before inner_text and title", () => {
        const img = A11yNodeInput{
            tag: "IMG",
            aria_label: "   ",
            labelled_by_text: "",
            inner_text: "ignored",
            alt: "  Company   Logo ",
            title: "Home",
            placeholder: "",
            aria_hidden: false,
        };
        let buf: [128]u8 = [0; 128];
        const n = computeAccessibleName(img, buf[0..128]);
        expect_eq(buf[0..n] as string, "Company Logo");
    });

    test("5. @cap/dom-accessibility-name — falls back through inner_text > placeholder > title", () => {
        let buf: [128]u8 = [0; 128];
        const input_ph = A11yNodeInput{
            tag: "input",
            aria_label: "",
            labelled_by_text: "",
            inner_text: "",
            alt: "unused",
            title: "Email tooltip",
            placeholder: "Enter email",
            aria_hidden: false,
        };
        const n1 = computeAccessibleName(input_ph, buf[0..128]);
        expect_eq(buf[0..n1] as string, "Enter email");

        const title_only = A11yNodeInput{
            tag: "div",
            aria_label: "",
            labelled_by_text: "",
            inner_text: "   ",
            alt: "",
            title: "  Info   Panel ",
            placeholder: "",
            aria_hidden: false,
        };
        const n2 = computeAccessibleName(title_only, buf[0..128]);
        expect_eq(buf[0..n2] as string, "Info Panel");
    });

    test("6. @cap/dom-accessibility-name — computeAccessibleDescription respects used_title_as_name", () => {
        let buf: [128]u8 = [0; 128];
        const n1 = computeAccessibleDescription("  Detailed   help ", "Title", true, buf[0..128]);
        expect_eq(buf[0..n1] as string, "Detailed help");

        const n2 = computeAccessibleDescription("", "  Tooltip  text ", false, buf[0..128]);
        expect_eq(buf[0..n2] as string, "Tooltip text");

        const n3 = computeAccessibleDescription("", "Tooltip text", true, buf[0..128]);
        expect_eq(n3, 0 as usize);
    });
}
