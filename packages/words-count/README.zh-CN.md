[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# words-count

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation multilingual (Latin + CJK) word/character/line counter and reading-time estimator for CP.

## 安装与导入

```bash
cap add words-count
```

```cp
import { isCjkCodepoint } from "words-count";
```

## 核心导出 API

- `export fn isCjkCodepoint(cp: u32): bool;`
- `export fn analyze(text: string): TextCounts;`
- `export fn countWords(text: string): usize;`
- `export fn estimateReadingSeconds(text: string, words_per_minute: u32): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`words-count@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`word-count@1.0.0`)，许可证：MIT
