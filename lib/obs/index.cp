// Source: cp/lib/obs/index.cp (Unified Exports for cap:obs)
// Unified Public Exports for Observability, OpenMetrics, SIEM/Access Logs, Static Analysis/CI Reports, eBPF BTF & LLM Eval Metrics (cap:obs & cap.obs)

// --- OpenMetrics / Prometheus Exposition Format ---

export {
    MetricType,
    Label,
    Sample,
    MetricFamily,
    MetricsDocument,
    parse,
} from "./openmetrics.cp";

// --- SIEM & HTTP Access Log Parsers (CEF, LEEF, NCSA CLF) ---

export {
    CEF_PREFIX,
    CefEvent,
    CefExtPair,
    CefExtIter,
    isCef,
    parseCefLine,
    findCefExt,
    unescapeCefHeader,
    unescapeCefExtValue,
    formatCefHeader,
} from "./cef.cp";

export {
    LEEF_PREFIX,
    LEEF_DEFAULT_DELIM,
    LeefEvent,
    LeefAttr,
    LeefAttrIter,
    isLeef,
    parseLeefLine,
    findLeefAttr,
    formatLeefHeader,
} from "./leef.cp";

export {
    ClfEntry,
    ClfStats,
    ClfIter,
    isClf,
    parseClfLine,
    summarizeClf,
} from "./clf.cp";

// --- Static Analysis, CI Test Reports & LLM Eval Metrics (SARIF, JUnit, TAP, Eval) ---

export {
    SarifLevel,
    SarifHeader,
    SarifRun,
    SarifResult,
    SarifStats,
    SarifRunIter,
    SarifResultIter,
    isSarif,
    parseSarifHeader,
    summarizeSarif,
} from "./sarif.cp";

export {
    JunitOutcome,
    JunitSuite,
    JunitCase,
    JunitSummary,
    JunitSuiteIter,
    JunitCaseIter,
    isJunitXml,
    summarizeJunit,
} from "./junit.cp";

export {
    TapEventKind,
    TapDirective,
    TapEvent,
    TapSummary,
    TapIter,
    summarizeTap,
} from "./tap.cp";

export {
    EvalError,
    MatchMode,
    EvalScorer,
} from "./evalMetrics.cp";

export {
    EvalCase,
    EvalSummary,
    RegressionDiff,
    EvalReport,
} from "./evalSuite.cp";

// --- Kernel eBPF BTF Type Graph Inspector ---

export {
    BTF_MAGIC,
    BTF_VERSION,
    BTF_HEADER_SIZE,
    BTF_TYPE_HEADER_SIZE,
    KIND_UNKN,
    KIND_INT,
    KIND_PTR,
    KIND_ARRAY,
    KIND_STRUCT,
    KIND_UNION,
    KIND_ENUM,
    KIND_FWD,
    KIND_TYPEDEF,
    KIND_VOLATILE,
    KIND_CONST,
    KIND_RESTRICT,
    KIND_FUNC,
    KIND_FUNC_PROTO,
    KIND_VAR,
    KIND_DATASEC,
    KIND_FLOAT,
    KIND_DECL_TAG,
    KIND_TYPE_TAG,
    KIND_ENUM64,
    INT_SIGNED,
    INT_CHAR,
    INT_BOOL,
    BtfHeader,
    BtfType,
    BtfIntInfo,
    BtfArray,
    BtfMember,
    BtfTypeIter,
    BtfSection,
    isBtf,
    parseBtf,
    parseBtfInt,
    parseBtfArray,
    btfMemberAt,
} from "./btf.cp";

// --- Zero-Alloc OTLP Span Exporter & Logarithmic Latency Histogram ---

export {
    HISTOGRAM_BUCKET_COUNT,
    LatencyBucketHistogram,
    OtlpSpanRecord,
    OtlpSpanBatch,
} from "./telemetry.cp";

// --- Zero-Alloc Structured Logger (JSONL / Logfmt + Trace Correlation) ---

export {
    OBS_LOG_MAX_FIELDS,
    ObsLogLevel,
    ObsLogFieldKind,
    ObsLogField,
    ObsLogRecord,
    ObsLogFormat,
    ObsLoggerConfig,
    ObsLoggerStats,
    ObsStructuredLogger,
} from "./structLog.cp";

// --- Zero-Alloc Online Welford/EWMA Anomaly Detector & Alert Cooldown Controller ---

export {
    AnomalyConfig,
    AnomalySeverity,
    AnomalyEval,
    AnomalyDetectorStats,
    OnlineAnomalyDetector,
} from "./anomaly.cp";

// --- Zero-Alloc HDR Log-Linear High-Dynamic-Range Latency Histogram ---

export {
    HDR_SUB_BUCKETS,
    HDR_NUM_SCALES,
    HDR_TOTAL_BUCKETS,
    HDR_MAX_SUPPORTED_VALUE,
    HdrQuantileSummary,
    HdrHistogram,
} from "./hdrHist.cp";

// --- Zero-Alloc OpenTelemetry Tail-Based Trace Sampler ---

export {
    TAIL_SAMPLER_MAX_TRACES,
    TailSampleReason,
    TailSamplerConfig,
    ActiveTraceSlot,
    TailSampleDecision,
    TailSamplerStats,
    TailTraceSampler,
} from "./traceSample.cp";

// --- Zero-Alloc Google SRE Multi-Window Multi-Burn-Rate SLO Monitor ---

export {
    SLO_RING_MINUTES,
    SloAlertSeverity,
    SloBurnConfig,
    SloWindowMetrics,
    SloBurnEvaluation,
    SloBurnMonitor,
} from "./sloBurn.cp";

// --- Zero-Alloc OpenTelemetry / Prometheus Histogram Exemplar Reservoir ---

export {
    EXEMPLAR_MAX_BOUNDS,
    EXEMPLAR_MAX_BUCKETS,
    EXEMPLAR_MAX_RESERVOIR_SLOTS,
    EXEMPLAR_FLAG_SAMPLED,
    ExemplarStrategy,
    ExemplarRecord,
    ExemplarBucketSummary,
    ExemplarReservoirStats,
    formatExemplarOpenMetrics,
    AlignedHistogramExemplarReservoir,
} from "./exemplar.cp";

// --- Zero-Alloc Prometheus / OpenTelemetry High-Cardinality Guard & __overflow__ Rollup ---

export {
    CARD_MAX_METRICS,
    CARD_MAX_SERIES,
    CARD_HLL_REGISTERS,
    CARD_OVERFLOW_LABEL,
    CardinalityOverflowPolicy,
    CardinalityDecision,
    MetricQuotaConfig,
    MetricCardinalitySnapshot,
    CardinalityGuardStats,
    fnv1aHash64,
    computeSeriesFingerprint,
    labelValueOrOverflow,
    CardinalityGuard,
} from "./cardinality.cp";

// --- Zero-Alloc Sampling Profiler Stack Aggregator & Brendan Gregg Folded Flamegraph Exporter ---

export {
    FLAME_MAX_SYMBOLS,
    FLAME_SYMBOL_POOL_BYTES,
    FLAME_MAX_STACKS,
    FLAME_MAX_DEPTH,
    FlameRecordStatus,
    FlameSymbolEntry,
    FlameStackEntry,
    FlameHotspot,
    FlameHotspotDiff,
    FlameFoldStats,
    FlameFoldProfiler,
    diffHotspots,
} from "./flameFold.cp";

// --- Zero-Alloc Distributed Tracing Flight Recorder / Ring Span Buffer ---

export {
    RING_TRACE_CAPACITY,
    RING_TRACE_FROZEN_CAPACITY,
    RING_TRACE_OP_MAX_LEN,
    RING_TRACE_TAG_MAX_LEN,
    RingSpanStatus,
    RingFreezeReason,
    RingSpan,
    SpanSelfTime,
    TraceAnalysis,
    RingTraceStats,
    RingTraceRecorder,
    analyzeTraceSpans,
    formatTraceSummary,
    formatChromeTraceEvent,
} from "./ringTrace.cp";

// --- Zero-Alloc Adaptive Structured Log Sampler & Storm Suppressor ---

export {
    LOG_SAMPLER_MAX_BUCKETS,
    LOG_SAMPLER_MAX_SUMMARIES,
    logSignatureHash,
    LogSampleLevel,
    LogSampleVerdict,
    LogSamplerConfig,
    LogSampleDecision,
    LogSuppressionSummary,
    LogSamplerStats,
    AdaptiveLogSampler,
} from "./logSample.cp";

// --- Zero-Alloc Prometheus-Style Alert Rules Evaluator & `for` Hysteresis FSM ---

export {
    ALERT_MAX_RULES,
    ALERT_MAX_SILENCES,
    ALERT_MAX_INHIBITIONS,
    ALERT_NAME_MAX_LEN,
    AlertCompareOp,
    AlertSeverity,
    AlertState,
    AlertNotifyKind,
    AlertSilenceTarget,
    AlertRuleSpec,
    AlertRuleEntry,
    AlertSilence,
    AlertInhibition,
    AlertEvalOutcome,
    FiringAlertSnapshot,
    AlertEngineStats,
    AlertRuleEngine,
} from "./alertRules.cp";

// --- Zero-Alloc Kayenta/Flagger-Style Canary Statistical Gate & Auto-Rollback Judge ---

export {
    CANARY_MAX_METRICS,
    CANARY_NAME_MAX_LEN,
    CANARY_BPS_SCALE,
    CanaryDirection,
    CanaryMetricStatus,
    CanaryVerdict,
    CanaryMetricSpec,
    CanaryMetricResult,
    CanaryGateConfig,
    CanaryJudgeReport,
    evaluateMetricSample,
    formatCanaryVerdict,
    formatCanaryReportJson,
    CanaryGateJudge,
} from "./canaryGate.cp";

// --- Zero-Alloc W3C Baggage Distributed Context Propagation & Redaction Engine ---

export {
    BAGGAGE_MAX_ENTRIES,
    BAGGAGE_MAX_HEADER_BYTES,
    BAGGAGE_MAX_MEMBER_BYTES,
    BAGGAGE_MAX_KEY_LEN,
    BAGGAGE_MAX_VALUE_LEN,
    BAGGAGE_MAX_PROPS_LEN,
    BaggageOpStatus,
    BaggageEntry,
    BaggageParseReport,
    isValidBaggageKey,
    BaggageContext,
    parseBaggageHeader,
    formatBaggageHeader,
} from "./baggage.cp";
