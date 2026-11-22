// Origin: Pure-CP Stdlib (AI Agent Tool-Call Argument JSON Schema Validator) | Created: 2026-11-22

fn isWs(b: u8): bool {
    return b == 32 || b == 9 || b == 10 || b == 13;
}

fn isDigit(b: u8): bool {
    return b >= 48 && b <= 57;
}

fn hexVal(b: u8): u16? {
    if (b >= 48 && b <= 57) return (b - 48) as u16;
    if (b >= 65 && b <= 70) return (b - 55) as u16;
    if (b >= 97 && b <= 102) return (b - 87) as u16;
    return null;
}

fn skipWs(s: string, start: usize): usize {
    let i = start;
    while (i < s.len && isWs(s[i])) {
        i += 1;
    }
    return i;
}

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Expected JSON value type for a `FieldSpec`.
pub enum FieldType {
    Any,
    String,
    Number,
    Integer,
    Boolean,
    Array,
    Object,
    Null,
}

/// Schema specification for a single top-level tool-call JSON argument field.
pub struct FieldSpec {
    name: string,
    kind: FieldType,
    required: bool,
    has_min_num: bool,
    min_num: f64,
    has_max_num: bool,
    max_num: f64,
    min_len: usize,
    max_len: usize,
    enum_values: []const string,
}

/// Error classification returned inside `ValidationReport`.
pub enum SchemaErrorKind {
    Ok,
    InvalidJsonObject,
    MissingRequiredField,
    TypeMismatch,
    NumberOutOfRange,
    LengthOutOfRange,
    EnumMismatch,
    UnknownField,
    DuplicateField,
}

/// Structured validation outcome returned by `validateToolArgs`.
pub struct ValidationReport {
    ok: bool,
    error_kind: SchemaErrorKind,
    field_name: string,
    checked_fields: usize,
    unknown_fields: usize,
}

struct ParsedStr {
    next_pos: usize,
    raw_start: usize,
    raw_end: usize,
    decoded_len: usize,
}

fn parseJsonString(s: string, start: usize): ParsedStr? {
    if (start >= s.len || s[start] != 34) {
        return null;
    }
    let i = start + 1;
    const raw_start = i;
    let decoded_len: usize = 0;

    while (i < s.len) {
        const c = s[i];
        if (c == 34) {
            return ParsedStr {
                next_pos: i + 1,
                raw_start: raw_start,
                raw_end: i,
                decoded_len: decoded_len,
            };
        }
        if (c < 32) {
            return null;
        }
        if (c == 92) { // '\\'
            i += 1;
            if (i >= s.len) {
                return null;
            }
            const esc = s[i];
            if (esc == 117) { // 'u' -> \uXXXX
                if (i + 4 >= s.len) {
                    return null;
                }
                const d0 = hexVal(s[i + 1]) ?? return null;
                const d1 = hexVal(s[i + 2]) ?? return null;
                const d2 = hexVal(s[i + 3]) ?? return null;
                const d3 = hexVal(s[i + 4]) ?? return null;
                const cp: u16 = (d0 << 12) | (d1 << 8) | (d2 << 4) | d3;
                if (cp < 0x80) {
                    decoded_len += 1;
                } else if (cp < 0x800) {
                    decoded_len += 2;
                } else {
                    decoded_len += 3;
                }
                i += 5;
            } else if (
                esc == 34 || esc == 92 || esc == 47 ||
                esc == 98 || esc == 102 || esc == 110 ||
                esc == 114 || esc == 116
            ) {
                decoded_len += 1;
                i += 1;
            } else {
                return null;
            }
        } else {
            decoded_len += 1;
            i += 1;
        }
    }
    return null;
}

fn rawJsonStringEquals(raw_inner: string, target: string): bool {
    let i: usize = 0;
    let t: usize = 0;
    while (i < raw_inner.len) {
        const c = raw_inner[i];
        if (c == 92) {
            i += 1;
            if (i >= raw_inner.len) return false;
            const esc = raw_inner[i];
            if (esc == 117) {
                if (i + 4 >= raw_inner.len) return false;
                const d0 = hexVal(raw_inner[i + 1]) ?? return false;
                const d1 = hexVal(raw_inner[i + 2]) ?? return false;
                const d2 = hexVal(raw_inner[i + 3]) ?? return false;
                const d3 = hexVal(raw_inner[i + 4]) ?? return false;
                const cp: u16 = (d0 << 12) | (d1 << 8) | (d2 << 4) | d3;
                i += 5;
                if (cp < 0x80) {
                    if (t >= target.len || target[t] != (cp as u8)) return false;
                    t += 1;
                } else if (cp < 0x800) {
                    if (t + 1 >= target.len) return false;
                    if (target[t] != ((0xC0 | (cp >> 6)) as u8)) return false;
                    if (target[t + 1] != ((0x80 | (cp & 0x3F)) as u8)) return false;
                    t += 2;
                } else {
                    if (t + 2 >= target.len) return false;
                    if (target[t] != ((0xE0 | (cp >> 12)) as u8)) return false;
                    if (target[t + 1] != ((0x80 | ((cp >> 6) & 0x3F)) as u8)) return false;
                    if (target[t + 2] != ((0x80 | (cp & 0x3F)) as u8)) return false;
                    t += 3;
                }
            } else {
                let decoded: u8 = 0;
                if (esc == 110) decoded = 10;
                else if (esc == 114) decoded = 13;
                else if (esc == 116) decoded = 9;
                else if (esc == 34) decoded = 34;
                else if (esc == 92) decoded = 92;
                else if (esc == 47) decoded = 47;
                else if (esc == 98) decoded = 8;
                else if (esc == 102) decoded = 12;
                else return false;
                if (t >= target.len || target[t] != decoded) return false;
                t += 1;
                i += 1;
            }
        } else {
            if (t >= target.len || target[t] != c) return false;
            t += 1;
            i += 1;
        }
    }
    return t == target.len;
}

struct ParsedNum {
    next_pos: usize,
    value: f64,
    is_integer: bool,
}

fn parseJsonNumber(s: string, start: usize): ParsedNum? {
    let i = start;
    if (i >= s.len) return null;

    let neg: bool = false;
    if (s[i] == 45) { // '-'
        neg = true;
        i += 1;
        if (i >= s.len) return null;
    }

    let val: f64 = 0.0;
    if (s[i] == 48) { // '0'
        i += 1;
        if (i < s.len && isDigit(s[i])) {
            return null; // leading zero forbidden in JSON
        }
    } else if (s[i] >= 49 && s[i] <= 57) {
        while (i < s.len && isDigit(s[i])) {
            val = val * 10.0 + ((s[i] - 48) as f64);
            i += 1;
        }
    } else {
        return null;
    }

    let has_frac: bool = false;
    let frac_scale: f64 = 0.1;
    if (i < s.len && s[i] == 46) { // '.'
        has_frac = true;
        i += 1;
        if (i >= s.len || !isDigit(s[i])) {
            return null;
        }
        while (i < s.len && isDigit(s[i])) {
            val += ((s[i] - 48) as f64) * frac_scale;
            frac_scale *= 0.1;
            i += 1;
        }
    }

    let has_exp: bool = false;
    if (i < s.len && (s[i] == 101 || s[i] == 69)) { // 'e' or 'E'
        has_exp = true;
        i += 1;
        let exp_neg: bool = false;
        if (i < s.len && (s[i] == 43 || s[i] == 45)) {
            exp_neg = s[i] == 45;
            i += 1;
        }
        if (i >= s.len || !isDigit(s[i])) {
            return null;
        }
        let exp_val: i32 = 0;
        while (i < s.len && isDigit(s[i])) {
            if (exp_val < 1000) {
                exp_val = exp_val * 10 + ((s[i] - 48) as i32);
            }
            i += 1;
        }
        let step: i32 = 0;
        while (step < exp_val) {
            if (exp_neg) {
                val *= 0.1;
            } else {
                val *= 10.0;
            }
            step += 1;
        }
    }

    if (neg) {
        val = -val;
    }

    return ParsedNum {
        next_pos: i,
        value: val,
        is_integer: !has_frac && !has_exp,
    };
}

fn skipJsonContainer(s: string, start: usize): usize? {
    if (start >= s.len) return null;
    const open = s[start];
    if (open != 123 && open != 91) return null;

    let depth: usize = 0;
    let i = start;
    while (i < s.len) {
        const c = s[i];
        if (c == 34) {
            const ps = parseJsonString(s, i) ?? return null;
            i = ps.next_pos;
            continue;
        }
        if (c == 123 || c == 91) {
            depth += 1;
        } else if (c == 125 || c == 93) {
            if (depth == 0) return null;
            depth -= 1;
            if (depth == 0) {
                return i + 1;
            }
        }
        i += 1;
    }
    return null;
}

fn matchLiteral(s: string, pos: usize, lit: string): bool {
    if (pos + lit.len > s.len) return false;
    let i: usize = 0;
    while (i < lit.len) {
        if (s[pos + i] != lit[i]) return false;
        i += 1;
    }
    return true;
}

struct KeySpan {
    start: usize,
    end: usize,
}

fn failReport(kind: SchemaErrorKind, field: string, checked: usize, unknown: usize): ValidationReport {
    return ValidationReport {
        ok: false,
        error_kind: kind,
        field_name: field,
        checked_fields: checked,
        unknown_fields: unknown,
    };
}

/// Validates a raw JSON object string directly against `specs` with zero heap allocations.
/// Checks JSON object syntax, duplicate keys, unknown fields, required fields, exact types
/// (`Integer` vs `Number`), numeric bounds `[min_num, max_num]`, decoded string length `[min_len, max_len]`,
/// and string enum membership.
pub fn validateToolArgs(
    json_obj: string,
    specs: []const FieldSpec,
    allow_unknown_fields: bool,
): ValidationReport {
    let pos = skipWs(json_obj, 0);
    if (pos >= json_obj.len || json_obj[pos] != 123) { // '{'
        return failReport(SchemaErrorKind.InvalidJsonObject, "", 0, 0);
    }
    pos = skipWs(json_obj, pos + 1);

    let seen_spec_bits: [4]u64 = [0, 0, 0, 0];
    let seen_keys: [64]KeySpan = [KeySpan { start: 0, end: 0 }; 64];
    let seen_key_count: usize = 0;
    let checked_fields: usize = 0;
    let unknown_fields: usize = 0;

    if (pos < json_obj.len && json_obj[pos] == 125) { // empty object {}
        pos = skipWs(json_obj, pos + 1);
        if (pos != json_obj.len) {
            return failReport(SchemaErrorKind.InvalidJsonObject, "", 0, 0);
        }
    } else {
        while (true) {
            const key_parsed = parseJsonString(json_obj, pos) ??
                return failReport(SchemaErrorKind.InvalidJsonObject, "", checked_fields, unknown_fields);
            const key_raw: string = json_obj[key_parsed.raw_start..key_parsed.raw_end];
            pos = skipWs(json_obj, key_parsed.next_pos);

            if (pos >= json_obj.len || json_obj[pos] != 58) { // ':'
                return failReport(SchemaErrorKind.InvalidJsonObject, key_raw, checked_fields, unknown_fields);
            }
            pos = skipWs(json_obj, pos + 1);
            if (pos >= json_obj.len) {
                return failReport(SchemaErrorKind.InvalidJsonObject, key_raw, checked_fields, unknown_fields);
            }

            // Duplicate key detection
            let dk: usize = 0;
            while (dk < seen_key_count) {
                const prev_raw: string = json_obj[seen_keys[dk].start..seen_keys[dk].end];
                if (strEq(prev_raw, key_raw)) {
                    return failReport(SchemaErrorKind.DuplicateField, key_raw, checked_fields, unknown_fields);
                }
                dk += 1;
            }
            if (seen_key_count < 64) {
                seen_keys[seen_key_count] = KeySpan { start: key_parsed.raw_start, end: key_parsed.raw_end };
                seen_key_count += 1;
            }

            // Look up matching FieldSpec
            let spec_found: bool = false;
            let spec_idx: usize = 0;
            let si: usize = 0;
            while (si < specs.len) {
                if (rawJsonStringEquals(key_raw, specs[si].name)) {
                    spec_found = true;
                    spec_idx = si;
                    break;
                }
                si += 1;
            }

            // Parse value token
            const vc = json_obj[pos];
            let val_kind = FieldType.Null;
            let val_num: f64 = 0.0;
            let val_is_int: bool = false;
            let val_str_raw: string = "";
            let val_str_len: usize = 0;

            if (vc == 34) { // string
                const vs = parseJsonString(json_obj, pos) ??
                    return failReport(SchemaErrorKind.InvalidJsonObject, key_raw, checked_fields, unknown_fields);
                val_kind = FieldType.String;
                val_str_raw = json_obj[vs.raw_start..vs.raw_end];
                val_str_len = vs.decoded_len;
                pos = vs.next_pos;
            } else if (vc == 123) { // object
                const next_p = skipJsonContainer(json_obj, pos) ??
                    return failReport(SchemaErrorKind.InvalidJsonObject, key_raw, checked_fields, unknown_fields);
                val_kind = FieldType.Object;
                pos = next_p;
            } else if (vc == 91) { // array
                const next_p = skipJsonContainer(json_obj, pos) ??
                    return failReport(SchemaErrorKind.InvalidJsonObject, key_raw, checked_fields, unknown_fields);
                val_kind = FieldType.Array;
                pos = next_p;
            } else if (vc == 116 && matchLiteral(json_obj, pos, "true")) {
                val_kind = FieldType.Boolean;
                pos += 4;
            } else if (vc == 102 && matchLiteral(json_obj, pos, "false")) {
                val_kind = FieldType.Boolean;
                pos += 5;
            } else if (vc == 110 && matchLiteral(json_obj, pos, "null")) {
                val_kind = FieldType.Null;
                pos += 4;
            } else if (vc == 45 || isDigit(vc)) {
                const pn = parseJsonNumber(json_obj, pos) ??
                    return failReport(SchemaErrorKind.InvalidJsonObject, key_raw, checked_fields, unknown_fields);
                val_kind = if (pn.is_integer) FieldType.Integer else FieldType.Number;
                val_num = pn.value;
                val_is_int = pn.is_integer;
                pos = pn.next_pos;
            } else {
                return failReport(SchemaErrorKind.InvalidJsonObject, key_raw, checked_fields, unknown_fields);
            }

            if (!spec_found) {
                unknown_fields += 1;
                if (!allow_unknown_fields) {
                    return failReport(SchemaErrorKind.UnknownField, key_raw, checked_fields, unknown_fields);
                }
            } else {
                const spec = specs[spec_idx];
                if (spec_idx < 256) {
                    const word = spec_idx / 64;
                    const bit = (spec_idx % 64) as u6;
                    seen_spec_bits[word] |= (1 as u64) << bit;
                }
                checked_fields += 1;

                // Type check
                let type_ok: bool = false;
                match (spec.kind) {
                    FieldType.Any => type_ok = true,
                    FieldType.Number => type_ok = (val_kind == FieldType.Number || val_kind == FieldType.Integer),
                    FieldType.Integer => type_ok = (val_kind == FieldType.Integer && val_is_int),
                    _ => type_ok = (val_kind == spec.kind),
                }
                if (!type_ok) {
                    return failReport(SchemaErrorKind.TypeMismatch, spec.name, checked_fields, unknown_fields);
                }

                // Numeric range check
                if (val_kind == FieldType.Number || val_kind == FieldType.Integer) {
                    if (spec.has_min_num && val_num < spec.min_num) {
                        return failReport(SchemaErrorKind.NumberOutOfRange, spec.name, checked_fields, unknown_fields);
                    }
                    if (spec.has_max_num && val_num > spec.max_num) {
                        return failReport(SchemaErrorKind.NumberOutOfRange, spec.name, checked_fields, unknown_fields);
                    }
                }

                // String length & enum checks
                if (val_kind == FieldType.String) {
                    if (val_str_len < spec.min_len) {
                        return failReport(SchemaErrorKind.LengthOutOfRange, spec.name, checked_fields, unknown_fields);
                    }
                    if (spec.max_len > 0 && val_str_len > spec.max_len) {
                        return failReport(SchemaErrorKind.LengthOutOfRange, spec.name, checked_fields, unknown_fields);
                    }
                    if (spec.enum_values.len > 0) {
                        let in_enum: bool = false;
                        let ei: usize = 0;
                        while (ei < spec.enum_values.len) {
                            if (rawJsonStringEquals(val_str_raw, spec.enum_values[ei])) {
                                in_enum = true;
                                break;
                            }
                            ei += 1;
                        }
                        if (!in_enum) {
                            return failReport(SchemaErrorKind.EnumMismatch, spec.name, checked_fields, unknown_fields);
                        }
                    }
                }
            }

            pos = skipWs(json_obj, pos);
            if (pos >= json_obj.len) {
                return failReport(SchemaErrorKind.InvalidJsonObject, "", checked_fields, unknown_fields);
            }
            if (json_obj[pos] == 44) { // ','
                pos = skipWs(json_obj, pos + 1);
                continue;
            }
            if (json_obj[pos] == 125) { // '}'
                pos = skipWs(json_obj, pos + 1);
                if (pos != json_obj.len) {
                    return failReport(SchemaErrorKind.InvalidJsonObject, "", checked_fields, unknown_fields);
                }
                break;
            }
            return failReport(SchemaErrorKind.InvalidJsonObject, "", checked_fields, unknown_fields);
        }
    }

    // Check missing required fields
    let ri: usize = 0;
    while (ri < specs.len) {
        if (specs[ri].required) {
            let was_seen: bool = false;
            if (ri < 256) {
                const word = ri / 64;
                const bit = (ri % 64) as u6;
                was_seen = (seen_spec_bits[word] & ((1 as u64) << bit)) != 0;
            }
            if (!was_seen) {
                return failReport(SchemaErrorKind.MissingRequiredField, specs[ri].name, checked_fields, unknown_fields);
            }
        }
        ri += 1;
    }

    return ValidationReport {
        ok: true,
        error_kind: SchemaErrorKind.Ok,
        field_name: "",
        checked_fields: checked_fields,
        unknown_fields: unknown_fields,
    };
}

fn appendStr(buf: *[]u8, off: usize, s: string): usize? {
    if (off + s.len > buf.*.len) {
        return null;
    }
    let i: usize = 0;
    while (i < s.len) {
        buf.*[off + i] = s[i];
        i += 1;
    }
    return off + s.len;
}

/// Formats a structured LLM-ready self-correction hint from `report` into `out_buf`.
/// Returns the number of bytes written, or `null` if `out_buf` is too small.
pub fn formatValidationHint(report: ValidationReport, out_buf: *[]u8): usize? {
    if (report.ok || report.error_kind == SchemaErrorKind.Ok) {
        return appendStr(out_buf, 0, "Tool arguments valid");
    }

    let prefix: string = "";
    let include_field: bool = true;
    match (report.error_kind) {
        SchemaErrorKind.InvalidJsonObject => {
            prefix = "Tool argument validation failed: invalid JSON object";
            include_field = false;
        }
        SchemaErrorKind.MissingRequiredField => prefix = "Tool argument validation failed: missing required field '",
        SchemaErrorKind.TypeMismatch => prefix = "Tool argument validation failed: type mismatch for field '",
        SchemaErrorKind.NumberOutOfRange => prefix = "Tool argument validation failed: number out of range for field '",
        SchemaErrorKind.LengthOutOfRange => prefix = "Tool argument validation failed: string length out of range for field '",
        SchemaErrorKind.EnumMismatch => prefix = "Tool argument validation failed: invalid enum value for field '",
        SchemaErrorKind.UnknownField => prefix = "Tool argument validation failed: unknown field '",
        SchemaErrorKind.DuplicateField => prefix = "Tool argument validation failed: duplicate field '",
        SchemaErrorKind.Ok => return appendStr(out_buf, 0, "Tool arguments valid"),
    }

    let off = appendStr(out_buf, 0, prefix) ?? return null;
    if (include_field) {
        off = appendStr(out_buf, off, report.field_name) ?? return null;
        off = appendStr(out_buf, off, "'") ?? return null;
    }
    return off;
}
