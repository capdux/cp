[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ua-parser-js

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation User-Agent parser with Browser/OS/Device/Engine detection, Search & AI Bot classification (GPTBot/ClaudeBot/PerplexityBot/Bytespider), and CLI runtime identification for CP

## 安装与导入

```bash
cap add ua-parser-js
```

```cp
import { toLowerAscii } from "ua-parser-js";
```

## 核心导出 API

- `export enum DeviceKind`
- `export enum BrowserKind`
- `export enum OsKind`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn findSubstr(haystack: string, needle: string): usize?;`
- `export fn findSubstrCI(haystack: string, needle: string): usize?;`
- `export fn contains(haystack: string, needle: string): bool;`
- `export fn containsCI(haystack: string, needle: string): bool;`
- `export fn isVersionChar(c: u8): bool;`
- `export fn extractVersionAfter(ua: string, prefix: string): string;`
- `export fn extractVersionAfterCI(ua: string, prefix: string): string;`
- `export fn matchAiBot(ua: string): BotMatch;`
- `export fn matchAnyBot(ua: string): BotMatch;`
- `export fn parseOs(ua: string, out: &mut UserAgent);`
- `export fn isGenericCli(ua: string): bool;`
- `export fn parseBrowserAndDevice(ua: string, out: &mut UserAgent);`
- `export fn parseUserAgent(ua: string): UserAgent;`
- `export fn isBot(ua: string): bool;`
- `export fn isAiBot(ua: string): bool;`
- `export fn isMobile(ua: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ua-parser-js@1.0.0`)，许可证：MIT
