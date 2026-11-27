[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# kmp

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation O(N+M) Knuth-Morris-Pratt (KMP) substring search and LPS failure function engine for CP.

## 安装与导入

```bash
cap add kmp
```

```cp
import { computeLps } from "kmp";
```

## 核心导出 API

- `export fn computeLps(pattern: &[]u8, out_lps: []usize): usize;`
- `export fn findFirst(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn findAll(haystack: &[]u8, needle: &[]u8, overlapping: bool, out_offsets: []usize): usize;`
- `export fn countMatches(haystack: &[]u8, needle: &[]u8, overlapping: bool): usize;`
- `export fn contains(haystack: &[]u8, needle: &[]u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`kmp@1.0.0`)，许可证：MIT
