// Source: cp/lib/vector/index.cp (Unified Exports for cap:vector)
// Unified Public Exports for Local Vector Retrieval & Hybrid RAG Engine (cap:vector & cap.vector)

export {
    VectorError,
    DistanceMetric,
    SearchHit,
    VectorIndex,
} from "./flatIndex.cp";

export {
    Bm25Config,
    Bm25Index,
} from "./bm25.cp";

export {
    RrfFusion,
    RankedDoc,
    ScalarQuantParams,
    rrfFuse,
    normalizeScoresMinMax,
    normalizeScoresZScore,
    weightedScoreFuse,
    mmrRerank,
    quantizeVectorU8,
    dequantizeVectorU8,
    quantizedDotU8,
} from "./hybrid.cp";

export {
    SemanticEntry,
    SemanticLookupHit,
    SemanticCacheStats,
    SemanticCache,
} from "./semanticCache.cp";

export {
    BinaryCandidate,
    RescoredHit,
    binaryWordsForDim,
    quantizeBinary,
    hammingDistanceWords,
    binaryCosineApprox,
    searchBinaryTopK,
    twoStageSearch,
} from "./binaryQuant.cp";

export {
    IvfHit,
    IvfBuildStats,
    IvfFlatIndex,
} from "./ivf.cp";

export {
    PqCodebook,
    PqHit,
    pqEncodeVector,
    pqDecodeVector,
    pqBuildAdcTable,
    pqAdcDistanceSq,
    pqSearchAdcTopK,
} from "./pq.cp";

export {
    mmrSelect,
    colbertMaxSim,
    minMaxNormalizeInPlace,
    zScoreNormalizeInPlace,
    ndcgAtK,
    mrrScore,
    recallAtK,
} from "./reranker.cp";

export {
    FILTER_BITSET_WORDS,
    FILTER_BITSET_MAX_BITS,
    FilterBitset,
    FilterOp,
    NumericFilter,
    FilterStrategy,
    FilterPlan,
    evalInt64Filter,
    evalFloat64Filter,
    planFilterStrategy,
    maskedTopKCosine,
} from "./filter.cp";

export {
    simdDotF32,
    simdDotF64,
    simdL2SqF32,
    simdL2DistF32,
    simdCosineF32,
    simdAxpyInPlaceF32,
    simdBatchTopKCosineF32,
    simdBatchTopKL2F32,
} from "./simdOps.cp";

export {
    KMeansConfig,
    KMeansResult,
    kmeansInitPlusPlusF32,
    kmeansAssignF32,
    kmeansFitF32,
} from "./kmeans.cp";

export {
    NAV_MAX_DEGREE,
    NAV_INVALID_NODE,
    NavGraphHit,
    NavNodeAdj,
    NavGraphIndex,
} from "./navGraph.cp";

export {
    SparseEntry,
    SparseHit,
    sparseNormalizeInPlace,
    sparseDot,
    sparseCosine,
    sparsePruneTopK,
    sparseBatchTopK,
} from "./sparseVec.cp";

export {
    LatencyPercentiles,
    AnnEvalSummary,
    computeLatencyPercentilesInPlace,
    computeGroundTruthL2TopK,
    annRecallAtK,
    evaluateAnnBatch,
} from "./evalHarness.cp";

export {
    Sq4Codebook,
    sq4PackedBytes,
    sq4FitCodebook,
    sq4EncodeVector,
    sq4DecodeVector,
    sq4AsymmetricL2Sq,
    sq4BatchTopK,
} from "./quantSq4.cp";

export {
    RabitqCodeMeta,
    rabitqWordsForDim,
    rabitqSignFlipInPlace,
    rabitqFastWalshHadamardInPlace,
    rabitqEncodeUnit,
    rabitqEstimateDot,
    rabitqEstimateL2Sq,
} from "./rabitq.cp";

export {
    computeMeanF32,
    subtractMeanInPlaceF32,
    fitTopKPrincipalAxesF32,
    projectEmbeddingsF32,
    jlRandomProjectF32,
} from "./projReduce.cp";

export {
    HybridRagHit,
    HybridRrfFusion,
    RagContextChunk,
    TokenBudgetResult,
    mmrRerankSimdF32,
    dedupNearDuplicatesL2F32,
    selectWithinTokenBudget,
} from "./hybridRag.cp";

export {
    RagDocMeta,
    RagSearchHit,
    RagStoreStats,
    InMemoryRagStore,
} from "./ragStore.cp";

export {
    MatryoshkaConfig,
    MatryoshkaHit,
    MatryoshkaStats,
    truncateAndNormalizeF32,
    buildCoarseEmbeddingsF32,
    searchMatryoshkaTwoStageF32,
    searchMatryoshkaTwoStageL2F32,
} from "./matryoshka.cp";

export {
    COLBERT_MAX_DOCS,
    ColbertConfig,
    ColbertDocSpan,
    ColbertSearchHit,
    ColbertIndexStats,
    ColbertLiteIndex,
    prune_low_norm_tokens,
} from "./colbert.cp";

export {
    SIMHASH_MAX_DOCS,
    SIMHASH_NUM_BANDS,
    SimHashDedupConfig,
    SimHashDocEntry,
    SimHashMatch,
    SimHashDedupStats,
    hammingDistanceU64,
    computeSimHash64,
    SimHashDedupIndex,
} from "./simhash.cp";

export {
    BM25F_MAX_FIELDS,
    BM25F_MAX_DOCS,
    BM25F_MAX_POSTINGS,
    Bm25fFieldSpec,
    Bm25fConfig,
    Bm25fHit,
    Bm25fIndexStats,
    Bm25fIndex,
} from "./bm25f.cp";

export {
    BIN_IDX_MAX_DOCS,
    BIN_IDX_MAX_WORDS,
    BinaryIndexConfig,
    BinaryIndexHit,
    BinaryIndexStats,
    BinaryVectorIndex,
} from "./binIndex.cp";

export {
    SEM_ROUTER_MAX_ROUTES,
    SEM_ROUTER_MAX_EXEMPLARS,
    SEM_ROUTER_NAME_MAX_LEN,
    SemanticAggMode,
    SemanticRouteStatus,
    SemanticRouterConfig,
    SemanticRouteHit,
    SemanticRouteDecision,
    SemanticRouterStats,
    SemanticIntentRouter,
} from "./semRouter.cp";

export {
    RAPTOR_MAX_NODES,
    RAPTOR_MAX_LEVELS,
    RAPTOR_NO_NODE,
    RaptorNodeKind,
    RaptorSearchMode,
    RaptorScoreFusion,
    RaptorTreeConfig,
    RaptorNode,
    RaptorSearchHit,
    RaptorClusterAssignment,
    RaptorTreeStats,
    RaptorTree,
} from "./raptor.cp";

export {
    FHNSW_MAX_NODES,
    FHNSW_MAX_DEGREE,
    FHNSW_INVALID_SLOT,
    FHNSW_ANY_TENANT,
    FilteredSearchPath,
    FilteredGraphStatus,
    FilteredGraphConfig,
    FilteredPredicate,
    FilteredGraphNode,
    FilteredSearchHit,
    FilteredSearchReport,
    FilteredGraphStats,
    FilteredHnswIndex,
} from "./filtHnsw.cp";

export {
    LateChunkPoolMode,
    LateChunkSpan,
    LateChunkHit,
    LateChunkStats,
    sanitizeVectorF32,
    normalizeVectorInPlaceF32,
    detectSemanticBoundaries,
    poolLateChunkSpan,
    poolLateChunks,
    searchLateChunks,
    computeLateChunkStats,
} from "./lateChunk.cp";

export {
    SQ8_MAX_DIM,
    SQ8_MAX_DOCS,
    Sq8Metric,
    Sq8Codebook,
    Sq8Calibrator,
    Sq8ErrorStats,
    Sq8SearchHit,
    Sq8IndexStats,
    sq8CalibrateBatch,
    encodeF32ToU8,
    decodeU8ToF32,
    sq8AsymDotF32,
    sq8AsymL2SqF32,
    sq8SymDotU8,
    evaluateSq8Error,
    Sq8VectorIndex,
} from "./sq8Quant.cp";

export {
    MULTIVEC_MAX_DOCS,
    MULTIVEC_MAX_DIM,
    MULTIVEC_ARENA_FLOATS,
    MultiVecScoreMode,
    MultiVecHit,
    MultiVecTokenAlign,
    MultiVecIndexStats,
    MultiVecIndex,
} from "./multiVec.cp";

export {
    SEM_DEDUP_MAX_CLUSTERS,
    SEM_DEDUP_MAX_DIM,
    SEM_DEDUP_BPS_SCALE,
    SemanticDedupStatus,
    SemanticDedupConfig,
    SemanticCluster,
    SemanticDedupDecision,
    SemanticClusterHit,
    SemanticDedupBatchReport,
    SemanticDedupStats,
    SemanticDeduplicator,
} from "./semanticDedup.cp";

export {
    QFUSION_MAX_VIEWS,
    QFUSION_MAX_HITS_PER_VIEW,
    QFUSION_MAX_UNIQUE_DOCS,
    QFUSION_DEFAULT_RRF_K,
    QueryFusionStrategy,
    QueryCentroidReport,
    QueryViewHit,
    FusedDocHit,
    QueryFusionConfig,
    QueryFusionReport,
    fuseQueryVectorsF32,
    MultiQueryFusion,
} from "./queryFusion.cp";
