[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fastest-levenshtein

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Levenshtein edit distance, OSA Damerau-Levenshtein transposition distance, normalized similarity, and closest-match lookup.

## 安装与导入

```bash
cap add fastest-levenshtein
```

```cp
import { absDiff } from "fastest-levenshtein";
```

## 核心导出 API

- `export fn absDiff(a: usize, b: usize): usize;`
- `export fn min3(a: usize, b: usize, c: usize): usize;`
- `export fn distance(a: string, b: string): usize;`
- `export fn damerauDistance(a: string, b: string): usize;`
- `export fn similarity(a: string, b: string): f64;`
- `export fn withinDistance(a: string, b: string, max_dist: usize): bool;`
- `export fn closestIndex(query: string, candidates: &[]string, max_dist: usize): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`fastest-levenshtein@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`levenshtein@1.0.0`)，许可证：MIT
