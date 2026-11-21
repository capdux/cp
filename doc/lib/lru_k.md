# `cap:lru_k` — 抗扫描污染 LRU-K 缓存替换引擎

## 概述

`cap:lru_k` 提供工业级抗全表扫描污染的 `LruKCache(K, V)` 缓存替换引擎，支持通过 `import { LruKCache, LruKError } from "cap:lru_k"` 或 `import cap from "cap"`（`cap.lru_k`）使用。

其核心采用经典的 **History Queue（历史访问队列） + Cache Queue（热缓存队列）** 双队列机制：
- 首次写入的键先进入 History Queue；
- 当键累计访问次数达到 `k_threshold`（默认典型值为 `2`，即 LRU-2）时，晋升至 Cache Queue；
- 突发全表扫描的单次访问冷键仅在 History Queue 内部按 LRU 淘汰，不会驱逐 Cache Queue 中的热点数据。

## 核心类型

- **`LruKError`**：
  - `InvalidCapacity`：`capacity == 0` 或 `history_capacity == 0` 时返回。
  - `InvalidK`：`k_threshold == 0` 时返回。
- **`LruKCache(comptime K: type, comptime V: type): type`**：
  - 泛型双队列抗扫描污染缓存结构，内部使用索引链表池与 `std.AutoHashMap` 实现 $O(1)$ 查找、晋升与淘汰。

## API 速查

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `init` | `init(alloc, capacity: usize, history_capacity: usize, k_threshold: usize) -> !Self` | 初始化缓存与历史队列容量及晋升阈值 `K` |
| `deinit` | `deinit(&self)` | 释放内部哈希表与节点池内存 |
| `put` | `put(&self, key: K, value: V) -> !void` | 插入或覆盖更新键值；达到 `K` 次访问自动晋升至缓存队列 |
| `get` | `get(&self, key: K) -> V?` | 查询键值，更新 `hits`/`misses` 统计并推进访问次数与队列位置 |
| `peek` | `peek(self, key: K) -> V?` | 只读查询键值，不改变队列顺序、访问计数与命中统计 |
| `remove` | `remove(&self, key: K) -> V?` | 从缓存或历史队列中移除指定键并返回原值 |
| `clear` | `clear(&self)` | 清空双队列及统计计数器，保留底层容器容量 |
| `len` | `len(self) -> usize` | 返回热缓存队列当前元素数量 |
| `historyLen` | `historyLen(self) -> usize` | 返回历史访问队列当前元素数量 |
| `hits` | `hits(self) -> u64` | 返回 `get` 累计命中次数 |
| `misses` | `misses(self) -> u64` | 返回 `get` 累计未命中次数 |

## 示例

```cp
import { LruKCache } from "cap:lru_k";

let cache = try LruKCache(u32, u64).init(alloc, 128, 256, 2);
defer cache.deinit();

// 热点键访问 2 次后晋升至 Cache Queue
try cache.put(1, 100);
_ = cache.get(1);

// 突发冷扫描键仅在 History Queue 中淘汰，不会驱逐热点键 1
var i: u32 = 1000;
while (i < 2000) {
    try cache.put(i, @as(u64, i));
    i += 1;
}

const hot = cache.peek(1); // 仍为 100
```
