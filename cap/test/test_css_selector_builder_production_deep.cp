import { test, expect } from "cap:test";
import {
    Combinator,
    ComponentKind,
    SelectorComponent,
    Specificity,
    SelectorBuilder,
} from "cap:css_selector_builder";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("css_selector_builder: buildMatchingOrder, specificity, and source toCss", () => {
    let buf: [128]u8 = [0; 128];
    let sb = SelectorBuilder.init();

    // Build: div#main > ul.nav li:hover::after
    expect(sb.pushSimple(SelectorComponent.typeTag("div")));
    expect(sb.pushSimple(SelectorComponent.id("main")));
    expect(sb.pushCombinator(Combinator.child));
    expect(sb.pushSimple(SelectorComponent.typeTag("ul")));
    expect(sb.pushSimple(SelectorComponent.className("nav")));
    expect(sb.pushCombinator(Combinator.descendant));
    expect(sb.pushSimple(SelectorComponent.typeTag("li")));
    expect(sb.pushSimple(SelectorComponent.pseudoClass("hover")));
    expect(sb.pushSimple(SelectorComponent.pseudoElement("after")));

    expect(sb.hasCombinators());
    expect(strEq(sb.toCss(buf[0..]), "div#main > ul.nav li:hover::after"));

    const built = sb.buildMatchingOrder();
    // Specificity: 1 id (#main), 2 classes (.nav, :hover), 4 types (div, ul, li, ::after)
    expect(built.specificity.ids == 1);
    expect(built.specificity.classes == 2);
    expect(built.specificity.types == 4);
    expect(built.has_pseudo_element);
    expect(!built.has_nesting);

    // Matching order: [li, :hover, ::after, ' ', ul, .nav, '>', div, #main]
    expect(built.len == 9);
    expect(built.components[0].kind == ComponentKind.type_tag && strEq(built.components[0].text, "li"));
    expect(built.components[3].kind == ComponentKind.combinator && built.components[3].combinator == Combinator.descendant);
    expect(built.components[4].kind == ComponentKind.type_tag && strEq(built.components[4].text, "ul"));
    expect(built.components[6].kind == ComponentKind.combinator && built.components[6].combinator == Combinator.child);
    expect(built.components[7].kind == ComponentKind.type_tag && strEq(built.components[7].text, "div"));
    expect(built.components[8].kind == ComponentKind.id && strEq(built.components[8].text, "main"));
});

test("css_selector_builder: addNestingPrefix and specificity comparison", () => {
    let buf: [64]u8 = [0; 64];
    let sb = SelectorBuilder.init();
    expect(sb.pushSimple(SelectorComponent.className("card")));
    expect(sb.pushSimple(SelectorComponent.attribute("data-active")));
    expect(sb.addNestingPrefix());

    expect(strEq(sb.toCss(buf[0..]), "& .card[data-active]"));
    const built = sb.buildMatchingOrder();
    expect(built.has_nesting);
    expect(built.specificity.eql(Specificity{ ids: 0, classes: 2, types: 0 }));

    const higher = Specificity{ ids: 1, classes: 0, types: 0 };
    expect(higher.gt(built.specificity));
    expect(higher.toPacked() > built.specificity.toPacked());
});
