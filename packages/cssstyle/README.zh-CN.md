[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cssstyle

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CSS inline style declaration parser, query, mutator, and serializer (npm cssstyle parity) in pure CP

## 安装与导入

```bash
cap add cssstyle
```

```cp
import { cstToLower } from "cssstyle";
```

## 核心导出 API

- `export fn cstToLower(c: u8): u8;`
- `export fn cstIsSpace(c: u8): bool;`
- `export fn cstEqIgnoreCase(a: string, b: string): bool;`
- `export fn cstTrim(s: string): string;`
- `export fn cstSkipSpaceAndComments(s: string, pos: usize): usize;`
- `export fn cstStripTrailingComments(s: string): string;`
- `export fn cstExtractImportant(raw: string): CstParsedValue;`
- `export fn cstWriteBytes(out: []u8, pos: usize, src: string, lowercase: bool): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cssstyle@1.0.0`)，许可证：MIT
