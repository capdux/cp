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

// --- Digital Signal Processing & Spectral Analysis ---

export {
    FilterType,
    BiquadCoeffs,
    BiquadFilter,
    FreqResponse,
    GoertzelResult,
    designBiquad,
    processSosBlock,
    frequencyResponse,
    goertzelDetect,
} from "./biquad.cp";

export {
    SosSection,
    SosState,
    designButterworthLowpass,
    designButterworthHighpass,
    designChebyshev1Lowpass,
    sosFilter,
    sosFiltFilt,
    sosMagnitudeDb,
} from "./iir.cp";

export {
    windowHann,
    windowHamming,
    windowBlackman,
    designLowpassFir,
    FirFilter,
    Biquad,
} from "./fir.cp";

export {
    FftError,
    Complex,
    WindowKind,
    FrequencyPeak,
    Fft,
} from "./fft.cp";

export {
    haarForward1d,
    haarInverse1d,
    haarForwardMulti1d,
    haarInverseMulti1d,
    db4Forward1d,
    db4Inverse1d,
    cdf53ForwardI32,
    cdf53InverseI32,
    softThreshold,
    hardThreshold,
    signalEnergy,
} from "./wavelet.cp";

// --- Time Series, Smoothing, Change-Point & Alignment ---

export {
    BoundaryMode,
    SavgolWorkspace,
    LoessConfig,
    savgolCoeffs,
    savgolFilter,
    loessSmooth1D,
} from "./savitzky.cp";

export {
    SeasonalityKind,
    SesResult,
    HoltConfig,
    HoltResult,
    HoltWintersConfig,
    HoltWintersResult,
    ForecastMetrics,
    sesForecast,
    holtForecast,
    holtWintersForecast,
    evaluateForecast,
} from "./holt.cp";

export {
    ShiftDirection,
    ChangePoint,
    CusumDetector,
    PageHinkleyDetector,
    EwmaControlChart,
    StreamingZScore,
    scanCusum,
} from "./cusum.cp";

export {
    DTW_INF,
    WarpPair,
    DtwPathResult,
    dtwDistance1D,
    dtwAlign1D,
    dtwDistanceMulti,
    computeKeoghEnvelope,
    lbKeogh,
    zNormalize,
} from "./dtw.cp";

// --- Stochastic Processes, Probabilistic Filtering & Space-Filling Curves ---

export {
    HmmModel,
    ViterbiResult,
    viterbiDecode,
    forwardScaled,
    backwardScaled,
    posteriorStates,
    baumWelchStep,
} from "./hmm.cp";

export {
    isStochasticMatrix,
    stepDistribution,
    transitionPower,
    StationaryResult,
    stationaryPowerIter,
    absorbingFundamental,
    expectedAbsorptionSteps,
    absorptionProbabilities,
    sampleTrajectory,
    entropyRate,
} from "./markov.cp";

export {
    ResampleKind,
    PropagateFn,
    LogLikelihoodFn,
    normalizeLogWeights,
    effectiveSampleSize,
    resampleIndices,
    ParticleFilter,
} from "./particle.cp";

export {
    HILBERT_MIN_ORDER,
    HILBERT_MAX_ORDER,
    MORTON3D_MAX_COORD,
    Point2D,
    Point3D,
    hilbert2dMaxIndex,
    hilbert2dEncode,
    hilbert2dDecode,
    morton2dEncode,
    morton2dDecode,
    morton3dEncode,
    morton3dDecode,
    manhattanDist2d,
} from "./hilbert.cp";

// --- Interpolation, ODE Solvers & SPD Matrix Factorization ---

export {
    CubicSegment,
    CubicSpline,
    buildNaturalCubicSpline,
    buildClampedCubicSpline,
    buildPchipSpline,
    catmullRom1d,
    catmullRom1dDeriv,
} from "./spline.cp";

export {
    OdeRhsFn,
    AccelFn,
    Rk45Result,
    rk2MidpointStep,
    rk4Step,
    rk4Integrate,
    rk45Step,
    rk45Integrate,
    verletStep,
} from "./ode.cp";

export {
    choleskyDecompose,
    forwardSubstitute,
    backwardSubstituteT,
    choleskySolve,
    choleskyLogDet,
    choleskyDet,
    choleskyInvert,
    ldltDecompose,
    ldltSolve,
} from "./cholesky.cp";

// ============================================================================
// Batch 40: 128-bit Fixed-Point Decimal Arithmetic & ISO 4217 Money
// ============================================================================
export {
    MAX_SCALE,
    DecimalError,
    RoundingMode,
    Decimal,
} from "./decimal.cp";

export {
    MoneyError,
    Money,
} from "./decimalMoney.cp";

// --- SIMD Vectorized Statistics & Linear Algebra Facade ---

export {
    SimdSummaryStats,
    simdSummaryStatsF32,
    simdPearsonCorrF32,
    simdCosineSimF32,
    simdEuclideanDistF32,
    simdZScoreNormalizeF32,
    simdSoftmaxF32,
    simdMatVecF32,
    simdAxpyF32,
} from "./simdStats.cp";
