[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# topological-sort

Zero-allocation deterministic Kahn topological sort, cycle detection, and parallel DAG level stratification (Rust topological-sort / petgraph / npm toposort parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:topological-sort` ([https://github.com/gifnksm/topological-sort-rs](https://github.com/gifnksm/topological-sort-rs))（原库名 `topological-sort` → CP 包名 `topological-sort`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `topological-sort` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add topological-sort
```

```cp
import { TopoResult, TopoGraph, clampNodes } from "topological-sort";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TopoResult` | `class` | 字段: `ok: bool, count: usize, order: [MAX_NODES]u16, levels: [MAX_NODES]u16` · 方法: `empty` |
| `TopoGraph` | `class` | 字段: `node_count: usize, edge_count: usize, head: [MAX_NODES]i16, edge_to: [MAX_EDGES]u16, edge_next: [MAX_EDGES]i16, in_deg: [MAX_NODES]u16, +1 项` · 方法: `init, reset, add_edge, in_degree, out_degree, sort, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `clampNodes` | `(n: usize): usize` | 调用 `clampNodes` 并返回 `usize`。 |

## 最小调用示例

```cp
import { clampNodes } from "topological-sort";

pub fn main() {
    let result = clampNodes(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
