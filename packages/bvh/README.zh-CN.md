[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bvh

Zero-allocation static 3D AABB Bounding Volume Hierarchy (BVH) with overlap queries and slab raycasting in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bvh` ([https://github.com/svenstaro/bvh](https://github.com/svenstaro/bvh))

| 元信息 | 值 |
| --- | --- |
| 包名 | `bvh` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bvh
```

```cp
import {
    Aabb,
    BvhItem,
    RayHit,
    BvhNode,
    BuildTask,
    RayFrame,
    Bvh,
    minF64,
} from "bvh";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Aabb` | `class` | 字段: `min_x: f64, min_y: f64, min_z: f64, max_x: f64, max_y: f64, max_z: f64` · 方法: `merge, overlaps, containsPoint, surfaceArea, rayHit` |
| `BvhItem` | `class` | 字段: `bounds: Aabb, id: u32` |
| `RayHit` | `class` | 字段: `id: u32, t: f64, hit: bool` |
| `BvhNode` | `class` | 字段: `bounds: Aabb, left: i32, right: i32, item_id: u32, is_leaf: bool` |
| `BuildTask` | `class` | 字段: `lo: usize, hi: usize, slot: usize` |
| `RayFrame` | `class` | 字段: `idx: i32, t_enter: f64` |
| `Bvh` | `class` | 字段: `nodes: [256]BvhNode, node_count: usize, item_count: usize` · 方法: `build, len, isEmpty, queryAabb, raycast` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `minF64` | `(a: f64, b: f64): f64` | 调用 `minF64` 并返回 `f64`。 |
| `maxF64` | `(a: f64, b: f64): f64` | 调用 `maxF64` 并返回 `f64`。 |
| `centroidOnAxis` | `(b: Aabb, axis: u8): f64` | 调用 `centroidOnAxis` 并返回 `f64`。 |
| `sortItemsByAxis` | `(items: []BvhItem, lo: usize, hi: usize, axis: u8)` | 就地执行 `sortItemsByAxis` 操作。 |

## 最小调用示例

```cp
import { minF64 } from "bvh";

pub fn main() {
    let result = minF64(0.0, 0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
