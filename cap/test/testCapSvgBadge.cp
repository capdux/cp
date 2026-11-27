// cap/test/testCapSvgBadge.cp
// Comprehensive unit tests for @cap/svg-badge

import { test, expect_eq } from "cap:test";
import {
    BadgeStyle,
    BadgeOptions,
    defaultBadge,
    resolveBadgeColor,
    estimateTextWidth,
    renderBadge,
    makeBadge,
} from "../../packages/badge-maker/src/index.cp";

fn contains(haystack: string, needle: string) -> bool {
    if (needle.len == 0) { return true; }
    if (needle.len > haystack.len) { return false; }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        let ok: bool = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) { return true; }
        i += 1;
    }
    return false;
}

fn main() {
    test("1. @cap/svg-badge — named color resolution", () => {
        expect_eq(resolveBadgeColor("brightgreen"), "#4c1");
        expect_eq(resolveBadgeColor("success"), "#4c1");
        expect_eq(resolveBadgeColor("critical"), "#e05d44");
        expect_eq(resolveBadgeColor("blue"), "#007ec6");
        expect_eq(resolveBadgeColor("#123456"), "#123456");
    });

    test("2. @cap/svg-badge — flat badge rendering", () => {
        let buf: [2048]u8 = undefined;
        const svg = makeBadge("build", "passing", "brightgreen", buf[0..2048]);
        expect_eq(contains(svg, "<svg xmlns=\"http://www.w3.org/2000/svg\""), true);
        expect_eq(contains(svg, "aria-label=\"build: passing\""), true);
        expect_eq(contains(svg, "fill=\"#4c1\""), true);
        expect_eq(contains(svg, "rx=\"3\""), true);
        expect_eq(contains(svg, "</svg>"), true);
    });

    test("3. @cap/svg-badge — flat-square & plastic styles", () => {
        let buf: [2048]u8 = undefined;
        const sq = renderBadge(BadgeOptions {
            label: "capdux",
            message: "v1.0.0",
            labelColor: "#333",
            color: "blue",
            style: BadgeStyle.FlatSquare,
        }, buf[0..2048]);
        expect_eq(contains(sq, "shape-rendering=\"crispEdges\""), true);
        expect_eq(contains(sq, "clipPath"), false);

        let buf2: [2048]u8 = undefined;
        const pl = renderBadge(BadgeOptions {
            label: "license",
            message: "MIT",
            labelColor: "grey",
            color: "green",
            style: BadgeStyle.Plastic,
        }, buf2[0..2048]);
        expect_eq(contains(pl, "rx=\"4\""), true);
        expect_eq(contains(pl, "stop-opacity=\".7\""), true);
    });

    test("4. @cap/svg-badge — XML escaping prevents XSS injection", () => {
        let buf: [2048]u8 = undefined;
        const svg = makeBadge("a<b&c", "\"x\">", "red", buf[0..2048]);
        expect_eq(contains(svg, "a&lt;b&amp;c"), true);
        expect_eq(contains(svg, "&quot;x&quot;&gt;"), true);
        expect_eq(contains(svg, "<b"), false);
    });

    test("5. @cap/svg-badge — message-only badge without label", () => {
        let buf: [2048]u8 = undefined;
        const svg = makeBadge("", "MIT", "informational", buf[0..2048]);
        expect_eq(contains(svg, "aria-label=\"MIT\""), true);
        expect_eq(contains(svg, "<rect x=\"0\""), true);
    });

    test("6. @cap/svg-badge — text width estimation", () => {
        const narrow = estimateTextWidth("iiii");
        const wide = estimateTextWidth("WWWW");
        expect_eq(wide > narrow, true);
    });
}
