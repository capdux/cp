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
