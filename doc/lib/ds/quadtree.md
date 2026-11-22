# CP 标准库：动态分裂/合并四叉树空间索引引擎 (`cap:quadtree` / `cap.quadtree`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 一、 架构总览与设计准则

`cap:quadtree` 是 CP 语言官方内置的纯原生、零 C 依赖的 **2D 动态分裂/合并四叉树空间索引引擎（`QuadTree`）**，面向 2D 游戏 AOI（Area of Interest）、地图 POI 检索、粒子碰撞加速与最近邻空间查询场景设计。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，基于索引池（`std.ArrayList(QuadNode)` + `free_slots`）实现 100% Safe CP 内存管理，支持 `import ... from "cap:ds"` 与 `import cap from "cap"; cap.quadtree.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/quadtree/quadtree.cp` | `// Source: cp/lib/quadtree/quadtree.cp (...)` | `QuadError`、`QuadEntry`、`QuadHit` 与 `QuadTree` 核心索引、分裂/合并与检索算法 |
| `lib/quadtree/index.cp` | `// Source: cp/lib/quadtree/index.cp (...)` | `cap:quadtree` 与 `cap.quadtree` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与数据结构 (`QuadError`, `QuadEntry`, `QuadHit`)

```typescript
pub err QuadError {
    OutOfBounds,
    InvalidBounds,
    InvalidConfig,
    OutOfMemory,
}

pub struct QuadEntry {
    id: u64;
    x: f64;
    y: f64;
}

pub struct QuadHit {
    id: u64;
    x: f64;
    y: f64;
    dist_sq: f64;
}
```

### 2. 四叉树引擎 (`QuadTree`)

- **构造与析构**：
  - `QuadTree.init(alloc, min_x, min_y, max_x, max_y, bucket_capacity, max_depth): QuadTree ! QuadError`
    - 校验 `min_x < max_x`、`min_y < max_y`（有限浮点数），否则返回 `QuadError.InvalidBounds`；
    - 校验 `bucket_capacity >= 1`，否则返回 `QuadError.InvalidConfig`。
  - `deinit(*_)` — 释放全部节点与条目存储。
- **动态插入、删除与更新**：
  - `insert(*_, id: u64, x: f64, y: f64): void ! QuadError` — 超出边界或非有限坐标返回 `OutOfBounds`；叶子条目数超过 `bucket_capacity` 且深度小于 `max_depth` 时自动四分裂下推；所有点重合时自动抑制无限分裂。
  - `remove(*_, id: u64, x: f64, y: f64): bool` — 删除匹配条目，并在四子叶总条目数回落至 `<= bucket_capacity` 时自动向上合并回收节点。
  - `update(*_, id: u64, old_x: f64, old_y: f64, new_x: f64, new_y: f64): bool ! QuadError` — 原子校验新坐标边界后迁移条目位置。
- **空间检索与状态查询**：
  - `queryRect(alloc, min_x, min_y, max_x, max_y): []QuadEntry ! QuadError` — 矩形区域检索（调用方负责 `alloc.free`）。
  - `queryCircle(alloc, cx, cy, radius): []QuadHit ! QuadError` — 圆形半径检索，返回带 `dist_sq` 的命中条目（调用方负责 `alloc.free`）。
  - `nearestK(alloc, qx, qy, k): []QuadHit ! QuadError` — 基于最小堆包围盒下界剪枝的 Best-First $K$ 近邻检索，按 `dist_sq` 升序（平局按 `id` 升序）返回最多 `k` 个结果（调用方负责 `alloc.free`）。
  - `len(): usize` — 返回当前索引条目总数。
  - `nodeCount(): usize` — 返回当前活跃四叉树节点总数。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { QuadTree, QuadEntry, QuadHit } from "cap:ds";

pub fn demoQuadTree(): ! {
    const alloc = std.heap.page_allocator;
    let qt = QuadTree.init(alloc, 0.0, 0.0, 1000.0, 1000.0, 8, 12)?;
    defer qt.deinit();

    qt.insert(1, 100.0, 200.0)?;
    qt.insert(2, 105.0, 205.0)?;

    const rect_hits = qt.queryRect(alloc, 90.0, 190.0, 120.0, 220.0)?;
    defer alloc.free(rect_hits);

    const knn = qt.nearestK(alloc, 102.0, 201.0, 1)?;
    defer alloc.free(knn);

    // 动态懒加载挂载等价用法
    let qt2 = cap.quadtree.QuadTree.init(alloc, -100.0, -100.0, 100.0, 100.0, 4, 8)?;
    defer qt2.deinit();
    qt2.insert(42, 10.0, 20.0)?;
}
```
