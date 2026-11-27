[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bezier-easing

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation quadratic/cubic Bezier curves, de Casteljau splitting, arc length, and CSS cubic-bezier easing for CP

## 安装与导入

```bash
cap add bezier-easing
```

```cp
import { sqrtF64 } from "bezier-easing";
```

## 核心导出 API

- `export fn sqrtF64(x: f64): f64;`
- `export fn absF64(x: f64): f64;`
- `export fn lerpPt2(a: Pt2, b: Pt2, t: f64): Pt2;`
- `export fn distPt2(a: Pt2, b: Pt2): f64;`
- `export fn splitCubic(c: CubicBezier, t: f64): CubicSplit;`
- `export fn evalUnitCubic(p1: f64, p2: f64, t: f64): f64;`
- `export fn derivUnitCubic(p1: f64, p2: f64, t: f64): f64;`
- `export fn cubicBezierEasing(x1: f64, y1: f64, x2: f64, y2: f64, x: f64): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`bezier-easing@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`kurbo@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`lyon_geom@1.0.0`)，许可证：MIT
