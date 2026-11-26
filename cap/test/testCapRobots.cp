// cap/test/testCapRobots.cp
// Comprehensive unit tests for @cap/robots

import { test, expect_eq } from "cap:test";
import {
    isAllowed,
    crawlDelay,
    firstSitemap,
} from "../../packages/@cap/robots/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/robots — Basic User-agent * Disallow /admin/", () => {
        const txt = "User-agent: *\nDisallow: /admin/\n";
        expect_eq(isAllowed(txt, "AnyBot", "/admin/"), false);
        expect_eq(isAllowed(txt, "AnyBot", "/admin/settings"), false);
        expect_eq(isAllowed(txt, "AnyBot", "/admin"), true);
        expect_eq(isAllowed(txt, "AnyBot", "/public/index.html"), true);
    });

    test("2. @cap/robots — Longer Allow overrides shorter Disallow & tie favors Allow", () => {
        const txt = "User-agent: *\nDisallow: /admin\nAllow: /admin/public\nDisallow: /exact\nAllow: /exact\n";
        expect_eq(isAllowed(txt, "Bot", "/admin/secret"), false);
        expect_eq(isAllowed(txt, "Bot", "/admin/public"), true);
        expect_eq(isAllowed(txt, "Bot", "/admin/public/doc.html"), true);
        // Same-length tie: Allow wins over Disallow
        expect_eq(isAllowed(txt, "Bot", "/exact"), true);
    });

    test("3. @cap/robots — Specific User-agent CapBot overrides * group", () => {
        const txt = "User-agent: *\nDisallow: /\n\nUser-agent: CapBot\nDisallow: /private/\nAllow: /\n";
        expect_eq(isAllowed(txt, "OtherBot/1.0", "/"), false);
        expect_eq(isAllowed(txt, "OtherBot/1.0", "/articles"), false);
        expect_eq(isAllowed(txt, "capbot/2.1", "/articles"), true);
        expect_eq(isAllowed(txt, "CapBot", "/private/data"), false);
    });

    test("4. @cap/robots — Wildcard * and end-of-path anchor $", () => {
        const txt = "User-agent: *\nDisallow: /*.pdf$\nDisallow: /api/*/secret\n";
        expect_eq(isAllowed(txt, "Bot", "/docs/report.pdf"), false);
        expect_eq(isAllowed(txt, "Bot", "/docs/report.pdf?download=1"), true);
        expect_eq(isAllowed(txt, "Bot", "/docs/report.pdf.html"), true);
        expect_eq(isAllowed(txt, "Bot", "/api/v1/secret/key"), false);
        expect_eq(isAllowed(txt, "Bot", "/api/v1/public"), true);
    });

    test("5. @cap/robots — Crawl-delay, Sitemap & '#' comment stripping", () => {
        const txt = "# Global rules\nUser-agent: *\nCrawl-delay: 10 # default delay\n\nUser-agent: CapBot\nCrawl-delay: 2\nSitemap: https://example.com/sitemap.xml # primary\nSitemap: https://example.com/sitemap2.xml\n";
        expect_eq(crawlDelay(txt, "OtherBot"), 10 as i64);
        expect_eq(crawlDelay(txt, "CapBot/1.0"), 2 as i64);
        expect_eq(sliceEq(firstSitemap(txt), "https://example.com/sitemap.xml"), true);
        expect_eq(crawlDelay("User-agent: *\nDisallow: /x\n", "Bot"), -1 as i64);
        expect_eq(firstSitemap("User-agent: *\nDisallow: /x\n").len, 0 as usize);
    });

    test("6. @cap/robots — Empty robots.txt & empty Disallow allow all paths", () => {
        expect_eq(isAllowed("", "CapBot", "/anything"), true);
        const allowAll = "User-agent: *\nDisallow:\n";
        expect_eq(isAllowed(allowAll, "CapBot", "/"), true);
        expect_eq(isAllowed(allowAll, "CapBot", "/admin/panel"), true);
    });
}
