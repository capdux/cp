[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# schema-utils

Zero-heap configuration option schema validator in pure CP with structured multi-error reports, regex-like pattern matching, field validators, and webpack-style error formatting.

- **Upstream Alignment**: Modeled after npm [`schema-utils`](https://github.com/webpack/schema-utils) `4.3.0` (`MIT`).
- **Package Version**: `4.3.0`

## Installation & Import

```bash
cap add schema-utils
```

In `cap.json`:

```json
{
  "dependencies": {
    "schema-utils": "^4.3.0"
  }
}
```

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

## Core API Reference

### Constants & Enum

| Symbol | Values | Description |
| :--- | :--- | :--- |
| `SchemaType` | `Any`, `String`, `Integer`, `Number`, `Boolean`, `Array`, `Object`, `Null` | JSON Schema value types |
| Error Codes | `SCHU_OK` (0), `SCHU_ERR_REQUIRED` (1), `SCHU_ERR_MIN_LEN` (2), `SCHU_ERR_MAX_LEN` (3), `SCHU_ERR_INT_MIN` (4), `SCHU_ERR_INT_MAX` (5), `SCHU_ERR_ENUM` (6), `SCHU_ERR_TYPE` (7), `SCHU_ERR_PATTERN` (8), `SCHU_ERR_ADDITIONAL_PROP` (9), `SCHU_ERR_MULTIPLE_OF` (10) | Machine-readable validation error codes |
| `SCHU_MAX_ERRORS` | `16` | Fixed capacity of `ValidationReport.errors` |

### Data Structures & Builders

| Type | Key Fields / Constructors | Description |
| :--- | :--- | :--- |
| `SchemaProperty` | `stringProp(name, required, min_length, max_length, pattern)`, `intProp(name, required, has_minimum, minimum, has_maximum, maximum)`, `boolProp(name, required)` | Property rule (also supports `multiple_of: i64`) |
| `SchemaValue` | `str(name, v)`, `int(name, v)`, `boolean(name, v)` | Typed key-value option entry to validate |
| `ObjectSchema` | `title: string`, `properties: &[]SchemaProperty`, `additional_properties: bool` | Schema definition for an options object |
| `FieldConstraint` | `name`, `required`, `min_len`, `max_len`, `has_int_range`, `min_int`, `max_int` | Single-field validation constraint |
| `ValidationResult` | `valid: bool`, `error_code: u32`, `message: string` | Result for single-field validators |
| `ValidationError` | `field`, `code`, `message`, `expected`, `actual` | Structured error item inside `ValidationReport` |
| `ValidationReport` | `valid: bool`, `error_count: usize`, `errors: [16]ValidationError` | Aggregated validation report returned by `validate` |

### Core Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `validate` | `validate(schema: ObjectSchema, values: &[]SchemaValue): ValidationReport` | Validate an option slice against an `ObjectSchema` |
| `validateStringField` | `validateStringField(value: string, present: bool, c: FieldConstraint): ValidationResult` | Validate string presence and length bounds |
| `validateIntField` | `validateIntField(value: i64, present: bool, c: FieldConstraint): ValidationResult` | Validate integer presence and `[min_int, max_int]` range |
| `validateEnumField` | `validateEnumField(value: string, allowed: &[]string): ValidationResult` | Check if `value` is in `allowed` |
| `matchPattern` | `matchPattern(pattern: string, text: string): bool` | Lightweight pattern match (`^`, `$`, `.`, `*`, `+`, `?`, `[a-z]`, escapes) |
| `validatePatternField` | `validatePatternField(value: string, pattern: string): ValidationResult` | Validate `value` against `pattern` |
| `schemaTypeName` | `schemaTypeName(t: SchemaType): string` | Return lowercase JSON Schema type name |
| `formatValidationError` | `formatValidationError(schema_name: string, err: ValidationError, out: []u8): usize` | Format a single error into caller buffer `out` |
| `formatValidationReport` | `formatValidationReport(schema_name: string, report: ValidationReport, out: []u8): usize` | Format full report into caller buffer `out` |

## Examples

### 1. Validating Plugin Options Against `ObjectSchema`

```typescript
import {
    SchemaProperty, SchemaValue, ObjectSchema,
    validate, formatValidationReport
} from "schema-utils";

fn validateConfig(): bool {
    let props = [
        SchemaProperty.stringProp("mode", true, 1, 16, "^(dev|prod)$"),
        SchemaProperty.intProp("port", true, true, 1, true, 65535),
        SchemaProperty.boolProp("minify", false)
    ];
    let schema = ObjectSchema {
        title: "BuildOptions",
        properties: &props,
        additional_properties: false
    };

    let values = [
        SchemaValue.str("mode", "prod"),
        SchemaValue.int("port", 8080),
        SchemaValue.boolean("minify", true)
    ];

    let report = validate(schema, &values);
    if (!report.valid) {
        let mut buf: [512]u8 = undefined;
        let _ = formatValidationReport(schema.title, report, &mut buf);
    }
    return report.valid;
}
```

### 2. Single-Field Enum & Pattern Checks

```typescript
import { validateEnumField, validatePatternField } from "schema-utils";

fn checkTarget(target: string, version: string): bool {
    let allowed = ["node", "web", "bun"];
    let r1 = validateEnumField(target, &allowed);
    let r2 = validatePatternField(version, "^[0-9]+\\.[0-9]+\\.[0-9]+$");
    return r1.valid && r2.valid;
}
```

## Differences from Upstream & CP Highlights

- **Zero-Heap Fixed Report**: Collects up to `SCHU_MAX_ERRORS` (16) structured errors inline on the stack without throwing exceptions or allocating error strings.
- **Built-in Lightweight Pattern Engine**: `matchPattern` runs without external regex dependencies or heap allocations.
- **Caller-Owned Formatting Buffers**: `formatValidationError` and `formatValidationReport` write formatted diagnostics into `out: []u8` and return the byte count.
