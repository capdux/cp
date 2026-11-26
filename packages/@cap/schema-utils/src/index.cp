// @cap/schema-utils — Zero-heap configuration schema validator (clean-room CP)

pub const SCHU_OK: u32 = 0;
pub const SCHU_ERR_REQUIRED: u32 = 1;
pub const SCHU_ERR_MIN_LEN: u32 = 2;
pub const SCHU_ERR_MAX_LEN: u32 = 3;
pub const SCHU_ERR_INT_MIN: u32 = 4;
pub const SCHU_ERR_INT_MAX: u32 = 5;
pub const SCHU_ERR_ENUM: u32 = 6;

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

fn schuStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
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
