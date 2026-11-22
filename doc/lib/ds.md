# CP 标准库：通用数据结构、空间索引、字符串匹配与概率草图手册 (`cap:ds` / `cap.ds`)

## 一、 架构总览与设计准则

`cap:ds` 是 CP 语言官方内置的纯原生高性能通用数据结构与算法集合，覆盖：
1. **有序树、区间查询与并查集**（`treap.cp`, `rmq.cp`, `skiplist.cp`, `segtree.cp`, `dsu.cp`, `intervalTree.cp`）
2. **多维空间索引**（`quadtree.cp`, `kdtree.cp`, `rtree.cp`）
3. **字符串匹配与大文本编辑缓冲**（`suffix.cp`, `kmp.cp`, `ahoCorasick.cp`, `rope.cp`）
4. **双向映射、环形缓冲、位图与概率草图**（`bimap.cp`, `circBuf.cp`, `bitset.cp`, `cuckoo.cp`, `hll.cp`, `roaring.cp`, `tdigest.cp`）

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:ds"` 与 `import cap from "cap"; cap.ds.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 核心职责 |
| :--- | :--- |
| `lib/ds/treap.cp` | 零分配随机平衡树（Treap）有序集合/字典与次序统计 |
| `lib/ds/rmq.cp` | Sparse Table `O(1)` RMQ（Min/Max/ArgMin/GCD）、欧拉序 LCA 与滑动窗口极值 |
| `lib/ds/skiplist.cp` | 32 层有序跳表 MemTable 与范围扫描索引 |
| `lib/ds/segtree.cp` | 树状数组（`FenwickTree`）与懒标记线段树（`LazySegmentTree`）区间查询/更新 |
| `lib/ds/dsu.cp` | 路径压缩并查集（`DisjointSet`）、带权并查集（`WeightedDsu`）与可撤销并查集（`RollbackDsu`） |
| `lib/ds/intervalTree.cp` | 泛型增广区间树（重叠区间检索与点刺查询） |
| `lib/ds/quadtree.cp` | 动态分裂/合并四叉树（2D 空间索引与范围/近邻查询） |
| `lib/ds/kdtree.cp` | 多维 K-D Tree 空间索引与 K 近邻（KNN）搜索 |
| `lib/ds/rtree.cp` | 2D AABB R-Tree 空间矩形索引与最近邻检索 |
| `lib/ds/suffix.cp` | 后缀数组、Kasai LCP 数组、模式范围搜索、不同子串计数与最长重复子串 |
| `lib/ds/kmp.cp` | KMP 前缀函数、模式搜索、最短周期、Z-Algorithm 与 Manacher 最长回文子串 |
| `lib/ds/ahoCorasick.cp` | Aho-Corasick 多模式自动机线性扫描匹配 |
| `lib/ds/rope.cp` | 平衡 Rope 文本缓冲（`O(log n)` 拼接、切分、插入、删除与行索引） |
| `lib/ds/bimap.cp` | 泛型双向 1-to-1 映射 `BiMap(K, V)` 与 `StringBiMap` |
| `lib/ds/circBuf.cp` | 泛型环形队列/循环缓冲区 `RingBuffer(T)` |
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
- **`intervalTree.cp`**：`IntervalError`, `Interval`, `IntervalTree`

### 2. 多维空间索引
- **`quadtree.cp`**：`QuadError`, `QuadEntry`, `QuadHit`, `QuadTree`
- **`kdtree.cp`**：`KdError`, `KdPointEntry`, `KdNeighbor`, `KdTree`
- **`rtree.cp`**：`RTreeError`, `Rect`, `RTreeEntry`, `NearestRect`, `RTree`

### 3. 字符串匹配与文本缓冲
- **`suffix.cp`**：`SuffixWorkspace`, `OccurrenceRange`, `LongestRepeat`, `buildSuffixArray`, `buildLcpArray`, `findPatternRange`, `countDistinctSubstrings`, `longestRepeatedSubstring`
- **`kmp.cp`**：`PalindromeSpan`, `computePrefixFunction`, `kmpFindFirst`, `kmpFindAll`, `kmpCount`, `shortestPeriod`, `computeZArray`, `manacherLongestPalindrome`, `countPalindromicSubstrings`
- **`ahoCorasick.cp`**：`Match`, `AhoCorasick`
- **`rope.cp`**：`Rope`, `RopeError`, `Node`, `NodePtr`

### 4. 映射、缓冲、位图与概率草图
- **`bimap.cp`**：`BiMap`, `BiMapError`, `StringBiMap`
- **`circBuf.cp`**：`RingError`, `RingBuffer`
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
| 树与有序集合 | `lib/ds/radix.cp` / `lib/ds/radixRouter.cp` | Compressed Patricia Radix Tree & Router | 压缩基数树与路由表 |
| 分布式状态与哈希 | `lib/ds/crdtPn.cp` | GCounter / PNCounter / GSet / LWWRegister | 状态型 CRDT 集合 |
| 分布式状态与哈希 | `lib/ds/ringhash.cp` | ConsistentRing / Jump / Rendezvous HRW | 一致性哈希环与 HRW 路由 |
| 文本驻留/模糊/校验 | `lib/ds/intern.cp` | StringInterner & `InternStringBiMap` | 字符串驻留池与双向映射（别名避让 `bimap.cp`） |
| 文本驻留/模糊/校验 | `lib/ds/fuzzy.cp` | Levenshtein / Damerau / Jaro-Winkler | 模糊匹配与候选排序 |
| 文本驻留/模糊/校验 | `lib/ds/merkle.cp` | SHA-256 Merkle Tree | 默克尔树构建与包容证明验证 |
| 参考归档（不导出） | `lib/ds/{bitset,disjoint_set,fenwick,segment_tree,skip_list,suffix_array,rope,trie_radix}_.cp` | 重叠数据结构（仅参考） | 与 `lib/ds/` 权威实现功能重叠，保留供对比分析，不在 `cap:ds` 重复导出 |

---

## 流式热点探测与高频 Top-K 统计基础库（`heavyHitter.cp`）

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

---

## 零分配 Fenwick 树状数组与 O(log N) 动态顺序统计/加权采样引擎（`fenwick.cp`）

面向实时动态前缀和、流式第 K 小顺序统计（Binary Lifting）与 O(log N) 动态加权随机采样（如动态权重路由表、优先级队列与词表采样）的 100% 纯 CP 零动态堆分配 Fenwick / Binary Indexed Tree 内核：

- **常量与统计**：`FENWICK_MAX_LEN = 1024`、`FenwickStats`（`len`, `total_sum`, `updates_count`）。
- **有符号前缀/区间和树（`FenwickTreeI64`）**：
  - `init(len)` / `build_from_slice(values)`（严格 $O(N)$ 线性建树）
  - `add(*_, idx, delta)` / `set(*_, idx, new_val)` / `point_value(idx)` / `prefix_sum(idx_inclusive)` / `range_sum(lo, hi_inclusive)` / `total_sum()` / `stats()`。
- **非负顺序统计与加权采样树（`FenwickOrderStat`）**：
  - `init(len)` / `add_weight(*_, idx, delta)` / `sub_weight(*_, idx, delta)`（防下溢，拒绝减成负数）
  - `prefix_weight(idx_inclusive)` / `total_weight()`
  - `find_lower_bound(target_cumulative: u64): usize?`：严格 $O(\log N)$ 二进制倍增查找满足 `prefix_weight(idx) >= target_cumulative` 的最小索引（自动跳过零权重空洞）。
  - `sample_weighted_index(rng_val: u64): usize?`：将任意 `u64` 均匀映射到 `[1, total_weight()]` 并按权重比例 $O(\log N)$ 采样。

### QuickStart 示例

```cp
import {
    FenwickTreeI64,
    FenwickOrderStat,
} from "cap:ds";

pub fn demoFenwickOrderStat() {
    // 1. O(N) 线性建树 + 动态点更与区间和
    const vals: [5]i64 = [10, -3, 7, 4, -2];
    let ft = FenwickTreeI64.build_from_slice(vals[0..]) ?? return;
    * = ft.add(1, 5);                 // vals[1]: -3 -> 2
    const rsum = ft.range_sum(1, 3);  // 2 + 7 + 4 = 13

    // 2. O(log N) 动态第 K 小与加权路由采样
    let os = FenwickOrderStat.init(4) ?? return;
    * = os.add_weight(0, 10);
    * = os.add_weight(2, 30); // idx 1 为零权重空洞
    * = os.add_weight(3, 60);
    const kth = os.find_lower_bound(11) ?? 0; // 自动跳过 idx 1 -> 返回 2
    const sampled = os.sample_weighted_index(123456789) ?? 0;
    * = rsum;
    * = kth;
    * = sampled;
}
```

---

## 零分配增强区间树、重叠/刺穿查询与连续区间合并引擎（`augInterval.cp`）

面向调度空闲槽位、内存页空洞查找、时间窗口冲突检测的 100% 纯 CP 零动态堆分配增强区间树与连续区间合并引擎（保留既有 `intervalTree.cp` 向下兼容，全新纯迭代无递归零堆实现）：

- **常量与数据模型**：
  - `AUG_ITREE_MAX_NODES = 256`、`AUG_ITREE_NO_NODE = 0xFFFF`
  - `IntervalSpan`（`start: i64`, `end: i64`, `payload_id: u64`）、`IntervalQueryResult`（`written`, `total_matched`, `truncated`）、`IntervalTreeStats`
- **区间合并与补集空洞提取纯函数**：
  - `coalesceIntervals(in_out_ranges: *[]IntervalSpan, count, merge_adjacent): usize?`：原地排序并合并重叠/相邻区间（`merge_adjacent` 支持合并 `[a, b]` 与 `[b+1, c]` 且防 `i64` 溢出）。
  - `findGaps(ranges, window_start, window_end, out_gaps: *[]IntervalSpan): IntervalQueryResult`：提取闭区间窗口 `[window_start, window_end]` 内所有未覆盖补集空洞。
- **核心引擎 `AugmentedIntervalTree`**：
  - `init()` / `buildFromSlice(items): AugmentedIntervalTree?`（纯迭代中位二分构建严格平衡增强二叉树并自底向上回填 `max_end`）
  - `insert(*_, start, end, payload_id): bool` / `rebuildBalanced(*_)` / `stats()`
  - `queryPoint(point, out_hits)`（刺穿查询）、`queryOverlap(low, high, out_hits)`（重叠查询）、`queryContained(low, high, out_hits)`（包含查询）、`hasAnyOverlap(low, high): bool`（CLRS O(log N) 单路径快检）、`findGapsInWindow(window_start, window_end, out_gaps)`。

### QuickStart 示例

```cp
import {
    IntervalSpan,
    coalesceIntervals,
    AugmentedIntervalTree,
} from "cap:ds";

pub fn demoAugmentedIntervalTree() {
    const items: [3]IntervalSpan = [
        IntervalSpan{ start: 10, end: 20, payload_id: 1 },
        IntervalSpan{ start: 15, end: 30, payload_id: 2 },
        IntervalSpan{ start: 50, end: 60, payload_id: 3 },
    ];
    let tree = AugmentedIntervalTree.buildFromSlice(items[0..]) ?? return;

    let hits: [4]IntervalSpan = [IntervalSpan{}; 4];
    let hits_s: []IntervalSpan = hits[0..];
    const q = tree.queryPoint(18, *hits_s); // total_matched == 2 (payload 1, 2)

    let gaps: [4]IntervalSpan = [IntervalSpan{}; 4];
    let gaps_s: []IntervalSpan = gaps[0..];
    const g = tree.findGapsInWindow(0, 100, *gaps_s); // [0,9], [31,49], [61,100]

    let mut_items: [3]IntervalSpan = items;
    let mut_s: []IntervalSpan = mut_items[0..];
    const merged_n = coalesceIntervals(*mut_s, 3, true) ?? 0; // 2 ([10,30], [50,60])
    * = q;
    * = g;
    * = merged_n;
}
```

### 零分配 Caffeine/Ristretto 风格 W-TinyLFU 频率草图准入与 SLRU 缓存引擎 (`tinyLfu.cp`)

面向高吞吐抗扫描缓存的 100% 纯 CP 零动态堆分配 **W-TinyLFU (Window-TinyLFU)** 缓存引擎：

- **4-bit Count-Min Sketch（`TinyLfuSketch`）**：
  - 4 行独立哈希 × 256 列 4-bit 饱和计数器（`0..=15`，打包于 `[4][16]u64` 共 512 字节）；
  - 周期性 SWAR 并行右移减半 `resetHalve()`（`(w >> 1) & 0x7777777777777777`），防止旧热点僵化。
- **三段 SLRU 缓存布局（`WTinyLfuCache`）**：
  - `Window` + `Probation` + `Protected` 三段 O(1) 静态侵入式双向链表（上限 `TINYLFU_MAX_CAPACITY = 256`）；
  - 新键进入 Window LRU；Window 溢出时与 Main Probation 队尾受害者通过 `sketch.estimate(candidate) >= sketch.estimate(victim)` 决胜准入，拦截一次性冷扫描污染；
  - Probation 二次命中自动晋升 Protected，Protected 溢出自动降级回 Probation。
- **核心导出**：
  - 常量与枚举：`TINYLFU_SKETCH_ROWS` (`4`)、`TINYLFU_SKETCH_WIDTH` (`256`)、`TINYLFU_MAX_COUNT` (`15`)、`TINYLFU_MAX_CAPACITY` (`256`)、`TinyLfuSegment`、`TinyLfuPutOutcome`
  - 结构体：`TinyLfuSketch`、`TinyLfuConfig`、`TinyLfuNode`、`TinyLfuStats`、`WTinyLfuCache`（`init`, `get`, `put`, `remove`, `contains`, `segmentOf`, `estimateFreq`, `len`, `capacity`, `stats`, `clear`）

```cp
import {
    TinyLfuConfig,
    WTinyLfuCache,
} from "cap:ds";

pub fn demoWTinyLfu() {
    const cfg = TinyLfuConfig{
        window_cap: 2,
        probation_cap: 4,
        protected_cap: 6,
        sketch_reset_threshold: 256,
    };
    if (let cache := WTinyLfuCache.init(cfg)) {
        * = cache.put(1001, 42);
        * = cache.get(1001); // 42
        * = cache.stats().hit_rate_bps;
    }
}
```

### 零分配路径压缩字节前缀树 (`prefixTrie.cp`)

面向路由表最长前缀匹配（LPM）、策略前缀索引与字典序前缀范围枚举的 100% 纯 CP 零动态堆分配 **Path-Compressed Byte Prefix Trie (Patricia/Radix KV)**：

- **常量与数据模型**：
  - `PREFIX_TRIE_MAX_NODES = 256`、`PREFIX_TRIE_BYTE_POOL_SIZE = 4096`（4 KiB 内联边字节池）、`PREFIX_TRIE_MAX_KEY_LEN = 128`、`PREFIX_TRIE_NO_NODE = 0xFFFF`
  - `PrefixMatch`（`matched_len`, `value`）、`PrefixScanHit`（`key_buf`, `key_len`, `value`, `keySlice()`）、`PrefixScanReport`（`written`, `total_matched`, `truncated`）、`PrefixTrieStats`
- **核心引擎 `PrefixTrie`**（纯迭代无递归）：
  - `init()`、`insert(*_, key, value): bool`（自动边分裂、零字节浪费原地缩短、满池前置原子检查、同键覆盖更新）
  - `get(key): u64?`（O(L) 精确查找）
  - `remove(*_, key): bool`（O(L) 删除终止标记，自动回收空叶并压缩合并单分支空内部节点）
  - `longestPrefixMatch(query): PrefixMatch?`（O(L) 最长前缀匹配）
  - `scanPrefix(prefix, *out_hits): PrefixScanReport`（纯迭代显式栈，按严格字典序枚举前缀匹配键值并报告缓冲截断统计）
  - `len()` / `isEmpty()` / `clear(*_)` / `stats()`

```cp
import {
    PrefixScanHit,
    PrefixTrie,
} from "cap:ds";

pub fn demoPrefixTrie() {
    let trie = PrefixTrie.init();
    * = trie.insert("/api", 10);
    * = trie.insert("/api/v1/users", 20);
    * = trie.insert("/api/v1/orders", 30);

    if (m := trie.longestPrefixMatch("/api/v1/users/42")) {
        * = m.value; // 20 (matched_len == 13)
    }
    let hits: [4]PrefixScanHit = [PrefixScanHit{}; 4];
    let hits_s: []PrefixScanHit = hits[0..];
    const rep = trie.scanPrefix("/api/v1/", *hits_s);
    * = rep.written; // 2 (字典序: orders, users)
}
```

---

## 收敛子模块深度文档

- [aho_corasick.md](ds/aho_corasick.md)
- [bimap.md](ds/bimap.md)
- [bitset.md](ds/bitset.md)
- [bitset_simd.md](ds/bitset_simd.md)
- [bptree.md](ds/bptree.md)
- [circ_buf.md](ds/circ_buf.md)
- [crdt_pn.md](ds/crdt_pn.md)
- [cuckoo.md](ds/cuckoo.md)
- [disjoint_set.md](ds/disjoint_set.md)
- [dsu.md](ds/dsu.md)
- [fenwick.md](ds/fenwick.md)
- [fuzzy.md](ds/fuzzy.md)
- [hll.md](ds/hll.md)
- [intern.md](ds/intern.md)
- [interval_tree.md](ds/interval_tree.md)
- [kdtree.md](ds/kdtree.md)
- [kmp.md](ds/kmp.md)
- [merkle_tree.md](ds/merkle_tree.md)
- [quadtree.md](ds/quadtree.md)
- [radix.md](ds/radix.md)
- [ringhash.md](ds/ringhash.md)
- [rmq.md](ds/rmq.md)
- [roaring.md](ds/roaring.md)
- [rope.md](ds/rope.md)
- [rope_text.md](ds/rope_text.md)
- [rtree.md](ds/rtree.md)
- [segment_tree.md](ds/segment_tree.md)
- [segtree.md](ds/segtree.md)
- [skip_list.md](ds/skip_list.md)
- [skiplist.md](ds/skiplist.md)
- [suffix.md](ds/suffix.md)
- [suffix_array.md](ds/suffix_array.md)
- [tdigest.md](ds/tdigest.md)
- [treap.md](ds/treap.md)
- [trie_radix.md](ds/trie_radix.md)
- [zset.md](ds/zset.md)
