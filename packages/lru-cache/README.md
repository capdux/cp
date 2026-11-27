# `@cap/lru` — 零堆分配 O(1) 工业级 LRU 缓存

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lru-cache` ([https://github.com/isaacs/node-lru-cache](https://github.com/isaacs/node-lru-cache)) / `npm:lru-cache` ([https://github.com/isaacs/node-lru-cache](https://github.com/isaacs/node-lru-cache))（原库名 `lru-cache` → CP 包名 `@cap/lru`）

> 独立 Clean-Room 纯 CP 实现，严格对标 npm `lru-cache`（周下载 150M+）与 Rust `lru` crate | 零外部依赖 | 许可协议：MIT

## 概述

`@cap/lru` 是一款面向高性能系统与低延迟服务端场景打造的纯 CP、零堆分配、确定性 O(1) LRU 缓存库，支持条目容量上限、字节/自定义权重上限、纳秒级 TTL 过期以及完备的运行时可观测性指标。

## 架构与 CP 特有性能优化亮点

1. **`undefined` 栈槽位池零初始化开销**：
   - 底层采用固定容量内联槽位池 `slots: [256]LruEntry = undefined` 搭配高水位 bump 指针（`allocated`）与侵入式空闲链表（`free_head`），初始化时无需清零整个 ~90KB 槽位数组，启动耗时恒定极低。
2. **侵入式双向链表 O(1) 零堆分配**：
   - 每个槽位内联 `prev`/`next`（`u16` 索引），`promote`、`evict`、`remove` 与 `popLru` 全部严格 O(1) 原地指针重连，全程零动态内存分配、零 GC 抖动。
3. **开放寻址 FNV-1a 64-bit 哈希索引 + 反向桶指针**：
   - 512 槽位（负载因子 ≤ 0.5）开放寻址线性探测表，支持 tombstone 删除复用与 75% 高水位自动轻量 `rehash`，彻底杜绝高频删改后的探测退化；
   - 每个 `LruEntry` 反向缓存自身所在桶索引 `bucket`，使得驱逐尾部与删除节点时连哈希重探测都省去，实现极致 O(1)。
4. **安全内联所有权存储**：
   - 内联定长缓冲区（`MAX_KEY_LEN = 64`、`MAX_VAL_LEN = 256`）拷贝持有键值字节，彻底避免外部切片失效导致的悬空引用问题。
5. **饱和算术防溢出**：
   - TTL 到期时间采用 `now_ns +| ttl_ns` 饱和加法计算，免疫时间戳接近 `u64` 上限时的整数回绕风险。

## 复杂度分析

| 操作 | 时间复杂度 | 堆分配 | 说明 |
| :--- | :---: | :---: | :--- |
| `init` | O(1) | 0 | 仅初始化 1KB 哈希桶表，`slots` 零清零 |
| `put` / `putWithOptions` | 均摊 O(1) | 0 | 自动处理容量与权重淘汰，必要时触发轻量 rehash |
| `get` | O(1) | 0 | 命中提升至 MRU 头部，过期则惰性驱逐 |
| `peek` / `contains` | O(1) | 0 | 纯只读查询，不改变 LRU 顺序与命中统计 |
| `remove` / `popLru` | O(1) | 0 | 反向桶索引直接标记 tombstone 并回收槽位 |
| `purgeExpired` | O(n) | 0 | 仅遍历当前活跃条目链表清理过期项 |
| `mostRecentKey` / `leastRecentKey` | O(1) | 0 | 直接读取链表头/尾槽位 |

## 导出常量与 API

### 常量

| 常量 | 值 | 说明 |
| :--- | :--- | :--- |
| `MAX_CAPACITY` | `256` | 最大支持槽位容量 |
| `HASH_BUCKETS` | `512` | 开放寻址哈希桶总数 |
| `MAX_KEY_LEN` | `64` | 单条 Key 最大字节长度 |
| `MAX_VAL_LEN` | `256` | 单条 Value 最大字节长度 |

### 结构体与方法

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `LruConfig { max_entries, max_weight, default_ttl_ns }` | 缓存初始化配置（`max_weight = 0` 不限权重，`default_ttl_ns = 0` 永不过期） |
| `LruStats { hits, misses, evictions, expirations, len, total_weight }` | 运行时统计快照，提供 `hitRate(): f64` 命中率计算 |
| `LruCache.init(cfg: LruConfig): LruCache` | 创建并初始化指定配置的 LRU 缓存实例 |
| `put(*_, key: []const u8, val: []const u8, now_ns: u64): bool` | 使用默认 TTL 与自动字节权重（`key.len + val.len`）写入或更新条目 |
| `putWithOptions(*_, key, val, ttl_ns: u64, weight: u64, now_ns: u64): bool` | 使用自定义 TTL（0=不过期）与权重（0=自动字节权重）写入或更新条目 |
| `get(*_, key: []const u8, now_ns: u64): ([]const u8)?` | 查询并提升至 MRU，更新 `hits`/`misses`；过期则惰性驱逐并计 `expirations` |
| `peek(key: []const u8, now_ns: u64): ([]const u8)?` | 只读查询（过滤过期项），不改变 LRU 顺序也不更新统计 |
| `contains(key: []const u8, now_ns: u64): bool` | 判断键是否存在且未过期（不改变 LRU 顺序） |
| `remove(*_, key: []const u8): bool` | 按键删除条目并释放权重与槽位 |
| `popLru(*_): bool` | 主动淘汰最久未使用（LRU 尾部）条目并计 `evictions` |
| `purgeExpired(*_, now_ns: u64): usize` | 批量清理所有在 `now_ns` 时刻已过期的条目，返回清理数量 |
| `clear(*_)` / `resetStats(*_)` | 清空全部缓存条目（保留统计） / 重置累计统计计数器 |
| `len(): usize` / `isEmpty(): bool` / `capacity(): usize` / `totalWeight(): u64` | 查询当前条目数、判空、容量上限与累计权重 |
| `stats(): LruStats` | 获取当前统计指标快照 |
| `mostRecentKey(): ([]const u8)?` / `leastRecentKey(): ([]const u8)?` | 查看 MRU（最近使用）与 LRU（最久未使用）条目的键 |

## 使用示例

### 示例 1：基础容量 LRU 缓存与 `peek` / `get` 顺序控制

```cp
import { LruCache, LruConfig } from "@cap/lru";

fn demoBasicLru() {
    let cache = LruCache.init(LruConfig{ max_entries: 3 });

    * = cache.put("a", "alpha", 0);
    * = cache.put("b", "beta", 0);
    * = cache.put("c", "gamma", 0);

    // peek 不改变顺序，此时 LRU 仍是 "a"
    const p = cache.peek("a", 0);
    * = p;

    // get 命中并将 "a" 提升到 MRU，此时 LRU 变为 "b"
    const v = cache.get("a", 0);
    * = v;

    // 插入第 4 条 "d"，自动淘汰最久未使用的 "b"
    * = cache.put("d", "delta", 0);
    // cache.contains("b", 0) == false
    // cache.mostRecentKey().? == "d"
    // cache.leastRecentKey().? == "c"
}
```

### 示例 2：基于 TTL 过期与 `purgeExpired` 批量清理

```cp
import { LruCache, LruConfig } from "@cap/lru";

fn demoTtlCache() {
    const SEC: u64 = 1_000_000_000;
    let cache = LruCache.init(LruConfig{
        max_entries: 64,
        default_ttl_ns: 5 * SEC,
    });

    const t0: u64 = 100 * SEC;
    * = cache.put("session:1", "user_alice", t0);
    // 单条自定义更短的 2 秒 TTL
    * = cache.putWithOptions("otp:99", "482910", 2 * SEC, 0, t0);

    // 3 秒后："otp:99" 惰性过期返回 null，"session:1" 仍有效
    * = cache.get("otp:99", t0 + 3 * SEC);
    * = cache.get("session:1", t0 + 3 * SEC);

    // 6 秒后批量清理全部过期项
    const purged = cache.purgeExpired(t0 + 6 * SEC);
    * = purged;
}
```

### 示例 3：按字节权重（`max_weight`）驱逐与命中率统计

```cp
import { LruCache, LruConfig } from "@cap/lru";

fn demoWeightedCache() {
    let cache = LruCache.init(LruConfig{
        max_entries: 128,
        max_weight: 100, // 最多缓存 100 权重单位
    });

    * = cache.putWithOptions("k1", "payload_1", 0, 40, 1);
    * = cache.putWithOptions("k2", "payload_2", 0, 40, 2);
    // 再写入一条权重 50 的条目，总权重超 100，自动淘汰最旧的 "k1"
    * = cache.putWithOptions("k3", "payload_3", 0, 50, 3);

    * = cache.get("k2", 4); // hit
    * = cache.get("k1", 5); // miss

    const st = cache.stats();
    const rate = st.hitRate(); // 0.5
    * = rate;
}
```

## 许可协议

MIT License — 本包为基于 CP 语言独立设计的 Clean-Room 实现，零外部依赖。
