// Type definitions for CP

export enum SchemaType {
    Any,
    String,
    Integer,
    Number,
    Boolean,
    Array,
    Object,
    Null
}

export fn schemaTypeName(t: SchemaType): string;
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

export class SchemaProperty {
    name: string,
    expected_type: SchemaType,
    required: bool,
    min_length: usize,
    max_length: usize,
    has_minimum: bool,
    minimum: i64,
    has_maximum: bool,
    maximum: i64,
    pattern: string,
    multiple_of: i64,
    fn stringProp(name: string, required: bool, min_length: usize, max_length: usize, pattern: string): SchemaProperty;
    fn intProp(name: string, required: bool, has_minimum: bool, minimum: i64, has_maximum: bool, maximum: i64): SchemaProperty;
    fn boolProp(name: string, required: bool): SchemaProperty;
}

export class SchemaValue {
    name: string,
    val_type: SchemaType,
    str_val: string,
    int_val: i64,
    bool_val: bool,
    fn str(name: string, v: string): SchemaValue;
    fn int(name: string, v: i64): SchemaValue;
    fn boolean(name: string, v: bool): SchemaValue;
}

export class ObjectSchema {
    title: string,
    properties: &[]SchemaProperty,
    additional_properties: bool,
}

export class ValidationError {
    field: string,
    code: u32,
    message: string,
    expected: string,
    actual: string,
}

export class ValidationReport {
    valid: bool,
    error_count: usize,
    errors: [16]ValidationError,
}

export fn schuStrEq(a: string, b: string): bool;
export fn schuWriteStr(out: []u8, pos: usize, s: string): usize;
export fn schuOk(): ValidationResult;
export fn schuFail(code: u32, msg: string): ValidationResult;
export fn validateStringField(value: string, present: bool, c: FieldConstraint): ValidationResult;
export fn validateIntField(value: i64, present: bool, c: FieldConstraint): ValidationResult;
export fn validateEnumField(value: string, allowed: &[]string): ValidationResult;
export fn schuMatchClass(pat: string, start: usize, end: usize, c: u8): bool;
export fn schuAtomLen(pat: string, pi: usize): usize;
export fn schuMatchAtom(pat: string, pi: usize, atom_len: usize, c: u8): bool;
export fn schuMatchHere(pat: string, pi: usize, text: string, ti: usize): bool;
export fn matchPattern(pattern: string, text: string): bool;
export fn validatePatternField(value: string, pattern: string): ValidationResult;
export fn schuTypeMatches(expected: SchemaType, actual: SchemaType): bool;
export fn schuPushError(report: &mut ValidationReport, err: ValidationError);
export fn validate(schema: ObjectSchema, values: &[]SchemaValue): ValidationReport;
export fn formatValidationError(schema_name: string, err: ValidationError, out: []u8): usize;
export fn formatValidationReport(schema_name: string, report: ValidationReport, out: []u8): usize;
