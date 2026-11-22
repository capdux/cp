// Source: cp/cap/test/test_sitemap_production_deep.cp
// Deep Production Test Suite for cap:sitemap
// Covers:
//   1. RFC 9309 RobotsRules: exact User-agent vs * fallback, longest-prefix Allow/Disallow,
//      equal-length Allow precedence, * and $ wildcards, Crawl-delay & Sitemap extraction
//   2. SitemapCodec.parseUrlSet & parseIndex with real XML <urlset> and <sitemapindex>
//   3. buildUrlSet / buildIndex round-trip, & entity escaping & invalid priority > 1.0 rejection
//   4. std.testing.allocator zero-leak verification & malformed XML rejection
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.sitemap`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SitemapError,
    ChangeFreq,
    SitemapKind,
    SitemapUrl,
    SitemapIndexItem,
    RobotsRules,
    SitemapCodec,
} from "cap:web";
import cap from "cap";

fn testRobotsRfc9309Matching()! {
    const alloc = std.testing.allocator;
    const robots_txt =
        "\xEF\xBB\xBF# RFC 9309 sample robots.txt\n" ++
        "Sitemap: https://cp-lang.org/sitemap.xml\n" ++
        "\n" ++
        "User-agent: Googlebot\n" ++
        "Disallow: /private/\n" ++
        "Allow: /private/public/\n" ++
        "Crawl-delay: 0.5\n" ++
        "\n" ++
        "User-agent: *\n" ++
        "Disallow: /admin\n" ++
        "Allow: /admin/docs/\n" ++
        "Disallow: /*.json$\n" ++
        "Allow: /api/v1/schema.json$\n" ++
        "Disallow: /exact\n" ++
        "Allow: /exact\n" ++
        "Crawl-delay: 2\n" ++
        "Sitemap: https://cp-lang.org/sitemap-news.xml\n";

    let rules = RobotsRules.parse(alloc, robots_txt) catch unreachable;
    defer rules.deinit();

    // 1. Exact User-agent match vs * fallback
    expect_eq(rules.isAllowed("Googlebot", "/private/secret"), false);
    expect_eq(rules.isAllowed("googlebot", "/private/public/page.html"), true);
    // Googlebot group does not disallow /admin
    expect_eq(rules.isAllowed("Googlebot", "/admin/panel"), true);
    expect_eq(rules.crawlDelayMs("Googlebot") ?? 0, 500 as u64);

    // 2. Fallback to * for unknown agent
    expect_eq(rules.isAllowed("CPBot", "/admin/panel"), false);
    // Longer Allow overrides shorter Disallow
    expect_eq(rules.isAllowed("CPBot", "/admin/docs/intro"), true);
    // Equal-length Allow beats Disallow
    expect_eq(rules.isAllowed("CPBot", "/exact"), true);
    expect_eq(rules.crawlDelayMs("CPBot") ?? 0, 2000 as u64);

    // 3. Wildcard * and end-anchor $
    expect_eq(rules.isAllowed("CPBot", "/data/users.json"), false);
    expect_eq(rules.isAllowed("CPBot", "/data/users.json?v=1"), true);
    expect_eq(rules.isAllowed("CPBot", "/api/v1/schema.json"), true);

    // 4. Global Sitemap directives
    const sms = rules.sitemaps();
    expect_eq(sms.len, 2 as usize);
    expect_eq(std.mem.eql(u8, sms[0], "https://cp-lang.org/sitemap.xml"), true);
    expect_eq(std.mem.eql(u8, sms[1], "https://cp-lang.org/sitemap-news.xml"), true);
}

fn testParseUrlSetAndIndex()! {
    const alloc = std.testing.allocator;

    const urlset_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
        "<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n" ++
        "  <url>\n" ++
        "    <loc>https://cp-lang.org/</loc>\n" ++
        "    <lastmod>2026-11-20</lastmod>\n" ++
        "    <changefreq>daily</changefreq>\n" ++
        "    <priority>1.0</priority>\n" ++
        "  </url>\n" ++
        "  <url>\n" ++
        "    <loc>https://cp-lang.org/search?a=1&amp;b=2</loc>\n" ++
        "    <changefreq>weekly</changefreq>\n" ++
        "    <priority>0.6</priority>\n" ++
        "  </url>\n" ++
        "</urlset>";

    const kind1 = SitemapCodec.detectKind(urlset_xml) catch unreachable;
    expect_eq(kind1 == .UrlSet, true);

    const urls = SitemapCodec.parseUrlSet(alloc, urlset_xml) catch unreachable;
    defer SitemapCodec.freeUrlSet(alloc, urls);

    expect_eq(urls.len, 2 as usize);
    expect_eq(std.mem.eql(u8, urls[0].loc, "https://cp-lang.org/"), true);
    expect_eq(std.mem.eql(u8, urls[0].lastmod, "2026-11-20"), true);
    expect_eq((urls[0].changefreq ?? .Never) == .Daily, true);
    expect_eq(urls[0].priority ?? -1.0, 1.0);

    expect_eq(std.mem.eql(u8, urls[1].loc, "https://cp-lang.org/search?a=1&b=2"), true);
    expect_eq((urls[1].changefreq ?? .Never) == .Weekly, true);

    const index_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
        "<sitemapindex xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n" ++
        "  <sitemap>\n" ++
        "    <loc>https://cp-lang.org/sitemap-docs.xml</loc>\n" ++
        "    <lastmod>2026-11-20T10:00:00Z</lastmod>\n" ++
        "  </sitemap>\n" ++
        "  <sitemap>\n" ++
        "    <loc>https://cp-lang.org/sitemap-blog.xml</loc>\n" ++
        "  </sitemap>\n" ++
        "</sitemapindex>";

    const kind2 = SitemapCodec.detectKind(index_xml) catch unreachable;
    expect_eq(kind2 == .Index, true);

    const items = SitemapCodec.parseIndex(alloc, index_xml) catch unreachable;
    defer SitemapCodec.freeIndex(alloc, items);

    expect_eq(items.len, 2 as usize);
    expect_eq(std.mem.eql(u8, items[0].loc, "https://cp-lang.org/sitemap-docs.xml"), true);
    expect_eq(std.mem.eql(u8, items[0].lastmod, "2026-11-20T10:00:00Z"), true);
    expect_eq(std.mem.eql(u8, items[1].loc, "https://cp-lang.org/sitemap-blog.xml"), true);
    expect_eq(items[1].lastmod.len, 0 as usize);
}

fn testBuildRoundTripEscapingAndPriorityGuard()! {
    const alloc = std.testing.allocator;

    const input_urls = [
        SitemapUrl{
            loc: "https://cp-lang.org/docs?q=a&page=2",
            lastmod: "2026-11-20",
            changefreq: .Hourly,
            priority: 0.75,
        },
        SitemapUrl{
            loc: "https://cp-lang.org/about",
            lastmod: "",
            changefreq: .Monthly,
            priority: 0.5,
        },
    ];

    const built_xml = SitemapCodec.buildUrlSet(alloc, &input_urls) catch unreachable;
    defer alloc.free(built_xml);

    // Ensure '&' was safely escaped to '&amp;'
    expect_eq(std.mem.indexOf(u8, built_xml, "&amp;") != null, true);

    const parsed = SitemapCodec.parseUrlSet(alloc, built_xml) catch unreachable;
    defer SitemapCodec.freeUrlSet(alloc, parsed);

    expect_eq(parsed.len, 2 as usize);
    expect_eq(std.mem.eql(u8, parsed[0].loc, "https://cp-lang.org/docs?q=a&page=2"), true);
    expect_eq((parsed[0].changefreq ?? .Never) == .Hourly, true);
    expect_eq(parsed[0].priority ?? -1.0, 0.75);
    expect_eq(std.mem.eql(u8, parsed[1].loc, "https://cp-lang.org/about"), true);

    // buildIndex round-trip
    const idx_items = [
        SitemapIndexItem{ loc: "https://cp-lang.org/s1.xml?t=1&v=2", lastmod: "2026-11-20" },
    ];
    const built_idx = SitemapCodec.buildIndex(alloc, &idx_items) catch unreachable;
    defer alloc.free(built_idx);

    const parsed_idx = SitemapCodec.parseIndex(alloc, built_idx) catch unreachable;
    defer SitemapCodec.freeIndex(alloc, parsed_idx);
    expect_eq(parsed_idx.len, 1 as usize);
    expect_eq(std.mem.eql(u8, parsed_idx[0].loc, "https://cp-lang.org/s1.xml?t=1&v=2"), true);

    // Invalid priority > 1.0 rejection
    const bad_urls = [
        SitemapUrl{ loc: "https://cp-lang.org/bad", priority: 1.5 },
    ];
    let prio_err = false;
    * = SitemapCodec.buildUrlSet(alloc, &bad_urls) catch e => {
        if (e == SitemapError.InvalidPriority) {
            prio_err = true;
        }
    };
    expect_eq(prio_err, true);
}

fn testZeroLeakAndMalformedXmlRejection()! {
    const alloc = std.testing.allocator;

    // 1. Malformed XML
    let xml_err = false;
    * = SitemapCodec.parseUrlSet(alloc, "<urlset><url><loc>https://a.com</url></urlset>") catch e => {
        if (e == SitemapError.InvalidXml) {
            xml_err = true;
        }
    };
    expect_eq(xml_err, true);

    // 2. Wrong root tag
    let root_err = false;
    * = SitemapCodec.parseUrlSet(alloc, "<rss><channel></channel></rss>") catch e => {
        if (e == SitemapError.InvalidSitemap) {
            root_err = true;
        }
    };
    expect_eq(root_err, true);

    // 3. Invalid priority inside XML
    let bad_p_xml = false;
    * = SitemapCodec.parseUrlSet(alloc, "<urlset><url><loc>https://a.com</loc><priority>2.0</priority></url></urlset>") catch e => {
        if (e == SitemapError.InvalidPriority) {
            bad_p_xml = true;
        }
    };
    expect_eq(bad_p_xml, true);

    // 4. Clean parse & free under std.testing.allocator
    let r = RobotsRules.parse(alloc, "User-agent: *\nDisallow: /tmp/\nSitemap: https://a.com/s.xml\n") catch unreachable;
    defer r.deinit();
    expect_eq(r.isAllowed("Bot", "/tmp/x"), false);
}

fn testDynamicCapSitemapMount()! {
    const alloc = std.testing.allocator;
    const xml = "<urlset><url><loc>https://cp-lang.org/dynamic</loc></url></urlset>";

    const kind = cap.web.SitemapCodec.detectKind(xml) catch unreachable;
    expect_eq(kind == cap.web.SitemapKind.UrlSet, true);

    const urls = cap.web.SitemapCodec.parseUrlSet(alloc, xml) catch unreachable;
    defer cap.web.SitemapCodec.freeUrlSet(alloc, urls);

    expect_eq(urls.len, 1 as usize);
    expect_eq(std.mem.eql(u8, urls[0].loc, "https://cp-lang.org/dynamic"), true);
}

test("sitemap_robots_rfc9309_matching", testRobotsRfc9309Matching);
test("sitemap_parse_urlset_and_index", testParseUrlSetAndIndex);
test("sitemap_build_roundtrip_escaping_and_priority", testBuildRoundTripEscapingAndPriorityGuard);
test("sitemap_zero_leak_and_malformed_xml", testZeroLeakAndMalformedXmlRejection);
test("sitemap_dynamic_cap_mount", testDynamicCapSitemapMount);
