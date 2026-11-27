[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# splines

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Natural Cubic Spline (Thomas O(N)), Catmull-Rom spline, quadratic/cubic Bezier curves, smoothstep, and linear interpolation.

## 安装与导入

```bash
cap add splines
```

```cp
import { lerp } from "splines";
```

## 核心导出 API

- `export fn lerp(a: f64, b: f64, t: f64): f64;`
- `export fn smoothstep(edge0: f64, edge1: f64, x: f64): f64;`
- `export fn bezierQuad(p0: Vec2, p1: Vec2, p2: Vec2, t: f64): Vec2;`
- `export fn bezierCubic(p0: Vec2, p1: Vec2, p2: Vec2, p3: Vec2, t: f64): Vec2;`
- `export fn catmullRom(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`splines@1.0.0`)，许可证：BSD-3-Clause
