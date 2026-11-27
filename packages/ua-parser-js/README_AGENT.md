# ua-parser-js@1.0.0 — AI Agent Quick Ref

> Zero-alloc User-Agent parser with Browser/OS/Device/Engine, CLI & AI Bot detection | Source: `npm:ua-parser-js` | License: MIT
> Import: `import { DeviceKind, BrowserKind, OsKind, UserAgent, parseUserAgent, isBot, isAiBot, isMobile } from "ua-parser-js";` | Types: `src/index.d.cp`

## Core Types

```cp
export enum DeviceKind { Desktop = 0, Mobile = 1, Tablet = 2, Bot = 3, Cli = 4, Unknown = 5 }
export enum BrowserKind {
    Chrome = 0, Edge = 1, Firefox = 2, Safari = 3, Opera = 4,
    Samsung = 5, Curl = 6, Bun = 7, Cap = 8, Bot = 9, Unknown = 10
}
export enum OsKind {
    Windows = 0, MacOS = 1, IOS = 2, Android = 3, Linux = 4,
    HarmonyOS = 5, ChromeOS = 6, Unknown = 7
}

export class UserAgent {
    browser: BrowserKind, browser_name: string, browser_version: string,
    os: OsKind, os_name: string, os_version: string,
    device: DeviceKind, engine_name: string,
    is_bot: bool, is_mobile: bool, is_ai_bot: bool, bot_name: string,
}

export class BotMatch { matched: bool, is_ai: bool, name: string, version: string }
```

## Core Functions

- `parseUserAgent(ua: string): UserAgent` — Full zero-alloc parse (no output buffer needed).
- `isBot(ua: string): bool` — Fast bot/crawler check.
- `isAiBot(ua: string): bool` — Fast AI crawler check (`GPTBot`, `ChatGPT-User`, `OAI-SearchBot`, `ClaudeBot`, `PerplexityBot`, `Bytespider`, `Google-Extended`, `CCBot`, `Meta-ExternalAgent`, `DeepSeekBot`, etc.).
- `isMobile(ua: string): bool` — Fast mobile check.
- `matchAiBot(ua: string): BotMatch` | `matchAnyBot(ua: string): BotMatch`

## Minimal Usage

```cp
import { parseUserAgent, isAiBot, BrowserKind, OsKind } from "ua-parser-js";

const info = parseUserAgent(ua_header);
if (info.is_ai_bot || isAiBot(ua_header)) { /* rate-limit AI crawler */ }
if (info.browser == BrowserKind.Chrome && info.os == OsKind.Android) { /* ... */ }
```

## Pitfalls & Rules

1. **Zero-copy slice lifetime**: `browser_version` and `os_version` slice the input `ua` string; keep `ua` alive while reading them.
2. **Use fast predicates on hot paths**: Call `isAiBot(ua)` / `isBot(ua)` / `isMobile(ua)` directly when you do not need full `UserAgent` fields.
3. **Bot precedence**: Bots set `is_bot == true`, `device == DeviceKind.Bot`, and `browser == BrowserKind.Bot`; AI bots also set `is_ai_bot == true`.
4. **No regex / ReDoS-safe**: Safe to run directly on untrusted raw HTTP headers; empty strings cleanly return `Unknown` enums.
