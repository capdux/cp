# schema-utils — AI Agent Quick Ref

- **Import**: `import { ObjectSchema, SchemaProperty, SchemaValue, validate, formatValidationReport, ... } from "schema-utils";`
- **Origin**: npm `schema-utils` v4.3.0 (MIT, zero-heap, max 16 errors/report)
- **Full Types**: `src/index.d.cp`

## Core Types
```cp
enum SchemaType { Any, String, Integer, Number, Boolean, Array, Object, Null }
class SchemaProperty {
    name: string, expected_type: SchemaType, required: bool,
    min_length: usize, max_length: usize,
    has_minimum: bool, minimum: i64, has_maximum: bool, maximum: i64,
    pattern: string, multiple_of: i64,
    fn stringProp(name: string, required: bool, min_length: usize, max_length: usize, pattern: string): SchemaProperty;
    fn intProp(name: string, required: bool, has_minimum: bool, minimum: i64, has_maximum: bool, maximum: i64): SchemaProperty;
    fn boolProp(name: string, required: bool): SchemaProperty;
}
class SchemaValue {
    name: string, val_type: SchemaType, str_val: string, int_val: i64, bool_val: bool,
    fn str(name: string, v: string): SchemaValue;
    fn int(name: string, v: i64): SchemaValue;
    fn boolean(name: string, v: bool): SchemaValue;
}
class ObjectSchema { title: string, properties: &[]SchemaProperty, additional_properties: bool }
class ValidationError { field: string, code: u32, message: string, expected: string, actual: string }
class ValidationReport { valid: bool, error_count: usize, errors: [16]ValidationError }
class FieldConstraint { name: string, required: bool, min_len: usize, max_len: usize, has_int_range: bool, min_int: i64, max_int: i64 }
class ValidationResult { valid: bool, error_code: u32, message: string }
```

## Functions
- `validate(schema: ObjectSchema, values: &[]SchemaValue): ValidationReport`
- `formatValidationError(schema_name: string, err: ValidationError, out: []u8): usize`
- `formatValidationReport(schema_name: string, report: ValidationReport, out: []u8): usize`
- `validateStringField(value: string, present: bool, c: FieldConstraint): ValidationResult`
- `validateIntField(value: i64, present: bool, c: FieldConstraint): ValidationResult`
- `validateEnumField(value: string, allowed: &[]string): ValidationResult`
- `validatePatternField(value: string, pattern: string): ValidationResult`
- `matchPattern(pattern: string, text: string): bool`
- `schemaTypeName(t: SchemaType): string`

## Gotchas
1. `ValidationReport.errors` holds at most **16** errors; iterate `0..report.error_count` (never full array).
2. Pass `pattern: ""` in `SchemaProperty.stringProp` and `max_length: 0` when no upper bound or pattern is needed.
3. `formatValidationReport` returns the byte length written into `out`; slice `out[0..written]` for the formatted string.
