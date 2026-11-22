// Source: cp/lib/db/index.cp (Unified Database, Storage Engine, Cache, Concurrency & Index Barrel)
// Consolidated cap:db standard library barrel covering B+Tree, LSM-Tree, WAL, SSTable, KV Store,
// MVCC, Lock Manager, Raft Log, LRU/LRU-K/LFU Caches, Slab Pool, HNSW Vector Index, BM25F Full-Text
// Search, Roaring Bitmap, Bloom/Cuckoo/Xor Filters & Streaming Sketches.
// Note: `hyperloglog_.cp` is archived for reference (overlap with `sketch.cp` HyperLogLog) and not re-exported.


// --- bplus ---
export {
    BPlusError,
    BPlusEntry,
    BPlusTree,
} from "./bplus.cp";

// --- lsm ---
export {
    MAX_KEY_LEN,
    LsmError,
    LsmEntry,
    LsmStats,
    SstRecord,
    SstLookup,
    SsTable,
    LsmEngine,
} from "./lsm.cp";

// --- wal ---
export {
    WAL_MAGIC,
    WAL_HEADER_SIZE,
    WAL_DEFAULT_MAX_RECORD_BYTES,
    WalError,
    WalRecordType,
    WalRecord,
    WalRecoveryResult,
    WalLog,
    crc32c,
} from "./wal.cp";

// --- sstable ---
export {
    SST_FOOTER_SIZE,
    SST_LEVELDB_MAGIC,
    SST_ROCKSDB_MAGIC,
    SST_BLOCK_TRAILER_SIZE,
    SstCompression,
    VarintResult,
    BlockHandle,
    BlockHandleResult,
    SstFooter,
    SstBlockView,
    SstBlockHeader,
    SstRawEntry,
    SstBlockIter,
    maskCrc,
    unmaskCrc,
    decodeVarint64,
    encodeVarint64,
    decodeBlockHandle,
    encodeBlockHandle,
    parseSstFooter,
    isSst,
    extractSstBlock,
    parseSstBlockInfo,
} from "./sstable.cp";

// --- kv ---
pub const _kv_wal = @import("./kv_wal.cp");
pub const KV_WAL_MAGIC = _kv_wal.WAL_MAGIC;
pub const KV_WAL_HEADER_SIZE = _kv_wal.WAL_HEADER_SIZE;
pub const KvWalRecord = _kv_wal.WalRecord;

export {
    KvError,
    WalOp,
    WalFrame,
    WalDecodeResult,
    WalCodec,
} from "./kv_wal.cp";

export {
    KvStoreOptions,
    KvStats,
    KvStore,
} from "./kv_store.cp";

// --- mvcc ---
export { MvccError, TxnState, VersionValue, WriteEntry, ActiveTxnInfo } from "./mvcc_types.cp";
export { MvccStore, MvccTxn } from "./mvcc_store.cp";

// --- lock ---
export {
    WaitForGraph,
} from "./lock_wait_graph.cp";

export {
    LockError,
    LockMode,
    NamedLockTable,
    LeaseToken,
    LeaseLockManager,
} from "./lock.cp";

// --- raft_log ---
export {
    RaftLogError,
    LogEntry,
    LogEntryInput,
    RaftLog,
} from "./raft_log.cp";

// --- lru ---
export {
    LruError,
    LruStats,
    LruCache,
} from "./lru.cp";

// --- lru_k ---
import { LruKCache as _LruKCache, LruKError as _LruKError } from "./lru_k.cp";

pub const LruKCache = _LruKCache;
pub const LruKError = _LruKError;

// --- lfu ---
export {
    TINYLFU_ROWS,
    TINYLFU_MAX_COUNT,
    TinyLfuSketch,
    LfuSlot,
    LfuCache,
} from "./lfu.cp";

// --- slab_pool ---
export { Handle, SlabError, SlabPool } from "./slab_pool.cp";

// --- hnsw ---
export { HnswError, DistanceMetric, HnswConfig, SearchHit as HnswSearchHit, computeDistance } from "./hnsw_metric.cp";
export { HnswIndex } from "./hnsw_graph.cp";

// --- search ---
export {
    SearchError,
    PrefixSuggestion,
    PrefixTrie,
} from "./search_trie.cp";

export {
    SearchHit,
    SearchIndex,
} from "./search_engine.cp";

// --- roaring_bitmap ---
pub const roaring = @import("./roaring_bitmap.cp");

pub const ARRAY_MAX_SIZE = roaring.ARRAY_MAX_SIZE;
pub const BITSET_WORDS = roaring.BITSET_WORDS;
pub const RoaringError = roaring.RoaringError;
pub const ArrayContainer = roaring.ArrayContainer;
pub const BitsetContainer = roaring.BitsetContainer;
pub const Container = roaring.Container;
pub const RoaringBucket = roaring.Bucket;
pub const RoaringBitmap = roaring.RoaringBitmap;

// --- bloom_cuckoo ---
pub const bc = @import("./bloom_cuckoo.cp");
pub const MAX_KICKS = bc.MAX_KICKS;
pub const FilterError = bc.FilterError;
pub const BloomFilter = bc.BloomFilter;
pub const CountingBloomFilter = bc.CountingBloomFilter;
pub const CuckooFilter = bc.CuckooFilter;
pub const CuckooBucket = bc.Bucket;

// --- xorfilter ---
export {
    XorFilterError,
    Xor8Filter,
    Xor16Filter,
} from "./xorfilter.cp";

// --- sketch ---
export {
    SketchError,
    HyperLogLog,
    Centroid,
    QuantileDigest,
    ReservoirSampler,
} from "./sketch.cp";

// --- redis_cluster ---
export {
    REDIS_CLUSTER_SLOTS,
    crc16Ccitt,
    extractHashTag,
    keySlot,
    ensureSameSlot,
} from "./redis_cluster_crc16.cp";

export {
    ClusterError,
    NodeEndpoint,
    SlotRange,
    RedirectKind,
    ClusterRedirect,
    ClusterTopology,
    parseRedirectError,
} from "./redis_cluster_topology.cp";

// --- valkey ---
export {
    DEFAULT_MAX_DEPTH,
    DEFAULT_MAX_BULK_LEN,
    DEFAULT_MAX_COLLECTION_LEN,
    ProtocolError,
    RespLimits,
    RespType,
    VerbatimString,
    MapEntry,
    RespValue,
    ParseResult,
    RespStreamParser,
    parseOne,
    parseOneWithLimits,
    parseInlineCommand,
    encodeValue,
    encodeCommand,
    encodeInlineCommand,
} from "./valkey.cp";

// --- query ---
export {
    QueryError,
    QueryDialect,
    QueryParam,
    BuiltQuery,
    IdentCodec,
} from "./query_types.cp";

export {
    SelectBuilder,
    InsertBuilder,
    UpdateBuilder,
    DeleteBuilder,
} from "./query_builder.cp";

// --- schema ---
export {
    SchemaParseError,
    SchemaType,
    StringFormat,
    PropertySchema,
    Schema,
} from "./schema.cp";

export {
    ValidationIssue,
    ValidationResult,
    SchemaValidator,
} from "./schema_validator.cp";

// --- conn_pool ---
export {
    DbPoolConfig,
    DbConnState,
    DbConnSlot,
    DbAcquireKind,
    DbAcquireOutcome,
    DbPoolStats,
    DbConnPool,
} from "./conn_pool.cp";

// --- wal_recovery ---
export {
    WAL_FRAME_MAGIC,
    WAL_FRAME_HEADER_SIZE,
    walCrc32Ieee,
    WalRecordKind,
    WalFrameView,
    WalRecoveryStatus,
    WalRecoveryReport,
    WalFrameWriter,
    WalRecoveryScanner,
} from "./wal_recovery.cp";

// --- lsm_compactor ---
export {
    MAX_LSM_LEVELS,
    MAX_LSM_SST_FILES,
    MAX_SST_KEY_LEN,
    MAX_COMPACTION_INPUTS,
    compareKeyBytesLex,
    LsmCompactionReason,
    LsmCompactionConfig,
    SstFileMeta,
    CompactionPlan,
    LsmLevelStats,
    LsmCompactionPlanner,
} from "./lsm_compactor.cp";

// --- mvcc_gc ---
export {
    MVCC_GC_MAX_VERSIONS,
    MVCC_GC_MAX_READERS,
    MvccVersionRecord,
    MvccReadSnapshot,
    MvccReadResult,
    MvccGcReport,
    MvccGcStats,
    MvccVersionGcStore,
} from "./mvcc_gc.cp";
