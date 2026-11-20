// Source: cp/cap/test/test_otlp_production_deep.cp (OTLP deep production tests)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    OtlpError,
    TraceParent,
    BaggageEntry,
    parseTraceParent,
    formatTraceParent,
    parseBaggage,
    formatBaggage,
    freeBaggage,
    AnyValue,
    KeyValue,
    ResourceInfo,
    SpanKind,
    SpanStatusCode,
    SpanStatus,
    SpanEvent,
    OtlpSpan,
    MetricKind,
    MetricDataPoint,
    OtlpMetric,
    SeverityNumber,
    OtlpLogRecord,
    encodeExportTraceRequestJson,
    encodeExportMetricsRequestJson,
    encodeExportLogsRequestJson,
} from "cap:otlp";

test("W3C TraceParent parse, format, newChildSpan and invalid rejection") {
    const alloc = std.testing.allocator;
    const valid = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    const tp = try parseTraceParent(valid);
    expect_eq(tp.version, 0);
    expect_eq(tp.isSampled(), true);

    const formatted = try formatTraceParent(alloc, &tp);
    defer alloc.free(formatted);
    try std.testing.expectEqualStrings(valid, formatted);

    const child_id: [8]u8 = [0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88];
    const child = try tp.newChildSpan(child_id);
    const child_hdr = try formatTraceParent(alloc, &child);
    defer alloc.free(child_hdr);
    try std.testing.expectEqualStrings("00-4bf92f3577b34da6a3ce929d0e0e4736-1122334455667788-01", child_hdr);

    try std.testing.expectError(OtlpError.ZeroTraceId, parseTraceParent("00-00000000000000000000000000000000-00f067aa0ba902b7-01"));
    try std.testing.expectError(OtlpError.ZeroSpanId, parseTraceParent("00-4bf92f3577b34da6a3ce929d0e0e4736-0000000000000000-01"));
    try std.testing.expectError(OtlpError.InvalidVersion, parseTraceParent("ff-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"));
    try std.testing.expectError(OtlpError.InvalidTraceParent, parseTraceParent("00-short-01"));
}

test("W3C Baggage roundtrip, percent-encoding and malformed/limit rejection") {
    const alloc = std.testing.allocator;
    const raw = "userId=alice%20smith,tenant=acme%2Fprod;ttl=60";
    const entries = try parseBaggage(alloc, raw);
    defer freeBaggage(alloc, entries);

    expect_eq(entries.len, 2);
    try std.testing.expectEqualStrings("userId", entries[0].key);
    try std.testing.expectEqualStrings("alice smith", entries[0].value);
    try std.testing.expectEqualStrings("acme/prod", entries[1].value);
    try std.testing.expectEqualStrings("ttl=60", entries[1].metadata.?);

    const encoded = try formatBaggage(alloc, entries);
    defer alloc.free(encoded);
    try std.testing.expectEqualStrings("userId=alice%20smith,tenant=acme%2Fprod;ttl=60", encoded);

    try std.testing.expectError(OtlpError.InvalidBaggage, parseBaggage(alloc, "a=1,,b=2"));
    try std.testing.expectError(OtlpError.InvalidBaggage, parseBaggage(alloc, "missing_eq"));

    let big_buf: [8200]u8 = undefined;
    @memset(&big_buf, 'a');
    try std.testing.expectError(OtlpError.BaggageLimitExceeded, parseBaggage(alloc, &big_buf));
}

test("OTLP Trace JSON export with escaping, events, attributes and status") {
    const alloc = std.testing.allocator;
    const res = ResourceInfo{
        .service_name = "cp-gateway",
        .service_version = "1.2.3",
    };

    const attrs = [
        KeyValue{ .key = "http.method", .value = AnyValue{ .string_value = "GET" } },
        KeyValue{ .key = "http.status_code", .value = AnyValue{ .int_value = 200 } },
        KeyValue{ .key = "cache.hit", .value = AnyValue{ .bool_value = true } },
    ];
    const events = [
        SpanEvent{
            .time_unix_nano = 1700000000000000500,
            .name = "query \"db\"\nready",
        },
    ];
    const spans = [
        OtlpSpan{
            .trace_id = [0x4b, 0xf9, 0x2f, 0x35, 0x77, 0xb3, 0x4d, 0xa6, 0xa3, 0xce, 0x92, 0x9d, 0x0e, 0x0e, 0x47, 0x36],
            .span_id = [0x00, 0xf0, 0x67, 0xaa, 0x0b, 0xa9, 0x02, 0xb7],
            .name = "GET /v1/chat",
            .kind = SpanKind.SERVER,
            .start_time_unix_nano = 1700000000000000000,
            .end_time_unix_nano = 1700000000001000000,
            .attributes = &attrs,
            .events = &events,
            .status = SpanStatus{ .code = SpanStatusCode.OK },
        },
    ];

    const json = try encodeExportTraceRequestJson(alloc, &res, &spans);
    defer alloc.free(json);

    try std.testing.expect(std.mem.indexOf(u8, json, "\"resourceSpans\":[") != null);
    try std.testing.expect(std.mem.indexOf(u8, json, "\"traceId\":\"4bf92f3577b34da6a3ce929d0e0e4736\"") != null);
    try std.testing.expect(std.mem.indexOf(u8, json, "\"spanId\":\"00f067aa0ba902b7\"") != null);
    try std.testing.expect(std.mem.indexOf(u8, json, "\"query \\\"db\\\"\\nready\"") != null);
    try std.testing.expect(std.mem.indexOf(u8, json, "\"intValue\":\"200\"") != null);
}

test("OTLP Metrics and Logs JSON export") {
    const alloc = std.testing.allocator;
    const res = ResourceInfo{ .service_name = "cp-worker" };

    const pts = [
        MetricDataPoint{
            .time_unix_nano = 1700000000000000000,
            .value_int = 42,
        },
    ];
    const metrics = [
        OtlpMetric{
            .name = "requests.total",
            .unit = "1",
            .kind = MetricKind.COUNTER,
            .data_points = &pts,
        },
    ];
    const m_json = try encodeExportMetricsRequestJson(alloc, &res, &metrics);
    defer alloc.free(m_json);
    try std.testing.expect(std.mem.indexOf(u8, m_json, "\"resourceMetrics\":[") != null);
    try std.testing.expect(std.mem.indexOf(u8, m_json, "\"requests.total\"") != null);
    try std.testing.expect(std.mem.indexOf(u8, m_json, "\"asInt\":\"42\"") != null);

    const logs = [
        OtlpLogRecord{
            .time_unix_nano = 1700000000000000000,
            .severity_number = SeverityNumber.ERROR,
            .severity_text = "ERROR",
            .body = AnyValue{ .string_value = "upstream timeout" },
            .trace_id = [0xaa, 0xbb, 0xcc, 0xdd, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11],
        },
    ];
    const l_json = try encodeExportLogsRequestJson(alloc, &res, &logs);
    defer alloc.free(l_json);
    try std.testing.expect(std.mem.indexOf(u8, l_json, "\"resourceLogs\":[") != null);
    try std.testing.expect(std.mem.indexOf(u8, l_json, "\"severityNumber\":17") != null);
    try std.testing.expect(std.mem.indexOf(u8, l_json, "\"upstream timeout\"") != null);
}

test("cap.otlp dynamic mount and zero-leak allocator check") {
    const alloc = std.testing.allocator;
    const tp = try cap.otlp.parseTraceParent("00-11111111111111111111111111111111-2222222222222222-01");
    const hdr = try cap.otlp.formatTraceParent(alloc, &tp);
    defer alloc.free(hdr);
    try std.testing.expectEqualStrings("00-11111111111111111111111111111111-2222222222222222-01", hdr);
}
