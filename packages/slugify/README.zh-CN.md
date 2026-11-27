[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# slugify

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation URL slug generator with UTF-8 Latin-1 / Latin Extended-A diacritic folding and truncation for CP.

## 安装与导入

```bash
cap add slugify
```

```cp
import { slugifyWithSep } from "slugify";
```

## 核心导出 API

- `export fn slugifyWithSep(input: string, sep: u8, lowercase: bool, out: []u8): usize;`
- `export fn slugify(input: string, out: []u8): usize;`
- `export fn slugifyTruncate(input: string, max_len: usize, out: []u8): usize;`
- `export fn isValidSlug(s: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`slugify@1.0.0`)，许可证：MIT
