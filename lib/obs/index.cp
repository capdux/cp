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
} from "./eval_metrics.cp";

export {
    EvalCase,
    EvalSummary,
    RegressionDiff,
    EvalReport,
} from "./eval_suite.cp";

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
} from "./telemetry_export.cp";

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
} from "./structured_log.cp";
