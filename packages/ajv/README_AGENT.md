# ajv@1.0.0 — AI Agent Quick Ref

- **Source**: `npm:ajv` (`ajv-validator/ajv`, MIT, v8 parity) | **Deps**: `zod`, `ipaddr.js`
- **Import**: `import { AjvSchema, AjvResult, AjvError, validateWithSchema, validateFormat, AJV_ERR_TYPE, AJV_ERR_REQUIRED, AJV_ERR_FORMAT, AJV_MAX_ERRORS } from "ajv";`
- **Full Types**: `packages/ajv/src/index.d.cp`

## Error Codes (`i32`)
```cp
AJV_ERR_INVALID_SCHEMA = 1   AJV_ERR_INVALID_DATA = 2       AJV_ERR_TYPE = 3
AJV_ERR_REQUIRED = 4         AJV_ERR_ADDITIONAL_PROPERTIES = 5
AJV_ERR_MINIMUM = 6          AJV_ERR_MAXIMUM = 7
AJV_ERR_EXCLUSIVE_MINIMUM = 8  AJV_ERR_EXCLUSIVE_MAXIMUM = 9
AJV_ERR_MIN_LENGTH = 10      AJV_ERR_MAX_LENGTH = 11
AJV_ERR_MIN_ITEMS = 12       AJV_ERR_MAX_ITEMS = 13
AJV_ERR_ENUM = 14            AJV_ERR_CONST = 15             AJV_ERR_FORMAT = 16
AJV_MAX_ERRORS: usize = 16
```

## Core Types & Functions
```cp
class AjvError  { instance_path: [64]u8, instance_path_len: usize, keyword: [32]u8, keyword_len: usize, code: i32; fn pathStr(): &[]u8; fn keywordStr(): &[]u8 }
class AjvResult { valid: bool, error_count: usize, errors: [16]AjvError; fn ok(): AjvResult; fn hasError(path: string, keyword: string, code: i32): bool; fn hasCode(code: i32): bool }
class AjvSchema { source: string, ok: bool; fn compile(schema_json: string): AjvSchema; fn validate(data_json: string): AjvResult }

fn validateWithSchema(schema_json: string, data_json: string): AjvResult
fn validateFormat(fmt: string, value: string): bool // email, uri, url, uuid, ipv4, ipv6, date
```

## Minimal Pattern
```cp
import { AjvSchema, AJV_ERR_MINIMUM } from "ajv";

const s = AjvSchema.compile("{\"type\":\"object\",\"required\":[\"age\"],\"properties\":{\"age\":{\"type\":\"integer\",\"minimum\":18}}}");
const r = s.validate("{\"age\":15}");
// r.valid == false, r.hasCode(AJV_ERR_MINIMUM) == true
```

## Pitfalls & Notes
1. **Schema Lifetime**: `AjvSchema.source` borrows `schema_json`; keep `schema_json` alive while calling `schema.validate(...)`.
2. **Error Cap**: At most `AJV_MAX_ERRORS` (16) errors are recorded in `res.errors`; `res.valid` is always accurately `false` even if more than 16 violations occur.
3. **Supported Formats**: `email`, `uri`, `url`, `uuid`, `ipv4`, `ipv6`, `date` (ISO `YYYY-MM-DD`). Unknown format names return `true` (per JSON Schema annotation default).
4. **Strict JSON Inputs**: Both `schema_json` and `data_json` must be valid JSON strings; malformed JSON yields `AJV_ERR_INVALID_SCHEMA` or `AJV_ERR_INVALID_DATA`.
5. **Internal Helpers**: `ajv*` functions are internal — use `AjvSchema`, `validateWithSchema`, and `validateFormat`.
