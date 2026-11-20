# CP 标准库：O(1) 权重感知与 TTL 过期 LRU 缓存 (`cap:lru` / `cap.lru`)

## 一、 架构总览与设计准则

`cap:lru` 是 CP 语言官方内置的纯原生 $O(1)$ LRU 内存缓存库，结合 `std.StringHashMapUnmanaged(usize)` 与连续槽位池上的侵入式双向链表，支持：

1. **严格 $O(1)$ 操作**：查询 (`get` / `peek` / `contains`)、插入/更新 (`put` / `putWeighted`)、删除 (`removeEntry`) 与容量/权重淘汰均在 $O(1)$ 均摊时间内完成。
2. **双维度容量约束**：同时支持最大条目数 `max_entries` 与字节权重上限 `max_weight`（`max_weight == 0` 表示不限制总权重），单条目权重超限时拦截返回 `LruError.EntryTooLarge`。
3. **确定性单调 TTL 过期**：所有过期逻辑通过显式注入 `now_ms: i64` 驱动，零 `std.posix` 与系统时钟依赖，支持 `get` 惰性过期淘汰与 `purgeExpired` 主动扫描清理。
4. **所有权安全键拷贝**：内部对传入 `key: []const u8` 执行深拷贝并由缓存托管生命周期，调用方临时缓冲区覆写或释放绝不导致悬垂。
5. **动态扩缩容 (`resize`)**：支持运行时调整 `max_entries` 与 `max_weight`，缩容时自动按 LRU 顺序淘汰旧条目。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/lru/lru.cp` | `// Source: cp/lib/lru/lru.cp (O(1) LRU Cache with TTL Expiration, Byte-Weight Limit & Hit Stats)` | `LruError`、`LruStats`、`LruCache<V>` 核心实现 |
| `lib/lru/index.cp` | `// Source: cp/lib/lru/index.cp (Unified Exports for cap:lru)` | `cap:lru` 与 `cap.lru` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. `LruError`

- `InvalidCapacity`：`max_entries == 0` 时返回
- `InvalidWeight`：`putWeighted` 传入 `weight == 0` 时返回
- `EntryTooLarge`：当 `max_weight > 0` 且单条目 `weight > max_weight` 时返回
- `OutOfMemory`：底层内存分配失败

### 2. `LruStats`

- 字段：
  - `len: usize`：当前条目数
  - `capacity: usize`：最大条目容量
  - `current_weight: usize`：当前所有条目权重之和
  - `max_weight: usize`：权重上限（`0` 表示不限）
  - `hits: u64`：命中次数
  - `misses: u64`：未命中次数（含惰性过期）
  - `evictions: u64`：因容量或权重上限触发的淘汰次数
  - `expirations: u64`：因 TTL 到期触发的过期清除次数
- 方法：
  - `hitRate(): f64`：命中率 `[0.0, 1.0]`，无查询时返回 `0.0`

### 3. `LruCache<V>`

- **生命周期与扩缩容**：
  - `init(allocator, max_entries, max_weight): LruCache<V>!`
  - `resize(*_, max_entries, max_weight): !`
  - `clear(*_)`
  - `deinit(*_)`
- **写入与更新**：
  - `put(*_, key, value): !`（默认权重 `1`，无 TTL）
  - `putWeighted(*_, key, value, weight, ttl_ms, now_ms): !`（`ttl_ms <= 0` 表示永不过期）
- **查询、探测与删除**：
  - `get(*_, key, now_ms): V?`（命中晋升至 MRU 并计入 `hits`；过期则惰性删除并计入 `expirations` 与 `misses`）
  - `peek(key, now_ms): V?`（只读探测，不改 LRU 顺序、不计入 `hits`/`misses`）
  - `contains(key, now_ms): bool`（只读存在性与未过期检查）
  - `removeEntry(*_, key): V?`（按键删除并返回原值）
  - `purgeExpired(*_, now_ms): usize`（主动清理所有在 `now_ms` 已过期的条目）
- **观测指标**：
  - `len(): usize`、`capacity(): usize`、`currentWeight(): usize`、`maxWeight(): usize`、`isEmpty(): bool`、`stats(): LruStats`

---

## 三、 使用示例

```typescript
import cap from "cap";
import { LruCache, LruStats, LruError } from "cap:lru";

pub fn demoLru(): !void {
    const alloc = std.heap.page_allocator;

    // 最多 100 条目，总权重上限 1024 字节
    let cache = LruCache<i32>.init(alloc, 100, 1024)?;
    defer cache.deinit();

    // 写入带权重和 TTL（5000ms）的条目
    cache.putWeighted("prompt:1", 42, 256, 5000, 1000)?;
    const hit = cache.get("prompt:1", 2000); // Some(42)
    * = hit;

    // 等价动态挂载用法
    let cache2 = cap.lru.LruCache<i32>.init(alloc, 16, 0)?;
    defer cache2.deinit();
    cache2.put("k1", 7)?;
}
```
