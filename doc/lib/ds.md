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
