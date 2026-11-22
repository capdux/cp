# `cap:octree` — 零堆分配 3D 空间八叉树索引库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:graph`，导入请使用 `import { ... } from "cap:graph"`。

`cap:octree` 提供纯 CP 实现、零堆分配（zero-alloc）的 3D 空间索引与几何查询引擎，基于调用方传入的固定容量节点池与侵入式条目池构建，适用于 3D 点云检索、碰撞粗筛、最近邻搜索与光线投射加速。

## 核心特性

- **100% 零堆分配**：节点池（`[]OctreeNode`）、条目池（`[]OctreeEntry`）及迭代遍历栈均在静态/栈缓冲中完成，无递归栈溢出风险。
- **完整 3D 基础几何**：`Vec3`、`Aabb3`（点包含、盒相交、点到盒距离平方、8 分象限编码与子盒切分）、`Ray3` 与 Kay-Kajiya Slab 法 `rayIntersectsAabb`。
- **多级自动分裂**：叶子点数超过 `leaf_capacity` 且未达 `max_depth` 时原地重挂链表分裂为 8 个子象限。
- **高效空间查询**：AABB 范围查询（`queryBox`）、球体范围查询（`querySphere`）、基于最小距离下界剪枝与最近优先排序的最近邻搜索（`nearestPoint`），以及射线穿透查询（`queryRay` / `queryRayLeaves`）。

---

## 导出清单

### 常量

| 名称 | 类型 | 说明 |
| :--- | :--- | :--- |
| `OCTREE_NO_INDEX` | `i32` | 空子节点/链表结束哨兵（`-1`） |
| `OCTREE_DEFAULT_LEAF_CAP` | `usize` | 默认叶子分裂阈值（`8`） |
| `OCTREE_DEFAULT_MAX_DEPTH` | `u8` | 默认最大深度（`8`） |
| `OCTREE_MAX_DEPTH_LIMIT` | `u8` | 最大允许深度上限（`16`） |

### 几何与结果结构

- `Vec3`：`{ x: f64, y: f64, z: f64 }`，方法 `init`, `add`, `sub`, `scale`, `dot`, `distanceSq`
- `Aabb3`：`{ min: Vec3, max: Vec3 }`，方法 `init`, `center`, `containsPoint`, `intersectsAabb`, `distanceSqToPoint`, `octantIndex`, `childAabb`, `expanded`
- `Ray3`：`{ origin: Vec3, dir: Vec3 }`，方法 `init`, `pointAt`
- `RayHitRange`：`{ t_min: f64, t_max: f64 }`
- `rayIntersectsAabb(ray: Ray3, box3: Aabb3, max_t: f64): RayHitRange?`
- `OctreePoint`：`{ pos: Vec3, id: u32 }`，方法 `init(x, y, z, id)`
- `OctreeEntry` / `OctreeNode`：调用方静态缓冲池元素类型
- `NearestResult`：`{ point: OctreePoint, distance_sq: f64 }`

### `Octree` 核心方法

| 方法签名 | 说明 |
| :--- | :--- |
| `Octree.init(bounds: Aabb3, nodes: []OctreeNode, entries: []OctreeEntry, leaf_capacity: usize, max_depth: u8): Octree?` | 初始化零分配八叉树 |
| `insert(*_, point: OctreePoint): bool` | 插入 3D 点并在超阈值时自动分裂；越界或条目池满返回 `false` |
| `queryBox(query: Aabb3, out_ids: *[]u32): usize` | AABB 盒范围查询，返回写入 `out_ids` 的数量 |
| `querySphere(center: Vec3, radius: f64, out_ids: *[]u32): usize` | 球体范围查询，返回写入 `out_ids` 的数量 |
| `nearestPoint(target: Vec3): NearestResult?` | 下界剪枝最近邻搜索；空树返回 `null` |
| `queryRay(ray: Ray3, max_t: f64, point_radius: f64, out_ids: *[]u32): usize` | 查询距离射线段 `[0, max_t]` 不超过 `point_radius` 的点 |
| `queryRayLeaves(ray: Ray3, max_t: f64, out_ids: *[]u32): usize` | 收集所有被射线穿过的叶子节点中的候选点 ID |
| `count(): usize` / `nodeCount(): usize` / `clear(*_)` | 状态查询与清空复用 |

---

## 使用示例

```typescript
import std from "std";
import {
    Vec3,
    Aabb3,
    Ray3,
    OctreePoint,
    OctreeEntry,
    OctreeNode,
    Octree,
    OCTREE_DEFAULT_LEAF_CAP,
    OCTREE_DEFAULT_MAX_DEPTH,
} from "cap:graph";

fn spatialDemo() {
    const world = Aabb3.init(Vec3.init(0.0, 0.0, 0.0), Vec3.init(100.0, 100.0, 100.0)) ?? return;
    let nodes: [256]OctreeNode = std.mem.zeroes([256]OctreeNode);
    let entries: [128]OctreeEntry = std.mem.zeroes([128]OctreeEntry);

    let tree = Octree.init(world, nodes[0..], entries[0..], OCTREE_DEFAULT_LEAF_CAP, OCTREE_DEFAULT_MAX_DEPTH) ?? return;
    * = tree.insert(OctreePoint.init(10.0, 20.0, 30.0, 1));
    * = tree.insert(OctreePoint.init(55.0, 60.0, 65.0, 2));

    let hits: [32]u32 = std.mem.zeroes([32]u32);
    let hits_s: []u32 = hits[0..];
    const n_box = tree.queryBox(Aabb3.init(Vec3.init(0.0, 0.0, 0.0), Vec3.init(30.0, 30.0, 40.0)) ?? world, &hits_s);
    const nearest = tree.nearestPoint(Vec3.init(12.0, 19.0, 31.0));
    const ray = Ray3.init(Vec3.init(0.0, 20.0, 30.0), Vec3.init(1.0, 0.0, 0.0));
    const n_ray = tree.queryRay(ray, 50.0, 1.0, &hits_s);

    * = n_box;
    * = nearest;
    * = n_ray;
}
```
