[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# ua-parser-js

Zero-allocation User-Agent parser with Browser/OS/Device/Engine detection, Search & AI Bot classification (`GPTBot`, `ClaudeBot`, `PerplexityBot`, `Bytespider`, `DeepSeekBot`, etc.), and CLI runtime identification for CP — clean-room MIT parity with npm `ua-parser-js`.

Disambiguates modern browsers accurately (Edge `Edg/`, Opera `OPR/`, Samsung Internet, Firefox, Chrome, Safari, CLI clients `curl`/`Bun`/`Cap`), detects desktop/mobile/tablet/bot/CLI devices across Windows/macOS/iOS/Android/Linux/HarmonyOS/ChromeOS, and returns a stack-allocated `UserAgent` struct with zero heap allocations.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **npm** |
| Original Package | `ua-parser-js` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/faisalman/ua-parser-js` |
| License | MIT |

Keep the exact package name when migrating from TypeScript/Node.js to CP:

```ts
// TypeScript / Node.js (npm: ua-parser-js)
import { UAParser } from "ua-parser-js";
const result = UAParser(req.headers["user-agent"]);
```

```cp
// CP (cap: ua-parser-js) — zero-allocation stack UserAgent & AI bot guard
import { parseUserAgent, isAiBot } from "ua-parser-js";

const ua = parseUserAgent(ua_header);
const block_ai = isAiBot(ua_header);
```

## Installation

```bash
cap add ua-parser-js
```

## Usage

### Basic Usage: Browser, OS, Device & Engine Classification

```cp
import {
    parseUserAgent,
    BrowserKind,
    OsKind,
    DeviceKind,
} from "ua-parser-js";

fn inspectRequest(ua_header: string) {
    const info = parseUserAgent(ua_header);

    if (info.browser == BrowserKind.Edge && info.os == OsKind.Windows) {
        // Desktop Microsoft Edge on Windows
        // info.browser_version, info.os_version, info.engine_name ("Blink")
    }
    if (info.device == DeviceKind.Mobile) {
        // Serve mobile layout
    }
    _ = info;
}
```

### Edge Cases: AI Crawler Filtering, CLI Runtimes & Empty UA Strings

```cp
import { parseUserAgent, isBot, isAiBot, isMobile, BrowserKind, DeviceKind } from "ua-parser-js";

fn demoGuards() {
    // 1. Fast AI crawler detection (GPTBot, ClaudeBot, PerplexityBot, Bytespider, CCBot, DeepSeekBot...)
    const ai_ua = "Mozilla/5.0 AppleWebKit/537.36 (KHTML, like Gecko); compatible; GPTBot/1.2";
    const ai = isAiBot(ai_ua); // true
    const bot = isBot(ai_ua);  // true

    // 2. CLI runtime detection (curl, Bun, Cap)
    const cli = parseUserAgent("curl/8.7.1");
    // cli.browser == BrowserKind.Curl, cli.device == DeviceKind.Cli

    // 3. Empty or unknown User-Agent degrades safely to Unknown
    const empty = parseUserAgent("");
    // empty.browser == BrowserKind.Unknown, empty.device == DeviceKind.Unknown
    const mob = isMobile(""); // false
    _ = ai;
    _ = bot;
    _ = cli;
    _ = empty;
    _ = mob;
}
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `DeviceKind` | `enum { Desktop, Mobile, Tablet, Bot, Cli, Unknown }` | Device category. |
| `BrowserKind` | `enum { Chrome, Edge, Firefox, Safari, Opera, Samsung, Curl, Bun, Cap, Bot, Unknown }` | Browser / runtime category. |
| `OsKind` | `enum { Windows, MacOS, IOS, Android, Linux, HarmonyOS, ChromeOS, Unknown }` | Operating system category. |
| `UserAgent` | `class { browser, browser_name, browser_version, os, os_name, os_version, device, engine_name, is_bot, is_mobile, is_ai_bot, bot_name }` | Full stack-allocated parse result with zero-copy version slices. |
| `parseUserAgent` | `fn(ua: string): UserAgent` | Performs full zero-allocation User-Agent parsing. |
| `isBot` | `fn(ua: string): bool` | Fast check for search engines, AI crawlers, and generic bots. |
| `isAiBot` | `fn(ua: string): bool` | Fast check specifically for AI training/retrieval crawlers. |
| `isMobile` | `fn(ua: string): bool` | Fast check for mobile handset User-Agents. |
| `matchAiBot` / `matchAnyBot` | `fn(ua: string): BotMatch` | Low-level bot matcher returning `{ matched, is_ai, name, version }`. |

## Performance & Safety Highlights

- **16-lane SIMD Acceleration**: Uses `cap:ascii` (`eqIgnoreCase`, `isDigit`) and `cap:bin` (`indexOfBytes`) for high-throughput substring and case-insensitive scanning without regex backtracking.
- **Zero Heap Allocation & Zero-Copy**: `UserAgent` is returned by value on the stack; version/name strings slice either static literals or `ua` directly.
- **ReDoS-Immune**: Pure linear substring scanning with zero regular expressions — safe for untrusted HTTP `User-Agent` headers at edge gateways.

## License

MIT — Clean-room CP implementation inspired by npm `ua-parser-js`.
