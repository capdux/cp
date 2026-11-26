import { test, expect_eq } from "cap:test";
import {
    PropInfo,
    findHtmlProp,
    isBooleanAttr,
    isSpaceSeparatedAttr,
    isCommaSeparatedAttr,
    attrToDomProp,
    domPropToAttr,
} from "../../packages/@cap/property-information/src/index.cp";

fn main() {
    test("1. @cap/property-information — bidirectional HTML/DOM name mapping (case-insensitive)", () => {
        expect_eq(attrToDomProp("class"), "className");
        expect_eq(attrToDomProp("CLASS"), "className");
        expect_eq(domPropToAttr("className"), "class");
        expect_eq(attrToDomProp("for"), "htmlFor");
        expect_eq(domPropToAttr("htmlFor"), "for");
        expect_eq(attrToDomProp("tabindex"), "tabIndex");
        expect_eq(domPropToAttr("tabIndex"), "tabindex");
        expect_eq(attrToDomProp("readonly"), "readOnly");
        expect_eq(domPropToAttr("readOnly"), "readonly");
        expect_eq(attrToDomProp("maxlength"), "maxLength");
        expect_eq(attrToDomProp("colspan"), "colSpan");
        expect_eq(attrToDomProp("rowspan"), "rowSpan");
    });

    test("2. @cap/property-information — classifies boolean attributes", () => {
        expect_eq(isBooleanAttr("disabled"), true);
        expect_eq(isBooleanAttr("CHECKED"), true);
        expect_eq(isBooleanAttr("readonly"), true);
        expect_eq(isBooleanAttr("readOnly"), true);
        expect_eq(isBooleanAttr("autofocus"), true);
        expect_eq(isBooleanAttr("novalidate"), true);
        expect_eq(isBooleanAttr("defer"), true);
        expect_eq(isBooleanAttr("async"), true);
        expect_eq(isBooleanAttr("open"), true);
        expect_eq(isBooleanAttr("class"), false);
    });

    test("3. @cap/property-information — classifies space-separated attributes", () => {
        expect_eq(isSpaceSeparatedAttr("class"), true);
        expect_eq(isSpaceSeparatedAttr("className"), true);
        expect_eq(isSpaceSeparatedAttr("rel"), true);
        expect_eq(isSpaceSeparatedAttr("sandbox"), true);
        expect_eq(isSpaceSeparatedAttr("headers"), true);
        expect_eq(isSpaceSeparatedAttr("ping"), true);
        expect_eq(isSpaceSeparatedAttr("for"), true);
        expect_eq(isSpaceSeparatedAttr("disabled"), false);
    });

    test("4. @cap/property-information — classifies comma-separated and numeric attributes", () => {
        expect_eq(isCommaSeparatedAttr("accept"), true);
        expect_eq(isCommaSeparatedAttr("srcset"), true);
        expect_eq(isCommaSeparatedAttr("srcSet"), true);
        expect_eq(isCommaSeparatedAttr("class"), false);

        const tab = findHtmlProp("tabIndex");
        expect_eq(tab.found, true);
        expect_eq(tab.is_numeric, true);
        expect_eq(tab.attribute, "tabindex");

        const cs = findHtmlProp("colspan");
        expect_eq(cs.found, true);
        expect_eq(cs.is_numeric, true);
        expect_eq(cs.property, "colSpan");
    });

    test("5. @cap/property-information — recognizes aria-* and data-* attributes", () => {
        const ariaLabel = findHtmlProp("aria-label");
        expect_eq(ariaLabel.found, true);
        expect_eq(ariaLabel.attribute, "aria-label");
        expect_eq(ariaLabel.is_space_separated, false);

        const ariaBy = findHtmlProp("aria-labelledby");
        expect_eq(ariaBy.found, true);
        expect_eq(ariaBy.is_space_separated, true);

        const dataId = findHtmlProp("data-user-id");
        expect_eq(dataId.found, true);
        expect_eq(dataId.property, "data-user-id");
    });

    test("6. @cap/property-information — handles unknown attributes gracefully", () => {
        const unk = findHtmlProp("x-custom-foo");
        expect_eq(unk.found, false);
        expect_eq(unk.property, "x-custom-foo");
        expect_eq(attrToDomProp("x-custom-foo"), "x-custom-foo");
        expect_eq(domPropToAttr("xCustomFoo"), "xCustomFoo");
        expect_eq(isBooleanAttr("x-custom-foo"), false);
    });
}
