# schema-utils — AI Agent Quick Ref

Zero-heap configuration option schema validator with multi-error `ValidationReport` (max 16 errors) and lightweight pattern matching.

## Import
```typescript
import {
    SchemaType, SchemaProperty, SchemaValue, ObjectSchema,
    FieldConstraint, ValidationResult, ValidationError, ValidationReport,
    validate, validateStringField, validateIntField, validateEnumField,
    validatePatternField, matchPattern, schemaTypeName,
    formatValidationError, formatValidationReport,
    SCHU_OK, SCHU_ERR_REQUIRED, SCHU_MAX_ERRORS
} from "schema-utils";
```

## API Signatures
```typescript
export const SCHU_OK: u32 = 0; // Errors 1..10: REQUIRED, MIN_LEN, MAX_LEN, INT_MIN, INT_MAX, ENUM, TYPE, PATTERN, ADDITIONAL_PROP, MULTIPLE_OF
export const SCHU_MAX_ERRORS: usize = 16;
export enum SchemaType { Any, String, Integer, Number, Boolean, Array, Object, Null }
export class SchemaProperty {
    name: string, expected_type: SchemaType, required: bool, min_length: usize, max_length: usize,
    has_minimum: bool, minimum: i64, has_maximum: bool, maximum: i64, pattern: string, multiple_of: i64,
    fn stringProp(name: string, required: bool, min_length: usize, max_length: usize, pattern: string): SchemaProperty;
    fn intProp(name: string, required: bool, has_minimum: bool, minimum: i64, has_maximum: bool, maximum: i64): SchemaProperty;
    fn boolProp(name: string, required: bool): SchemaProperty;
}
export class SchemaValue {
    name: string, val_type: SchemaType, str_val: string, int_val: i64, bool_val: bool,
    fn str(name: string, v: string): SchemaValue;
    fn int(name: string, v: i64): SchemaValue;
    fn boolean(name: string, v: bool): SchemaValue;
}
export class ObjectSchema { title: string, properties: &[]SchemaProperty, additional_properties: bool }
export class FieldConstraint { name: string, required: bool, min_len: usize, max_len: usize, has_int_range: bool, min_int: i64, max_int: i64 }
export class ValidationResult { valid: bool, error_code: u32, message: string }
export class ValidationError { field: string, code: u32, message: string, expected: string, actual: string }
export class ValidationReport { valid: bool, error_count: usize, errors: [16]ValidationError }

export fn schemaTypeName(t: SchemaType): string;
export fn validateStringField(value: string, present: bool, c: FieldConstraint): ValidationResult;
export fn validateIntField(value: i64, present: bool, c: FieldConstraint): ValidationResult;
export fn validateEnumField(value: string, allowed: &[]string): ValidationResult;
export fn matchPattern(pattern: string, text: string): bool;
export fn validatePatternField(value: string, pattern: string): ValidationResult;
export fn validate(schema: ObjectSchema, values: &[]SchemaValue): ValidationReport;
export fn formatValidationError(schema_name: string, err: ValidationError, out: []u8): usize;
export fn formatValidationReport(schema_name: string, report: ValidationReport, out: []u8): usize;
```

## Minimal Example
```typescript
import { SchemaProperty, SchemaValue, ObjectSchema, validate } from "schema-utils";

fn check(): bool {
    let props = [SchemaProperty.intProp("port", true, true, 1, true, 65535)];
    let schema = ObjectSchema { title: "Server", properties: &props, additional_properties: false };
    let vals = [SchemaValue.int("port", 3000)];
    return validate(schema, &vals).valid;
}
```

## Pitfalls & Rules
- `ObjectSchema.properties` and `validate(..., values)` take read-only slices `&[]SchemaProperty` / `&[]SchemaValue`.
- `ValidationReport.errors` holds at most `SCHU_MAX_ERRORS` (16) entries; iterate `0..report.error_count`.
- `max_length == 0` / `pattern == ""` / `multiple_of == 0` disable those respective checks on `SchemaProperty`.
- `formatValidationError` and `formatValidationReport` write into caller-owned `out: []u8` and return bytes written.
