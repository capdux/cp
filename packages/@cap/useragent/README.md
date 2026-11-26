# `@cap/useragent` — Zero-Allocation User-Agent Parser & Bot Classifier

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ua-parser-js` ([https://github.com/faisalman/ua-parser-js](https://github.com/faisalman/ua-parser-js))（原库名 `ua-parser-js` → CP 包名 `@cap/useragent`）

> Clean-room pure-CP User-Agent parser modeled after npm `ua-parser-js`, `bowser`, and Rust `woothee` | MIT License
>
> 零堆分配纯 CP User-Agent 解析器与爬虫/AI 爬虫识别引擎，对标 npm `ua-parser-js` / `bowser` 与 Rust `woothee`（MIT 协议）。

## Installation / 安装

```bash
cap add @cap/useragent
```

## Features / 核心特性

- **Zero Heap Allocations / 零堆分配**: Stack-allocated `UserAgent` struct with zero-copy slices into static literals or the input UA string
- **Accurate Browser Disambiguation / 精准浏览器识别**: Distinguishes `Edge` (`Edg/`), `Opera` (`OPR/`), `Samsung Internet`, `Firefox`, `Chrome`, `Safari` (excluding Chrome/CriOS), CLI runtimes (`curl`, `Bun`, `Cap`), and Bots
- **OS & Device Classification / 系统与设备分类**: `Windows`, `macOS`, `iOS`, `Android`, `Linux`, `HarmonyOS` / `OpenHarmony`, `ChromeOS`; `Desktop`, `Mobile`, `Tablet`, `Bot`, `Cli`, `Unknown`
- **Dedicated AI Crawler Detection / AI 爬虫识别**: Built-in `isAiBot()` detects `GPTBot`, `ChatGPT-User`, `OAI-SearchBot`, `ClaudeBot`, `PerplexityBot`, `Bytespider`, `Google-Extended`, `CCBot`, `Meta-ExternalAgent`, `DeepSeekBot`, etc.

## Exported API / 导出 API

| Symbol / Signature | Description / 说明 |
| :--- | :--- |
| `DeviceKind` | `Desktop` / `Mobile` / `Tablet` / `Bot` / `Cli` / `Unknown` |
| `BrowserKind` | `Chrome` / `Edge` / `Firefox` / `Safari` / `Opera` / `Samsung` / `Curl` / `Bun` / `Cap` / `Bot` / `Unknown` |
| `OsKind` | `Windows` / `MacOS` / `IOS` / `Android` / `Linux` / `HarmonyOS` / `ChromeOS` / `Unknown` |
| `UserAgent` | Struct with `browser`, `browser_name`, `browser_version`, `os`, `os_name`, `os_version`, `device`, `engine_name`, `is_bot`, `is_mobile`, `is_ai_bot`, `bot_name` |
| `parseUserAgent(ua: string): UserAgent` | Full zero-allocation UA parse |
| `isBot(ua: string): bool` | Fast bot / crawler check |
| `isAiBot(ua: string): bool` | Fast AI crawler check (`GPTBot`, `ClaudeBot`, `PerplexityBot`, `Bytespider`, `Google-Extended`, `CCBot`, ...) |
| `isMobile(ua: string): bool` | Fast mobile phone check |

## Usage Example / 使用示例

```cp
import { parseUserAgent, isAiBot, BrowserKind, OsKind, DeviceKind } from "@cap/useragent";

fn inspectRequest(ua_header: string) {
    const info = parseUserAgent(ua_header);
    if (info.is_ai_bot) {
        // Block or rate-limit AI crawlers (e.g. GPTBot, ClaudeBot)
    }
    if (info.browser == BrowserKind.Edge && info.os == OsKind.Windows) {
        // Desktop Edge on Windows
    }
    if (info.device == DeviceKind.Mobile) {
        // Serve mobile layout
    }
    * = isAiBot(ua_header);
}
```
