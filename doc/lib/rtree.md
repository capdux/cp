# CP 标准库：纯 CP 2D AABB R-Tree 空间索引 (`cap:rtree`)

## 一、 架构总览与设计准则

`cap:rtree` 是 CP 语言官方内置的纯 CP **二维轴对齐包围盒（2D AABB）动态 R-Tree 空间索引引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。采用节点索引池（`std.ArrayListUnmanaged(RTreeNode)` + `free_slots` 回收槽）管理树节点，支持可配置节点最大容量（`max_entries >= 4`）、最小面积增量选叶（`ChooseLeaf`）、Guttman 二次分裂（`Quadratic Split`）、删除后祖先 MBR 精确收紧，以及基于最小堆的 Best-First 分支界定 $k$-近邻查询（`kNearestPoint`）。

既可通过 `import { ... } from "cap:rtree"` 导入，也支持 `import cap from "cap"; cap.rtree.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/rtree/rtree.cp` | `// Source: cp/lib/rtree/rtree.cp (...)` | `RTreeError`、`Rect`、`RTreeEntry<V>`、`NearestRect<V>`、`RTree<V>` |
| `lib/rtree/index.cp` | `// Source: cp/lib/rtree/index.cp (...)` | `cap:rtree` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与几何/条目类型

- **`RTreeError`**：
  - `InvalidBox`：矩形坐标包含 `NaN`/`Inf` 或满足 `min_x > max_x` / `min_y > max_y`，或点查询坐标非有限浮点数。
  - `InvalidCapacity`：初始化 `RTree` 时传入 `max_entries < 4`。
  - `OutOfMemory`：节点池、分裂缓冲区或查询结果切片内存分配失败。
- **`Rect`**：`{ min_x: f64, min_y: f64, max_x: f64, max_y: f64 }`
  - `Rect.init(min_x, min_y, max_x, max_y): Rect ! RTreeError`
  - `isValid(): bool`
  - `area(): f64`
  - `overlaps(other: Rect): bool`（含边界接触相交）
  - `containsPoint(x: f64, y: f64): bool`
  - `containsRect(other: Rect): bool`
  - `enlarge(other: Rect): Rect`
  - `enlargementCost(other: Rect): f64`
  - `distanceSqToPoint(x: f64, y: f64): f64`
- **`RTreeEntry<V>`**：`{ id: u64, box_rect: Rect, value: V }`
- **`NearestRect<V>`**：`{ entry: RTreeEntry<V>, dist_sq: f64 }`

### 2. `RTree<V>` 核心方法

- **生命周期与容量**：
  - `RTree<V>.init(allocator): RTree<V> ! RTreeError`：使用默认最大容量 `16` 初始化。
  - `RTree<V>.initWithCapacity(allocator, max_entries: usize): RTree<V> ! RTreeError`：要求 `max_entries >= 4`。
  - `deinit(*_)`：释放全部节点池与内部列表内存。
  - `clear(*_)`：清空所有条目并重置根节点，保留内存池容量。
- **插入与删除**：
  - `insert(*_, id: u64, box_rect: Rect, value: V): void ! RTreeError`：`ChooseLeaf` 选择面积增量最小的叶子插入，超容时自底向上触发 `Quadratic Split` 并在必要时分裂根节点。
  - `removeById(*_, id: u64): bool`：删除匹配的条目，自动清理空节点、收缩单子根节点并向上收紧祖先 MBR。
- **空间查询**：
  - `searchOverlap(allocator, query_rect: Rect): []RTreeEntry<V> ! RTreeError`：返回所有与 `query_rect` 相交的条目。调用方需使用 `allocator.free(res)` 释放结果切片。
  - `searchPoint(allocator, x: f64, y: f64): []RTreeEntry<V> ! RTreeError`：返回所有包含点 `(x, y)` 的条目。调用方需使用 `allocator.free(res)` 释放结果切片。
  - `kNearestPoint(allocator, x: f64, y: f64, k: usize): []NearestRect<V> ! RTreeError`：基于最小堆的 Best-First 分支界定剪枝搜索，返回按 `dist_sq` 升序排列的至多 `k` 个最近矩形。调用方需使用 `allocator.free(res)` 释放结果切片。
- **状态查询**：
  - `len(): usize`
  - `isEmpty(): bool`
  - `bounds(): Rect?`

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { Rect, RTree } from "cap:rtree";
import cap from "cap";

pub fn demoRTree(): ! {
    const alloc = std.heap.page_allocator;

    let tree = RTree<u32>.initWithCapacity(alloc, 8)?;
    defer tree.deinit();

    tree.insert(1, Rect.init(0.0, 0.0, 10.0, 10.0)?, 100)?;
    tree.insert(2, Rect.init(5.0, 5.0, 15.0, 15.0)?, 200)?;

    const hits = tree.searchOverlap(alloc, Rect.init(8.0, 8.0, 12.0, 12.0)?)?;
    defer alloc.free(hits);

    const knn = tree.kNearestPoint(alloc, 20.0, 20.0, 1)?;
    defer alloc.free(knn);

    let dyn_tree = cap.rtree.RTree<i32>.init(alloc)?;
    defer dyn_tree.deinit();
    dyn_tree.insert(10, cap.rtree.Rect.init(-2.0, -2.0, 2.0, 2.0)?, 42)?;
}
```
