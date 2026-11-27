[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# didyoumean

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation spell-suggestion & edit-distance engine (didyoumean / fastest-levenshtein / Rust strsim parity: Levenshtein, Damerau-Levenshtein, Jaro-Winkler & CLI command suggestion) for CP

## 安装与导入

```bash
cap add didyoumean
```

```cp
import { levenshtein } from "didyoumean";
```

## 核心导出 API

- `export fn levenshtein(a: string, b: string): usize;`
- `export fn damerauLevenshtein(a: string, b: string, ignore_case: bool): usize;`
- `export fn similarity(a: string, b: string): f64;`
- `export fn didYouMeanWith(input: string, candidates: []string, max_dist: usize): Suggestion;`
- `export fn didYouMean(input: string, candidates: []string): string?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`didyoumean@1.0.0`)，许可证：Apache-2.0
