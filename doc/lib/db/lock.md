# CP 标准库：逻辑读写锁、死锁检测与租约协调引擎手册 (`cap:lock`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:db`，导入请使用 `import { ... } from "cap:db"`。

## 一、 架构总览与设计准则

`cap:lock` 是 CP 语言官方内置的纯 CP 逻辑协调与并发控制标准库，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为多 Agent 工具互斥访问、状态分区并发控制与分布式租约协调打造，既可通过 `import { ... } from "cap:db"` 导入，也支持 `import cap from "cap"; cap.lock.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│             CP 逻辑读写锁、等待图死锁检测与租约协调引擎 (cap:lock)        │
├──────────────────────────────────┬──────────────────────────────────────┤
│ 等待图与环路检测 (wait_graph.cp)  │ 读写锁表与租约管理器 (lock.cp)        │
│ WaitForGraph (3色迭代DFS判环)     │ NamedLockTable (RW/重入/AB-BA拦截)   │
│                                  │ LeaseLockManager (单调Fencing Token)  │
└──────────────────────────────────┴──────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/lock/wait_graph.cp` | `// Source: cp/lib/lock/wait_graph.cp (Directed Wait-For Graph & Cycle Detector for Deadlock Detection)` | `WaitForGraph`（有向等待边维护、3 色迭代 DFS 环路检测） |
| `lib/lock/lock.cp` | `// Source: cp/lib/lock/lock.cp (Named RW Lock Table with Wait-For Deadlock Detection & Fencing-Token Lease Manager)` | `LockError`、`LockMode`、`NamedLockTable`、`LeaseToken`、`LeaseLockManager` |
| `lib/lock/index.cp` | `// Source: cp/lib/lock/index.cp (Unified Exports for cap:lock)` | `cap:lock` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与锁模式（`lock.cp`）

- **`LockError`**：
  - `LockBusy`：资源已被其他持有者以冲突模式或未过期租约占用。
  - `NotOwner`：非当前持有者尝试调用 `unlock`、`renewLease` 或 `releaseLease`。
  - `DeadlockDetected`：`tryAcquire` 阻塞并登记等待边后在 `WaitForGraph` 中形成死锁环路，触发即时回滚与拦截。
  - `LeaseExpired`：租约已过期、已被释放或 `fencing_token` 已失效。
  - `InvalidTtl`：`ttl_ms` 为 `0`。
  - `OutOfMemory`：底层内存分配失败。
- **`LockMode`**：`Shared`（共享读锁）、`Exclusive`（排他写锁）。

### 2. 有向等待图 `WaitForGraph`（`wait_graph.cp`）

- `init(alloc): WaitForGraph` / `deinit()`
- `addEdge(waiter, holder): !`：登记 `waiter -> holder` 有向等待边（幂等去重）。
- `removeEdge(waiter, holder)` / `clearWaiter(waiter)`：移除指定等待边或清空 `waiter` 的全部出边。
- `hasCycle(alloc): bool !`：基于显式栈的 3 色迭代 DFS 判环，支持自环与多跳环路。
- `nodeCount(): usize` / `edgeCount(): usize`。

### 3. 逻辑读写锁表 `NamedLockTable`（`lock.cp`）

- `init(alloc): NamedLockTable` / `deinit()`
- `tryAcquire(resource, owner, mode): bool ! LockError`：
  - 支持多读者 `Shared` 并发兼容、同 `owner` 重入（含唯一读者升级为 `Exclusive`）、写者 `Exclusive` 排他互斥；
  - 若被其他持有者阻塞，自动在内部 `WaitForGraph` 登记 `owner -> blocker`；若形成环路，立即回滚本次等待边并返回 `LockError.DeadlockDetected`；否则返回 `false`。
- `unlock(resource, owner): void ! LockError`：递减重入计数并在归零时释放锁，同时清除 `owner` 对 `resource` 的等待与阻塞记录；非持有者调用返回 `LockError.NotOwner`。
- `holderCount(resource): usize` / `isResourceLocked(resource): bool` / `detectCycle(alloc): bool ! LockError`。

### 4. 单调 Fencing Token TTL 租约管理器 `LeaseLockManager`（`lock.cp`）

- **`LeaseToken`**：`resource: string`、`owner: string`、`fencing_token: u64`、`expires_at_ms: u64`。
- `init(alloc): LeaseLockManager` / `deinit()`
- `acquireLease(resource, owner, ttl_ms, now_ms): LeaseToken ! LockError`：
  - 当资源空闲、前任租约已过期（`now_ms >= expires_at_ms`）或同一 `owner` 重入时，允许获取/抢占并递增全局单调 `fencing_token`；
  - 若仍在有效期内且属于其他 `owner`，返回 `LockError.LockBusy`。
- `renewLease(resource, owner, fencing_token, ttl_ms, now_ms): LeaseToken ! LockError`：校验租约未过期且 `owner` 与 `fencing_token` 严格匹配，防止过时客户端续约。
- `releaseLease(resource, owner, fencing_token): void ! LockError`：校验属主与令牌匹配后释放租约（属主不匹配返回 `NotOwner`，已过期或令牌不匹配返回 `LeaseExpired`）。
- `validateToken(resource, fencing_token, now_ms): bool`：供下游存储层校验写请求是否持有当前有效最新令牌，彻底杜绝脑裂旧主写入。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { NamedLockTable, LeaseLockManager, LockMode } from "cap:db";
import cap from "cap";

pub fn demoLock(): ! {
    const alloc = std.heap.page_allocator;

    let table = NamedLockTable.init(alloc);
    defer table.deinit();

    * = table.tryAcquire("tool:git", "agent-a", .Shared)?;
    * = table.tryAcquire("tool:git", "agent-b", .Shared)?;
    table.unlock("tool:git", "agent-a")?;
    table.unlock("tool:git", "agent-b")?;

    let mgr = cap.lock.LeaseLockManager.init(alloc);
    defer mgr.deinit();

    const tok = mgr.acquireLease("partition:0", "worker-1", 5000, 1000)?;
    const valid = mgr.validateToken("partition:0", tok.fencing_token, 3000);
    * = valid;
}
```
