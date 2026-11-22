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
