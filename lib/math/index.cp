// Source: cp/lib/math/index.cp (Unified Exports for cap:math)
// Unified Public Exports for Numerical Computing, Linear Algebra, Special Functions, Optimization, Filtering & AI Vector Retrieval Engine (cap:math & cap.math)

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

// --- Linear Algebra & Matrix Decompositions ---

export {
    LuInfo,
    luFactorize,
    luSolve,
    luDeterminant,
    luInverse,
    matVecResidualInf,
    solveTridiagonal,
} from "./lu.cp";

export {
    QR_DEFAULT_TOL,
    qrFactorize,
    qrApplyQt,
    qrFormQ,
    qrExtractR,
    qrSolveLeastSquares,
    qrOrthogonalityError,
    qrReconstructionError,
} from "./qr.cp";

export {
    SvdResult,
    svdJacobi,
    svdSolve,
    svdPseudoinverse,
    svdReconstructionError,
} from "./svd.cp";

export {
    jacobiEigenSymmetric,
    PcaFitResult,
    pcaFit,
    pcaTransform,
    pcaInverseTransform,
    pcaReconstructionMse,
} from "./pca.cp";

// --- Special Functions & Numerical Quadrature ---

export {
    j0,
    j1,
    jn,
    y0,
    y1,
    yn,
    besselI0,
    besselI1,
    i0e,
    i1e,
    k0,
    k1,
    k0e,
    k1e,
} from "./bessel.cp";

export {
    lgamma,
    tgamma,
    digamma,
    gammaP,
    gammaQ,
    lbeta,
    beta,
    betainc,
    erf,
    erfc,
    logFactorial,
    logBinomial,
} from "./gamma.cp";

export {
    IntegrandFn,
    Gk15Estimate,
    QuadResult,
    gaussLegendre5,
    gaussLegendre10,
    compositeSimpson,
    gaussKronrod15,
    adaptiveGk15,
    romberg,
} from "./quadrature.cp";

// --- Mathematical Optimization & Root-Finding ---

export {
    ConstraintKind,
    SimplexStatus,
    SimplexResult,
    SimplexWorkspace,
    simplexRequiredTableauLen,
    solveGeneral,
    solveMaxLe,
    solveMinLe,
} from "./simplex.cp";

export {
    ValueGradFn,
    ValueOnlyFn,
    BfgsConfig,
    BfgsResult,
    BfgsWorkspace,
    LbfgsWorkspace,
    bfgsMinimize,
    lbfgsMinimize,
    finiteDiffGradient,
} from "./bfgs.cp";

export {
    ObjectiveFn,
    ScalarFn,
    NelderConfig,
    NelderWorkspace,
    NelderResult,
    Minimize1DResult,
    Root1DResult,
    nelderMead,
    goldenSectionMin,
    brentRoot,
} from "./nelder.cp";

export {
    ResidualFn,
    NewtonConfig,
    NewtonResult,
    NewtonWorkspace,
    LmConfig,
    LmResult,
    LmWorkspace,
    RootResult,
    newtonSystem,
    levenbergMarquardt,
    newton1D,
    secant1D,
} from "./newton.cp";

export {
    CoolingSchedule,
    RealEnergyFn,
    PermEnergyFn,
    AnnealConfig,
    AnnealResult,
    temperatureAt,
    metropolisAccept,
    annealContinuous,
    annealPermutation,
} from "./anneal.cp";

export {
    GlobalOptResult,
    PsoConfig,
    PsoWorkspace,
    DeConfig,
    DeWorkspace,
    psoMinimize,
    deMinimize,
} from "./pso.cp";

export {
    RealObjectiveFn,
    PermObjectiveFn,
    RealGaConfig,
    PermGaConfig,
    GaResult,
    RealGaWorkspace,
    PermGaWorkspace,
    isValidPermutation,
    orderCrossover,
    realGaMinimize,
    permGaMinimize,
} from "./ga.cp";

// --- State Estimation & Sensor Fusion Filters ---

export {
    Kalman1D,
    Kalman2D,
    AlphaBetaFilter,
} from "./kalman.cp";

export {
    TransitionFn,
    MeasurementFn,
    EkfWorkspace,
    UkfParams,
    numericalJacobianF,
    numericalJacobianH,
    ekfPredict,
    ekfUpdate,
    generateSigmaPoints,
    ukfPredict,
} from "./ekf.cp";
