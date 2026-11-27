[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# quadtree_rs

Zero-allocation pooled 2D QuadTree spatial index supporting point insertion, automatic subdivision, rectangular range queries, and radius queries for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:quadtree_rs` ([https://github.com/ambuc/quadtree_rs](https://github.com/ambuc/quadtree_rs)) / `npm:d3-quadtree` ([https://github.com/d3/d3-quadtree](https://github.com/d3/d3-quadtree))（原库名 `quadtree_rs` → CP 包名 `quadtree_rs`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `quadtree_rs` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add quadtree_rs
```

```cp
import {
    Rect2,
    QuadItem,
    QuadNode,
    QuadTree,
    qtMakeLeaf,
    qtChildIndex,
} from "quadtree_rs";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Rect2` | `class` | 字段: `min_x: f64, min_y: f64, max_x: f64, max_y: f64` · 方法: `new, contains, intersects` |
| `QuadItem` | `class` | 字段: `x: f64, y: f64, id: u32, next_item: i32` |
| `QuadNode` | `class` | 字段: `bounds: Rect2, first_item: i32, count: u32, children: [4]i32, is_leaf: bool` |
| `QuadTree` | `class` | 字段: `node_count: usize, item_count: usize, max_leaf_items: u32, max_depth: u32` · 方法: `empty, init, insert, queryRect, queryRadius` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `qtMakeLeaf` | `(bounds: Rect2): QuadNode` | 调用 `qtMakeLeaf` 并返回 `QuadNode`。 |
| `qtChildIndex` | `(b: Rect2, x: f64, y: f64): usize` | 调用 `qtChildIndex` 并返回 `usize`。 |

## 最小调用示例

```cp
import { qtMakeLeaf } from "quadtree_rs";

pub fn main() {
    let result = qtMakeLeaf(/* bounds: Rect2 */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
