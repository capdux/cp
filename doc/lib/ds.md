# CP 标准库：通用数据结构、空间索引、字符串匹配与概率草图手册 (`cap:ds` / `cap.ds`)

## 一、 架构总览与设计准则

`cap:ds` 是 CP 语言官方内置的纯原生高性能通用数据结构与算法集合，覆盖：
1. **有序树、区间查询与并查集**（`treap.cp`, `rmq.cp`, `skiplist.cp`, `segtree.cp`, `dsu.cp`, `interval_tree.cp`）
2. **多维空间索引**（`quadtree.cp`, `kdtree.cp`, `rtree.cp`）
3. **字符串匹配与大文本编辑缓冲**（`suffix.cp`, `kmp.cp`, `aho_corasick.cp`, `rope.cp`）
4. **双向映射、环形缓冲、位图与概率草图**（`bimap.cp`, `circ_buf.cp`, `bitset.cp`, `cuckoo.cp`, `hll.cp`, `roaring.cp`, `tdigest.cp`）

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:ds"` 与 `import cap from "cap"; cap.ds.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 核心职责 |
| :--- | :--- |
| `lib/ds/treap.cp` | 零分配随机平衡树（Treap）有序集合/字典与次序统计 |
| `lib/ds/rmq.cp` | Sparse Table `O(1)` RMQ（Min/Max/ArgMin/GCD）、欧拉序 LCA 与滑动窗口极值 |
| `lib/ds/skiplist.cp` | 32 层有序跳表 MemTable 与范围扫描索引 |
| `lib/ds/segtree.cp` | 树状数组（`FenwickTree`）与懒标记线段树（`LazySegmentTree`）区间查询/更新 |
| `lib/ds/dsu.cp` | 路径压缩并查集（`DisjointSet`）、带权并查集（`WeightedDsu`）与可撤销并查集（`RollbackDsu`） |
| `lib/ds/interval_tree.cp` | 泛型增广区间树（重叠区间检索与点刺查询） |
| `lib/ds/quadtree.cp` | 动态分裂/合并四叉树（2D 空间索引与范围/近邻查询） |
| `lib/ds/kdtree.cp` | 多维 K-D Tree 空间索引与 K 近邻（KNN）搜索 |
| `lib/ds/rtree.cp` | 2D AABB R-Tree 空间矩形索引与最近邻检索 |
| `lib/ds/suffix.cp` | 后缀数组、Kasai LCP 数组、模式范围搜索、不同子串计数与最长重复子串 |
| `lib/ds/kmp.cp` | KMP 前缀函数、模式搜索、最短周期、Z-Algorithm 与 Manacher 最长回文子串 |
| `lib/ds/aho_corasick.cp` | Aho-Corasick 多模式自动机线性扫描匹配 |
| `lib/ds/rope.cp` | 平衡 Rope 文本缓冲（`O(log n)` 拼接、切分、插入、删除与行索引） |
| `lib/ds/bimap.cp` | 泛型双向 1-to-1 映射 `BiMap(K, V)` 与 `StringBiMap` |
| `lib/ds/circ_buf.cp` | 泛型环形队列/循环缓冲区 `RingBuffer(T)` |
| `lib/ds/bitset.cp` | 64-bit 字加速位集合（`BitSet`，按位与/或/异或/计数/前导搜索） |
| `lib/ds/cuckoo.cp` | 4 槽 16-bit 指纹 Cuckoo Filter（支持动态删除的概率成员过滤） |
| `lib/ds/hll.cp` | HyperLogLog 基数估算器（低内存去重计数） |
| `lib/ds/roaring.cp` | Roaring Bitmap 压缩 32-bit 整型集合引擎（Array/Bitmap/Run 容器） |
| `lib/ds/tdigest.cp` | T-Digest 流式分位数估算（极端尾延迟 p99/p99.9 高精度） |
| `lib/ds/index.cp` | `cap:ds` 与 `cap.ds` 统一零冲突公开导出入口（20 个子模块） |

---

## 二、 核心 API 索引

### 1. 有序树、区间查询与并查集
- **`treap.cp`**：`TREAP_NIL`, `TreapNode`, `TreapEntry`, `Treap`
- **`rmq.cp`**：`floorLog2`, `sparseTableCells`, `gcdU64`, `RmqMin`, `RmqMax`, `RmqArgMin`, `RangeGcd`, `EulerLca`, `slidingWindowMin`, `slidingWindowMax`
- **`skiplist.cp`**：`SkipListError`, `SkipEntry`, `SkipList`
- **`segtree.cp`**：`SegTreeError`, `RangeStats`, `FenwickTree`, `LazySegmentTree`
- **`dsu.cp`**：`DsuError`, `DisjointSet`, `WeightedDsu`, `RollbackDsu`
- **`interval_tree.cp`**：`IntervalError`, `Interval`, `IntervalTree`

### 2. 多维空间索引
- **`quadtree.cp`**：`QuadError`, `QuadEntry`, `QuadHit`, `QuadTree`
- **`kdtree.cp`**：`KdError`, `KdPointEntry`, `KdNeighbor`, `KdTree`
- **`rtree.cp`**：`RTreeError`, `Rect`, `RTreeEntry`, `NearestRect`, `RTree`

### 3. 字符串匹配与文本缓冲
- **`suffix.cp`**：`SuffixWorkspace`, `OccurrenceRange`, `LongestRepeat`, `buildSuffixArray`, `buildLcpArray`, `findPatternRange`, `countDistinctSubstrings`, `longestRepeatedSubstring`
- **`kmp.cp`**：`PalindromeSpan`, `computePrefixFunction`, `kmpFindFirst`, `kmpFindAll`, `kmpCount`, `shortestPeriod`, `computeZArray`, `manacherLongestPalindrome`, `countPalindromicSubstrings`
- **`aho_corasick.cp`**：`Match`, `AhoCorasick`
- **`rope.cp`**：`Rope`, `RopeError`, `Node`, `NodePtr`

### 4. 映射、缓冲、位图与概率草图
- **`bimap.cp`**：`BiMap`, `BiMapError`, `StringBiMap`
- **`circ_buf.cp`**：`RingError`, `RingBuffer`
- **`bitset.cp`**：`BitSet`, `BitSetError`
- **`cuckoo.cp`**：`CuckooError`, `CuckooStats`, `CuckooFilter`
- **`hll.cp`**：`HllError`, `HyperLogLog`
- **`roaring.cp`**：`RoaringError`, `RoaringBitmap`
- **`tdigest.cp`**：`TDigestError`, `Centroid`, `TDigest`

---

## 三、 使用示例

```typescript
import cap from "cap";
import {
    DisjointSet,
    kmpFindFirst,
    RoaringBitmap,
    HyperLogLog,
} from "cap:ds";

pub fn demoDs(alloc: std.mem.Allocator): !void {
    // 1. 并查集
    let dsu = DisjointSet.init(alloc, 4)?;
    defer dsu.deinit();
    * = dsu.unite(0, 1)?;
    * = dsu.connected(0, 1)?;

    // 2. KMP 字符串查找
    const idx = kmpFindFirst("abracadabra", "cada");
    * = idx;

    // 3. Roaring Bitmap 压缩位图
    let bm = RoaringBitmap.init(alloc);
    defer bm.deinit();
    * = bm.insert(42)?;

    // 4. HyperLogLog 基数估算 & cap.ds 动态懒加载
    let hll = cap.ds.HyperLogLog.init(alloc, 12)?;
    defer hll.deinit();
    hll.add("agent-1");
    * = hll.estimate();
}
```

---

## 第 2 波收敛集成的数据结构、CRDT 与检索组件

以下纯 CP 零外部依赖数据结构模块已统一聚合导出至 `cap:ds`（`lib/ds/index.cp`），零符号冲突：

| 分类 | 文件路径 | 核心能力 | 说明 |
| :--- | :--- | :--- | :--- |
| 树与有序集合 | `lib/ds/bptree.cp` | Ordered B+ Tree Index | 泛型 B+ Tree 键值索引 |
| 树与有序集合 | `lib/ds/zset.cp` | SkipList + HashMap SortedSet | 双索引有序集合引擎 |
| 树与有序集合 | `lib/ds/radix.cp` / `lib/ds/radix_router.cp` | Compressed Patricia Radix Tree & Router | 压缩基数树与路由表 |
| 分布式状态与哈希 | `lib/ds/crdt_pn.cp` | GCounter / PNCounter / GSet / LWWRegister | 状态型 CRDT 集合 |
| 分布式状态与哈希 | `lib/ds/ringhash.cp` | ConsistentRing / Jump / Rendezvous HRW | 一致性哈希环与 HRW 路由 |
| 文本驻留/模糊/校验 | `lib/ds/intern.cp` | StringInterner & `InternStringBiMap` | 字符串驻留池与双向映射（别名避让 `bimap.cp`） |
| 文本驻留/模糊/校验 | `lib/ds/fuzzy.cp` | Levenshtein / Damerau / Jaro-Winkler | 模糊匹配与候选排序 |
| 文本驻留/模糊/校验 | `lib/ds/merkle_tree.cp` | SHA-256 Merkle Tree | 默克尔树构建与包容证明验证 |
| 参考归档（不导出） | `lib/ds/{bitset,disjoint_set,fenwick,segment_tree,skip_list,suffix_array,rope,trie_radix}_.cp` | 重叠数据结构（仅参考） | 与 `lib/ds/` 权威实现功能重叠，保留供对比分析，不在 `cap:ds` 重复导出 |

---

## 流式热点探测与高频 Top-K 统计基础库（`heavy_hitter.cp`）

面向实时流量热点分析、缓存准入（W-TinyLFU / Heavy-Hitter 识别）与 AI 高频查询检测的 100% 纯 CP、零堆分配、确定性流式频率估计与热点追踪引擎：

### 1. Space-Saving 热点追踪器（`HeavyHitterEntry` / `HeavyHitterTracker`）
- 经典 Metwally Space-Saving 算法：固定 `K` 槽位（调用方栈/静态切片 `[]HeavyHitterEntry`），`O(K)` 插入与最小计数槽替换，严格保证任意被追踪项满足 `est_count - error_bound <= true_freq <= est_count` 且 `error_bound <= floor(total_weight / K)`：
  - `HeavyHitterTracker.init(slots_buf: *[]HeavyHitterEntry): HeavyHitterTracker?`（空槽返回 `null`）
  - `observe(*_, key: string, weight: u64): bool`（拒绝 `weight == 0`）
  - `topK(out_entries: *[]HeavyHitterEntry): usize`（按 `est_count` 降序、同分按 `key_hash` 升序确定性输出）
  - `lookup(key: string): HeavyHitterEntry?` / `guaranteedMinCount(key: string): u64`
  - `totalWeight(): u64` / `len(): usize` / `capacity(): usize` / `evictionCount(): u64` / `reset(*_)`

### 2. Conservative-Update Count-Min Sketch（`CmsSketchConfig` / `ConservativeCms`）
- 调用方 `[]u32` 扁平缓冲零堆承载（`depth × width`），采用 Conservative Update（仅递增小于 `min_val + delta` 的行，显著压低高偏斜 Zipf 分布下的过估计误差）：
  - `CmsSketchConfig.init(depth, width, seed = 0x517CC1B727220A95): CmsSketchConfig?` / `isValid()` / `requiredCells(): usize?`
  - `ConservativeCms.init(counters_buf: *[]u32, cfg: CmsSketchConfig): ConservativeCms?`
  - `add(*_, key: string, delta: u32): bool`（拒绝 `delta == 0`）
  - `estimate(key: string): u32`
  - `mergeFrom(*_, other: ConservativeCms): bool`（分布式分片饱和聚合，校验 `depth`/`width`/`seed` 一致）
  - `decayHalf(*_)`（滑动时间窗指数半衰 `>> 1`）/ `reset(*_)` / `totalCount()` / `rowCount()` / `colCount()`

### QuickStart 示例

```cp
import {
    HeavyHitterEntry,
    HeavyHitterTracker,
    CmsSketchConfig,
    ConservativeCms,
} from "cap:ds";

pub fn demoHeavyHitterAndCms() {
    // 1. Space-Saving 零堆热点追踪器（固定 8 槽位）
    let slots: [8]HeavyHitterEntry = [HeavyHitterEntry{}; 8];
    let slots_s: []HeavyHitterEntry = slots[0..];
    let tracker = HeavyHitterTracker.init(slots_s) ?? return;

    * = tracker.observe("/api/chat", 500);
    * = tracker.observe("/api/embed", 200);
    * = tracker.observe("/health", 10);

    let top: [2]HeavyHitterEntry = [HeavyHitterEntry{}; 2];
    let top_s: []HeavyHitterEntry = top[0..];
    const n_top = tracker.topK(top_s); // top[0].label == "/api/chat"

    // 2. Conservative-Update Count-Min Sketch（4 x 256 栈缓冲）
    const cfg = CmsSketchConfig.init(4, 256) ?? return;
    let buf: [1024]u32 = [0; 1024];
    let buf_s: []u32 = buf[0..];
    let cms = ConservativeCms.init(buf_s, cfg) ?? return;
    * = cms.add("prompt:summarize", 42);
    const est = cms.estimate("prompt:summarize"); // >= 42
    * = n_top;
    * = est;
}
```
