// @cap/json-schema — Zero-allocation JSON Schema Draft-07/2020-12 validator for CP
// Clean-room MIT implementation inspired by npm `ajv` and Rust `jsonschema`.

pub const MAX_REQUIRED_KEYS: usize = 8;
pub const MAX_ENUM_VALUES: usize = 8;

pub enum JsonType: u8 {
    Any = 0,
    Null = 1,
    Boolean = 2,
    Integer = 3,
    Number = 4,
    String = 5,
    Array = 6,
    Object = 7,
}

pub struct ValidationError {
    pub code: string,
    pub message: string,
    pub path: string,

    pub static fn none() -> ValidationError {
        return ValidationError{
            code: "",
            message: "",
            path: "",
        };
    }
}

pub struct ValidationResult {
    pub valid: bool,
    pub error_count: usize,
    pub first_error: ValidationError,

    pub static fn ok() -> ValidationResult {
        return ValidationResult{
            valid: true,
            error_count: 0,
            first_error: ValidationError.none(),
        };
    }
}

pub struct SchemaRule {
    pub expected_type: JsonType,
    pub has_min_len: bool,
    pub min_len: usize,
    pub has_max_len: bool,
    pub max_len: usize,
    pub has_min_val: bool,
    pub min_val: f64,
    pub has_max_val: bool,
    pub max_val: f64,
    pub exclusive_min: bool,
    pub exclusive_max: bool,
    pub required_keys: [8]string,
    pub required_count: usize,
    pub enum_values: [8]string,
    pub enum_count: usize,

    pub static fn any() -> SchemaRule {
        return SchemaRule{
            expected_type: JsonType.Any,
            has_min_len: false,
            min_len: 0,
            has_max_len: false,
            max_len: 0,
            has_min_val: false,
            min_val: 0.0,
            has_max_val: false,
            max_val: 0.0,
            exclusive_min: false,
            exclusive_max: false,
            required_keys: ["", "", "", "", "", "", "", ""],
            required_count: 0,
            enum_values: ["", "", "", "", "", "", "", ""],
            enum_count: 0,
        };
    }

    pub fn addRequired(*_, key: string) {
        if (_.required_count < MAX_REQUIRED_KEYS && key.len > 0) {
            _.required_keys[_.required_count] = key;
            _.required_count += 1;
        }
    }

    pub fn addEnum(*_, val: string) {
        if (_.enum_count < MAX_ENUM_VALUES) {
            _.enum_values[_.enum_count] = val;
            _.enum_count += 1;
        }
    }

    pub fn setExclusive(*_, min_excl: bool, max_excl: bool) {
        _.exclusive_min = min_excl;
        _.exclusive_max = max_excl;
    }
}

// ---------------------------------------------------------------------------
// Rule Constructors
// ---------------------------------------------------------------------------

pub fn stringRule(min_len: usize, max_len: usize) -> SchemaRule {
    let r = SchemaRule.any();
    r.expected_type = JsonType.String;
    r.has_min_len = min_len > 0;
    r.min_len = min_len;
    r.has_max_len = max_len > 0;
    r.max_len = max_len;
    return r;
}

pub fn numberRule(min_val: f64, max_val: f64) -> SchemaRule {
    let r = SchemaRule.any();
    r.expected_type = JsonType.Number;
    r.has_min_val = true;
    r.min_val = min_val;
    r.has_max_val = true;
    r.max_val = max_val;
    return r;
}

pub fn integerRule(min_val: f64, max_val: f64) -> SchemaRule {
    let r = SchemaRule.any();
    r.expected_type = JsonType.Integer;
    r.has_min_val = true;
    r.min_val = min_val;
    r.has_max_val = true;
    r.max_val = max_val;
    return r;
}

pub fn arrayRule(min_items: usize, max_items: usize) -> SchemaRule {
    let r = SchemaRule.any();
    r.expected_type = JsonType.Array;
    r.has_min_len = min_items > 0;
    r.min_len = min_items;
    r.has_max_len = max_items > 0;
    r.max_len = max_items;
    return r;
}

pub fn objectRule() -> SchemaRule {
    let r = SchemaRule.any();
    r.expected_type = JsonType.Object;
    return r;
}

// ---------------------------------------------------------------------------
// Zero-Allocation RFC 8259 JSON Lexer / Structural Scanner
// ---------------------------------------------------------------------------

fn isWs(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8) -> bool {
    return c >= '0' && c <= '9';
}

fn isHex(c: u8) -> bool {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn skipWs(s: string, start: usize) -> usize {
    let i = start;
    while (i < s.len && isWs(s[i])) {
        i += 1;
    }
    return i;
}

fn eqStr(a: string, b: string) -> bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn skipJsonString(s: string, start: usize) -> usize? {
    if (start >= s.len || s[start] != '"') {
        return null;
    }
    let i = start + 1;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            return i + 1;
        }
        if (c < 0x20) {
            return null;
        }
        if (c == '\\') {
            i += 1;
            if (i >= s.len) return null;
            const esc = s[i];
            if (esc == '"' || esc == '\\' || esc == '/' || esc == 'b' || esc == 'f' || esc == 'n' || esc == 'r' || esc == 't') {
                i += 1;
            } else if (esc == 'u') {
                if (i + 4 >= s.len) return null;
                if (!isHex(s[i + 1]) || !isHex(s[i + 2]) || !isHex(s[i + 3]) || !isHex(s[i + 4])) {
                    return null;
                }
                i += 5;
            } else {
                return null;
            }
            continue;
        }
        i += 1;
    }
    return null;
}

struct ParsedJsonNumber {
    ok: bool,
    is_int: bool,
    val: f64,
    next_pos: usize,
}

fn pow10(exp: i32) -> f64 {
    let res: f64 = 1.0;
    if (exp > 0) {
        let e = exp;
        while (e > 0) {
            res *= 10.0;
            e -= 1;
        }
    } else if (exp < 0) {
        let e = -exp;
        while (e > 0) {
            res /= 10.0;
            e -= 1;
        }
    }
    return res;
}

fn parseJsonNumber(s: string, start: usize) -> ParsedJsonNumber {
    const bad = ParsedJsonNumber{ ok: false, is_int: false, val: 0.0, next_pos: start };
    if (start >= s.len) return bad;

    let i = start;
    let neg = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
    }
    if (i >= s.len || !isDigit(s[i])) {
        return bad;
    }

    let int_part: f64 = 0.0;
    if (s[i] == '0') {
        i += 1;
        if (i < s.len && isDigit(s[i])) {
            return bad; // Leading zeros forbidden in JSON
        }
    } else {
        while (i < s.len && isDigit(s[i])) {
            int_part = int_part * 10.0 + ((s[i] - '0') as f64);
            i += 1;
        }
    }

    let is_int = true;
    let frac_part: f64 = 0.0;
    let frac_scale: f64 = 1.0;
    if (i < s.len && s[i] == '.') {
        is_int = false;
        i += 1;
        if (i >= s.len || !isDigit(s[i])) {
            return bad;
        }
        while (i < s.len && isDigit(s[i])) {
            frac_part = frac_part * 10.0 + ((s[i] - '0') as f64);
            frac_scale *= 10.0;
            i += 1;
        }
    }

    let val = int_part + (frac_part / frac_scale);

    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        is_int = false;
        i += 1;
        let exp_neg = false;
        if (i < s.len && s[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < s.len && s[i] == '+') {
            i += 1;
        }
        if (i >= s.len || !isDigit(s[i])) {
            return bad;
        }
        let exp_val: i32 = 0;
        while (i < s.len && isDigit(s[i])) {
            if (exp_val < 300) {
                exp_val = exp_val * 10 + ((s[i] - '0') as i32);
            }
            i += 1;
        }
        val *= pow10(if (exp_neg) -exp_val else exp_val);
    }

    if (neg) {
        val = -val;
    }
    return ParsedJsonNumber{ ok: true, is_int: is_int, val: val, next_pos: i };
}

fn skipJsonValue(s: string, start: usize, depth: usize) -> usize? {
    if (depth > 64) {
        return null;
    }
    let i = skipWs(s, start);
    if (i >= s.len) {
        return null;
    }
    const c = s[i];
    if (c == '"') {
        return skipJsonString(s, i);
    }
    if (c == '-' || isDigit(c)) {
        const num = parseJsonNumber(s, i);
        if (!num.ok) return null;
        return num.next_pos;
    }
    if (c == 't') {
        if (i + 4 <= s.len && eqStr(s[i..(i + 4)], "true")) return i + 4;
        return null;
    }
    if (c == 'f') {
        if (i + 5 <= s.len && eqStr(s[i..(i + 5)], "false")) return i + 5;
        return null;
    }
    if (c == 'n') {
        if (i + 4 <= s.len && eqStr(s[i..(i + 4)], "null")) return i + 4;
        return null;
    }
    if (c == '[') {
        i = skipWs(s, i + 1);
        if (i >= s.len) return null;
        if (s[i] == ']') return i + 1;
        while (true) {
            const next_opt = skipJsonValue(s, i, depth + 1);
            if (next_opt == null) return null;
            i = skipWs(s, next_opt.?);
            if (i >= s.len) return null;
            if (s[i] == ']') return i + 1;
            if (s[i] != ',') return null;
            i = skipWs(s, i + 1);
        }
    }
    if (c == '{') {
        i = skipWs(s, i + 1);
        if (i >= s.len) return null;
        if (s[i] == '}') return i + 1;
        while (true) {
            const key_end = skipJsonString(s, i);
            if (key_end == null) return null;
            i = skipWs(s, key_end.?);
            if (i >= s.len || s[i] != ':') return null;
            const val_end = skipJsonValue(s, i + 1, depth + 1);
            if (val_end == null) return null;
            i = skipWs(s, val_end.?);
            if (i >= s.len) return null;
            if (s[i] == '}') return i + 1;
            if (s[i] != ',') return null;
            i = skipWs(s, i + 1);
        }
    }
    return null;
}

/// Counts logical Unicode characters inside a validated JSON string literal `s[start..end]`.
fn countJsonStringChars(s: string, start: usize, end: usize) -> usize {
    let chars: usize = 0;
    let i = start + 1;
    const inner_end = end - 1;
    while (i < inner_end) {
        const c = s[i];
        if (c == '\\') {
            if (i + 1 < inner_end && s[i + 1] == 'u') {
                chars += 1;
                i += 6;
            } else {
                chars += 1;
                i += 2;
            }
        } else if ((c & 0xC0) != 0x80) {
            // Count UTF-8 leading bytes (ASCII or multi-byte start)
            chars += 1;
            i += 1;
        } else {
            i += 1;
        }
    }
    return chars;
}

fn countArrayItems(s: string, start: usize) -> usize {
    let i = skipWs(s, start + 1);
    if (i < s.len && s[i] == ']') {
        return 0;
    }
    let count: usize = 0;
    while (i < s.len) {
        const next_opt = skipJsonValue(s, i, 1 as usize);
        if (next_opt == null) break;
        count += 1;
        i = skipWs(s, next_opt.?);
        if (i >= s.len || s[i] == ']') break;
        if (s[i] == ',') {
            i = skipWs(s, i + 1);
        }
    }
    return count;
}

// ---------------------------------------------------------------------------
// Public API: Type Detection & Validation
// ---------------------------------------------------------------------------

/// Detects the RFC 8259 JSON type of `json` (distinguishing `Integer` vs `Number`).
/// Returns `JsonType.Any` if `json` is empty or syntactically invalid.
pub fn detectJsonType(json: string) -> JsonType {
    const start = skipWs(json, 0 as usize);
    const end_opt = skipJsonValue(json, start, 0 as usize);
    if (end_opt == null) {
        return JsonType.Any;
    }
    if (skipWs(json, end_opt.?) != json.len) {
        return JsonType.Any;
    }

    const c = json[start];
    if (c == 'n') return JsonType.Null;
    if (c == 't' || c == 'f') return JsonType.Boolean;
    if (c == '"') return JsonType.String;
    if (c == '[') return JsonType.Array;
    if (c == '{') return JsonType.Object;
    if (c == '-' || isDigit(c)) {
        const num = parseJsonNumber(json, start);
        return if (num.is_int) JsonType.Integer else JsonType.Number;
    }
    return JsonType.Any;
}

fn addErr(res: ValidationResult, code: string, msg: string, path: string) -> ValidationResult {
    const err = if (res.error_count == 0)
        ValidationError{
            code: code,
            message: msg,
            path: path,
        }
    else
        res.first_error;
    return ValidationResult{
        valid: false,
        error_count: res.error_count + 1,
        first_error: err,
    };
}

/// Validates a raw JSON string against `rule` with zero heap allocation.
pub fn validateJson(json: string, rule: SchemaRule) -> ValidationResult {
    let res = ValidationResult.ok();

    const start = skipWs(json, 0 as usize);
    const end_opt = skipJsonValue(json, start, 0 as usize);
    if (end_opt == null || skipWs(json, end_opt.?) != json.len) {
        return addErr(res, "invalid_json", "input is not valid RFC 8259 JSON", "$");
    }
    const end = end_opt.?;
    const trimmed = json[start..end];
    const actual_type = detectJsonType(trimmed);

    // 1. Type constraint
    if (rule.expected_type != JsonType.Any) {
        let type_ok = (actual_type == rule.expected_type);
        if (rule.expected_type == JsonType.Number && actual_type == JsonType.Integer) {
            type_ok = true;
        }
        if (!type_ok) {
            return addErr(res, "type", "JSON value type does not match schema", "$");
        }
    }

    // 2. String constraints (minLength / maxLength)
    if (actual_type == JsonType.String) {
        const str_len = countJsonStringChars(json, start, end);
        if (rule.has_min_len && str_len < rule.min_len) {
            res = addErr(res, "minLength", "string length is below minLength", "$");
        }
        if (rule.has_max_len && str_len > rule.max_len) {
            res = addErr(res, "maxLength", "string length exceeds maxLength", "$");
        }
    }

    // 3. Number / Integer constraints (minimum / maximum / exclusive)
    if (actual_type == JsonType.Integer || actual_type == JsonType.Number) {
        const num = parseJsonNumber(json, start);
        if (rule.has_min_val) {
            if (rule.exclusive_min) {
                if (num.val <= rule.min_val) {
                    res = addErr(res, "exclusiveMinimum", "number must be strictly greater than minimum", "$");
                }
            } else if (num.val < rule.min_val) {
                res = addErr(res, "minimum", "number is below minimum", "$");
            }
        }
        if (rule.has_max_val) {
            if (rule.exclusive_max) {
                if (num.val >= rule.max_val) {
                    res = addErr(res, "exclusiveMaximum", "number must be strictly less than maximum", "$");
                }
            } else if (num.val > rule.max_val) {
                res = addErr(res, "maximum", "number exceeds maximum", "$");
            }
        }
    }

    // 4. Array constraints (minItems / maxItems)
    if (actual_type == JsonType.Array) {
        const items = countArrayItems(json, start);
        if (rule.has_min_len && items < rule.min_len) {
            res = addErr(res, "minItems", "array has fewer items than minItems", "$");
        }
        if (rule.has_max_len && items > rule.max_len) {
            res = addErr(res, "maxItems", "array has more items than maxItems", "$");
        }
    }

    // 5. Object constraints (required keys at top level)
    if (actual_type == JsonType.Object && rule.required_count > 0) {
        let found: [8]bool = [false, false, false, false, false, false, false, false];
        let i = skipWs(json, start + 1);
        if (i < end && json[i] != '}') {
            while (i < end) {
                const k_end_opt = skipJsonString(json, i);
                if (k_end_opt == null) break;
                const k_end = k_end_opt.?;
                const key_raw = json[(i + 1)..(k_end - 1)];

                let rk: usize = 0;
                while (rk < rule.required_count) {
                    if (eqStr(key_raw, rule.required_keys[rk])) {
                        found[rk] = true;
                    }
                    rk += 1;
                }

                i = skipWs(json, k_end);
                if (i >= end || json[i] != ':') break;
                const v_end_opt = skipJsonValue(json, i + 1, 1 as usize);
                if (v_end_opt == null) break;
                i = skipWs(json, v_end_opt.?);
                if (i >= end || json[i] == '}') break;
                if (json[i] == ',') {
                    i = skipWs(json, i + 1);
                }
            }
        }

        let rk: usize = 0;
        while (rk < rule.required_count) {
            if (!found[rk]) {
                res = addErr(res, "required", "missing required object property", rule.required_keys[rk]);
            }
            rk += 1;
        }
    }

    // 6. Enum allowlist constraint
    if (rule.enum_count > 0) {
        let matched = false;
        const inner_str = if (actual_type == JsonType.String && trimmed.len >= 2)
            trimmed[1..(trimmed.len - 1)]
        else
            trimmed;

        let e_i: usize = 0;
        while (e_i < rule.enum_count) {
            const allowed = rule.enum_values[e_i];
            if (eqStr(trimmed, allowed) || eqStr(inner_str, allowed)) {
                matched = true;
                break;
            }
            e_i += 1;
        }
        if (!matched) {
            res = addErr(res, "enum", "value is not in allowed enum list", "$");
        }
    }

    return res;
}
