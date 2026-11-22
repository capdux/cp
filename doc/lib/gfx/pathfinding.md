# CP 标准库：2D 网格寻路与视线平滑引擎手册 (`cap:pathfinding` / `cap.pathfinding`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:gfx`，导入请使用 `import { ... } from "cap:gfx"`。

## 一、 架构总览与设计准则

`cap:pathfinding` 是 CP 语言官方内置的纯原生 2D 网格地图寻路与空间规划引擎，专为机器人导航、游戏 AI 与多 Agent 空间规划打造。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import { ... } from "cap:gfx"` 与 `import cap from "cap"; cap.pathfinding.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/pathfinding/grid.cp` | `// Source: cp/lib/pathfinding/grid.cp (2D Grid Map, A* Search, Dijkstra Distance Field & Line-of-Sight Smoothing)` | `PathError`、`HeuristicKind`、`GridCoord`、`PathResult`、`GridMap` |
| `lib/pathfinding/index.cp` | `// Source: cp/lib/pathfinding/index.cp (Unified Exports for cap:pathfinding)` | `cap:pathfinding` 与 `cap.pathfinding` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与基础类型（`grid.cp`）

- **`PathError`**：`InvalidDimensions`、`OutOfBounds`、`BlockedEndpoint`、`NoPathFound`、`InvalidCost`、`OutOfMemory`
- **`HeuristicKind`**：`Manhattan`、`Octile`、`Euclidean`
- **`GridCoord`**：`x: usize`、`y: usize`，提供 `GridCoord.init(x, y)` 与 `eql(other)`
- **`PathResult`**：`coords: []GridCoord`、`total_cost: f64`、`nodes_expanded: usize`，提供 `deinit(*_)` 释放路径切片

### 2. 网格地图与寻路引擎（`GridMap`）

- **构造与销毁**：
  - `GridMap.init(alloc, width, height): GridMap ! PathError`：初始化无障碍网格（默认地形权重 `1.0`）；宽高为 `0` 或乘积溢出返回 `PathError.InvalidDimensions`。
  - `deinit(*_)`：释放内部障碍表与地形权重缓冲区。
- **地形与障碍管理**：
  - `setBlocked(*_, x, y, blocked): void ! PathError` / `isBlocked(x, y): bool ! PathError`
  - `setCost(*_, x, y, weight: f64): void ! PathError` / `getCost(x, y): f64 ! PathError`：设置/读取单元格通行代价倍率（要求 `weight >= 1.0` 且非无穷大，否则返回 `PathError.InvalidCost`）。
- **视线检测与路径搜索**：
  - `hasLineOfSight(a: GridCoord, b: GridCoord): bool`：基于整数 DDA / 超覆盖 Bresenham 光线投射，经过网格顶点时严格校验两侧正交邻格，严防斜穿障碍物对角。
  - `findPathAStar(alloc, start, goal, allow_diagonal: bool, heuristic: HeuristicKind): PathResult ! PathError`：二叉最小堆驱动的 A* 最短路径搜索。`allow_diagonal = true` 时仅当两个相邻正交格均非障碍才允许对角移动（彻底杜绝穿墙角）；不可达返回 `PathError.NoPathFound`。
  - `computeDistanceField(alloc, goal, allow_diagonal: bool): ([]f64) ! PathError`：基于 Dijkstra 的全图最短代价场，每个单元格存储从该点出发到达 `goal` 的最小代价，不可达或障碍格为 `+inf`。
  - `smoothPath(alloc, path: []const GridCoord): ([]GridCoord) ! PathError`：基于 `hasLineOfSight` 贪心剔除共线及冗余拐点，拉直锯齿路径。

---

## 三、 使用示例

```typescript
import cap from "cap";
import { GridMap, GridCoord, HeuristicKind } from "cap:gfx";

pub fn demoPathfinding(): !void {
    const alloc = std.heap.page_allocator;

    let grid = GridMap.init(alloc, 10, 10)?;
    defer grid.deinit();

    grid.setBlocked(4, 2, true)?;
    grid.setBlocked(4, 3, true)?;
    grid.setCost(3, 4, 8.0)?;

    let res = grid.findPathAStar(
        alloc,
        GridCoord.init(0, 0),
        GridCoord.init(8, 5),
        true,
        HeuristicKind.Octile,
    )?;
    defer res.deinit();

    const smoothed = grid.smoothPath(alloc, res.coords)?;
    defer alloc.free(smoothed);

    // 等价动态挂载用法：cap.pathfinding.GridMap
    const field = cap.pathfinding.GridMap.computeDistanceField(grid, alloc, GridCoord.init(8, 5), true)?;
    defer alloc.free(field);
}
```
