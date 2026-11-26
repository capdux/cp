// Type definitions for CP

export class FieldConstraint {
    name: string,
    required: bool,
    min_len: usize,
    max_len: usize,
    has_int_range: bool,
    min_int: i64,
    max_int: i64,
}

export class ValidationResult {
    valid: bool,
    error_code: u32,
    message: string,
}

export fn schuStrEq(a: string, b: string): bool;
export fn schuOk(): ValidationResult;
export fn schuFail(code: u32, msg: string): ValidationResult;
export fn validateStringField(value: string, present: bool, c: FieldConstraint): ValidationResult;
export fn validateIntField(value: i64, present: bool, c: FieldConstraint): ValidationResult;
export fn validateEnumField(value: string, allowed: &[]string): ValidationResult;
