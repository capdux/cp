[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# kdbush

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation static 2D K-D Tree with balanced median-split build, hyperplane-pruned nearest neighbor, rectangle and radius queries (kdbush / kiddo parity)

## 安装与导入

```bash
cap add kdbush
```

```cp
import { buildKdTree } from "kdbush";
```

## 核心导出 API

- `export fn buildKdTree(pts: []KdPt, nodes: []KdNode): i32;`
- `export fn nearestNeighbor(nodes: []KdNode, root: i32, qx: f64, qy: f64): NearestResult;`
- `export fn rangeQueryRect(nodes: []KdNode, root: i32, min_x: f64, min_y: f64, max_x: f64, max_y: f64, out_ids: []u32): usize;`
- `export fn radiusQuery(nodes: []KdNode, root: i32, cx: f64, cy: f64, radius: f64, out_ids: []u32): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`kdbush@1.0.0`)，许可证：MIT
