// Source: cp/lib/otlp/model.cp (OTLP Traces / Metrics / Logs data models)

pub const AnyValue = union(enum) {
    string_value: []const u8,
    bool_value: bool,
    int_value: i64,
    double_value: f64,
};

pub struct KeyValue {
    key: []const u8,
    value: AnyValue,
}

pub struct ResourceInfo {
    service_name: []const u8,
    service_version: ([]const u8)? = null,
    attributes: []const KeyValue = &[],
}

pub enum SpanKind : u8 {
    UNSPECIFIED = 0,
    INTERNAL = 1,
    SERVER = 2,
    CLIENT = 3,
    PRODUCER = 4,
    CONSUMER = 5,
}

pub enum SpanStatusCode : u8 {
    UNSET = 0,
    OK = 1,
    ERROR = 2,
}

pub struct SpanStatus {
    code: SpanStatusCode = SpanStatusCode.UNSET,
    message: ([]const u8)? = null,
}

pub struct SpanEvent {
    time_unix_nano: u64,
    name: []const u8,
    attributes: []const KeyValue = &[],
}

pub struct SpanLink {
    trace_id: [16]u8,
    span_id: [8]u8,
    trace_state: ([]const u8)? = null,
    attributes: []const KeyValue = &[],
}

pub struct OtlpSpan {
    trace_id: [16]u8,
    span_id: [8]u8,
    parent_span_id: ([8]u8)? = null,
    name: []const u8,
    kind: SpanKind = SpanKind.INTERNAL,
    start_time_unix_nano: u64,
    end_time_unix_nano: u64,
    attributes: []const KeyValue = &[],
    events: []const SpanEvent = &[],
    links: []const SpanLink = &[],
    status: SpanStatus = SpanStatus{},
}

pub enum MetricKind : u8 {
    COUNTER = 0,
    GAUGE = 1,
    HISTOGRAM = 2,
}

pub struct MetricDataPoint {
    time_unix_nano: u64,
    value_int: i64? = null,
    value_double: f64? = null,
    count: u64? = null,
    sum: f64? = null,
    bucket_counts: []const u64 = &[],
    explicit_bounds: []const f64 = &[],
    attributes: []const KeyValue = &[],
}

pub struct OtlpMetric {
    name: []const u8,
    description: []const u8 = "",
    unit: []const u8 = "",
    kind: MetricKind = MetricKind.COUNTER,
    is_monotonic: bool = true,
    data_points: []const MetricDataPoint = &[],
}

pub enum SeverityNumber : u8 {
    TRACE = 1,
    DEBUG = 5,
    INFO = 9,
    WARN = 13,
    ERROR = 17,
    FATAL = 21,
}

pub struct OtlpLogRecord {
    time_unix_nano: u64,
    severity_number: SeverityNumber = SeverityNumber.INFO,
    severity_text: []const u8 = "INFO",
    body: AnyValue,
    attributes: []const KeyValue = &[],
    trace_id: ([16]u8)? = null,
    span_id: ([8]u8)? = null,
}
