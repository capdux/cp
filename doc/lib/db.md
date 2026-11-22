# `cap:db` — 统一数据库存储引擎、事务并发、缓存与索引标准库

> 聚合路径：`lib/db/index.cp`（`import { ... } from "cap:db"` / `import cap from "cap"; cap.db.*`）

`cap:db` 是 CP 标准库的统一数据库与存储内核主包，100% 纯 CP 零外部 C 依赖，涵盖 **有序/日志结构存储引擎（B+Tree / LSM-Tree / WAL / SSTable / KV）**、**事务隔离与并发锁（MVCC / Lock Manager / Raft Log）**、**缓存与对象池（LRU / LRU-K / LFU & W-TinyLFU / Slab Pool）** 以及 **向量/全文/位图/概率索引（HNSW / BM25F Search / Roaring Bitmap / Bloom & Cuckoo / XorFilter / Streaming Sketches）**。

---

## 模块架构总览

| 分类 | 文件路径 | 核心能力 | 导出别名（如适用） |
| :--- | :--- | :--- | :--- |
| 存储引擎与 KV | `lib/db/bplus.cp` | B+ Tree 有序键值与范围扫描引擎 | — |
| 存储引擎与 KV | `lib/db/lsm.cp` | LSM-Tree 内存表/分级合并存储引擎 | — |
| 存储引擎与 KV | `lib/db/wal.cp` | CRC32C 校验预写日志（WAL）与严格崩溃恢复 | — |
| 存储引擎与 KV | `lib/db/sstable.cp` | 零堆分配 LevelDB / RocksDB SSTable 解析器 | — |
| 存储引擎与 KV | `lib/db/kv_wal.cp` / `lib/db/kv_store.cp` | 嵌入式持久化 KV 存储引擎与日志编解码 | `KV_WAL_MAGIC`, `KV_WAL_HEADER_SIZE`, `KvWalRecord` |
| 事务、锁与日志 | `lib/db/mvcc_types.cp` / `lib/db/mvcc_store.cp` | MVCC 快照隔离事务存储 | — |
| 事务、锁与日志 | `lib/db/lock_wait_graph.cp` / `lib/db/lock.cp` | 命名锁表、租约锁管理器与死锁等待图检测 | — |
| 事务、锁与日志 | `lib/db/raft_log.cp` | Raft 复制日志与快照压缩 | — |
| 缓存与内存池 | `lib/db/lru.cp` | O(1) LRU 缓存（支持 TTL） | — |
| 缓存与内存池 | `lib/db/lru_k.cp` | 抗全表扫描 LRU-K 缓存 | — |
| 缓存与内存池 | `lib/db/lfu.cp` | 零堆分配 LFU 缓存与 4-bit W-TinyLFU Count-Min Sketch | — |
| 缓存与内存池 | `lib/db/slab_pool.cp` | 代际 O(1) Slab 对象池（防 ABA 句柄） | — |
| 向量与全文索引 | `lib/db/hnsw_metric.cp` / `lib/db/hnsw_graph.cp` | 多层可导航小世界图（HNSW）向量近似最近邻检索 | `HnswSearchHit` |
| 向量与全文索引 | `lib/db/search_trie.cp` / `lib/db/search_engine.cp` | 多字段 BM25F 全文检索、布尔/短语查询、前缀 Trie 与高亮摘要 | — |
| 位图与概率索引 | `lib/db/roaring_bitmap.cp` | Roaring Bitmap 压缩整数集合与位运算 | — |
| 位图与概率索引 | `lib/db/bloom_cuckoo.cp` | Bloom、Counting Bloom 与 Cuckoo 过滤器 | — |
| 位图与概率索引 | `lib/db/xorfilter.cp` | Xor8 / Xor16 静态低空间成员过滤器 | — |
| 位图与概率索引 | `lib/db/sketch.cp` | 流式概率草图（`HyperLogLog` / `QuantileDigest` / `ReservoirSampler`） | — |
| 参考归档（不导出） | `lib/db/hyperloglog_.cp` | 独立 64-bit HyperLogLog 实现（仅参考） | 与 `sketch.cp` 中 `HyperLogLog` 重叠，不在 `cap:db` 重复导出 |

---

## 快速示例

```cp
const std = @import("std");
import {
    BPlusTree,
    LruCache,
    RoaringBitmap,
    SearchIndex,
    HyperLogLog,
} from "cap:db";

fn main()! {
    const alloc = std.heap.page_allocator;

    // 1. B+ Tree 有序键值存储
    let tree = BPlusTree.init(alloc, 5)?;
    defer tree.deinit();

    // 2. LRU 缓存
    let cache = LruCache<i64>.init(alloc, 128, 0)?;
    defer cache.deinit();

    // 3. Roaring Bitmap 压缩集合
    let bm = RoaringBitmap.init(alloc);
    defer bm.deinit();

    // 4. BM25F 全文检索引擎
    let idx = SearchIndex.init(alloc);
    defer idx.deinit();

    // 5. HyperLogLog 基数估算
    let hll = HyperLogLog.init(alloc, 10)?;
    defer hll.deinit();
}
```
