[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# ua-parser-js

纯 CP 零堆分配 User-Agent 解析与爬虫识别引擎 —— 洁净室 MIT 实现，对标 npm `ua-parser-js`。

精准区分主流浏览器（Edge `Edg/`、Opera `OPR/`、Samsung Internet、Firefox、Chrome、Safari）、CLI 运行时（`curl`/`Bun`/`Cap`）、操作系统（Windows/macOS/iOS/Android/Linux/HarmonyOS/ChromeOS）与设备类型，并内置搜索爬虫与 AI 爬虫（`GPTBot`、`ClaudeBot`、`PerplexityBot`、`Bytespider`、`DeepSeekBot` 等）极速识别。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源平台 | **npm** |
| 原包名 | `ua-parser-js`（CP 版 `v1.0.0`） |
| 上游仓库 | `https://github.com/faisalman/ua-parser-js` |
| 许可证 | MIT |

从 TypeScript/Node.js 迁移至 CP 时无需改名：

```ts
// TypeScript / Node.js (npm: ua-parser-js)
import { UAParser } from "ua-parser-js";
const result = UAParser(req.headers["user-agent"]);
```

```cp
// CP (cap: ua-parser-js) — 零堆分配栈上 UserAgent 与 AI 爬虫拦截
import { parseUserAgent, isAiBot } from "ua-parser-js";

const ua = parseUserAgent(ua_header);
const block_ai = isAiBot(ua_header);
```

## 安装

```bash
cap add ua-parser-js
```

## 使用示例

### 基础用法：浏览器、操作系统与设备分类

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
        // Windows 桌面端 Edge 浏览器
        // info.browser_version / info.os_version / info.engine_name ("Blink")
    }
    if (info.device == DeviceKind.Mobile) {
        // 返回移动端布局
    }
    _ = info;
}
```

### 边界用法：AI 爬虫识别、CLI 工具与空 UA 字符串

```cp
import { parseUserAgent, isBot, isAiBot, isMobile, BrowserKind, DeviceKind } from "ua-parser-js";

fn demoGuards() {
    // 1. 极速识别 AI 爬虫（GPTBot / ClaudeBot / PerplexityBot / Bytespider / CCBot / DeepSeekBot 等）
    const ai_ua = "Mozilla/5.0 AppleWebKit/537.36 (KHTML, like Gecko); compatible; GPTBot/1.2";
    const ai = isAiBot(ai_ua); // true
    const bot = isBot(ai_ua);  // true

    // 2. 识别命令行客户端（curl / Bun / Cap）
    const cli = parseUserAgent("curl/8.7.1");
    // cli.browser == BrowserKind.Curl, cli.device == DeviceKind.Cli

    // 3. 空串或未知 UA 安全降级为 Unknown，绝不 Panic
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

## 核心 API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `DeviceKind` | `enum { Desktop, Mobile, Tablet, Bot, Cli, Unknown }` | 设备类型枚举 |
| `BrowserKind` | `enum { Chrome, Edge, Firefox, Safari, Opera, Samsung, Curl, Bun, Cap, Bot, Unknown }` | 浏览器/运行时枚举 |
| `OsKind` | `enum { Windows, MacOS, IOS, Android, Linux, HarmonyOS, ChromeOS, Unknown }` | 操作系统枚举 |
| `UserAgent` | `class { browser, browser_name, browser_version, os, os_name, os_version, device, engine_name, is_bot, is_mobile, is_ai_bot, bot_name }` | 栈上零拷贝解析结果结构体 |
| `parseUserAgent` | `fn(ua: string): UserAgent` | 执行完整零分配 UA 解析 |
| `isBot` | `fn(ua: string): bool` | 快速判断是否为搜索引擎、AI 或通用爬虫 |
| `isAiBot` | `fn(ua: string): bool` | 快速判断是否为 AI 采集/检索爬虫 |
| `isMobile` | `fn(ua: string): bool` | 快速判断是否为移动端设备 |
| `matchAiBot` / `matchAnyBot` | `fn(ua: string): BotMatch` | 底层爬虫匹配，返回 `{ matched, is_ai, name, version }` |

## 性能与安全亮点

- **16-lane SIMD 加速**：底层使用 `cap:ascii`（`eqIgnoreCase`、`isDigit`）与 `cap:bin`（`indexOfBytes`）实现无正则的高速子串扫描。
- **零堆分配与零拷贝切片**：`UserAgent` 按值返回在栈上，版本号与名称均直接切片自静态常量或输入 `ua`。
- **天然免疫 ReDoS**：彻底摒弃回溯正则，线性时间扫描，适合高并发网关与边缘节点处理不可信 HTTP `User-Agent` 头。

## 许可证

MIT — 洁净室纯 CP 实现，灵感源自 npm `ua-parser-js`。
