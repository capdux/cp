[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pathfinding

Zero-allocation weighted shortest-path engine with Dijkstra, Bellman-Ford negative-cycle detection, and path reconstruction (Rust pathfinding / petgraph parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:pathfinding` ([https://github.com/evenfurther/pathfinding](https://github.com/evenfurther/pathfinding))（原库名 `pathfinding` → CP 包名 `pathfinding`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `pathfinding` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pathfinding
```

```cp
import { ShortestPaths, WeightedGraph, emptyShortestPaths } from "pathfinding";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ShortestPaths` | `class` | 字段: `ok: bool, node_count: usize, source: usize, dist: [MAX_NODES]i64, prev: [MAX_NODES]i16` · 方法: `isReachable, distanceTo, reconstructPath` |
| `WeightedGraph` | `class` | 字段: `node_count: usize, edge_count: usize, head: [MAX_NODES]i16, edge_src: [MAX_EDGES]u16, edge_dst: [MAX_EDGES]u16, edge_weight: [MAX_EDGES]i64, +1 项` · 方法: `init, nodeCount, edgeCount, addEdge, addUndirectedEdge, reset, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `emptyShortestPaths` | `(n_nodes: usize, src: usize, ok_val: bool): ShortestPaths` | 调用 `emptyShortestPaths` 并返回 `ShortestPaths`。 |

## 最小调用示例

```cp
import { emptyShortestPaths } from "pathfinding";

pub fn main() {
    let result = emptyShortestPaths(0, 0, false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
