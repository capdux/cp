// Origin: Pure-CP Stdlib (Unified Classical Machine Learning, Clustering & Search Suite: cap:ml) | Created: 2026-11-22
// ============================================================================
// Pure-CP Zero-Alloc Classical Machine Learning, Clustering, Dimensionality
// Reduction, Robust Estimation, Approximate Nearest Neighbors & RL/Search.
// ============================================================================

// --- Classification, Regression & Dimensionality Reduction ---
export {
    KernelKind,
    KernelParams,
    SmoConfig,
    SmoResult,
    LinearSvmResult,
    evalKernel,
    svmFitSmo,
    svmDecision,
    svmPredict,
    linearSvmFit,
    linearSvmDecision,
} from "./svm.cp";

export {
    DistanceMetric,
    WeightMode,
    Neighbor,
    distanceMetric,
    findKNearest,
    knnClassify,
    knnRegress,
    knnLeaveOneOutAccuracy,
} from "./knn.cp";

export {
    DTREE_NO_NODE,
    SplitCriterion,
    TreeNode,
    TreeConfig,
    TreeWorkspace,
    DecisionTree,
} from "./dtree.cp";

export {
    sigmoid,
    logLossBinary,
    softmax,
    LogRegConfig,
    LogRegResult,
    MultinomialResult,
    ClassificationMetrics,
    fitBinary,
    predictProbaBinary,
    predictBinary,
    fitMultinomial,
    predictProbaMultinomial,
    predictClassMultinomial,
    binaryMetrics,
} from "./logreg.cp";

export {
    GaussianNbModel,
    fitGaussianNb,
    gaussianNbPredictProba,
    gaussianNbPredict,
    MultinomialNbModel,
    fitMultinomialNb,
    multinomialNbPredictProba,
    multinomialNbPredict,
    BernoulliNbModel,
    fitBernoulliNb,
    bernoulliNbPredictProba,
    bernoulliNbPredict,
} from "./naivebayes.cp";

export {
    softThreshold,
    LassoConfig,
    LassoResult,
    LassoWorkspace,
    elasticNetFit,
    lassoFit,
    ridgeFit,
    linearPredict,
    mseScore,
    r2Score,
} from "./lasso.cp";

export {
    PavaWorkspace,
    IsotonicModel,
    pavaIncreasing,
    pavaDecreasing,
    fitIsotonic,
    isotonicPredict,
    brierScore,
    expectedCalibrationError,
} from "./isotonic.cp";

export {
    RobustKind,
    HuberRegConfig,
    HuberRegResult,
    HuberRegWorkspace,
    TheilSenResult,
    median,
    madScale,
    trimmedMean,
    huberLocation,
    huberLoss,
    huberWeight,
    tukeyBiweight,
    huberRegress,
    theilSen1D,
} from "./huber.cp";

export {
    NmfLoss,
    NmfConfig,
    NmfResult,
    NmfWorkspace,
    nmfFit,
    nmfTransform,
    nmfReconstruct,
    nmfFrobeniusError,
    nmfKlDivergence,
} from "./nmf.cp";

export {
    MdsResult,
    SmacofResult,
    pairwiseEuclidean,
    kruskalStress1,
    doubleCenterGram,
    classicalMds,
    smacofRefine,
} from "./mds.cp";

export {
    RansacConfig,
    Point2,
    Point3,
    Line2Model,
    Circle2Model,
    Plane3Model,
    Linear1DModel,
    ransacFitLine2D,
    ransacFitCircle2D,
    ransacFitPlane3D,
    ransacFitLinear1D,
} from "./ransac.cp";

// --- Clustering, Community Detection & Approximate Retrieval ---
export {
    DBSCAN_NOISE,
    DBSCAN_UNVISITED,
    DbscanMetric,
    DbscanResult,
    DbscanWorkspace,
    dbscanCluster,
    dbscanFromDistanceMatrix,
    silhouetteScore,
} from "./dbscan.cp";

export {
    GmmConfig,
    GmmFitResult,
    gmmFit,
    gmmLogProbSample,
    gmmPredictProba,
    gmmPredict,
    gmmScoreSamples,
} from "./gmm.cp";

export {
    LaplacianKind,
    FiedlerResult,
    buildLaplacian,
    fiedlerBisection,
    spectralEmbedding,
    normalizedCut,
    rbfAffinityMatrix,
} from "./spectral.cp";

export {
    WeightedGraph,
    LouvainConfig,
    LouvainResult,
    LouvainWorkspace,
    computeModularity,
    louvainDetect,
    labelPropagation,
} from "./louvain.cp";

export {
    LSH_NO_ENTRY,
    LshEntry,
    CosineLsh,
    EuclideanLsh,
    estimatedCosineFromHamming,
} from "./lsh.cp";

// --- Sequential Decision Making & Tree Search ---
export {
    Ucb1Bandit,
    EpsilonGreedyBandit,
    ThompsonBandit,
    Exp3Bandit,
    cumulativeRegret,
} from "./bandit.cp";

export {
    MCTS_NO_NODE,
    MctsNode,
    MctsConfig,
    MctsTree,
} from "./mcts.cp";

// --- Wave-2: K-Means++, PageRank/HITS, Polyfit, Probability & Optimization ---

export {
    KmeansResult,
    sqDist,
    kmeansPredictOne,
    kmeansInitPlusPlus,
    kmeansFit,
    kmeansInertia,
    silhouetteScore as kmeansSilhouetteScore,
} from "./kmeans.cp";

export {
    CsrGraph,
    PageRankResult,
    HitsResult,
    pagerank,
    personalizedPagerank,
    hits,
    topNode,
    sumScores,
} from "./pagerank.cp";

export {
    POLYFIT_MAX_DEG,
    LinearFit,
    FitMetrics,
    polyEval,
    polyDerivEval,
    polyDerivCoeffs,
    polyIntegrateCoeffs,
    linearFit,
    polyfit,
    polyfitRidge,
    evalFitMetrics,
    polyRootBisect,
    polyRootNewton,
} from "./polyfit.cp";

export {
    ProbError,
    Rng,
    NormalDist,
    ExponentialDist,
    BinomialDist,
    PoissonDist,
    AbTestResult,
    HypothesisTest,
} from "./prob.cp";

export {
    OptError,
    ScalarFn,
    VectorFn,
    GradFn,
    RootResult,
    MinResult1D,
    OptResult,
    Optimizer,
} from "./optimize.cp";

export {
    LinalgError,
    Matrix,
    dot,
    vecNorm1,
    vecNorm2,
    vecNormInf,
    leastSquares,
} from "./linalg.cp";

// --- AI Dataset Dedup, Guardrails, Experimentation & Pathfinding ---

export {
    DedupError,
    SimHashHit,
    LshHit,
    SimHash,
    SimHashIndex,
    MinHash,
    LshIndex,
} from "./dedup.cp";

export {
    GuardrailError,
    PiiKind,
    PiiFinding,
    RedactionResult,
    PiiScanner,
} from "./pii.cp";

export {
    GuardrailAction,
    GuardrailVerdict,
    GuardrailPolicy,
} from "./policy.cp";
