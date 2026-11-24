// ============================================================================
// cap:zon — Zero-Dependency ZON (Zig Object Notation) Parser, Validator & Formatter
// ============================================================================

const std = @import("std");

pub err ZonError {
    UnexpectedEof,
    UnexpectedToken,
    InvalidNumber,
    InvalidString,
    InvalidEscape,
    InvalidChar,
    MixedContainer,
    MaxDepthExceeded,
    TrailingTokens,
}

pub const DEFAULT_MAX_DEPTH: usize = 64;

pub enum ZonTag {
    Null,
    Bool,
    Int,
    Float,
    String,
    EnumLit,
    Struct,
    Array,
}

pub struct ZonField {
    key: []const u8,
    value: ZonValue,
}

pub struct ZonValue {
    tag: ZonTag,
    bool_val: bool = false,
    int_val: i64 = 0,
    float_val: f64 = 0.0,
    str_val: []const u8 = "",
    fields: []const ZonField = &.{},
    items: []const ZonValue = &.{},

    pub fn isNull(): bool {
        return _.tag == .Null;
    }

    pub fn asBool(): bool? {
        if (_.tag != .Bool) return null;
        return _.bool_val;
    }

    pub fn asInt(): i64? {
        if (_.tag != .Int) return null;
        return _.int_val;
    }

    pub fn asFloat(): f64? {
        if (_.tag == .Float) return _.float_val;
        if (_.tag == .Int) return @as(f64, @floatFromInt(_.int_val));
        return null;
    }

    pub fn asString(): ([]const u8)? {
        if (_.tag != .String) return null;
        return _.str_val;
    }

    pub fn asEnum(): ([]const u8)? {
        if (_.tag != .EnumLit) return null;
        return _.str_val;
    }

    pub fn len(): usize {
        if (_.tag == .Struct) return _.fields.len;
        if (_.tag == .Array) return _.items.len;
        if (_.tag == .String || _.tag == .EnumLit) return _.str_val.len;
        return 0;
    }

    pub fn get(key: []const u8): ZonValue? {
        if (_.tag != .Struct) return null;
        let i: usize = 0;
        while (i < _.fields.len) : (i += 1) {
            if (std.mem.eql(u8, _.fields[i].key, key)) {
                return _.fields[i].value;
            }
        }
        return null;
    }

    pub fn at(index: usize): ZonValue? {
        if (_.tag != .Array) return null;
        if (index >= _.items.len) return null;
        return _.items[index];
    }
}

/// Arena-backed parsed ZON document with deterministic idempotent `~init()` RAII cleanup.
pub struct ZonDocument {
    arena: std.heap.ArenaAllocator,
    root: ZonValue,
    active: bool,

    pub static fn parse(allocator: std.mem.Allocator, src: []const u8): ZonDocument! {
        let arena = std.heap.ArenaAllocator.init(allocator);
        errdefer arena.deinit();
        let parser = Parser{
            alloc: arena.allocator(),
            src: src,
            pos: 0,
            depth: 0,
        };
        const root = try parser.parseRoot();
        return ZonDocument{
            arena: arena,
            root: root,
            active: true,
        };
    }

    ~init() {
        if (_.active) {
            _.arena.deinit();
            _.active = false;
        }
    }

    pub fn deinit(*_) {
        _.~init();
    }

    pub fn get(key: []const u8): ZonValue? {
        return _.root.get(key);
    }
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isIdentCont(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

fn hexDigit(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

struct Parser {
    alloc: std.mem.Allocator,
    src: []const u8,
    pos: usize,
    depth: usize,

    fn skipWsAndComments(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                _.pos += 1;
            } else if (c == '/' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '/') {
                _.pos += 2;
                while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                    _.pos += 1;
                }
            } else {
                break;
            }
        }
    }

    fn parseRoot(*_): ZonValue! {
        _.skipWsAndComments();
        if (_.pos >= _.src.len) return error.UnexpectedEof;
        const val = try _.parseValue();
        _.skipWsAndComments();
        if (_.pos != _.src.len) return error.TrailingTokens;
        return val;
    }

    fn parseValue(*_): ZonValue! {
        _.skipWsAndComments();
        if (_.pos >= _.src.len) return error.UnexpectedEof;
        const c = _.src[_.pos];
        if (c == '.') {
            if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '{') {
                return _.parseContainer();
            }
            return _.parseEnumLit();
        }
        if (c == '"') return _.parseString();
        if (c == '\\' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '\\') {
            return _.parseMultilineString();
        }
        if (c == '\'') return _.parseCharLit();
        if (c == '-' || c == '+' || (c >= '0' && c <= '9')) {
            return _.parseNumber();
        }
        if (isIdentStart(c)) {
            return _.parseKeyword();
        }
        return error.UnexpectedToken;
    }

    fn parseKeyword(*_): ZonValue! {
        const start = _.pos;
        while (_.pos < _.src.len && isIdentCont(_.src[_.pos])) {
            _.pos += 1;
        }
        const word = _.src[start .. _.pos];
        if (std.mem.eql(u8, word, "true")) {
            return ZonValue{ tag: .Bool, bool_val: true };
        }
        if (std.mem.eql(u8, word, "false")) {
            return ZonValue{ tag: .Bool, bool_val: false };
        }
        if (std.mem.eql(u8, word, "null")) {
            return ZonValue{ tag: .Null };
        }
        if (std.mem.eql(u8, word, "nan")) {
            return ZonValue{ tag: .Float, float_val: std.math.nan(f64) };
        }
        if (std.mem.eql(u8, word, "inf")) {
            return ZonValue{ tag: .Float, float_val: std.math.inf(f64) };
        }
        return error.UnexpectedToken;
    }

    fn parseIdentName(*_): ([]const u8)! {
        if (_.pos >= _.src.len) return error.UnexpectedEof;
        if (_.src[_.pos] == '@' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '"') {
            _.pos += 1;
            const s = try _.parseString();
            return s.str_val;
        }
        if (!isIdentStart(_.src[_.pos])) return error.UnexpectedToken;
        const start = _.pos;
        while (_.pos < _.src.len && isIdentCont(_.src[_.pos])) {
            _.pos += 1;
        }
        return _.src[start .. _.pos];
    }

    fn parseEnumLit(*_): ZonValue! {
        _.pos += 1;
        const name = try _.parseIdentName();
        return ZonValue{ tag: .EnumLit, str_val: name };
    }

    fn parseString(*_): ZonValue! {
        _.pos += 1;
        let out = std.ArrayList(u8).empty;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '"') {
                _.pos += 1;
                return ZonValue{ tag: .String, str_val: out.items };
            }
            if (c == '\n' || c == '\r') return error.InvalidString;
            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= _.src.len) return error.InvalidEscape;
                const esc = _.src[_.pos];
                _.pos += 1;
                if (esc == 'n') {
                    try out.append(_.alloc, '\n');
                } else if (esc == 'r') {
                    try out.append(_.alloc, '\r');
                } else if (esc == 't') {
                    try out.append(_.alloc, '\t');
                } else if (esc == '\\') {
                    try out.append(_.alloc, '\\');
                } else if (esc == '"') {
                    try out.append(_.alloc, '"');
                } else if (esc == '\'') {
                    try out.append(_.alloc, '\'');
                } else if (esc == 'x') {
                    if (_.pos + 2 > _.src.len) return error.InvalidEscape;
                    const hi = hexDigit(_.src[_.pos]) orelse return error.InvalidEscape;
                    const lo = hexDigit(_.src[_.pos + 1]) orelse return error.InvalidEscape;
                    _.pos += 2;
                    try out.append(_.alloc, (hi << 4) | lo);
                } else {
                    return error.InvalidEscape;
                }
            } else {
                try out.append(_.alloc, c);
                _.pos += 1;
            }
        }
        return error.InvalidString;
    }

    fn parseMultilineString(*_): ZonValue! {
        let out = std.ArrayList(u8).empty;
        let first_line = true;
        while (_.pos + 1 < _.src.len && _.src[_.pos] == '\\' && _.src[_.pos + 1] == '\\') {
            if (!first_line) {
                try out.append(_.alloc, '\n');
            }
            first_line = false;
            _.pos += 2;
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                _.pos += 1;
            }
            let end = _.pos;
            if (end > start && _.src[end - 1] == '\r') {
                end -= 1;
            }
            try out.appendSlice(_.alloc, _.src[start .. end]);
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            while (_.pos < _.src.len && (_.src[_.pos] == ' ' || _.src[_.pos] == '\t')) {
                _.pos += 1;
            }
        }
        return ZonValue{ tag: .String, str_val: out.items };
    }

    fn parseCharLit(*_): ZonValue! {
        _.pos += 1;
        if (_.pos >= _.src.len) return error.InvalidChar;
        let ch: u8 = 0;
        if (_.src[_.pos] == '\\') {
            _.pos += 1;
            if (_.pos >= _.src.len) return error.InvalidChar;
            const esc = _.src[_.pos];
            _.pos += 1;
            if (esc == 'n') ch = '\n'
            else if (esc == 'r') ch = '\r'
            else if (esc == 't') ch = '\t'
            else if (esc == '\\') ch = '\\'
            else if (esc == '\'') ch = '\''
            else if (esc == '"') ch = '"'
            else return error.InvalidEscape;
        } else {
            ch = _.src[_.pos];
            _.pos += 1;
        }
        if (_.pos >= _.src.len || _.src[_.pos] != '\'') return error.InvalidChar;
        _.pos += 1;
        return ZonValue{ tag: .Int, int_val: @as(i64, ch) };
    }

    fn parseNumber(*_): ZonValue! {
        const start = _.pos;
        if (_.src[_.pos] == '-' || _.src[_.pos] == '+') {
            _.pos += 1;
        }
        if (_.pos < _.src.len && isIdentStart(_.src[_.pos])) {
            const kw_start = _.pos;
            while (_.pos < _.src.len && isIdentCont(_.src[_.pos])) {
                _.pos += 1;
            }
            const kw = _.src[kw_start .. _.pos];
            if (std.mem.eql(u8, kw, "inf")) {
                const neg = _.src[start] == '-';
                return ZonValue{ tag: .Float, float_val: if (neg) -std.math.inf(f64) else std.math.inf(f64) };
            }
            return error.InvalidNumber;
        }
        let is_float = false;
        let is_hex = false;
        if (_.pos + 1 < _.src.len && _.src[_.pos] == '0' && (_.src[_.pos + 1] == 'x' || _.src[_.pos + 1] == 'X')) {
            is_hex = true;
            _.pos += 2;
        }
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if ((c >= '0' && c <= '9') || c == '_' || (is_hex && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))) {
                _.pos += 1;
            } else if (!is_hex && (c == 'b' || c == 'B' || c == 'o' || c == 'O')) {
                _.pos += 1;
            } else if (!is_hex && c == '.') {
                is_float = true;
                _.pos += 1;
            } else if (!is_hex && (c == 'e' || c == 'E')) {
                is_float = true;
                _.pos += 1;
                if (_.pos < _.src.len && (_.src[_.pos] == '+' || _.src[_.pos] == '-')) {
                    _.pos += 1;
                }
            } else {
                break;
            }
        }
        const slice = _.src[start .. _.pos];
        if (is_float) {
            const fval = std.fmt.parseFloat(f64, slice) catch return error.InvalidNumber;
            return ZonValue{ tag: .Float, float_val: fval };
        }
        const ival = std.fmt.parseInt(i64, slice, 0) catch return error.InvalidNumber;
        return ZonValue{ tag: .Int, int_val: ival };
    }

    fn isFieldStart(): bool {
        if (_.pos >= _.src.len || _.src[_.pos] != '.') return false;
        let p = _.pos + 1;
        if (p >= _.src.len) return false;
        if (_.src[p] == '{') return false;
        if (_.src[p] == '@' && p + 1 < _.src.len && _.src[p + 1] == '"') {
            p += 2;
            while (p < _.src.len && _.src[p] != '"') {
                if (_.src[p] == '\\') p += 1;
                p += 1;
            }
            if (p >= _.src.len) return false;
            p += 1;
        } else if (isIdentStart(_.src[p])) {
            while (p < _.src.len && isIdentCont(_.src[p])) {
                p += 1;
            }
        } else {
            return false;
        }
        while (p < _.src.len && (_.src[p] == ' ' || _.src[p] == '\t' || _.src[p] == '\r' || _.src[p] == '\n')) {
            p += 1;
        }
        return p < _.src.len && _.src[p] == '=';
    }

    fn parseContainer(*_): ZonValue! {
        if (_.depth >= DEFAULT_MAX_DEPTH) return error.MaxDepthExceeded;
        _.depth += 1;
        defer _.depth -= 1;

        _.pos += 2; // skip ".{"
        _.skipWsAndComments();
        if (_.pos < _.src.len && _.src[_.pos] == '}') {
            _.pos += 1;
            return ZonValue{ tag: .Struct };
        }

        if (_.isFieldStart()) {
            let fields = std.ArrayList(ZonField).empty;
            while (_.pos < _.src.len) {
                _.skipWsAndComments();
                if (_.pos < _.src.len && _.src[_.pos] == '}') {
                    _.pos += 1;
                    return ZonValue{ tag: .Struct, fields: fields.items };
                }
                if (!_.isFieldStart()) return error.MixedContainer;
                _.pos += 1; // skip '.'
                const key = try _.parseIdentName();
                _.skipWsAndComments();
                if (_.pos >= _.src.len || _.src[_.pos] != '=') return error.UnexpectedToken;
                _.pos += 1;
                const val = try _.parseValue();
                try fields.append(_.alloc, ZonField{ key: key, value: val });
                _.skipWsAndComments();
                if (_.pos < _.src.len && _.src[_.pos] == ',') {
                    _.pos += 1;
                } else if (_.pos < _.src.len && _.src[_.pos] == '}') {
                    _.pos += 1;
                    return ZonValue{ tag: .Struct, fields: fields.items };
                } else {
                    return error.UnexpectedToken;
                }
            }
            return error.UnexpectedEof;
        } else {
            let items = std.ArrayList(ZonValue).empty;
            while (_.pos < _.src.len) {
                _.skipWsAndComments();
                if (_.pos < _.src.len && _.src[_.pos] == '}') {
                    _.pos += 1;
                    return ZonValue{ tag: .Array, items: items.items };
                }
                if (_.isFieldStart()) return error.MixedContainer;
                const val = try _.parseValue();
                try items.append(_.alloc, val);
                _.skipWsAndComments();
                if (_.pos < _.src.len && _.src[_.pos] == ',') {
                    _.pos += 1;
                } else if (_.pos < _.src.len && _.src[_.pos] == '}') {
                    _.pos += 1;
                    return ZonValue{ tag: .Array, items: items.items };
                } else {
                    return error.UnexpectedToken;
                }
            }
            return error.UnexpectedEof;
        }
    }
}

fn writeIndent(out: *std.ArrayList(u8), alloc: std.mem.Allocator, spaces: usize): void! {
    let i: usize = 0;
    while (i < spaces) : (i += 1) {
        try out.append(alloc, ' ');
    }
}

fn writeEscapedString(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): void! {
    try out.append(alloc, '"');
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '"') {
            try out.appendSlice(alloc, "\\\"");
        } else if (c == '\\') {
            try out.appendSlice(alloc, "\\\\");
        } else if (c == '\n') {
            try out.appendSlice(alloc, "\\n");
        } else if (c == '\r') {
            try out.appendSlice(alloc, "\\r");
        } else if (c == '\t') {
            try out.appendSlice(alloc, "\\t");
        } else {
            try out.append(alloc, c);
        }
    }
    try out.append(alloc, '"');
}

fn isSimpleIdent(s: []const u8): bool {
    if (s.len == 0 || !isIdentStart(s[0])) return false;
    let i: usize = 1;
    while (i < s.len) : (i += 1) {
        if (!isIdentCont(s[i])) return false;
    }
    return true;
}

fn writeValue(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: ZonValue, indent: usize, level: usize): void! {
    if (val.tag == .Null) {
        try out.appendSlice(alloc, "null");
        return;
    }
    if (val.tag == .Bool) {
        try out.appendSlice(alloc, if (val.bool_val) "true" else "false");
        return;
    }
    if (val.tag == .Int) {
        let buf: [32]u8 = undefined;
        const s = std.fmt.bufPrint(&buf, "{d}", .{val.int_val}) catch "";
        try out.appendSlice(alloc, s);
        return;
    }
    if (val.tag == .Float) {
        let buf: [64]u8 = undefined;
        const s = std.fmt.bufPrint(&buf, "{d}", .{val.float_val}) catch "";
        try out.appendSlice(alloc, s);
        return;
    }
    if (val.tag == .String) {
        try writeEscapedString(out, alloc, val.str_val);
        return;
    }
    if (val.tag == .EnumLit) {
        try out.append(alloc, '.');
        if (isSimpleIdent(val.str_val)) {
            try out.appendSlice(alloc, val.str_val);
        } else {
            try out.append(alloc, '@');
            try writeEscapedString(out, alloc, val.str_val);
        }
        return;
    }
    if (val.tag == .Struct) {
        if (val.fields.len == 0) {
            try out.appendSlice(alloc, ".{}");
            return;
        }
        try out.appendSlice(alloc, ".{");
        if (indent > 0) try out.append(alloc, '\n') else try out.append(alloc, ' ');
        let i: usize = 0;
        while (i < val.fields.len) : (i += 1) {
            if (indent > 0) try writeIndent(out, alloc, (level + 1) * indent);
            try out.append(alloc, '.');
            if (isSimpleIdent(val.fields[i].key)) {
                try out.appendSlice(alloc, val.fields[i].key);
            } else {
                try out.append(alloc, '@');
                try writeEscapedString(out, alloc, val.fields[i].key);
            }
            try out.appendSlice(alloc, " = ");
            try writeValue(out, alloc, val.fields[i].value, indent, level + 1);
            if (indent > 0) {
                try out.appendSlice(alloc, ",\n");
            } else if (i + 1 < val.fields.len) {
                try out.appendSlice(alloc, ", ");
            }
        }
        if (indent > 0) try writeIndent(out, alloc, level * indent) else try out.append(alloc, ' ');
        try out.append(alloc, '}');
        return;
    }
    if (val.tag == .Array) {
        if (val.items.len == 0) {
            try out.appendSlice(alloc, ".{}");
            return;
        }
        try out.appendSlice(alloc, ".{");
        if (indent > 0) try out.append(alloc, '\n') else try out.append(alloc, ' ');
        let i: usize = 0;
        while (i < val.items.len) : (i += 1) {
            if (indent > 0) try writeIndent(out, alloc, (level + 1) * indent);
            try writeValue(out, alloc, val.items[i], indent, level + 1);
            if (indent > 0) {
                try out.appendSlice(alloc, ",\n");
            } else if (i + 1 < val.items.len) {
                try out.appendSlice(alloc, ", ");
            }
        }
        if (indent > 0) try writeIndent(out, alloc, level * indent) else try out.append(alloc, ' ');
        try out.append(alloc, '}');
        return;
    }
}

/// Parses ZON source text into an arena-backed `ZonDocument`.
pub fn parseZon(allocator: std.mem.Allocator, src: []const u8): ZonDocument! {
    return ZonDocument.parse(allocator, src);
}

/// Validates whether `src` is syntactically well-formed ZON.
pub fn validateZon(src: []const u8): bool {
    let doc = ZonDocument.parse(std.heap.page_allocator, src) catch return false;
    doc.deinit();
    return true;
}

/// Formats a `ZonValue` into ZON text (`indent == 0` for compact, `indent > 0` for pretty).
pub fn formatZon(allocator: std.mem.Allocator, val: ZonValue, indent: usize): ([]u8)! {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(allocator);
    try writeValue(&out, allocator, val, indent, 0);
    return out.toOwnedSlice(allocator);
}

/// Formats a `ZonValue` into single-line compact ZON text.
pub fn formatZonCompact(allocator: std.mem.Allocator, val: ZonValue): ([]u8)! {
    return formatZon(allocator, val, 0);
}
