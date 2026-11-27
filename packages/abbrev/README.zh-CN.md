[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# abbrev

> 版本：`1.0.0` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap unique unambiguous prefix abbreviation generator and resolver for CP (clean-room ISC implementation of npm abbrev).

## 安装与导入

```bash
cap add abbrev
```

```cp
import { abvStrEq } from "abbrev";
```

## 核心导出 API

- `export fn abvStrEq(a: string, b: string): bool;`
- `export fn abvStrLt(a: string, b: string): bool;`
- `export fn abvHasPrefix(word: string, prefix: string): bool;`
- `export fn abvLcp(a: string, b: string): usize;`
- `export fn abbrev(words: &[]string, out: []AbbrevEntry): usize;`
- `export fn shortestAbbrev(words: &[]string, target: string): string?;`
- `export fn resolveAbbrev(words: &[]string, prefix: string): string?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`abbrev@1.0.0`)，许可证：ISC
