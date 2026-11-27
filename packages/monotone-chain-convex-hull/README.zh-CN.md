[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# monotone-chain-convex-hull

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 monotone-chain-convex-hull

## 安装与导入

```bash
cap add monotone-chain-convex-hull
```

```cp
import { absF64 } from "monotone-chain-convex-hull";
```

## 核心导出 API

- `export fn absF64(x: f64): f64;`
- `export fn sqrtF64(x: f64): f64;`
- `export fn ptLess(a: Pt2, b: Pt2): bool;`
- `export fn ptEqual(a: Pt2, b: Pt2): bool;`
- `export fn crossOAB(o: Pt2, a: Pt2, b: Pt2): f64;`
- `export fn sortPointsXY(pts: []Pt2);`
- `export fn convexHull(work_pts: []Pt2, out_hull: []Pt2): usize;`
- `export fn hullArea(hull: &[]Pt2): f64;`
- `export fn hullPerimeter(hull: &[]Pt2): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`monotone-chain-convex-hull@1.0.0`)，许可证：MIT
