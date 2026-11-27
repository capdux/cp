[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# isbn

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ISO 2108 ISBN-10, ISBN-13, ISSN, and EAN-13 validator and converter for CP.

## 安装与导入

```bash
cap add isbn
```

```cp
import { stripHyphens } from "isbn";
```

## 核心导出 API

- `export fn stripHyphens(input: string, out: []u8): usize;`
- `export fn computeIsbn13CheckDigit(prefix12: string): u8;`
- `export fn isValidIsbn10(input: string): bool;`
- `export fn isValidEan13(input: string): bool;`
- `export fn isValidIsbn13(input: string): bool;`
- `export fn isValidIssn(input: string): bool;`
- `export fn isValidIsbn(input: string): bool;`
- `export fn isbn10To13(isbn10: string, out: []u8): usize;`
- `export fn isbn13To10(isbn13: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`isbn@1.0.0`)，许可证：MIT
