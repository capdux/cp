[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ordered-float

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap IEEE-754 total ordering, canonical hashing, and NaN-safe min/max/clamp for f64 and f32 (clean-room port of Rust ordered-float)

## 安装与导入

```bash
cap add ordered-float
```

```cp
import { isNanF64 } from "ordered-float";
```

## 核心导出 API

- `export fn isNanF64(x: f64): bool;`
- `export fn isInfiniteF64(x: f64): bool;`
- `export fn isFiniteF64(x: f64): bool;`
- `export fn cmpOrderedF64(a: f64, b: f64): i32;`
- `export fn eqOrderedF64(a: f64, b: f64): bool;`
- `export fn hashOrderedF64(x: f64): u64;`
- `export fn sortOrderedF64(arr: []f64);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ordered-float@1.0.0`)，许可证：MIT
