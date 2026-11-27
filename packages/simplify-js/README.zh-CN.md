[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# simplify-js

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Ramer-Douglas-Peucker 2D polyline simplification for GPS tracks and vector graphics in pure CP

## 安装与导入

```bash
cap add simplify-js
```

```cp
import { pointToSegmentDistSq } from "simplify-js";
```

## 核心导出 API

- `export fn pointToSegmentDistSq(p: Pt2, a: Pt2, b: Pt2): f64;`
- `export fn simplifyPolyline(pts: []Pt2, epsilon: f64, keep_mask: []u8, out: []Pt2): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`simplify-js@1.0.0`)，许可证：MIT
