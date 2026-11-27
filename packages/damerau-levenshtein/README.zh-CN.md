[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# damerau-levenshtein

> 版本：`1.0.8` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Damerau-Levenshtein (Optimal String Alignment) edit distance and similarity ratio

## 安装与导入

```bash
cap add damerau-levenshtein
```

```cp
import { damerauLevenshteinSteps } from "damerau-levenshtein";
```

## 核心导出 API

- `export fn damerauLevenshteinSteps(a: string, b: string): usize;`
- `export fn damerauLevenshtein(a: string, b: string): DamLevResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`damerau-levenshtein@1.0.8`)（https://github.com/tad-lispy/node-damerau-levenshtein），许可证：MIT
