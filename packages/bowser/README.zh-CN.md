[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bowser

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation User-Agent parser for browser, OS, engine, and platform detection (npm bowser parity) in pure CP

## 安装与导入

```bash
cap add bowser
```

```cp
import { bwsIsVersionChar } from "bowser";
```

## 核心导出 API

- `export enum PlatformType`
- `export fn bwsIsVersionChar(c: u8): bool;`
- `export fn bwsFindSubstrCI(haystack: string, needle: string): usize?;`
- `export fn bwsContains(haystack: string, needle: string): bool;`
- `export fn bwsContainsCI(haystack: string, needle: string): bool;`
- `export fn bwsExtractVersionAt(ua: string, start: usize): string;`
- `export fn bwsExtractAfter(ua: string, token: string): string;`
- `export fn bwsDetectBrowser(ua: string): BwsPair;`
- `export fn bwsDetectOS(ua: string): BwsPair;`
- `export fn bwsDetectEngine(ua: string): string;`
- `export fn bwsDetectPlatform(ua: string, os_name: string): PlatformType;`
- `export fn parseUserAgent(ua: string): ParsedUA;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`bowser@1.0.0`)，许可证：MIT
