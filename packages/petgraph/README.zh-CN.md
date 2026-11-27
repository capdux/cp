[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# petgraph

Zero-allocation directed/undirected graph and classic graph algorithms (BFS, iterative DFS, Kahn topoSort, cycle detection, Dijkstra) (Rust petgraph v0.6 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:petgraph` ([https://github.com/petgraph/petgraph](https://github.com/petgraph/petgraph))（原库名 `petgraph` → CP 包名 `petgraph`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `petgraph` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add petgraph
```

```cp
import { DiGraph, pgcSatAdd } from "petgraph";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DiGraph` | `class` | 字段: `node_count: usize, edge_count: usize, edge_from: [256]usize, edge_to: [256]usize, edge_weight: [256]u64, head: [64]usize, +4 项` · 方法: `new, nodeCount, edgeCount, addNode, addEdge, addUndirectedEdge, +9 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pgcSatAdd` | `(a: u64, b: u64): u64` | 调用 `pgcSatAdd` 并返回 `u64`。 |

## 最小调用示例

```cp
import { pgcSatAdd } from "petgraph";

pub fn main() {
    let result = pgcSatAdd(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
