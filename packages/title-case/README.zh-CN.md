[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# title-case

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation AP/Chicago style English title-case, sentence-case, and capitalization engine in pure CP

## 安装与导入

```bash
cap add title-case
```

```cp
import { isUpper } from "title-case";
```

## 核心导出 API

- `export fn isUpper(b: u8): bool;`
- `export fn isLower(b: u8): bool;`
- `export fn isAlpha(b: u8): bool;`
- `export fn isDigit(b: u8): bool;`
- `export fn toLower(b: u8): u8;`
- `export fn toUpper(b: u8): u8;`
- `export fn eqLower(a: string, lower_target: string): bool;`
- `export fn isSmallWord(word: string): bool;`
- `export fn isWordChar(b: u8): bool;`
- `export fn isAllCapsAcronym(word: string): bool;`
- `export fn hasMoreWords(input: string, start_pos: usize): bool;`
- `export fn toTitleCase(input: string, out: []u8): usize;`
- `export fn toSentenceCase(input: string, out: []u8): usize;`
- `export fn capitalizeFirst(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`title-case@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`titlecase@1.0.0`)，许可证：MIT
