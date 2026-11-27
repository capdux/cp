[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# kdbush

Pure CP zero-allocation static 2D K-D Tree with balanced median-split build, hyperplane-pruned nearest neighbor, rectangle and radius queries (kdbush / kiddo parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:kdbush` ([https://github.com/mourner/kdbush](https://github.com/mourner/kdbush))（原库名 `kdbush` → CP 包名 `kdbush`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `kdbush` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add kdbush
```

```cp
import {
    KdPt,
    KdNode,
    NearestResult,
    KdBuildFrame,
    KdSearchFrame,
    kdCoord,
    kdSwap,
    kdInsertionSort,
} from "kdbush";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `KdPt` | `class` | 字段: `x: f64, y: f64, id: u32` |
| `KdNode` | `class` | 字段: `pt: KdPt, left: i32, right: i32, axis: u8` |
| `NearestResult` | `class` | 字段: `found: bool, id: u32, dist_sq: f64` |
| `KdBuildFrame` | `class` | 字段: `lo: usize, hi: usize, axis: u8, parent: i32, is_left: bool` |
| `KdSearchFrame` | `class` | 字段: `idx: i32, min_dist_sq: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `kdCoord` | `(pt: KdPt, axis: u8): f64` | 调用 `kdCoord` 并返回 `f64`。 |
| `kdSwap` | `(pts: []KdPt, i: usize, j: usize)` | 就地执行 `kdSwap` 操作。 |
| `kdInsertionSort` | `(pts: []KdPt, lo: usize, hi: usize, axis: u8)` | 就地执行 `kdInsertionSort` 操作。 |
| `kdSelectMedian` | `(pts: []KdPt, lo_in: usize, hi_in: usize, target: usize, axis: u8)` | 就地执行 `kdSelectMedian` 操作。 |
| `buildKdTree` | `(pts: []KdPt, nodes: []KdNode): i32` | 调用 `buildKdTree` 并返回 `i32`。 |
| `nearestNeighbor` | `(nodes: []KdNode, root: i32, qx: f64, qy: f64): NearestResult` | 调用 `nearestNeighbor` 并返回 `NearestResult`。 |
| `rangeQueryRect` | `(nodes: []KdNode, root: i32, min_x: f64, min_y: f64, max_x: f64, max_y: f64, out_ids: []u32): usize` | 调用 `rangeQueryRect` 并返回 `usize`。 |
| `radiusQuery` | `(nodes: []KdNode, root: i32, cx: f64, cy: f64, radius: f64, out_ids: []u32): usize` | 调用 `radiusQuery` 并返回 `usize`。 |

## 最小调用示例

```cp
import { kdCoord } from "kdbush";

pub fn main() {
    let result = kdCoord(/* pt: KdPt */, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
