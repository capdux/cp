# CP 标准库：通用有界资源与连接池引擎手册 (`cap:pool`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:time`，导入请使用 `import { ... } from "cap:time"`。

## 一、 架构总览与设计准则

`cap:pool` 是 CP 语言官方内置的纯 CP **通用有界资源与连接池引擎（`ResourcePool`）**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为数据库连接、HTTP 长连接客户端、gRPC 会话与 AI 推理沙盒实例提供工业级确定性资源复用能力。既可通过 `import { ... } from "cap:time"` 导入，也支持 `import cap from "cap"; cap.pool.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/pool/pool.cp` | `// Source: cp/lib/pool/pool.cp (...)` | `PoolError`、`BorrowOrder`、`PoolConfig`、`PooledHandle`、`PoolStats`、`ResourcePool` |
| `lib/pool/index.cp` | `// Source: cp/lib/pool/index.cp (...)` | `cap:pool` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集、借出策略与配置

- **`PoolError`**：
  - `PoolExhausted`：活跃与空闲资源总数已达 `max_size`，无法继续借出或新建。
  - `InvalidConfig`：配置非法（如 `max_size == 0` 或 `min_idle > max_size`）。
  - `CreateFailed`：外部 `CreateFn` 创建底层资源失败。
  - `PoolClosed`：池已调用 `deinit()` 关闭后继续操作。
  - `InvalidHandle`：归还或丢弃了无效、越界或已归还的 `PooledHandle`。
  - `OutOfMemory`：初始化底层槽位数组分配失败。
- **`BorrowOrder`**：
  - `Lifo`：后进先出（优先复用最近归还的“热”连接）。
  - `Fifo`：先进先出（按顺序轮转复用空闲连接，均衡连接活跃度）。
- **`PoolConfig`**：
  - `min_idle: usize = 0`：最小空闲预热与保底数量。
  - `max_size: usize = 10`：池容量硬上限。
  - `idle_timeout_ms: u64 = 0`：空闲超时驱逐阈值（毫秒，`0` 表示不限制）。
  - `max_lifetime_ms: u64 = 0`：单资源最大存活期（毫秒，`0` 表示不限制）。
  - `borrow_order: BorrowOrder = .Lifo`：空闲队列借出策略。

### 2. 回调签名、句柄与可观测统计

- **`CreateFn`**：`*const fn (*anyopaque, std.mem.Allocator) *anyopaque ! PoolError`
- **`DestroyFn`**：`*const fn (*anyopaque, *anyopaque, std.mem.Allocator) void`
- **`ValidateFn`**：`*const fn (*anyopaque, *anyopaque) bool`
- **`PooledHandle`**：`slot_id: usize`, `ptr: *anyopaque`, `created_at_ms: u64`, `borrow_count: u64`
- **`PoolStats`**：`total_created`, `total_destroyed`, `active_count`, `idle_count`, `exhausted_rejects`, `evicted_idle`, `evicted_lifetime`, `failed_validations`

### 3. `ResourcePool` 核心方法

- **`init(alloc, cfg, user_ctx, create_fn, destroy_fn, validate_fn?): ResourcePool ! PoolError`**：校验配置并预分配槽位表与空闲环形队列。
- **`deinit()`**：销毁所有剩余空闲与活跃资源（保证 `total_created == total_destroyed` 零泄漏）并释放内部缓冲区。
- **`warmup(now_ms: u64): void ! PoolError`**：按 `cfg.min_idle` 预热创建空闲资源。
- **`acquire(now_ms: u64): PooledHandle ! PoolError`**：按 `Lifo`/`Fifo` 从空闲队列取用；自动剔除超时或 `validate_fn` 失败的失效资源；无空闲且未达 `max_size` 时新建；满载返回 `PoolError.PoolExhausted`。
- **`checkin(handle: PooledHandle, now_ms: u64): void ! PoolError`**：归还借出句柄至空闲队列；若已达 `max_lifetime_ms` 则直接销毁。
- **`discard(handle: PooledHandle): void ! PoolError`**：标记损坏并立即销毁底层资源，释放槽位配额。
- **`evictExpired(now_ms: u64): usize ! PoolError`**：主动扫描清理超期空闲资源，并在之后自动补足至 `min_idle` 下限。
- **`stats(): PoolStats`**：返回实时运行时统计快照。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { ResourcePool, PoolConfig, BorrowOrder, PoolError } from "cap:time";
import cap from "cap";

pub fn demoPool(ctx_ptr: *anyopaque, create_cb: cap.pool.CreateFn, destroy_cb: cap.pool.DestroyFn): ! {
    const alloc = std.heap.page_allocator;
    let pool = ResourcePool.init(
        alloc,
        PoolConfig{
            min_idle: 2,
            max_size: 4,
            idle_timeout_ms: 30000,
            max_lifetime_ms: 300000,
            borrow_order: BorrowOrder.Lifo,
        },
        ctx_ptr,
        create_cb,
        destroy_cb,
        null,
    )?;
    defer pool.deinit();

    pool.warmup(1000)?;
    const handle = pool.acquire(1010)?;
    pool.checkin(handle, 1020)?;
}
```
