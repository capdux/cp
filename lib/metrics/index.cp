// Source: cp/lib/metrics/index.cp (Unified Exports for cap:metrics)
// Unified Public Exports for Prometheus/OpenMetrics Observability Engine (cap:metrics & cap.metrics).

export {
    MetricsError,
    MetricLabel,
    MetricNameValidator,
    Counter,
    Gauge,
} from "./primitives.cp";

export {
    DEFAULT_HTTP_BUCKETS,
    HistogramState,
    MetricsHistogram,
} from "./histogram.cp";

export {
    MetricKind,
    MetricsRegistry,
} from "./registry.cp";
