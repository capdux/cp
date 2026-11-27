[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# ajv

> **Source Attribution**: Clean-room MIT CP implementation of [`npm:ajv`](https://github.com/ajv-validator/ajv) (MIT, v8 parity) | CP Package Version: `1.0.0` | Dependencies: `zod`, `ipaddr.js`

Zero-heap JSON Schema Draft-07 / 2020-12 validation engine for CP. Validates types, `required`, `properties`, `additionalProperties`, numeric bounds (`minimum`/`maximum`/`exclusiveMinimum`/`exclusiveMaximum`), string length (`minLength`/`maxLength`), array bounds (`minItems`/`maxItems`/`items`), `enum`, `const`, and `format` (`email`, `uri`/`url`, `uuid`, `ipv4`, `ipv6`, `date`) with structured JSON Pointer error reporting.

## Zero-Cost Migration (TypeScript / JS → CP)

Keep the exact same package name (`"ajv"`) — no renaming required.

| TypeScript / Node.js (`npm:ajv`) | CP (`ajv`) |
| :--- | :--- |
| `import Ajv from "ajv";` | `import { AjvSchema, validateWithSchema } from "ajv";` |
| `const validate = ajv.compile(schema);` | `const schema = AjvSchema.compile(schema_json);` |
| `const ok = validate(data);` | `const res = schema.validate(data_json); // res.valid` |
| `validate.errors` | `res.errors[0..res.error_count]` (`pathStr()`, `keywordStr()`, `code`) |

## API Reference

### Error Codes & Limit

- `AJV_ERR_INVALID_SCHEMA = 1`, `AJV_ERR_INVALID_DATA = 2`, `AJV_ERR_TYPE = 3`, `AJV_ERR_REQUIRED = 4`, `AJV_ERR_ADDITIONAL_PROPERTIES = 5`
- `AJV_ERR_MINIMUM = 6`, `AJV_ERR_MAXIMUM = 7`, `AJV_ERR_EXCLUSIVE_MINIMUM = 8`, `AJV_ERR_EXCLUSIVE_MAXIMUM = 9`
- `AJV_ERR_MIN_LENGTH = 10`, `AJV_ERR_MAX_LENGTH = 11`, `AJV_ERR_MIN_ITEMS = 12`, `AJV_ERR_MAX_ITEMS = 13`
- `AJV_ERR_ENUM = 14`, `AJV_ERR_CONST = 15`, `AJV_ERR_FORMAT = 16`
- `AJV_MAX_ERRORS: usize = 16`

### Core Types & Functions

| Symbol / Signature | Description |
| :--- | :--- |
| `AjvError { instance_path: [64]u8, keyword: [32]u8, code: i32 }` | Validation error with `pathStr(): &[]u8` and `keywordStr(): &[]u8`. |
| `AjvResult { valid: bool, error_count: usize, errors: [16]AjvError }` | Validation outcome with `hasError(path, keyword, code)` and `hasCode(code)`. |
| `AjvSchema.compile(schema_json: string): AjvSchema` | Validate and wrap a JSON Schema string (`schema.ok == true` if valid JSON object/bool). |
| `schema.validate(data_json: string): AjvResult` | Validate a JSON payload against the compiled schema. |
| `validateWithSchema(schema_json: string, data_json: string): AjvResult` | One-shot compile + validate convenience function. |
| `validateFormat(fmt: string, value: string): bool` | Standalone format check (`"email"`, `"uri"`, `"url"`, `"uuid"`, `"ipv4"`, `"ipv6"`, `"date"`). |

## Usage Examples

### Compiling & Validating a JSON Schema

```cp
import { AjvSchema, AJV_ERR_REQUIRED, AJV_ERR_FORMAT } from "ajv";

fn demoValidate() {
    const schema_json = "{\"type\":\"object\",\"required\":[\"email\",\"age\"],\"properties\":{\"email\":{\"type\":\"string\",\"format\":\"email\"},\"age\":{\"type\":\"integer\",\"minimum\":18}},\"additionalProperties\":false}";
    const schema = AjvSchema.compile(schema_json);

    const ok_res = schema.validate("{\"email\":\"dev@cp.lang\",\"age\":25}");
    // ok_res.valid == true, ok_res.error_count == 0

    const bad_res = schema.validate("{\"email\":\"not-an-email\"}");
    // bad_res.valid == false
    // bad_res.hasCode(AJV_ERR_REQUIRED) == true
    // bad_res.hasCode(AJV_ERR_FORMAT) == true
    * = ok_res; * = bad_res;
}
```

### One-Shot Validation & Standalone Format Check

```cp
import { validateWithSchema, validateFormat } from "ajv";

fn demoFormatAndOneShot() {
    const res = validateWithSchema("{\"type\":\"array\",\"minItems\":1,\"items\":{\"type\":\"string\",\"enum\":[\"a\",\"b\"]}}", "[\"a\",\"b\"]");
    // res.valid == true

    const is_ip = validateFormat("ipv6", "2001:db8::1");   // true
    const is_dt = validateFormat("date", "2026-11-27");    // true
    * = res; * = is_ip; * = is_dt;
}
```

## Performance & Safety Highlights

- **Zero Heap Allocation**: Operates directly on raw JSON slices without building a heap DOM; `AjvResult` stores up to 16 `AjvError` records inline (`undefined`-initialized stack buffers).
- **Ecosystem Reuse**: Delegates `format` validation directly to `zod` (`zEmail`, `zUrl`, `zUuid`, `zEnum`) and `ipaddr.js` (`isValidIpv4`, `isValidIpv6`) — zero duplicated validators.
- **Depth & Malformed Input Guards**: Strict JSON scanner and bounded recursion depth prevent stack exhaustion on deeply nested or malformed payloads.

## License

MIT
