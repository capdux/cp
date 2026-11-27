// cap/test/testCapUserAgent.cp
// Comprehensive unit tests for @cap/useragent:
// 1. Desktop browsers: Chrome, Edge (Edg/), Opera (OPR/), Firefox, Safari
// 2. Mobile & Tablet: iPhone Safari, Android Chrome, iPad, HarmonyOS / OpenHarmony
// 3. Search Engine Bots: Googlebot, Bingbot
// 4. AI Crawlers: GPTBot, ClaudeBot, PerplexityBot, Bytespider, Google-Extended, CCBot
// 5. CLI & Runtime clients: curl, Bun, Cap
// 6. Empty string, malformed/minimal UA fallback, and ChromeOS / Samsung Internet

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    DeviceKind,
    BrowserKind,
    OsKind,
    UserAgent,
    parseUserAgent,
    isBot,
    isAiBot,
    isMobile,
} from "../../packages/ua-parser-js/src/index.cp";

fn main() {
    test("1. @cap/useragent — Desktop Browsers (Chrome, Edge, Opera, Firefox, Safari)", () => {
        const chrome_ua = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.6367.91 Safari/537.36";
        const c = parseUserAgent(chrome_ua);
        expect_eq(c.browser == BrowserKind.Chrome, true);
        expect_eq(std.mem.eql(u8, c.browser_name, "Chrome"), true);
        expect_eq(std.mem.eql(u8, c.browser_version, "124.0.6367.91"), true);
        expect_eq(c.os == OsKind.Windows, true);
        expect_eq(std.mem.eql(u8, c.os_version, "10.0"), true);
        expect_eq(c.device == DeviceKind.Desktop, true);
        expect_eq(std.mem.eql(u8, c.engine_name, "Blink"), true);
        expect_eq(c.is_bot, false);
        expect_eq(c.is_mobile, false);

        const edge_ua = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36 Edg/124.0.2478.80";
        const e = parseUserAgent(edge_ua);
        expect_eq(e.browser == BrowserKind.Edge, true);
        expect_eq(std.mem.eql(u8, e.browser_version, "124.0.2478.80"), true);
        expect_eq(e.os == OsKind.Windows, true);
        expect_eq(e.device == DeviceKind.Desktop, true);

        const opera_ua = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36 OPR/109.0.0.0";
        const o = parseUserAgent(opera_ua);
        expect_eq(o.browser == BrowserKind.Opera, true);
        expect_eq(std.mem.eql(u8, o.browser_version, "109.0.0.0"), true);
        expect_eq(o.os == OsKind.MacOS, true);
        expect_eq(std.mem.eql(u8, o.os_version, "10_15_7"), true);

        const ff_ua = "Mozilla/5.0 (X11; Linux x86_64; rv:125.0) Gecko/20100101 Firefox/125.0";
        const f = parseUserAgent(ff_ua);
        expect_eq(f.browser == BrowserKind.Firefox, true);
        expect_eq(std.mem.eql(u8, f.browser_version, "125.0"), true);
        expect_eq(f.os == OsKind.Linux, true);
        expect_eq(std.mem.eql(u8, f.engine_name, "Gecko"), true);

        const safari_ua = "Mozilla/5.0 (Macintosh; Intel Mac OS X 14_4_1) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.4.1 Safari/605.1.15";
        const s = parseUserAgent(safari_ua);
        expect_eq(s.browser == BrowserKind.Safari, true);
        expect_eq(std.mem.eql(u8, s.browser_version, "17.4.1"), true);
        expect_eq(s.os == OsKind.MacOS, true);
        expect_eq(std.mem.eql(u8, s.engine_name, "WebKit"), true);
    });

    test("2. @cap/useragent — Mobile & Tablet (iPhone, Android, iPad, HarmonyOS)", () => {
        const iphone_ua = "Mozilla/5.0 (iPhone; CPU iPhone OS 17_4_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.4.1 Mobile/15E148 Safari/604.1";
        const ip = parseUserAgent(iphone_ua);
        expect_eq(ip.browser == BrowserKind.Safari, true);
        expect_eq(ip.os == OsKind.IOS, true);
        expect_eq(std.mem.eql(u8, ip.os_version, "17_4_1"), true);
        expect_eq(ip.device == DeviceKind.Mobile, true);
        expect_eq(ip.is_mobile, true);
        expect_eq(isMobile(iphone_ua), true);

        const android_ua = "Mozilla/5.0 (Linux; Android 14; Pixel 8) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.6367.82 Mobile Safari/537.36";
        const an = parseUserAgent(android_ua);
        expect_eq(an.browser == BrowserKind.Chrome, true);
        expect_eq(an.os == OsKind.Android, true);
        expect_eq(std.mem.eql(u8, an.os_version, "14"), true);
        expect_eq(an.device == DeviceKind.Mobile, true);
        expect_eq(an.is_mobile, true);

        const ipad_ua = "Mozilla/5.0 (iPad; CPU OS 17_4 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.4 Mobile/15E148 Safari/604.1";
        const pd = parseUserAgent(ipad_ua);
        expect_eq(pd.os == OsKind.IOS, true);
        expect_eq(pd.device == DeviceKind.Tablet, true);
        expect_eq(pd.is_mobile, false);

        const harmony_ua = "Mozilla/5.0 (Linux; HarmonyOS 4.0.0; NOH-AN00) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Mobile Safari/537.36";
        const hm = parseUserAgent(harmony_ua);
        expect_eq(hm.os == OsKind.HarmonyOS, true);
        expect_eq(std.mem.eql(u8, hm.os_version, "4.0.0"), true);
        expect_eq(hm.device == DeviceKind.Mobile, true);
        expect_eq(hm.is_mobile, true);
    });

    test("3. @cap/useragent — Search Engine Crawlers (Googlebot, Bingbot)", () => {
        const gbot_ua = "Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)";
        const g = parseUserAgent(gbot_ua);
        expect_eq(g.is_bot, true);
        expect_eq(g.is_ai_bot, false);
        expect_eq(g.browser == BrowserKind.Bot, true);
        expect_eq(g.device == DeviceKind.Bot, true);
        expect_eq(std.mem.eql(u8, g.bot_name, "Googlebot"), true);
        expect_eq(std.mem.eql(u8, g.browser_version, "2.1"), true);
        expect_eq(isBot(gbot_ua), true);
        expect_eq(isAiBot(gbot_ua), false);

        const bing_ua = "Mozilla/5.0 AppleWebKit/537.36 (KHTML, like Gecko; compatible; bingbot/2.0; +http://www.bing.com/bingbot.htm) Chrome/116.0.1938.76 Safari/537.36";
        const b = parseUserAgent(bing_ua);
        expect_eq(b.is_bot, true);
        expect_eq(b.is_ai_bot, false);
        expect_eq(std.mem.eql(u8, b.bot_name, "Bingbot"), true);
        expect_eq(std.mem.eql(u8, b.browser_version, "2.0"), true);
    });

    test("4. @cap/useragent — AI Crawlers (GPTBot, ClaudeBot, PerplexityBot, Bytespider, Google-Extended, CCBot)", () => {
        const gpt_ua = "Mozilla/5.0 AppleWebKit/537.36 (KHTML, like Gecko; compatible; GPTBot/1.2; +https://openai.com/gptbot)";
        const gpt = parseUserAgent(gpt_ua);
        expect_eq(gpt.is_bot, true);
        expect_eq(gpt.is_ai_bot, true);
        expect_eq(gpt.device == DeviceKind.Bot, true);
        expect_eq(std.mem.eql(u8, gpt.bot_name, "GPTBot"), true);
        expect_eq(std.mem.eql(u8, gpt.browser_version, "1.2"), true);
        expect_eq(isAiBot(gpt_ua), true);

        const claude_ua = "Mozilla/5.0 AppleWebKit/537.36 (KHTML, like Gecko; compatible; ClaudeBot/1.0; +claudebot@anthropic.com)";
        const cl = parseUserAgent(claude_ua);
        expect_eq(cl.is_bot, true);
        expect_eq(cl.is_ai_bot, true);
        expect_eq(std.mem.eql(u8, cl.bot_name, "ClaudeBot"), true);

        expect_eq(isAiBot("Mozilla/5.0 AppleWebKit/537.36 (KHTML, like Gecko; compatible; PerplexityBot/1.0)"), true);
        expect_eq(isAiBot("Mozilla/5.0 (Linux; Android 5.0) AppleWebKit/537.36 (KHTML, like Gecko) Mobile Safari/537.36 (compatible; Bytespider)"), true);
        expect_eq(isAiBot("Google-Extended"), true);
        expect_eq(isAiBot("CCBot/2.0 (https://commoncrawl.org/faq/)"), true);
    });

    test("5. @cap/useragent — CLI & Runtime Clients (curl, Bun, Cap)", () => {
        const curl_ua = "curl/8.5.0";
        const cu = parseUserAgent(curl_ua);
        expect_eq(cu.browser == BrowserKind.Curl, true);
        expect_eq(std.mem.eql(u8, cu.browser_version, "8.5.0"), true);
        expect_eq(cu.device == DeviceKind.Cli, true);
        expect_eq(cu.is_bot, false);
        expect_eq(cu.is_mobile, false);

        const bun_ua = "Bun/1.1.8";
        const bn = parseUserAgent(bun_ua);
        expect_eq(bn.browser == BrowserKind.Bun, true);
        expect_eq(std.mem.eql(u8, bn.browser_version, "1.1.8"), true);
        expect_eq(bn.device == DeviceKind.Cli, true);

        const cap_ua = "Cap/1.0.0 (Linux x86_64)";
        const cp = parseUserAgent(cap_ua);
        expect_eq(cp.browser == BrowserKind.Cap, true);
        expect_eq(std.mem.eql(u8, cp.browser_version, "1.0.0"), true);
        expect_eq(cp.device == DeviceKind.Cli, true);
        expect_eq(cp.os == OsKind.Linux, true);
    });

    test("6. @cap/useragent — Empty, Malformed UA Fallback & ChromeOS / Samsung", () => {
        const empty = parseUserAgent("");
        expect_eq(empty.browser == BrowserKind.Unknown, true);
        expect_eq(empty.os == OsKind.Unknown, true);
        expect_eq(empty.device == DeviceKind.Unknown, true);
        expect_eq(empty.is_bot, false);
        expect_eq(empty.is_ai_bot, false);
        expect_eq(empty.is_mobile, false);
        expect_eq(isBot(""), false);
        expect_eq(isAiBot(""), false);
        expect_eq(isMobile(""), false);

        const junk = parseUserAgent("CustomApp/999 ???");
        expect_eq(junk.browser == BrowserKind.Unknown, true);
        expect_eq(junk.os == OsKind.Unknown, true);
        expect_eq(junk.device == DeviceKind.Unknown, true);

        const cros_ua = "Mozilla/5.0 (X11; CrOS x86_64 14541.0.0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36";
        const cr = parseUserAgent(cros_ua);
        expect_eq(cr.os == OsKind.ChromeOS, true);
        expect_eq(std.mem.eql(u8, cr.os_version, "14541.0.0"), true);
        expect_eq(cr.browser == BrowserKind.Chrome, true);
        expect_eq(cr.device == DeviceKind.Desktop, true);

        const sam_ua = "Mozilla/5.0 (Linux; Android 13; SM-S918B) AppleWebKit/537.36 (KHTML, like Gecko) SamsungBrowser/23.0 Chrome/115.0.0.0 Mobile Safari/537.36";
        const sm = parseUserAgent(sam_ua);
        expect_eq(sm.browser == BrowserKind.Samsung, true);
        expect_eq(std.mem.eql(u8, sm.browser_version, "23.0"), true);
        expect_eq(sm.os == OsKind.Android, true);
        expect_eq(sm.is_mobile, true);
    });
}
