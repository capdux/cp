[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# `zod`

Zero-heap type-safe schema validation framework for CP — a clean-room MIT counterpart to npm [`zod`](https://github.com/colinhacks/zod) (v3 parity), providing standalone primitive validators and a stack-allocated `ZObjectSchema` JSON object validator with structured `ZIssue` reporting.

## Installation & Import

```bash
cap add zod
```

```cp
import {
    Z_STRING, Z_NUMBER, Z_INT, Z_BOOL, Z_EMAIL, Z_URL, Z_UUID, Z_ENUM, Z_ARRAY, Z_OBJECT,
    Z_ERR_INVALID_JSON, Z_ERR_NOT_OBJECT, Z_ERR_REQUIRED, Z_ERR_INVALID_TYPE,
    Z_ERR_NOT_INT, Z_ERR_TOO_SMALL, Z_ERR_TOO_BIG,
    Z_ERR_INVALID_EMAIL, Z_ERR_INVALID_URL, Z_ERR_INVALID_UUID, Z_ERR_INVALID_ENUM,
    Z_MAX_RULES, Z_MAX_ISSUES,
    ZFieldRule, ZIssue, ZParseResult, ZObjectSchema,
    zString, zNumber, zEmail, zUrl, zUuid, zEnum,
} from "zod";
```

## Core API

### Rule Kind & Error Code Constants

| Group | Constants | Values |
| :--- | :--- | :--- |
| Rule Kinds (`u8`) | `Z_STRING`, `Z_NUMBER`, `Z_INT`, `Z_BOOL`, `Z_EMAIL`, `Z_URL`, `Z_UUID`, `Z_ENUM`, `Z_ARRAY`, `Z_OBJECT` | `1..10` |
| Error Codes (`i32`) | `Z_ERR_INVALID_JSON`, `Z_ERR_NOT_OBJECT`, `Z_ERR_REQUIRED`, `Z_ERR_INVALID_TYPE`, `Z_ERR_NOT_INT`, `Z_ERR_TOO_SMALL`, `Z_ERR_TOO_BIG`, `Z_ERR_INVALID_EMAIL`, `Z_ERR_INVALID_URL`, `Z_ERR_INVALID_UUID`, `Z_ERR_INVALID_ENUM` | `1..11` |
| Limits (`usize`) | `Z_MAX_RULES`, `Z_MAX_ISSUES` | `24`, `16` |

### Standalone Primitive Validators

| Function | Signature | Description |
| :--- | :--- | :--- |
| `zString` | `(raw: string, min_len: usize, max_len: usize): bool` | Validate string byte length within `[min_len, max_len]` |
| `zNumber` | `(raw: string, min_v: f64, max_v: f64): bool` | Parse numeric string and check `[min_v, max_v]` |
| `zEmail` | `(s: string): bool` | Validate email syntax |
| `zUrl` | `(s: string): bool` | Validate WHATWG URL (backed by `url`) |
| `zUuid` | `(s: string): bool` | Validate canonical UUID (`8-4-4-4-12` hex) |
| `zEnum` | `(s: string, csv: string): bool` | Check membership in comma-separated enum values (e.g., `"admin,user,guest"`) |

### Schema & Result Types

| Type | Key Methods / Constructors | Description |
| :--- | :--- | :--- |
| `ZFieldRule` | `empty()`, `basic(name, kind, required)`, `strRange(name, required, min_len, max_len)`, `numRange(name, kind, required, min_val, max_val)`, `enumRule(name, required, enum_csv)` | Declarative field constraint descriptor |
| `ZIssue` | `empty()`, `fieldName(): []const u8` | Single validation error (`field: [48]u8`, `field_len`, `code: i32`) |
| `ZParseResult` | `empty()`, `addIssue(*_, field_name, code)`, `hasIssue(field_name, code): bool` | Fixed-capacity result (`ok: bool`, `issue_count`, `issues: [16]ZIssue`) |
| `ZObjectSchema` | `new()`, `field(*_, rule: ZFieldRule): bool`, `safeParseJson(json: string): ZParseResult` | Zero-heap JSON object validator (up to 24 field rules) |

## Examples

### 1. Validating a JSON Payload with `ZObjectSchema`

```cp
import { print } from "cap:io";
import {
    ZObjectSchema, ZFieldRule,
    Z_EMAIL, Z_INT, Z_BOOL, Z_ERR_TOO_SMALL,
} from "zod";

fn main() {
    let schema = ZObjectSchema.new();
    schema.field(ZFieldRule.strRange("username", true, 3, 20));
    schema.field(ZFieldRule.basic("email", Z_EMAIL, true));
    schema.field(ZFieldRule.numRange("age", Z_INT, true, 18.0, 120.0));
    schema.field(ZFieldRule.enumRule("role", true, "admin,member,guest"));
    schema.field(ZFieldRule.basic("active", Z_BOOL, false));

    const payload = "{\"username\":\"alice\",\"email\":\"alice@cp.dev\",\"age\":16,\"role\":\"admin\"}";
    const res = schema.safeParseJson(payload);
    if (!res.ok) {
        print("issues:", res.issue_count);
        print("age too small:", res.hasIssue("age", Z_ERR_TOO_SMALL)); // true
    }
}
```

### 2. Direct Primitive Format Checks

```cp
import { print } from "cap:io";
import { zEmail, zUrl, zUuid, zEnum, zNumber } from "zod";

fn main() {
    print(zEmail("user@example.com"));                       // true
    print(zUrl("https://cp-lang.org/packages/zod"));         // true
    print(zUuid("550e8400-e29b-41d4-a716-446655440000"));    // true
    print(zEnum("staging", "dev,staging,prod"));             // true
    print(zNumber("42.5", 0.0, 100.0));                      // true
}
```

## Differences from Upstream & Performance Highlights

- **Zero-Heap JSON Scanning**: `safeParseJson` validates raw JSON strings directly on the stack without materializing a heap AST or `HashMap`, reusing `kind-of` and `url`.
- **Fixed Inline Buffers**: Rules (`Z_MAX_RULES = 24`) and issues (`Z_MAX_ISSUES = 16`) live in fixed arrays inside value structs — no allocator required and deterministic `O(n)` latency.
- **No Exceptions**: Mirrors Zod's `.safeParse()` pattern by always returning `ZParseResult` with `ok: bool` and typed `Z_ERR_*` codes.
