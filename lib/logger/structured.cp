// Source: cp/lib/logger/structured.cp (Structured JSON & Logfmt Logger)
// lib/logger/structured.cp
// Industrial-grade zero-heap Structured Logger supporting JSON, Logfmt, PrettyAnsi,
// strict RFC 8259 string escaping, child field binding (withField), sensitive field
// redaction (password/token/authorization/secret -> "[REDACTED]"), and pluggable sinks.

const std = @import("std");
import { LogLevel } from "./logger.cp";

pub const STRUCTURED_MAX_FIELDS: usize = 32;
pub const STRUCTURED_MAX_BOUND_FIELDS: usize = 16;
pub const STRUCTURED_MAX_REDACT_KEYS: usize = 16;
pub const STRUCTURED_SINK_BUF_SIZE: usize = 8192;

pub type LogSinkFn = fn (line: []const u8, ctx: *!?) void;

fn slAppendBytes(out_buf: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out_buf.len) {
        out_buf[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn slAppendI64(out_buf: []u8, pos: usize, val: i64): usize {
    let tmp: [32]u8 = [0; 32];
    const s = std.fmt.bufPrint(tmp[0..32], "{d}", .{val}) catch "0";
    return slAppendBytes(out_buf, pos, s);
}

fn slAppendU64(out_buf: []u8, pos: usize, val: u64): usize {
    let tmp: [32]u8 = [0; 32];
    const s = std.fmt.bufPrint(tmp[0..32], "{d}", .{val}) catch "0";
    return slAppendBytes(out_buf, pos, s);
}

fn slAppendF64(out_buf: []u8, pos: usize, val: f64, for_json: bool): usize {
    if (std.math.isNan(val) || std.math.isInf(val)) {
        if (for_json) {
            return slAppendBytes(out_buf, pos, "null");
        }
        if (std.math.isNan(val)) {
            return slAppendBytes(out_buf, pos, "NaN");
        }
        return slAppendBytes(out_buf, pos, if (val < 0.0) "-Inf" else "Inf");
    }
    let tmp: [64]u8 = undefined;
    const s = std.fmt.bufPrint(tmp[0..64], "{d}", .{val}) catch "0";
    return slAppendBytes(out_buf, pos, s);
}

fn slHexLowerDigit(nibble: u8): u8 {
    if (nibble < 10) return '0' + nibble;
    return 'a' + (nibble - 10);
}

fn slAppendJsonStr(out_buf: []u8, pos: usize, s: []const u8): usize {
    let p = slAppendBytes(out_buf, pos, "\"");
    let i: usize = 0;
    while (i < s.len && p < out_buf.len) {
        const c = s[i];
        if (c == '"') {
            p = slAppendBytes(out_buf, p, "\\\"");
        } else if (c == '\\') {
            p = slAppendBytes(out_buf, p, "\\\\");
        } else if (c == '\n') {
            p = slAppendBytes(out_buf, p, "\\n");
        } else if (c == '\r') {
            p = slAppendBytes(out_buf, p, "\\r");
        } else if (c == '\t') {
            p = slAppendBytes(out_buf, p, "\\t");
        } else if (c < 0x20) {
            let esc: [6]u8 = [
                '\\',
                'u',
                '0',
                '0',
                slHexLowerDigit((c >> 4) & 0x0f),
                slHexLowerDigit(c & 0x0f),
            ];
            p = slAppendBytes(out_buf, p, esc[0..6]);
        } else {
            p = slAppendBytes(out_buf, p, s[i .. i + 1]);
        }
        i += 1;
    }
    return slAppendBytes(out_buf, p, "\"");
}

fn slNeedsLogfmtQuote(s: []const u8): bool {
    if (s.len == 0) return true;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c <= 0x20 || c == '=' || c == '"' || c == '\\' || c >= 0x7f) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn slAppendLogfmtStr(out_buf: []u8, pos: usize, s: []const u8): usize {
    if (!slNeedsLogfmtQuote(s)) {
        return slAppendBytes(out_buf, pos, s);
    }
    return slAppendJsonStr(out_buf, pos, s);
}

fn slToLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn slAsciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (slToLowerAscii(a[i]) != slToLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn slNowUnixNs(): u64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    zig {
        _ = std.os.linux.clock_gettime(.REALTIME, &ts);
    }
    if (ts.sec <= 0) return 0;
    return (ts.sec as u64) * 1000000000 + (ts.nsec as u64);
}

/// Strongly typed structured log field value (Tagged Union).
pub enum LogFieldValue {
    Str: []const u8,
    Int: i64,
    Uint: u64,
    Float: f64,
    Bool: bool,
    Null,

    static fn str(v: []const u8): LogFieldValue {
        return LogFieldValue.Str(v);
    }

    static fn int(v: i64): LogFieldValue {
        return LogFieldValue.Int(v);
    }

    static fn uint(v: u64): LogFieldValue {
        return LogFieldValue.Uint(v);
    }

    static fn float(v: f64): LogFieldValue {
        return LogFieldValue.Float(v);
    }

    static fn boolean(v: bool): LogFieldValue {
        return LogFieldValue.Bool(v);
    }

    static fn nil(): LogFieldValue {
        return LogFieldValue.Null;
    }
}

/// Key-value field attached to structured log records and spans.
pub struct LogField {
    key: []const u8 = "";
    value: LogFieldValue = LogFieldValue.Null;

    static fn init(key: []const u8, value: LogFieldValue): LogField {
        return LogField { key: key, value: value };
    }

    static fn str(key: []const u8, val: []const u8): LogField {
        return LogField { key: key, value: LogFieldValue.Str(val) };
    }

    static fn int(key: []const u8, val: i64): LogField {
        return LogField { key: key, value: LogFieldValue.Int(val) };
    }

    static fn uint(key: []const u8, val: u64): LogField {
        return LogField { key: key, value: LogFieldValue.Uint(val) };
    }

    static fn float(key: []const u8, val: f64): LogField {
        return LogField { key: key, value: LogFieldValue.Float(val) };
    }

    static fn boolean(key: []const u8, val: bool): LogField {
        return LogField { key: key, value: LogFieldValue.Bool(val) };
    }

    static fn nil(key: []const u8): LogField {
        return LogField { key: key, value: LogFieldValue.Null };
    }
}

/// Output wire/display format for structured logs.
pub enum LogFormat: u8 {
    Json = 0,
    Logfmt = 1,
    PrettyAnsi = 2,
}

/// Fixed-capacity zero-heap structured log event record.
pub struct StructuredRecord {
    timestamp_ns: u64 = 0;
    level: LogLevel = LogLevel.Info;
    message: []const u8 = "";
    logger_name: []const u8 = "";
    trace_id: []const u8 = "";
    span_id: []const u8 = "";
    fields: [STRUCTURED_MAX_FIELDS]LogField = undefined;
    field_count: usize = 0;

    static fn init(level: LogLevel, message: []const u8): StructuredRecord {
        return StructuredRecord {
            timestamp_ns: 0,
            level: level,
            message: message,
            logger_name: "",
            trace_id: "",
            span_id: "",
            fields: undefined,
            field_count: 0,
        };
    }

    static fn initNow(level: LogLevel, message: []const u8): StructuredRecord {
        return StructuredRecord {
            timestamp_ns: slNowUnixNs(),
            level: level,
            message: message,
            logger_name: "",
            trace_id: "",
            span_id: "",
            fields: undefined,
            field_count: 0,
        };
    }

    fn setTimestamp(*_, ts_ns: u64) {
        _.timestamp_ns = ts_ns;
    }

    fn setLogger(*_, name: []const u8) {
        _.logger_name = name;
    }

    fn setTrace(*_, trace_id: []const u8, span_id: []const u8) {
        _.trace_id = trace_id;
        _.span_id = span_id;
    }

    fn addField(*_, key: []const u8, value: LogFieldValue): bool {
        let i: usize = 0;
        while (i < _.field_count) {
            if (std.mem.eql(u8, _.fields[i].key, key)) {
                _.fields[i].value = value;
                return true;
            }
            i += 1;
        }
        if (_.field_count >= STRUCTURED_MAX_FIELDS) {
            return false;
        }
        _.fields[_.field_count] = LogField { key: key, value: value };
        _.field_count += 1;
        return true;
    }

    fn addStr(*_, key: []const u8, val: []const u8): bool {
        return _.addField(key, LogFieldValue.Str(val));
    }

    fn addInt(*_, key: []const u8, val: i64): bool {
        return _.addField(key, LogFieldValue.Int(val));
    }

    fn addUint(*_, key: []const u8, val: u64): bool {
        return _.addField(key, LogFieldValue.Uint(val));
    }

    fn addFloat(*_, key: []const u8, val: f64): bool {
        return _.addField(key, LogFieldValue.Float(val));
    }

    fn addBool(*_, key: []const u8, val: bool): bool {
        return _.addField(key, LogFieldValue.Bool(val));
    }

    fn getField(key: []const u8): LogFieldValue? {
        let i: usize = 0;
        while (i < _.field_count) {
            if (std.mem.eql(u8, _.fields[i].key, key)) {
                return _.fields[i].value;
            }
            i += 1;
        }
        return null;
    }
}

/// Stateless formatter rendering `StructuredRecord` to JSON, Logfmt, or PrettyAnsi with redaction.
pub static struct LogFormatter {
    static fn isDefaultRedactedKey(key: []const u8): bool {
        return slAsciiEqlIgnoreCase(key, "password")
            || slAsciiEqlIgnoreCase(key, "token")
            || slAsciiEqlIgnoreCase(key, "authorization")
            || slAsciiEqlIgnoreCase(key, "secret");
    }

    static fn shouldRedact(key: []const u8, redact_keys: []const []const u8): bool {
        if (redact_keys.len == 0) {
            return LogFormatter.isDefaultRedactedKey(key);
        }
        let i: usize = 0;
        while (i < redact_keys.len) {
            if (slAsciiEqlIgnoreCase(key, redact_keys[i])) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    static fn escapeJsonString(out_buf: []u8, pos: usize, s: []const u8): usize {
        return slAppendJsonStr(out_buf, pos, s);
    }

    static fn appendFieldValueJson(out_buf: []u8, pos: usize, val: LogFieldValue): usize {
        return switch (val) {
            .Str s => slAppendJsonStr(out_buf, pos, s),
            .Int v => slAppendI64(out_buf, pos, v),
            .Uint v => slAppendU64(out_buf, pos, v),
            .Float v => slAppendF64(out_buf, pos, v, true),
            .Bool b => slAppendBytes(out_buf, pos, if (b) "true" else "false"),
            .Null => slAppendBytes(out_buf, pos, "null"),
        };
    }

    static fn appendFieldValueLogfmt(out_buf: []u8, pos: usize, val: LogFieldValue): usize {
        return switch (val) {
            .Str s => slAppendLogfmtStr(out_buf, pos, s),
            .Int v => slAppendI64(out_buf, pos, v),
            .Uint v => slAppendU64(out_buf, pos, v),
            .Float v => slAppendF64(out_buf, pos, v, false),
            .Bool b => slAppendBytes(out_buf, pos, if (b) "true" else "false"),
            .Null => slAppendBytes(out_buf, pos, "null"),
        };
    }

    static fn formatJson(rec: StructuredRecord, out_buf: []u8, redact_keys: []const []const u8): []const u8 {
        let pos: usize = 0;
        pos = slAppendBytes(out_buf, pos, "{\"ts\":");
        pos = slAppendU64(out_buf, pos, rec.timestamp_ns);
        pos = slAppendBytes(out_buf, pos, ",\"level\":");
        pos = slAppendJsonStr(out_buf, pos, rec.level.label());
        if (rec.logger_name.len > 0) {
            pos = slAppendBytes(out_buf, pos, ",\"logger\":");
            pos = slAppendJsonStr(out_buf, pos, rec.logger_name);
        }
        if (rec.trace_id.len > 0) {
            pos = slAppendBytes(out_buf, pos, ",\"trace_id\":");
            pos = slAppendJsonStr(out_buf, pos, rec.trace_id);
        }
        if (rec.span_id.len > 0) {
            pos = slAppendBytes(out_buf, pos, ",\"span_id\":");
            pos = slAppendJsonStr(out_buf, pos, rec.span_id);
        }
        pos = slAppendBytes(out_buf, pos, ",\"msg\":");
        pos = slAppendJsonStr(out_buf, pos, rec.message);

        let i: usize = 0;
        while (i < rec.field_count && pos < out_buf.len) {
            const f = rec.fields[i];
            pos = slAppendBytes(out_buf, pos, ",");
            pos = slAppendJsonStr(out_buf, pos, f.key);
            pos = slAppendBytes(out_buf, pos, ":");
            if (LogFormatter.shouldRedact(f.key, redact_keys)) {
                pos = slAppendJsonStr(out_buf, pos, "[REDACTED]");
            } else {
                pos = LogFormatter.appendFieldValueJson(out_buf, pos, f.value);
            }
            i += 1;
        }
        pos = slAppendBytes(out_buf, pos, "}");
        return out_buf[0..pos];
    }

    static fn formatLogfmt(rec: StructuredRecord, out_buf: []u8, redact_keys: []const []const u8): []const u8 {
        let pos: usize = 0;
        pos = slAppendBytes(out_buf, pos, "ts=");
        pos = slAppendU64(out_buf, pos, rec.timestamp_ns);
        pos = slAppendBytes(out_buf, pos, " level=");
        pos = slAppendBytes(out_buf, pos, rec.level.label());
        if (rec.logger_name.len > 0) {
            pos = slAppendBytes(out_buf, pos, " logger=");
            pos = slAppendLogfmtStr(out_buf, pos, rec.logger_name);
        }
        if (rec.trace_id.len > 0) {
            pos = slAppendBytes(out_buf, pos, " trace_id=");
            pos = slAppendLogfmtStr(out_buf, pos, rec.trace_id);
        }
        if (rec.span_id.len > 0) {
            pos = slAppendBytes(out_buf, pos, " span_id=");
            pos = slAppendLogfmtStr(out_buf, pos, rec.span_id);
        }
        pos = slAppendBytes(out_buf, pos, " msg=");
        pos = slAppendLogfmtStr(out_buf, pos, rec.message);

        let i: usize = 0;
        while (i < rec.field_count && pos < out_buf.len) {
            const f = rec.fields[i];
            pos = slAppendBytes(out_buf, pos, " ");
            pos = slAppendLogfmtStr(out_buf, pos, f.key);
            pos = slAppendBytes(out_buf, pos, "=");
            if (LogFormatter.shouldRedact(f.key, redact_keys)) {
                pos = slAppendBytes(out_buf, pos, "[REDACTED]");
            } else {
                pos = LogFormatter.appendFieldValueLogfmt(out_buf, pos, f.value);
            }
            i += 1;
        }
        return out_buf[0..pos];
    }

    static fn formatPrettyAnsi(rec: StructuredRecord, out_buf: []u8, redact_keys: []const []const u8): []const u8 {
        let pos: usize = 0;
        pos = slAppendBytes(out_buf, pos, rec.level.ansiColor());
        pos = slAppendBytes(out_buf, pos, "[");
        pos = slAppendBytes(out_buf, pos, rec.level.label());
        pos = slAppendBytes(out_buf, pos, "]\x1b[0m");

        if (rec.logger_name.len > 0) {
            pos = slAppendBytes(out_buf, pos, " \x1b[1m");
            pos = slAppendBytes(out_buf, pos, rec.logger_name);
            pos = slAppendBytes(out_buf, pos, "\x1b[0m");
        }
        if (rec.trace_id.len > 0) {
            pos = slAppendBytes(out_buf, pos, " \x1b[90mtrace=");
            pos = slAppendBytes(out_buf, pos, rec.trace_id);
            if (rec.span_id.len > 0) {
                pos = slAppendBytes(out_buf, pos, "/");
                pos = slAppendBytes(out_buf, pos, rec.span_id);
            }
            pos = slAppendBytes(out_buf, pos, "\x1b[0m");
        }
        pos = slAppendBytes(out_buf, pos, " ");
        pos = slAppendBytes(out_buf, pos, rec.message);

        let i: usize = 0;
        while (i < rec.field_count && pos < out_buf.len) {
            const f = rec.fields[i];
            pos = slAppendBytes(out_buf, pos, " \x1b[90m");
            pos = slAppendBytes(out_buf, pos, f.key);
            pos = slAppendBytes(out_buf, pos, "=\x1b[0m");
            if (LogFormatter.shouldRedact(f.key, redact_keys)) {
                pos = slAppendBytes(out_buf, pos, "\x1b[33m[REDACTED]\x1b[0m");
            } else {
                pos = LogFormatter.appendFieldValueLogfmt(out_buf, pos, f.value);
            }
            i += 1;
        }
        return out_buf[0..pos];
    }

    static fn format(rec: StructuredRecord, fmt: LogFormat, out_buf: []u8, redact_keys: []const []const u8): []const u8 {
        return switch (fmt) {
            .Json => LogFormatter.formatJson(rec, out_buf, redact_keys),
            .Logfmt => LogFormatter.formatLogfmt(rec, out_buf, redact_keys),
            .PrettyAnsi => LogFormatter.formatPrettyAnsi(rec, out_buf, redact_keys),
        };
    }
}

/// Zero-heap structured logger with level filtering, bound child context, redaction, and sink support.
pub struct StructuredLogger {
    min_level: LogLevel = LogLevel.Info;
    format: LogFormat = LogFormat.Json;
    name: []const u8 = "";
    trace_id: []const u8 = "";
    span_id: []const u8 = "";
    bound_fields: [STRUCTURED_MAX_BOUND_FIELDS]LogField = undefined;
    bound_count: usize = 0;
    redact_keys: [STRUCTURED_MAX_REDACT_KEYS][]const u8 = undefined;
    redact_count: usize = 0;
    sink_buf: [STRUCTURED_SINK_BUF_SIZE]u8 = undefined;
    sink_len: usize = 0;
    sink_fn: LogSinkFn? = null;
    sink_ctx: *!? = null;
    emitted_count: u64 = 0;
    dropped_count: u64 = 0;
    fixed_ts_ns: u64 = 0;
    use_fixed_ts: bool = false;

    static fn init(min_level: LogLevel, format: LogFormat): StructuredLogger {
        let logger = StructuredLogger {
            min_level: min_level,
            format: format,
            name: "",
            trace_id: "",
            span_id: "",
            bound_fields: undefined,
            bound_count: 0,
            redact_keys: undefined,
            redact_count: 4,
            sink_buf: undefined,
            sink_len: 0,
            sink_fn: null,
            sink_ctx: null,
            emitted_count: 0,
            dropped_count: 0,
            fixed_ts_ns: 0,
            use_fixed_ts: false,
        };
        logger.redact_keys[0] = "password";
        logger.redact_keys[1] = "token";
        logger.redact_keys[2] = "authorization";
        logger.redact_keys[3] = "secret";
        return logger;
    }

    static fn initNamed(name: []const u8, min_level: LogLevel, format: LogFormat): StructuredLogger {
        let logger = StructuredLogger.init(min_level, format);
        logger.name = name;
        return logger;
    }

    fn setMinLevel(*_, level: LogLevel) {
        _.min_level = level;
    }

    fn setFormat(*_, fmt: LogFormat) {
        _.format = fmt;
    }

    fn setFixedTimestamp(*_, ts_ns: u64) {
        _.fixed_ts_ns = ts_ns;
        _.use_fixed_ts = true;
    }

    fn setSink(*_, cb: LogSinkFn, ctx: *!?) {
        _.sink_fn = cb;
        _.sink_ctx = ctx;
    }

    fn clearSink(*_) {
        _.sink_fn = null;
        _.sink_ctx = null;
    }

    fn addRedactKey(*_, key: []const u8): bool {
        let i: usize = 0;
        while (i < _.redact_count) {
            if (slAsciiEqlIgnoreCase(_.redact_keys[i], key)) {
                return true;
            }
            i += 1;
        }
        if (_.redact_count >= STRUCTURED_MAX_REDACT_KEYS) {
            return false;
        }
        _.redact_keys[_.redact_count] = key;
        _.redact_count += 1;
        return true;
    }

    fn redactKeys(): []const []const u8 {
        return _.redact_keys[0.._.redact_count];
    }

    fn cloneConfig(): StructuredLogger {
        let child = StructuredLogger {
            min_level: _.min_level,
            format: _.format,
            name: _.name,
            trace_id: _.trace_id,
            span_id: _.span_id,
            bound_fields: _.bound_fields,
            bound_count: _.bound_count,
            redact_keys: _.redact_keys,
            redact_count: _.redact_count,
            sink_buf: undefined,
            sink_len: 0,
            sink_fn: _.sink_fn,
            sink_ctx: _.sink_ctx,
            emitted_count: 0,
            dropped_count: 0,
            fixed_ts_ns: _.fixed_ts_ns,
            use_fixed_ts: _.use_fixed_ts,
        };
        return child;
    }

    fn withField(key: []const u8, value: LogFieldValue): StructuredLogger {
        let child = _.cloneConfig();
        let i: usize = 0;
        while (i < child.bound_count) {
            if (std.mem.eql(u8, child.bound_fields[i].key, key)) {
                child.bound_fields[i].value = value;
                return child;
            }
            i += 1;
        }
        if (child.bound_count < STRUCTURED_MAX_BOUND_FIELDS) {
            child.bound_fields[child.bound_count] = LogField { key: key, value: value };
            child.bound_count += 1;
        }
        return child;
    }

    fn withStr(key: []const u8, val: []const u8): StructuredLogger {
        return _.withField(key, LogFieldValue.Str(val));
    }

    fn withInt(key: []const u8, val: i64): StructuredLogger {
        return _.withField(key, LogFieldValue.Int(val));
    }

    fn withUint(key: []const u8, val: u64): StructuredLogger {
        return _.withField(key, LogFieldValue.Uint(val));
    }

    fn withBool(key: []const u8, val: bool): StructuredLogger {
        return _.withField(key, LogFieldValue.Bool(val));
    }

    fn withTrace(trace_id: []const u8, span_id: []const u8): StructuredLogger {
        let child = _.cloneConfig();
        child.trace_id = trace_id;
        child.span_id = span_id;
        return child;
    }

    fn buildRecord(level: LogLevel, msg: []const u8, fields: []const LogField): StructuredRecord {
        let rec = StructuredRecord.init(level, msg);
        rec.timestamp_ns = if (_.use_fixed_ts) _.fixed_ts_ns else slNowUnixNs();
        rec.logger_name = _.name;
        rec.trace_id = _.trace_id;
        rec.span_id = _.span_id;

        let bi: usize = 0;
        while (bi < _.bound_count) {
            * = rec.addField(_.bound_fields[bi].key, _.bound_fields[bi].value);
            bi += 1;
        }
        let fi: usize = 0;
        while (fi < fields.len) {
            * = rec.addField(fields[fi].key, fields[fi].value);
            fi += 1;
        }
        return rec;
    }

    fn log(*_, level: LogLevel, msg: []const u8, fields: []const LogField): bool {
        if (!level.shouldEmit(_.min_level)) {
            _.dropped_count += 1;
            return false;
        }
        const rec = _.buildRecord(level, msg, fields);
        let line_buf: [2048]u8 = undefined;
        const line = LogFormatter.format(rec, _.format, line_buf[0..2048], _.redact_keys[0.._.redact_count]);

        if (cb := _.sink_fn) {
            cb(line, _.sink_ctx);
        }

        if (_.sink_len + line.len + 1 <= STRUCTURED_SINK_BUF_SIZE) {
            _.sink_len = slAppendBytes(_.sink_buf[0..STRUCTURED_SINK_BUF_SIZE], _.sink_len, line);
            _.sink_len = slAppendBytes(_.sink_buf[0..STRUCTURED_SINK_BUF_SIZE], _.sink_len, "\n");
        }
        _.emitted_count += 1;
        return true;
    }

    fn verbose(*_, msg: []const u8, fields: []const LogField): bool {
        return _.log(LogLevel.Verbose, msg, fields);
    }

    fn debug(*_, msg: []const u8, fields: []const LogField): bool {
        return _.log(LogLevel.Debug, msg, fields);
    }

    fn info(*_, msg: []const u8, fields: []const LogField): bool {
        return _.log(LogLevel.Info, msg, fields);
    }

    fn warn(*_, msg: []const u8, fields: []const LogField): bool {
        return _.log(LogLevel.Warn, msg, fields);
    }

    fn err(*_, msg: []const u8, fields: []const LogField): bool {
        return _.log(LogLevel.Err, msg, fields);
    }

    fn captured(): []const u8 {
        return _.sink_buf[0.._.sink_len];
    }

    fn clearBuffer(*_) {
        _.sink_len = 0;
    }
}
