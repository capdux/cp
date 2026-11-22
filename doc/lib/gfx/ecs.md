# CP 标准库：Sparse-Set 实体组件系统 (`cap:ecs` / `cap.ecs`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:gfx`，导入请使用 `import { ... } from "cap:gfx"`。

## 一、 架构总览与设计准则

`cap:ecs` 是 CP 语言官方内置的纯原生、零 C 依赖的 **Sparse-Set 架构实体组件系统（Entity Component System）**，面向高性能游戏循环、多 Agent 仿真世界与物理引擎场景设计。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，基于代际 `EntityId` 与紧凑双同步数组实现 100% Safe CP 内存管理，支持 `import ... from "cap:gfx"` 与 `import cap from "cap"; cap.ecs.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/ecs/ecs.cp` | `// Source: cp/lib/ecs/ecs.cp (...)` | `EcsError`、`EntityId`、`EntityRegistry`、`ComponentPool<T>` 与 `EcsQuery` 核心实现 |
| `lib/ecs/index.cp` | `// Source: cp/lib/ecs/index.cp (...)` | `cap:ecs` 与 `cap.ecs` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与代际实体标识 (`EcsError`, `EntityId`)

```typescript
pub err EcsError {
    DeadEntity,
    ComponentNotFound,
    OutOfMemory,
}

pub struct EntityId {
    index: u32;
    generation: u32;
}
```

- `EntityId.INVALID`：哨兵无效实体（`index = maxInt(u32), generation = 0`）。
- `eql(other: EntityId): bool` — 校验 `index` 与 `generation` 完全相等。
- `isValid(): bool` — 判断是否非 `INVALID` 哨兵且 `generation != 0`。
- `raw(): u64` / `EntityId.fromRaw(raw_val: u64): EntityId` — 64 位无损打包与解包。

### 2. 实体生命周期管理器 (`EntityRegistry`)

- `EntityRegistry.init(alloc: std.mem.Allocator): EntityRegistry` / `deinit(*_)`
- `spawn(*_): EntityId ! EcsError` — 优先从 `free_list` 复用已回收槽位并沿用递增后的 `generation`，否则分配新槽位（初始 `generation = 1`）。
- `despawn(*_, entity: EntityId): bool` — 校验 `isAlive(entity)` 后递增槽位 `generation`（防 ABA 悬垂引用）并将索引压入 `free_list`；旧 `EntityId` 即刻失效。
- `isAlive(entity: EntityId): bool` — $O(1)$ 校验代际匹配。
- `aliveCount(): usize` / `capacity(): usize`。

### 3. 稀疏-紧凑组件池 (`ComponentPool(comptime T: type)`)

采用经典 Sparse-Set 布局：`sparse[entity.index] -> dense_idx`，配合同步紧凑数组 `dense_entities` 与 `dense_data`：

- `init(alloc: std.mem.Allocator): Self` / `deinit(*_)` / `clear(*_)`
- `set(*_, entity: EntityId, reg: *const EntityRegistry, val: T): void ! EcsError` — 校验 `reg.isAlive(entity)`，否则返回 `EcsError.DeadEntity`；$O(1)$ 插入或覆盖组件。
- `get(*_, entity: EntityId): *T!?` / `getConst(entity: EntityId): *const T!?` — 严格校验 `dense_entities[idx].eql(entity)`，返回紧凑数组元素指针或 `null`。
- `has(entity: EntityId): bool` — $O(1)$ 存在性与代际一致性检查。
- `remove(*_, entity: EntityId): bool` — $O(1)$ Swap-and-Pop 移除组件，并同步更新被交换末尾元素在 `sparse` 中的映射索引。
- `len(): usize` / `isEmpty(): bool`
- `entities(): []const EntityId` / `items(*_): []T` / `itemsConst(): []const T` — 直接返回连续线性切片，实现零指针跳转遍历。

### 4. 多组件交集查询 (`EcsQuery`)

- `EcsQuery.collectJoined2(alloc, comptime A: type, comptime B: type, pool_a: *const ComponentPool(A), pool_b: *const ComponentPool(B)): []EntityId ! EcsError`
  - 自动选择元素数量更少的组件池作为驱动集遍历，并在另一池中执行 $O(1)$ `has(e)` 校验，返回同时拥有 `A` 与 `B` 的 `[]EntityId`（调用方负责 `alloc.free`）。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { EntityRegistry, ComponentPool, EcsQuery } from "cap:gfx";

struct Position { x: f32; y: f32; }
struct Velocity { vx: f32; vy: f32; }

pub fn demoEcs(): ! {
    const alloc = std.heap.page_allocator;
    let reg = EntityRegistry.init(alloc);
    defer reg.deinit();

    let pos_pool = ComponentPool(Position).init(alloc);
    defer pos_pool.deinit();
    let vel_pool = ComponentPool(Velocity).init(alloc);
    defer vel_pool.deinit();

    const e1 = reg.spawn()?;
    pos_pool.set(e1, &reg, Position{ x: 0.0, y: 0.0 })?;
    vel_pool.set(e1, &reg, Velocity{ vx: 1.5, vy: -0.5 })?;

    const joined = EcsQuery.collectJoined2(alloc, Position, Velocity, &pos_pool, &vel_pool)?;
    defer alloc.free(joined);

    for (e of joined) {
        if (p := pos_pool.get(e)) {
            if (v := vel_pool.getConst(e)) {
                p.x += v.vx;
                p.y += v.vy;
            }
        }
    }

    // 动态懒加载挂载等价用法
    let reg2 = cap.ecs.EntityRegistry.init(alloc);
    defer reg2.deinit();
    const e2 = reg2.spawn()?;
    * = reg2.despawn(e2);
}
```
