[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# parry2d

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 2D axis-aligned bounding box (AABB) collision detection, union, intersection, and ray slab intersection for CP

## 安装与导入

```bash
cap add parry2d
```

```cp
import { minF64 } from "parry2d";
```

## 核心导出 API

- `export fn minF64(a: f64, b: f64): f64;`
- `export fn maxF64(a: f64, b: f64): f64;`
- `export fn new(min_x: f64, min_y: f64, max_x: f64, max_y: f64): Aabb2;`
- `export fn fromCenterSize(cx: f64, cy: f64, w: f64, h: f64): Aabb2;`
- `export fn fromPoints(pts: &[]Pt2): Aabb2;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`parry2d@1.0.0`)，许可证：MIT
