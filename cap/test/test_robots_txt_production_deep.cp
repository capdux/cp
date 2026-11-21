// Source: cp/cap/test/test_robots_txt_production_deep.cp (Production deep tests for cap:robots_txt)

import { test, expect_eq } from "cap:test";
import {
    matchRobotsPattern,
    isAllowed,
    crawlDelaySec,
    SitemapIter,
} from "cap:robots_txt";

test("robots_txt: matchRobotsPattern wildcards, $ end anchor and specificity length") {
    expect_eq(matchRobotsPattern("/admin", "/admin/settings").?, 6);
    expect_eq(matchRobotsPattern("/admin$", "/admin").?, 7);
    expect_eq(matchRobotsPattern("/admin$", "/admin/settings") == null, true);

    expect_eq(matchRobotsPattern("/*.pdf$", "/docs/report.pdf").?, 7);
    expect_eq(matchRobotsPattern("/*.pdf$", "/docs/report.pdf?v=1") == null, true);
    expect_eq(matchRobotsPattern("/a*b*c$", "/a111b222b333c").?, 7);
    expect_eq(matchRobotsPattern("/*", "/anything").?, 2);
    expect_eq(matchRobotsPattern("/$", "/").?, 2);
    expect_eq(matchRobotsPattern("/$", "/index.html") == null, true);
    expect_eq(matchRobotsPattern("", "/anything") == null, true);
}

test("robots_txt: RFC 9309 most-specific User-agent selection and wildcard fallback") {
    const txt =
        "\xEF\xBB\xBF" ++
        "# global rules\n" ++
        "User-agent: *\n" ++
        "Disallow: /private/\n" ++
        "Allow: /private/docs\n" ++
        "\n" ++
        "User-agent: Googlebot\n" ++
        "Disallow: /no-google/\n" ++
        "\n" ++
        "User-agent: Googlebot-Image\n" ++
        "Disallow: /\n" ++
        "Allow: /images/\n";

    // 普通爬虫命中 * 组
    expect_eq(isAllowed(txt, "CapBot/1.0", "/public"), true);
    expect_eq(isAllowed(txt, "CapBot/1.0", "/private/secret"), false);
    expect_eq(isAllowed(txt, "CapBot/1.0", "/private/docs/intro"), true);

    // Googlebot 命中 Googlebot 组（覆盖 * 组，因此 /private/secret 对其放行，/no-google/ 禁止）
    expect_eq(isAllowed(txt, "googlebot/2.1", "/private/secret"), true);
    expect_eq(isAllowed(txt, "googlebot/2.1", "/no-google/page"), false);

    // Googlebot-Image 比 Googlebot 更具体
    expect_eq(isAllowed(txt, "Googlebot-Image/1.0", "/index.html"), false);
    expect_eq(isAllowed(txt, "Googlebot-Image/1.0", "/images/logo.png"), true);
}

test("robots_txt: equal-length Allow beats Disallow and empty Disallow allows all") {
    const txt =
        "User-agent: *\n" ++
        "Disallow: /page\n" ++
        "Allow: /page\n" ++
        "\n" ++
        "User-agent: OpenBot\n" ++
        "Disallow:\n";

    // 同长度冲突 Allow 胜出
    expect_eq(isAllowed(txt, "AnyBot", "/page"), true);
    expect_eq(isAllowed(txt, "AnyBot", "/page/sub"), true);

    // 空 Disallow: 表示全部允许
    expect_eq(isAllowed(txt, "OpenBot", "/anything/at/all"), true);
}

test("robots_txt: crawlDelaySec per-group extraction and invalid value guard") {
    const txt =
        "User-agent: *\r\n" ++
        "Crawl-delay: 10 # default delay\r\n" ++
        "\r\n" ++
        "User-agent: FastBot\r\n" ++
        "Crawl-delay: 2\r\n" ++
        "\r\n" ++
        "User-agent: BadDelayBot\r\n" ++
        "Crawl-delay: abc\r\n";

    expect_eq(crawlDelaySec(txt, "UnknownBot").?, 10);
    expect_eq(crawlDelaySec(txt, "FastBot/1.0").?, 2);
    expect_eq(crawlDelaySec(txt, "BadDelayBot") == null, true);
    expect_eq(crawlDelaySec("", "AnyBot") == null, true);
}

test("robots_txt: SitemapIter extracts all Sitemap URLs with comments and CRLF") {
    const txt =
        "\xEF\xBB\xBF" ++
        "User-agent: *\r\n" ++
        "Disallow: /tmp/\r\n" ++
        "Sitemap: https://example.com/sitemap-main.xml # primary\r\n" ++
        "sitemap: https://example.com/sitemap-news.xml\n" ++
        "Sitemap:   \n";

    let it = SitemapIter.init(txt);

    const s1 = it.next().?;
    expect_eq(std.mem.eql(u8, s1, "https://example.com/sitemap-main.xml"), true);

    const s2 = it.next().?;
    expect_eq(std.mem.eql(u8, s2, "https://example.com/sitemap-news.xml"), true);

    expect_eq(it.next() == null, true);
}
