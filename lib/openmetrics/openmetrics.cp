// Source: native-cp-stdlib (OpenMetrics / Prometheus Text Format)

const std = @import("std");

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

    pub fn fromString(raw: []const u8): MetricType? {
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
    help: []const u8?,
    unit: []const u8?,
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

struct FamilyBuilder {
    name: []const u8,
    help: []const u8?,
    unit: []const u8?,
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

    pub fn parse(alloc: std.mem.Allocator, text: []const u8): MetricsDocument! {
        let builders = std.ArrayList(FamilyBuilder).empty;
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
                    const rest = skipSpaces(after_hash[4..]);
                    const split = splitFirstToken(rest);
                    if (split.token.len == 0) {
                        return error.InvalidHelpLine;
                    }
                    const fam_idx = try getOrCreateFamily(alloc, &builders, split.token);
                    const unescaped = try unescapeHelp(alloc, skipSpaces(split.rest));
                    if (old_help := builders.items[fam_idx].help) {
                        alloc.free(old_help);
                    }
                    builders.items[fam_idx].help = unescaped;
                    continue;
                }
                if (std.mem.startsWith(u8, after_hash, "TYPE")) {
                    const rest = skipSpaces(after_hash[4..]);
                    const split1 = splitFirstToken(rest);
                    const split2 = splitFirstToken(skipSpaces(split1.rest));
                    if (split1.token.len == 0 or split2.token.len == 0) {
                        return error.InvalidTypeLine;
                    }
                    const mtype = MetricType.fromString(split2.token) orelse return error.InvalidMetricType;
                    const fam_idx = try getOrCreateFamily(alloc, &builders, split1.token);
                    builders.items[fam_idx].metric_type = mtype;
                    continue;
                }
                if (std.mem.startsWith(u8, after_hash, "UNIT")) {
                    const rest = skipSpaces(after_hash[4..]);
                    const split1 = splitFirstToken(rest);
                    const split2 = splitFirstToken(skipSpaces(split1.rest));
                    if (split1.token.len == 0) {
                        return error.InvalidUnitLine;
                    }
                    const fam_idx = try getOrCreateFamily(alloc, &builders, split1.token);
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

            let sample = try parseSampleLine(alloc, line);
            errdefer sample.deinit(alloc);

            const fam_idx = try resolveFamilyForSample(alloc, &builders, sample.name);
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
                try appendEscapedHelp(alloc, &out, h);
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
            try out.appendSlice(alloc, fam.metric_type.toString());
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
                        try appendEscapedLabelValue(alloc, &out, s.labels[li].value);
                        try out.append(alloc, '"');
                        li += 1;
                    }
                    try out.append(alloc, '}');
                }
                try out.append(alloc, ' ');
                try appendFloatValue(alloc, &out, s.value);
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

struct TokenSplit {
    token: []const u8,
    rest: []const u8,
}

fn skipSpaces(s: []const u8): []const u8 {
    let i: usize = 0;
    while (i < s.len and (s[i] == ' ' or s[i] == '\t')) {
        i += 1;
    }
    return s[i..];
}

fn splitFirstToken(s: []const u8): TokenSplit {
    let i: usize = 0;
    while (i < s.len and s[i] != ' ' and s[i] != '\t') {
        i += 1;
    }
    return TokenSplit{
        .token = s[0..i],
        .rest = s[i..],
    };
}

fn getOrCreateFamily(
    alloc: std.mem.Allocator,
    builders: *std.ArrayList(FamilyBuilder),
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

    try builders.append(alloc, FamilyBuilder{
        .name = name_copy,
        .help = null,
        .unit = null,
        .metric_type = MetricType.Untyped,
        .samples = std.ArrayList(Sample).empty,
    });
    return builders.items.len - 1;
}

fn isFamilySampleMatch(fam_name: []const u8, sample_name: []const u8): bool {
    if (std.mem.eql(u8, fam_name, sample_name)) return true;
    if (!std.mem.startsWith(u8, sample_name, fam_name)) return false;
    const suffix = sample_name[fam_name.len..];
    return std.mem.eql(u8, suffix, "_bucket") or
        std.mem.eql(u8, suffix, "_sum") or
        std.mem.eql(u8, suffix, "_count") or
        std.mem.eql(u8, suffix, "_total") or
        std.mem.eql(u8, suffix, "_created");
}

fn resolveFamilyForSample(
    alloc: std.mem.Allocator,
    builders: *std.ArrayList(FamilyBuilder),
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
        if (isFamilySampleMatch(builders.items[i].name, sample_name)) {
            return i;
        }
    }
    return try getOrCreateFamily(alloc, builders, sample_name);
}

fn unescapeHelp(alloc: std.mem.Allocator, raw: []const u8): []u8! {
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

fn appendEscapedHelp(alloc: std.mem.Allocator, out: *std.ArrayList(u8), text: []const u8)! {
    let i: usize = 0;
    while (i < text.len) {
        const c = text[i];
        if (c == '\\') {
            try out.appendSlice(alloc, "\\\\");
        } else if (c == '\n') {
            try out.appendSlice(alloc, "\\n");
        } else {
            try out.append(alloc, c);
        }
        i += 1;
    }
}

fn appendEscapedLabelValue(alloc: std.mem.Allocator, out: *std.ArrayList(u8), text: []const u8)! {
    let i: usize = 0;
    while (i < text.len) {
        const c = text[i];
        if (c == '\\') {
            try out.appendSlice(alloc, "\\\\");
        } else if (c == '\n') {
            try out.appendSlice(alloc, "\\n");
        } else if (c == '"') {
            try out.appendSlice(alloc, "\\\"");
        } else {
            try out.append(alloc, c);
        }
        i += 1;
    }
}

fn appendFloatValue(alloc: std.mem.Allocator, out: *std.ArrayList(u8), val: f64)! {
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

fn parseMetricFloat(raw: []const u8): f64! {
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

fn parseTimestampMs(raw: []const u8): i64! {
    if (_ := std.mem.indexOfScalar(u8, raw, '.')) {
        const sec = try std.fmt.parseFloat(f64, raw);
        return @as(i64, @intFromFloat(@round(sec * 1000.0)));
    }
    return try std.fmt.parseInt(i64, raw, 10);
}

fn parseSampleLine(alloc: std.mem.Allocator, line: []const u8): Sample! {
    let i: usize = 0;
    while (i < line.len and line[i] != '{' and line[i] != ' ' and line[i] != '\t') {
        i += 1;
    }
    if (i == 0) {
        return error.InvalidMetricName;
    }
    const raw_name = line[0..i];
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
                try lval_buf.append(alloc, c);
                i += 1;
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

    const after_labels = skipSpaces(line[i..]);
    if (after_labels.len == 0) {
        return error.MissingMetricValue;
    }

    const val_split = splitFirstToken(after_labels);
    const val = try parseMetricFloat(val_split.token);

    let ts_ms: i64? = null;
    const after_val = skipSpaces(val_split.rest);
    if (after_val.len > 0 and after_val[0] != '#') {
        const ts_split = splitFirstToken(after_val);
        ts_ms = try parseTimestampMs(ts_split.token);
    }

    return Sample{
        .name = name_copy,
        .labels = try labels.toOwnedSlice(alloc),
        .value = val,
        .timestamp_ms = ts_ms,
    };
}
