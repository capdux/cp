// Source: cp/lib/logger/trace.cp (W3C Trace Context & Lightweight Span Tracer)
// lib/logger/trace.cp
// W3C Trace Context Level 1 (traceparent & tracestate) parser/formatter and
// zero-heap lightweight OpenTelemetry-compatible Span tracer with JSON export.

const std = @import("std");
import { LogField, LogFieldValue, LogFormatter } from "./structured.cp";

pub const TRACEPARENT_LEN: usize = 55;
pub const TRACESTATE_MAX_ENTRIES: usize = 32;
pub const SPAN_MAX_ATTRS: usize = 16;
pub const SPAN_MAX_EVENTS: usize = 16;
pub const SPAN_EVENT_MAX_ATTRS: usize = 8;

fn trAppendBytes(out_buf: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out_buf.len) {
        out_buf[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn trAppendU64(out_buf: []u8, pos: usize, val: u64): usize {
    let tmp: [32]u8 = [0; 32];
    const s = std.fmt.bufPrint(tmp[0..32], "{d}", .{val}) catch "0";
    return trAppendBytes(out_buf, pos, s);
}

fn trHexLowerDigit(nibble: u8): u8 {
    if (nibble < 10) return '0' + nibble;
    return 'a' + (nibble - 10);
}

fn trIsLowerHex(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

fn trHexVal(c: u8): u8 {
    if (c >= '0' && c <= '9') return c - '0';
    return (c - 'a') + 10;
}

fn trValidateHexNonZero(s: []const u8): bool {
    let all_zero = true;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (!trIsLowerHex(c)) return false;
        if (c != '0') all_zero = false;
        i += 1;
    }
    return !all_zero;
}

fn trNowNs(): u64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    zig {
        _ = std.os.linux.clock_gettime(.REALTIME, &ts);
    }
    if (ts.sec <= 0) return 0;
    return (ts.sec as u64) * 1000000000 + (ts.nsec as u64);
}

fn trTrimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t')) {
        start += 1;
    }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) {
        end -= 1;
    }
    return s[start..end];
}

fn trIsKeyChar(c: u8): bool {
    return (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9')
        || c == '_'
        || c == '-'
        || c == '*'
        || c == '/';
}

fn trIsValidSimpleKey(s: []const u8): bool {
    if (s.len == 0 || s.len > 256) return false;
    if (!(s[0] >= 'a' && s[0] <= 'z')) return false;
    let i: usize = 1;
    while (i < s.len) {
        if (!trIsKeyChar(s[i])) return false;
        i += 1;
    }
    return true;
}

fn trIsValidTraceStateKey(key: []const u8): bool {
    if (key.len == 0 || key.len > 256) return false;
    let at_pos: usize? = null;
    let i: usize = 0;
    while (i < key.len) {
        if (key[i] == '@') {
            if (at_pos != null) return false;
            at_pos = i;
        }
        i += 1;
    }
    if (pos := at_pos) {
        const tenant = key[0..pos];
        const vendor = key[pos + 1 .. key.len];
        if (tenant.len == 0 || tenant.len > 241) return false;
        if (vendor.len == 0 || vendor.len > 14) return false;
        if (!((tenant[0] >= 'a' && tenant[0] <= 'z') || (tenant[0] >= '0' && tenant[0] <= '9'))) {
            return false;
        }
        let ti: usize = 1;
        while (ti < tenant.len) {
            if (!trIsKeyChar(tenant[ti])) return false;
            ti += 1;
        }
        return trIsValidSimpleKey(vendor);
    }
    return trIsValidSimpleKey(key);
}

fn trIsValidTraceStateValue(val: []const u8): bool {
    if (val.len == 0 || val.len > 256) return false;
    if (val[val.len - 1] == ' ') return false;
    let i: usize = 0;
    while (i < val.len) {
        const c = val[i];
        if (c < 0x20 || c > 0x7e || c == ',' || c == '=') return false;
        i += 1;
    }
    return true;
}

/// W3C Trace Context Level 1 `traceparent` header representation (`00-<32hex>-<16hex>-<2hex>`).
pub struct TraceParent {
    version: u8 = 0;
    trace_id: [32]u8 = [0; 32];
    span_id: [16]u8 = [0; 16];
    flags: u8 = 0;

    /// Parses and strictly validates a W3C `traceparent` header.
    /// Rejects non-`00` version (including `ff`), uppercase hex, invalid lengths, and all-zero IDs.
    static fn parse(header: []const u8): TraceParent? {
        if (header.len != TRACEPARENT_LEN) return null;
        if (header[2] != '-' || header[35] != '-' || header[52] != '-') return null;

        if (!trIsLowerHex(header[0]) || !trIsLowerHex(header[1])) return null;
        const ver = (trHexVal(header[0]) << 4) | trHexVal(header[1]);
        if (ver != 0) return null;

        const tid = header[3..35];
        if (!trValidateHexNonZero(tid)) return null;

        const sid = header[36..52];
        if (!trValidateHexNonZero(sid)) return null;

        if (!trIsLowerHex(header[53]) || !trIsLowerHex(header[54])) return null;
        const flags = (trHexVal(header[53]) << 4) | trHexVal(header[54]);

        let tp = TraceParent {
            version: ver,
            trace_id: [0; 32],
            span_id: [0; 16],
            flags: flags,
        };
        let i: usize = 0;
        while (i < 32) {
            tp.trace_id[i] = tid[i];
            i += 1;
        }
        let j: usize = 0;
        while (j < 16) {
            tp.span_id[j] = sid[j];
            j += 1;
        }
        return tp;
    }

    /// Constructs a `TraceParent` from validated lowercase hex `trace_id` (32) and `span_id` (16).
    static fn init(trace_id: []const u8, span_id: []const u8, sampled: bool): TraceParent? {
        if (trace_id.len != 32 || span_id.len != 16) return null;
        if (!trValidateHexNonZero(trace_id)) return null;
        if (!trValidateHexNonZero(span_id)) return null;

        let tp = TraceParent {
            version: 0,
            trace_id: [0; 32],
            span_id: [0; 16],
            flags: if (sampled) 0x01 else 0x00,
        };
        let i: usize = 0;
        while (i < 32) {
            tp.trace_id[i] = trace_id[i];
            i += 1;
        }
        let j: usize = 0;
        while (j < 16) {
            tp.span_id[j] = span_id[j];
            j += 1;
        }
        return tp;
    }

    fn isSampled(): bool {
        return (_.flags & 0x01) != 0;
    }

    fn setSampled(*_, sampled: bool) {
        if (sampled) {
            _.flags = _.flags | 0x01;
        } else {
            _.flags = _.flags & 0xfe;
        }
    }

    /// Formats this `TraceParent` into `out_buf` (must be >= 55 bytes) and returns the written slice.
    fn format(out_buf: []u8): []const u8 {
        if (out_buf.len < TRACEPARENT_LEN) return "";
        out_buf[0] = trHexLowerDigit((_.version >> 4) & 0x0f);
        out_buf[1] = trHexLowerDigit(_.version & 0x0f);
        out_buf[2] = '-';
        let i: usize = 0;
        while (i < 32) {
            out_buf[3 + i] = _.trace_id[i];
            i += 1;
        }
        out_buf[35] = '-';
        let j: usize = 0;
        while (j < 16) {
            out_buf[36 + j] = _.span_id[j];
            j += 1;
        }
        out_buf[52] = '-';
        out_buf[53] = trHexLowerDigit((_.flags >> 4) & 0x0f);
        out_buf[54] = trHexLowerDigit(_.flags & 0x0f);
        return out_buf[0..TRACEPARENT_LEN];
    }

    fn copyTraceId(out_buf: []u8): []const u8 {
        if (out_buf.len < 32) return "";
        let i: usize = 0;
        while (i < 32) {
            out_buf[i] = _.trace_id[i];
            i += 1;
        }
        return out_buf[0..32];
    }

    fn copySpanId(out_buf: []u8): []const u8 {
        if (out_buf.len < 16) return "";
        let i: usize = 0;
        while (i < 16) {
            out_buf[i] = _.span_id[i];
            i += 1;
        }
        return out_buf[0..16];
    }

    /// Derives a child `TraceParent` preserving `trace_id` and `flags` with a new validated `span_id`.
    fn childSpan(new_span_id: []const u8): TraceParent? {
        if (new_span_id.len != 16) return null;
        if (!trValidateHexNonZero(new_span_id)) return null;
        let child = TraceParent {
            version: _.version,
            trace_id: _.trace_id,
            span_id: [0; 16],
            flags: _.flags,
        };
        let i: usize = 0;
        while (i < 16) {
            child.span_id[i] = new_span_id[i];
            i += 1;
        }
        return child;
    }
}

/// Single key-value entry in a W3C `tracestate` list.
pub struct TraceStateEntry {
    key: []const u8 = "";
    value: []const u8 = "";
}

/// W3C Trace Context `tracestate` parser, ordered list-member store, and formatter.
pub struct TraceState {
    entries: [TRACESTATE_MAX_ENTRIES]TraceStateEntry = undefined;
    count: usize = 0;

    static fn empty(): TraceState {
        return TraceState {
            entries: undefined,
            count: 0,
        };
    }

    static fn parse(header: []const u8): TraceState? {
        let ts = TraceState.empty();
        if (header.len == 0) return ts;

        let pos: usize = 0;
        while (pos <= header.len) {
            let next_comma = header.len;
            let scan = pos;
            while (scan < header.len) {
                if (header[scan] == ',') {
                    next_comma = scan;
                    break;
                }
                scan += 1;
            }
            const member = trTrimOws(header[pos..next_comma]);
            if (member.len > 0) {
                let eq_pos: usize? = null;
                let k: usize = 0;
                while (k < member.len) {
                    if (member[k] == '=') {
                        eq_pos = k;
                        break;
                    }
                    k += 1;
                }
                const eq = eq_pos ?? return null;
                const key = member[0..eq];
                const val = member[eq + 1 .. member.len];
                if (!trIsValidTraceStateKey(key) || !trIsValidTraceStateValue(val)) {
                    return null;
                }
                let dup = false;
                let ei: usize = 0;
                while (ei < ts.count) {
                    if (std.mem.eql(u8, ts.entries[ei].key, key)) {
                        dup = true;
                        break;
                    }
                    ei += 1;
                }
                if (!dup && ts.count < TRACESTATE_MAX_ENTRIES) {
                    ts.entries[ts.count] = TraceStateEntry { key: key, value: val };
                    ts.count += 1;
                }
            }
            if (next_comma >= header.len) break;
            pos = next_comma + 1;
        }
        return ts;
    }

    fn len(): usize {
        return _.count;
    }

    fn get(key: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].key, key)) {
                return _.entries[i].value;
            }
            i += 1;
        }
        return null;
    }

    /// Inserts or updates `key=value`, moving it to the front (leftmost) per W3C Trace Context spec.
    fn set(*_, key: []const u8, value: []const u8): bool {
        if (!trIsValidTraceStateKey(key) || !trIsValidTraceStateValue(value)) {
            return false;
        }
        let existing_idx: usize? = null;
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].key, key)) {
                existing_idx = i;
                break;
            }
            i += 1;
        }

        if (idx := existing_idx) {
            let j = idx;
            while (j > 0) {
                _.entries[j] = _.entries[j - 1];
                j -= 1;
            }
            _.entries[0] = TraceStateEntry { key: key, value: value };
            return true;
        }

        const target_end = if (_.count < TRACESTATE_MAX_ENTRIES) _.count else TRACESTATE_MAX_ENTRIES - 1;
        let k = target_end;
        while (k > 0) {
            _.entries[k] = _.entries[k - 1];
            k -= 1;
        }
        _.entries[0] = TraceStateEntry { key: key, value: value };
        if (_.count < TRACESTATE_MAX_ENTRIES) {
            _.count += 1;
        }
        return true;
    }

    fn remove(*_, key: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.entries[i].key, key)) {
                let j = i;
                while (j + 1 < _.count) {
                    _.entries[j] = _.entries[j + 1];
                    j += 1;
                }
                _.count -= 1;
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn format(out_buf: []u8): []const u8 {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.count && pos < out_buf.len) {
            if (i > 0) {
                pos = trAppendBytes(out_buf, pos, ",");
            }
            pos = trAppendBytes(out_buf, pos, _.entries[i].key);
            pos = trAppendBytes(out_buf, pos, "=");
            pos = trAppendBytes(out_buf, pos, _.entries[i].value);
            i += 1;
        }
        return out_buf[0..pos];
    }
}

/// OpenTelemetry-compatible Span execution status.
pub enum SpanStatus: u8 {
    Unset = 0,
    Ok = 1,
    Error = 2,

    fn label(): []const u8 {
        return switch (_) {
            .Unset => "UNSET",
            .Ok => "OK",
            .Error => "ERROR",
        };
    }
}

/// Timestamped annotation event attached to a `Span`.
pub struct SpanEvent {
    name: []const u8 = "";
    timestamp_ns: u64 = 0;
    attributes: [SPAN_EVENT_MAX_ATTRS]LogField = undefined;
    attr_count: usize = 0;
}

/// Lightweight zero-heap OpenTelemetry-compatible Span with attributes, events, and JSON export.
pub struct Span {
    trace_id: [32]u8 = [0; 32];
    span_id: [16]u8 = [0; 16];
    parent_span_id: [16]u8 = [0; 16];
    has_parent: bool = false;
    name: []const u8 = "";
    service_name: []const u8 = "";
    start_ns: u64 = 0;
    end_ns: u64 = 0;
    ended: bool = false;
    status: SpanStatus = SpanStatus.Unset;
    status_message: []const u8 = "";
    attributes: [SPAN_MAX_ATTRS]LogField = undefined;
    attr_count: usize = 0;
    events: [SPAN_MAX_EVENTS]SpanEvent = undefined;
    event_count: usize = 0;

    static fn startAt(name: []const u8, tp: TraceParent, start_ns: u64): Span {
        return Span {
            trace_id: tp.trace_id,
            span_id: tp.span_id,
            parent_span_id: [0; 16],
            has_parent: false,
            name: name,
            service_name: "",
            start_ns: start_ns,
            end_ns: start_ns,
            ended: false,
            status: SpanStatus.Unset,
            status_message: "",
            attributes: undefined,
            attr_count: 0,
            events: undefined,
            event_count: 0,
        };
    }

    static fn start(name: []const u8, tp: TraceParent): Span {
        return Span.startAt(name, tp, trNowNs());
    }

    static fn startChildAt(name: []const u8, parent: Span, new_span_id: []const u8, start_ns: u64): Span? {
        if (new_span_id.len != 16 || !trValidateHexNonZero(new_span_id)) return null;
        let sp = Span {
            trace_id: parent.trace_id,
            span_id: [0; 16],
            parent_span_id: parent.span_id,
            has_parent: true,
            name: name,
            service_name: parent.service_name,
            start_ns: start_ns,
            end_ns: start_ns,
            ended: false,
            status: SpanStatus.Unset,
            status_message: "",
            attributes: undefined,
            attr_count: 0,
            events: undefined,
            event_count: 0,
        };
        let i: usize = 0;
        while (i < 16) {
            sp.span_id[i] = new_span_id[i];
            i += 1;
        }
        return sp;
    }

    static fn startChild(name: []const u8, parent: Span, new_span_id: []const u8): Span? {
        return Span.startChildAt(name, parent, new_span_id, trNowNs());
    }

    fn setParent(*_, parent_span_id: []const u8): bool {
        if (parent_span_id.len != 16 || !trValidateHexNonZero(parent_span_id)) return false;
        let i: usize = 0;
        while (i < 16) {
            _.parent_span_id[i] = parent_span_id[i];
            i += 1;
        }
        _.has_parent = true;
        return true;
    }

    fn setServiceName(*_, svc: []const u8) {
        _.service_name = svc;
    }

    fn setAttr(*_, key: []const u8, val: LogFieldValue): bool {
        let i: usize = 0;
        while (i < _.attr_count) {
            if (std.mem.eql(u8, _.attributes[i].key, key)) {
                _.attributes[i].value = val;
                return true;
            }
            i += 1;
        }
        if (_.attr_count >= SPAN_MAX_ATTRS) return false;
        _.attributes[_.attr_count] = LogField { key: key, value: val };
        _.attr_count += 1;
        return true;
    }

    fn setAttrStr(*_, key: []const u8, val: []const u8): bool {
        return _.setAttr(key, LogFieldValue.Str(val));
    }

    fn setAttrInt(*_, key: []const u8, val: i64): bool {
        return _.setAttr(key, LogFieldValue.Int(val));
    }

    fn setAttrBool(*_, key: []const u8, val: bool): bool {
        return _.setAttr(key, LogFieldValue.Bool(val));
    }

    fn setAttrFloat(*_, key: []const u8, val: f64): bool {
        return _.setAttr(key, LogFieldValue.Float(val));
    }

    fn addEventAt(*_, name: []const u8, timestamp_ns: u64, attrs: []const LogField): bool {
        if (_.event_count >= SPAN_MAX_EVENTS) return false;
        let ev = SpanEvent {
            name: name,
            timestamp_ns: timestamp_ns,
            attributes: undefined,
            attr_count: 0,
        };
        let i: usize = 0;
        while (i < attrs.len && ev.attr_count < SPAN_EVENT_MAX_ATTRS) {
            ev.attributes[ev.attr_count] = attrs[i];
            ev.attr_count += 1;
            i += 1;
        }
        _.events[_.event_count] = ev;
        _.event_count += 1;
        return true;
    }

    fn addEvent(*_, name: []const u8, attrs: []const LogField): bool {
        return _.addEventAt(name, trNowNs(), attrs);
    }

    fn setStatus(*_, status: SpanStatus, message: []const u8) {
        _.status = status;
        _.status_message = message;
    }

    fn endAt(*_, end_ns: u64) {
        _.end_ns = if (end_ns >= _.start_ns) end_ns else _.start_ns;
        _.ended = true;
    }

    fn end(*_) {
        _.endAt(trNowNs());
    }

    fn durationNs(): u64 {
        if (!_.ended || _.end_ns < _.start_ns) return 0;
        return _.end_ns - _.start_ns;
    }

    /// Exports this `Span` as an OpenTelemetry-style JSON object into `out_buf`.
    fn toJson(out_buf: []u8): []const u8 {
        let pos: usize = 0;
        pos = trAppendBytes(out_buf, pos, "{\"traceId\":");
        pos = LogFormatter.escapeJsonString(out_buf, pos, _.trace_id[0..32]);
        pos = trAppendBytes(out_buf, pos, ",\"spanId\":");
        pos = LogFormatter.escapeJsonString(out_buf, pos, _.span_id[0..16]);
        if (_.has_parent) {
            pos = trAppendBytes(out_buf, pos, ",\"parentSpanId\":");
            pos = LogFormatter.escapeJsonString(out_buf, pos, _.parent_span_id[0..16]);
        }
        pos = trAppendBytes(out_buf, pos, ",\"name\":");
        pos = LogFormatter.escapeJsonString(out_buf, pos, _.name);
        if (_.service_name.len > 0) {
            pos = trAppendBytes(out_buf, pos, ",\"serviceName\":");
            pos = LogFormatter.escapeJsonString(out_buf, pos, _.service_name);
        }
        pos = trAppendBytes(out_buf, pos, ",\"startTimeUnixNano\":");
        pos = trAppendU64(out_buf, pos, _.start_ns);
        pos = trAppendBytes(out_buf, pos, ",\"endTimeUnixNano\":");
        pos = trAppendU64(out_buf, pos, _.end_ns);
        pos = trAppendBytes(out_buf, pos, ",\"durationNs\":");
        pos = trAppendU64(out_buf, pos, _.durationNs());

        pos = trAppendBytes(out_buf, pos, ",\"status\":{\"code\":");
        pos = LogFormatter.escapeJsonString(out_buf, pos, _.status.label());
        if (_.status_message.len > 0) {
            pos = trAppendBytes(out_buf, pos, ",\"message\":");
            pos = LogFormatter.escapeJsonString(out_buf, pos, _.status_message);
        }
        pos = trAppendBytes(out_buf, pos, "}");

        pos = trAppendBytes(out_buf, pos, ",\"attributes\":{");
        let ai: usize = 0;
        while (ai < _.attr_count && pos < out_buf.len) {
            if (ai > 0) pos = trAppendBytes(out_buf, pos, ",");
            pos = LogFormatter.escapeJsonString(out_buf, pos, _.attributes[ai].key);
            pos = trAppendBytes(out_buf, pos, ":");
            pos = LogFormatter.appendFieldValueJson(out_buf, pos, _.attributes[ai].value);
            ai += 1;
        }
        pos = trAppendBytes(out_buf, pos, "}");

        pos = trAppendBytes(out_buf, pos, ",\"events\":[");
        let ei: usize = 0;
        while (ei < _.event_count && pos < out_buf.len) {
            if (ei > 0) pos = trAppendBytes(out_buf, pos, ",");
            const ev = _.events[ei];
            pos = trAppendBytes(out_buf, pos, "{\"name\":");
            pos = LogFormatter.escapeJsonString(out_buf, pos, ev.name);
            pos = trAppendBytes(out_buf, pos, ",\"timeUnixNano\":");
            pos = trAppendU64(out_buf, pos, ev.timestamp_ns);
            pos = trAppendBytes(out_buf, pos, ",\"attributes\":{");
            let eai: usize = 0;
            while (eai < ev.attr_count && pos < out_buf.len) {
                if (eai > 0) pos = trAppendBytes(out_buf, pos, ",");
                pos = LogFormatter.escapeJsonString(out_buf, pos, ev.attributes[eai].key);
                pos = trAppendBytes(out_buf, pos, ":");
                pos = LogFormatter.appendFieldValueJson(out_buf, pos, ev.attributes[eai].value);
                eai += 1;
            }
            pos = trAppendBytes(out_buf, pos, "}}");
            ei += 1;
        }
        pos = trAppendBytes(out_buf, pos, "]}");
        return out_buf[0..pos];
    }
}
