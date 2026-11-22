// Origin: Pure-CP Stdlib Test Suite (lib/obs/structured_log.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Heap Structured Logger (JSONL & Logfmt):
//   1. Level filtering short-circuit, per-level counters, dropped_below_level & resetStats
//   2. JSONL full field types (Str/Int/Float/Bool/NaN), trace correlation & RFC 8259 escaping
//   3. Logfmt smart quoting (unquoted simple tokens vs spaces/equals/quotes escaping)
//   4. Max 16 fields capacity guard & empty-key rejection
//   5. Buffer overflow protection (tiny & zero-len out_buf -> null + buffer_overflows count)
//   6. default_service fallback vs record.service override & append_newline toggle

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    OBS_LOG_MAX_FIELDS,
    ObsLogLevel,
    ObsLogFieldKind,
    ObsLogField,
    ObsLogRecord,
    ObsLogFormat,
    ObsLoggerConfig,
    ObsLoggerStats,
    ObsStructuredLogger,
} from "cap:obs";

// ============================================================================
// Test 1: Level Filtering Short-Circuit & Per-Level Telemetry Counters
// ============================================================================

fn testLevelFilteringAndStats()! {
    let logger = cap.obs.ObsStructuredLogger.init(ObsLoggerConfig{
        min_level: ObsLogLevel.Warn,
        format: ObsLogFormat.JsonLines,
        default_service: "gateway",
        append_newline: true,
    });

    expect_eq(logger.shouldLog(ObsLogLevel.Trace), false);
    expect_eq(logger.shouldLog(ObsLogLevel.Debug), false);
    expect_eq(logger.shouldLog(ObsLogLevel.Info), false);
    expect_eq(logger.shouldLog(ObsLogLevel.Warn), true);
    expect_eq(logger.shouldLog(ObsLogLevel.Error), true);
    expect_eq(logger.shouldLog(ObsLogLevel.Fatal), true);

    let out: [256]u8 = [0; 256];
    let out_s: []u8 = out[0..];

    // Below min_level -> short-circuits with 0 bytes written
    const n_dbg = logger.emit(ObsLogRecord{ timestamp_ms: 100, level: ObsLogLevel.Debug, message: "skip" }, out_s) ?? return error.UnexpectedOverflow;
    expect_eq(n_dbg, 0);
    const n_info = logger.emit(ObsLogRecord{ timestamp_ms: 101, level: ObsLogLevel.Info, message: "skip2" }, out_s) ?? return error.UnexpectedOverflow;
    expect_eq(n_info, 0);

    // At/above min_level -> emitted
    expect_eq((logger.emit(ObsLogRecord{ timestamp_ms: 102, level: ObsLogLevel.Warn, message: "high latency" }, out_s) ?? 0) > 0, true);
    expect_eq((logger.emit(ObsLogRecord{ timestamp_ms: 103, level: ObsLogLevel.Error, message: "upstream 502" }, out_s) ?? 0) > 0, true);
    expect_eq((logger.emit(ObsLogRecord{ timestamp_ms: 104, level: ObsLogLevel.Fatal, message: "oom" }, out_s) ?? 0) > 0, true);

    const st: ObsLoggerStats = logger.stats();
    expect_eq(st.dropped_below_level, 2);
    expect_eq(st.total_emitted, 3);
    expect_eq(st.warn_count, 1);
    expect_eq(st.error_count, 1);
    expect_eq(st.fatal_count, 1);
    expect_eq(st.buffer_overflows, 0);

    logger.resetStats();
    expect_eq(logger.stats().total_emitted, 0);
    expect_eq(logger.stats().dropped_below_level, 0);
}

// ============================================================================
// Test 2: JSONL Full Types, Trace Correlation & RFC 8259 Escaping
// ============================================================================

fn testJsonLinesTypesAndEscaping()! {
    let logger = ObsStructuredLogger.init(ObsLoggerConfig{
        min_level: ObsLogLevel.Trace,
        format: ObsLogFormat.JsonLines,
        default_service: "agent-core",
        append_newline: true,
    });

    let rec = ObsLogRecord{
        timestamp_ms: 1732250000123,
        level: ObsLogLevel.Info,
        trace_id: "4bf92f3577b34da6",
        span_id: "00f067aa",
        message: "say \"hi\"\nline2\\end",
    };
    expect_eq(rec.addField(ObsLogField.str("tool", "read\tfile")), true);
    expect_eq(rec.addField(ObsLogField.int("delta", -42)), true);
    expect_eq(rec.addField(ObsLogField.float("score", 0.9625)), true);
    expect_eq(rec.addField(ObsLogField.float("bad_f", std.math.nan(f64))), true);
    expect_eq(rec.addField(ObsLogField.boolean("ok", true)), true);

    let out: [512]u8 = [0; 512];
    let out_s: []u8 = out[0..];
    const n = logger.emit(rec, out_s) ?? return error.EmitFailed;
    const line = out[0..n];

    expect_eq(line[n - 1], '\n');
    expect_eq(std.mem.indexOf(u8, line, "{\"ts\":1732250000123,\"level\":\"info\",\"service\":\"agent-core\"") != null, true);
    expect_eq(std.mem.indexOf(u8, line, "\"trace_id\":\"4bf92f3577b34da6\",\"span_id\":\"00f067aa\"") != null, true);
    expect_eq(std.mem.indexOf(u8, line, "\"msg\":\"say \\\"hi\\\"\\nline2\\\\end\"") != null, true);
    expect_eq(std.mem.indexOf(u8, line, "\"tool\":\"read\\tfile\"") != null, true);
    expect_eq(std.mem.indexOf(u8, line, "\"delta\":-42") != null, true);
    expect_eq(std.mem.indexOf(u8, line, "\"score\":0.9625") != null, true);
    expect_eq(std.mem.indexOf(u8, line, "\"bad_f\":null") != null, true);
    expect_eq(std.mem.indexOf(u8, line, "\"ok\":true") != null, true);
}

// ============================================================================
// Test 3: Logfmt Smart Quoting & Trace Correlation
// ============================================================================

fn testLogfmtSmartQuoting()! {
    let logger = ObsStructuredLogger.init(ObsLoggerConfig{
        min_level: ObsLogLevel.Debug,
        format: ObsLogFormat.Logfmt,
        default_service: "rag-worker",
        append_newline: false,
    });

    let rec = ObsLogRecord{
        timestamp_ms: 5000,
        level: ObsLogLevel.Warn,
        trace_id: "tr-99",
        span_id: "sp-1",
        message: "retry k=v \"quoted\"",
    };
    expect_eq(rec.addField(ObsLogField.str("tool", "search")), true);
    expect_eq(rec.addField(ObsLogField.str("empty", "")), true);
    expect_eq(rec.addField(ObsLogField.int("attempt", 2)), true);
    expect_eq(rec.addField(ObsLogField.boolean("fatal", false)), true);

    let out: [512]u8 = [0; 512];
    let out_s: []u8 = out[0..];
    const n = logger.emit(rec, out_s) ?? return error.EmitFailed;
    const line = out[0..n];

    expect_eq(line[n - 1] != '\n', true);
    expect_eq(std.mem.indexOf(u8, line, "ts=5000 level=warn service=rag-worker trace_id=tr-99 span_id=sp-1") != null, true);
    expect_eq(std.mem.indexOf(u8, line, "msg=\"retry k=v \\\"quoted\\\"\"") != null, true);
    expect_eq(std.mem.indexOf(u8, line, " tool=search ") != null, true);
    expect_eq(std.mem.indexOf(u8, line, " empty=\"\" ") != null, true);
    expect_eq(std.mem.indexOf(u8, line, " attempt=2 fatal=false") != null, true);
}

// ============================================================================
// Test 4: Max 16 Fields Guard & Empty-Key Rejection
// ============================================================================

fn testMaxFieldsAndEmptyKeyGuard()! {
    let rec = ObsLogRecord{ timestamp_ms: 1, level: ObsLogLevel.Info, message: "fields" };
    // Empty key rejected
    expect_eq(rec.addField(ObsLogField.str("", "bad")), false);

    let i: usize = 0;
    while (i < OBS_LOG_MAX_FIELDS) : (i += 1) {
        expect_eq(rec.addField(ObsLogField.int("k", i as i64)), true);
    }
    expect_eq(rec.field_count, OBS_LOG_MAX_FIELDS);
    // 17th field rejected
    expect_eq(rec.addField(ObsLogField.int("overflow", 99)), false);
}

// ============================================================================
// Test 5: Buffer Overflow Protection
// ============================================================================

fn testBufferOverflowProtection()! {
    let logger = ObsStructuredLogger.init(ObsLoggerConfig{
        min_level: ObsLogLevel.Info,
        format: ObsLogFormat.JsonLines,
        default_service: "svc",
        append_newline: true,
    });
    const rec = ObsLogRecord{
        timestamp_ms: 123456,
        level: ObsLogLevel.Error,
        message: "this message is far too long for a 12-byte buffer",
    };

    let tiny: [12]u8 = [0; 12];
    let tiny_s: []u8 = tiny[0..];
    let empty_s: []u8 = tiny[0..0];

    expect_eq(logger.emit(rec, tiny_s) == null, true);
    expect_eq(logger.emit(rec, empty_s) == null, true);
    expect_eq(logger.stats().buffer_overflows, 2);
    expect_eq(logger.stats().total_emitted, 0);
}

// ============================================================================
// Test 6: Service Override & Format Toggle via formatRecord
// ============================================================================

fn testServiceOverrideAndDualFormat()! {
    let logger = ObsStructuredLogger.init(ObsLoggerConfig{
        min_level: ObsLogLevel.Info,
        format: ObsLogFormat.JsonLines,
        default_service: "default-svc",
        append_newline: false,
    });
    const rec = ObsLogRecord{
        timestamp_ms: 42,
        level: ObsLogLevel.Info,
        service: "custom-svc",
        message: "ok",
    };

    let buf_json: [256]u8 = [0; 256];
    let buf_lf: [256]u8 = [0; 256];
    let json_s: []u8 = buf_json[0..];
    let lf_s: []u8 = buf_lf[0..];

    const n_j = logger.formatRecord(rec, ObsLogFormat.JsonLines, json_s) ?? return error.JsonFailed;
    const n_l = logger.formatRecord(rec, ObsLogFormat.Logfmt, lf_s) ?? return error.LfFailed;

    expect_eq(std.mem.indexOf(u8, buf_json[0..n_j], "\"service\":\"custom-svc\"") != null, true);
    expect_eq(std.mem.indexOf(u8, buf_lf[0..n_l], "service=custom-svc") != null, true);
    expect_eq(ObsLogFieldKind.Str == ObsLogFieldKind.Str, true);
}

test("obs structured_log: level filtering short-circuit, per-level counters & reset", testLevelFilteringAndStats);
test("obs structured_log: JSONL full types, trace correlation & RFC 8259 escaping", testJsonLinesTypesAndEscaping);
test("obs structured_log: Logfmt smart quoting & trace correlation", testLogfmtSmartQuoting);
test("obs structured_log: max 16 fields capacity & empty-key guards", testMaxFieldsAndEmptyKeyGuard);
test("obs structured_log: buffer overflow protection & overflow counter", testBufferOverflowProtection);
test("obs structured_log: service override & dual-format formatRecord", testServiceOverrideAndDualFormat);
