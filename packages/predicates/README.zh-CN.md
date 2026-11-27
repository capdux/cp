[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# predicates

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap composable boolean predicates (IntPredicate, StrPredicate, FloatPredicate) for testing and assertions in pure CP

## 安装与导入

```bash
cap add predicates
```

```cp
import { prdEqIgnoreCase } from "predicates";
```

## 核心导出 API

- `export enum IntPredKind`
- `export enum StrPredKind`
- `export enum FloatPredKind`
- `export fn prdEqIgnoreCase(a: string, b: string): bool;`
- `export fn prdIsFinite(x: f64): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`predicates@1.0.0`)，许可证：MIT OR Apache-2.0
