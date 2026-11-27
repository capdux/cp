[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# string-similarity

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Sorensen-Dice bigram string similarity coefficient and best-match finder for CP.

## 安装与导入

```bash
cap add string-similarity
```

```cp
import { diceCoefficient } from "string-similarity";
```

## 核心导出 API

- `export fn diceCoefficient(a: string, b: string): f64;`
- `export fn diceCaseSensitive(a: string, b: string): f64;`
- `export fn bestMatchIndex(target: string, candidates: &[]string, min_rating: f64): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`string-similarity@1.0.0`)，许可证：MIT
