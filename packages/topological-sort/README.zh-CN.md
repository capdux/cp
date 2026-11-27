[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# topological-sort

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation deterministic Kahn topological sort, cycle detection, and parallel DAG level stratification (Rust topological-sort / petgraph / npm toposort parity) for CP

## 安装与导入

```bash
cap add topological-sort
```

```cp
import { clampNodes } from "topological-sort";
```

## 核心导出 API

- `export fn clampNodes(n: usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`topological-sort@1.0.0`)，许可证：MIT
