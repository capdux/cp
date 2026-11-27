[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# b-spline

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 b-spline

## 安装与导入

```bash
cap add b-spline
```

```cp
import { hermite1D } from "b-spline";
```

## 核心导出 API

- `export fn hermite1D(p0: f64, m0: f64, p1: f64, m1: f64, t: f64): f64;`
- `export fn hermite2D(p0: Pt2, m0: Pt2, p1: Pt2, m1: Pt2, t: f64): Pt2;`
- `export fn catmullRom1D(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64;`
- `export fn catmullRom2D(p0: Pt2, p1: Pt2, p2: Pt2, p3: Pt2, t: f64): Pt2;`
- `export fn uniformCubicBSpline1D(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64;`
- `export fn uniformCubicBSpline2D(p0: Pt2, p1: Pt2, p2: Pt2, p3: Pt2, t: f64): Pt2;`
- `export fn evalCatmullRomChain(pts: &[]Pt2, u: f64): Pt2;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`b-spline@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`bspline@1.0.0`)，许可证：MIT
