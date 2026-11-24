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
    SKIPLIST_MAX_LEVEL,
    SKIPLIST_DEFAULT_MAX_LEVEL,
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
    scalarIntervalCountOverlaps,
    simdIntervalCountOverlaps,
} from "./intervalTree.cp";

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
    minDistSqBatchScalar,
    minDistSqBatchSimd,
    overlapBatchScalar,
    overlapBatchSimd,
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
    MatchKind,
    AcError,
    AhoCorasickOptions,
    Match,
    AhoCorasick,
} from "./ahoCorasick.cp";

export {
    Rope,
    RopeError,
    Node,
    NodePtr,
    simdRopeCountNewlines,
    scalarRopeCountNewlines,
    simdRopeFindNewline,
    scalarRopeFindNewline,
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
} from "./circBuf.cp";

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
    HLL_MIN_PRECISION,
    HLL_MAX_PRECISION,
    HLL_SERIAL_HEADER_LEN,
    HllError,
    simdHllMergeRegisters,
    scalarHllMergeRegisters,
    simdHllCountZeros,
    scalarHllCountZeros,
    validateHllSerialized,
    HyperLogLog,
} from "./hll.cp";

export {
    ROARING_SERIAL_COOKIE_NO_RUNCONTAINER,
    ROARING_SERIAL_COOKIE_RUNCONTAINER,
    ROARING_NO_OFFSET_THRESHOLD,
    RoaringError,
    RoaringBitmap,
    scalarBitmapPopcount,
    simdBitmapPopcount,
    scalarBitmapAnd,
    simdBitmapAnd,
    scalarBitmapOr,
    simdBitmapOr,
    scalarBitmapXor,
    simdBitmapXor,
    scalarBitmapAndNot,
    simdBitmapAndNot,
} from "./roaring.cp";

export {
    TDIGEST_MIN_COMPRESSION,
    TDIGEST_MAX_COMPRESSION,
    TDigestError,
    Centroid,
    TdMinMax,
    simdTdSumWeightsF64,
    scalarTdSumWeightsF64,
    simdTdMinMaxF64,
    scalarTdMinMaxF64,
    TDigest,
} from "./tdigest.cp";

// === Consolidated Wave-2 Data Structures, CRDTs, Consistent Hashing & Graph/Text Indexes ===

// --- bptree ---
export {
    BPlusTree,
    scalarBptCountLessI64,
    simdBptCountLessI64,
} from "./bptree.cp";

// --- crdt_pn ---
pub import { GCounter, PNCounter, LWWRegister, GSet, CounterEntry } from "./crdtPn.cp";

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
} from "./radixRouter.cp";

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
} from "./merkle.cp";

// --- heavy_hitter ---
export {
    HeavyHitterEntry,
    HeavyHitterTracker,
    CmsSketchConfig,
    ConservativeCms,
} from "./heavyHitter.cp";

// --- fenwick_order ---
export {
    FENWICK_MAX_LEN,
    FenwickStats,
    FenwickTreeI64,
    FenwickOrderStat,
} from "./fenwick.cp";

// --- augmented_interval_tree ---
export {
    AUG_ITREE_MAX_NODES,
    AUG_ITREE_NO_NODE,
    IntervalSpan,
    IntervalQueryResult,
    IntervalTreeStats,
    coalesceIntervals,
    findGaps,
    AugmentedIntervalTree,
} from "./augInterval.cp";

// --- w_tinylfu ---
export {
    TINYLFU_SKETCH_ROWS,
    TINYLFU_SKETCH_WORDS_PER_ROW,
    TINYLFU_SKETCH_WIDTH,
    TINYLFU_MAX_COUNT,
    TINYLFU_MAX_CAPACITY,
    TINYLFU_NO_NODE,
    TinyLfuSketch,
    TinyLfuSegment,
    TinyLfuPutOutcome,
    TinyLfuConfig,
    TinyLfuNode,
    TinyLfuStats,
    WTinyLfuCache,
} from "./tinyLfu.cp";

// --- prefixTrie (Zero-Alloc Path-Compressed Byte Prefix Trie / Patricia-Radix KV) ---
export {
    PREFIX_TRIE_MAX_NODES,
    PREFIX_TRIE_BYTE_POOL_SIZE,
    PREFIX_TRIE_MAX_KEY_LEN,
    PREFIX_TRIE_NO_NODE,
    PrefixMatch,
    PrefixScanHit,
    PrefixScanReport,
    PrefixTrieStats,
    PrefixTrie,
} from "./prefixTrie.cp";

// --- countingBloom (Zero-Alloc 4-Bit Saturating Counting Bloom Filter with Deletion) ---
export {
    CBLOOM_MAX_BUCKETS,
    CBLOOM_MIN_BUCKETS,
    CBLOOM_STORAGE_BYTES,
    CBLOOM_MAX_HASHES,
    CBLOOM_MAX_COUNTER,
    CountingBloomStats,
    CountingBloomFilter,
} from "./countingBloom.cp";

// --- spscRing (Zero-Alloc Power-of-Two Bitmask Batch SPSC Ring & Hysteresis Backpressure Queue) ---
export {
    SPSC_MIN_CAPACITY,
    SPSC_MAX_CAPACITY,
    SPSC_DEFAULT_HIGH_WATERMARK_PERMILLE,
    SPSC_DEFAULT_LOW_WATERMARK_PERMILLE,
    spscIsValidCapacity,
    SpscWatermarkConfig,
    SpscBackpressureState,
    SpscRingStats,
    SpscRingU64,
} from "./spscRing.cp";

// --- orSet (Zero-Alloc Observed-Remove Set CRDT & LWW-Register64) ---
export {
    ORSET_MAX_REPLICAS,
    ORSET_MAX_ENTRIES,
    ORSET_KEY_MAX_LEN,
    OrSetDot,
    OrSetOpStatus,
    OrSetEntry,
    OrSetStats,
    OrSetCausalContext,
    OrSetCrdt,
    LwwRegister64,
} from "./orSet.cp";

// --- sparseSet (Zero-Alloc O(1) SparseSet & ECS Dense SparseMapU64) ---
export {
    SPARSE_SET_UNIVERSE,
    SPARSE_SET_INVALID_IDX,
    SparseSetStats,
    SparseSet,
    SparseMapU64,
} from "./sparseSet.cp";

// === W29-Sync R4: Consolidated Core Data Structures ===
export {
    BoundedArray,
} from "./boundedArray.cp";

export {
    BPlusError,
    KvEntry,
    compareKeys,
} from "./bplusTypes.cp";

import bplus_tree_mod from "./bplusTree.cp";
pub const SlottedBPlusTree = bplus_tree_mod.BPlusTree;

export {
    RangeIter,
    NULL_NODE,
    MIN_ORDER,
} from "./bplusTree.cp";

export {
    SlottedPage,
    encodeSlottedPage,
    decodeSlottedPage,
    PAGE_SIZE,
    PAGE_MAGIC,
    PAGE_HEADER_SIZE,
    PAGE_FOOTER_SIZE,
    SLOT_SIZE,
    MAX_PAYLOAD_SIZE,
} from "./bplusSlottedPage.cp";

export {
    HiveArray,
} from "./hiveArray.cp";

export {
    IntrusiveField,
    IntrusiveHeap,
} from "./intrusiveHeap.cp";

export {
    StaticLinearFifo,
} from "./linearFifo.cp";

export {
    SpscQueue,
} from "./lockfreeSpsc.cp";

export {
    MpmcQueue,
} from "./lockfreeMpmc.cp";

export {
    BoundedChannel,
    ChannelError,
} from "./lockfreeChannel.cp";

export {
    CACHE_LINE_LENGTH,
    QUEUE_PADDING_LENGTH,
    UnboundedQueue,
} from "./lockfreeQueue.cp";

export {
    StaticHashMap,
    KeyValue,
    Slot,
    StringContext,
    AsciiIgnoreCaseContext,
    IntContext,
    EMPTY_HASH,
    fnv1a64,
    fnv1a64AsciiLower,
    splitmix64,
} from "./staticHashMap.cp";

export {
    ObjectPool,
    PoolNode,
} from "./objectPool.cp";

// === W29-Sync R7: Consolidated Exact Size String Matcher ===
export {
    ExactSizeMatcher,
    Matcher4,
    Matcher8,
    Matcher16,
    packU32,
    packU32Lower,
    packU64,
    packU64Lower,
    packU128,
    packU128Lower,
} from "./exactMatcher.cp";
