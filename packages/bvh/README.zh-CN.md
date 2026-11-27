[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bvh

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation static 3D AABB Bounding Volume Hierarchy (BVH) with overlap queries and slab raycasting in pure CP

## 安装与导入

```bash
cap add bvh
```

```cp
import { minF64 } from "bvh";
```

## 核心导出 API

- `export fn minF64(a: f64, b: f64): f64;`
- `export fn maxF64(a: f64, b: f64): f64;`
- `export fn centroidOnAxis(b: Aabb, axis: u8): f64;`
- `export fn sortItemsByAxis(items: []BvhItem, lo: usize, hi: usize, axis: u8);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bvh@1.0.0`)，许可证：MIT
