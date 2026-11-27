[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# astar

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap 2D grid A* shortest pathfinding engine with 4-way Manhattan & 8-way Octile diagonal search and path reconstruction.

## 安装与导入

```bash
cap add astar
```

```cp
import { absDiffUsize } from "astar";
```

## 核心导出 API

- `export fn absDiffUsize(a: usize, b: usize): i64;`
- `export fn heuristic(x1: usize, y1: usize, x2: usize, y2: usize, allow_diagonal: bool): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`astar@1.0.0`)，许可证：MIT
