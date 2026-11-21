import { test, expect } from "cap:test";
import {
    normalizeLabel,
    labelsEqual,
    parseRefDefDest,
    parseRefDefTitle,
    parseRefDef,
    RefDefTable,
} from "cap:md_ref_defs";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("md_ref_defs: label normalization and zero-alloc equality", () => {
    let buf: [64]u8 = [0; 64];
    const norm = normalizeLabel("  Foo \t\n  BAR  ", buf[0..]);
    expect(norm != null);
    expect(strEq(norm.?, "foo bar"));

    expect(labelsEqual("  Foo\nBar ", "foo   bar"));
    expect(!labelsEqual("foo bar", "foobar"));
});

test("md_ref_defs: destination and title parsers", () => {
    const d1 = parseRefDefDest("<https://example.com/a b>", 0);
    expect(d1 != null);
    expect(strEq(d1.?.dest, "https://example.com/a b"));

    const d2 = parseRefDefDest("https://en.wikipedia.org/wiki/CP_(lang) rest", 0);
    expect(d2 != null);
    expect(strEq(d2.?.dest, "https://en.wikipedia.org/wiki/CP_(lang)"));

    // Unbalanced paren rejected
    expect(parseRefDefDest("https://example.com/(unclosed", 0) == null);

    const t1 = parseRefDefTitle("\"Hello \\\"World\\\"\"", 0);
    expect(t1 != null);
    expect(strEq(t1.?.title, "Hello \\\"World\\\""));

    const t2 = parseRefDefTitle("(Paren Title)", 0);
    expect(t2 != null);
    expect(strEq(t2.?.title, "Paren Title"));

    // Nested '(' inside () title rejected
    expect(parseRefDefTitle("(bad (nested))", 0) == null);
    // Blank line inside title rejected
    expect(parseRefDefTitle("\"line1\n\nline2\"", 0) == null);
});

test("md_ref_defs: parseRefDef and RefDefTable first-definition-wins", () => {
    const doc =
        "[Foo Bar]: https://first.example.com \"First Title\"\n" ++
        "[foo   bar]: https://second.example.com 'Ignored'\n" ++
        "  [Angles]:\n    <https://angles.test/a b>\n" ++
        "    (Next Line Title)\n" ++
        "Paragraph starts here.\n";

    let table = RefDefTable.init();
    const consumed = table.parseAll(doc);
    expect(table.len == 2);
    expect(strEq(doc[consumed..], "Paragraph starts here.\n"));

    const found1 = table.lookup("FOO\tbar");
    expect(found1 != null);
    expect(strEq(found1.?.dest, "https://first.example.com"));
    expect(strEq(found1.?.title, "First Title"));

    const found2 = table.lookup("angles");
    expect(found2 != null);
    expect(strEq(found2.?.dest, "https://angles.test/a b"));
    expect(strEq(found2.?.title, "Next Line Title"));

    // Same-line trailing junk after title invalid
    expect(parseRefDef("[a]: /url \"title\" junk\n", 0) == null);
    // Empty/whitespace label invalid
    expect(parseRefDef("[   ]: /url\n", 0) == null);
});
