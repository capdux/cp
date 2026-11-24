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
| 存储引擎与 KV | `lib/db/kvWal.cp` / `lib/db/kvStore.cp` | 嵌入式持久化 KV 存储引擎与日志编解码 | `KV_WAL_MAGIC`, `KV_WAL_HEADER_SIZE`, `KvWalRecord` |
| 事务、锁与日志 | `lib/db/mvccTypes.cp` / `lib/db/mvccStore.cp` | MVCC 快照隔离事务存储 | — |
| 事务、锁与日志 | `lib/db/lockGraph.cp` / `lib/db/lock.cp` | 命名锁表、租约锁管理器与死锁等待图检测 | — |
| 事务、锁与日志 | `lib/db/raftLog.cp` / `lib/db/raftVote.cp` | Raft 复制日志、快照安装与 Sans-IO 选主/Pre-Vote/Figure-8 提交状态机（对标 `tikv/raft-rs` Apache-2.0 加固 + 4-lane `@Vector(4, u64)` SIMD 法定人数统计与最大索引扫描；测试：`cap/test/testDbRaftRustRefSuite.cp`） | — |
| 缓存与内存池 | `lib/db/lru.cp` | O(1) LRU 缓存（支持 TTL） | — |
| 缓存与内存池 | `lib/db/lruK.cp` | 抗全表扫描 LRU-K 缓存 | — |
| 缓存与内存池 | `lib/db/lfu.cp` | 零堆分配 LFU 缓存与 4-bit W-TinyLFU Count-Min Sketch | — |
| 缓存与内存池 | `lib/db/slabPool.cp` | 代际 O(1) Slab 对象池（防 ABA 句柄） | — |
| 向量与全文索引 | `lib/db/hnswMetric.cp` / `lib/db/hnswGraph.cp` | 多层可导航小世界图（HNSW）向量近似最近邻检索 | `HnswSearchHit` |
| 向量与全文索引 | `lib/db/searchTrie.cp` / `lib/db/searchEng.cp` | 多字段 BM25F 全文检索、布尔/短语查询、前缀 Trie 与高亮摘要 | — |
| 位图与概率索引 | `lib/db/roaring.cp` | Roaring Bitmap 压缩整数集合与位运算 | — |
| 位图与概率索引 | `lib/db/bloomCuckoo.cp` | Bloom、Counting Bloom 与 Cuckoo 过滤器 | — |
| 位图与概率索引 | `lib/db/xorfilter.cp` | Xor8 / Xor16 静态低空间成员过滤器 | — |
| 位图与概率索引 | `lib/db/sketch.cp` | 流式概率草图（`HyperLogLog` / `QuantileDigest` / `ReservoirSampler`） | — |
| 参考归档（不导出） | `lib/db/hyperloglog_.cp` | 独立 64-bit HyperLogLog 实现（仅参考） | 与 `sketch.cp` 中 `HyperLogLog` 重叠，不在 `cap:db` 重复导出 |
| 缓存集群路由 | `lib/db/redisClusterCrc16.cp` / `lib/db/redisClusterTopology.cp` | Redis/Valkey Cluster CRC16-CCITT 槽位路由、Hash Tag、拓扑映射与 MOVED/ASK 重定向解析 | — |
| 缓存协议编解码 | `lib/db/valkey.cp` | RESP2/RESP3 零拷贝增量流式解析器、内联命令解析与编码器（`RespStreamParser` / `RespValue` / `encodeCommand`） | — |
| SQL 查询构建器 | `lib/db/queryTypes.cp` / `lib/db/queryBuilder.cp` | 参数化 SQL 查询构建器（PostgreSQL `$1..$N`、MySQL/SQLite `?` 防注入 `SelectBuilder`/`InsertBuilder`/`UpdateBuilder`/`DeleteBuilder`） | — |
| Schema 校验引擎 | `lib/db/schema.cp` / `lib/db/schemaValidator.cp` | JSON Schema Draft 2020-12 & AI Tool Calling 运行时结构化校验器（`Schema` / `SchemaValidator`） | — |
| 连接池生命周期引擎 | `lib/db/connPool.cp` | 零堆分配数据库连接池状态机（LIFO 温连接复用、空闲/最大寿命淘汰、不健康连续错误摘除与满载背压 `DbConnPool`） | — |
| 崩溃恢复 WAL 引擎 | `lib/db/walRecover.cp` / `lib/db/writeBatch.cp` | 零堆分配 IEEE CRC32 防撕裂写 WAL 帧编解码、崩溃恢复与原子 WriteBatch / GroupCommit 引擎（对标 `fjall-rs/fjall` MIT OR Apache-2.0 加固 + 16-lane `@Vector(16, u8)` SIMD 载荷拷贝与键值比对；测试：`cap/test/testDbWalBatchRustRefSuite.cp`） | — |
| LSM 分层压缩规划器 | `lib/db/compactor.cp` | 零堆分配 RocksDB/Pebble 级 LSM-Tree 分层压缩规划器与 SSTable 键区间重叠选择器（`LsmCompactionPlanner`） | — |
| MVCC 快照与水位线 GC | `lib/db/mvccGc.cp` | 零堆分配 MVCC 多版本链、快照隔离时间旅行读与 Low-Watermark 安全水位线垃圾回收引擎（`MvccVersionGcStore`） | — |

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

### 零堆分配数据库连接池状态机示例 (`connPool.cp`)

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

### 零堆分配 CRC32 防撕裂写 WAL 崩溃恢复示例 (`walRecover.cp`)

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

### 零堆分配 LSM-Tree 分层压缩规划器示例 (`compactor.cp`)

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

### 零堆分配 MVCC 快照隔离与安全水位线 GC 引擎示例 (`mvccGc.cp`)

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

### 零分配 CLOCK 二次机会数据库页缓冲池管理器示例 (`bufferPool.cp`)

面向 PostgreSQL `shared_buffers` / SQLite Pager / InnoDB Buffer Pool 级存储引擎的 100% 纯 CP 零动态堆分配 CLOCK（Second-Chance）引用计数页帧缓冲池与脏页刷盘管理器（固定内联上限 `BUFFER_POOL_MAX_FRAMES = 128`）：

- **常量与模型**：`BUFFER_POOL_MAX_FRAMES` (`128`)、`BUFFER_POOL_INVALID_PAGE` (`0xFFFFFFFFFFFFFFFF`)、`PageFrameMeta`、`PageFetchOutcome`、`BufferPoolStats`。
- **核心引擎 `ClockPageBufferPool`**：
  - `init(capacity): ClockPageBufferPool?`：校验 `1 <= capacity <= 128`，零堆初始化。
  - `fetch_page(*_, page_id): PageFetchOutcome`：命中则递增 `pin_count` 并置 `ref_bit = true`；未命中优先分配空帧；满池时推进 `clock_hand` 执行最多两圈 CLOCK 二次机会扫描（跳过 `pin_count > 0` 固定页，对 `ref_bit == true` 清零给二次机会，淘汰首个冷未固定页并报告 `evicted_page_id` 与 `evicted_was_dirty`；全部固定则返回 `ok = false, all_pinned = true`）。
  - `unpin_page(*_, page_id, mark_dirty): bool`：递减 `pin_count`（防下溢）并可选标脏。
  - `flush_dirty_pages(*_, out_dirty_page_ids): usize`：收集脏页号并清零已写出帧的 `dirty` 位，天然支持小缓冲分批刷盘不丢页。
  - `invalidate_page(*_, page_id): bool`：仅允许释放 `pin_count == 0` 的空闲驻留页；`stats(): BufferPoolStats`。

```cp
import { ClockPageBufferPool } from "cap:db";

pub fn demoClockPageBufferPool() {
    let pool = ClockPageBufferPool.init(4) ?? return;

    const f1 = pool.fetch_page(10); // miss -> frame 0, pin=1
    * = pool.unpin_page(10, true);  // pin=0, dirty=true

    // 分批刷盘：收集所有脏页号并清零 dirty 位
    let dirty_buf: [4]u64 = [0; 4];
    let dirty_s: []u64 = dirty_buf[0..];
    const flushed = pool.flush_dirty_pages(dirty_s);
    * = f1;
    * = flushed; // 1 (dirty_buf[0] == 10)
}
```

### 零分配 MVCC 快照隔离与版本可见性 / GC 水位线引擎 (`mvccSnap.cp`)

面向 PostgreSQL / InnoDB 级事务存储引擎的 100% 纯 CP 零动态堆分配 MVCC Snapshot Isolation（`MvccReadView`）与多版本元组链 GC 引擎：

- **常量与枚举**：
  - `MVCC_SNAP_MAX_ACTIVE_TXNS = 64`、`MVCC_SNAP_MAX_TX_HISTORY = 512`、`MVCC_SNAP_MAX_VERSIONS = 256`、`MVCC_SNAP_INVALID_TX = 0`、`MVCC_SNAP_INF_TX = 0xFFFFFFFFFFFFFFFF`、`MVCC_SNAP_NO_SLOT = 0xFFFF`
  - `MvccTxStatus`（`unknown | active | committed | aborted`）
  - `MvccConflictKind`（`none | active_writer_conflict | committed_after_snapshot | invalid_tx`）
- **核心数据模型**：
  - `MvccReadView`（`viewer_tx_id`, `xmin`, `xmax`, 升序 `active_tx_ids`, `active_count`；内置二分查找 `isTxInActiveSet(tx_id)`）
  - `MvccTupleVersion`（`key_hash`, `value`, `begin_tx`, `end_tx`, `prev_version_slot`, `deleted`, `occupied`）
  - `MvccWriteConflict`（`has_conflict`, `kind`, `conflicting_tx_id`, `version_slot`）、`MvccSnapshotStats`
- **核心引擎 `MvccSnapshotManager`**：
  - 事务生命周期：`beginTx(*_): u64?`（单调分配）、`commitTx(*_, tx_id): bool`、`abortTx(*_, tx_id): bool`、`txStatus(tx_id): MvccTxStatus`。
  - 快照与可见性：`captureReadView(viewer_tx_id): MvccReadView?`、`isTxVisibleInView(tx_id, view)`、`isVersionVisible(version, view)`（严格实现自写可见、`< xmax` 且不在活跃集合的已提交版本可见、已回滚版本/终止符不可见）。
  - 写写冲突检测：`detectWriteConflict(key_hash, writer_tx_id, view): MvccWriteConflict`（标准 First-Committer-Wins 冲突检测，区分 `active_writer_conflict` 与 `committed_after_snapshot`）。
  - 多版本元组链与 GC：`insertVersion`、`deleteKey`、`readVisibleVersion`、`computeGcWatermark()`（最小活跃事务 ID 或 `next_tx_id`）、`canReclaimVersion(version, gc_watermark)`、`reclaimDeadVersions(*_): usize`（回收时自动旁路重连 `prev_version_slot` 不断链）。

```cp
import { MvccSnapshotManager } from "cap:db";

pub fn demoMvccSnapshot() {
    let mgr = MvccSnapshotManager.init();
    const t1 = mgr.beginTx() ?? return;
    * = mgr.insertVersion(0xA1, 100, t1);
    * = mgr.commitTx(t1);

    const reader = mgr.beginTx() ?? return;
    const view = mgr.captureReadView(reader) ?? return;

    const t2 = mgr.beginTx() ?? return;
    * = mgr.insertVersion(0xA1, 200, t2);
    * = mgr.commitTx(t2);

    // Snapshot Isolation: reader 仍看到 100，不受后续提交的 t2 影响
    if (v := mgr.readVisibleVersion(0xA1, view)) {
        * = v.value; // 100
    }
    * = mgr.commitTx(reader);
    * = mgr.reclaimDeadVersions(); // 回收已被覆盖的旧版本 100
}
```

### 零分配数据库槽页二进制布局、原地紧凑化与 CRC32 校验引擎 (`slottedPage.cp`)

面向 PostgreSQL / SQLite 经典存储引擎的 100% 纯 CP 零动态堆分配 4KB/8KB（任意 `64..65535B`）槽页二进制布局引擎，直接操作调用方持有的 `page_buf: *[]u8` 实现零拷贝读写与 O(1) 额外内存原地紧凑化：

- **二进制布局与常量**：
  - `SLOTTED_PAGE_MAGIC = 0x31535043` (`"CPS1"`)、`SLOTTED_PAGE_HEADER_SIZE = 32`、`SLOTTED_SLOT_SIZE = 6`、`SLOTTED_PAGE_MIN_SIZE = 64`、`SLOTTED_PAGE_MAX_SIZE = 65535`
  - `SLOT_FLAG_ALIVE = 0x0001`、`SLOT_FLAG_DELETED = 0x0002`
  - 32B 固定小端页头：`magic` / `page_id` / `lsn` / `flags` / `slot_count` / `free_start` / `free_end` / `fragmented_bytes` / `crc32`；槽目录自偏移 `32` 向高地址增长，记录载荷自页尾向低地址增长。
- **元数据与校验状态**：
  - `SlottedPageStatus`（`ok` | `invalid_page_size` | `invalid_magic` | `checksum_mismatch` | `corrupt_header` | `corrupt_slot_bounds` | `overlapping_records`）
  - `SlottedPageHeader`、`SlottedSlotMeta`（`isAlive()`, `isDeleted()`）、`SlottedFreeSpace`
- **核心函数**：
  - `initPage(*page, page_id, lsn, flags)` / `readPageHeader(page)` / `readSlotMeta(page, slot_id)` / `freeSpaceBytes(page)` / `setPageLsn(*page, lsn)`
  - `insertRecord(*page, record): u16?`：优先复用墓碑槽；连续空间不足但碎片可回收时自动触发原地 `compactPage`。
  - `getRecord(page, slot_id): ([]const u8)?`：零拷贝返回存活记录只读切片。
  - `updateRecordInPlace(*page, slot_id, new_record): bool`：等长/缩小原地覆写并累计碎片；扩容原子校验容量并在必要时自动紧凑化，失败不破坏原记录。
  - `deleteRecord(*page, slot_id): bool` / `compactPage(*page): bool`（O(1) 额外内存反向无重叠搬移）
  - `slottedPageCrc32(page)` / `sealChecksum(*page)` / `verifyPageIntegrity(page)`（全面校验页大小、Magic、CRC32、页头指针、槽边界与记录非重叠）

```cp
import {
    SlottedPageStatus,
    initPage,
    insertRecord,
    getRecord,
    deleteRecord,
    sealChecksum,
    verifyPageIntegrity,
} from "cap:db";

pub fn demoSlottedPage() {
    let page_arr: [4096]u8 = [0; 4096];
    let page: []u8 = page_arr[0..];
    * = initPage(*page, 42, 1001, 0);

    const s0 = insertRecord(*page, "row:alpha:100") ?? return;
    const s1 = insertRecord(*page, "row:beta:200") ?? return;
    * = deleteRecord(*page, s0); // 墓碑标记，后续插入可自动复用槽 0 并按需紧凑化

    if (rec := getRecord(page[0..], s1)) {
        * = rec; // 零拷贝切片 "row:beta:200"
    }
    * = sealChecksum(*page);
    const st = verifyPageIntegrity(page[0..]);
    * = (st == SlottedPageStatus.ok); // true
}
```

### 零分配 64B 缓存行对齐 Blocked Bloom Filter (`bloomBlock.cp`)

面向 SSTable / LSM-Tree 存储引擎的 100% 纯 CP 零动态堆分配 **Cache-Line Aligned Blocked Bloom Filter**（RocksDB / Parquet Split-Block 经典架构）：

- **常量与数据模型**：
  - `BLOOM_BLOCK_MAGIC = 0x31425043`（`"CPB1"`）、`BLOOM_BLOCK_HEADER_SIZE = 12`、`BLOOM_BLOCK_WORDS = 8`、`BLOOM_BLOCK_BITS = 512`、`BLOOM_BLOCK_BYTES = 64`、`BLOOM_BLOCK_MAX_BLOCKS = 256`、`BLOOM_BLOCK_MAX_PROBES = 16`。
  - `BloomBlockParams`（`num_blocks`, `num_probes`, `bits_per_key`, `estimated_fpr`）、`BloomBlockStats`、`BloomBlock`（单 64B 缓存行 512-bit 块，内建 `@popCount`）。
- **核心函数与 `BlockedBloomFilter` 方法**：
  - `recommendBloomParams(expected_keys, target_fpr): BloomBlockParams?`：按目标误判率与预期键数自动规划最优块数与块内探测次数。
  - `bloomSplitMix64(x)` / `bloomHashBytes(data)`：FNV-1a + SplitMix64 双哈希原语。
  - `BlockedBloomFilter.init(num_blocks, num_probes)` / `fromParams(params)`：校验几何合法性（`1..=256` 块、`1..=16` 次探测）。
  - `insertHash(*_, h)` / `insertBytes(*_, key)` / `mayContainHash(h)` / `mayContainBytes(key)`：单次探测仅访问 1 个 64B 缓存行，零假阴性。
  - `unionInPlace(*_, other): bool`：同构过滤器按位合并（异构原子拒绝无副作用）。
  - `setBitCount()` / `fillRatio()` / `estimateFalsePositiveRate()` / `stats()` / `clear(*_)`：硬件 `@popCount` 精确置位密度与实时误判率估算。
  - `serializedSize()` / `serialize(out: *[]u8): usize?` / `verifySerializedBytes(data): bool` / `deserialize(data): BlockedBloomFilter?`：紧凑 SSTable Filter Block 落盘与零拷贝头校验。

```cp
import {
    recommendBloomParams,
    BlockedBloomFilter,
} from "cap:db";

pub fn demoBloomBlock() {
    if (params := recommendBloomParams(1000, 0.01)) {
        if (let bf := BlockedBloomFilter.fromParams(params)) {
            bf.insertBytes("user:1001");
            bf.insertBytes("user:1002");
            * = bf.mayContainBytes("user:1001"); // true
            * = bf.estimateFalsePositiveRate();

            let buf: [2048]u8 = [0; 2048];
            let s: []u8 = buf[0..];
            if (n := bf.serialize(*s)) {
                if (restored := BlockedBloomFilter.deserialize(buf[0..n])) {
                    * = restored.mayContainBytes("user:1002"); // true
                }
            }
        }
    }
}
```

---

## 零分配 LSM-Tree 有序 MemTable（`memTable.cp`）

面向 LSM-Tree 键值存储引擎的 100% 纯 CP 零动态堆分配有序内存表，内置 8 KiB 紧凑字节池与 128 槽位字典序二分查找索引：

- **常量与枚举**：
  - `MEMTABLE_MAX_ENTRIES = 128`、`MEMTABLE_ARENA_BYTES = 8192`
  - `MemTableWriteStatus`（`ok | frozen | empty_key | stale_seq | index_full | arena_full`）
  - `MemTableGetKind`（`found_value | found_tombstone | not_found`）
- **数据模型**：
  - `MemTableEntryRef`（`seq_num`, `is_tombstone`, `key_off`, `key_len`, `val_off`, `val_len`）
  - `MemTableGetResult`（`kind`, `seq_num`, `entry`）、`MemTableStats`（`entry_count`, `live_count`, `tombstone_count`, `arena_used_bytes`, `min_seq_num`, `max_seq_num`, `frozen`）
- **核心引擎 `SortedMemTable`**：
  - `init()` / `freeze(*_)` / `isFrozen()` / `stats()`
  - `put(*_, key, value, seq_num): MemTableWriteStatus`：严格单调 `seq_num` 写入，同 key 自动复用键字节与原位值槽位；冻结态/空 key/过期序列号/满池均原子拒绝。
  - `deleteKey(*_, key, seq_num): MemTableWriteStatus`：写入墓碑记录保留 `seq_num` 以遮蔽下层 SSTable（即使键不在当前 MemTable 中）。
  - `get(key): MemTableGetResult`：`O(log N)` 二分点查，精确区分 `found_value` / `found_tombstone` / `not_found`。
  - `scanRange(start_key, end_key, include_tombstones, *out_entries)` / `scanPrefix(prefix, *out_entries)`：`O(log N)` 定位起点的严格字典序范围/前缀扫描。
  - `copyKey` / `copyValue` / `keyEquals` / `valueEquals`：安全无悬垂键值读取与比对。

```cp
import {
    MemTableEntryRef,
    SortedMemTable,
} from "cap:db";

pub fn demoSortedMemTable() {
    let mt = SortedMemTable.init();
    * = mt.put("user:2", "bob", 1);
    * = mt.put("user:1", "alice", 2);
    * = mt.deleteKey("user:2", 3);
    mt.freeze(); // 冻结准备 Flush 至 SSTable

    let out: [8]MemTableEntryRef = [MemTableEntryRef{}; 8];
    let out_s: []MemTableEntryRef = out[0..];
    const n = mt.scanRange("", "", true, *out_s);
    * = n; // 2: ["user:1"(live), "user:2"(tombstone)]
}
```

---

### 零分配原子写批次与 LSM Group Commit 合并协调器 (`writeBatch.cp`)

面向 LSM-Tree / RocksDB 风格持久化引擎的 100% 纯 CP 零动态堆分配原子写批次（`WriteBatch`）与多写者组提交（`GroupCommitCoordinator`）协调器：

- **常量与枚举**：
  - `WRITE_BATCH_HEADER_SIZE = 12`（8B LE `seq` + 4B LE `count`）、`WRITE_BATCH_TRAILER_SIZE = 4`（4B LE CRC32-IEEE）、`WRITE_BATCH_MAX_BYTES = 4096`、`WRITE_BATCH_MAX_SAVEPOINTS = 16`、`GROUP_COMMIT_MAX_WRITERS = 16`、`GROUP_COMMIT_MAX_BYTES = 8192`
  - `WriteBatchOp`（`put | delete | single_delete | merge`）
  - `WriteBatchStatus`（`ok | buffer_full | empty_key | savepoint_stack_full | no_savepoint | truncated | invalid_tag | malformed_varint | count_mismatch | checksum_mismatch | invalid_batch`）
  - `GroupCommitWriterStatus`（`committed | empty_noop | rejected_invalid | rejected_oversize | rejected_queue_full`）
- **原子 `WriteBatch` 与零拷贝解码**：
  - `WriteBatch.init()` / `setSequence(*_, seq)` / `put(*_, k, v)` / `delete(*_, k)` / `singleDelete(*_, k)` / `merge(*_, k, operand)` / `clear(*_)`
  - 嵌套保存点事务回滚：`setSavePoint(*_)` / `popSavePoint(*_)` / `rollbackToSavePoint(*_)`（最多 16 层嵌套）
  - 帧编码与完整性校验：`encodeFramed(*out_buf): usize?`、`checksum(): u32`、`verify()`、`verifyWriteBatchRaw(raw)`、`verifyWriteBatchFramed(framed)`、`writeBatchCrc32(data)`
  - 零拷贝正向迭代器：`WriteBatchIter.fromRaw(raw)` / `WriteBatchIter.fromFramed(framed)` / `next(*_): WriteBatchRecord?`（直接返回指向原字节切片的零拷贝 `key`/`value` 与递增 `seq`）
- **多写者 `GroupCommitCoordinator`**：
  - `GroupCommitConfig`（`max_writers`, `max_batch_bytes`, `initial_seq`；`defaults()` / `isValid()`）
  - `GroupCommitCoordinator.init(cfg)` / `enqueueBatch(*_, writer_id, batch)` / `enqueueRaw(*_, writer_id, raw)`：首位入队写者自动成为 Leader，后续写者作为 Follower 顺序零拷贝合并载荷
  - `flush(*_, *out_framed, *out_receipts): GroupCommitFlushResult?`：为全组分配严格单调连续 `seq` 区间，生成单块带 CRC32 校验和的落盘帧，并逐一返回每位写者的 `GroupCommitReceipt`（含 `is_leader`, `[start_seq, end_seq)`, `payload_offset`, `payload_bytes`）
  - `stats(): GroupCommitStats`：统计落盘轮次、累计提交写者数、累计操作数、落盘字节数、待提交队列与拒绝次数。

```cp
import {
    WriteBatch,
    WriteBatchIter,
    GroupCommitConfig,
    GroupCommitReceipt,
    GroupCommitCoordinator,
} from "cap:db";

pub fn demoGroupCommit() {
    let wb1 = WriteBatch.init();
    * = wb1.put("k1", "v1");
    * = wb1.merge("cnt", "+1");

    let wb2 = WriteBatch.init();
    * = wb2.delete("k_old");
    * = wb2.singleDelete("k_tmp");

    if (c := GroupCommitCoordinator.init(GroupCommitConfig.defaults())) {
        let coord = c;
        * = coord.enqueueBatch(101, wb1);
        * = coord.enqueueBatch(102, wb2);

        let framed: [4096]u8 = [0; 4096];
        let framed_s: []u8 = framed[0..];
        let receipts: [4]GroupCommitReceipt = [GroupCommitReceipt{}; 4];
        let receipts_s: []GroupCommitReceipt = receipts[0..];

        if (res := coord.flush(*framed_s, *receipts_s)) {
            let it = WriteBatchIter.fromFramed(framed[0..res.merged_framed_bytes]);
            while (rec := it.next()) {
                * = rec.seq;
            }
        }
    }
}
```

---

### 零分配确定性 Arena SkipList 有序 KV 引擎 (`skipList.cp`)

专为 LSM-Tree MemTable 打造的 100% 纯 CP 零动态堆分配确定性跳表引擎，内联定长节点池与 8 KiB 字节 Arena，使用可设种子的 xorshift64 PRNG（1/4 分支因子）实现严格字典序 O(log N) 读写与范围扫描：

- **常量与枚举**：
  - `SKIPLIST_MAX_NODES = 256`、`SKIPLIST_MAX_LEVEL = 12`、`SKIPLIST_ARENA_BYTES = 8192`、`SKIPLIST_NO_NODE = 0xFFFF`、`SKIPLIST_DEFAULT_SEED`
  - `SkipListWriteStatus`（`ok | empty_key | node_pool_full | arena_full`）
  - `SkipListGetKind`（`found | tombstone | not_found`）
- **数据模型**：
  - `SkipListNode`、`SkipListEntryView`、`SkipListGetResult`、`SkipListStats`（`total_nodes`, `live_keys`, `tombstone_keys`, `current_max_level`, `arena_used_bytes`, `arena_util_permille`, `node_util_permille`, `total_comparisons`, `next_version`）
- **核心引擎 `ArenaSkipList`**：
  - `init()` / `initWithSeed(seed)` / `clear(*_)`
  - `put(*_, key, value): SkipListWriteStatus`：O(log N) 字典序插入或更新（同键新值 `<= val_cap` 零额外 Arena 原地覆写，否则前置原子校验扩容，杜绝半写损坏；单调递增 `version` 并可复活墓碑键）；
  - `delete(*_, key): SkipListWriteStatus`：写入墓碑删除标记（含独立墓碑插入以遮蔽下层 SSTable）；
  - `get(*_, key)` / `getConst(key)`：O(log N) 精确点查；
  - `seek(lower_key)` / `nextNode(idx)` / `entryAt(idx)` / `rangeScan(start_key, end_key, include_tombstones, *out_entries)`：前向游标与 `[start_key, end_key)` 区间扫描；
  - `keySlice(entry)` / `valueSlice(entry)` / `shouldFlush(threshold_permille)` / `stats()`。

```cp
import {
    SkipListEntryView,
    ArenaSkipList,
} from "cap:db";

pub fn demoArenaSkipList() {
    let sl = ArenaSkipList.initWithSeed(42);
    * = sl.put("user:03", "charlie");
    * = sl.put("user:01", "alice");
    * = sl.put("user:02", "bob");
    * = sl.delete("user:02"); // 写入墓碑

    let entries: [4]SkipListEntryView = [SkipListEntryView{}; 4];
    let entries_s: []SkipListEntryView = entries[0..];
    const n = sl.rangeScan("user:01", "user:99", false, *entries_s);
    // 按字典序返回 ["user:01", "user:03"] (跳过墓碑 user:02)
    * = n;
    * = sl.shouldFlush(750);
}
```

---

### 零分配 RocksDB/LevelDB 前缀压缩数据块与稀疏块索引二分查找引擎 (`sstableIndex.cp`)

面向 LSM-Tree SSTable 磁盘数据块格式的 100% 纯 CP 零动态堆分配前缀压缩编解码与稀疏块索引引擎：

- **常量与枚举**：
  - `PREFIX_BLOCK_DEFAULT_RESTART_INTERVAL = 16`、`PREFIX_BLOCK_MAX_BYTES = 4096`、`PREFIX_BLOCK_MAX_RESTARTS = 128`、`PREFIX_BLOCK_MAX_KEY_LEN = 256`、`SST_INDEX_MAX_BLOCKS = 64`、`SST_INDEX_MAX_KEY_LEN = 256`
  - `PrefixBlockStatus`（`ok | empty_key | key_too_long | out_of_order_key | already_finished | restarts_full | buffer_full | index_full`）
  - `PrefixBlockLookupStatus`（`found | not_found | buffer_too_small | corrupt_block`）
- **核心类型**：
  - `PrefixBlockBuilder`：每 `restart_interval` 条写入完整 Restart Point，中间记录以 `(shared_len, unshared_len, val_len)` LEB128 varint 前缀压缩编码，封口 `finish()` 写入 4B 小端 Restart Offset 数组与 4B count；强制严格递增键校验与前置空间原子预检，提供 `stats(): PrefixBlockStats` 压缩率遥测。
  - `PrefixBlockReader` / `PrefixBlockCursor`：`open(data)` 完整校验截断、越界/非单调 Restart Offset 与 `shared_len == 0` 约束；`get(key, out_key, out_val)` 与 `seek(lower_key)` 执行 O(log R) Restart Point 二分查找 + 线性精确定位，零动态堆分配前向迭代。
  - `SstSparseIndex`：定长 64 块围栏键（`last_key`）稀疏索引，支持 `addBlock` 严格递增围栏键写入、O(log B) 二分 `locateBlock(key)` 与区间裁剪 `locateRange(start_key, end_key)`。

```cp
import {
    PrefixBlockBuilder,
    PrefixBlockReader,
    SstSparseIndex,
} from "cap:db";

pub fn demoSstablePrefixIndex() {
    let builder = PrefixBlockBuilder.init(4);
    * = builder.add("tenant:acme:user:001", "alice");
    * = builder.add("tenant:acme:user:002", "bob");
    * = builder.add("tenant:acme:user:003", "carol");
    const block = builder.finish();

    let idx = SstSparseIndex.init();
    * = idx.addBlock(builder.lastKeySlice(), 0, @as(u32, @intCast(block.len)), builder.stats().entry_count);

    if (reader := PrefixBlockReader.open(block)) {
        let kbuf: [64]u8 = [0; 64];
        let vbuf: [64]u8 = [0; 64];
        let ks: []u8 = kbuf[0..];
        let vs: []u8 = vbuf[0..];
        const hit = reader.get("tenant:acme:user:002", *ks, *vs);
        * = hit.found;
    }
}
```

---

### 零分配 ARIES 风格 WAL 模糊检查点与安全日志截断协调器 (`walCheckpt.cp`)

面向关系型/KV 存储引擎崩溃恢复与 WAL 日志安全回收的 100% 纯 CP 零动态堆分配 ARIES 模糊检查点协调器：

- **常量与枚举**：
  - `WAL_CKPT_MAX_DIRTY_PAGES = 128`、`WAL_CKPT_MAX_ACTIVE_TXNS = 32`、`WAL_CKPT_MAX_SEGMENTS = 32`
  - `WalCheckptOpStatus`（`ok | invalid_arg | duplicate_id | capacity_full | not_found | segment_not_reclaimable`）
- **数据模型**：
  - `WalDirtyPageEntry`（`page_id`, `rec_lsn`, `last_lsn`）、`WalActiveTxnEntry`（`tx_id`, `pinned_lsn`）、`WalSegmentEntry`（`seg_id`, `start_lsn`, `end_lsn`, `size_bytes`, `closed`）
  - `WalCheckpointPlan`（`redo_lsn`, `min_active_tx_lsn`, `safe_truncate_lsn`, `reclaimable_seg_count`, `reclaimable_bytes`, `reclaimableIds()`）、`WalCheckpointStats`
- **核心引擎 `WalCheckpointCoordinator`**：
  - 脏页表管理：`markPageDirty(*_, page_id, lsn)`（保留首次 `rec_lsn`、推进 `last_lsn`）、`markPageFlushed(*_, page_id, flushed_lsn)`（仅当 `flushed_lsn >= last_lsn` 清除脏页，防止模糊刷盘期间并发新写丢失）、`getDirtyPage(page_id)`。
  - 活跃事务/快照钉住：`beginTxn(*_, tx_id, pinned_lsn)` / `endTxn(*_, tx_id)`。
  - WAL 段注册与安全回收：`registerSegment` / `closeSegment` / `reclaimSegment` / `reclaimSafeSegments`。
  - ARIES 水位规划与提交：`computeCheckpointPlan(current_flushed_lsn)`（`redo_lsn = min(DPT.rec_lsn)`，`safe_truncate_lsn = min(redo_lsn, min(active_tx.pinned_lsn))`，自动按 `end_lsn` 升序列出全部已关闭且 `end_lsn < safe_truncate_lsn` 的可回收段）、`commitCheckpoint(*_, checkpoint_lsn, current_flushed_lsn)`、`stats(current_flushed_lsn)`。

```cp
import { WalCheckpointCoordinator } from "cap:db";

pub fn demoWalCheckpoint() {
    let ckpt = WalCheckpointCoordinator.init();
    * = ckpt.registerSegment(1, 1, 100, 4096, true);
    * = ckpt.registerSegment(2, 101, 200, 4096, true);
    * = ckpt.markPageDirty(10, 120);
    * = ckpt.beginTxn(1, 150);

    const plan = ckpt.commitCheckpoint(220, 220);
    // redo_lsn == 120, safe_truncate_lsn == 120 -> 段 1 (end_lsn 100 < 120) 可安全回收!
    * = ckpt.reclaimSafeSegments(plan.safe_truncate_lsn);
}
```

### 零分配 Sans-IO Raft 选主、Pre-Vote 防分区扰动与法定人数提交状态机 (`raftVote.cp`)

与 `raftLog.cp` 互补的 100% 纯 CP 零动态堆分配 Sans-IO Raft 共识选主状态机：

- **常量与消息模型**：
  - `RAFT_MAX_PEERS = 15`、`RAFT_NO_NODE = 0`
  - `RaftRole`（`follower | pre_candidate | candidate | leader`；配套 `formatRaftRole`）
  - `RaftVoteRejectReason`（`none | stale_term | already_voted | log_not_up_to_date | leader_lease_active`）
  - `RaftVoteRequest`、`RaftVoteResponse`、`RaftHeartbeat`、`RaftPeerProgress`、`RaftElectionStats`
  - `isLogUpToDate(cand_last_term, cand_last_index, local_last_term, local_last_index): bool`（Raft §5.4.1 日志最新性比较）
- **核心引擎 `RaftElectionNode`**：
  - `init(node_id, cluster_size, election_timeout_ms, heartbeat_interval_ms, enable_pre_vote)` / `quorumSize()` / `isLeaderLeaseActive(now_ms)`
  - `tick(*_, now_ms, last_log_term, last_log_index): RaftVoteRequest?`（选举超时自动触发 Pre-Vote 或正式 RequestVote）
  - `handleVoteRequest(*_, req, now_ms, local_last_term, local_last_index): RaftVoteResponse`（Leader Lease 防惊群、Pre-Vote 不递增任期/不落盘 `voted_for`、§5.4.1 日志最新性校验）
  - `handleVoteResponse(*_, resp, now_ms, last_log_term, last_log_index): RaftVoteRequest?`（Pre-Vote 达法定人数自动晋级正式 Candidate；正式 Candidate 达法定人数自动当选 Leader；更高任期立即退位）
  - `handleHeartbeat(*_, hb, now_ms)` / `recordPeerAck(*_, peer_id, match_index, now_ms)` / `computeQuorumCommitIndex(local_last_index)` / `advanceCommitIndex(*_, local_last_index)` / `stepDown(*_, new_term)` / `stats()`

```cp
import {
    RaftRole,
    RaftElectionNode,
} from "cap:db";

pub fn demoRaftElection() {
    let n1 = RaftElectionNode.init(1, 3, 300, 100, true) ?? return;
    let n2 = RaftElectionNode.init(2, 3, 300, 100, true) ?? return;

    // 超时发起 Pre-Vote -> n2 同意 -> n1 晋级正式 Candidate -> n2 投正式票 -> n1 当选 Leader
    if (pre_req := n1.tick(500, 1, 10)) {
        const pre_resp = n2.handleVoteRequest(pre_req, 500, 1, 10);
        if (vote_req := n1.handleVoteResponse(pre_resp, 510, 1, 10)) {
            const vote_resp = n2.handleVoteRequest(vote_req, 510, 1, 10);
            * = n1.handleVoteResponse(vote_resp, 520, 1, 10);
        }
    }
    * = (n1.stats().role == RaftRole.leader);
}
```

### 零分配关系查询优化器内核：MCV+等深直方图、System-R/Selinger DP 与无上限多表连接规划 (`queryCost.cp`)

面向关系型数据库与分析型查询优化器（System R / PostgreSQL `pg_statistic` 成本模型）的 100% 纯 CP 零动态堆分配查询优化器引擎，支持外部切片驱动的无上限直方图与多表连接规划：

- **常量、错误集与枚举**：
  - `QCOST_MAX_TABLES = 8`、`QCOST_DP_MAX_TABLES = 14`（`1 << 14 = 16,384` DP 状态）、`QCOST_BPS_SCALE = 10000`
  - `QueryOptError`（`InvalidHistogram | InvalidTables | BufferTooSmall | UnsupportedDpTableCount | InvalidEdge`）
  - `PredicateKind`、`AccessPathKind`、`JoinAlgoKind`
- **基础选择率与物理路径择优**：
  - `CostModelParams`、`TableStats`、`AccessPathCost`、`JoinPlanCost`、`JoinOrderPlan`
  - `estimatePredicateSelectivity` / `combineSelectivityAnd` / `combineSelectivityOr`
  - `chooseBestAccessPath`（`seq_scan` / `index_scan` / `index_only_scan`）与 `chooseBestJoinAlgo`（`nested_loop` / `hash_join` / `merge_join`）
  - `planLeftDeepJoinOrder(tables, join_selectivity, params)`（向后兼容 8 表固定缓冲左深贪心规划）
- **PostgreSQL `pg_statistic` 风格 MCV + Equi-Depth 等深直方图引擎**：
  - `McvEntry`、`HistBucket`、`ColumnHistogram`（外部传入 `mcvs` 与 `buckets` 切片）
  - `buildEquiDepthHistogram(sorted_values, num_buckets, out_buckets): usize ! QueryOptError`（从任意长度已排序样本构建等深桶与桶内精确 NDV）
  - `estimateEqSelectivityHist(hist, target): f64`（先命中 MCV 精确返回倾斜频率，未命中按剩余非 MCV 概率质量均摊）
  - `estimateRangeSelectivityHist(hist, low, high, include_low, include_high): f64`（叠加区间内 MCV 频率 + 全覆盖桶贡献 + 边界桶线性插值，消除倾斜数据 10x~100x 误估）
- **System-R / Selinger 位掩码动态规划连接规划器（Left-Deep & Bushy）**：
  - `JoinGraphEdge`、`DpJoinState`、`DpJoinPlanResult`、`dpJoinWorkRequiredLen(num_tables)`
  - `planOptimalJoinOrderDp(tables, edges, allow_bushy, params, dp_work, out_eval_order): DpJoinPlanResult ! QueryOptError`（词典序优先最小化笛卡尔积再最小化总代价，支持左深树与 Bushy 树全局最优搜索）
- **无上限外部切片贪心连接规划器（32 / 64+ 表大查询）**：
  - `planGreedyJoinOrderUnbounded(tables, edges, params, used_flags, out_order, out_algos): DpJoinPlanResult ! QueryOptError`

```cp
import {
    CostModelParams,
    TableStats,
    McvEntry,
    HistBucket,
    ColumnHistogram,
    buildEquiDepthHistogram,
    estimateEqSelectivityHist,
    estimateRangeSelectivityHist,
    JoinGraphEdge,
    DpJoinState,
    planOptimalJoinOrderDp,
} from "cap:db";

pub fn demoQueryCostPlanner()! {
    // 1. MCV + Equi-Depth 直方图精准处理倾斜数据
    const sorted_tail: [8]f64 = [10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0];
    let buckets: [4]HistBucket = [HistBucket{}; 4];
    const nb = buildEquiDepthHistogram(sorted_tail[0..], 4, buckets[0..])?;
    const mcvs: [1]McvEntry = [McvEntry{ value: 1.0, freq: 0.60 }];
    const hist = ColumnHistogram{
        mcvs: mcvs[0..],
        buckets: buckets[0..nb],
        null_frac: 0.0,
        ndv: 1000.0,
        total_rows: 1_000_000.0,
    };
    * = estimateEqSelectivityHist(hist, 1.0); // ~0.60 (精确命中 MCV)
    * = estimateRangeSelectivityHist(hist, 0.0, 45.0, true, true);

    // 2. System-R / Selinger Bushy DP 全局最优连接规划
    const tables: [4]TableStats = [
        TableStats{ table_id: 1, row_count: 10000.0, page_count: 200.0 },
        TableStats{ table_id: 2, row_count: 5000.0, page_count: 100.0 },
        TableStats{ table_id: 3, row_count: 10000.0, page_count: 200.0 },
        TableStats{ table_id: 4, row_count: 5000.0, page_count: 100.0 },
    ];
    const edges: [3]JoinGraphEdge = [
        JoinGraphEdge{ left_idx: 0, right_idx: 1, selectivity: 0.0001 },
        JoinGraphEdge{ left_idx: 2, right_idx: 3, selectivity: 0.0001 },
        JoinGraphEdge{ left_idx: 1, right_idx: 2, selectivity: 0.01 },
    ];
    let dp_work: [16]DpJoinState = [DpJoinState{}; 16];
    let order: [4]u16 = [0; 4];
    const res = planOptimalJoinOrderDp(tables[0..], edges[0..], true, CostModelParams.defaults(), dp_work[0..], order[0..])?;
    * = res.total_cost;
}
```

---

## 零堆无上限 LSM-Tree + CRC32 WAL + SSTable + Leveled Compaction 存储引擎（`lsmEngine.cp`）

> 对标加固基准：`fjall-rs/lsm-tree`（MIT / Apache-2.0）

面向嵌入式 KV 与本地持久化引擎的 100% 纯 CP 零动态堆分配、外部切片驱动 LSM-Tree 核心（无固定键值对或合并路数上限）：

- **SIMD 与校验和热路径加速**：
  - `compareBytes` / `commonPrefixLen` / `copySlice`：16-lane `@Vector(16, u8)` 向量化加速（与导出标量参考 `compareBytesScalar` / `commonPrefixLenScalar` 100% 逐字节等价）。
  - `crc32Ieee`：4-bit LUT 加速 IEEE 802.3 CRC32（与导出标量参考 `crc32IeeeScalar` 100% 逐位一致）。
  - `hasPrefix(key, prefix)` / `computePrefixUpperBound(prefix, *out)`：对标 `fjall-rs/lsm-tree` `prefix_upper_range` 语义，精确处理空前缀与全 `0xFF` 边界。
- **Write-Ahead Log（`LsmWalCodec`）**：
  - `encodePut(key, val, seq, *out)` / `encodeDelete(key, seq, *out)` / `encodeRangeDelete(start_key, end_key, seq, *out)` / `decodeFrameAt(log, offset)`：带魔数 `LSM_WAL_MAGIC`、版本、长度前缀、保留字节校验、溢出安全边界校验与全帧 IEEE CRC32。
  - `recoverLog(log, *out_records): LsmWalRecoveryStats` / `replayIntoMemTable(log, *mem)`：顺序扫描恢复（支持 Put / Delete / RangeDelete），遇到尾部半写或损坏帧时自动安全截断并汇报 `recovered_records`、`valid_bytes`、`corrupt_tail_bytes` 与 `max_seq`。
- **MVCC 有序 MemTable（`LsmMemTable`）**：
  - 由调用方 `LsmMemBuffers`（`entries`、`order`、`arena`）外部切片驱动，严格按 `(key ASC, seq DESC)` 维护内部序，内置 `LSM_MAX_RANGE_TOMBSTONES`（32）范围墓碑追踪。
  - `put(key, val, seq)` / `delete(key, seq)` / `deleteRange(start_key, end_key, seq)` / `getAt(key, snapshot_seq)`（MVCC 快照隔离点查，自动裁决点写与范围墓碑的 seq 覆盖关系）/ `scanRange` / `scanPrefix` / `scanRangeReverse` / `flushToSst(...)`。
- **块式 SSTable 与内置 Bloom Filter（`SstBuilder` / `SstReader` / `SstCursor`）**：
  - **Data Blocks**：增量前缀压缩（`shared_prefix_len` + `unshared_key`）+ 每块 4 字节 IEEE CRC32 + 严格溢出安全与畸形帧拦截。
  - **Index Block**：记录每块 `last_key` 与偏移，支持二分定位 + 块级 CRC32 + `block_count` 头一致性校验。
  - **Bloom Filter Block**：Kirsch-Mitzenmacher 双重哈希 `k` 轮探测（默认 10 bits/key、`k=7`，假阳性率 `< 1%`），`mayContain` 快速拒绝不存在的 Key。
  - **64 字节 Footer & 全文件校验**：`verifyFullIntegrity(*key_scratch): SstVerifyReport` 校验全部 Data Block 边界/CRC32/前缀单调性与 Bloom 零假阴性不变量；支持 `scanRange` / `scanPrefix` / `scanRangeReverse`。
- **K-Way Leveled Compaction（`compactSstables`）**：
  - 支持任意 K 路 SSTable 归并（调用方提供 `cursors_work: []SstCursor` 与 `key_bufs_work`），在尊重 `min_active_snapshot_seq`（`gc_watermark_seqno`）的前提下自动去重重复 `(key, seq)`、按 `range_tombstones: []const LsmRangeDeleteBound` 裁剪过期键、回收被覆盖旧版本并在 `drop_tombstones = true`（底层）时清除过期 Tombstone。
- **端到端统一引擎（`LsmDbEngine`）**：
  - 写入与持久化：`put` / `delete` / `deleteRange`（先写 WAL 再写 MemTable）、`flushMemTableToSst`、`recoverFromWal`。
  - MVCC 快照生命周期与 GC 水位线：`openSnapshot(*_): LsmSnapshotHandle`、`releaseSnapshot(*_, handle)`、`activeSnapshotCount()`、`gcWatermarkSeqno(): u64`（返回最老活跃快照 seqno，无活跃快照时返回 `u64::MAX`）。
  - 分层快照读与扫描：`getLayered`、`scanRangeLayered`、`scanPrefixLayered`、`scanRangeReverseLayered`（`MemTable -> 新 SSTable -> 旧 SSTable`）。

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

## 历史数据库存储域散装模块收敛（W29-Sync R7）

- `cap:lsm_sst` -> `lib/db/lsm{Block,Bloom,Sstable}.cp`：导出 `SstError`, `LsmBlockHandle`, `BlockBuilder`, `BlockReader`, `BlockIter`, `BloomFilter`, `buildBloomFilter`, `mayContain`, `SST_MAGIC`, `LSM_SST_FOOTER_SIZE`, `BLOCK_TRAILER_SIZE`, `SstOptions`, `SstEntry`, `SstOwnedEntry`, `SstGetResult`, `LsmSstBuilder`, `LsmSstReader`, `SstMergeIter`。
