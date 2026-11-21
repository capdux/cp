import { test, expect } from "cap:test";
import {
    AutolinkKind,
    AutolinkSpan,
    findPermissiveAutolink,
    findNextAutolink,
    checkLeftBoundary,
    checkRightBoundary,
    isListBullet,
    isListItemMark,
} from "cap:autolink";

test("autolink: URL autolink detection with path, query, fragment", () => {
    const s = "See https://example.com/docs/api?v=2&ok=1#top for details.";
    const m = findNextAutolink(s, 0, false).?;
    expect(m.kind == .url).toBeTrue();
    expect(m.slice(s)).toBe("https://example.com/docs/api?v=2&ok=1#top");
});

test("autolink: GFM trailing unbalanced paren and HTML entity trimming", () => {
    const s1 = "(visit https://en.wikipedia.org/wiki/Foo_(bar))";
    const m1 = findNextAutolink(s1, 0, false).?;
    expect(m1.slice(s1)).toBe("https://en.wikipedia.org/wiki/Foo_(bar)");

    const s2 = "Check https://example.com/commonmark&hl; now";
    const m2 = findNextAutolink(s2, 0, false).?;
    expect(m2.slice(s2)).toBe("https://example.com/commonmark");
});

test("autolink: www and email detection + sequential scanning", () => {
    const s = "Site www.bun.sh/docs or mail dev.team+cp@example.org!";
    const m1 = findNextAutolink(s, 0, false).?;
    expect(m1.kind == .www).toBeTrue();
    expect(m1.slice(s)).toBe("www.bun.sh/docs");

    const m2 = findNextAutolink(s, m1.end, false).?;
    expect(m2.kind == .email).toBeTrue();
    expect(m2.slice(s)).toBe("dev.team+cp@example.org");

    const m3 = findNextAutolink(s, m2.end, false);
    expect(m3 == null).toBeTrue();
});

test("autolink: emphasis boundaries and negative/invalid cases", () => {
    const emph = "*https://cp-lang.org*";
    expect(findNextAutolink(emph, 0, false) == null).toBeTrue();
    const m = findNextAutolink(emph, 0, true).?;
    expect(m.slice(emph)).toBe("https://cp-lang.org");

    expect(findNextAutolink("not a link http://localhost only", 0, false) == null).toBeTrue();
    expect(findNextAutolink("bad @example.com email", 0, false) == null).toBeTrue();
    expect(isListBullet('-')).toBeTrue();
    expect(isListBullet('.')).toBeFalse();
    expect(isListItemMark(')')).toBeTrue();
    expect(checkLeftBoundary(" abc", 1, false)).toBeTrue();
    expect(checkRightBoundary("abc.", 3, false)).toBeTrue();
});
