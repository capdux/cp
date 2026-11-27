[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# jaro-winkler

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Jaro and Jaro-Winkler string similarity metrics and best-match finder in pure CP

## 安装与导入

```bash
cap add jaro-winkler
```

```cp
import { jaro } from "jaro-winkler";
```

## 核心导出 API

- `export fn jaro(a: string, b: string): f64;`
- `export fn jaroWinklerWithScale(a: string, b: string, prefix_scale: f64): f64;`
- `export fn jaroWinkler(a: string, b: string): f64;`
- `export fn bestMatchIndex(query: string, candidates: &[]string, min_score: f64): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`jaro-winkler@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`jaro-winkler@1.0.0`)，许可证：MIT
