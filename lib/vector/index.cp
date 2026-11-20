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
} from "./hybrid.cp";
