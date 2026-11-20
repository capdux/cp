// Source: cp/lib/math/index.cp (Unified Exports for cap:math)
// Unified Public Exports for Numerical Computing & AI Vector Retrieval Engine (cap:math & cap.math)

export {
    Vec2,
    Vec3,
    Vec4,
    Mat4,
} from "./vec.cp";

export {
    StatsErr,
    StatsSummary,
    LinearRegressionResult,
    Stats,
    pearsonCorrelation,
    linearRegression,
} from "./stats.cp";

export {
    QuantizedVecI8,
    dotF32,
    l2NormF32,
    normalizeL2,
    cosineSimilarityF32,
    euclideanDistanceF32,
    manhattanDistanceF32,
    softmaxInPlace,
    argMaxF32,
    topKIndices,
    quantizeSymmetricI8,
    dequantizeSymmetricI8,
    dotQuantizedI8,
} from "./embedding.cp";
