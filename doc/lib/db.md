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
| 缓存集群路由 | `lib/db/redis_cluster_crc16.cp` / `lib/db/redis_cluster_topology.cp` | Redis/Valkey Cluster CRC16-CCITT 槽位路由、Hash Tag、拓扑映射与 MOVED/ASK 重定向解析 | — |
| 缓存协议编解码 | `lib/db/valkey.cp` | RESP2/RESP3 零拷贝增量流式解析器、内联命令解析与编码器（`RespStreamParser` / `RespValue` / `encodeCommand`） | — |
| SQL 查询构建器 | `lib/db/query_types.cp` / `lib/db/query_builder.cp` | 参数化 SQL 查询构建器（PostgreSQL `$1..$N`、MySQL/SQLite `?` 防注入 `SelectBuilder`/`InsertBuilder`/`UpdateBuilder`/`DeleteBuilder`） | — |
| Schema 校验引擎 | `lib/db/schema.cp` / `lib/db/schema_validator.cp` | JSON Schema Draft 2020-12 & AI Tool Calling 运行时结构化校验器（`Schema` / `SchemaValidator`） | — |
| 连接池生命周期引擎 | `lib/db/conn_pool.cp` | 零堆分配数据库连接池状态机（LIFO 温连接复用、空闲/最大寿命淘汰、不健康连续错误摘除与满载背压 `DbConnPool`） | — |
| 崩溃恢复 WAL 引擎 | `lib/db/wal_recovery.cp` | 零堆分配 IEEE CRC32 防撕裂写 WAL 帧编解码与崩溃恢复引擎（`WalFrameWriter` / `WalRecoveryScanner`） | — |
| LSM 分层压缩规划器 | `lib/db/lsm_compactor.cp` | 零堆分配 RocksDB/Pebble 级 LSM-Tree 分层压缩规划器与 SSTable 键区间重叠选择器（`LsmCompactionPlanner`） | — |
| MVCC 快照与水位线 GC | `lib/db/mvcc_gc.cp` | 零堆分配 MVCC 多版本链、快照隔离时间旅行读与 Low-Watermark 安全水位线垃圾回收引擎（`MvccVersionGcStore`） | — |

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

### 零堆分配数据库连接池状态机示例 (`conn_pool.cp`)

```cp
import {
    DbPoolConfig,
    DbConnSlot,
    DbAcquireKind,
    DbConnPool,
} from "cap:db";

pub fn demoDbConnPool() {
    let slots: [8]DbConnSlot = [DbConnSlot{}; 8];
    const cfg = DbPoolConfig.init(4, 1, 60000, 600000, 2) ?? return;
    let pool = DbConnPool.init(slots[0..], cfg) ?? return;

    const acq = pool.acquire(1000);
    if (acq.kind == DbAcquireKind.Created) {
        * = pool.bindHandle(acq.slot_index, 42); // 绑定底层连接句柄
    }
    * = pool.release(acq.slot_index, true, 1200); // 健康归还以供下次 LIFO 复用
    * = pool.evictExpired(90000);                 // 批量清理过期空闲连接
    * = pool.stats();
}
```

### 零堆分配 CRC32 防撕裂写 WAL 崩溃恢复示例 (`wal_recovery.cp`)

```cp
import {
    WalRecordKind,
    WalFrameView,
    WalFrameWriter,
    WalRecoveryScanner,
} from "cap:db";

pub fn demoWalCrashRecovery() {
    let raw_log: [256]u8 = [0; 256];
    let writer = WalFrameWriter.init(raw_log[0..]);

    * = writer.appendFrame(1, WalRecordKind.Put, "k1=v1");
    * = writer.appendFrame(2, WalRecordKind.Commit, "");
    * = writer.appendFrame(3, WalRecordKind.Checkpoint, "snap_01");

    // 模拟断电时尾部仅写入半帧：截断最后 4 字节后扫描恢复
    const full = writer.writtenBytes();
    const torn = full[0 .. full.len - 4];

    let frames: [8]WalFrameView = [WalFrameView{}; 8];
    const report = WalRecoveryScanner.scanAndRecover(torn, frames[0..]);
    * = report.valid_end_offset;   // 安全回退至第 2 帧末尾
    * = report.last_valid_lsn;     // 2
    * = report.torn_tail_bytes;    // 被截断的尾部撕裂字节数
}
```

### 零堆分配 LSM-Tree 分层压缩规划器示例 (`lsm_compactor.cp`)

面向 RocksDB / Pebble 级存储引擎的 100% 纯 CP 零堆分配分层压缩规划内核（固定 128 SSTable 内联槽位）：支持 L0 重叠簇展开 + L1 扇出、Ln->Ln+1 空间超限与零写放大 `trivial_move` 快路径、高墓碑率 `tombstone_reclaim` 回收及 `being_compacted` 并发冲突跳过。

```cp
import {
    LsmCompactionConfig,
    SstFileMeta,
    LsmCompactionPlanner,
} from "cap:db";

pub fn demoLsmCompactionPlanner() {
    const cfg = LsmCompactionConfig{
        max_levels: 4,
        l0_compaction_trigger: 2,
        base_level_bytes: 1024,
        level_multiplier: 10,
        tombstone_trigger_bps: 3000,
    };
    let planner = LsmCompactionPlanner.init(cfg) ?? return;

    // L1 空间超限（1500 > 1024），且 L2 零重叠 -> 触发零写放大 trivial_move 快路径
    if (f1 := SstFileMeta.init(101, 1, 1500, "a", "f", 100)) {
        * = planner.add_sst(f1);
    }
    if (f2 := SstFileMeta.init(201, 2, 500, "m", "z", 50)) {
        * = planner.add_sst(f2);
    }

    const plan = planner.plan_next_compaction();
    if (plan.valid && plan.is_trivial_move) {
        * = planner.mark_compacting(plan.source_file_ids[0], true);
    }
}
```

### 零堆分配 MVCC 快照隔离与安全水位线 GC 引擎示例 (`mvcc_gc.cp`)

面向 PostgreSQL / TiKV / RocksDB 事务存储引擎的 100% 纯 CP 零堆分配 MVCC 多版本链、快照读可见性判定与 Low-Watermark 安全水位线垃圾回收内核（固定 `MVCC_GC_MAX_VERSIONS = 256` 版本槽、`MVCC_GC_MAX_READERS = 32` 活跃读事务槽）：

- **类型**：`MvccVersionRecord`、`MvccReadSnapshot`、`MvccReadResult`、`MvccGcReport`、`MvccGcStats`、`MvccVersionGcStore`。
- **核心方法**：`init()`、`begin_read_tx(*_, tx_id, read_ts)`、`end_read_tx(*_, tx_id)`、`safe_horizon_ts()`、`write_version(*_, key_hash, commit_ts, val_tag, is_tombstone)`（同键严格递增 `commit_ts` 防历史覆写）、`read_visible(key_hash, snapshot_ts)`、`collect_garbage(*_): MvccGcReport`（安全回收水位线之下的冗余覆盖版本及无新版本的孤立墓碑）、`stats()`。

```cp
import { MvccVersionGcStore } from "cap:db";

pub fn demoMvccSafeHorizonGc() {
    let store = MvccVersionGcStore.init();
    const k: u64 = 0x1001;

    * = store.write_version(k, 10, 100, false);
    * = store.write_version(k, 20, 200, false);
    * = store.write_version(k, 30, 300, false);

    // 长运行读事务钉住安全水位线于 ts=25：保护 ts=20 基底版本不被回收，仅回收 ts=10
    * = store.begin_read_tx(1, 25);
    const rep1 = store.collect_garbage(); // safe_horizon_ts=25, reclaimed=1
    const snap = store.read_visible(k, 25); // val_tag == 200

    // 读事务结束后水位线推进至 30，安全回收 ts=20
    * = store.end_read_tx(1);
    const rep2 = store.collect_garbage(); // safe_horizon_ts=30, reclaimed=1
    * = rep1;
    * = snap;
    * = rep2;
}
```

---

## 收敛子模块深度文档

- [bloom_cuckoo.md](db/bloom_cuckoo.md)
- [bplus.md](db/bplus.md)
- [hnsw.md](db/hnsw.md)
- [hyperloglog.md](db/hyperloglog.md)
- [kv.md](db/kv.md)
- [lfu.md](db/lfu.md)
- [lock.md](db/lock.md)
- [lru.md](db/lru.md)
- [lru_k.md](db/lru_k.md)
- [lsm.md](db/lsm.md)
- [mvcc.md](db/mvcc.md)
- [query.md](db/query.md)
- [raft_log.md](db/raft_log.md)
- [redis_cluster.md](db/redis_cluster.md)
- [roaring_bitmap.md](db/roaring_bitmap.md)
- [schema.md](db/schema.md)
- [search.md](db/search.md)
- [sketch.md](db/sketch.md)
- [slab_pool.md](db/slab_pool.md)
- [sstable.md](db/sstable.md)
- [valkey.md](db/valkey.md)
- [wal.md](db/wal.md)
- [xorfilter.md](db/xorfilter.md)
