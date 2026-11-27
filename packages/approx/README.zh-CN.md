[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# approx

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap IEEE-754 floating-point approximate equality: abs_diff_eq, relative_eq, and ulps_eq for f64 and f32 (clean-room port of Rust approx)

## 安装与导入

```bash
cap add approx
```

```cp
import { apxAbsF64 } from "approx";
```

## 核心导出 API

- `export fn apxAbsF64(x: f64): f64;`
- `export fn apxAbsF32(x: f32): f32;`
- `export fn apxIsNanF64(x: f64): bool;`
- `export fn apxIsNanF32(x: f32): bool;`
- `export fn apxIsInfF64(x: f64): bool;`
- `export fn apxIsInfF32(x: f32): bool;`
- `export fn absDiffEqF64(a: f64, b: f64, epsilon: f64): bool;`
- `export fn absDiffEqF32(a: f32, b: f32, epsilon: f32): bool;`
- `export fn relativeEqF64(a: f64, b: f64, epsilon: f64, max_relative: f64): bool;`
- `export fn relativeEqF32(a: f32, b: f32, epsilon: f32, max_relative: f32): bool;`
- `export fn ulpsDistanceF64(a: f64, b: f64): u64?;`
- `export fn ulpsEqF64(a: f64, b: f64, epsilon: f64, max_ulps: u64): bool;`
- `export fn ulpsDistanceF32(a: f32, b: f32): u32?;`
- `export fn ulpsEqF32(a: f32, b: f32, epsilon: f32, max_ulps: u32): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`approx@1.0.0`)，许可证：Apache-2.0
