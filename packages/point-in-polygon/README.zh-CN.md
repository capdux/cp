[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# point-in-polygon

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 point-in-polygon

## 安装与导入

```bash
cap add point-in-polygon
```

```cp
import { absF64 } from "point-in-polygon";
```

## 核心导出 API

- `export fn absF64(x: f64): f64;`
- `export fn sqrtF64(x: f64): f64;`
- `export fn signedArea(pts: &[]Pt2): f64;`
- `export fn area(pts: &[]Pt2): f64;`
- `export fn isCounterClockwise(pts: &[]Pt2): bool;`
- `export fn perimeter(pts: &[]Pt2): f64;`
- `export fn centroid(pts: &[]Pt2): Pt2;`
- `export fn pointOnSegment(p: Pt2, a: Pt2, b: Pt2): bool;`
- `export fn containsPoint(pts: &[]Pt2, p: Pt2): bool;`
- `export fn isConvex(pts: &[]Pt2): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`point-in-polygon@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`geo-types@1.0.0`)，许可证：MIT
