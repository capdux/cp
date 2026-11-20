# CP 标准库：高性能内存缓存与概率数据结构手册 (`cap:cache` / `cap.cache`)

## 一、 架构总览与设计准则

`cap:cache` 是 CP 语言官方内置的纯原生高性能内存缓存与概率数据结构库，专为 AI Agent 响应缓存、Prompt/Embedding 缓存、高频去重与热键频次统计打造。包含三大核心组件：

1. **`LruCache<V>` + `CacheStats`**（`lib/cache/lru.cp`）：基于 `std.StringHashMapUnmanaged(usize)` 与预分配连续槽位池上的侵入式双向链表，实现严格 $O(1)$ 插入、查询、淘汰与确定性 `now_ms` TTL 过期；对传入字符串键执行所有权安全深拷贝，绝不悬垂。
2. **`BloomFilter`**（`lib/cache/bloom.cp`）：基于 64 位字数组与 FNV-1a 64 + SplitMix64 双哈希（Kirsch-Mitzenmacher 策略）的空间高效布隆过滤器，支持最优参数自动推导 (`initOptimal`)、集合并 (`unionWith`) 与基数估计 (`estimatedCount`)。
3. **`CountMinSketch`**（`lib/cache/cms.cp`）：亚线性空间流式频次估计器，保障永不低估真实频次 (`estimate(x) >= true_freq(x)`)，支持精度误差界初始化 (`initWithError`) 与分布式多实例无损合并 (`merge`)。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:cache"` 与 `import cap from "cap"; cap.cache.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/cache/lru.cp` | `// Source: cp/lib/cache/lru.cp (O(1) LRU + TTL In-Memory Cache)` | `CacheErr`、`CacheStats`、`LruNode<V>`、`LruCache<V>` |
| `lib/cache/bloom.cp` | `// Source: cp/lib/cache/bloom.cp (Space-Efficient Bloom Filter with Double Hashing)` | `BloomFilter` 双哈希布隆过滤器 |
| `lib/cache/cms.cp` | `// Source: cp/lib/cache/cms.cp (Count-Min Sketch Frequency Estimator)` | `CountMinSketch` 流式频次估计器 |
| `lib/cache/index.cp` | `// Source: cp/lib/cache/index.cp (Unified Exports for cap:cache)` | `cap:cache` 与 `cap.cache` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. O(1) LRU + TTL 内存缓存（`lru.cp`）

- **`CacheErr`**：`OutOfMemory`、`InvalidCapacity`、`InvalidParameter`、`SizeMismatch`
- **`CacheStats`**：
  - 字段：`hits: u64`、`misses: u64`、`evictions: u64`、`expirations: u64`、`len: usize`、`capacity: usize`
  - 方法：`hitRate(): f64`、`totalRequests(): u64`
- **`LruCache<V>`**：
  - 生命周期：`init(alloc, max_capacity): LruCache<V>!`、`deinit(*_)`、`clear(*_)`
  - 写入：`put(*_, key, value): !`、`putWithTtl(*_, key, value, ttl_ms, now_ms): !`
  - 读取与探测：`get(*_, key): V?`、`getAt(*_, key, now_ms): V?`（惰性过期淘汰并更新 `expirations`/`misses`）、`peek(key): V?`、`peekAt(key, now_ms): V?`、`contains(key): bool`、`containsAt(key, now_ms): bool`
  - 删除与主动清理：`remove(*_, key): bool`、`evictExpired(*_, now_ms): usize`
  - 元信息：`len(): usize`、`capacity(): usize`、`isEmpty(): bool`、`stats(): CacheStats`

### 2. 双哈希布隆过滤器（`bloom.cp`）

- **`BloomFilter`**：
  - 构造与销毁：`initOptimal(alloc, expected_items, false_positive_rate): BloomFilter!`、`initRaw(alloc, bit_count, num_hashes): BloomFilter!`、`deinit(*_)`
  - 集合操作：`insert(*_, item)`、`mightContain(item): bool`、`unionWith(*_, other): !`、`clear(*_)`
  - 观测指标：`fillRatio(): f64`、`estimatedCount(): f64`、`bitCount(): usize`、`numHashes(): usize`、`insertedCount(): u64`

### 3. Count-Min Sketch 频次估计器（`cms.cp`）

- **`CountMinSketch`**：
  - 构造与销毁：`init(alloc, num_width, num_depth): CountMinSketch!`、`initWithError(alloc, epsilon, delta): CountMinSketch!`、`deinit(*_)`
  - 计数与查询：`increment(*_, item, count)`、`estimate(item): u64`、`merge(*_, other): !`、`clear(*_)`
  - 观测指标：`totalCount(): u64`、`width(): usize`、`depth(): usize`

---

## 三、 使用示例

```typescript
import cap from "cap";
import { LruCache, BloomFilter, CountMinSketch } from "cap:cache";

pub fn demoCache(): !void {
    const alloc = std.heap.page_allocator;

    // 1. O(1) LRU + TTL 缓存
    let lru = LruCache<i32>.init(alloc, 128)?;
    defer lru.deinit();
    lru.putWithTtl("prompt:hash_1", 42, 5000, 1000)?;
    const hit = lru.getAt("prompt:hash_1", 2000); // Some(42)
    * = hit;

    // 2. 布隆过滤器去重
    let bf = BloomFilter.initOptimal(alloc, 10000, 0.01)?;
    defer bf.deinit();
    bf.insert("doc:abc");
    const maybe = bf.mightContain("doc:abc"); // true
    * = maybe;

    // 3. Count-Min Sketch 热键频次估计 + cap.cache 动态挂载等价用法
    let cms = cap.cache.CountMinSketch.initWithError(alloc, 0.001, 0.01)?;
    defer cms.deinit();
    cms.increment("tool:search", 5);
    const est = cms.estimate("tool:search"); // >= 5
    * = est;
}
```
