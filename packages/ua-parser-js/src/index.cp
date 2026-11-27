// @cap/useragent — Zero-Allocation Industrial User-Agent Parser & Bot Classifier
// Clean-room pure-CP implementation modeled after npm `ua-parser-js`, `bowser`, and Rust `woothee` (MIT License).
//
// Features:
// - Zero heap allocations: all returned strings are static literals or zero-copy slices of the input UA
// - Accurate Browser disambiguation: Edge (`Edg/`), Opera (`OPR/`), Samsung Internet, Firefox, Chrome, Safari, CLI runtimes (`curl`/`Bun`/`Cap`), Bots
// - OS detection: HarmonyOS / OpenHarmony, iOS, Android, ChromeOS, Windows, macOS, Linux
// - Device classification: Desktop, Mobile, Tablet, Bot, Cli, Unknown
// - Dedicated AI crawler detection (`isAiBot`): GPTBot, ClaudeBot, PerplexityBot, Bytespider, Google-Extended, CCBot, etc.

const std = @import("std");

/// High-level device category.
pub enum DeviceKind: u8 {
    Desktop = 0,
    Mobile = 1,
    Tablet = 2,
    Bot = 3,
    Cli = 4,
    Unknown = 5,
}

/// Identified browser, runtime, or bot client kind.
pub enum BrowserKind: u8 {
    Chrome = 0,
    Edge = 1,
    Firefox = 2,
    Safari = 3,
    Opera = 4,
    Samsung = 5,
    Curl = 6,
    Bun = 7,
    Cap = 8,
    Bot = 9,
    Unknown = 10,
}

/// Identified operating system family.
pub enum OsKind: u8 {
    Windows = 0,
    MacOS = 1,
    IOS = 2,
    Android = 3,
    Linux = 4,
    HarmonyOS = 5,
    ChromeOS = 6,
    Unknown = 7,
}

/// Parsed User-Agent metadata (stack-allocated, zero heap allocations).
pub struct UserAgent {
    browser: BrowserKind = BrowserKind.Unknown,
    browser_name: string = "Unknown",
    browser_version: string = "",
    os: OsKind = OsKind.Unknown,
    os_name: string = "Unknown",
    os_version: string = "",
    device: DeviceKind = DeviceKind.Unknown,
    engine_name: string = "Unknown",
    is_bot: bool = false,
    is_mobile: bool = false,
    is_ai_bot: bool = false,
    bot_name: string = "",
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn findSubstr(haystack: string, needle: string): usize? {
    if (needle.len == 0) return 0;
    if (needle.len > haystack.len) return null;
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) : (i += 1) {
        let j: usize = 0;
        let matched = true;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                matched = false;
                break;
            }
        }
        if (matched) return i;
    }
    return null;
}

fn findSubstrCI(haystack: string, needle: string): usize? {
    if (needle.len == 0) return 0;
    if (needle.len > haystack.len) return null;
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) : (i += 1) {
        let j: usize = 0;
        let matched = true;
        while (j < needle.len) : (j += 1) {
            if (toLowerAscii(haystack[i + j]) != toLowerAscii(needle[j])) {
                matched = false;
                break;
            }
        }
        if (matched) return i;
    }
    return null;
}

fn contains(haystack: string, needle: string): bool {
    return findSubstr(haystack, needle) != null;
}

fn containsCI(haystack: string, needle: string): bool {
    return findSubstrCI(haystack, needle) != null;
}

fn isVersionChar(c: u8): bool {
    return (c >= '0' && c <= '9') || c == '.' || c == '_';
}

/// Extracts a version token immediately following `prefix` (case-sensitive).
fn extractVersionAfter(ua: string, prefix: string): string {
    if (idx := findSubstr(ua, prefix)) {
        const start = idx + prefix.len;
        let end = start;
        while (end < ua.len && isVersionChar(ua[end])) : (end += 1) {}
        if (end > start) return ua[start..end];
    }
    return "";
}

/// Extracts a version token immediately following `prefix` (case-insensitive).
fn extractVersionAfterCI(ua: string, prefix: string): string {
    if (idx := findSubstrCI(ua, prefix)) {
        const start = idx + prefix.len;
        let end = start;
        while (end < ua.len && isVersionChar(ua[end])) : (end += 1) {}
        if (end > start) return ua[start..end];
    }
    return "";
}

struct BotMatch {
    matched: bool = false,
    is_ai: bool = false,
    name: string = "",
    version: string = "",
}

fn matchAiBot(ua: string): BotMatch {
    if (contains(ua, "GPTBot")) {
        return BotMatch{ matched: true, is_ai: true, name: "GPTBot", version: extractVersionAfter(ua, "GPTBot/") };
    }
    if (contains(ua, "ChatGPT-User")) {
        return BotMatch{ matched: true, is_ai: true, name: "ChatGPT-User", version: extractVersionAfter(ua, "ChatGPT-User/") };
    }
    if (contains(ua, "OAI-SearchBot")) {
        return BotMatch{ matched: true, is_ai: true, name: "OAI-SearchBot", version: extractVersionAfter(ua, "OAI-SearchBot/") };
    }
    if (contains(ua, "ClaudeBot")) {
        return BotMatch{ matched: true, is_ai: true, name: "ClaudeBot", version: extractVersionAfter(ua, "ClaudeBot/") };
    }
    if (contains(ua, "Claude-Web")) {
        return BotMatch{ matched: true, is_ai: true, name: "Claude-Web", version: extractVersionAfter(ua, "Claude-Web/") };
    }
    if (containsCI(ua, "anthropic-ai")) {
        return BotMatch{ matched: true, is_ai: true, name: "anthropic-ai", version: extractVersionAfterCI(ua, "anthropic-ai/") };
    }
    if (contains(ua, "PerplexityBot")) {
        return BotMatch{ matched: true, is_ai: true, name: "PerplexityBot", version: extractVersionAfter(ua, "PerplexityBot/") };
    }
    if (contains(ua, "Bytespider")) {
        return BotMatch{ matched: true, is_ai: true, name: "Bytespider", version: extractVersionAfter(ua, "Bytespider/") };
    }
    if (contains(ua, "Google-Extended")) {
        return BotMatch{ matched: true, is_ai: true, name: "Google-Extended", version: extractVersionAfter(ua, "Google-Extended/") };
    }
    if (contains(ua, "CCBot")) {
        return BotMatch{ matched: true, is_ai: true, name: "CCBot", version: extractVersionAfter(ua, "CCBot/") };
    }
    if (contains(ua, "Applebot-Extended")) {
        return BotMatch{ matched: true, is_ai: true, name: "Applebot-Extended", version: extractVersionAfter(ua, "Applebot-Extended/") };
    }
    if (contains(ua, "Meta-ExternalAgent")) {
        return BotMatch{ matched: true, is_ai: true, name: "Meta-ExternalAgent", version: extractVersionAfter(ua, "Meta-ExternalAgent/") };
    }
    if (contains(ua, "Amazonbot")) {
        return BotMatch{ matched: true, is_ai: true, name: "Amazonbot", version: extractVersionAfter(ua, "Amazonbot/") };
    }
    if (contains(ua, "Diffbot")) {
        return BotMatch{ matched: true, is_ai: true, name: "Diffbot", version: extractVersionAfter(ua, "Diffbot/") };
    }
    if (containsCI(ua, "cohere-ai")) {
        return BotMatch{ matched: true, is_ai: true, name: "cohere-ai", version: extractVersionAfterCI(ua, "cohere-ai/") };
    }
    if (contains(ua, "DeepSeekBot")) {
        return BotMatch{ matched: true, is_ai: true, name: "DeepSeekBot", version: extractVersionAfter(ua, "DeepSeekBot/") };
    }
    return BotMatch{};
}

fn matchAnyBot(ua: string): BotMatch {
    const ai = matchAiBot(ua);
    if (ai.matched) return ai;

    if (containsCI(ua, "Googlebot")) {
        return BotMatch{ matched: true, is_ai: false, name: "Googlebot", version: extractVersionAfterCI(ua, "Googlebot/") };
    }
    if (containsCI(ua, "Bingbot")) {
        return BotMatch{ matched: true, is_ai: false, name: "Bingbot", version: extractVersionAfterCI(ua, "Bingbot/") };
    }
    if (containsCI(ua, "Baiduspider")) {
        return BotMatch{ matched: true, is_ai: false, name: "Baiduspider", version: extractVersionAfterCI(ua, "Baiduspider/") };
    }
    if (containsCI(ua, "YandexBot")) {
        return BotMatch{ matched: true, is_ai: false, name: "YandexBot", version: extractVersionAfterCI(ua, "YandexBot/") };
    }
    if (containsCI(ua, "DuckDuckBot")) {
        return BotMatch{ matched: true, is_ai: false, name: "DuckDuckBot", version: extractVersionAfterCI(ua, "DuckDuckBot/") };
    }
    if (contains(ua, "Applebot")) {
        return BotMatch{ matched: true, is_ai: false, name: "Applebot", version: extractVersionAfter(ua, "Applebot/") };
    }
    if (contains(ua, "Slurp")) {
        return BotMatch{ matched: true, is_ai: false, name: "Yahoo! Slurp", version: "" };
    }
    if (contains(ua, "facebookexternalhit")) {
        return BotMatch{ matched: true, is_ai: false, name: "FacebookBot", version: extractVersionAfter(ua, "facebookexternalhit/") };
    }
    if (containsCI(ua, "Twitterbot")) {
        return BotMatch{ matched: true, is_ai: false, name: "Twitterbot", version: extractVersionAfterCI(ua, "Twitterbot/") };
    }
    if (containsCI(ua, "LinkedInBot")) {
        return BotMatch{ matched: true, is_ai: false, name: "LinkedInBot", version: extractVersionAfterCI(ua, "LinkedInBot/") };
    }
    if (containsCI(ua, "Slackbot")) {
        return BotMatch{ matched: true, is_ai: false, name: "Slackbot", version: extractVersionAfterCI(ua, "Slackbot/") };
    }
    if (containsCI(ua, "Discordbot")) {
        return BotMatch{ matched: true, is_ai: false, name: "Discordbot", version: extractVersionAfterCI(ua, "Discordbot/") };
    }
    if (containsCI(ua, "TelegramBot")) {
        return BotMatch{ matched: true, is_ai: false, name: "TelegramBot", version: "" };
    }
    if (containsCI(ua, "AhrefsBot")) {
        return BotMatch{ matched: true, is_ai: false, name: "AhrefsBot", version: extractVersionAfterCI(ua, "AhrefsBot/") };
    }
    if (containsCI(ua, "SemrushBot")) {
        return BotMatch{ matched: true, is_ai: false, name: "SemrushBot", version: extractVersionAfterCI(ua, "SemrushBot/") };
    }
    // Generic crawler/spider/headless heuristics
    if (containsCI(ua, "crawler")
        || containsCI(ua, "spider")
        || containsCI(ua, "bot/")
        || containsCI(ua, "bot;")
        || containsCI(ua, "bot)")
        || containsCI(ua, "headlesschrome")
        || containsCI(ua, "lighthouse"))
    {
        return BotMatch{ matched: true, is_ai: false, name: "Bot", version: "" };
    }
    return BotMatch{};
}

fn parseOs(ua: string, out: *UserAgent) {
    // HarmonyOS / OpenHarmony (check before Android & Linux)
    if (contains(ua, "HarmonyOS") || contains(ua, "OpenHarmony")) {
        out.os = OsKind.HarmonyOS;
        out.os_name = "HarmonyOS";
        let v = extractVersionAfter(ua, "HarmonyOS ");
        if (v.len == 0) v = extractVersionAfter(ua, "HarmonyOS/");
        if (v.len == 0) v = extractVersionAfter(ua, "OpenHarmony ");
        if (v.len == 0) v = extractVersionAfter(ua, "OpenHarmony/");
        out.os_version = v;
        return;
    }
    // iOS (check before macOS because iOS UAs contain "like Mac OS X")
    if (contains(ua, "iPhone") || contains(ua, "iPad") || contains(ua, "iPod") || contains(ua, "CPU iPhone OS")) {
        out.os = OsKind.IOS;
        out.os_name = "iOS";
        let v = extractVersionAfter(ua, "CPU iPhone OS ");
        if (v.len == 0) v = extractVersionAfter(ua, "CPU OS ");
        if (v.len == 0) v = extractVersionAfter(ua, "iPhone OS ");
        out.os_version = v;
        return;
    }
    // Android (check before Linux because Android UAs contain "Linux;")
    if (contains(ua, "Android")) {
        out.os = OsKind.Android;
        out.os_name = "Android";
        out.os_version = extractVersionAfter(ua, "Android ");
        return;
    }
    // ChromeOS
    if (contains(ua, "CrOS")) {
        out.os = OsKind.ChromeOS;
        out.os_name = "ChromeOS";
        let v = extractVersionAfter(ua, "CrOS x86_64 ");
        if (v.len == 0) v = extractVersionAfter(ua, "CrOS aarch64 ");
        if (v.len == 0) v = extractVersionAfter(ua, "CrOS armv7l ");
        out.os_version = v;
        return;
    }
    // Windows
    if (contains(ua, "Windows") || contains(ua, "Win64") || contains(ua, "Win32")) {
        out.os = OsKind.Windows;
        out.os_name = "Windows";
        out.os_version = extractVersionAfter(ua, "Windows NT ");
        return;
    }
    // macOS
    if (contains(ua, "Mac OS X") || contains(ua, "Macintosh")) {
        out.os = OsKind.MacOS;
        out.os_name = "macOS";
        out.os_version = extractVersionAfter(ua, "Mac OS X ");
        return;
    }
    // Linux
    if (contains(ua, "Linux") || contains(ua, "X11")) {
        out.os = OsKind.Linux;
        out.os_name = "Linux";
        return;
    }
}

fn isGenericCli(ua: string): bool {
    return containsCI(ua, "Wget/")
        || containsCI(ua, "HTTPie/")
        || containsCI(ua, "python-requests/")
        || containsCI(ua, "Go-http-client/")
        || containsCI(ua, "undici")
        || containsCI(ua, "node-fetch");
}

fn parseBrowserAndDevice(ua: string, out: *UserAgent) {
    // 1. Bots (AI bots + search/social crawlers)
    const bot = matchAnyBot(ua);
    if (bot.matched) {
        out.is_bot = true;
        out.is_ai_bot = bot.is_ai;
        out.bot_name = bot.name;
        out.browser = BrowserKind.Bot;
        out.browser_name = bot.name;
        out.browser_version = bot.version;
        out.engine_name = "Bot";
        out.device = DeviceKind.Bot;
        out.is_mobile = false;
        return;
    }

    // 2. CLI / Runtime HTTP clients
    if (containsCI(ua, "curl/")) {
        out.browser = BrowserKind.Curl;
        out.browser_name = "curl";
        out.browser_version = extractVersionAfterCI(ua, "curl/");
        out.engine_name = "curl";
        out.device = DeviceKind.Cli;
        out.is_mobile = false;
        return;
    }
    if (contains(ua, "Bun/")) {
        out.browser = BrowserKind.Bun;
        out.browser_name = "Bun";
        out.browser_version = extractVersionAfter(ua, "Bun/");
        out.engine_name = "JavaScriptCore";
        out.device = DeviceKind.Cli;
        out.is_mobile = false;
        return;
    }
    if (contains(ua, "Cap/") || contains(ua, "cap/")) {
        out.browser = BrowserKind.Cap;
        out.browser_name = "Cap";
        let v = extractVersionAfter(ua, "Cap/");
        if (v.len == 0) v = extractVersionAfter(ua, "cap/");
        out.browser_version = v;
        out.engine_name = "Cap";
        out.device = DeviceKind.Cli;
        out.is_mobile = false;
        return;
    }

    // 3. Web Browsers (strict specificity order)
    if (contains(ua, "Edg/") || contains(ua, "EdgA/") || contains(ua, "EdgiOS/") || contains(ua, "Edge/")) {
        out.browser = BrowserKind.Edge;
        out.browser_name = "Edge";
        if (contains(ua, "Edg/")) {
            out.browser_version = extractVersionAfter(ua, "Edg/");
            out.engine_name = "Blink";
        } else if (contains(ua, "EdgA/")) {
            out.browser_version = extractVersionAfter(ua, "EdgA/");
            out.engine_name = "Blink";
        } else if (contains(ua, "EdgiOS/")) {
            out.browser_version = extractVersionAfter(ua, "EdgiOS/");
            out.engine_name = "WebKit";
        } else {
            out.browser_version = extractVersionAfter(ua, "Edge/");
            out.engine_name = "EdgeHTML";
        }
    } else if (contains(ua, "OPR/") || contains(ua, "Opera/")) {
        out.browser = BrowserKind.Opera;
        out.browser_name = "Opera";
        if (contains(ua, "OPR/")) {
            out.browser_version = extractVersionAfter(ua, "OPR/");
            out.engine_name = "Blink";
        } else {
            out.browser_version = extractVersionAfter(ua, "Opera/");
            out.engine_name = "Presto";
        }
    } else if (contains(ua, "SamsungBrowser/")) {
        out.browser = BrowserKind.Samsung;
        out.browser_name = "Samsung Internet";
        out.browser_version = extractVersionAfter(ua, "SamsungBrowser/");
        out.engine_name = "Blink";
    } else if (contains(ua, "Firefox/") || contains(ua, "FxiOS/")) {
        out.browser = BrowserKind.Firefox;
        out.browser_name = "Firefox";
        if (contains(ua, "Firefox/")) {
            out.browser_version = extractVersionAfter(ua, "Firefox/");
            out.engine_name = "Gecko";
        } else {
            out.browser_version = extractVersionAfter(ua, "FxiOS/");
            out.engine_name = "WebKit";
        }
    } else if (contains(ua, "Chrome/") || contains(ua, "CriOS/")) {
        out.browser = BrowserKind.Chrome;
        out.browser_name = "Chrome";
        if (contains(ua, "Chrome/")) {
            out.browser_version = extractVersionAfter(ua, "Chrome/");
            out.engine_name = "Blink";
        } else {
            out.browser_version = extractVersionAfter(ua, "CriOS/");
            out.engine_name = "WebKit";
        }
    } else if (contains(ua, "Safari/") && !contains(ua, "Chromium/")) {
        out.browser = BrowserKind.Safari;
        out.browser_name = "Safari";
        let v = extractVersionAfter(ua, "Version/");
        if (v.len == 0) v = extractVersionAfter(ua, "Safari/");
        out.browser_version = v;
        out.engine_name = "WebKit";
    } else if (isGenericCli(ua)) {
        out.device = DeviceKind.Cli;
        out.is_mobile = false;
        return;
    } else {
        if (contains(ua, "AppleWebKit/")) {
            out.engine_name = "WebKit";
        } else if (contains(ua, "Gecko/")) {
            out.engine_name = "Gecko";
        } else if (contains(ua, "Trident/")) {
            out.engine_name = "Trident";
        }
    }

    // 4. Device kind & mobile flag
    if (contains(ua, "iPad")
        || contains(ua, "Tablet")
        || contains(ua, "PlayBook")
        || contains(ua, "Silk/")
        || (out.os == OsKind.Android && !contains(ua, "Mobile")))
    {
        out.device = DeviceKind.Tablet;
        out.is_mobile = false;
    } else if (contains(ua, "iPhone")
        || contains(ua, "iPod")
        || contains(ua, "Mobile")
        || contains(ua, "Phone")
        || contains(ua, "Windows Phone")
        || contains(ua, "BlackBerry")
        || contains(ua, "Opera Mini")
        || contains(ua, "IEMobile")
        || out.os == OsKind.HarmonyOS)
    {
        out.device = DeviceKind.Mobile;
        out.is_mobile = true;
    } else if (out.os == OsKind.Windows
        || out.os == OsKind.MacOS
        || out.os == OsKind.Linux
        || out.os == OsKind.ChromeOS
        || out.browser != BrowserKind.Unknown)
    {
        out.device = DeviceKind.Desktop;
        out.is_mobile = false;
    } else {
        out.device = DeviceKind.Unknown;
        out.is_mobile = false;
    }
}

/// Parses a raw HTTP `User-Agent` header string with zero heap allocations.
pub fn parseUserAgent(ua: string): UserAgent {
    let res = UserAgent{};
    if (ua.len == 0) return res;
    parseOs(ua, *res);
    parseBrowserAndDevice(ua, *res);
    return res;
}

/// Returns `true` if `ua` belongs to any search engine, social, monitoring, or AI bot/crawler.
pub fn isBot(ua: string): bool {
    if (ua.len == 0) return false;
    return matchAnyBot(ua).matched;
}

/// Returns `true` if `ua` belongs to a known AI training/search crawler
/// (e.g. `GPTBot`, `ClaudeBot`, `PerplexityBot`, `Bytespider`, `Google-Extended`, `CCBot`).
pub fn isAiBot(ua: string): bool {
    if (ua.len == 0) return false;
    return matchAiBot(ua).matched;
}

/// Returns `true` if `ua` represents a mobile phone client (excludes bots, CLI tools, and tablets).
pub fn isMobile(ua: string): bool {
    if (ua.len == 0) return false;
    return parseUserAgent(ua).is_mobile;
}
