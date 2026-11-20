// Source: cp/lib/otlp/index.cp (OpenTelemetry OTLP standard library entry)
export {
    MAX_BAGGAGE_BYTES,
    MAX_BAGGAGE_ENTRIES,
    OtlpError,
    TraceParent,
    BaggageEntry,
    parseTraceParent,
    formatTraceParent,
    parseBaggage,
    formatBaggage,
    freeBaggage,
} from "./context.cp";

export {
    AnyValue,
    KeyValue,
    ResourceInfo,
    SpanKind,
    SpanStatusCode,
    SpanStatus,
    SpanEvent,
    SpanLink,
    OtlpSpan,
    MetricKind,
    MetricDataPoint,
    OtlpMetric,
    SeverityNumber,
    OtlpLogRecord,
} from "./model.cp";

export {
    encodeExportTraceRequestJson,
    encodeExportMetricsRequestJson,
    encodeExportLogsRequestJson,
} from "./json.cp";
