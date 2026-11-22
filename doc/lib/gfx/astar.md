# `cap:astar` — 2D 网格 A* 最短路径与视线检测零分配库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:gfx`，导入请使用 `import { ... } from "cap:gfx"`。

零堆分配（Zero-Alloc）的纯 CP 2D 占据栅格 A* 最短路径搜索与整数 Bresenham 网格视线检测库，支持 4 邻域曼哈顿启发式、8 邻域 Octile 启发式（严格防切角）以及无重复节点溢出的二叉最小堆开放表，适用于游戏 AI 寻路、机器人栅格导航与物流路径规划。

## 核心特性

- **零堆分配**：通过 `AstarWorkspace` 复用调用方预分配定长切片，支持高频寻路零 GC/零堆开销。
- **整数精确代价**：采用 `COST_CARDINAL = 10`（正交）与 `COST_DIAGONAL = 14`（对角 ≈ `10 * sqrt(2)`）全整数运算，杜绝浮点累积误差。
- **8 邻域严格防切角**：对角步进要求目标格及两个相邻正交格同时可通行，防止智能体穿过对角相接的墙缝。
- **原地降键二叉最小堆**：对已入堆节点的更优路径原地更新并上浮，确保开放表容量严格不超过 `width * height`。
- **整数 Bresenham 视线检测**：`hasLineOfSight` 以纯整数运算判断两点连线是否无障碍，可用于路径平滑（Theta* / String Pulling）。

## 导出 API

### 协议常量

- `COST_CARDINAL: u32 = 10`
- `COST_DIAGONAL: u32 = 14`

### 数据模型

- `GridPos { x: i32, y: i32 }`
- `AstarResult { found: bool, path_len: usize, total_cost: u32, nodes_expanded: u32 }`
- `AstarWorkspace { g_score: []u32, parent: []i32, closed: []u8, heap_nodes: []u32, heap_f: []u32 }`

### 核心函数

- `manhattanHeuristic(a: GridPos, b: GridPos): u32`
- `octileHeuristic(a: GridPos, b: GridPos): u32`
- `astarFindPath(grid: []const u8, width: usize, height: usize, start: GridPos, goal: GridPos, allow_diagonal: bool, ws: *AstarWorkspace, out_path: []GridPos): AstarResult?`
  - 返回 `null`：尺寸为 0、切片/工作区长度不足、坐标越界或 `out_path` 容量不足。
  - 返回 `AstarResult { found: false, ... }`：起点/终点为障碍或目标不可达。
- `hasLineOfSight(grid: []const u8, width: usize, height: usize, a: GridPos, b: GridPos): bool`

## 使用示例

```cp
import std from "std";
import {
    GridPos,
    AstarWorkspace,
    astarFindPath,
    hasLineOfSight,
} from "cap:gfx";

fn example() {
    const w: usize = 8;
    const h: usize = 8;
    let grid: [64]u8 = std.mem.zeroes([64]u8);
    let g_buf: [64]u32 = std.mem.zeroes([64]u32);
    let p_buf: [64]i32 = std.mem.zeroes([64]i32);
    let c_buf: [64]u8 = std.mem.zeroes([64]u8);
    let hn_buf: [64]u32 = std.mem.zeroes([64]u32);
    let hf_buf: [64]u32 = std.mem.zeroes([64]u32);
    let path: [64]GridPos = std.mem.zeroes([64]GridPos);

    let ws = AstarWorkspace {
        g_score: g_buf[0..],
        parent: p_buf[0..],
        closed: c_buf[0..],
        heap_nodes: hn_buf[0..],
        heap_f: hf_buf[0..],
    };

    const s = GridPos { x: 0, y: 0 };
    const g = GridPos { x: 7, y: 7 };
    * = hasLineOfSight(grid[0..], w, h, s, g); // true
    * = astarFindPath(grid[0..], w, h, s, g, true, &ws, path[0..]);
}
```
