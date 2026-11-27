[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# regex-syntax

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation regular expression syntax validator, metacharacter escaper, and literal prefix extractor (Rust regex-syntax v0.8 parity)

## 安装与导入

```bash
cap add regex-syntax
```

```cp
import { isMetaCharacter } from "regex-syntax";
```

## 核心导出 API

- `export fn isMetaCharacter(c: u8): bool;`
- `export fn escapeRegex(input: string, out: []u8): usize;`
- `export fn isWordByte(b: u8): bool;`
- `export fn extractLiteralPrefix(pattern: string, out: []u8): usize;`
- `export fn validateRegexSyntax(pattern: string): RegexSyntaxInfo;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`regex-syntax@1.0.0`)，许可证：MIT OR Apache-2.0
