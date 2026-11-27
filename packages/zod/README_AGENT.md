# `zod` — AI Agent Quick Ref

Zero-heap schema & JSON object validation framework for CP (npm `zod` v3 parity, backed by `kind-of` and `url`).

## Import

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

## Core API Signatures

```cp
// Constants
// Kinds (u8): Z_STRING=1, Z_NUMBER=2, Z_INT=3, Z_BOOL=4, Z_EMAIL=5, Z_URL=6, Z_UUID=7, Z_ENUM=8, Z_ARRAY=9, Z_OBJECT=10
// Errors (i32): Z_ERR_INVALID_JSON=1 .. Z_ERR_INVALID_ENUM=11
// Limits (usize): Z_MAX_RULES=24, Z_MAX_ISSUES=16

// Standalone validators
fn zString(raw: string, min_len: usize, max_len: usize): bool;
fn zNumber(raw: string, min_v: f64, max_v: f64): bool;
fn zEmail(s: string): bool;
fn zUrl(s: string): bool;
fn zUuid(s: string): bool;
fn zEnum(s: string, csv: string): bool;

// Field rule builders
ZFieldRule.empty(): ZFieldRule;
ZFieldRule.basic(name: string, kind: u8, required: bool): ZFieldRule;
ZFieldRule.strRange(name: string, required: bool, min_len: usize, max_len: usize): ZFieldRule;
ZFieldRule.numRange(name: string, kind: u8, required: bool, min_val: f64, max_val: f64): ZFieldRule;
ZFieldRule.enumRule(name: string, required: bool, enum_csv: string): ZFieldRule;

// Result & schema
ZIssue.fieldName(): []const u8;
ZParseResult.addIssue(*_, field_name: string, code: i32);
ZParseResult.hasIssue(field_name: string, code: i32): bool;
ZObjectSchema.new(): ZObjectSchema;
schema.field(*_, rule: ZFieldRule): bool;         // returns false if > 24 rules
schema.safeParseJson(json: string): ZParseResult; // read-only receiver
```

## Minimal Example

```cp
import { ZObjectSchema, ZFieldRule, Z_EMAIL, Z_INT } from "zod";

let schema = ZObjectSchema.new();
schema.field(ZFieldRule.basic("email", Z_EMAIL, true));
schema.field(ZFieldRule.numRange("age", Z_INT, true, 0.0, 150.0));
const res = schema.safeParseJson("{\"email\":\"a@b.co\",\"age\":25}");
// res.ok == true
```

## Gotchas & Borrow Rules

- `schema.field(*_, ...)` mutates `ZObjectSchema`; declare `let schema = ZObjectSchema.new()`.
- `ZObjectSchema` holds at most `24` rules (`Z_MAX_RULES`); `field()` returns `false` when full.
- `ZParseResult` holds at most `16` issues (`Z_MAX_ISSUES`) and truncates `field` names to 48 bytes.
- `numRange` requires passing `Z_NUMBER` or `Z_INT` explicitly as `kind`.
