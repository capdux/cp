import { test, expect_eq } from "cap:test";
import {
    SitemapWriter,
    SitemapIndexWriter,
    SitemapUrl,
    FREQ_NONE,
    FREQ_DAILY,
    FREQ_WEEKLY,
    isValidPriorityTenths,
} from "../../packages/sitemap/src/index.cp";

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        let j: usize = 0;
        let ok: bool = true;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn main() {
    test("basic sitemap urlset with all optional fields", () => {
        let buf: [2048]u8 = undefined;
        let w = SitemapWriter.init(buf[0..2048]);
        expect_eq(w.addUrl(SitemapUrl {
            loc: "https://capdux.org/",
            lastmod: "2026-11-26",
            changefreq: FREQ_DAILY,
            priority_tenths: 10,
        }), true);
        expect_eq(w.addUrl(SitemapUrl {
            loc: "https://capdux.org/packages",
            lastmod: "",
            changefreq: FREQ_WEEKLY,
            priority_tenths: 8,
        }), true);
        let n = w.finish();
        expect_eq(n > 0, true);
        expect_eq(w.urlCount(), 2);
        let xml = w.written();
        expect_eq(containsSubstr(xml, "<loc>https://capdux.org/</loc>"), true);
        expect_eq(containsSubstr(xml, "<lastmod>2026-11-26</lastmod>"), true);
        expect_eq(containsSubstr(xml, "<changefreq>daily</changefreq>"), true);
        expect_eq(containsSubstr(xml, "<priority>1.0</priority>"), true);
        expect_eq(containsSubstr(xml, "<priority>0.8</priority>"), true);
        expect_eq(containsSubstr(xml, "</urlset>"), true);
    });

    test("XML entity escaping protects ampersands and angle brackets", () => {
        let buf: [1024]u8 = undefined;
        let w = SitemapWriter.init(buf[0..1024]);
        expect_eq(w.addUrl(SitemapUrl {
            loc: "https://example.com/search?a=1&b=2<x>",
            lastmod: "",
            changefreq: FREQ_NONE,
            priority_tenths: -1,
        }), true);
        let n = w.finish();
        expect_eq(n > 0, true);
        let xml = w.written();
        expect_eq(containsSubstr(xml, "&amp;b=2&lt;x&gt;"), true);
        expect_eq(containsSubstr(xml, "<priority>"), false);
        expect_eq(containsSubstr(xml, "<changefreq>"), false);
    });

    test("sitemapindex generation with multiple sitemaps", () => {
        let buf: [1024]u8 = undefined;
        let idx = SitemapIndexWriter.init(buf[0..1024]);
        expect_eq(idx.addSitemap("https://capdux.org/sitemap-1.xml", "2026-11-26"), true);
        expect_eq(idx.addSitemap("https://capdux.org/sitemap-2.xml", ""), true);
        let n = idx.finish();
        expect_eq(n > 0, true);
        expect_eq(idx.entryCount(), 2);
        let xml = idx.written();
        expect_eq(containsSubstr(xml, "<sitemapindex "), true);
        expect_eq(containsSubstr(xml, "<loc>https://capdux.org/sitemap-1.xml</loc>"), true);
        expect_eq(containsSubstr(xml, "</sitemapindex>"), true);
    });

    test("empty loc and post-finish writes are rejected", () => {
        let buf: [512]u8 = undefined;
        let w = SitemapWriter.init(buf[0..512]);
        expect_eq(w.addUrl(SitemapUrl {
            loc: "",
            lastmod: "",
            changefreq: FREQ_NONE,
            priority_tenths: -1,
        }), false);
        expect_eq(w.finish() > 0, true);
        expect_eq(w.addUrl(SitemapUrl {
            loc: "https://example.com/after",
            lastmod: "",
            changefreq: FREQ_NONE,
            priority_tenths: 5,
        }), false);
    });

    test("undersized buffer triggers clean overflow error -1", () => {
        let tiny: [32]u8 = undefined;
        let w = SitemapWriter.init(tiny[0..32]);
        expect_eq(w.addUrl(SitemapUrl {
            loc: "https://example.com/",
            lastmod: "",
            changefreq: FREQ_NONE,
            priority_tenths: 5,
        }), false);
        expect_eq(w.finish(), -1);
    });

    test("priority validation helper", () => {
        expect_eq(isValidPriorityTenths(0), true);
        expect_eq(isValidPriorityTenths(5), true);
        expect_eq(isValidPriorityTenths(10), true);
        expect_eq(isValidPriorityTenths(-1), false);
        expect_eq(isValidPriorityTenths(11), false);
    });
}
