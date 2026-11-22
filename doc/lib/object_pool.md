# `cap:object_pool` — 侵入式单链表空闲对象池

> 参考来源：/data/dev/bun/src/collections/pool.cp | 吸收日期：2026-11-22

`cap:object_pool` 提供实例级、侵入式单链表空闲对象池 `ObjectPool<T>`：

- 零额外链表节点开销：空闲节点直接以侵入式单链表串联，`acquire` / `recycle` 均为 **O(1)**；
- 支持上限控制（`max_count > 0` 超限直接销毁释放）与无上限模式（`max_count == 0`）；
- 若池化结构体 `T` 实现 `fn reset(*_)`，复用借出时自动触发状态重置；
- 实现 `~init()` 确定性 RAII 析构，离开作用域自动释放池内所有残留空闲节点，零内存泄漏。

---

## 快速导入

```cp
import { ObjectPool, PoolNode } from "cap:object_pool";
```

---

## API 参考

### 1. `PoolNode<T>`

| 字段 | 类型 | 说明 |
| :--- | :--- | :--- |
| `next` | `(*PoolNode<T>!)?` | 侵入式单链表下一节点指针 |
| `data` | `T` | 托管业务对象数据 |

### 2. `ObjectPool<T>`（含 `~init()` RAII 析构）

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `init` | `static fn init(allocator, max_count: usize): ObjectPool<T>` | 构造指定上限的对象池（`0` 为无上限） |
| `initUnbounded` | `static fn initUnbounded(allocator): ObjectPool<T>` | 构造无上限对象池 |
| `acquire` | `fn acquire(*_): (*PoolNode<T>!)!` | 借出节点（优先复用空闲链头并自动调用 `reset()`，否则新分配） |
| `acquireWith` | `fn acquireWith(*_, initial: T): (*PoolNode<T>!)!` | 借出节点并赋予指定初值 |
| `acquireIfCached` | `fn acquireIfCached(*_): (*PoolNode<T>!)?` | 仅当池中有缓存时借出，不触发堆分配 |
| `recycle` | `fn recycle(*_, node: *PoolNode<T>!)` | 归还节点；超出 `max_count` 时直接释放不缓存 |
| `destroy` | `fn destroy(*_, node: *PoolNode<T>!)` | 直接销毁不再归还的节点 |
| `clear` | `fn clear(*_)` | 立即释放池中所有空闲节点（幂等） |
| `count` / `isFull` / `isEmpty` | `fn ...(): usize / bool` | 池内空闲数、满池与判空状态 |
| `hits` / `allocated` / `freed` / `hitRatePermille` | `fn ...(): usize / u32` | 复用命中数、累计分配/释放数与命中率（‰） |

---

## 使用示例

```cp
import { ObjectPool } from "cap:object_pool";

struct RequestCtx {
    status: u16 = 200,
    bytes_sent: usize = 0,

    fn reset(*_) {
        _.status = 200;
        _.bytes_sent = 0;
    }
}

fn handleRequests(alloc: std.mem.Allocator)! {
    let pool = ObjectPool<RequestCtx>.init(alloc, 16);
    const node = pool.acquire()?;
    unsafe {
        node.data.status = 404;
        node.data.bytes_sent = 512;
    }
    pool.recycle(node);
    // 下次 acquire 自动复用同一节点并触发 reset()
} // 离开作用域自动触发 pool.~init() 全量释放
```

## 上游行为契约与内存安全边界

1. **侵入式 LIFO 缓存局部性（源自 Bun `collections/pool.cp`）**：
   - `ObjectPool<T>` 采用单链表头插/头取（LIFO）复用策略，刚归还的节点最先被再次借出，最大化 CPU L1/L2 缓存热命中率；`acquire` / `recycle` 均为严格 $O(1)$ 无额外元数据分配。
2. **状态复位与防污契约**：
   - 池化类型 `T` 实现 `fn reset(*_)` 时，`acquire` 复用命中会自动重置对象字段，杜绝跨请求敏感数据残留；若需跳过分配仅探测缓存，使用 `acquireIfCached`。
3. **上限背压与 RAII 生命周期**：
   - 生产服务建议设置合理 `max_count`（避免流量尖峰后池内滞留过多空闲内存），超限时 `recycle` 直接调用 `allocator` 释放；离开作用域自动执行 `~init()` 清理池中所有空闲节点，但已借出未归还的节点须由调用方 `recycle` 或 `destroy`。
