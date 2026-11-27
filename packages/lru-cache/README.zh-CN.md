[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# lru-cache

> **源包溯源**：Clean-Room MIT 纯 CP 实现，对标 [`npm:lru-cache`](https://github.com/isaacs/node-lru-cache)（ISC/MIT）| CP 包版本：`1.0.0` | 零外部依赖

零堆分配、确定性 O(1) 工业级 LRU 缓存库，内置侵入式双向链表、开放寻址 FNV-1a 64 位哈希索引、纳秒级 TTL 过期、字节/自定义权重淘汰与运行时命中率统计。

## 零成本迁移对照（TypeScript / JS → CP）

直接沿用原包名 `"lru-cache"`，无需改名即可完成迁移：

| TypeScript / Node.js (`npm:lru-cache`) | CP (`lru-cache`) |
| :--- | :--- |
| `import { LRUCache } from "lru-cache";` | `import { LruCache, LruConfig } from "lru-cache";` |
| `new LRUCache({ max: 128, ttl: 5000 })` | `LruCache.init(LruConfig{ max_entries: 128, default_ttl_ns: 5_000_000_000 })` |
| `cache.set(k, v)` | `cache.put(k, v, now_ns)` |
| `cache.get(k)` / `cache.peek(k)` | `cache.get(k, now_ns)` / `cache.peek(k, now_ns)` |
| `cache.delete(k)` / `cache.purgeStale()` | `cache.remove(k)` / `cache.purgeExpired(now_ns)` |

## 核心 API

### 导出常量

- `MAX_CAPACITY: usize = 256`（最大槽位池容量）
- `HASH_BUCKETS: usize = 512`（开放寻址哈希桶数量，负载因子 ≤ 0.5）
- `MAX_KEY_LEN: usize = 64`（单条 Key 最大字节数）
- `MAX_VAL_LEN: usize = 256`（单条 Value 最大字节数）

### 类型与方法

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `LruConfig { max_entries, max_weight, default_ttl_ns }` | 初始化配置（`max_weight = 0` 不限权重，`default_ttl_ns = 0` 永不过期）。 |
| `LruStats { hits, misses, evictions, expirations, len, total_weight }` | 统计快照，提供 `hitRate(): f64`。 |
| `LruCache.init(cfg: LruConfig): LruCache` | 初始化缓存（`max_entries` 自动上限裁剪为 `MAX_CAPACITY`）。 |
| `put(*_, key: &[]u8, val: &[]u8, now_ns: u64): bool` | 以默认 TTL 和自动字节权重（`key.len + val.len`）写入或更新。 |
| `putWithOptions(*_, key, val, ttl_ns: u64, weight: u64, now_ns: u64): bool` | 指定自定义 TTL 与权重（`0` 表示自动字节权重）写入或更新。 |
| `get(*_, key: &[]u8, now_ns: u64): (&[]u8)?` | 查询并提升至 MRU，过期则惰性驱逐并更新统计。 |
| `peek(key: &[]u8, now_ns: u64): (&[]u8)?` | 只读查询（过滤过期项），不改变 LRU 顺序与命中统计。 |
| `contains(key: &[]u8, now_ns: u64): bool` | 检查键是否存在且未过期（不改变顺序）。 |
| `remove(*_, key: &[]u8): bool` / `popLru(*_): bool` | 按键删除 / 主动淘汰最久未使用的尾部条目。 |
| `purgeExpired(*_, now_ns: u64): usize` | 批量清理 `now_ns` 时刻已过期条目，返回清理数。 |
| `clear(*_)` / `resetStats(*_)` | 清空缓存条目 / 重置累计统计计数器。 |
| `len()` / `isEmpty()` / `capacity()` / `totalWeight()` / `stats()` | 查询当前条目数、判空、容量、累计权重与统计快照。 |
| `mostRecentKey(): (&[]u8)?` / `leastRecentKey(): (&[]u8)?` | 查看 MRU 头部键或 LRU 尾部键。 |

## 实战示例

### 基础容量淘汰与 `peek` / `get` 顺序控制

```cp
import { LruCache, LruConfig } from "lru-cache";

fn demoBasicLru() {
    let cache = LruCache.init(LruConfig{ max_entries: 3 });

    * = cache.put("a", "alpha", 0);
    * = cache.put("b", "beta", 0);
    * = cache.put("c", "gamma", 0);

    // peek 不改变顺序，"a" 仍位于 LRU 尾部
    const p = cache.peek("a", 0);
    * = p;

    // get 命中并将 "a" 提升至 MRU，"b" 变为最旧条目
    const v = cache.get("a", 0);
    * = v;

    // 插入 "d" 自动淘汰 "b"
    * = cache.put("d", "delta", 0);
    // cache.contains("b", 0) == false
    // cache.mostRecentKey().? == "d"
    // cache.leastRecentKey().? == "c"
}
```

### TTL 过期与权重驱逐

```cp
import { LruCache, LruConfig } from "lru-cache";

fn demoTtlAndWeight() {
    const SEC: u64 = 1_000_000_000;
    let cache = LruCache.init(LruConfig{
        max_entries: 64,
        max_weight: 100,
        default_ttl_ns: 5 * SEC,
    });

    const t0: u64 = 100 * SEC;
    * = cache.putWithOptions("k1", "payload_1", 2 * SEC, 40, t0);
    * = cache.putWithOptions("k2", "payload_2", 5 * SEC, 40, t0);
    // 总权重超过 100，自动驱逐最旧的 "k1"
    * = cache.putWithOptions("k3", "payload_3", 5 * SEC, 50, t0);

    const purged = cache.purgeExpired(t0 + 6 * SEC);
    const st = cache.stats();
    const rate = st.hitRate();
    * = purged;
    * = rate;
}
```

## 性能与安全亮点

- **`undefined` 槽位池零初始化**：`slots: [256]LruEntry = undefined` 配合高水位指针 `allocated` 与空闲链表 `free_head`，`init()` 无需清零 ~90 KB 槽位数组。
- **严格 O(1) 侵入式链表 + 反向桶指针**：每个 `LruEntry` 内联 `prev`/`next` 与 `bucket` 索引，淘汰与删除省去二次哈希探测，全程零堆分配。
- **Tombstone 复用与自动 Rehash**：512 桶开放寻址 FNV-1a 哈希表在 75% 占用率（384）时自动轻量重建，避免高频增删导致探测退化。
- **内联所有权与饱和 TTL**：键值拷贝入定长缓冲区（`≤64 B` / `≤256 B`）消除悬空切片；过期时间使用饱和加法 `now_ns +| ttl_ns` 防止 `u64` 溢出回绕。

## 许可协议

MIT
