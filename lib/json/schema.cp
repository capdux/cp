// Origin: Pure-CP Stdlib (cap:json/schema Zero-Heap JSON Schema Validator, exact-mirror & OpenAPI 3.1.0 Emitter) | Created: 2026-11-25
// ============================================================================
// Pure-CP zero-heap JSON Schema validation & transformation (`cap:json/schema`):
// - Zero-reflection ObjectSchema + validateJsonObject (including recursive nested Object & Array validation)
// - Elysia exact-mirror compatible strict field pruning (exactMirrorJson)
// - OpenAPI 3.1.0 / JSON Schema Draft 2020-12 emitter (schemaToOpenApiJson)
// ============================================================================

pub const SCHEMA_MAX_FIELDS: usize = 32;
pub const SCHEMA_MAX_ISSUES: usize = 16;
pub const SCHEMA_MAX_JSON_DEPTH: usize = 32;

fn hexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

// ============================================================================
// 2. Zero-Reflection Schema Types & Builder
// ============================================================================

pub enum FieldKind {
    Str,
    Int,
    Float,
    Bool,
    Email,
    Uuid,
    Url,
    Enum,
    Object,
    Array,
}

pub struct FieldRule {
    name: string = "",
    kind: FieldKind = FieldKind.Str,
    required: bool = false,
    min_len: usize = 0,
    max_len: usize = 0,
    min_val: i64 = 0,
    max_val: i64 = 0,
    has_min_val: bool = false,
    has_max_val: bool = false,
    enum_values: string = "",
    item_kind: FieldKind = FieldKind.Str,
    nested_schema: *const ObjectSchema? = null,
}

pub struct ObjectSchema {
    name: string = "",
    fields: [32]FieldRule = [FieldRule{}; 32],
    field_count: usize = 0,

    pub static fn init(name: string): ObjectSchema {
        return ObjectSchema{
            name: name,
        };
    }

    pub fn addRule(*_, rule: FieldRule): *ObjectSchema {
        if (_.field_count < SCHEMA_MAX_FIELDS) {
            _.fields[_.field_count] = rule;
            _.field_count += 1;
        }
        return _;
    }

    pub fn strField(*_, name: string, required: bool, min_len: usize, max_len: usize): *ObjectSchema {
        return _.addRule(FieldRule{
            name: name,
            kind: FieldKind.Str,
            required: required,
            min_len: min_len,
            max_len: max_len,
        });
    }

    pub fn intField(*_, name: string, required: bool, min_val: i64, max_val: i64): *ObjectSchema {
        return _.addRule(FieldRule{
            name: name,
            kind: FieldKind.Int,
            required: required,
            min_val: min_val,
            max_val: max_val,
            has_min_val: true,
            has_max_val: true,
        });
    }

    pub fn floatField(*_, name: string, required: bool, min_val: i64, max_val: i64): *ObjectSchema {
        return _.addRule(FieldRule{
            name: name,
            kind: FieldKind.Float,
            required: required,
            min_val: min_val,
            max_val: max_val,
            has_min_val: true,
            has_max_val: true,
        });
    }

    pub fn boolField(*_, name: string, required: bool): *ObjectSchema {
        return _.addRule(FieldRule{
            name: name,
            kind: FieldKind.Bool,
            required: required,
        });
    }

    pub fn emailField(*_, name: string, required: bool): *ObjectSchema {
        return _.addRule(FieldRule{
            name: name,
            kind: FieldKind.Email,
            required: required,
            min_len: 3,
            max_len: 254,
        });
    }

    pub fn uuidField(*_, name: string, required: bool): *ObjectSchema {
        return _.addRule(FieldRule{
            name: name,
            kind: FieldKind.Uuid,
            required: required,
            min_len: 36,
            max_len: 36,
        });
    }

    pub fn urlField(*_, name: string, required: bool): *ObjectSchema {
        return _.addRule(FieldRule{
            name: name,
            kind: FieldKind.Url,
            required: required,
            min_len: 8,
            max_len: 2048,
        });
    }

    pub fn enumField(*_, name: string, required: bool, enum_values: string): *ObjectSchema {
        return _.addRule(FieldRule{
            name: name,
            kind: FieldKind.Enum,
            required: required,
            enum_values: enum_values,
        });
    }

    pub fn objectField(*_, name: string, required: bool, nested: *const ObjectSchema): *ObjectSchema {
        return _.addRule(FieldRule{
            name: name,
            kind: FieldKind.Object,
            required: required,
            nested_schema: nested,
        });
    }

    pub fn arrayField(*_, name: string, required: bool, item_kind: FieldKind, min_items: usize, max_items: usize): *ObjectSchema {
        return _.addRule(FieldRule{
            name: name,
            kind: FieldKind.Array,
            required: required,
            min_len: min_items,
            max_len: max_items,
            item_kind: item_kind,
        });
    }

    pub fn objectArrayField(*_, name: string, required: bool, item_schema: *const ObjectSchema, min_items: usize, max_items: usize): *ObjectSchema {
        return _.addRule(FieldRule{
            name: name,
            kind: FieldKind.Array,
            required: required,
            min_len: min_items,
            max_len: max_items,
            item_kind: FieldKind.Object,
            nested_schema: item_schema,
        });
    }

    pub fn findField(name: string): usize? {
        let i: usize = 0;
        while (i < _.field_count) : (i += 1) {
            if (std.mem.eql(u8, _.fields[i].name, name)) {
                return i;
            }
        }
        return null;
    }
}

pub struct ValidationIssue {
    field: string = "",
    code: string = "",
    message: string = "",
}

pub struct ValidationReport {
    ok: bool = true,
    issue_count: usize = 0,
    issues: [16]ValidationIssue = [ValidationIssue{}; 16],

    pub fn addIssue(*_, field: string, code: string, message: string) {
        _.ok = false;
        if (_.issue_count < SCHEMA_MAX_ISSUES) {
            _.issues[_.issue_count] = ValidationIssue{
                field: field,
                code: code,
                message: message,
            };
            _.issue_count += 1;
        }
    }
}

// ============================================================================
// 3. Zero-Heap Lightweight JSON Object Scanner & Format Validators
// ============================================================================

enum JsonValKind {
    Str,
    Int,
    Float,
    Bool,
    Null,
    Object,
    Array,
}

struct JsonValueSpan {
    kind: JsonValKind = JsonValKind.Null,
    raw: string = "",
    next_pos: usize = 0,
}

fn isJsonWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn skipWs(s: string, start: usize): usize {
    let i: usize = start;
    while (i < s.len && isJsonWs(s[i])) : (i += 1) {}
    return i;
}

/// Scans a JSON string starting at `start` (must be `"`). Returns index after closing `"` or `null`.
fn scanJsonStringEnd(s: string, start: usize): usize? {
    if (start >= s.len || s[start] != '"') return null;
    let i: usize = start + 1;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') return i + 1;
        if (c < 0x20) return null;
        if (c == '\\') {
            if (i + 1 >= s.len) return null;
            const esc = s[i + 1];
            if (esc == '"' || esc == '\\' || esc == '/' || esc == 'b' || esc == 'f' || esc == 'n' || esc == 'r' || esc == 't') {
                i += 2;
            } else if (esc == 'u') {
                if (i + 5 >= s.len) return null;
                if (hexNibble(s[i + 2]) == null || hexNibble(s[i + 3]) == null || hexNibble(s[i + 4]) == null || hexNibble(s[i + 5]) == null) {
                    return null;
                }
                i += 6;
            } else {
                return null;
            }
        } else {
            i += 1;
        }
    }
    return null;
}

/// Decodes a validated JSON string literal (including surrounding quotes) into `out_buf`.
/// Returns decoded byte length, or `null` if `out_buf` is too small.
fn unescapeJsonString(raw_quoted: string, out_buf: []u8): usize? {
    if (raw_quoted.len < 2 || raw_quoted[0] != '"' || raw_quoted[raw_quoted.len - 1] != '"') return null;
    const inner = raw_quoted[1 .. raw_quoted.len - 1];
    let i: usize = 0;
    let out_pos: usize = 0;
    while (i < inner.len) {
        const c = inner[i];
        if (c != '\\') {
            if (out_pos >= out_buf.len) return null;
            out_buf[out_pos] = c;
            out_pos += 1;
            i += 1;
            continue;
        }
        if (i + 1 >= inner.len) return null;
        const esc = inner[i + 1];
        if (esc == 'u') {
            if (i + 5 >= inner.len) return null;
            const h0 = @as(u32, hexNibble(inner[i + 2]) orelse return null);
            const h1 = @as(u32, hexNibble(inner[i + 3]) orelse return null);
            const h2 = @as(u32, hexNibble(inner[i + 4]) orelse return null);
            const h3 = @as(u32, hexNibble(inner[i + 5]) orelse return null);
            const cp: u32 = (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
            if (cp < 0x80) {
                if (out_pos >= out_buf.len) return null;
                out_buf[out_pos] = @intCast(cp);
                out_pos += 1;
            } else if (cp < 0x800) {
                if (out_pos + 2 > out_buf.len) return null;
                out_buf[out_pos] = @intCast(0xC0 | (cp >> 6));
                out_buf[out_pos + 1] = @intCast(0x80 | (cp & 0x3F));
                out_pos += 2;
            } else {
                if (out_pos + 3 > out_buf.len) return null;
                out_buf[out_pos] = @intCast(0xE0 | (cp >> 12));
                out_buf[out_pos + 1] = @intCast(0x80 | ((cp >> 6) & 0x3F));
                out_buf[out_pos + 2] = @intCast(0x80 | (cp & 0x3F));
                out_pos += 3;
            }
            i += 6;
        } else {
            if (out_pos >= out_buf.len) return null;
            const mapped: u8 = switch (esc) {
                '"' => '"',
                '\\' => '\\',
                '/' => '/',
                'b' => 0x08,
                'f' => 0x0C,
                'n' => '\n',
                'r' => '\r',
                't' => '\t',
                else => return null,
            };
            out_buf[out_pos] = mapped;
            out_pos += 1;
            i += 2;
        }
    }
    return out_pos;
}

fn scanJsonNumber(s: string, start: usize): JsonValueSpan? {
    let i: usize = start;
    if (i >= s.len) return null;
    if (s[i] == '-') i += 1;
    if (i >= s.len) return null;

    if (s[i] == '0') {
        i += 1;
        if (i < s.len && s[i] >= '0' && s[i] <= '9') return null;
    } else if (s[i] >= '1' && s[i] <= '9') {
        while (i < s.len && s[i] >= '0' && s[i] <= '9') : (i += 1) {}
    } else {
        return null;
    }

    let is_float = false;
    if (i < s.len && s[i] == '.') {
        is_float = true;
        i += 1;
        if (i >= s.len || s[i] < '0' || s[i] > '9') return null;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') : (i += 1) {}
    }
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        is_float = true;
        i += 1;
        if (i < s.len && (s[i] == '+' || s[i] == '-')) i += 1;
        if (i >= s.len || s[i] < '0' || s[i] > '9') return null;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') : (i += 1) {}
    }

    return JsonValueSpan{
        kind: if (is_float) JsonValKind.Float else JsonValKind.Int,
        raw: s[start..i],
        next_pos: i,
    };
}

fn scanJsonValue(s: string, start: usize, depth: usize): JsonValueSpan? {
    if (depth > SCHEMA_MAX_JSON_DEPTH) return null;
    const p = skipWs(s, start);
    if (p >= s.len) return null;
    const c = s[p];

    if (c == '"') {
        const end = scanJsonStringEnd(s, p) orelse return null;
        return JsonValueSpan{ kind: JsonValKind.Str, raw: s[p..end], next_pos: end };
    }
    if (c == '-' || (c >= '0' && c <= '9')) {
        return scanJsonNumber(s, p);
    }
    if (c == 't') {
        if (p + 4 <= s.len && std.mem.eql(u8, s[p .. p + 4], "true")) {
            return JsonValueSpan{ kind: JsonValKind.Bool, raw: s[p .. p + 4], next_pos: p + 4 };
        }
        return null;
    }
    if (c == 'f') {
        if (p + 5 <= s.len && std.mem.eql(u8, s[p .. p + 5], "false")) {
            return JsonValueSpan{ kind: JsonValKind.Bool, raw: s[p .. p + 5], next_pos: p + 5 };
        }
        return null;
    }
    if (c == 'n') {
        if (p + 4 <= s.len && std.mem.eql(u8, s[p .. p + 4], "null")) {
            return JsonValueSpan{ kind: JsonValKind.Null, raw: s[p .. p + 4], next_pos: p + 4 };
        }
        return null;
    }
    if (c == '{') {
        let i = skipWs(s, p + 1);
        if (i >= s.len) return null;
        if (s[i] == '}') {
            return JsonValueSpan{ kind: JsonValKind.Object, raw: s[p .. i + 1], next_pos: i + 1 };
        }
        while (true) {
            i = skipWs(s, i);
            const k_end = scanJsonStringEnd(s, i) orelse return null;
            i = skipWs(s, k_end);
            if (i >= s.len || s[i] != ':') return null;
            const v_span = scanJsonValue(s, i + 1, depth + 1) orelse return null;
            i = skipWs(s, v_span.next_pos);
            if (i >= s.len) return null;
            if (s[i] == '}') {
                return JsonValueSpan{ kind: JsonValKind.Object, raw: s[p .. i + 1], next_pos: i + 1 };
            }
            if (s[i] != ',') return null;
            i += 1;
        }
    }
    if (c == '[') {
        let i = skipWs(s, p + 1);
        if (i >= s.len) return null;
        if (s[i] == ']') {
            return JsonValueSpan{ kind: JsonValKind.Array, raw: s[p .. i + 1], next_pos: i + 1 };
        }
        while (true) {
            const v_span = scanJsonValue(s, i, depth + 1) orelse return null;
            i = skipWs(s, v_span.next_pos);
            if (i >= s.len) return null;
            if (s[i] == ']') {
                return JsonValueSpan{ kind: JsonValKind.Array, raw: s[p .. i + 1], next_pos: i + 1 };
            }
            if (s[i] != ',') return null;
            i += 1;
        }
    }
    return null;
}

fn parseStrictI64(raw: string): i64? {
    if (raw.len == 0) return null;
    let neg = false;
    let i: usize = 0;
    if (raw[0] == '-') {
        neg = true;
        i = 1;
    }
    if (i >= raw.len) return null;
    let acc: u64 = 0;
    const max_pos_u64: u64 = 9223372036854775807;
    const limit: u64 = if (neg) (max_pos_u64 + 1) else max_pos_u64;
    while (i < raw.len) : (i += 1) {
        const c = raw[i];
        if (c < '0' || c > '9') return null;
        const digit: u64 = @as(u64, c - '0');
        if (acc > (limit - digit) / 10) return null;
        acc = acc * 10 + digit;
    }
    if (neg) {
        if (acc == max_pos_u64 + 1) return -9223372036854775807 - 1;
        return -@as(i64, @intCast(acc));
    }
    return @as(i64, @intCast(acc));
}

fn parseApproxF64(raw: string): f64? {
    if (raw.len == 0) return null;
    let i: usize = 0;
    let neg = false;
    if (raw[i] == '-') {
        neg = true;
        i += 1;
    }
    if (i >= raw.len) return null;

    let int_part: f64 = 0.0;
    let has_digits = false;
    while (i < raw.len && raw[i] >= '0' && raw[i] <= '9') : (i += 1) {
        has_digits = true;
        int_part = int_part * 10.0 + @as(f64, @floatFromInt(raw[i] - '0'));
    }
    if (!has_digits) return null;

    let frac_part: f64 = 0.0;
    if (i < raw.len && raw[i] == '.') {
        i += 1;
        let frac_scale: f64 = 0.1;
        let has_frac = false;
        while (i < raw.len && raw[i] >= '0' && raw[i] <= '9') : (i += 1) {
            has_frac = true;
            frac_part += @as(f64, @floatFromInt(raw[i] - '0')) * frac_scale;
            frac_scale *= 0.1;
        }
        if (!has_frac) return null;
    }

    let val: f64 = int_part + frac_part;
    if (i < raw.len && (raw[i] == 'e' || raw[i] == 'E')) {
        i += 1;
        let exp_neg = false;
        if (i < raw.len && (raw[i] == '+' || raw[i] == '-')) {
            exp_neg = raw[i] == '-';
            i += 1;
        }
        let exp_val: i32 = 0;
        let has_exp = false;
        while (i < raw.len && raw[i] >= '0' && raw[i] <= '9') : (i += 1) {
            has_exp = true;
            if (exp_val < 308) {
                exp_val = exp_val * 10 + @as(i32, raw[i] - '0');
            }
        }
        if (!has_exp) return null;
        let e: i32 = 0;
        while (e < exp_val) : (e += 1) {
            if (exp_neg) {
                val *= 0.1;
            } else {
                val *= 10.0;
            }
        }
    }
    if (i != raw.len) return null;
    return if (neg) -val else val;
}

fn isValidEmail(s: string): bool {
    if (s.len < 3 || s.len > 254) return false;
    let at_pos: usize = 0;
    let at_count: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c <= 0x20 || c >= 0x7F) return false;
        if (c == '@') {
            at_pos = i;
            at_count += 1;
        }
    }
    if (at_count != 1) return false;
    if (at_pos == 0 || at_pos > 64 || at_pos + 3 > s.len) return false;
    const domain = s[at_pos + 1 ..];
    if (domain[0] == '.' || domain[domain.len - 1] == '.') return false;
    let has_dot = false;
    let j: usize = 0;
    while (j < domain.len) : (j += 1) {
        const c = domain[j];
        if (c == '.') {
            if (j > 0 && domain[j - 1] == '.') return false;
            has_dot = true;
        } else if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-')) {
            return false;
        }
    }
    return has_dot;
}

fn isValidUuid(s: string): bool {
    if (s.len != 36) return false;
    let i: usize = 0;
    while (i < 36) : (i += 1) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (s[i] != '-') return false;
        } else {
            if (hexNibble(s[i]) == null) return false;
        }
    }
    return true;
}

fn isValidUrl(s: string): bool {
    if (s.len < 8 || s.len > 2048) return false;
    let host_start: usize = 0;
    if (s.len >= 8 && std.mem.eql(u8, s[0..8], "https://")) {
        host_start = 8;
    } else if (s.len >= 7 && std.mem.eql(u8, s[0..7], "http://")) {
        host_start = 7;
    } else {
        return false;
    }
    if (host_start >= s.len) return false;
    let host_len: usize = 0;
    let i: usize = host_start;
    let in_host = true;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c <= 0x20 || c >= 0x7F) return false;
        if (in_host) {
            if (c == '/' || c == '?' || c == '#') {
                in_host = false;
            } else {
                host_len += 1;
            }
        }
    }
    return host_len > 0;
}

fn isInEnumList(val: string, enum_values: string): bool {
    if (enum_values.len == 0) return false;
    let pos: usize = 0;
    while (pos <= enum_values.len) {
        let end: usize = pos;
        while (end < enum_values.len && enum_values[end] != ',') : (end += 1) {}
        if (std.mem.eql(u8, enum_values[pos..end], val)) {
            return true;
        }
        if (end >= enum_values.len) break;
        pos = end + 1;
    }
    return false;
}

fn validateFieldValue(rule: FieldRule, val: JsonValueSpan, report: *ValidationReport) {
    if (val.kind == JsonValKind.Null) {
        if (rule.required) {
            report.addIssue(rule.name, "required", "field is required and cannot be null");
        }
        return;
    }

    switch (rule.kind) {
        FieldKind.Str => {
            if (val.kind != JsonValKind.Str) {
                report.addIssue(rule.name, "type", "expected string");
                return;
            }
            let str_buf: [2048]u8 = undefined;
            const decoded_len = unescapeJsonString(val.raw, str_buf[0..]) orelse (val.raw.len - 2);
            if (decoded_len < rule.min_len) {
                report.addIssue(rule.name, "min_len", "string shorter than minimum length");
            }
            if (rule.max_len > 0 && decoded_len > rule.max_len) {
                report.addIssue(rule.name, "max_len", "string exceeds maximum length");
            }
        },
        FieldKind.Int => {
            if (val.kind != JsonValKind.Int) {
                report.addIssue(rule.name, "type", "expected integer");
                return;
            }
            const parsed = parseStrictI64(val.raw) orelse {
                report.addIssue(rule.name, "range", "integer out of 64-bit range");
                return;
            };
            if (rule.has_min_val && parsed < rule.min_val) {
                report.addIssue(rule.name, "min_val", "integer below minimum value");
            }
            if (rule.has_max_val && parsed > rule.max_val) {
                report.addIssue(rule.name, "max_val", "integer exceeds maximum value");
            }
        },
        FieldKind.Float => {
            if (val.kind != JsonValKind.Float && val.kind != JsonValKind.Int) {
                report.addIssue(rule.name, "type", "expected number");
                return;
            }
            const parsed = parseApproxF64(val.raw) orelse {
                report.addIssue(rule.name, "format", "invalid floating-point number");
                return;
            };
            if (rule.has_min_val && parsed < @as(f64, @floatFromInt(rule.min_val))) {
                report.addIssue(rule.name, "min_val", "number below minimum value");
            }
            if (rule.has_max_val && parsed > @as(f64, @floatFromInt(rule.max_val))) {
                report.addIssue(rule.name, "max_val", "number exceeds maximum value");
            }
        },
        FieldKind.Bool => {
            if (val.kind != JsonValKind.Bool) {
                report.addIssue(rule.name, "type", "expected boolean");
            }
        },
        FieldKind.Email => {
            if (val.kind != JsonValKind.Str) {
                report.addIssue(rule.name, "type", "expected email string");
                return;
            }
            let str_buf: [512]u8 = undefined;
            const decoded_len = unescapeJsonString(val.raw, str_buf[0..]) orelse {
                report.addIssue(rule.name, "format", "invalid email format");
                return;
            };
            if (!isValidEmail(str_buf[0..decoded_len])) {
                report.addIssue(rule.name, "format", "invalid email format");
            }
        },
        FieldKind.Uuid => {
            if (val.kind != JsonValKind.Str) {
                report.addIssue(rule.name, "type", "expected uuid string");
                return;
            }
            let str_buf: [64]u8 = undefined;
            const decoded_len = unescapeJsonString(val.raw, str_buf[0..]) orelse {
                report.addIssue(rule.name, "format", "invalid uuid format");
                return;
            };
            if (!isValidUuid(str_buf[0..decoded_len])) {
                report.addIssue(rule.name, "format", "invalid uuid format");
            }
        },
        FieldKind.Url => {
            if (val.kind != JsonValKind.Str) {
                report.addIssue(rule.name, "type", "expected url string");
                return;
            }
            let str_buf: [2048]u8 = undefined;
            const decoded_len = unescapeJsonString(val.raw, str_buf[0..]) orelse {
                report.addIssue(rule.name, "format", "invalid url format");
                return;
            };
            if (!isValidUrl(str_buf[0..decoded_len])) {
                report.addIssue(rule.name, "format", "invalid url format");
            }
        },
        FieldKind.Enum => {
            if (val.kind != JsonValKind.Str) {
                report.addIssue(rule.name, "type", "expected enum string");
                return;
            }
            let str_buf: [256]u8 = undefined;
            const decoded_len = unescapeJsonString(val.raw, str_buf[0..]) orelse {
                report.addIssue(rule.name, "enum", "invalid enum value");
                return;
            };
            if (!isInEnumList(str_buf[0..decoded_len], rule.enum_values)) {
                report.addIssue(rule.name, "enum", "value not in allowed enum set");
            }
        },
        FieldKind.Object => {
            if (val.kind != JsonValKind.Object) {
                report.addIssue(rule.name, "type", "expected nested JSON object");
                return;
            }
            if (sub_schema := rule.nested_schema) {
                const sub_rep = validateJsonObject(val.raw, sub_schema);
                let si: usize = 0;
                while (si < sub_rep.issue_count) : (si += 1) {
                    const iss = sub_rep.issues[si];
                    report.addIssue(iss.field, iss.code, iss.message);
                }
            }
        },
        FieldKind.Array => {
            if (val.kind != JsonValKind.Array) {
                report.addIssue(rule.name, "type", "expected JSON array");
                return;
            }
            const raw = val.raw;
            let pos = skipWs(raw, 0);
            if (pos >= raw.len || raw[pos] != '[') {
                report.addIssue(rule.name, "type", "expected JSON array starting with '['");
                return;
            }
            pos = skipWs(raw, pos + 1);
            let item_count: usize = 0;
            const item_rule = FieldRule{
                name: rule.name,
                kind: rule.item_kind,
                required: true,
                enum_values: rule.enum_values,
                nested_schema: rule.nested_schema,
            };
            if (pos < raw.len && raw[pos] != ']') {
                while (true) {
                    const elem_span = scanJsonValue(raw, pos, 1) orelse {
                        report.addIssue(rule.name, "json", "invalid array element");
                        return;
                    };
                    validateFieldValue(item_rule, elem_span, report);
                    item_count += 1;
                    pos = skipWs(raw, elem_span.next_pos);
                    if (pos >= raw.len) {
                        report.addIssue(rule.name, "json", "unterminated JSON array");
                        return;
                    }
                    if (raw[pos] == ']') break;
                    if (raw[pos] != ',') {
                        report.addIssue(rule.name, "json", "expected ',' or ']' in JSON array");
                        return;
                    }
                    pos = skipWs(raw, pos + 1);
                }
            }
            if (rule.min_len > 0 && item_count < rule.min_len) {
                report.addIssue(rule.name, "min_items", "array has fewer items than minimum");
            }
            if (rule.max_len > 0 && item_count > rule.max_len) {
                report.addIssue(rule.name, "max_items", "array exceeds maximum item count");
            }
        },
    }
}

/// Zero-heap JSON object parser + schema validator.
/// Validates required fields, types, lengths, numeric bounds, email, UUID, URL, and enum membership.
pub fn validateJsonObject(json_body: string, schema: *const ObjectSchema): ValidationReport {
    let report = ValidationReport{};
    let seen: [32]bool = [false; 32];

    let pos = skipWs(json_body, 0);
    if (pos >= json_body.len || json_body[pos] != '{') {
        report.addIssue("$", "json", "expected JSON object starting with '{'");
        return report;
    }
    pos = skipWs(json_body, pos + 1);
    if (pos >= json_body.len) {
        report.addIssue("$", "json", "unexpected end of JSON object");
        return report;
    }

    let key_buf: [256]u8 = undefined;
    if (json_body[pos] != '}') {
        while (true) {
            pos = skipWs(json_body, pos);
            const k_end = scanJsonStringEnd(json_body, pos) orelse {
                report.addIssue("$", "json", "invalid or unterminated object key");
                return report;
            };
            const raw_key = json_body[pos..k_end];
            pos = skipWs(json_body, k_end);
            if (pos >= json_body.len || json_body[pos] != ':') {
                report.addIssue("$", "json", "expected ':' after object key");
                return report;
            }
            const v_span = scanJsonValue(json_body, pos + 1, 1) orelse {
                report.addIssue("$", "json", "invalid JSON value");
                return report;
            };

            if (klen := unescapeJsonString(raw_key, key_buf[0..])) {
                const key_str = key_buf[0..klen];
                if (idx := schema.findField(key_str)) {
                    seen[idx] = true;
                    validateFieldValue(schema.fields[idx], v_span, &report);
                }
            }

            pos = skipWs(json_body, v_span.next_pos);
            if (pos >= json_body.len) {
                report.addIssue("$", "json", "unexpected end of JSON object");
                return report;
            }
            if (json_body[pos] == '}') {
                pos += 1;
                break;
            }
            if (json_body[pos] != ',') {
                report.addIssue("$", "json", "expected ',' or '}' in JSON object");
                return report;
            }
            pos += 1;
        }
    } else {
        pos += 1;
    }

    pos = skipWs(json_body, pos);
    if (pos != json_body.len) {
        report.addIssue("$", "json", "trailing characters after JSON object");
        return report;
    }

    let fi: usize = 0;
    while (fi < schema.field_count) : (fi += 1) {
        if (schema.fields[fi].required && !seen[fi]) {
            report.addIssue(schema.fields[fi].name, "required", "missing required field");
        }
    }

    return report;
}

// ============================================================================
// 4. Elysia `exact-mirror` Compatible Strict Field Pruning
// ============================================================================

struct SchemaBufWriter {
    buf: []u8,
    pos: usize = 0,

    pub fn writeBytes(*_, data: string): bool {
        if (_.pos + data.len > _.buf.len) return false;
        let i: usize = 0;
        while (i < data.len) : (i += 1) {
            _.buf[_.pos + i] = data[i];
        }
        _.pos += data.len;
        return true;
    }

    pub fn writeByte(*_, b: u8): bool {
        if (_.pos >= _.buf.len) return false;
        _.buf[_.pos] = b;
        _.pos += 1;
        return true;
    }

    pub fn writeJsonEscapedStr(*_, s: string): bool {
        if (!_.writeByte('"')) return false;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c == '"') {
                if (!_.writeBytes("\\\"")) return false;
            } else if (c == '\\') {
                if (!_.writeBytes("\\\\")) return false;
            } else if (c == '\n') {
                if (!_.writeBytes("\\n")) return false;
            } else if (c == '\r') {
                if (!_.writeBytes("\\r")) return false;
            } else if (c == '\t') {
                if (!_.writeBytes("\\t")) return false;
            } else if (c < 0x20) {
                continue;
            } else {
                if (!_.writeByte(c)) return false;
            }
        }
        return _.writeByte('"');
    }

    pub fn writeU64Ascii(*_, val: u64): bool {
        let tmp: [24]u8 = undefined;
        let len: usize = 0;
        let v: u64 = val;
        if (v == 0) {
            return _.writeByte('0');
        }
        while (v > 0) {
            tmp[len] = @as(u8, @intCast(v % 10)) + '0';
            len += 1;
            v /= 10;
        }
        if (_.pos + len > _.buf.len) return false;
        let i: usize = 0;
        while (i < len) : (i += 1) {
            _.buf[_.pos + i] = tmp[len - 1 - i];
        }
        _.pos += len;
        return true;
    }

    pub fn writeI64Ascii(*_, val: i64): bool {
        if (val < 0) {
            if (!_.writeByte('-')) return false;
            if (val == (-9223372036854775807 - 1)) {
                return _.writeBytes("9223372036854775808");
            }
            return _.writeU64Ascii(@as(u64, @intCast(-val)));
        }
        return _.writeU64Ascii(@as(u64, @intCast(val)));
    }

    pub fn writeUsizeAscii(*_, val: usize): bool {
        return _.writeU64Ascii(@as(u64, val));
    }
}

/// 1:1 counterpart to Elysia `exact-mirror`: parses `json_body`, keeps ONLY fields
/// declared in `schema` (stripping undeclared/sensitive fields such as `password_hash`
/// or `internal_role`), and serializes clean JSON into `out_buf`.
/// Returns bytes written or `null` on malformed JSON or insufficient `out_buf`.
pub fn exactMirrorJson(json_body: string, schema: *const ObjectSchema, out_buf: []u8): usize? {
    let pos = skipWs(json_body, 0);
    if (pos >= json_body.len || json_body[pos] != '{') return null;
    pos = skipWs(json_body, pos + 1);
    if (pos >= json_body.len) return null;

    let field_raw_vals: [32]string = [""; 32];
    let field_present: [32]bool = [false; 32];
    let key_buf: [256]u8 = undefined;

    if (json_body[pos] != '}') {
        while (true) {
            pos = skipWs(json_body, pos);
            const k_end = scanJsonStringEnd(json_body, pos) orelse return null;
            const raw_key = json_body[pos..k_end];
            pos = skipWs(json_body, k_end);
            if (pos >= json_body.len || json_body[pos] != ':') return null;
            const v_span = scanJsonValue(json_body, pos + 1, 1) orelse return null;

            if (klen := unescapeJsonString(raw_key, key_buf[0..])) {
                const key_str = key_buf[0..klen];
                if (idx := schema.findField(key_str)) {
                    let probe = ValidationReport{};
                    validateFieldValue(schema.fields[idx], v_span, &probe);
                    if (probe.ok) {
                        field_raw_vals[idx] = v_span.raw;
                        field_present[idx] = true;
                    }
                }
            }

            pos = skipWs(json_body, v_span.next_pos);
            if (pos >= json_body.len) return null;
            if (json_body[pos] == '}') {
                pos += 1;
                break;
            }
            if (json_body[pos] != ',') return null;
            pos += 1;
        }
    } else {
        pos += 1;
    }

    pos = skipWs(json_body, pos);
    if (pos != json_body.len) return null;

    let w = SchemaBufWriter{ buf: out_buf };
    if (!w.writeByte('{')) return null;

    let emitted: usize = 0;
    let fi: usize = 0;
    while (fi < schema.field_count) : (fi += 1) {
        if (!field_present[fi]) continue;
        if (emitted > 0) {
            if (!w.writeByte(',')) return null;
        }
        if (!w.writeJsonEscapedStr(schema.fields[fi].name)) return null;
        if (!w.writeByte(':')) return null;
        if (!w.writeBytes(field_raw_vals[fi])) return null;
        emitted += 1;
    }

    if (!w.writeByte('}')) return null;
    return w.pos;
}

// ============================================================================
// 5. OpenAPI 3.1.0 JSON Schema Emitter
// ============================================================================

/// Exports `ObjectSchema` as an OpenAPI 3.1.0 / JSON Schema Draft 2020-12 object fragment.
/// Returns bytes written or `null` if `out_buf` is too small.
pub fn schemaToOpenApiJson(schema: *const ObjectSchema, out_buf: []u8): usize? {
    let w = SchemaBufWriter{ buf: out_buf };
    if (!w.writeByte('{')) return null;

    if (schema.name.len > 0) {
        if (!w.writeBytes("\"title\":")) return null;
        if (!w.writeJsonEscapedStr(schema.name)) return null;
        if (!w.writeByte(',')) return null;
    }

    if (!w.writeBytes("\"type\":\"object\",\"properties\":{")) return null;

    let fi: usize = 0;
    while (fi < schema.field_count) : (fi += 1) {
        const rule = schema.fields[fi];
        if (fi > 0) {
            if (!w.writeByte(',')) return null;
        }
        if (!w.writeJsonEscapedStr(rule.name)) return null;
        if (!w.writeBytes(":{")) return null;

        switch (rule.kind) {
            FieldKind.Str => {
                if (!w.writeBytes("\"type\":\"string\"")) return null;
                if (rule.min_len > 0) {
                    if (!w.writeBytes(",\"minLength\":")) return null;
                    if (!w.writeUsizeAscii(rule.min_len)) return null;
                }
                if (rule.max_len > 0) {
                    if (!w.writeBytes(",\"maxLength\":")) return null;
                    if (!w.writeUsizeAscii(rule.max_len)) return null;
                }
            },
            FieldKind.Int => {
                if (!w.writeBytes("\"type\":\"integer\"")) return null;
                if (rule.has_min_val) {
                    if (!w.writeBytes(",\"minimum\":")) return null;
                    if (!w.writeI64Ascii(rule.min_val)) return null;
                }
                if (rule.has_max_val) {
                    if (!w.writeBytes(",\"maximum\":")) return null;
                    if (!w.writeI64Ascii(rule.max_val)) return null;
                }
            },
            FieldKind.Float => {
                if (!w.writeBytes("\"type\":\"number\"")) return null;
                if (rule.has_min_val) {
                    if (!w.writeBytes(",\"minimum\":")) return null;
                    if (!w.writeI64Ascii(rule.min_val)) return null;
                }
                if (rule.has_max_val) {
                    if (!w.writeBytes(",\"maximum\":")) return null;
                    if (!w.writeI64Ascii(rule.max_val)) return null;
                }
            },
            FieldKind.Bool => {
                if (!w.writeBytes("\"type\":\"boolean\"")) return null;
            },
            FieldKind.Email => {
                if (!w.writeBytes("\"type\":\"string\",\"format\":\"email\"")) return null;
            },
            FieldKind.Uuid => {
                if (!w.writeBytes("\"type\":\"string\",\"format\":\"uuid\"")) return null;
            },
            FieldKind.Url => {
                if (!w.writeBytes("\"type\":\"string\",\"format\":\"uri\"")) return null;
            },
            FieldKind.Enum => {
                if (!w.writeBytes("\"type\":\"string\",\"enum\":[")) return null;
                let epos: usize = 0;
                let eidx: usize = 0;
                while (epos <= rule.enum_values.len && rule.enum_values.len > 0) {
                    let eend: usize = epos;
                    while (eend < rule.enum_values.len && rule.enum_values[eend] != ',') : (eend += 1) {}
                    if (eidx > 0) {
                        if (!w.writeByte(',')) return null;
                    }
                    if (!w.writeJsonEscapedStr(rule.enum_values[epos..eend])) return null;
                    eidx += 1;
                    if (eend >= rule.enum_values.len) break;
                    epos = eend + 1;
                }
                if (!w.writeByte(']')) return null;
            },
            FieldKind.Object => {
                if (!w.writeBytes("\"type\":\"object\"")) return null;
            },
            FieldKind.Array => {
                if (!w.writeBytes("\"type\":\"array\"")) return null;
                if (rule.min_len > 0) {
                    if (!w.writeBytes(",\"minItems\":")) return null;
                    if (!w.writeUsizeAscii(rule.min_len)) return null;
                }
                if (rule.max_len > 0) {
                    if (!w.writeBytes(",\"maxItems\":")) return null;
                    if (!w.writeUsizeAscii(rule.max_len)) return null;
                }
            },
        }

        if (!w.writeByte('}')) return null;
    }

    if (!w.writeByte('}')) return null;

    let req_count: usize = 0;
    let ri: usize = 0;
    while (ri < schema.field_count) : (ri += 1) {
        if (schema.fields[ri].required) req_count += 1;
    }
    if (req_count > 0) {
        if (!w.writeBytes(",\"required\":[")) return null;
        let emitted: usize = 0;
        let rj: usize = 0;
        while (rj < schema.field_count) : (rj += 1) {
            if (!schema.fields[rj].required) continue;
            if (emitted > 0) {
                if (!w.writeByte(',')) return null;
            }
            if (!w.writeJsonEscapedStr(schema.fields[rj].name)) return null;
            emitted += 1;
        }
        if (!w.writeByte(']')) return null;
    }

    if (!w.writeBytes(",\"additionalProperties\":false}")) return null;
    return w.pos;
}
