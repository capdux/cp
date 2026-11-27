[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# reading-time

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation bilingual EN/CJK document statistics, reading-time estimator, and LLM token estimator for CP

## 安装与导入

```bash
cap add reading-time
```

```cp
import { decodeUtf8 } from "reading-time";
```

## 核心导出 API

- `export fn decodeUtf8(s: string, pos: usize): DecodedChar;`
- `export fn isCjkChar(cp: u32): bool;`
- `export fn isWordCoreChar(cp: u32): bool;`
- `export fn isDigitByte(c: u8): bool;`
- `export fn isWsCp(cp: u32): bool;`
- `export fn isSentenceEnd(cp: u32): bool;`
- `export fn analyzeTextWithWpm(text: string, en_wpm: usize, cjk_cpm: usize): TextStats;`
- `export fn analyzeText(text: string): TextStats;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn appendUsize(out: []u8, pos: usize, val: usize): usize;`
- `export fn formatReadingTime(stats: TextStats, zh: bool, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`reading-time@1.0.0`)，许可证：MIT
