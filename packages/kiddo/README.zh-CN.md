[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# kiddo

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation static 2D KD-Tree spatial index with exact nearest-neighbor and radius range queries in pure CP

## 安装与导入

```bash
cap add kiddo
```

```cp
import { coordAt } from "kiddo";
```

## 核心导出 API

- `export fn coordAt(p: KdPoint2D, axis: u8): f64;`
- `export fn sortRangeByAxis(pts: []KdPoint2D, lo: usize, hi: usize, axis: u8);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`kiddo@1.0.0`)，许可证：MIT
