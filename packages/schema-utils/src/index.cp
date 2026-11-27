// @cap/schema-utils — Zero-heap configuration schema validator & webpack-style error formatter
// Clean-room MIT port of npm `schema-utils` (`validate`, `formatValidationError`).

pub const SCHU_OK: u32 = 0;
pub const SCHU_ERR_REQUIRED: u32 = 1;
pub const SCHU_ERR_MIN_LEN: u32 = 2;
pub const SCHU_ERR_MAX_LEN: u32 = 3;
pub const SCHU_ERR_INT_MIN: u32 = 4;
pub const SCHU_ERR_INT_MAX: u32 = 5;
pub const SCHU_ERR_ENUM: u32 = 6;
pub const SCHU_ERR_TYPE: u32 = 7;
pub const SCHU_ERR_PATTERN: u32 = 8;
pub const SCHU_ERR_ADDITIONAL_PROP: u32 = 9;
pub const SCHU_ERR_MULTIPLE_OF: u32 = 10;

pub const SCHU_MAX_ERRORS: usize = 16;

pub enum SchemaType {
    Any,
    String,
    Integer,
    Number,
    Boolean,
    Array,
    Object,
    Null,
}

pub fn schemaTypeName(t: SchemaType): string {
    return switch (t) {
        .Any => "any",
        .String => "string",
        .Integer => "integer",
        .Number => "number",
        .Boolean => "boolean",
        .Array => "array",
        .Object => "object",
        .Null => "null",
    };
}

pub struct FieldConstraint {
    name: string,
    required: bool,
    min_len: usize,
    max_len: usize,
    has_int_range: bool,
    min_int: i64,
    max_int: i64,
}

pub struct ValidationResult {
    valid: bool,
    error_code: u32,
    message: string,
}

pub struct SchemaProperty {
    pub name: string,
    pub expected_type: SchemaType,
    pub required: bool,
    pub min_length: usize,
    pub max_length: usize,
    pub has_minimum: bool,
    pub minimum: i64,
    pub has_maximum: bool,
    pub maximum: i64,
    pub pattern: string,
    pub multiple_of: i64,

    pub static fn stringProp(
        name: string,
        required: bool,
        min_length: usize,
        max_length: usize,
        pattern: string,
    ): SchemaProperty {
        return SchemaProperty {
            name: name,
            expected_type: .String,
            required: required,
            min_length: min_length,
            max_length: max_length,
            has_minimum: false,
            minimum: 0,
            has_maximum: false,
            maximum: 0,
            pattern: pattern,
            multiple_of: 0,
        };
    }

    pub static fn intProp(
        name: string,
        required: bool,
        has_minimum: bool,
        minimum: i64,
        has_maximum: bool,
        maximum: i64,
    ): SchemaProperty {
        return SchemaProperty {
            name: name,
            expected_type: .Integer,
            required: required,
            min_length: 0,
            max_length: 0,
            has_minimum: has_minimum,
            minimum: minimum,
            has_maximum: has_maximum,
            maximum: maximum,
            pattern: "",
            multiple_of: 0,
        };
    }

    pub static fn boolProp(name: string, required: bool): SchemaProperty {
        return SchemaProperty {
            name: name,
            expected_type: .Boolean,
            required: required,
            min_length: 0,
            max_length: 0,
            has_minimum: false,
            minimum: 0,
            has_maximum: false,
            maximum: 0,
            pattern: "",
            multiple_of: 0,
        };
    }
}

pub struct SchemaValue {
    pub name: string,
    pub val_type: SchemaType,
    pub str_val: string,
    pub int_val: i64,
    pub bool_val: bool,

    pub static fn str(name: string, v: string): SchemaValue {
        return SchemaValue { name: name, val_type: .String, str_val: v, int_val: 0, bool_val: false };
    }

    pub static fn int(name: string, v: i64): SchemaValue {
        return SchemaValue { name: name, val_type: .Integer, str_val: "", int_val: v, bool_val: false };
    }

    pub static fn boolean(name: string, v: bool): SchemaValue {
        return SchemaValue { name: name, val_type: .Boolean, str_val: "", int_val: 0, bool_val: v };
    }
}

pub struct ObjectSchema {
    pub title: string,
    pub properties: []const SchemaProperty,
    pub additional_properties: bool,
}

pub struct ValidationError {
    pub field: string,
    pub code: u32,
    pub message: string,
    pub expected: string,
    pub actual: string,
}

pub struct ValidationReport {
    pub valid: bool,
    pub error_count: usize,
    pub errors: [16]ValidationError,
}

fn schuStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn schuWriteStr(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn schuOk(): ValidationResult {
    return ValidationResult{
        valid: true,
        error_code: SCHU_OK,
        message: "",
    };
}

fn schuFail(code: u32, msg: string): ValidationResult {
    return ValidationResult{
        valid: false,
        error_code: code,
        message: msg,
    };
}

pub fn validateStringField(
    value: string,
    present: bool,
    c: FieldConstraint,
): ValidationResult {
    if (!present) {
        if (c.required) {
            return schuFail(SCHU_ERR_REQUIRED, "required field is missing");
        }
        return schuOk();
    }
    if (c.min_len > 0 && value.len < c.min_len) {
        return schuFail(SCHU_ERR_MIN_LEN, "string length is below min_len");
    }
    if (c.max_len > 0 && value.len > c.max_len) {
        return schuFail(SCHU_ERR_MAX_LEN, "string length exceeds max_len");
    }
    return schuOk();
}

pub fn validateIntField(
    value: i64,
    present: bool,
    c: FieldConstraint,
): ValidationResult {
    if (!present) {
        if (c.required) {
            return schuFail(SCHU_ERR_REQUIRED, "required field is missing");
        }
        return schuOk();
    }
    if (c.has_int_range) {
        if (value < c.min_int) {
            return schuFail(SCHU_ERR_INT_MIN, "integer value is below min_int");
        }
        if (value > c.max_int) {
            return schuFail(SCHU_ERR_INT_MAX, "integer value exceeds max_int");
        }
    }
    return schuOk();
}

pub fn validateEnumField(
    value: string,
    allowed: []const string,
): ValidationResult {
    let i: usize = 0;
    while (i < allowed.len) : (i += 1) {
        if (schuStrEq(value, allowed[i])) {
            return schuOk();
        }
    }
    return schuFail(SCHU_ERR_ENUM, "value is not in allowed enum list");
}

// --- Lightweight zero-heap JSON Schema regex engine (^, $, ., *, +, ?, [a-z], \d, \w, \s) ---

fn schuMatchClass(pat: string, start: usize, end: usize, c: u8): bool {
    let i = start;
    let negated = false;
    if (i < end && pat[i] == '^') {
        negated = true;
        i += 1;
    }
    let matched = false;
    while (i < end) : (i += 1) {
        if (i + 2 < end && pat[i + 1] == '-') {
            if (c >= pat[i] && c <= pat[i + 2]) {
                matched = true;
            }
            i += 2;
        } else if (pat[i] == c) {
            matched = true;
        }
    }
    return if (negated) { !matched } else { matched };
}

fn schuAtomLen(pat: string, pi: usize): usize {
    if (pi >= pat.len) return 0;
    if (pat[pi] == '\\' && pi + 1 < pat.len) return 2;
    if (pat[pi] == '[') {
        let j = pi + 1;
        while (j < pat.len && pat[j] != ']') : (j += 1) {}
        if (j < pat.len) return (j - pi) + 1;
    }
    return 1;
}

fn schuMatchAtom(pat: string, pi: usize, atom_len: usize, c: u8): bool {
    if (atom_len == 1) {
        if (pat[pi] == '.') return c != '\n';
        return pat[pi] == c;
    }
    if (pat[pi] == '\\' && atom_len == 2) {
        const esc = pat[pi + 1];
        if (esc == 'd') return c >= '0' && c <= '9';
        if (esc == 'w') {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        }
        if (esc == 's') return c == ' ' || c == '\t' || c == '\r' || c == '\n';
        return c == esc;
    }
    if (pat[pi] == '[' && atom_len >= 2) {
        return schuMatchClass(pat, pi + 1, pi + atom_len - 1, c);
    }
    return false;
}

fn schuMatchHere(pat: string, pi: usize, text: string, ti: usize): bool {
    if (pi >= pat.len) return true;
    if (pat[pi] == '$' && pi + 1 == pat.len) {
        return ti == text.len;
    }
    const alen = schuAtomLen(pat, pi);
    const next_pi = pi + alen;
    if (next_pi < pat.len) {
        const q = pat[next_pi];
        if (q == '*') {
            let k = ti;
            while (true) {
                if (schuMatchHere(pat, next_pi + 1, text, k)) return true;
                if (k >= text.len || !schuMatchAtom(pat, pi, alen, text[k])) break;
                k += 1;
            }
            return false;
        }
        if (q == '+') {
            if (ti >= text.len || !schuMatchAtom(pat, pi, alen, text[ti])) return false;
            let k = ti + 1;
            while (true) {
                if (schuMatchHere(pat, next_pi + 1, text, k)) return true;
                if (k >= text.len || !schuMatchAtom(pat, pi, alen, text[k])) break;
                k += 1;
            }
            return false;
        }
        if (q == '?') {
            if (ti < text.len && schuMatchAtom(pat, pi, alen, text[ti])) {
                if (schuMatchHere(pat, next_pi + 1, text, ti + 1)) return true;
            }
            return schuMatchHere(pat, next_pi + 1, text, ti);
        }
    }
    if (ti < text.len && schuMatchAtom(pat, pi, alen, text[ti])) {
        return schuMatchHere(pat, next_pi, text, ti + 1);
    }
    return false;
}

/// Evaluates whether `text` matches a JSON Schema `pattern` regex (unanchored unless `^`/`$` used).
pub fn matchPattern(pattern: string, text: string): bool {
    if (pattern.len == 0) return true;
    if (pattern[0] == '^') {
        return schuMatchHere(pattern, 1, text, 0);
    }
    let pos: usize = 0;
    while (pos <= text.len) : (pos += 1) {
        if (schuMatchHere(pattern, 0, text, pos)) return true;
    }
    return false;
}

/// Validates a string against a regex `pattern`.
pub fn validatePatternField(value: string, pattern: string): ValidationResult {
    if (matchPattern(pattern, value)) {
        return schuOk();
    }
    return schuFail(SCHU_ERR_PATTERN, "string does not match required pattern");
}

fn schuTypeMatches(expected: SchemaType, actual: SchemaType): bool {
    if (expected == .Any) return true;
    if (expected == .Number && actual == .Integer) return true;
    return expected == actual;
}

fn schuPushError(report: *ValidationReport, err: ValidationError) {
    report.valid = false;
    if (report.error_count < SCHU_MAX_ERRORS) {
        report.errors[report.error_count] = err;
        report.error_count += 1;
    }
}

/// Validates a slice of `SchemaValue` entries against an `ObjectSchema` (`schema-utils.validate`).
/// Checks `type`, `required`, `minimum`, `maximum`, `minLength`, `maxLength`, `pattern`, `multipleOf`, and `additionalProperties`.
pub fn validate(schema: ObjectSchema, values: []const SchemaValue): ValidationReport {
    let errs: [16]ValidationError = undefined;
    let ei: usize = 0;
    while (ei < SCHU_MAX_ERRORS) : (ei += 1) {
        errs[ei] = ValidationError { field: "", code: SCHU_OK, message: "", expected: "", actual: "" };
    }
    let report = ValidationReport { valid: true, error_count: 0, errors: errs };

    // 1. Validate each declared schema property
    let pi: usize = 0;
    while (pi < schema.properties.len) : (pi += 1) {
        const prop = schema.properties[pi];
        let found = false;
        let vi: usize = 0;
        while (vi < values.len) : (vi += 1) {
            if (schuStrEq(values[vi].name, prop.name)) {
                found = true;
                const val = values[vi];
                if (!schuTypeMatches(prop.expected_type, val.val_type)) {
                    schuPushError(&report, ValidationError {
                        field: prop.name,
                        code: SCHU_ERR_TYPE,
                        message: "should be of expected type",
                        expected: schemaTypeName(prop.expected_type),
                        actual: schemaTypeName(val.val_type),
                    });
                    break;
                }
                if (val.val_type == .String) {
                    if (prop.min_length > 0 && val.str_val.len < prop.min_length) {
                        schuPushError(&report, ValidationError {
                            field: prop.name,
                            code: SCHU_ERR_MIN_LEN,
                            message: "should be longer than minLength",
                            expected: "minLength",
                            actual: val.str_val,
                        });
                    }
                    if (prop.max_length > 0 && val.str_val.len > prop.max_length) {
                        schuPushError(&report, ValidationError {
                            field: prop.name,
                            code: SCHU_ERR_MAX_LEN,
                            message: "should be shorter than maxLength",
                            expected: "maxLength",
                            actual: val.str_val,
                        });
                    }
                    if (prop.pattern.len > 0 && !matchPattern(prop.pattern, val.str_val)) {
                        schuPushError(&report, ValidationError {
                            field: prop.name,
                            code: SCHU_ERR_PATTERN,
                            message: "should match pattern",
                            expected: prop.pattern,
                            actual: val.str_val,
                        });
                    }
                } else if (val.val_type == .Integer || val.val_type == .Number) {
                    if (prop.has_minimum && val.int_val < prop.minimum) {
                        schuPushError(&report, ValidationError {
                            field: prop.name,
                            code: SCHU_ERR_INT_MIN,
                            message: "should be >= minimum",
                            expected: "minimum",
                            actual: "",
                        });
                    }
                    if (prop.has_maximum && val.int_val > prop.maximum) {
                        schuPushError(&report, ValidationError {
                            field: prop.name,
                            code: SCHU_ERR_INT_MAX,
                            message: "should be <= maximum",
                            expected: "maximum",
                            actual: "",
                        });
                    }
                    if (prop.multiple_of > 0 && @rem(val.int_val, prop.multiple_of) != 0) {
                        schuPushError(&report, ValidationError {
                            field: prop.name,
                            code: SCHU_ERR_MULTIPLE_OF,
                            message: "should be multipleOf constraint",
                            expected: "multipleOf",
                            actual: "",
                        });
                    }
                }
                break;
            }
        }
        if (!found && prop.required) {
            schuPushError(&report, ValidationError {
                field: prop.name,
                code: SCHU_ERR_REQUIRED,
                message: "is required",
                expected: schemaTypeName(prop.expected_type),
                actual: "undefined",
            });
        }
    }

    // 2. Check additionalProperties: false
    if (!schema.additional_properties) {
        let vi: usize = 0;
        while (vi < values.len) : (vi += 1) {
            let known = false;
            let k: usize = 0;
            while (k < schema.properties.len) : (k += 1) {
                if (schuStrEq(values[vi].name, schema.properties[k].name)) {
                    known = true;
                    break;
                }
            }
            if (!known) {
                schuPushError(&report, ValidationError {
                    field: values[vi].name,
                    code: SCHU_ERR_ADDITIONAL_PROP,
                    message: "is an unknown property",
                    expected: "known property",
                    actual: values[vi].name,
                });
            }
        }
    }

    return report;
}

/// Formats a single `ValidationError` into a human-readable webpack `schema-utils` message.
pub fn formatValidationError(schema_name: string, err: ValidationError, out: []u8): usize {
    let pos: usize = 0;
    pos = schuWriteStr(out, pos, "Invalid configuration object. ");
    pos = schuWriteStr(out, pos, if (schema_name.len > 0) { schema_name } else { "Object" });
    pos = schuWriteStr(out, pos, " does not match the API schema.\n - configuration.");
    pos = schuWriteStr(out, pos, err.field);
    pos = schuWriteStr(out, pos, " ");
    pos = schuWriteStr(out, pos, err.message);
    if (err.expected.len > 0 && err.actual.len > 0) {
        pos = schuWriteStr(out, pos, " (expected ");
        pos = schuWriteStr(out, pos, err.expected);
        pos = schuWriteStr(out, pos, ", got ");
        pos = schuWriteStr(out, pos, err.actual);
        pos = schuWriteStr(out, pos, ")");
    }
    pos = schuWriteStr(out, pos, ".");
    return pos;
}

/// Formats all errors in a `ValidationReport` into `out`.
pub fn formatValidationReport(schema_name: string, report: ValidationReport, out: []u8): usize {
    if (report.valid || report.error_count == 0) return 0;
    let pos: usize = 0;
    pos = schuWriteStr(out, pos, "Invalid configuration object. ");
    pos = schuWriteStr(out, pos, if (schema_name.len > 0) { schema_name } else { "Object" });
    pos = schuWriteStr(out, pos, " does not match the API schema.");
    let i: usize = 0;
    while (i < report.error_count) : (i += 1) {
        const err = report.errors[i];
        pos = schuWriteStr(out, pos, "\n - configuration.");
        pos = schuWriteStr(out, pos, err.field);
        pos = schuWriteStr(out, pos, " ");
        pos = schuWriteStr(out, pos, err.message);
        pos = schuWriteStr(out, pos, ".");
    }
    return pos;
}
