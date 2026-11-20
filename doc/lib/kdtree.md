# CP 标准库：多维 K-D Tree 空间索引引擎 (`cap:kdtree` / `cap.kdtree`)

## 一、 架构总览与设计准则

`cap:kdtree` 是 CP 语言官方内置的纯原生、零 C 依赖的 **多维 K-D Tree 空间索引引擎（`KdTree<V>`）**，支持任意正整数维度（`dims >= 1`），面向高维向量检索、空间最近邻匹配、$K$ 近邻（K-NN）、球形半径检索与轴对齐超矩形（AABB）范围查询场景设计。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，基于节点索引池（`std.ArrayListUnmanaged(KdNode<V>)`）与深拷贝坐标缓冲实现 100% Safe CP 内存管理，支持 `import ... from "cap:kdtree"` 与 `import cap from "cap"; cap.kdtree.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/kdtree/kdtree.cp` | `// Source: cp/lib/kdtree/kdtree.cp (...)` | `KdError`、`KdPointEntry<V>`、`KdNeighbor<V>` 与 `KdTree<V>` 平衡建树、动态插入及四类空间检索算法 |
| `lib/kdtree/index.cp` | `// Source: cp/lib/kdtree/index.cp (...)` | `cap:kdtree` 与 `cap.kdtree` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与数据结构 (`KdError`, `KdPointEntry<V>`, `KdNeighbor<V>`)

```typescript
pub err KdError {
    InvalidDimension,
    DimensionMismatch,
    InvalidRadius,
    InvalidBounds,
    EmptyTree,
    OutOfMemory,
}

pub struct KdPointEntry<V> {
    pub coords: []const f64;
    pub value: V;
}

pub struct KdNeighbor<V> {
    pub coords: []const f64;
    pub value: V;
    pub dist_sq: f64;
    pub dist: f64;
}
```

### 2. 多维 K-D Tree 引擎 (`KdTree<V>`)

- **构造与析构**：
  - `KdTree<V>.build(allocator, dims: usize, entries: []const KdPointEntry<V>): KdTree<V> ! KdError`
    - 校验 `dims >= 1`（否则返回 `KdError.InvalidDimension`）及所有点坐标长度等于 `dims`（否则返回 `KdError.DimensionMismatch`）；
    - 内部按各维度方差选取最大方差维度并以中位数划分构建平衡 K-D Tree，深拷贝坐标数据。
  - `KdTree<V>.initEmpty(allocator, dims: usize): KdTree<V> ! KdError`
    - 初始化空多维 K-D Tree，校验 `dims >= 1`。
  - `deinit(*_)` — 释放全部节点与坐标缓冲。
- **动态插入**：
  - `insert(*_, coords: []const f64, value: V): void ! KdError` — 校验 `coords.len == dims()`，深拷贝坐标并按树深度轮转分割维度增量插入。
- **空间检索与状态查询**：
  - `nearest(query: []const f64): KdNeighbor<V>? ! KdError` — 剪枝回溯查找精确最近邻；空树返回 `null`。
  - `kNearest(allocator, query: []const f64, k: usize): []KdNeighbor<V> ! KdError` — 基于有界最大堆剪枝查找最多 `k` 个精确近邻，按 `dist_sq` 升序返回（调用方负责 `allocator.free`）。
  - `withinRadius(allocator, query: []const f64, radius: f64): []KdNeighbor<V> ! KdError` — 球形半径检索（`radius >= 0` 且有限，否则返回 `KdError.InvalidRadius`），按 `dist_sq` 升序返回（调用方负责 `allocator.free`）。
  - `rangeSearch(allocator, min_bounds: []const f64, max_bounds: []const f64): []KdPointEntry<V> ! KdError` — 轴对齐超矩形 AABB 范围检索（校验 `min_bounds[d] <= max_bounds[d]` 且有限，否则返回 `KdError.InvalidBounds`；调用方负责 `allocator.free`）。
  - `len(): usize` — 返回存储的点总数。
  - `dims(): usize` — 返回点维度。
  - `isEmpty(): bool` — 判断树是否为空。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { KdTree, KdPointEntry, KdNeighbor, KdError } from "cap:kdtree";

pub fn demoKdTree(): ! {
    const alloc = std.heap.page_allocator;
    const p1: [3]f64 = [1.0, 2.0, 3.0];
    const p2: [3]f64 = [4.0, 5.0, 6.0];
    const entries: [2]KdPointEntry<u64> = [
        KdPointEntry<u64>{ coords: *p1, value: 10 },
        KdPointEntry<u64>{ coords: *p2, value: 20 },
    ];

    let tree = KdTree<u64>.build(alloc, 3, *entries)?;
    defer tree.deinit();

    const q: [3]f64 = [1.1, 2.1, 2.9];
    const nn = tree.nearest(*q)?;
    if (nn != null) {
        // nn.?.value == 10
    }

    const knn = tree.kNearest(alloc, *q, 2)?;
    defer alloc.free(knn);

    // 动态懒加载挂载等价用法
    let tree2 = cap.kdtree.KdTree<u64>.initEmpty(alloc, 2)?;
    defer tree2.deinit();
    const p3: [2]f64 = [3.0, 4.0];
    tree2.insert(*p3, 99)?;
}
```
