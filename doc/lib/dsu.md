# CP 标准库：并查集、带权差分约束并查集与可回滚并查集手册 (`cap:dsu`)

## 一、 架构总览与设计准则

`cap:dsu` 是 CP 语言官方内置的纯 CP **并查集家族（`DisjointSet`、`WeightedDsu`、`RollbackDsu`）** 引擎，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为图连通性判定、变量差分约束求解（2-SAT/势能关系）以及离线分治/可撤销图论算法打造。既可通过 `import { ... } from "cap:dsu"` 导入，也支持 `import cap from "cap"; cap.dsu.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/dsu/dsu.cp` | `// Source: cp/lib/dsu/dsu.cp (...)` | `DsuError`、`DisjointSet`、`WeightedDsu`、`RollbackDsu` |
| `lib/dsu/index.cp` | `// Source: cp/lib/dsu/index.cp (...)` | `cap:dsu` 与 `cap.dsu` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `DsuError`

- `IndexOutOfBounds`：节点编号 `>= n`（含 `n == 0` 空集访问任意下标）。
- `ConflictConstraint`：`WeightedDsu.relate` 检测到新约束与已有连通分量内的差分关系矛盾。
- `NothingToRollback`：`RollbackDsu.undo` 在历史栈为空时调用。
- `OutOfMemory`：底层数组或历史栈/分组切片内存分配失败。

### 2. `DisjointSet`（迭代路径压缩 + 按大小合并）

均摊时间复杂度 $O(\alpha(N))$，采用两遍迭代式路径压缩避免递归栈溢出。

- **`DisjointSet.init(allocator, n): DisjointSet ! DsuError`** / **`deinit(*_)`**
  - 支持 `n == 0` 零堆分配空集。
- **`find(*_, x): usize ! DsuError`**：查找代表元并压缩路径。
- **`unite(*_, a, b): bool ! DsuError`**：合并 `a` 与 `b` 所在集合；若发生合并返回 `true`，已在同一集合返回 `false`。
- **`connected(*_, a, b): bool ! DsuError`**：判断两节点是否连通。
- **`componentSize(*_, x): usize ! DsuError`**：返回 `x` 所在连通分量的大小。
- **`componentCount(): usize`**：返回当前连通分量总数。
- **`groups(*_, allocator): [][]usize ! DsuError`**：将所有元素按连通分量分组返回（调用方需依次释放每个内层切片与外层切片）。
- **`reset(*_)`**：零重新分配将所有节点重置为独立单元素集合。

### 3. `WeightedDsu`（带权/差分约束并查集）

维护势能不变量 `val[x] - val[parent[x]] == weight[x]`，支持带符号传递性推导与矛盾检测。

- **`WeightedDsu.init(allocator, n): WeightedDsu ! DsuError`** / **`deinit(*_)`**
- **`find(*_, x): usize ! DsuError`**：路径压缩并同步累加权值。
- **`relate(*_, a, b, difference: i64): bool ! DsuError`**：添加约束 `val[a] - val[b] == difference`。
  - 若 `a, b` 未连通则合并并返回 `true`。
  - 若已连通且约束一致返回 `false`；若矛盾则返回 `DsuError.ConflictConstraint`。
- **`diff(*_, a, b): i64? ! DsuError`**：若连通返回 `val[a] - val[b]`，不连通返回 `null`。
- **`componentCount(): usize`**：返回当前连通分量总数。

### 4. `RollbackDsu`（可持久化回滚并查集）

采用按大小合并 + 显式历史栈，**不做路径压缩**以保证单次查找 $O(\log N)$ 与单次撤销 $O(1)$ 的严格复杂度。

- **`RollbackDsu.init(allocator, n): RollbackDsu ! DsuError`** / **`deinit(*_)`**
- **`find(x): usize ! DsuError`** / **`connected(a, b): bool ! DsuError`** / **`componentCount(): usize`**
- **`unite(*_, a, b): bool ! DsuError`**：合并并压入撤销记录（若已连通则不压栈并返回 `false`）。
- **`snapshot(): usize`**：捕获当前历史栈深度作为回滚快照令牌。
- **`undo(*_): void ! DsuError`**：撤销最近一次有效合并；栈空时返回 `DsuError.NothingToRollback`。
- **`rollbackTo(*_, snap: usize)`**：将状态精准回滚至 `snap` 时刻。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { DisjointSet, WeightedDsu, RollbackDsu, DsuError } from "cap:dsu";
import cap from "cap";

pub fn demoDsu(): void ! DsuError {
    const alloc = std.heap.page_allocator;

    let dsu = DisjointSet.init(alloc, 5)?;
    defer dsu.deinit();
    * = dsu.unite(0, 1)?;
    * = dsu.unite(1, 2)?;
    const same = dsu.connected(0, 2)?;
    * = same;

    let wdsu = WeightedDsu.init(alloc, 4)?;
    defer wdsu.deinit();
    * = wdsu.relate(0, 1, 10)?;
    * = wdsu.relate(1, 2, -3)?;
    const d02 = wdsu.diff(0, 2)?; // 7
    * = d02;

    let rdsu = cap.dsu.RollbackDsu.init(alloc, 4)?;
    defer rdsu.deinit();
    const snap = rdsu.snapshot();
    * = rdsu.unite(0, 3)?;
    rdsu.rollbackTo(snap);
}
```
