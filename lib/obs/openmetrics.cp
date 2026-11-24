// Reference-hardened against HdrHistogram/HdrHistogram_rust (MIT OR Apache-2.0) & OpenMetrics 1.0
// Source: native-cp-stdlib (OpenMetrics / Prometheus Text Format)
// Pure CP OpenMetrics 1.0 / Prometheus text exposition parser & serializer with:
//   - Strict Metric Name (`[a-zA-Z_:][a-zA-Z0-9_:]*`) & Label Name (`[a-zA-Z_][a-zA-Z0-9_]*`) validation
//   - Label Value escaping (`\"`, `\n`, `\\`) & raw control-char / CRLF injection rejection
//   - Special float (`+Inf`, `-Inf`, `NaN`) parsing & canonical OpenMetrics 1.0 formatting
//   - 16-lane @Vector(16, u8) SIMD (`OmVec16u8`) escape/control scanner (`simdOmFindNeedEscape`)

const std = @import("std");

/// Module-local 16-lane byte vector alias for OpenMetrics escape/control scanning hot paths.
type OmVec16u8 = @Vector(16, u8);

// ============================================================================
// 1. OpenMetrics 1.0 Name Validators & 16-Lane SIMD Escape Scanner (0-Diff)
// ============================================================================

fn omIsAsciiAlpha(c: u8): bool {
    return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z');
}

fn omIsAsciiDigit(c: u8): bool {
    return c >= '0' and c <= '9';
}

/// Validates an OpenMetrics 1.0 / Prometheus metric name: `[a-zA-Z_:][a-zA-Z0-9_:]*`.
pub fn isValidMetricName(name: []const u8): bool {
    if (name.len == 0) return false;
    const c0 = name[0];
    if (!omIsAsciiAlpha(c0) and c0 != '_' and c0 != ':') return false;
    let i: usize = 1;
    while (i < name.len) : (i += 1) {
        const c = name[i];
        if (!omIsAsciiAlpha(c) and !omIsAsciiDigit(c) and c != '_' and c != ':') {
            return false;
        }
    }
    return true;
}

/// Validates an OpenMetrics 1.0 / Prometheus label name: `[a-zA-Z_][a-zA-Z0-9_]*`.
pub fn isValidLabelName(name: []const u8): bool {
    if (name.len == 0) return false;
    const c0 = name[0];
    if (!omIsAsciiAlpha(c0) and c0 != '_') return false;
    let i: usize = 1;
    while (i < name.len) : (i += 1) {
        const c = name[i];
        if (!omIsAsciiAlpha(c) and !omIsAsciiDigit(c) and c != '_') {
            return false;
        }
    }
    return true;
}

fn omNeedsEscapeByte(c: u8): bool {
    return c == '"' or c == '\\' or c < 0x20 or c == 0x7F;
}

/// Scalar reference: returns the first index in `s` that requires OpenMetrics escaping or is a
/// control byte (`"`, `\`, `< 0x20` including `\n`/`\r`/`\0`, or `0x7F`), or `null` if clean.
pub fn scalarOmFindNeedEscape(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (omNeedsEscapeByte(s[i])) return i;
    }
    return null;
}

/// 16-lane SIMD accelerated scanner for bytes requiring OpenMetrics escaping or control-char rejection
/// (`"`, `\`, `< 0x20` including `\n`/`\r`/`\0`, or `0x7F`).
pub fn simdOmFindNeedEscape(s: []const u8): usize? {
    const v_quote: OmVec16u8 = @splat('"');
    const v_bslash: OmVec16u8 = @splat('\\');
    const v_space: OmVec16u8 = @splat(0x20);
    const v_del: OmVec16u8 = @splat(0x7F);

    let i: usize = 0;
    while (i + 16 <= s.len) {
        const v: OmVec16u8 = s[i .. i + 16][0..16].*;
        const hit = (v == v_quote) | (v == v_bslash) | (v < v_space) | (v == v_del);
        const mask: u16 = @bitCast(hit);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < s.len) : (i += 1) {
        if (omNeedsEscapeByte(s[i])) return i;
    }
    return null;
}

// ============================================================================
// 2. OpenMetrics 1.0 Data Models
// ============================================================================

pub enum MetricType {
    Counter,
    Gauge,
    Histogram,
    Summary,
    Untyped,

    pub fn toString(): []const u8 {
        return switch (_) {
            MetricType.Counter => "counter",
            MetricType.Gauge => "gauge",
            MetricType.Histogram => "histogram",
            MetricType.Summary => "summary",
            MetricType.Untyped => "untyped",
        };
    }

    pub static fn fromString(raw: []const u8): MetricType? {
        if (std.mem.eql(u8, raw, "counter")) return MetricType.Counter;
        if (std.mem.eql(u8, raw, "gauge")) return MetricType.Gauge;
        if (std.mem.eql(u8, raw, "histogram")) return MetricType.Histogram;
        if (std.mem.eql(u8, raw, "summary")) return MetricType.Summary;
        if (std.mem.eql(u8, raw, "untyped")) return MetricType.Untyped;
        return null;
    }
}

pub struct Label {
    name: []const u8,
    value: []const u8,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.name);
        alloc.free(_.value);
    }
}

pub struct Sample {
    name: []const u8,
    labels: []Label,
    value: f64,
    timestamp_ms: i64?,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.name);
        let i: usize = 0;
        while (i < _.labels.len) {
            _.labels[i].deinit(alloc);
            i += 1;
        }
        alloc.free(_.labels);
    }
}

pub struct MetricFamily {
    name: []const u8,
    help: ([]const u8)?,
    unit: ([]const u8)?,
    metric_type: MetricType,
    samples: []Sample,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.name);
        if (h := _.help) {
            alloc.free(h);
        }
        if (u := _.unit) {
            alloc.free(u);
        }
        let i: usize = 0;
        while (i < _.samples.len) {
            _.samples[i].deinit(alloc);
            i += 1;
        }
        alloc.free(_.samples);
    }
}

struct OmFamilyBuilder {
    name: []const u8,
    help: ([]const u8)?,
    unit: ([]const u8)?,
    metric_type: MetricType,
    samples: std.ArrayList(Sample),

    fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.name);
        if (h := _.help) {
            alloc.free(h);
        }
        if (u := _.unit) {
            alloc.free(u);
        }
        let i: usize = 0;
        while (i < _.samples.items.len) {
            _.samples.items[i].deinit(alloc);
            i += 1;
        }
        _.samples.deinit(alloc);
    }
}

pub struct MetricsDocument {
    families: []MetricFamily,
    has_eof: bool,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.families.len) {
            _.families[i].deinit(alloc);
            i += 1;
        }
        alloc.free(_.families);
    }

    pub fn findFamily(target_name: []const u8): (*const MetricFamily)? {
        let i: usize = 0;
        while (i < _.families.len) {
            if (std.mem.eql(u8, _.families[i].name, target_name)) {
                return &_.families[i];
            }
            i += 1;
        }
        return null;
    }

    pub static fn parse(alloc: std.mem.Allocator, text: []const u8): MetricsDocument! {
        let builders = std.ArrayList(OmFamilyBuilder).empty;
        errdefer {
            let bi: usize = 0;
            while (bi < builders.items.len) {
                builders.items[bi].deinit(alloc);
                bi += 1;
            }
            builders.deinit(alloc);
        }

        let eof_seen = false;
        let pos: usize = 0;

        while (pos < text.len) {
            const line_start = pos;
            while (pos < text.len and text[pos] != '\n') {
                pos += 1;
            }
            let raw_line = text[line_start..pos];
            if (pos < text.len and text[pos] == '\n') {
                pos += 1;
            }
            if (raw_line.len > 0 and raw_line[raw_line.len - 1] == '\r') {
                raw_line = raw_line[0 .. raw_line.len - 1];
            }
            // Reject embedded CR or NUL inside a line (CRLF injection / binary payload guard).
            let ci: usize = 0;
            while (ci < raw_line.len) : (ci += 1) {
                const ch = raw_line[ci];
                if (ch == '\r' or ch == 0x00) {
                    return error.InvalidControlChar;
                }
            }

            const line = std.mem.trim(u8, raw_line, " \t");
            if (line.len == 0) {
                continue;
            }

            if (line[0] == '#') {
                const after_hash = std.mem.trim(u8, line[1..], " \t");
                if (std.mem.eql(u8, after_hash, "EOF")) {
                    eof_seen = true;
                    continue;
                }
                if (std.mem.startsWith(u8, after_hash, "HELP")) {
                    const rest = omSkipSpaces(after_hash[4..]);
                    const split = omSplitFirstToken(rest);
                    if (!isValidMetricName(split.token)) {
                        return error.InvalidHelpLine;
                    }
                    const fam_idx = try omGetOrCreateFamily(alloc, &builders, split.token);
                    const unescaped = try omUnescapeHelp(alloc, omSkipSpaces(split.rest));
                    if (old_help := builders.items[fam_idx].help) {
                        alloc.free(old_help);
                    }
                    builders.items[fam_idx].help = unescaped;
                    continue;
                }
                if (std.mem.startsWith(u8, after_hash, "TYPE")) {
                    const rest = omSkipSpaces(after_hash[4..]);
                    const split1 = omSplitFirstToken(rest);
                    const split2 = omSplitFirstToken(omSkipSpaces(split1.rest));
                    if (!isValidMetricName(split1.token) or split2.token.len == 0) {
                        return error.InvalidTypeLine;
                    }
                    const mtype = MetricType.fromString(split2.token) orelse return error.InvalidMetricType;
                    const fam_idx = try omGetOrCreateFamily(alloc, &builders, split1.token);
                    builders.items[fam_idx].metric_type = mtype;
                    continue;
                }
                if (std.mem.startsWith(u8, after_hash, "UNIT")) {
                    const rest = omSkipSpaces(after_hash[4..]);
                    const split1 = omSplitFirstToken(rest);
                    const split2 = omSplitFirstToken(omSkipSpaces(split1.rest));
                    if (!isValidMetricName(split1.token)) {
                        return error.InvalidUnitLine;
                    }
                    const fam_idx = try omGetOrCreateFamily(alloc, &builders, split1.token);
                    if (split2.token.len > 0) {
                        const unit_copy = try alloc.dupe(u8, split2.token);
                        if (old_unit := builders.items[fam_idx].unit) {
                            alloc.free(old_unit);
                        }
                        builders.items[fam_idx].unit = unit_copy;
                    }
                    continue;
                }
                continue;
            }

            let sample = try omParseSampleLine(alloc, line);
            errdefer sample.deinit(alloc);

            const fam_idx = try omResolveFamilyForSample(alloc, &builders, sample.name);
            try builders.items[fam_idx].samples.append(alloc, sample);
        }

        let families = std.ArrayList(MetricFamily).empty;
        errdefer {
            let fi: usize = 0;
            while (fi < families.items.len) {
                families.items[fi].deinit(alloc);
                fi += 1;
            }
            families.deinit(alloc);
        }

        let i: usize = 0;
        while (i < builders.items.len) {
            const owned_samples = try builders.items[i].samples.toOwnedSlice(alloc);
            const fam = MetricFamily{
                .name = builders.items[i].name,
                .help = builders.items[i].help,
                .unit = builders.items[i].unit,
                .metric_type = builders.items[i].metric_type,
                .samples = owned_samples,
            };
            builders.items[i].samples.deinit(alloc);
            try families.append(alloc, fam);
            i += 1;
        }
        builders.deinit(alloc);

        return MetricsDocument{
            .families = try families.toOwnedSlice(alloc),
            .has_eof = eof_seen,
        };
    }

    pub fn serialize(alloc: std.mem.Allocator): []u8! {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);

        let fi: usize = 0;
        while (fi < _.families.len) {
            const fam = _.families[fi];
            if (h := fam.help) {
                try out.appendSlice(alloc, "# HELP ");
                try out.appendSlice(alloc, fam.name);
                try out.append(alloc, ' ');
                try omAppendEscapedHelp(alloc, &out, h);
                try out.append(alloc, '\n');
            }
            if (u := fam.unit) {
                try out.appendSlice(alloc, "# UNIT ");
                try out.appendSlice(alloc, fam.name);
                try out.append(alloc, ' ');
                try out.appendSlice(alloc, u);
                try out.append(alloc, '\n');
            }
            try out.appendSlice(alloc, "# TYPE ");
            try out.appendSlice(alloc, fam.name);
            try out.append(alloc, ' ');
            try out.appendSlice(alloc, omMetricTypeStr(fam.metric_type));
            try out.append(alloc, '\n');

            let si: usize = 0;
            while (si < fam.samples.len) {
                const s = fam.samples[si];
                try out.appendSlice(alloc, s.name);
                if (s.labels.len > 0) {
                    try out.append(alloc, '{');
                    let li: usize = 0;
                    while (li < s.labels.len) {
                        if (li > 0) {
                            try out.append(alloc, ',');
                        }
                        try out.appendSlice(alloc, s.labels[li].name);
                        try out.appendSlice(alloc, "=\"");
                        try omAppendEscapedLabelValue(alloc, &out, s.labels[li].value);
                        try out.append(alloc, '"');
                        li += 1;
                    }
                    try out.append(alloc, '}');
                }
                try out.append(alloc, ' ');
                try omAppendFloatValue(alloc, &out, s.value);
                if (ts := s.timestamp_ms) {
                    try out.append(alloc, ' ');
                    const ts_str = try std.fmt.allocPrint(alloc, "{d}", .{ts});
                    defer alloc.free(ts_str);
                    try out.appendSlice(alloc, ts_str);
                }
                try out.append(alloc, '\n');
                si += 1;
            }
            fi += 1;
        }

        if (_.has_eof) {
            try out.appendSlice(alloc, "# EOF\n");
        }

        return try out.toOwnedSlice(alloc);
    }
}

/// Top-level convenience entry point parsing an OpenMetrics 1.0 / Prometheus text document.
pub fn parse(alloc: std.mem.Allocator, text: []const u8): MetricsDocument! {
    return MetricsDocument.parse(alloc, text);
}

// ============================================================================
// 3. Internal Parsing & SIMD-Accelerated Escaping Helpers
// ============================================================================

fn omMetricTypeStr(mt: MetricType): []const u8 {
    return switch (mt) {
        MetricType.Counter => "counter",
        MetricType.Gauge => "gauge",
        MetricType.Histogram => "histogram",
        MetricType.Summary => "summary",
        MetricType.Untyped => "untyped",
    };
}

struct OmTokenSplit {
    token: []const u8,
    rest: []const u8,
}

fn omSkipSpaces(s: []const u8): []const u8 {
    let i: usize = 0;
    while (i < s.len and (s[i] == ' ' or s[i] == '\t')) {
        i += 1;
    }
    return s[i..];
}

fn omSplitFirstToken(s: []const u8): OmTokenSplit {
    let i: usize = 0;
    while (i < s.len and s[i] != ' ' and s[i] != '\t') {
        i += 1;
    }
    return OmTokenSplit{
        .token = s[0..i],
        .rest = s[i..],
    };
}

fn omGetOrCreateFamily(
    alloc: std.mem.Allocator,
    builders: *std.ArrayList(OmFamilyBuilder),
    name: []const u8,
): usize! {
    let i: usize = 0;
    while (i < builders.items.len) {
        if (std.mem.eql(u8, builders.items[i].name, name)) {
            return i;
        }
        i += 1;
    }
    const name_copy = try alloc.dupe(u8, name);
    errdefer alloc.free(name_copy);

    try builders.append(alloc, OmFamilyBuilder{
        .name = name_copy,
        .help = null,
        .unit = null,
        .metric_type = MetricType.Untyped,
        .samples = std.ArrayList(Sample).empty,
    });
    return builders.items.len - 1;
}

fn omIsFamilySampleMatch(fam_name: []const u8, sample_name: []const u8): bool {
    if (std.mem.eql(u8, fam_name, sample_name)) return true;
    if (!std.mem.startsWith(u8, sample_name, fam_name)) return false;
    const suffix = sample_name[fam_name.len..];
    return std.mem.eql(u8, suffix, "_bucket") or
        std.mem.eql(u8, suffix, "_sum") or
        std.mem.eql(u8, suffix, "_count") or
        std.mem.eql(u8, suffix, "_total") or
        std.mem.eql(u8, suffix, "_created");
}

fn omResolveFamilyForSample(
    alloc: std.mem.Allocator,
    builders: *std.ArrayList(OmFamilyBuilder),
    sample_name: []const u8,
): usize! {
    let i: usize = builders.items.len;
    while (i > 0) {
        i -= 1;
        if (std.mem.eql(u8, builders.items[i].name, sample_name)) {
            return i;
        }
    }
    i = builders.items.len;
    while (i > 0) {
        i -= 1;
        if (omIsFamilySampleMatch(builders.items[i].name, sample_name)) {
            return i;
        }
    }
    return try omGetOrCreateFamily(alloc, builders, sample_name);
}

fn omUnescapeHelp(alloc: std.mem.Allocator, raw: []const u8): []u8! {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    let i: usize = 0;
    while (i < raw.len) {
        if (raw[i] == '\\' and i + 1 < raw.len) {
            const nxt = raw[i + 1];
            if (nxt == '\\') {
                try out.append(alloc, '\\');
                i += 2;
                continue;
            }
            if (nxt == 'n') {
                try out.append(alloc, '\n');
                i += 2;
                continue;
            }
        }
        try out.append(alloc, raw[i]);
        i += 1;
    }
    return try out.toOwnedSlice(alloc);
}

fn omAppendEscapedHelp(alloc: std.mem.Allocator, out: *std.ArrayList(u8), text: []const u8)! {
    let i: usize = 0;
    while (i < text.len) {
        const c = text[i];
        if (c == '\\') {
            try out.appendSlice(alloc, "\\\\");
        } else if (c == '\n') {
            try out.appendSlice(alloc, "\\n");
        } else if (c == '\r' or (c < 0x20 and c != '\t') or c == 0x7F) {
            return error.InvalidControlChar;
        } else {
            try out.append(alloc, c);
        }
        i += 1;
    }
}

/// 16-lane SIMD accelerated label value serializer escaping `\\`, `\n`, `\"` and rejecting
/// illegal raw control bytes (such as `\r`, `\0`, `0x7F`) to prevent CRLF injection.
fn omAppendEscapedLabelValue(alloc: std.mem.Allocator, out: *std.ArrayList(u8), text: []const u8)! {
    let pos: usize = 0;
    while (pos < text.len) {
        const rem = text[pos..];
        if (rel := simdOmFindNeedEscape(rem)) {
            if (rel > 0) {
                try out.appendSlice(alloc, rem[0..rel]);
            }
            const c = rem[rel];
            if (c == '\\') {
                try out.appendSlice(alloc, "\\\\");
            } else if (c == '\n') {
                try out.appendSlice(alloc, "\\n");
            } else if (c == '"') {
                try out.appendSlice(alloc, "\\\"");
            } else if (c == '\t') {
                try out.append(alloc, '\t');
            } else {
                return error.InvalidControlChar;
            }
            pos += rel + 1;
        } else {
            try out.appendSlice(alloc, rem);
            break;
        }
    }
}

fn omAppendFloatValue(alloc: std.mem.Allocator, out: *std.ArrayList(u8), val: f64)! {
    if (std.math.isNan(val)) {
        try out.appendSlice(alloc, "NaN");
        return;
    }
    if (std.math.isPositiveInf(val)) {
        try out.appendSlice(alloc, "+Inf");
        return;
    }
    if (std.math.isNegativeInf(val)) {
        try out.appendSlice(alloc, "-Inf");
        return;
    }
    const formatted = try std.fmt.allocPrint(alloc, "{d}", .{val});
    defer alloc.free(formatted);
    try out.appendSlice(alloc, formatted);
}

fn omParseMetricFloat(raw: []const u8): f64! {
    if (std.mem.eql(u8, raw, "NaN") or std.mem.eql(u8, raw, "nan") or std.mem.eql(u8, raw, "NAN")) {
        return std.math.nan(f64);
    }
    if (std.mem.eql(u8, raw, "+Inf") or std.mem.eql(u8, raw, "Inf") or std.mem.eql(u8, raw, "+inf") or std.mem.eql(u8, raw, "inf")) {
        return std.math.inf(f64);
    }
    if (std.mem.eql(u8, raw, "-Inf") or std.mem.eql(u8, raw, "-inf")) {
        return -std.math.inf(f64);
    }
    return std.fmt.parseFloat(f64, raw);
}

fn omParseTimestampMs(raw: []const u8): i64! {
    if (_ := std.mem.indexOfScalar(u8, raw, '.')) {
        const sec = try std.fmt.parseFloat(f64, raw);
        if (std.math.isNan(sec) or std.math.isInf(sec)) {
            return error.InvalidTimestamp;
        }
        return @as(i64, @intFromFloat(@round(sec * 1000.0)));
    }
    return try std.fmt.parseInt(i64, raw, 10);
}

fn omParseSampleLine(alloc: std.mem.Allocator, line: []const u8): Sample! {
    let i: usize = 0;
    while (i < line.len and line[i] != '{' and line[i] != ' ' and line[i] != '\t') {
        i += 1;
    }
    if (i == 0) {
        return error.InvalidMetricName;
    }
    const raw_name = line[0..i];
    if (!isValidMetricName(raw_name)) {
        return error.InvalidMetricName;
    }
    const name_copy = try alloc.dupe(u8, raw_name);
    errdefer alloc.free(name_copy);

    let labels = std.ArrayList(Label).empty;
    errdefer {
        let li: usize = 0;
        while (li < labels.items.len) {
            labels.items[li].deinit(alloc);
            li += 1;
        }
        labels.deinit(alloc);
    }

    if (i < line.len and line[i] == '{') {
        i += 1;
        while (true) {
            while (i < line.len and (line[i] == ' ' or line[i] == '\t')) {
                i += 1;
            }
            if (i >= line.len) {
                return error.UnterminatedLabels;
            }
            if (line[i] == '}') {
                i += 1;
                break;
            }

            const lname_start = i;
            while (i < line.len and line[i] != '=' and line[i] != ' ' and line[i] != '\t') {
                i += 1;
            }
            if (i == lname_start or i >= line.len) {
                return error.InvalidLabelName;
            }
            const lname = line[lname_start..i];
            if (!isValidLabelName(lname)) {
                return error.InvalidLabelName;
            }
            while (i < line.len and (line[i] == ' ' or line[i] == '\t')) {
                i += 1;
            }
            if (i >= line.len or line[i] != '=') {
                return error.ExpectedEqualsInLabel;
            }
            i += 1;
            while (i < line.len and (line[i] == ' ' or line[i] == '\t')) {
                i += 1;
            }
            if (i >= line.len or line[i] != '"') {
                return error.ExpectedQuoteInLabel;
            }
            i += 1;

            let lval_buf = std.ArrayList(u8).empty;
            errdefer lval_buf.deinit(alloc);

            let closed = false;
            while (i < line.len) {
                const rem = line[i..];
                if (rel := simdOmFindNeedEscape(rem)) {
                    if (rel > 0) {
                        try lval_buf.appendSlice(alloc, rem[0..rel]);
                    }
                    i += rel;
                    const c = line[i];
                    if (c == '"') {
                        i += 1;
                        closed = true;
                        break;
                    }
                    if (c == '\\') {
                        if (i + 1 >= line.len) {
                            return error.InvalidEscapeSequence;
                        }
                        const nxt = line[i + 1];
                        if (nxt == '\\') {
                            try lval_buf.append(alloc, '\\');
                        } else if (nxt == 'n') {
                            try lval_buf.append(alloc, '\n');
                        } else if (nxt == '"') {
                            try lval_buf.append(alloc, '"');
                        } else {
                            return error.InvalidEscapeSequence;
                        }
                        i += 2;
                        continue;
                    }
                    if (c == '\t') {
                        try lval_buf.append(alloc, '\t');
                        i += 1;
                        continue;
                    }
                    // Reject unescaped control bytes (< 0x20 including \r/\n/\0) and 0x7F.
                    return error.InvalidControlChar;
                } else {
                    try lval_buf.appendSlice(alloc, rem);
                    i = line.len;
                    break;
                }
            }
            if (!closed) {
                return error.UnterminatedLabelValue;
            }

            const lname_copy = try alloc.dupe(u8, lname);
            errdefer alloc.free(lname_copy);
            const lval_copy = try lval_buf.toOwnedSlice(alloc);
            errdefer alloc.free(lval_copy);

            try labels.append(alloc, Label{
                .name = lname_copy,
                .value = lval_copy,
            });

            while (i < line.len and (line[i] == ' ' or line[i] == '\t')) {
                i += 1;
            }
            if (i < line.len and line[i] == ',') {
                i += 1;
                continue;
            }
            if (i < line.len and line[i] == '}') {
                i += 1;
                break;
            }
            return error.ExpectedCommaOrBrace;
        }
    }

    const after_labels = omSkipSpaces(line[i..]);
    if (after_labels.len == 0) {
        return error.MissingMetricValue;
    }

    const val_split = omSplitFirstToken(after_labels);
    const val = try omParseMetricFloat(val_split.token);

    let ts_ms: i64? = null;
    const after_val = omSkipSpaces(val_split.rest);
    if (after_val.len > 0 and after_val[0] != '#') {
        const ts_split = omSplitFirstToken(after_val);
        ts_ms = try omParseTimestampMs(ts_split.token);
    }

    return Sample{
        .name = name_copy,
        .labels = try labels.toOwnedSlice(alloc),
        .value = val,
        .timestamp_ms = ts_ms,
    };
}
