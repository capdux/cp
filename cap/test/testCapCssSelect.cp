import { test, expect_eq } from "cap:test";
import {
    Specificity,
    computeSpecificity,
    CssElement,
    matchesCompoundSelector,
    matchesSelectorList,
    countSelectorParts,
} from "../../packages/css-select/src/index.cp";

fn main() {
    test("1. @cap/css-select — W3C Selectors Level 4 specificity calculation", () => {
        const s_univ = computeSpecificity("*");
        expect_eq(s_univ.ids, 0);
        expect_eq(s_univ.classes, 0);
        expect_eq(s_univ.tags, 0);

        const s_tag = computeSpecificity("ul > li::before");
        expect_eq(s_tag.ids, 0);
        expect_eq(s_tag.classes, 0);
        expect_eq(s_tag.tags, 3);

        const s_complex = computeSpecificity("div#app.card.active[data-id=\"42\"]:hover::after");
        expect_eq(s_complex.ids, 1);
        expect_eq(s_complex.classes, 4);
        expect_eq(s_complex.tags, 2);

        const s_where = computeSpecificity("p:where(#ignored.cls)");
        expect_eq(s_where.ids, 0);
        expect_eq(s_where.classes, 0);
        expect_eq(s_where.tags, 1);

        const s_is = computeSpecificity("section:is(.a, #hero)");
        expect_eq(s_is.ids, 1);
        expect_eq(s_is.classes, 0);
        expect_eq(s_is.tags, 1);
    });

    test("2. @cap/css-select — Specificity comparison & monotonic toScore()", () => {
        const low = computeSpecificity("div p span");
        const mid = computeSpecificity(".card.active");
        const high = computeSpecificity("#main");
        const same = Specificity.new(1, 0, 0);

        expect_eq(low.cmp(mid), -1);
        expect_eq(mid.cmp(high), -1);
        expect_eq(high.cmp(low), 1);
        expect_eq(high.cmp(same), 0);

        expect_eq(low.toScore() < mid.toScore(), true);
        expect_eq(mid.toScore() < high.toScore(), true);
        expect_eq(high.toScore(), same.toScore());
    });

    test("3. @cap/css-select — Compound selector matching (tag/case-insensitive, *, #id, .class)", () => {
        const el = CssElement.new("DIV", "app", "card active primary", "data-role", "admin");

        expect_eq(matchesCompoundSelector("*", el), true);
        expect_eq(matchesCompoundSelector("div", el), true);
        expect_eq(matchesCompoundSelector("Div", el), true);
        expect_eq(matchesCompoundSelector("span", el), false);
        expect_eq(matchesCompoundSelector("#app", el), true);
        expect_eq(matchesCompoundSelector("#other", el), false);
        expect_eq(matchesCompoundSelector(".card", el), true);
        expect_eq(matchesCompoundSelector(".car", el), false);
        expect_eq(matchesCompoundSelector("div#app.card.active", el), true);
        expect_eq(matchesCompoundSelector("div#app.card.missing", el), false);
    });

    test("4. @cap/css-select — Attribute selectors ([attr], =, ^=, $=, *=, quotes & flags)", () => {
        const el = CssElement.new("a", "link-1", "nav-link", "href", "https://cap.dev/docs/index.html");

        expect_eq(matchesCompoundSelector("[href]", el), true);
        expect_eq(matchesCompoundSelector("[HREF]", el), true);
        expect_eq(matchesCompoundSelector("[target]", el), false);
        expect_eq(matchesCompoundSelector("a[href=\"https://cap.dev/docs/index.html\"]", el), true);
        expect_eq(matchesCompoundSelector("a[href^='https://']", el), true);
        expect_eq(matchesCompoundSelector("a[href^='http://']", el), false);
        expect_eq(matchesCompoundSelector("a[href$='.html']", el), true);
        expect_eq(matchesCompoundSelector("a[href$='.png']", el), false);
        expect_eq(matchesCompoundSelector("a[href*=cap.dev]", el), true);
        expect_eq(matchesCompoundSelector("a[href*=\"CAP.DEV\" i]", el), true);
        expect_eq(matchesCompoundSelector("a[href*=\"\"]", el), false);
    });

    test("5. @cap/css-select — Comma-separated selector list matching", () => {
        const el = CssElement.new("h2", "main", "title hero", "data-tag", "a,b");

        expect_eq(matchesSelectorList("h1, .title, #other", el), true);
        expect_eq(matchesSelectorList("h1, .subtitle, #main", el), true);
        expect_eq(matchesSelectorList("h1, span, .missing", el), false);
        expect_eq(matchesSelectorList("p, [data-tag=\"a,b\"]", el), true);
    });

    test("6. @cap/css-select — countSelectorParts across combinators, brackets, and parens", () => {
        expect_eq(countSelectorParts(""), 0);
        expect_eq(countSelectorParts("   "), 0);
        expect_eq(countSelectorParts("div#app.card"), 1);
        expect_eq(countSelectorParts("ul > li.item + li"), 3);
        expect_eq(countSelectorParts("nav ul > li ~ li a[href=\"a > b\"]"), 5);
        expect_eq(countSelectorParts("div:nth-child(2n + 1) > span"), 2);
    });
}
