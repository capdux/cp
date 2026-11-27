[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# kiddo

Zero-allocation static 2D KD-Tree spatial index with exact nearest-neighbor and radius range queries in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:kiddo` ([https://github.com/sdd/kiddo](https://github.com/sdd/kiddo))（原库名 `kiddo` → CP 包名 `kiddo`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `kiddo` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add kiddo
```

```cp
import {
    KdPoint2D,
    NearestResult,
    KdNode,
    BuildTask,
    SearchFrame,
    KdTree2D,
    coordAt,
    sortRangeByAxis,
} from "kiddo";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `KdPoint2D` | `class` | 字段: `x: f64, y: f64, id: u32` |
| `NearestResult` | `class` | 字段: `id: u32, dist_sq: f64, found: bool` |
| `KdNode` | `class` | 字段: `x: f64, y: f64, id: u32, axis: u8, left: i32, right: i32` |
| `BuildTask` | `class` | 字段: `lo: usize, hi: usize, depth: usize, slot: usize` |
| `SearchFrame` | `class` | 字段: `idx: i32, plane_dist_sq: f64` |
| `KdTree2D` | `class` | 字段: `nodes: [256]KdNode, node_count: usize` · 方法: `build, len, isEmpty, nearest, withinRadius` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `coordAt` | `(p: KdPoint2D, axis: u8): f64` | 调用 `coordAt` 并返回 `f64`。 |
| `sortRangeByAxis` | `(pts: []KdPoint2D, lo: usize, hi: usize, axis: u8)` | 就地执行 `sortRangeByAxis` 操作。 |

## 最小调用示例

```cp
import { coordAt } from "kiddo";

pub fn main() {
    let result = coordAt(/* p: KdPoint2D */, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
