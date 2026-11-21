import {
    UserAgentIter,
    BrowserFamily,
    OsFamily,
    isBotUserAgent,
    isMobileUserAgent,
    detectBrowser,
    detectOs,
    findProductVersion,
} from "cap:http";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("UserAgentIter skips comments and extracts product versions", () => {
    const chrome = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36";
    let it = UserAgentIter.init(chrome);
    const p1 = it.next().?;
    expect(strEq(p1.name, "Mozilla") && strEq(p1.version.?, "5.0"));
    const p2 = it.next().?;
    expect(strEq(p2.name, "AppleWebKit"));
    const p3 = it.next().?;
    expect(strEq(p3.name, "Chrome") && strEq(p3.version.?, "128.0.0.0"));
    const p4 = it.next().?;
    expect(strEq(p4.name, "Safari"));
    expect(it.next() == null);

    expect(strEq(findProductVersion(chrome, "chrome").?, "128.0.0.0"));
    expect(detectBrowser(chrome) == BrowserFamily.chrome);
    expect(detectOs(chrome) == OsFamily.macos);
    expect(!isMobileUserAgent(chrome));
    expect(!isBotUserAgent(chrome));
});

test("detectBrowser and detectOs classify Edge, Safari, Bun, curl, Android, iOS, and bots", () => {
    const edge = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/128.0 Safari/537.36 Edg/128.0";
    expect(detectBrowser(edge) == BrowserFamily.edge);
    expect(detectOs(edge) == OsFamily.windows);

    const ios = "Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X) AppleWebKit/605.1.15 Mobile/15E148 Safari/604.1";
    expect(detectBrowser(ios) == BrowserFamily.safari);
    expect(detectOs(ios) == OsFamily.ios);
    expect(isMobileUserAgent(ios));

    const bot = "Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)";
    expect(isBotUserAgent(bot));
    expect(detectBrowser(bot) == BrowserFamily.bot);

    expect(detectBrowser("Bun/1.3.14") == BrowserFamily.bun);
    expect(detectBrowser("curl/8.7.1") == BrowserFamily.curl);
});
