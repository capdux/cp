// Source: cp/lib/vector/index.cp (Unified Exports for cap:vector)
// Unified Public Exports for Local Vector Retrieval & Hybrid RAG Engine (cap:vector & cap.vector)

export {
    VectorError,
    DistanceMetric,
    SearchHit,
    VectorIndex,
} from "./flat_index.cp";

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
} from "./semantic_cache.cp";

export {
    BinaryCandidate,
    RescoredHit,
    binaryWordsForDim,
    quantizeBinary,
    hammingDistanceWords,
    binaryCosineApprox,
    searchBinaryTopK,
    twoStageSearch,
} from "./binary_quant.cp";

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
} from "./simd_ops.cp";

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
} from "./nav_graph.cp";

export {
    SparseEntry,
    SparseHit,
    sparseNormalizeInPlace,
    sparseDot,
    sparseCosine,
    sparsePruneTopK,
    sparseBatchTopK,
} from "./sparse_vec.cp";

export {
    LatencyPercentiles,
    AnnEvalSummary,
    computeLatencyPercentilesInPlace,
    computeGroundTruthL2TopK,
    annRecallAtK,
    evaluateAnnBatch,
} from "./eval_harness.cp";

export {
    Sq4Codebook,
    sq4PackedBytes,
    sq4FitCodebook,
    sq4EncodeVector,
    sq4DecodeVector,
    sq4AsymmetricL2Sq,
    sq4BatchTopK,
} from "./quant_sq4.cp";

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
} from "./proj_reduce.cp";

export {
    HybridRagHit,
    HybridRrfFusion,
    RagContextChunk,
    TokenBudgetResult,
    mmrRerankSimdF32,
    dedupNearDuplicatesL2F32,
    selectWithinTokenBudget,
} from "./hybrid_rag.cp";

export {
    RagDocMeta,
    RagSearchHit,
    RagStoreStats,
    InMemoryRagStore,
} from "./rag_store.cp";

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
} from "./colbert_index.cp";

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
} from "./simhash_dedup.cp";

export {
    BM25F_MAX_FIELDS,
    BM25F_MAX_DOCS,
    BM25F_MAX_POSTINGS,
    Bm25fFieldSpec,
    Bm25fConfig,
    Bm25fHit,
    Bm25fIndexStats,
    Bm25fIndex,
} from "./bm25f_index.cp";

export {
    BIN_IDX_MAX_DOCS,
    BIN_IDX_MAX_WORDS,
    BinaryIndexConfig,
    BinaryIndexHit,
    BinaryIndexStats,
    BinaryVectorIndex,
} from "./binary_index.cp";

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
} from "./semantic_router.cp";

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
} from "./raptor_tree.cp";
