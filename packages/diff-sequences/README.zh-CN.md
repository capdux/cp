[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# diff-sequences

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Longest Common Subsequence (LCS) length, diff insert/delete stats, LCS similarity, and Longest Common Substring locator.

## 安装与导入

```bash
cap add diff-sequences
```

```cp
import { lcsLength } from "diff-sequences";
```

## 核心导出 API

- `export fn lcsLength(a: string, b: string): usize;`
- `export fn diffStats(a: string, b: string): DiffStats;`
- `export fn lcsSimilarity(a: string, b: string): f64;`
- `export fn longestCommonSubstring(a: string, b: string): SubstringMatch;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`diff-sequences@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`lcs-diff@1.0.0`)，许可证：MIT
