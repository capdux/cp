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
| `lib/ds/skiplist.cp` | 32 层有序跳表 MemTable 与范围扫描索引（对标 `crossbeam-rs/crossbeam` MIT/Apache-2.0 加固：最大层数夹紧 `initClamped`、`upperBound`/`predecessor`/`successor`/`floor` 与严格单调不变量校验 `verifyInvariants`） |
| `lib/ds/segtree.cp` | 树状数组（`FenwickTree`）与懒标记线段树（`LazySegmentTree`）区间查询/更新 |
| `lib/ds/dsu.cp` | 路径压缩并查集（`DisjointSet`）、带权并查集（`WeightedDsu`）与可撤销并查集（`RollbackDsu`） |
| `lib/ds/intervalTree.cp` | 泛型增广区间树（重叠区间检索与点刺查询） |
| `lib/ds/quadtree.cp` | 动态分裂/合并四叉树（2D 空间索引与范围/近邻查询） |
| `lib/ds/kdtree.cp` | 多维 K-D Tree 空间索引与 K 近邻（KNN）搜索 |
| `lib/ds/rtree.cp` | 2D AABB R*-Tree 空间索引（对标 `georust/rstar`：STR 批量建树、欠载重插删除、半径/包含/相交/K-NN 查询与 4-lane SIMD FMA 批处理核） |
| `lib/ds/suffix.cp` | 后缀数组、Kasai LCP 数组、模式范围搜索、不同子串计数与最长重复子串 |
| `lib/ds/kmp.cp` | KMP 前缀函数、模式搜索、最短周期、Z-Algorithm 与 Manacher 最长回文子串 |
| `lib/ds/ahoCorasick.cp` | Aho-Corasick 多模式自动机（对标 `BurntSushi/aho-corasick`：`Standard`/`LeftmostFirst`/`LeftmostLongest`、ASCII 大小写折叠、零堆扫描/批量替换与 `cap:simd` 预过滤） |
| `lib/ds/rope.cp` | 平衡 Rope 文本缓冲（对标 `cessen/ropey` MIT 加固：`O(log n)` 零分配删除重平衡、`splitAt`/`concat`/`verifyInvariants` 与 16-lane SIMD `\n` 计数/定位引擎） |
| `lib/ds/bimap.cp` | 泛型双向 1-to-1 映射 `BiMap(K, V)` 与 `StringBiMap` |
| `lib/ds/circBuf.cp` | 泛型环形队列/循环缓冲区 `RingBuffer(T)` |
| `lib/ds/bitset.cp` | 64-bit 字加速位集合（`BitSet`，按位与/或/异或/计数/前导搜索） |
| `lib/ds/cuckoo.cp` | 4 槽 16-bit 指纹 Cuckoo Filter（支持动态删除的概率成员过滤） |
| `lib/ds/hll.cp` | HyperLogLog 基数估算器（低内存去重计数） |
| `lib/ds/roaring.cp` | Roaring Bitmap 压缩 32-bit 整型集合引擎（对标 `RoaringBitmap/roaring-rs` MIT/Apache-2.0 加固：Array/Bitset 4096 阈值自适应切换、`select`/`isSubset`/`isDisjoint`/`equals`/`addRangeInclusive`、Portable Roaring Format `12346`/`12347` 严苛编解码与 4-lane `@Vector(4, u64)` SIMD 位图集合核） |
| `lib/ds/tdigest.cp` | T-Digest 流式分位数估算（极端尾延迟 p99/p99.9 高精度） |
| `lib/ds/index.cp` | `cap:ds` 与 `cap.ds` 统一零冲突公开导出入口（20 个子模块） |

---

## 二、 核心 API 索引

### 1. 有序树、区间查询与并查集
- **`treap.cp`**：`TREAP_NIL`, `TreapNode`, `TreapEntry`, `Treap`
- **`rmq.cp`**：`floorLog2`, `sparseTableCells`, `gcdU64`, `RmqMin`, `RmqMax`, `RmqArgMin`, `RangeGcd`, `EulerLca`, `slidingWindowMin`, `slidingWindowMax`
- **`skiplist.cp`**：`SKIPLIST_MAX_LEVEL`, `SKIPLIST_DEFAULT_MAX_LEVEL`, `SkipListError`, `SkipEntry`, `SkipList`（`init`, `initClamped`, `initWithSeed`, `upsert`, `get`, `contains`, `removeKey`, `lowerBound`, `upperBound`, `predecessor`, `successor`, `floor`, `first`, `last`, `rangeScan`, `verifyInvariants`；测试：`cap/test/testDsRopeSkipRustRefSuite.cp`）
- **`segtree.cp`**：`SegTreeError`, `RangeStats`, `FenwickTree`, `LazySegmentTree`
- **`dsu.cp`**：`DsuError`, `DisjointSet`, `WeightedDsu`, `RollbackDsu`
- **`intervalTree.cp`**：`IntervalError`, `Interval`, `IntervalTree`

### 2. 多维空间索引
- **`quadtree.cp`**：`QuadError`, `QuadEntry`, `QuadHit`, `QuadTree`
- **`kdtree.cp`**：`KdError`, `KdPointEntry`, `KdNeighbor`, `KdTree`
- **`rtree.cp`**：`RTreeError`, `Rect`, `RTreeEntry`, `NearestRect`, `minDistSqBatchScalar`, `minDistSqBatchSimd`, `overlapBatchScalar`, `overlapBatchSimd`, `RTree`（`init`, `initWithCapacity`, `bulkLoad`, `bulkLoadWithCapacity`, `insert`, `remove`, `removeById`, `locateInEnvelopeIntersecting`, `locateInEnvelopeContained`, `locateWithinDistance`, `nearestK`, `nearestNeighbor`, `searchOverlap`, `searchPoint`, `kNearestPoint`, `verifyInvariants`）

### 3. 字符串匹配与文本缓冲
- **`suffix.cp`**：`SuffixWorkspace`, `OccurrenceRange`, `LongestRepeat`, `buildSuffixArray`, `buildLcpArray`, `findPatternRange`, `countDistinctSubstrings`, `longestRepeatedSubstring`
- **`kmp.cp`**：`PalindromeSpan`, `computePrefixFunction`, `kmpFindFirst`, `kmpFindAll`, `kmpCount`, `shortestPeriod`, `computeZArray`, `manacherLongestPalindrome`, `countPalindromicSubstrings`
- **`ahoCorasick.cp`**：`MatchKind`（`Standard` / `LeftmostFirst` / `LeftmostLongest`）, `AcError`, `AhoCorasickOptions`, `Match`, `AhoCorasick`（`init`, `initWithOptions`, `deinit`, `prefilterActive`, `prefilterByteCount`, `findFirst`, `findNextAt`, `findAllNonOverlapping`, `findAllOverlappingInto`, `findAllOverlapping`, `replaceAllInto`, `containsAny`）
- **`rope.cp`**：`Rope`（`init`, `fromSlice`, `insert`, `delete`, `concat`, `splitAt`, `charAt`, `sliceRange`, `lineAt`, `lineCount`, `len`, `toString`, `verifyInvariants`）, `RopeError`, `Node`, `NodePtr`, `simdRopeCountNewlines`, `scalarRopeCountNewlines`, `simdRopeFindNewline`, `scalarRopeFindNewline`（测试：`cap/test/testDsRopeSkipRustRefSuite.cp`）

### 4. 映射、缓冲、位图与概率草图
- **`bimap.cp`**：`BiMap`, `BiMapError`, `StringBiMap`
- **`circBuf.cp`**：`RingError`, `RingBuffer`
- **`bitset.cp`**：`BitSet`, `BitSetError`
- **`cuckoo.cp`**：`CuckooError`, `CuckooStats`, `CuckooFilter`
- **`hll.cp`**（对标 `pwoolcoc/hyperloglog` MIT 加固 + 16-lane `@Vector(16, u8)` SIMD 寄存器合并/零计数）：`HLL_MIN_PRECISION`/`HLL_MAX_PRECISION`、`HllError`、`HyperLogLog`、`validateHllSerialized`、`simdHllMergeRegisters`/`scalarHllMergeRegisters`、`simdHllCountZeros`/`scalarHllCountZeros`（回归测试：`cap/test/testDsSketchesRustRefSuite.cp`）
- **`roaring.cp`**：`ROARING_SERIAL_COOKIE_NO_RUNCONTAINER` (`12346`), `ROARING_SERIAL_COOKIE_RUNCONTAINER` (`12347`), `ROARING_NO_OFFSET_THRESHOLD` (`4`), `RoaringError`, `RoaringBitmap`（`init`, `deinit`, `clone`, `add`, `addRange`, `addRangeInclusive`, `removeVal`, `contains`, `cardinality`, `isEmpty`, `minimum`, `maximum`, `rank`, `select`, `isSubset`, `isDisjoint`, `equals`, `containerCount`, `isContainerBitset`, `bitAnd`, `bitOr`, `bitAndNot`, `bitXor`, `andCardinality`, `intersects`, `toSlice`, `serialize`, `deserialize`, `serializePortable`, `deserializePortable`）, `scalarBitmapPopcount`, `simdBitmapPopcount`, `scalarBitmapAnd`, `simdBitmapAnd`, `scalarBitmapOr`, `simdBitmapOr`, `scalarBitmapXor`, `simdBitmapXor`, `scalarBitmapAndNot`, `simdBitmapAndNot`；回归测试：`cap/test/testDsRoaringRustRefSuite.cp`（边界与 4095/4096/4097 阈值震荡、集合代数律与参考模型差分、Portable Format 12346/12347 编解码与畸形字节流拦截、4-lane SIMD 0 差异对拍、1,600 组确定性 PRNG Fuzz）
- **`tdigest.cp`**（对标 Dunning t-digest Apache-2.0 加固 + 4-lane `@Vector(4, f64)` SIMD 权重求和/极值扫描）：`TDIGEST_MIN_COMPRESSION`/`TDIGEST_MAX_COMPRESSION`、`TDigestError`、`Centroid`、`TdMinMax`、`TDigest`（严格拒绝 `NaN`/`±Inf`/非正权重，支持 `addBatch`/`merge`/`quantile`）、`simdTdSumWeightsF64`/`scalarTdSumWeightsF64`、`simdTdMinMaxF64`/`scalarTdMinMaxF64`（回归测试：`cap/test/testDsSketchesRustRefSuite.cp`）

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

### 零分配 4-bit 饱和计数布隆过滤器 (`countingBloom.cp`)

面向缓存去重、流式集合维护与网络路由黑名单动态删改的 100% 纯 CP 零动态堆分配 4-bit 饱和计数布隆过滤器（支持安全删除、防假阴性饱和保护与饱和并集）：

- **常量与数据模型**：
  - `CBLOOM_MAX_BUCKETS = 4096`、`CBLOOM_MIN_BUCKETS = 16`、`CBLOOM_STORAGE_BYTES = 2048`（紧凑半字节 nibble 存储）、`CBLOOM_MAX_HASHES = 8`、`CBLOOM_MAX_COUNTER = 15`
  - `CountingBloomStats`（`num_buckets`, `num_hashes`, `non_zero_buckets`, `saturated_buckets`, `total_inserts`, `total_removes`, `rejected_removes`, `saturation_hits`, `estimated_items`, `estimated_fpr`）
- **核心引擎 `CountingBloomFilter`**：
  - `init(bucket_count, hash_count): CountingBloomFilter?`（校验 `bucket_count` 为 `16..=4096` 之间的 2 的幂、`hash_count` 在 `1..=8`）/ `initDefault()`
  - `insert(*_, key)` / `insertU64(*_, key)` / `contains(key)` / `containsU64(key)` / `minCount(key): u8`
  - `remove(*_, key)` / `removeU64(*_, key)`：删除前先校验 `contains(key)` 避免误扣减破坏其他键；已达 4-bit 饱和值 `15` 的计数器保持饱和不减，彻底杜绝假阴性。
  - `mergeAdd(*_, other): bool`：同配置过滤器执行 4-bit 饱和并集 `min(15, a + b)`。
  - `clear(*_)` / `nonZeroBuckets()` / `saturatedBuckets()` / `estimatedItems()` / `estimatedFpr()` / `stats()`。

```cp
import { CountingBloomFilter } from "cap:ds";

pub fn demoCountingBloom() {
    let bf = CountingBloomFilter.init(1024, 4) ?? return;
    * = bf.insert("session:alpha");
    * = bf.insert("session:beta");
    * = bf.contains("session:alpha"); // true
    * = bf.remove("session:alpha");   // true (精确删除)
    * = bf.contains("session:alpha"); // false
    * = bf.contains("session:beta");  // true (不受影响)
}
```

---

### 零分配 2 的幂位掩码批量 SPSC 环形通道与高低水位背压队列 (`spscRing.cp`)

面向高性能生产者-消费者管道、流式遥测滑窗与滞回背压限流的 100% 纯 CP 零动态堆分配 SPSC 环形队列：

- **常量与数据模型**：
  - `SPSC_MIN_CAPACITY = 4`、`SPSC_MAX_CAPACITY = 256`、`SPSC_DEFAULT_HIGH_WATERMARK_PERMILLE = 800`、`SPSC_DEFAULT_LOW_WATERMARK_PERMILLE = 250`
  - `spscIsValidCapacity(cap): bool`（校验 `4..=256` 之间的 2 的幂）
  - `SpscWatermarkConfig`（`high_permille`, `low_permille`）、`SpscBackpressureState`（`normal | throttled`）、`SpscRingStats`
- **核心引擎 `SpscRingU64`**：
  - `init(capacity): SpscRingU64?` / `initWithWatermarks(capacity, wm): SpscRingU64?`：采用单调递增 `u64` 游标与 `& mask` 零取模索引，满/空状态精确区分且不浪费 1 槽。
  - 单条与随机访问：`tryPush(*_, item)`、`tryPop(*_)`、`peek()`、`peekAt(offset)`、`pushOverwrite(*_, item): u64?`（满时覆写最旧元素并返回被淘汰值）。
  - 批量零拷贝：`pushBatch(*_, items)`、`popBatch(*_, out_items)`、`skip(*_, count)`、`clear(*_)`。
  - 滞回高低水位背压：`fillPermille()`、`isAboveHighWatermark()`、`isBelowLowWatermark()`、`isBackpressured()`、`stats()`。

```cp
import { SpscRingU64 } from "cap:ds";

pub fn demoSpscRing() {
    let ring = SpscRingU64.init(8) ?? return;
    const batch: [4]u64 = [10, 20, 30, 40];
    * = ring.pushBatch(batch[0..]);

    let out: [4]u64 = [0; 4];
    let out_s: []u64 = out[0..];
    const popped = ring.popBatch(out_s);
    * = popped; // 4 ([10, 20, 30, 40])
}
```

### 零堆分配 Observed-Remove Set (OR-Set) 与 LWW-Register64 CRDT 引擎 (`orSet.cp`)

与 `crdtPn.cp`（GCounter / PNCounter / GSet / LWWRegister）互补的 100% 纯 CP 零动态堆分配无墓碑 **Add-Wins Observed-Remove Set (AWORSet)** 与 **LWW-Register64** 分布式无冲突复制数据结构：

- **常量与模型**：
  - `ORSET_MAX_REPLICAS = 16`、`ORSET_MAX_ENTRIES = 64`、`ORSET_KEY_MAX_LEN = 48`
  - `OrSetDot`（`replica_id: u16`, `seq: u64`；提供 `equals`）
  - `OrSetOpStatus`（`ok | capacity_full | key_too_long | unknown_replica | not_found`）
  - `OrSetEntry`（`key()`, `dot`）、`OrSetStats`（`active_dots`, `distinct_elements`, `replicas_tracked`, `merge_count`）
- **因果上下文 `OrSetCausalContext`**：
  - 紧凑 16 副本向量时钟 `clocks: [ORSET_MAX_REPLICAS]u64`，提供 `init()`、`nextDot(*_, replica_id)`、`hasSeen(dot)`、`merge(*_, other)`、`dominates(other)`、`equals(other)`、`trackedReplicas()`。
- **无墓碑 Add-Wins OR-Set `OrSetCrdt`**：
  - `init(replica_id)` / `add(*_, elem): OrSetOpStatus` / `remove(*_, elem): bool`（仅删除本地已观测 dot，绝不误删并发新增的未观测 dot）
  - `contains(elem)` / `distinctCount()` / `dotCount()` / `equalsElements(other)` / `stats()`
  - `merge(*_, remote): OrSetOpStatus`：严格满足 CRDT 结合律、交换律、幂等律，满容时零副作用返回 `.capacity_full`。
- **64 位最后写入胜出寄存器 `LwwRegister64`**：
  - `init(replica_id)` / `initWithValue(replica_id, val, ts_ms)` / `set(*_, val, ts_ms)` / `get(): u64?` / `merge(*_, remote): bool`（基于 `(timestamp_ms, replica_id, value)` 确定性全序决胜与幂等合并）。

```cp
import { OrSetCrdt, LwwRegister64 } from "cap:ds";

pub fn demoOrSetCrdt() {
    let a = OrSetCrdt.init(0);
    let b = OrSetCrdt.init(1);

    * = a.add("task-1");
    * = b.merge(a);

    // 并发冲突：A 删除 "task-1"，B 并发重新添加 "task-1" -> Add-Wins 保留 B 的新 dot
    * = a.remove("task-1");
    * = b.add("task-1");
    * = a.merge(b);
    * = b.merge(a);
    // a.contains("task-1") == true && b.contains("task-1") == true

    let r1 = LwwRegister64.initWithValue(1, 100, 5000);
    let r2 = LwwRegister64.initWithValue(2, 200, 5000);
    * = r1.merge(r2); // 同时间戳下 replica_id=2 胜出，r1.get() == 200
}
```

### 零分配 O(1) SparseSet 整数集合与 ECS 密集键值映射引擎 (`sparseSet.cp`)

面向 ECS 实体组件系统、图算法访问集合与高频重置场景的 100% 纯 CP 零动态堆分配 Briggs-Torczon O(1) 稀疏集合与并行密集键值映射：

- **常量与统计**：
  - `SPARSE_SET_UNIVERSE = 1024`、`SPARSE_SET_INVALID_IDX = 0xFFFF`
  - `SparseSetStats`（`len`, `universe`, `insert_ops`, `remove_ops`, `clear_ops`）
- **`SparseSet`**（`[0, 1024)` 整数集合）：
  - `init()` / `len()` / `isEmpty()` / `contains(id)`（双向不变量校验，完全免疫 `clear()` 后旧脏 `sparse` 槽位）
  - `insert(*_, id): bool`（O(1) 幂等插入）/ `remove(*_, id): bool`（O(1) 尾元素 swap-remove 无空洞）/ `clear(*_)`（O(1) 仅归零 `dense_len`）/ `denseSlice(): []const u16` / `stats()`
  - 集合代数：`intersectInto(other, *out)`、`unionInto(other, *out)`、`differenceInto(other, *out)`、`isSubsetOf(other)`
- **`SparseMapU64`**（ECS 并行密集键值映射）：
  - `init()` / `put(*_, id, val)` / `get(id): u64?` / `remove(*_, id): u64?`（同步 swap-remove 保持 `denseIds()` 与 `denseValues()` 严格 1-to-1 对齐）/ `contains(id)` / `len()` / `clear(*_)` / `denseIds()` / `denseValues()`

```cp
import { SparseSet, SparseMapU64 } from "cap:ds";

pub fn demoSparseSet() {
    let active = SparseSet.init();
    * = active.insert(10);
    * = active.insert(42);
    * = active.remove(10); // O(1) swap-remove

    let hp = SparseMapU64.init();
    * = hp.put(42, 100);
    * = hp.get(42); // 100
    active.clear(); // O(1) 仅重置 dense_len = 0
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

## 历史数据结构域散装模块收敛（W29-Sync R4）

以下 8 个原独立顶层目录已统一收敛至 `cap:ds`（`lib/ds/`），可直接从 `"cap:ds"` 导入：

| 原顶层目录 | 收敛目标文件 | 主要导出符号 |
|---|---|---|
| `cap:bounded_array` | `lib/ds/boundedArray.cp` | `BoundedArray` |
| `cap:bplus_tree` | `lib/ds/bplus{Types,SlottedPage,Tree}.cp` | `SlottedBPlusTree`, `BPlusError`, `KvEntry`, `RangeIter`, `SlottedPage`, `encodeSlottedPage`, `decodeSlottedPage`, `PAGE_SIZE`, `MAX_PAYLOAD_SIZE` |
| `cap:hive_array` | `lib/ds/hiveArray.cp` | `HiveArray` |
| `cap:intrusive_heap` | `lib/ds/intrusiveHeap.cp` | `IntrusiveField`, `IntrusiveHeap` |
| `cap:linear_fifo` | `lib/ds/linearFifo.cp` | `StaticLinearFifo` |
| `cap:lockfree_queue` | `lib/ds/lockfree{Queue,Spsc,Mpmc,Channel}.cp` | `SpscQueue`, `MpmcQueue`, `BoundedChannel`, `ChannelError`, `UnboundedQueue`, `CACHE_LINE_LENGTH`, `QUEUE_PADDING_LENGTH` |
| `cap:static_hashmap` | `lib/ds/staticHashMap.cp` | `StaticHashMap`, `KeyValue`, `Slot`, `StringContext`, `AsciiIgnoreCaseContext`, `IntContext`, `fnv1a64`, `fnv1a64AsciiLower`, `splitmix64` |
| `cap:object_pool` | `lib/ds/objectPool.cp` | `ObjectPool`, `PoolNode` |
| `cap:exact_matcher` | `lib/ds/exactMatcher.cp` | `ExactSizeMatcher`, `Matcher4`, `Matcher8`, `Matcher16`, `packU32`, `packU32Lower`, `packU64`, `packU64Lower`, `packU128`, `packU128Lower` |

---

## B+ 树与增强区间树深度加固（对标 `rust-lang/rust` BTree & `rust-bio/rust-bio` IntervalTree MIT OR Apache-2.0：`bptree.cp` / `intervalTree.cp`）

- **`bptree.cp`（有序 B+ 树索引加固）**：
  - **4-lane `@Vector(4, i64)` SIMD 槽位定位**：`simdBptCountLessI64` / `scalarBptCountLessI64`（0 差异对拍），`i64` 键查找/插入/删除/范围扫描热路径直连 4-lane SIMD。
  - **结构不变量与安全边界**：新增 `contains`、`isEmpty` 与 `verifyInvariants()`（校验全叶等深、节点键数 `[MIN_KEYS, MAX_KEYS]`、键严格单调递增、分隔键界定与叶链表完整递增遍历）。
- **`intervalTree.cp`（增强区间树加固）**：
  - **4-lane `@Vector(4, i64)` SIMD 重叠批量计数**：`simdIntervalCountOverlaps` / `scalarIntervalCountOverlaps`（0 差异对拍），新增 `countOverlaps` 热路径直连 4-lane SIMD 批处理。
  - **严苛边界与增强属性校验**：全入口严格拦截 `low > high` 返回 `IntervalError.InvalidInterval`，新增 `isEmpty` 与 `verifyInvariants()`（校验 BST 序、`low <= high`、精确增强属性 `max_high` 与节点计数）。
