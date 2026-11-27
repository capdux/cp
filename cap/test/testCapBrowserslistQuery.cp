import { test, expect_eq } from "cap:test";
import {
    BrowserOp,
    BrowserClause,
    parseBrowserClause,
    matchBrowserVersion,
} from "../../packages/browserslist/src/index.cp";

fn main() {
    test("1. @cap/browserslist-query — parses and matches Chrome >= 120", () => {
        const c = parseBrowserClause("Chrome >= 120");
        expect_eq(c.negated, false);
        expect_eq(c.is_and, false);
        expect_eq(c.browser, "Chrome");
        expect_eq(c.op == BrowserOp.Ge, true);
        expect_eq(c.version_major, 120 as u32);

        expect_eq(matchBrowserVersion(c, "chrome", 120, 0, 125), true);
        expect_eq(matchBrowserVersion(c, "Chrome", 122, 1, 125), true);
        expect_eq(matchBrowserVersion(c, "Chrome", 119, 9, 125), false);
        expect_eq(matchBrowserVersion(c, "Firefox", 125, 0, 125), false);
    });

    test("2. @cap/browserslist-query — parses and matches minor versions (node >= 20.10)", () => {
        const c = parseBrowserClause("node >= 20.10");
        expect_eq(c.browser, "node");
        expect_eq(c.version_major, 20 as u32);
        expect_eq(c.version_minor, 10 as u32);

        expect_eq(matchBrowserVersion(c, "Node", 20, 10, 22), true);
        expect_eq(matchBrowserVersion(c, "node", 20, 11, 22), true);
        expect_eq(matchBrowserVersion(c, "node", 21, 0, 22), true);
        expect_eq(matchBrowserVersion(c, "node", 20, 9, 22), false);
    });

    test("3. @cap/browserslist-query — parses and matches negated clause (not IE <= 11)", () => {
        const c = parseBrowserClause("not IE <= 11");
        expect_eq(c.negated, true);
        expect_eq(c.browser, "IE");
        expect_eq(c.op == BrowserOp.Le, true);
        expect_eq(c.version_major, 11 as u32);

        expect_eq(matchBrowserVersion(c, "ie", 11, 0, 11), false);
        expect_eq(matchBrowserVersion(c, "ie", 9, 0, 11), false);
        expect_eq(matchBrowserVersion(c, "Chrome", 120, 0, 125), true);
    });

    test("4. @cap/browserslist-query — parses and matches last N browser versions", () => {
        const c = parseBrowserClause("last 2 Firefox versions");
        expect_eq(c.op == BrowserOp.LastN, true);
        expect_eq(c.count, 2 as u32);
        expect_eq(c.browser, "Firefox");

        expect_eq(matchBrowserVersion(c, "firefox", 130, 0, 130), true);
        expect_eq(matchBrowserVersion(c, "firefox", 129, 0, 130), true);
        expect_eq(matchBrowserVersion(c, "firefox", 128, 0, 130), false);
        expect_eq(matchBrowserVersion(c, "chrome", 130, 0, 130), false);
    });

    test("5. @cap/browserslist-query — handles defaults and not dead clauses", () => {
        const d = parseBrowserClause("defaults");
        expect_eq(d.op == BrowserOp.Defaults, true);
        expect_eq(matchBrowserVersion(d, "Chrome", 124, 0, 125), true);
        expect_eq(matchBrowserVersion(d, "Chrome", 120, 0, 125), false);
        expect_eq(matchBrowserVersion(d, "IE", 11, 0, 11), false);

        const nd = parseBrowserClause("and not dead");
        expect_eq(nd.is_and, true);
        expect_eq(nd.negated, true);
        expect_eq(nd.op == BrowserOp.Dead, true);
        expect_eq(matchBrowserVersion(nd, "IE", 11, 0, 11), false);
        expect_eq(matchBrowserVersion(nd, "Chrome", 125, 0, 125), true);
    });

    test("6. @cap/browserslist-query — handles Gt, Lt, Eq and unknown clauses", () => {
        const gt = parseBrowserClause("Safari > 16");
        expect_eq(matchBrowserVersion(gt, "safari", 16, 0, 17), false);
        expect_eq(matchBrowserVersion(gt, "safari", 17, 0, 17), true);

        const eq = parseBrowserClause("Edge = 120");
        expect_eq(matchBrowserVersion(eq, "edge", 120, 0, 125), true);
        expect_eq(matchBrowserVersion(eq, "edge", 121, 0, 125), false);

        const unk = parseBrowserClause("invalid query");
        expect_eq(unk.op == BrowserOp.Unknown, true);
        expect_eq(matchBrowserVersion(unk, "Chrome", 120, 0, 125), false);
    });
}
