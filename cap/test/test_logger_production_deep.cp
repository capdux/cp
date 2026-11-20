// Source: cp/cap/test/test_logger_production_deep.cp (Production Structured Logger, W3C Trace Context & Sampler Deep Tests)
// cap/test/test_logger_production_deep.cp
// Deep verification for upgraded cap:logger suite:
//   1. StructuredLogger & LogFormatter (JSON/Logfmt/PrettyAnsi, RFC 8259 escapes, withField, sinks, redaction)
//   2. W3C TraceParent & TraceState (strict parsing, all-zero rejection, childSpan, move-to-front tracestate)
//   3. OTel-style Span lifecycle, attributes, events, duration & JSON export
//   4. RatioSampler, BurstSampler & RingLogBuffer<CAP> deterministic behavior
//   5. Backward-compatible cap:logger diagnostics (LineIndex, DiagMsg, Log) alongside new exports

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    LogLevel,
    Loc,
    Range,
    Source,
    DiagNote,
    DiagMsg,
    Log,
    LogFieldValue,
    LogField,
    LogFormat,
    StructuredRecord,
    LogFormatter,
    StructuredLogger,
    TraceParent,
    TraceState,
    SpanStatus,
    SpanEvent,
    Span,
    RatioSampler,
    BurstSampler,
    RingLogBuffer,
} from "cap:logger";

fn containsStr(haystack: []const u8, needle: []const u8): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

struct SinkCaptureState {
    calls: usize = 0;
    last_len: usize = 0;
}

fn testSinkCallback(line: []const u8, ctx: *!?) {
    if (raw := ctx) {
        unsafe {
            let st = raw as *SinkCaptureState!;
            st.calls += 1;
            st.last_len = line.len;
        }
    }
}

fn testStructuredDeep()! {
    // 1.1 Strict JSON escaping including control characters (\u00XX)
    let esc_buf: [256]u8 = [0; 256];
    const ctrl_str: [6]u8 = ['a', '"', '\\', '\n', 0x01, 'z'];
    const esc_pos = LogFormatter.escapeJsonString(esc_buf[0..256], 0, ctrl_str[0..6]);
    const escaped = esc_buf[0..esc_pos];
    expect_eq(std.mem.eql(u8, escaped, "\"a\\\"\\\\\\n\\u0001z\""), true);

    // 1.2 StructuredLogger JSON + Redaction + withField + Level filter
    let root = StructuredLogger.initNamed("api-gateway", LogLevel.Info, LogFormat.Json);
    root.setFixedTimestamp(1700000000123456789);
    * = root.addRedactKey("x_api_key");

    let req_logger = root
        .withStr("service", "auth")
        .withInt("attempt", 1)
        .withTrace("4bf92f3577b34da6a3ce929d0e0e4736", "00f067aa0ba902b7");

    // Filtered out (Debug < Info)
    const debug_ok = req_logger.debug("ignored debug", []);
    expect_eq(debug_ok, false);
    expect_eq(req_logger.dropped_count, 1);

    let cap_state = SinkCaptureState { calls: 0, last_len: 0 };
    unsafe {
        req_logger.setSink(testSinkCallback, (*cap_state as *!));
    }

    const fields: [6]LogField = [
        LogField.str("password", "super-secret-123"),
        LogField.str("Authorization", "Bearer jwt-xyz"),
        LogField.str("token", "tok_live_999"),
        LogField.str("secret", "s3cr3t"),
        LogField.str("X_Api_Key", "key_abc"),
        LogField.boolean("ok", true),
    ];
    const info_ok = req_logger.info("login \"handled\"\tok", fields[0..6]);
    expect_eq(info_ok, true);
    expect_eq(req_logger.emitted_count, 1);
    expect_eq(cap_state.calls, 1);
    expect_eq(cap_state.last_len > 20, true);

    const json_out = req_logger.captured();
    expect_eq(containsStr(json_out, "\"ts\":1700000000123456789"), true);
    expect_eq(containsStr(json_out, "\"level\":\"info\""), true);
    expect_eq(containsStr(json_out, "\"logger\":\"api-gateway\""), true);
    expect_eq(containsStr(json_out, "\"trace_id\":\"4bf92f3577b34da6a3ce929d0e0e4736\""), true);
    expect_eq(containsStr(json_out, "\"msg\":\"login \\\"handled\\\"\\tok\""), true);
    expect_eq(containsStr(json_out, "\"service\":\"auth\""), true);
    expect_eq(containsStr(json_out, "\"password\":\"[REDACTED]\""), true);
    expect_eq(containsStr(json_out, "\"Authorization\":\"[REDACTED]\""), true);
    expect_eq(containsStr(json_out, "\"token\":\"[REDACTED]\""), true);
    expect_eq(containsStr(json_out, "\"secret\":\"[REDACTED]\""), true);
    expect_eq(containsStr(json_out, "\"X_Api_Key\":\"[REDACTED]\""), true);
    expect_eq(containsStr(json_out, "super-secret-123"), false);
    expect_eq(containsStr(json_out, "Bearer jwt-xyz"), false);
    expect_eq(containsStr(json_out, "\"ok\":true"), true);

    // 1.3 Logfmt & PrettyAnsi formatting
    let rec = StructuredRecord.init(LogLevel.Warn, "slow query");
    rec.setTimestamp(42);
    * = rec.addStr("sql", "select * from users");
    * = rec.addInt("ms", 125);
    * = rec.addStr("token", "raw_tok");
    * = rec.addField("extra", LogFieldValue.nil());

    let fmt_buf: [512]u8 = [0; 512];
    const logfmt_out = LogFormatter.format(rec, LogFormat.Logfmt, fmt_buf[0..512], []);
    expect_eq(containsStr(logfmt_out, "ts=42 level=warn msg=\"slow query\""), true);
    expect_eq(containsStr(logfmt_out, "sql=\"select * from users\""), true);
    expect_eq(containsStr(logfmt_out, "ms=125"), true);
    expect_eq(containsStr(logfmt_out, "token=[REDACTED]"), true);
    expect_eq(containsStr(logfmt_out, "extra=null"), true);
    expect_eq(containsStr(logfmt_out, "raw_tok"), false);

    let ansi_buf: [512]u8 = [0; 512];
    const ansi_out = LogFormatter.format(rec, LogFormat.PrettyAnsi, ansi_buf[0..512], []);
    expect_eq(containsStr(ansi_out, "\x1b["), true);
    expect_eq(containsStr(ansi_out, "[warn]"), true);
    expect_eq(containsStr(ansi_out, "[REDACTED]"), true);
}

fn testTraceContextDeep()! {
    const valid_header = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    const tp_opt = TraceParent.parse(valid_header);
    expect_eq(tp_opt != null, true);
    const tp = tp_opt.?;
    expect_eq(tp.version, 0);
    expect_eq(tp.flags, 1);
    expect_eq(tp.isSampled(), true);

    let out_tp: [64]u8 = [0; 64];
    const formatted = tp.format(out_tp[0..64]);
    expect_eq(std.mem.eql(u8, formatted, valid_header), true);

    // Derive child span preserving trace_id and flags
    const child = tp.childSpan("b7ad6b7169203331").?;
    let out_child: [64]u8 = [0; 64];
    expect_eq(
        std.mem.eql(
            u8,
            child.format(out_child[0..64]),
            "00-4bf92f3577b34da6a3ce929d0e0e4736-b7ad6b7169203331-01",
        ),
        true,
    );
    expect_eq(child.childSpan("0000000000000000") == null, true);

    // Strict rejections per W3C Trace Context Level 1
    expect_eq(TraceParent.parse("ff-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01") == null, true);
    expect_eq(TraceParent.parse("01-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01") == null, true);
    expect_eq(TraceParent.parse("00-00000000000000000000000000000000-00f067aa0ba902b7-01") == null, true);
    expect_eq(TraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-0000000000000000-01") == null, true);
    expect_eq(TraceParent.parse("00-4BF92F3577B34DA6A3CE929D0E0E4736-00f067aa0ba902b7-01") == null, true);
    expect_eq(TraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01-extra") == null, true);
    expect_eq(TraceParent.parse("00_4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01") == null, true);

    // TraceState parsing, query, move-to-front update & format
    let ts = TraceState.parse("rojo=00f067aa0ba902b7, congo=t61rcWkgMzE, tenant1@vendor=abc").?;
    expect_eq(ts.len(), 3);
    expect_eq(std.mem.eql(u8, ts.get("congo").?, "t61rcWkgMzE"), true);
    expect_eq(ts.get("missing") == null, true);

    // Update existing entry -> moves to leftmost position
    expect_eq(ts.set("congo", "updated99"), true);
    expect_eq(ts.set("cp", "v1"), true);
    let ts_buf: [256]u8 = [0; 256];
    expect_eq(
        std.mem.eql(
            u8,
            ts.format(ts_buf[0..256]),
            "cp=v1,congo=updated99,rojo=00f067aa0ba902b7,tenant1@vendor=abc",
        ),
        true,
    );
    expect_eq(ts.remove("rojo"), true);
    expect_eq(ts.len(), 3);

    // Invalid tracestate rejections
    expect_eq(TraceState.parse("InvalidKey=123") == null, true);
    expect_eq(TraceState.parse("missing_equals") == null, true);
}

fn testSpanDeep()! {
    const root_tp = TraceParent.init(
        "0af7651916cd43dd8448eb211c80319c",
        "b7ad6b7169203331",
        true,
    ).?;
    let root_span = Span.startAt("http.server.request", root_tp, 1000000);
    root_span.setServiceName("checkout-svc");
    * = root_span.setAttrStr("http.method", "POST");
    * = root_span.setAttrInt("http.status_code", 200);
    * = root_span.setAttrBool("http.tls", true);
    * = root_span.addEventAt("cache.miss", 1000500, [
        LogField.str("cache.key", "cart:42"),
    ]);
    root_span.setStatus(SpanStatus.Ok, "");
    root_span.endAt(1002500);

    expect_eq(root_span.durationNs(), 2500);
    expect_eq(root_span.ended, true);

    let child_span = Span.startChildAt("db.query", root_span, "e457b5a2e4d86bd1", 1000600).?;
    child_span.setStatus(SpanStatus.Error, "deadlock detected");
    child_span.endAt(1001800);
    expect_eq(child_span.has_parent, true);
    expect_eq(child_span.durationNs(), 1200);

    let json_buf: [2048]u8 = [0; 2048];
    const root_json = root_span.toJson(json_buf[0..2048]);
    expect_eq(containsStr(root_json, "\"traceId\":\"0af7651916cd43dd8448eb211c80319c\""), true);
    expect_eq(containsStr(root_json, "\"spanId\":\"b7ad6b7169203331\""), true);
    expect_eq(containsStr(root_json, "\"name\":\"http.server.request\""), true);
    expect_eq(containsStr(root_json, "\"serviceName\":\"checkout-svc\""), true);
    expect_eq(containsStr(root_json, "\"durationNs\":2500"), true);
    expect_eq(containsStr(root_json, "\"status\":{\"code\":\"OK\"}"), true);
    expect_eq(containsStr(root_json, "\"http.method\":\"POST\""), true);
    expect_eq(containsStr(root_json, "\"http.status_code\":200"), true);
    expect_eq(containsStr(root_json, "\"events\":[{\"name\":\"cache.miss\""), true);

    let child_buf: [1024]u8 = [0; 1024];
    const child_json = child_span.toJson(child_buf[0..1024]);
    expect_eq(containsStr(child_json, "\"parentSpanId\":\"b7ad6b7169203331\""), true);
    expect_eq(containsStr(child_json, "\"code\":\"ERROR\",\"message\":\"deadlock detected\""), true);
}

fn testSamplerRingDeep()! {
    // 4.1 RatioSampler boundary & determinism
    const never = RatioSampler.init(0.0);
    const always = RatioSampler.init(1.0);
    expect_eq(never.shouldSample("trace-abc"), false);
    expect_eq(always.shouldSample("trace-abc"), true);

    const half = RatioSampler.initWithSeed(0.5, 42);
    const first_decision = half.shouldSample("0af7651916cd43dd8448eb211c80319c");
    expect_eq(half.shouldSample("0af7651916cd43dd8448eb211c80319c"), first_decision);

    let sampled_hits: usize = 0;
    let k: usize = 0;
    while (k < 200) {
        let key_buf: [32]u8 = [0; 32];
        const key = std.fmt.bufPrint(key_buf[0..32], "req-{d}", .{k}) catch "req";
        if (half.shouldSample(key)) {
            sampled_hits += 1;
        }
        k += 1;
    }
    expect_eq(sampled_hits > 60 && sampled_hits < 140, true);

    // 4.2 BurstSampler: allow first 3 in window, then 1 out of every 4
    let burst = BurstSampler.init(3, 4, 1000000);
    let t1_accepted: usize = 0;
    let i: usize = 0;
    while (i < 11) {
        if (burst.shouldSampleAt(1000 + (i as u64) * 100)) {
            t1_accepted += 1;
        }
        i += 1;
    }
    expect_eq(t1_accepted, 5);
    expect_eq(burst.acceptedCount(), 5);
    expect_eq(burst.droppedCount(), 6);

    // Window 2 at t=2000000: resets burst allowance
    expect_eq(burst.shouldSampleAt(2000000), true);
    expect_eq(burst.shouldSampleAt(2000001), true);
    expect_eq(burst.shouldSampleAt(2000002), true);
    expect_eq(burst.shouldSampleAt(2000003), false);

    // 4.3 RingLogBuffer<3>: zero-leak overwrite & chronological dump
    let ring = RingLogBuffer<3>.init();
    expect_eq(ring.capacity(), 3);
    expect_eq(ring.len(), 0);

    let r1 = StructuredRecord.init(LogLevel.Info, "m1");
    let r2 = StructuredRecord.init(LogLevel.Warn, "m2");
    let r3 = StructuredRecord.init(LogLevel.Err, "m3");
    let r4 = StructuredRecord.init(LogLevel.Info, "m4");
    * = r4.addStr("secret", "hidden");

    ring.push(r1);
    ring.push(r2);
    ring.push(r3);
    ring.push(r4);

    expect_eq(ring.isFull(), true);
    expect_eq(ring.len(), 3);
    expect_eq(ring.totalPushed(), 4);
    expect_eq(ring.overwrittenCount(), 1);
    expect_eq(std.mem.eql(u8, ring.get(0).?.message, "m2"), true);
    expect_eq(std.mem.eql(u8, ring.get(1).?.message, "m3"), true);
    expect_eq(std.mem.eql(u8, ring.latest().?.message, "m4"), true);

    let dump_buf: [1024]u8 = [0; 1024];
    const json_dump = ring.dumpJson(dump_buf[0..1024], []);
    expect_eq(containsStr(json_dump, "\"msg\":\"m1\""), false);
    expect_eq(containsStr(json_dump, "\"msg\":\"m2\""), true);
    expect_eq(containsStr(json_dump, "\"msg\":\"m4\""), true);
    expect_eq(containsStr(json_dump, "\"secret\":\"[REDACTED]\""), true);

    let logfmt_dump_buf: [1024]u8 = [0; 1024];
    const logfmt_dump = ring.dumpLogfmt(logfmt_dump_buf[0..1024], []);
    expect_eq(containsStr(logfmt_dump, "msg=m2"), true);
    expect_eq(containsStr(logfmt_dump, "secret=[REDACTED]"), true);
}

fn testLoggerCompatDeep()! {
    const code = "let a = 1;\nlet a = 2;\n";
    const src = Source.initIndexed("app.cp", code);
    let msg = DiagMsg.fromSource(LogLevel.Err, src, Range.init(15, 1), "duplicate 'a'", "");
    * = msg.addNoteFromSource(src, Range.init(4, 1), "first defined here");

    let diag_log = Log.initWithMaxErrors(LogLevel.Info, 1);
    diag_log.addMsg(msg);
    diag_log.addError(src, Loc.init(11), "second error suppressed");
    expect_eq(diag_log.isErrorLimitReached(), true);
    expect_eq(diag_log.suppressed_errors, 1);

    let out_buf: [1024]u8 = [0; 1024];
    const rendered = diag_log.formatAll(out_buf[0..1024], false);
    expect_eq(containsStr(rendered, "duplicate 'a'"), true);
    expect_eq(containsStr(rendered, "first defined here"), true);
}

fn main() {
    test("1. cap:logger Structured — JSON/Logfmt/Pretty Escaping, withField, Sinks & Redaction", () => {
        testStructuredDeep()?;
    });
    test("2. cap:logger Trace — W3C TraceParent Strict Validation, childSpan & TraceState", () => {
        testTraceContextDeep()?;
    });
    test("3. cap:logger Span — Lifecycle, Attributes, Events & OTel JSON Export", () => {
        testSpanDeep()?;
    });
    test("4. cap:logger Sampler — RatioSampler, BurstSampler & RingLogBuffer", () => {
        testSamplerRingDeep()?;
    });
    test("5. cap:logger Compatibility — Existing Diagnostic Log, Source, LineIndex & max_errors", () => {
        testLoggerCompatDeep()?;
    });
}
