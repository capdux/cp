[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# lerp

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap interpolation, range remapping, angle conversion, and easing curves in pure CP

## 安装与导入

```bash
cap add lerp
```

```cp
import { absF64 } from "lerp";
```

## 核心导出 API

- `export fn absF64(x: f64): f64;`
- `export fn minF64(a: f64, b: f64): f64;`
- `export fn maxF64(a: f64, b: f64): f64;`
- `export fn clamp(x: f64, lo: f64, hi: f64): f64;`
- `export fn clamp01(x: f64): f64;`
- `export fn approxEq(a: f64, b: f64, eps: f64): bool;`
- `export fn lerp(a: f64, b: f64, t: f64): f64;`
- `export fn lerpClamped(a: f64, b: f64, t: f64): f64;`
- `export fn inverseLerp(a: f64, b: f64, value: f64): f64;`
- `export fn remap(in_min: f64, in_max: f64, out_min: f64, out_max: f64, value: f64): f64;`
- `export fn remapClamped(in_min: f64, in_max: f64, out_min: f64, out_max: f64, value: f64): f64;`
- `export fn smoothstep(edge0: f64, edge1: f64, x: f64): f64;`
- `export fn smootherstep(edge0: f64, edge1: f64, x: f64): f64;`
- `export fn step(edge: f64, x: f64): f64;`
- `export fn wrap(x: f64, lo: f64, hi: f64): f64;`
- `export fn pingPong(t: f64, length: f64): f64;`
- `export fn degToRad(deg: f64): f64;`
- `export fn radToDeg(rad: f64): f64;`
- `export fn easeInQuad(t: f64): f64;`
- `export fn easeOutQuad(t: f64): f64;`
- `export fn easeInOutQuad(t: f64): f64;`
- `export fn easeInCubic(t: f64): f64;`
- `export fn easeOutCubic(t: f64): f64;`
- `export fn easeInOutCubic(t: f64): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`lerp@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`eases@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`smoothstep@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`interpolation@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`keyframe@1.0.0`)，许可证：MIT
