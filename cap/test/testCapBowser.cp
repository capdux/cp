import { test, expect_eq } from "cap:test";
import { PlatformType, ParsedUA, parseUserAgent } from "../../packages/@cap/bowser/src/index.cp";

fn main() {
    test("1. @cap/bowser — Chrome and Edge on Windows Desktop (Blink)", () => {
        const chrome: ParsedUA = parseUserAgent(
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.6099.109 Safari/537.36"
        );
        expect_eq(chrome.browser_name, "Chrome");
        expect_eq(chrome.browser_version, "120.0.6099.109");
        expect_eq(chrome.os_name, "Windows");
        expect_eq(chrome.os_version, "10.0");
        expect_eq(chrome.engine_name, "Blink");
        expect_eq(chrome.platform == PlatformType.Desktop, true);

        const edge: ParsedUA = parseUserAgent(
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 Edg/120.0.2210.91"
        );
        expect_eq(edge.browser_name, "Edge");
        expect_eq(edge.browser_version, "120.0.2210.91");
        expect_eq(edge.os_name, "Windows");
        expect_eq(edge.engine_name, "Blink");
        expect_eq(edge.platform == PlatformType.Desktop, true);
    });

    test("2. @cap/bowser — Firefox on Linux Desktop (Gecko)", () => {
        const ff: ParsedUA = parseUserAgent(
            "Mozilla/5.0 (X11; Linux x86_64; rv:121.0) Gecko/20100101 Firefox/121.0"
        );
        expect_eq(ff.browser_name, "Firefox");
        expect_eq(ff.browser_version, "121.0");
        expect_eq(ff.os_name, "Linux");
        expect_eq(ff.os_version, "");
        expect_eq(ff.engine_name, "Gecko");
        expect_eq(ff.platform == PlatformType.Desktop, true);
    });

    test("3. @cap/bowser — Safari on macOS Desktop and iOS Mobile/Tablet (WebKit)", () => {
        const mac: ParsedUA = parseUserAgent(
            "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.2 Safari/605.1.15"
        );
        expect_eq(mac.browser_name, "Safari");
        expect_eq(mac.browser_version, "17.2");
        expect_eq(mac.os_name, "macOS");
        expect_eq(mac.os_version, "10_15_7");
        expect_eq(mac.engine_name, "WebKit");
        expect_eq(mac.platform == PlatformType.Desktop, true);

        const iphone: ParsedUA = parseUserAgent(
            "Mozilla/5.0 (iPhone; CPU iPhone OS 17_2 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.2 Mobile/15E148 Safari/604.1"
        );
        expect_eq(iphone.browser_name, "Safari");
        expect_eq(iphone.browser_version, "17.2");
        expect_eq(iphone.os_name, "iOS");
        expect_eq(iphone.os_version, "17_2");
        expect_eq(iphone.engine_name, "WebKit");
        expect_eq(iphone.platform == PlatformType.Mobile, true);

        const ipad: ParsedUA = parseUserAgent(
            "Mozilla/5.0 (iPad; CPU OS 17_2 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.2 Mobile/15E148 Safari/604.1"
        );
        expect_eq(ipad.browser_name, "Safari");
        expect_eq(ipad.os_name, "iOS");
        expect_eq(ipad.os_version, "17_2");
        expect_eq(ipad.platform == PlatformType.Tablet, true);
    });

    test("4. @cap/bowser — Android Mobile vs Tablet", () => {
        const phone: ParsedUA = parseUserAgent(
            "Mozilla/5.0 (Linux; Android 14; Pixel 8) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.6099.144 Mobile Safari/537.36"
        );
        expect_eq(phone.browser_name, "Chrome");
        expect_eq(phone.browser_version, "120.0.6099.144");
        expect_eq(phone.os_name, "Android");
        expect_eq(phone.os_version, "14");
        expect_eq(phone.engine_name, "Blink");
        expect_eq(phone.platform == PlatformType.Mobile, true);

        const tablet: ParsedUA = parseUserAgent(
            "Mozilla/5.0 (Linux; Android 13; Pixel Tablet) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.6099.144 Safari/537.36"
        );
        expect_eq(tablet.os_name, "Android");
        expect_eq(tablet.os_version, "13");
        expect_eq(tablet.platform == PlatformType.Tablet, true);
    });

    test("5. @cap/bowser — Opera and Brave browsers", () => {
        const opera: ParsedUA = parseUserAgent(
            "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 OPR/106.0.0.0"
        );
        expect_eq(opera.browser_name, "Opera");
        expect_eq(opera.browser_version, "106.0.0.0");
        expect_eq(opera.os_name, "macOS");
        expect_eq(opera.engine_name, "Blink");
        expect_eq(opera.platform == PlatformType.Desktop, true);

        const brave: ParsedUA = parseUserAgent(
            "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 Brave/1.61.109"
        );
        expect_eq(brave.browser_name, "Brave");
        expect_eq(brave.browser_version, "1.61.109");
        expect_eq(brave.os_name, "Linux");
        expect_eq(brave.engine_name, "Blink");
    });

    test("6. @cap/bowser — Googlebot, curl, and empty/unknown UA", () => {
        const bot: ParsedUA = parseUserAgent(
            "Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)"
        );
        expect_eq(bot.browser_name, "Googlebot");
        expect_eq(bot.browser_version, "2.1");
        expect_eq(bot.platform == PlatformType.Bot, true);

        const curl: ParsedUA = parseUserAgent("curl/8.4.0");
        expect_eq(curl.browser_name, "curl");
        expect_eq(curl.browser_version, "8.4.0");
        expect_eq(curl.platform == PlatformType.Unknown, true);

        const empty: ParsedUA = parseUserAgent("");
        expect_eq(empty.browser_name, "");
        expect_eq(empty.browser_version, "");
        expect_eq(empty.os_name, "");
        expect_eq(empty.engine_name, "");
        expect_eq(empty.platform == PlatformType.Unknown, true);
    });
}
