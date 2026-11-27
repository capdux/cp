import { test, expect_eq } from "cap:test";
import {
    NAME_SOURCE_NONE,
    NAME_SOURCE_ARIA_LABELLEDBY,
    NAME_SOURCE_ARIA_LABEL,
    NAME_SOURCE_ALT,
    NAME_SOURCE_HOST_LABEL,
    NAME_SOURCE_VALUE,
    NAME_SOURCE_CONTENT,
    NAME_SOURCE_PLACEHOLDER,
    NAME_SOURCE_TITLE,
    A11yNodeInput,
    A11yNodeEx,
    ComputeNameOptions,
    defaultComputeOptions,
    defaultA11yNodeEx,
    isPresentationRole,
    isHiddenElement,
    allowsNameFromContent,
    collapseWhitespace,
    computeAccessibleName,
    computeAccessibleNameWithOptions,
    computeAccessibleDescription,
    computeAccessibleDescriptionForNode,
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

    test("7. @cap/dom-accessibility-name — W3C AccName host labels (input label/submit/fieldset/table/figure)", () => {
        let buf: [128]u8 = [0; 128];
        const opts = defaultComputeOptions();

        let inp = defaultA11yNodeEx("input");
        inp.label_text = "  Username  ";
        inp.placeholder = "type user";
        const r1 = computeAccessibleNameWithOptions(inp, opts, buf[0..128]);
        expect_eq(r1.source, NAME_SOURCE_HOST_LABEL);
        expect_eq(buf[0..r1.len] as string, "Username");

        let sub = defaultA11yNodeEx("input");
        sub.input_type = "submit";
        const r2 = computeAccessibleNameWithOptions(sub, opts, buf[0..128]);
        expect_eq(r2.source, NAME_SOURCE_VALUE);
        expect_eq(buf[0..r2.len] as string, "Submit");

        let fs = defaultA11yNodeEx("fieldset");
        fs.legend_text = "Billing Address";
        const r3 = computeAccessibleNameWithOptions(fs, opts, buf[0..128]);
        expect_eq(r3.source, NAME_SOURCE_HOST_LABEL);
        expect_eq(buf[0..r3.len] as string, "Billing Address");

        let tbl = defaultA11yNodeEx("table");
        tbl.caption_text = "Quarterly Sales";
        const r4 = computeAccessibleNameWithOptions(tbl, opts, buf[0..128]);
        expect_eq(buf[0..r4.len] as string, "Quarterly Sales");

        let fig = defaultA11yNodeEx("figure");
        fig.figcaption_text = "Architecture Diagram";
        const r5 = computeAccessibleNameWithOptions(fig, opts, buf[0..128]);
        expect_eq(buf[0..r5.len] as string, "Architecture Diagram");
    });

    test("8. @cap/dom-accessibility-name — include_hidden, presentation role & description for node", () => {
        let buf: [128]u8 = [0; 128];
        let hidden_btn = defaultA11yNodeEx("button");
        hidden_btn.aria_label = "Secret Action";
        hidden_btn.aria_hidden = true;

        const r_hidden = computeAccessibleNameWithOptions(hidden_btn, defaultComputeOptions(), buf[0..128]);
        expect_eq(r_hidden.len, 0 as usize);
        expect_eq(r_hidden.source, NAME_SOURCE_NONE);

        let inc_opts = defaultComputeOptions();
        inc_opts.include_hidden = true;
        const r_shown = computeAccessibleNameWithOptions(hidden_btn, inc_opts, buf[0..128]);
        expect_eq(r_shown.source, NAME_SOURCE_ARIA_LABEL);
        expect_eq(buf[0..r_shown.len] as string, "Secret Action");

        expect_eq(isPresentationRole("presentation"), true);
        expect_eq(isPresentationRole("none"), true);
        expect_eq(isPresentationRole("button"), false);
        expect_eq(isHiddenElement(false, true, false), true);
        expect_eq(isHiddenElement(false, true, true), false);
        expect_eq(allowsNameFromContent("button", ""), true);
        expect_eq(allowsNameFromContent("div", "presentation"), false);

        let field = defaultA11yNodeEx("input");
        field.label_text = "Password";
        field.described_by_text = "At least 12 characters";
        field.title = "Password tooltip";
        const name_res = computeAccessibleNameWithOptions(field, defaultComputeOptions(), buf[0..128]);
        expect_eq(name_res.used_title, false);
        const desc_len = computeAccessibleDescriptionForNode(
            field,
            defaultComputeOptions(),
            name_res.used_title,
            name_res.used_placeholder,
            buf[0..128],
        );
        expect_eq(buf[0..desc_len] as string, "At least 12 characters");
    });
}
