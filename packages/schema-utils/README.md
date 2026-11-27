[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# schema-utils

Zero-heap configuration option schema validator (`validate` / `ObjectSchema` / `SchemaProperty` / `SchemaValue`), built-in regex-like pattern matcher (`matchPattern`), field validators, and human-readable validation report formatter in pure CP.

## Source & Zero-Cost Migration

| Field | Value |
| --- | --- |
| **Upstream Registry** | [npmjs.com/package/schema-utils](https://www.npmjs.com/package/schema-utils) |
| **Upstream Package** | `schema-utils` v4.3.0 |
| **Upstream Repository** | https://github.com/webpack/schema-utils |
| **License** | MIT |

Keep the exact package name when migrating from TypeScript/npm to CP:

```ts
// TypeScript (npm)
import { validate } from "schema-utils";
validate(schema, options, { name: "MyPlugin" });
```

```cp
// CP — same package name, zero-heap schema validation & formatted reports
import {
    ObjectSchema, SchemaProperty, SchemaValue,
    validate, formatValidationReport,
} from "schema-utils";
```

## Installation

```bash
cap add schema-utils
```

## Quick Start & Practical Examples

### 1. Structured Object Schema Validation (`validate` & `formatValidationReport`)

```cp
import {
    ObjectSchema, SchemaProperty, SchemaValue,
    validate, formatValidationReport, formatValidationError,
} from "schema-utils";

fn validateConfig() {
    let props: [3]SchemaProperty = [
        SchemaProperty.stringProp("host", true, 1, 64, "^[a-zA-Z0-9.-]+$"),
        SchemaProperty.intProp("port", true, true, 1, true, 65535),
        SchemaProperty.boolProp("tls", false),
    ];

    let schema = ObjectSchema {
        title: "ServerOptions",
        properties: &props,
        additional_properties: false,
    };

    let values: [3]SchemaValue = [
        SchemaValue.str("host", "api.example.com"),
        SchemaValue.int("port", 8080),
        SchemaValue.boolean("tls", true),
    ];

    let report = validate(schema, &values);
    if (!report.valid) {
        mut out: [512]u8 = [0; 512];
        let written = formatValidationReport("ServerOptions", report, &mut out);
        // out[0..written] contains formatted multi-error diagnostics
    }
}
```

### 2. Standalone Field & Pattern Validators

```cp
import {
    FieldConstraint, validateStringField, validateIntField,
    validateEnumField, validatePatternField, matchPattern,
    SchemaType, schemaTypeName,
} from "schema-utils";

fn demoFieldValidators() {
    let c = FieldConstraint {
        name: "username",
        required: true,
        min_len: 3,
        max_len: 20,
        has_int_range: false,
        min_int: 0,
        max_int: 0,
    };
    let r1 = validateStringField("alice", true, c); // r1.valid == true

    let allowed: [3]string = ["dev", "staging", "prod"];
    let r2 = validateEnumField("prod", &allowed);   // r2.valid == true

    let ok = matchPattern("^[a-z]+-[0-9]+$", "worker-42"); // true
    let r3 = validatePatternField("worker-42", "^[a-z]+-[0-9]+$");

    let tname = schemaTypeName(SchemaType.Integer); // "integer"
}
```

## Performance & Safety Highlights

- **Zero Heap Allocations**: `ValidationReport` stores up to 16 structured `ValidationError` entries inline on the stack; formatting writes directly into caller-provided `[]u8` buffers.
- **Built-in Zero-Alloc Pattern Engine**: `matchPattern` supports anchors (`^`, `$`), character classes (`[a-z0-9]`), quantifiers, and wildcards without external regex dependencies.
- **Strict Additional-Property Guards**: Setting `additional_properties: false` catches unknown configuration keys automatically.
- **Multi-Error Collection**: `validate` gathers all schema violations (missing required fields, type mismatches, bounds, pattern failures, `multiple_of`) in a single pass.

## API Reference

See [`src/index.d.cp`](src/index.d.cp) for complete type signatures, or [`README_AGENT.md`](README_AGENT.md) for the compact AI-agent reference.
