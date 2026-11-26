// @cap/bowser — Zero-allocation User-Agent parser in pure CP
// Clean-room MIT implementation inspired by npm `bowser`.

/// Device / client platform classification.
pub enum PlatformType {
    Desktop,
    Mobile,
    Tablet,
    Bot,
    Unknown,
}

/// Parsed User-Agent components (zero-copy slices referencing the input UA string).
pub struct ParsedUA {
    browser_name: string,
    browser_version: string,
    os_name: string,
    os_version: string,
    engine_name: string,
    platform: PlatformType,
}

struct BwsPair {
    name: string,
    version: string,
}

fn bwsToLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn bwsIsVersionChar(c: u8): bool {
    return (c >= '0' && c <= '9') || c == '.' || c == '_';
}

fn bwsFindSubstr(haystack: string, needle: string): usize? {
    if (needle.len == 0) {
        return 0;
    }
    if (needle.len > haystack.len) {
        return null;
    }
    const limit: usize = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) : (i += 1) {
        let j: usize = 0;
        let matched: bool = true;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return i;
        }
    }
    return null;
}

fn bwsFindSubstrCI(haystack: string, needle: string): usize? {
    if (needle.len == 0) {
        return 0;
    }
    if (needle.len > haystack.len) {
        return null;
    }
    const limit: usize = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) : (i += 1) {
        let j: usize = 0;
        let matched: bool = true;
        while (j < needle.len) : (j += 1) {
            if (bwsToLower(haystack[i + j]) != bwsToLower(needle[j])) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return i;
        }
    }
    return null;
}

fn bwsContains(haystack: string, needle: string): bool {
    return bwsFindSubstr(haystack, needle) != null;
}

fn bwsContainsCI(haystack: string, needle: string): bool {
    return bwsFindSubstrCI(haystack, needle) != null;
}

fn bwsExtractVersionAt(ua: string, start: usize): string {
    let idx: usize = start;
    while (idx < ua.len && (ua[idx] == ' ' || ua[idx] == '/')) : (idx += 1) {}
    const vstart: usize = idx;
    while (idx < ua.len && bwsIsVersionChar(ua[idx])) : (idx += 1) {}
    if (idx > vstart && (ua[idx - 1] == '.' || ua[idx - 1] == '_')) {
        idx -= 1;
    }
    return ua[vstart..idx];
}

fn bwsExtractAfter(ua: string, token: string): string {
    if (pos := bwsFindSubstr(ua, token)) {
        return bwsExtractVersionAt(ua, pos + token.len);
    }
    return "";
}

fn bwsDetectBrowser(ua: string): BwsPair {
    if (bwsContains(ua, "Googlebot")) {
        return BwsPair { name: "Googlebot", version: bwsExtractAfter(ua, "Googlebot/") };
    }
    if (bwsContains(ua, "curl/")) {
        return BwsPair { name: "curl", version: bwsExtractAfter(ua, "curl/") };
    }
    if (bwsContains(ua, "Edg/")) {
        return BwsPair { name: "Edge", version: bwsExtractAfter(ua, "Edg/") };
    }
    if (bwsContains(ua, "EdgA/")) {
        return BwsPair { name: "Edge", version: bwsExtractAfter(ua, "EdgA/") };
    }
    if (bwsContains(ua, "EdgiOS/")) {
        return BwsPair { name: "Edge", version: bwsExtractAfter(ua, "EdgiOS/") };
    }
    if (bwsContains(ua, "Edge/")) {
        return BwsPair { name: "Edge", version: bwsExtractAfter(ua, "Edge/") };
    }
    if (bwsContains(ua, "OPR/")) {
        return BwsPair { name: "Opera", version: bwsExtractAfter(ua, "OPR/") };
    }
    if (bwsContains(ua, "Opera/")) {
        const v: string = bwsExtractAfter(ua, "Version/");
        if (v.len > 0) {
            return BwsPair { name: "Opera", version: v };
        }
        return BwsPair { name: "Opera", version: bwsExtractAfter(ua, "Opera/") };
    }
    if (bwsContains(ua, "Brave/")) {
        return BwsPair { name: "Brave", version: bwsExtractAfter(ua, "Brave/") };
    }
    if (bwsContains(ua, "Brave")) {
        return BwsPair { name: "Brave", version: bwsExtractAfter(ua, "Chrome/") };
    }
    if (bwsContains(ua, "Firefox/")) {
        return BwsPair { name: "Firefox", version: bwsExtractAfter(ua, "Firefox/") };
    }
    if (bwsContains(ua, "FxiOS/")) {
        return BwsPair { name: "Firefox", version: bwsExtractAfter(ua, "FxiOS/") };
    }
    if (bwsContains(ua, "Chrome/")) {
        return BwsPair { name: "Chrome", version: bwsExtractAfter(ua, "Chrome/") };
    }
    if (bwsContains(ua, "CriOS/")) {
        return BwsPair { name: "Chrome", version: bwsExtractAfter(ua, "CriOS/") };
    }
    if (bwsContains(ua, "Safari/")) {
        const v: string = bwsExtractAfter(ua, "Version/");
        if (v.len > 0) {
            return BwsPair { name: "Safari", version: v };
        }
        return BwsPair { name: "Safari", version: bwsExtractAfter(ua, "Safari/") };
    }
    return BwsPair { name: "", version: "" };
}

fn bwsDetectOS(ua: string): BwsPair {
    if (bwsContains(ua, "iPhone") || bwsContains(ua, "iPad") || bwsContains(ua, "iPod")) {
        let v: string = bwsExtractAfter(ua, "CPU iPhone OS ");
        if (v.len == 0) {
            v = bwsExtractAfter(ua, "CPU OS ");
        }
        if (v.len == 0) {
            v = bwsExtractAfter(ua, "iPhone OS ");
        }
        return BwsPair { name: "iOS", version: v };
    }
    if (bwsContains(ua, "Android")) {
        let v: string = bwsExtractAfter(ua, "Android ");
        if (v.len == 0) {
            v = bwsExtractAfter(ua, "Android/");
        }
        return BwsPair { name: "Android", version: v };
    }
    if (bwsContains(ua, "Windows")) {
        let v: string = bwsExtractAfter(ua, "Windows NT ");
        if (v.len == 0) {
            v = bwsExtractAfter(ua, "Windows Phone ");
        }
        return BwsPair { name: "Windows", version: v };
    }
    if (bwsContains(ua, "Mac OS X") || bwsContains(ua, "Macintosh")) {
        return BwsPair { name: "macOS", version: bwsExtractAfter(ua, "Mac OS X ") };
    }
    if (bwsContains(ua, "Linux") || bwsContains(ua, "X11")) {
        return BwsPair { name: "Linux", version: "" };
    }
    return BwsPair { name: "", version: "" };
}

fn bwsDetectEngine(ua: string): string {
    if (bwsContains(ua, "Firefox/") || (bwsContains(ua, "Gecko/") && !bwsContains(ua, "like Gecko"))) {
        return "Gecko";
    }
    const is_ios: bool = bwsContains(ua, "iPhone") || bwsContains(ua, "iPad") || bwsContains(ua, "iPod");
    if (!is_ios && (bwsContains(ua, "Chrome/") || bwsContains(ua, "Chromium/") || bwsContains(ua, "Edg/") || bwsContains(ua, "EdgA/") || bwsContains(ua, "OPR/"))) {
        return "Blink";
    }
    if (bwsContains(ua, "AppleWebKit/")) {
        return "WebKit";
    }
    return "";
}

fn bwsDetectPlatform(ua: string, os_name: string): PlatformType {
    if (bwsContainsCI(ua, "bot") || bwsContainsCI(ua, "spider") || bwsContainsCI(ua, "crawl") || bwsContainsCI(ua, "slurp")) {
        return PlatformType.Bot;
    }
    if (bwsContains(ua, "iPad") || bwsContainsCI(ua, "tablet")) {
        return PlatformType.Tablet;
    }
    if (bwsContains(ua, "Android") && !bwsContains(ua, "Mobile")) {
        return PlatformType.Tablet;
    }
    if (bwsContains(ua, "iPhone") || bwsContains(ua, "iPod") || bwsContains(ua, "Mobile") || bwsContains(ua, "Android")) {
        return PlatformType.Mobile;
    }
    if (os_name.len > 0) {
        return PlatformType.Desktop;
    }
    return PlatformType.Unknown;
}

/// Parse a User-Agent string into browser, OS, engine, and platform metadata with zero heap allocations.
pub fn parseUserAgent(ua: string): ParsedUA {
    const browser: BwsPair = bwsDetectBrowser(ua);
    const os: BwsPair = bwsDetectOS(ua);
    const engine_name: string = bwsDetectEngine(ua);
    const platform: PlatformType = bwsDetectPlatform(ua, os.name);

    return ParsedUA {
        browser_name: browser.name,
        browser_version: browser.version,
        os_name: os.name,
        os_version: os.version,
        engine_name: engine_name,
        platform: platform,
    };
}
