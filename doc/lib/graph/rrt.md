# `cap:rrt` — 零堆分配 2D 采样运动规划库（RRT & RRT*）

> [!NOTE]
> 本子模块已归并至统一主包 `cap:graph`，导入请使用 `import { ... } from "cap:graph"`。

`cap:rrt` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的 2D 采样运动规划引擎，支持标准快速探索随机树（RRT）、渐近最优 RRT*（邻域最优父节点选择 + 树重连 `rewire` + 零分配子树代价传播）以及贪心视线快捷平滑（`smoothPath`）。内置无全局状态 SplitMix64 伪随机数生成器，保证相同 `seed` 的完全确定性复现。

## 核心特性

- **100% 零堆分配**：树节点池（`[]RrtNode`）与输出路径缓冲（`[]Point2`）均由调用方提供，零动态分配。
- **精确碰撞检测原语**：内置点/线段与圆形障碍物、轴对齐矩形障碍物（Liang-Barsky Slab 算法）的精确相交判定。
- **标准 RRT 与渐近最优 RRT***：通过 `cfg.use_rrt_star` 切换；RRT* 自动在 `rewire_radius` 邻域内选择最小代价无碰撞父节点并执行重连与子树代价传播。
- **贪心视线路径平滑（`smoothPath`）**：在保证每段严格无碰撞的前提下尽可能跳过中间冗余拐点，缩短路径总长。
- **完整防御校验**：拦截 `NaN`/非法包围盒、起终点越界或在障碍物内、非法配置及缓冲容量不足。

---

## 导出清单

### 常量与结构体

- `RRT_NO_PARENT: u32 = 0xFFFFFFFF`
- `Point2`：`{ x: f64, y: f64 }`
- `Bounds2`：`{ min_x: f64, min_y: f64, max_x: f64, max_y: f64 }`
- `CircleObstacle`：`{ cx: f64, cy: f64, radius: f64 }`
- `RectObstacle`：`{ min_x: f64, min_y: f64, max_x: f64, max_y: f64 }`
- `RrtNode`：`{ pos: Point2, parent: u32, cost: f64 }`
- `RrtConfig`：`{ step_size, goal_tolerance, goal_bias, rewire_radius, max_iterations, seed, use_rrt_star }`，附 `RrtConfig.defaultRrt()` / `RrtConfig.defaultRrtStar()`
- `RrtResult`：`{ found: bool, node_count: usize, path_len: usize, total_cost: f64, iterations: usize }`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `pointInCircle(p: Point2, c: CircleObstacle): bool` | 点是否在圆内（含边界） |
| `pointInRect(p: Point2, r: RectObstacle): bool` | 点是否在轴对齐矩形内（含边界） |
| `segmentIntersectsCircle(a: Point2, b: Point2, c: CircleObstacle): bool` | 线段与圆相交判定 |
| `segmentIntersectsRect(a: Point2, b: Point2, r: RectObstacle): bool` | 线段与轴对齐矩形相交判定 |
| `isSegmentCollisionFree(a: Point2, b: Point2, circles: []const CircleObstacle, rects: []const RectObstacle): bool` | 线段对全部障碍物是否严格无碰撞 |
| `planRrt2D(start, goal, bounds, circles, rects, cfg, nodes_pool: *[]RrtNode, path_out: *[]Point2): RrtResult?` | 2D RRT / RRT* 路径规划 |
| `smoothPath(path: []const Point2, circles: []const CircleObstacle, rects: []const RectObstacle, out_path: *[]Point2): usize` | 贪心视线快捷平滑 |
| `pathLength(path: []const Point2): f64` | 计算折线欧氏总长度 |

---

## 使用示例

```typescript
import std from "std";
import {
    Point2,
    Bounds2,
    CircleObstacle,
    RectObstacle,
    RrtNode,
    RrtConfig,
    planRrt2D,
    smoothPath,
    pathLength,
} from "cap:graph";

fn rrtDemo() {
    const start = Point2 { x: 0.5, y: 5.0 };
    const goal = Point2 { x: 9.5, y: 5.0 };
    const bounds = Bounds2 { min_x: 0.0, min_y: 0.0, max_x: 10.0, max_y: 10.0 };
    const circles = [CircleObstacle { cx: 5.0, cy: 5.0, radius: 1.8 }];
    const rects: [0]RectObstacle = [];

    let pool_arr: [512]RrtNode = std.mem.zeroes([512]RrtNode);
    let path_arr: [128]Point2 = std.mem.zeroes([128]Point2);
    let smooth_arr: [128]Point2 = std.mem.zeroes([128]Point2);
    let pool: []RrtNode = pool_arr[0..];
    let path_out: []Point2 = path_arr[0..];
    let smooth_out: []Point2 = smooth_arr[0..];

    const res_opt = planRrt2D(
        start,
        goal,
        bounds,
        circles[0..],
        rects[0..],
        RrtConfig.defaultRrtStar(),
        &pool,
        &path_out,
    );
    if (res := res_opt) {
        if (res.found) {
            const s_len = smoothPath(path_arr[0..res.path_len], circles[0..], rects[0..], &smooth_out);
            const len = pathLength(smooth_arr[0..s_len]);
            * = len;
        }
    }
}
```
