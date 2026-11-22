# `cap:slab_pool` — 工业级固定块 Slab 对象池

> [!NOTE]
> 本子模块已归并至统一主包 `cap:db`，导入请使用 `import { ... } from "cap:db"`。

## 概述

`cap:slab_pool` 提供基于固定容量连续槽位数组与侵入式空闲链表（Intrusive Free List）的 O(1) 对象池 `SlabPool(T)`。每个槽位携带 32 位代际计数器（`generation`），通过 `Handle { index, generation }` 防止悬垂句柄与 ABA 误访问，并在对象释放后实现零内存碎片的原位槽位复用。

## 核心类型

- **`SlabError`**:
  - `InvalidCapacity`: 容量为 `0` 或超出 `u32` 索引上限。
  - `PoolFull`: 对象池已满，无法继续分配。
  - `InvalidHandle`: 句柄索引越界或代际为 `0`。
  - `StaleHandle`: 槽位已被释放或已复用为新代际，拦截悬垂/ABA 访问。
  - `OutOfMemory`: 初始化分配底层槽位数组失败。
- **`Handle`**:
  - `index: u32`: 槽位下标。
  - `generation: u32`: 槽位代际号（从 `1` 起跳，回绕时跳过 `0`）。
  - `eql(self, other: Handle): bool`: 判断两个句柄是否完全一致。
- **`SlabPool(comptime T: type)`**: 固定容量泛型 Slab 对象池。

## API 速查

| 方法 | 说明 | 复杂度 |
| :--- | :--- | :--- |
| `init(alloc, cap_limit)` | 初始化固定容量 Slab 对象池 | O(N) |
| `deinit(*self)` | 释放底层槽位内存 | O(1) |
| `insert(*self, val: T)` | 分配空闲槽位并写入值，返回 `SlabError!Handle` | O(1) |
| `get(*self, h: Handle)` | 校验句柄并返回可变指针 `SlabError!*T` | O(1) |
| `getConst(self, h: Handle)` | 校验句柄并返回只读值拷贝 `SlabError!T` | O(1) |
| `remove(*self, h: Handle)` | 回收槽位、递增代际并压入空闲链表，返回原值 | O(1) |
| `isValid(self, h: Handle)` | 检查句柄当前是否仍然有效 | O(1) |
| `len(self)` / `capacity(self)` | 查询当前存活对象数与总容量 | O(1) |
| `clear(*self)` / `reset(*self)` | 回收全部存活对象、递增其代际并重建空闲链表 | O(N) |
| `forEachActive(*self, ctx, cb)` | 遍历所有存活槽位并调用 `cb(ctx, handle, *T)` | O(N) |

## 示例

```cp
import { SlabPool, Handle, SlabError } from "cap:db";

let pool = SlabPool(u64).init(allocator, 16)?;
defer pool.deinit();

const h1 = pool.insert(42)?;
const ptr = pool.get(h1)?;
ptr.* = 99;

const removed = pool.remove(h1)?;
// h1 已失效，再次访问将返回 SlabError.StaleHandle
```
