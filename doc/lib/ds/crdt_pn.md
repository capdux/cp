# `cap:crdt_pn` — State-Based CRDTs

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 1. 概述

`cap:crdt_pn` 提供纯 CP 实现的工业级分布式状态型无冲突复制数据类型（State-Based CRDTs / CvRDT）。所有类型的 `merge` 操作均严格满足半格（Join-Semilattice）数学性质——**幂等性（Idempotency）**、**交换律（Commutativity）**与**结合律（Associativity）**，可在多副本并发写入、网络分区与消息乱序重传场景下保证最终一致性（Strong Eventual Consistency）。支持 `import { ... } from "cap:ds"` 与 `cap.crdt_pn` 动态挂载。

## 2. 核心类型

- `GCounter`：只增计数器，按 `node_id` 独立累加，`merge` 取各节点最大值。
- `PNCounter`：正负计数器，内部由 `pos: GCounter` 与 `neg: GCounter` 组成，支持并发增减。
- `LWWRegister(comptime T: type)`：最后写入胜出寄存器，按 `(ts, node_id)` 全序决胜。
- `GSet(comptime T: type)`：只增集合，支持去重添加与并集收敛合并。

## 3. API 速查

| 类型 | 方法 | 说明 |
| :--- | :--- | :--- |
| `GCounter` | `init(alloc)` / `deinit(&self)` | 初始化与释放内存 |
| `GCounter` | `increment(&self, node_id: u64, delta: u64)` | 指定节点增加计数 |
| `GCounter` | `value(self): u64` / `merge(&self, other)` | 求和估值与半格合并 |
| `PNCounter` | `init(alloc)` / `deinit(&self)` | 初始化与释放内存 |
| `PNCounter` | `increment` / `decrement` | 指定节点增/减计数 |
| `PNCounter` | `value(self): i64` / `merge(&self, other)` | 计算净值与双向合并 |
| `LWWRegister(T)` | `init(val, ts, node_id)` / `get(self): T` | 构造与读取当前值 |
| `LWWRegister(T)` | `set(&self, val, ts, node_id)` / `merge(&self, other)` | 按 `(ts, node_id)` 更新与合并 |
| `GSet(T)` | `init(alloc)` / `deinit(&self)` | 初始化与释放集合 |
| `GSet(T)` | `add(&self, item)` / `contains(self, item)` / `len(self)` | 去重插入、查询与基数 |
| `GSet(T)` | `merge(&self, other)` | 集合并集收敛 |

## 4. 示例

```typescript
import { GCounter, PNCounter, LWWRegister, GSet } from "cap:ds";
import cap from "cap";

let c = GCounter.init(alloc);
defer c.deinit();
try c.increment(1, 10);

let reg = cap.crdt_pn.LWWRegister(i64).init(100, 1, 1);
reg.set(200, 2, 1);
```
