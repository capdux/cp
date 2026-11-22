// Origin: Pure-CP Stdlib (Zero-Heap Structured Logger with Trace Correlation & JSONL/Logfmt Renderer) | Created: 2026-11-22
// Pure CP zero-heap-allocation structured observability logging facade:
//   - Level filtering short-circuit (`Trace | Debug | Info | Warn | Error | Fatal`)
//   - Built-in trace correlation (`service`, `trace_id`, `span_id`) + up to 16 typed KV fields (`Str | Int | Float | Bool`)
//   - Dual-format zero-alloc rendering: RFC 8259 JSON Lines (`JsonLines`) & smart-quoted `Logfmt`
//   - Per-level telemetry counters, drop tracking & strict buffer overflow protection

const std = @import("std");

pub const OBS_LOG_MAX_FIELDS: usize = 16;

/// Log severity level ordered from `Trace` (lowest) to `Fatal` (highest).
pub enum ObsLogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
}

fn levelRank(lvl: ObsLogLevel): u8 {
    match (lvl) {
        ObsLogLevel.Trace => return 0,
        ObsLogLevel.Debug => return 1,
        ObsLogLevel.Info => return 2,
        ObsLogLevel.Warn => return 3,
        ObsLogLevel.Error => return 4,
        ObsLogLevel.Fatal => return 5,
    }
}

fn levelLabel(lvl: ObsLogLevel): []const u8 {
    match (lvl) {
        ObsLogLevel.Trace => return "trace",
        ObsLogLevel.Debug => return "debug",
        ObsLogLevel.Info => return "info",
        ObsLogLevel.Warn => return "warn",
        ObsLogLevel.Error => return "error",
        ObsLogLevel.Fatal => return "fatal",
    }
}

/// Value type discriminator for an `ObsLogField`.
pub enum ObsLogFieldKind {
    Str,
    Int,
    Float,
    Bool,
}

/// Single typed key-value attribute attached to an `ObsLogRecord`.
pub struct ObsLogField {
    key: string = "";
    kind: ObsLogFieldKind = ObsLogFieldKind.Str;
    str_val: string = "";
    int_val: i64 = 0;
    float_val: f64 = 0.0;
    bool_val: bool = false;

    static fn str(key: string, val: string): ObsLogField {
        return ObsLogField{ key: key, kind: ObsLogFieldKind.Str, str_val: val };
    }

    static fn int(key: string, val: i64): ObsLogField {
        return ObsLogField{ key: key, kind: ObsLogFieldKind.Int, int_val: val };
    }

    static fn float(key: string, val: f64): ObsLogField {
        return ObsLogField{ key: key, kind: ObsLogFieldKind.Float, float_val: val };
    }

    static fn boolean(key: string, val: bool): ObsLogField {
        return ObsLogField{ key: key, kind: ObsLogFieldKind.Bool, bool_val: val };
    }
}

/// Zero-heap structured log event carrying trace context and up to 16 typed KV fields.
pub struct ObsLogRecord {
    timestamp_ms: i64 = 0;
    level: ObsLogLevel = ObsLogLevel.Info;
    service: string = "";
    trace_id: string = "";
    span_id: string = "";
    message: string = "";
    fields: [16]ObsLogField = [ObsLogField{}; 16];
    field_count: usize = 0;

    /// Appends a structured field to this record. Rejects empty keys or >16 fields.
    fn addField(*_, field: ObsLogField): bool {
        if (field.key.len == 0 || _.field_count >= OBS_LOG_MAX_FIELDS) return false;
        _.fields[_.field_count] = field;
        _.field_count += 1;
        return true;
    }
}

/// Wire output format for `ObsStructuredLogger`.
pub enum ObsLogFormat {
    JsonLines,
    Logfmt,
}

/// Configuration for `ObsStructuredLogger`.
pub struct ObsLoggerConfig {
    min_level: ObsLogLevel = ObsLogLevel.Info;
    format: ObsLogFormat = ObsLogFormat.JsonLines;
    default_service: string = "";
    append_newline: bool = true;
}

/// Cumulative emission, drop, and overflow statistics for `ObsStructuredLogger`.
pub struct ObsLoggerStats {
    total_emitted: u64 = 0;
    trace_count: u64 = 0;
    debug_count: u64 = 0;
    info_count: u64 = 0;
    warn_count: u64 = 0;
    error_count: u64 = 0;
    fatal_count: u64 = 0;
    dropped_below_level: u64 = 0;
    buffer_overflows: u64 = 0;
}

// ============================================================================
// Low-Level Zero-Alloc Buffer Writers & Escapers
// ============================================================================

fn appendByte(out: *[]u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn appendBytes(out: *[]u8, pos: *usize, src: []const u8): bool {
    if (pos.* > out.len || src.len > out.len - pos.*) return false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[pos.* + i] = src[i];
    }
    pos.* += src.len;
    return true;
}

fn hexLower(n: u8): u8 {
    return if (n < 10) (48 + n) else (87 + n);
}

fn appendU64(out: *[]u8, pos: *usize, v: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let cur: u64 = v;
    if (cur == 0) {
        tmp[0] = 48;
        n = 1;
    } else {
        while (cur > 0) {
            tmp[n] = (48 + (cur % 10)) as u8;
            cur /= 10;
            n += 1;
        }
    }
    if (pos.* > out.len || n > out.len - pos.*) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos.* + i] = tmp[n - 1 - i];
    }
    pos.* += n;
    return true;
}

fn appendI64(out: *[]u8, pos: *usize, v: i64): bool {
    if (v < 0) {
        if (!appendByte(out, pos, '-')) return false;
        const mag: u64 = (-(v + 1)) as u64 + 1;
        return appendU64(out, pos, mag);
    }
    return appendU64(out, pos, v as u64);
}

fn appendFiniteF64(out: *[]u8, pos: *usize, v: f64): bool {
    let x = v;
    if (x < 0.0) {
        if (!appendByte(out, pos, '-')) return false;
        x = 0.0 - x;
    }
    const whole: u64 = x as u64;
    const frac_raw: f64 = (x - (whole as f64)) * 10000.0 + 0.5;
    let frac: u64 = frac_raw as u64;
    let whole_adj = whole;
    if (frac >= 10000) {
        whole_adj += 1;
        frac = 0;
    }

    if (!appendU64(out, pos, whole_adj)) return false;
    if (frac == 0) return true;

    if (!appendByte(out, pos, '.')) return false;
    let digits: [4]u8 = [
        (48 + ((frac / 1000) % 10)) as u8,
        (48 + ((frac / 100) % 10)) as u8,
        (48 + ((frac / 10) % 10)) as u8,
        (48 + (frac % 10)) as u8,
    ];
    let len: usize = 4;
    while (len > 1 && digits[len - 1] == 48) {
        len -= 1;
    }
    return appendBytes(out, pos, digits[0..len]);
}

fn appendJsonString(out: *[]u8, pos: *usize, s: []const u8): bool {
    if (!appendByte(out, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b == '"') {
            if (!appendBytes(out, pos, "\\\"")) return false;
        } else if (b == '\\') {
            if (!appendBytes(out, pos, "\\\\")) return false;
        } else if (b == 8) {
            if (!appendBytes(out, pos, "\\b")) return false;
        } else if (b == 12) {
            if (!appendBytes(out, pos, "\\f")) return false;
        } else if (b == '\n') {
            if (!appendBytes(out, pos, "\\n")) return false;
        } else if (b == '\r') {
            if (!appendBytes(out, pos, "\\r")) return false;
        } else if (b == '\t') {
            if (!appendBytes(out, pos, "\\t")) return false;
        } else if (b < 0x20) {
            if (!appendBytes(out, pos, "\\u00")) return false;
            if (!appendByte(out, pos, hexLower((b >> 4) & 0x0F))) return false;
            if (!appendByte(out, pos, hexLower(b & 0x0F))) return false;
        } else {
            if (!appendByte(out, pos, b)) return false;
        }
    }
    return appendByte(out, pos, '"');
}

fn needsLogfmtQuoting(s: []const u8): bool {
    if (s.len == 0) return true;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b <= 0x20 || b == '"' || b == '=' || b == '\\') {
            return true;
        }
    }
    return false;
}

fn appendLogfmtValue(out: *[]u8, pos: *usize, s: []const u8): bool {
    if (!needsLogfmtQuoting(s)) {
        return appendBytes(out, pos, s);
    }
    if (!appendByte(out, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b == '"') {
            if (!appendBytes(out, pos, "\\\"")) return false;
        } else if (b == '\\') {
            if (!appendBytes(out, pos, "\\\\")) return false;
        } else if (b == '\n') {
            if (!appendBytes(out, pos, "\\n")) return false;
        } else if (b == '\r') {
            if (!appendBytes(out, pos, "\\r")) return false;
        } else if (b == '\t') {
            if (!appendBytes(out, pos, "\\t")) return false;
        } else {
            if (!appendByte(out, pos, b)) return false;
        }
    }
    return appendByte(out, pos, '"');
}

/// Zero-heap-allocation structured logger supporting JSON Lines and Logfmt output.
pub struct ObsStructuredLogger {
    config: ObsLoggerConfig;
    stats_data: ObsLoggerStats = ObsLoggerStats{};

    /// Initializes a structured logger with `config`.
    static fn init(config: ObsLoggerConfig): ObsStructuredLogger {
        return ObsStructuredLogger{
            config: config,
            stats_data: ObsLoggerStats{},
        };
    }

    /// Returns `true` iff `level` meets or exceeds `config.min_level`.
    fn shouldLog(level: ObsLogLevel): bool {
        return levelRank(level) >= levelRank(_.config.min_level);
    }

    /// Formats `record` in the requested `format` (`JsonLines` or `Logfmt`) directly into `out_buf`.
    /// Returns bytes written, or `null` if `out_buf` is too small.
    fn formatRecord(record: ObsLogRecord, format: ObsLogFormat, out_buf: *[]u8): usize? {
        if (out_buf.len == 0) return null;
        let pos: usize = 0;
        const svc: string = if (record.service.len > 0) record.service else _.config.default_service;
        const limit: usize = if (record.field_count > OBS_LOG_MAX_FIELDS) OBS_LOG_MAX_FIELDS else record.field_count;

        if (format == ObsLogFormat.JsonLines) {
            if (!appendBytes(out_buf, &pos, "{\"ts\":")) return null;
            if (!appendI64(out_buf, &pos, record.timestamp_ms)) return null;
            if (!appendBytes(out_buf, &pos, ",\"level\":")) return null;
            if (!appendJsonString(out_buf, &pos, levelLabel(record.level))) return null;
            if (svc.len > 0) {
                if (!appendBytes(out_buf, &pos, ",\"service\":")) return null;
                if (!appendJsonString(out_buf, &pos, svc)) return null;
            }
            if (record.trace_id.len > 0) {
                if (!appendBytes(out_buf, &pos, ",\"trace_id\":")) return null;
                if (!appendJsonString(out_buf, &pos, record.trace_id)) return null;
            }
            if (record.span_id.len > 0) {
                if (!appendBytes(out_buf, &pos, ",\"span_id\":")) return null;
                if (!appendJsonString(out_buf, &pos, record.span_id)) return null;
            }
            if (!appendBytes(out_buf, &pos, ",\"msg\":")) return null;
            if (!appendJsonString(out_buf, &pos, record.message)) return null;

            let i: usize = 0;
            while (i < limit) : (i += 1) {
                const f = record.fields[i];
                if (f.key.len == 0) continue;
                if (!appendByte(out_buf, &pos, ',')) return null;
                if (!appendJsonString(out_buf, &pos, f.key)) return null;
                if (!appendByte(out_buf, &pos, ':')) return null;
                match (f.kind) {
                    ObsLogFieldKind.Str => {
                        if (!appendJsonString(out_buf, &pos, f.str_val)) return null;
                    },
                    ObsLogFieldKind.Int => {
                        if (!appendI64(out_buf, &pos, f.int_val)) return null;
                    },
                    ObsLogFieldKind.Float => {
                        if (std.math.isNan(f.float_val) || std.math.isInf(f.float_val)) {
                            if (!appendBytes(out_buf, &pos, "null")) return null;
                        } else {
                            if (!appendFiniteF64(out_buf, &pos, f.float_val)) return null;
                        }
                    },
                    ObsLogFieldKind.Bool => {
                        if (!appendBytes(out_buf, &pos, if (f.bool_val) "true" else "false")) return null;
                    },
                }
            }
            if (!appendByte(out_buf, &pos, '}')) return null;
        } else {
            // Logfmt
            if (!appendBytes(out_buf, &pos, "ts=")) return null;
            if (!appendI64(out_buf, &pos, record.timestamp_ms)) return null;
            if (!appendBytes(out_buf, &pos, " level=")) return null;
            if (!appendBytes(out_buf, &pos, levelLabel(record.level))) return null;
            if (svc.len > 0) {
                if (!appendBytes(out_buf, &pos, " service=")) return null;
                if (!appendLogfmtValue(out_buf, &pos, svc)) return null;
            }
            if (record.trace_id.len > 0) {
                if (!appendBytes(out_buf, &pos, " trace_id=")) return null;
                if (!appendLogfmtValue(out_buf, &pos, record.trace_id)) return null;
            }
            if (record.span_id.len > 0) {
                if (!appendBytes(out_buf, &pos, " span_id=")) return null;
                if (!appendLogfmtValue(out_buf, &pos, record.span_id)) return null;
            }
            if (!appendBytes(out_buf, &pos, " msg=")) return null;
            if (!appendLogfmtValue(out_buf, &pos, record.message)) return null;

            let i: usize = 0;
            while (i < limit) : (i += 1) {
                const f = record.fields[i];
                if (f.key.len == 0) continue;
                if (!appendByte(out_buf, &pos, ' ')) return null;
                if (!appendBytes(out_buf, &pos, f.key)) return null;
                if (!appendByte(out_buf, &pos, '=')) return null;
                match (f.kind) {
                    ObsLogFieldKind.Str => {
                        if (!appendLogfmtValue(out_buf, &pos, f.str_val)) return null;
                    },
                    ObsLogFieldKind.Int => {
                        if (!appendI64(out_buf, &pos, f.int_val)) return null;
                    },
                    ObsLogFieldKind.Float => {
                        if (std.math.isNan(f.float_val)) {
                            if (!appendBytes(out_buf, &pos, "NaN")) return null;
                        } else if (std.math.isInf(f.float_val)) {
                            if (!appendBytes(out_buf, &pos, if (f.float_val < 0.0) "-Inf" else "Inf")) return null;
                        } else {
                            if (!appendFiniteF64(out_buf, &pos, f.float_val)) return null;
                        }
                    },
                    ObsLogFieldKind.Bool => {
                        if (!appendBytes(out_buf, &pos, if (f.bool_val) "true" else "false")) return null;
                    },
                }
            }
        }

        if (_.config.append_newline) {
            if (!appendByte(out_buf, &pos, '\n')) return null;
        }
        return pos;
    }

    /// Emits `record` into `out_buf` if `shouldLog(record.level)` passes, updating per-level stats.
    /// Returns `0` when filtered out below `min_level`, bytes written on success, or `null` on buffer overflow.
    fn emit(*_, record: ObsLogRecord, out_buf: *[]u8): usize? {
        if (!_.shouldLog(record.level)) {
            _.stats_data.dropped_below_level += 1;
            return 0;
        }

        const n = _.formatRecord(record, _.config.format, out_buf);
        if (written := n) {
            _.stats_data.total_emitted += 1;
            match (record.level) {
                ObsLogLevel.Trace => _.stats_data.trace_count += 1,
                ObsLogLevel.Debug => _.stats_data.debug_count += 1,
                ObsLogLevel.Info => _.stats_data.info_count += 1,
                ObsLogLevel.Warn => _.stats_data.warn_count += 1,
                ObsLogLevel.Error => _.stats_data.error_count += 1,
                ObsLogLevel.Fatal => _.stats_data.fatal_count += 1,
            }
            return written;
        }

        _.stats_data.buffer_overflows += 1;
        return null;
    }

    /// Returns a snapshot of cumulative emission, drop, and overflow statistics.
    fn stats(): ObsLoggerStats {
        return _.stats_data;
    }

    /// Resets all telemetry counters to zero.
    fn resetStats(*_) {
        _.stats_data = ObsLoggerStats{};
    }
}
