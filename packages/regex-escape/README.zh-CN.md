[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# regex-escape

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap regular expression literal escaper, character-class escaper, unescaper, and metacharacter detector.

## 安装与导入

```bash
cap add regex-escape
```

```cp
import { isMetaChar } from "regex-escape";
```

## 核心导出 API

- `export fn isMetaChar(c: u8): bool;`
- `export fn isEscapeTarget(c: u8): bool;`
- `export fn needsEscape(input: string): bool;`
- `export fn escapedLen(input: string): usize;`
- `export fn escapeRegex(input: string, out: []u8): usize;`
- `export fn escapeCharClass(input: string, out: []u8): usize;`
- `export fn unescapeRegexLiteral(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`regex-escape@1.0.0`)，许可证：MIT
