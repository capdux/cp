// Source: cp/lib/ds/index.cp (Unified Exports for cap:ds)
// Unified Public Exports for General-Purpose Data Structures, Spatial Indexes, String Matching & Probabilistic Sketches (cap:ds & cap.ds)

// --- Ordered Trees, Range Queries & Disjoint Sets ---

export {
    TREAP_NIL,
    TreapNode,
    TreapEntry,
    Treap,
} from "./treap.cp";

export {
    floorLog2,
    sparseTableCells,
    gcdU64,
    RmqMin,
    RmqMax,
    RmqArgMin,
    RangeGcd,
    EulerLca,
    slidingWindowMin,
    slidingWindowMax,
} from "./rmq.cp";

export {
    SkipListError,
    SkipEntry,
    SkipList,
} from "./skiplist.cp";

export {
    SegTreeError,
    RangeStats,
    FenwickTree,
    LazySegmentTree,
} from "./segtree.cp";

export {
    DsuError,
    DisjointSet,
    WeightedDsu,
    RollbackDsu,
} from "./dsu.cp";

export {
    IntervalError,
    Interval,
    IntervalTree,
} from "./interval_tree.cp";

// --- Spatial Indexes ---

export {
    QuadError,
    QuadEntry,
    QuadHit,
    QuadTree,
} from "./quadtree.cp";

export {
    KdError,
    KdPointEntry,
    KdNeighbor,
    KdTree,
} from "./kdtree.cp";

export {
    RTreeError,
    Rect,
    RTreeEntry,
    NearestRect,
    RTree,
} from "./rtree.cp";

// --- String Matching & Text Buffers ---

export {
    SuffixWorkspace,
    OccurrenceRange,
    LongestRepeat,
    buildSuffixArray,
    buildLcpArray,
    findPatternRange,
    countDistinctSubstrings,
    longestRepeatedSubstring,
} from "./suffix.cp";

export {
    PalindromeSpan,
    computePrefixFunction,
    kmpFindFirst,
    kmpFindAll,
    kmpCount,
    shortestPeriod,
    computeZArray,
    manacherLongestPalindrome,
    countPalindromicSubstrings,
} from "./kmp.cp";

export {
    Match,
    AhoCorasick,
} from "./aho_corasick.cp";

export {
    Rope,
    RopeError,
    Node,
    NodePtr,
} from "./rope.cp";

// --- Maps, Buffers, Bitsets & Probabilistic Sketches ---

export {
    BiMap,
    BiMapError,
    StringBiMap,
} from "./bimap.cp";

export {
    RingError,
    RingBuffer,
} from "./circ_buf.cp";

export {
    BitSet,
    BitSetError,
} from "./bitset.cp";

export {
    CuckooError,
    CuckooStats,
    CuckooFilter,
} from "./cuckoo.cp";

export {
    HllError,
    HyperLogLog,
} from "./hll.cp";

export {
    RoaringError,
    RoaringBitmap,
} from "./roaring.cp";

export {
    TDigestError,
    Centroid,
    TDigest,
} from "./tdigest.cp";

// === Consolidated Wave-2 Data Structures, CRDTs, Consistent Hashing & Graph/Text Indexes ===

// --- bptree ---
export { BPlusTree } from "./bptree.cp";

// --- crdt_pn ---
pub import { GCounter, PNCounter, LWWRegister, GSet, CounterEntry } from "./crdt_pn.cp";

// --- ringhash ---
export {
    RingHashError,
    ConsistentRing,
    RendezvousHasher,
    jumpConsistentHash,
} from "./ringhash.cp";

// --- zset ---
export {
    ZSetError,
    ZSetEntry,
    ZSetAgg,
    SortedSet,
} from "./zset.cp";

// --- radix ---
export {
    RadixError,
    RadixMatch,
    RadixEntry,
    RadixTree,
} from "./radix.cp";

export {
    RouteParam,
    RouteMatch,
    RadixRouter,
} from "./radix_router.cp";

// --- intern ---
pub const _intern = @import("./intern.cp");
pub const InternStringBiMap = _intern.StringBiMap;
export {
    InternError,
    SymbolId,
    StringInterner,

} from "./intern.cp";

// --- fuzzy ---
export {
    FuzzyError,
    FuzzyMatch,
    levenshtein,
    damerauLevenshtein,
    jaro,
    jaroWinkler,
    fuzzyScore,
    rankCandidates,
    freeMatches,
} from "./fuzzy.cp";

// --- merkle_tree ---
export {
    ProofStep,
    MerkleProof,
    MerkleTree,
    hashLeaf,
    hashInternal,
    verifyProof,
} from "./merkle_tree.cp";

// --- heavy_hitter ---
export {
    HeavyHitterEntry,
    HeavyHitterTracker,
    CmsSketchConfig,
    ConservativeCms,
} from "./heavy_hitter.cp";

// --- fenwick_order ---
export {
    FENWICK_MAX_LEN,
    FenwickStats,
    FenwickTreeI64,
    FenwickOrderStat,
} from "./fenwick_order.cp";
