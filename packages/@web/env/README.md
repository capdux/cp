[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/env

Official startup environment schema validation & App state injection plugin for cap:web (inspired by @fastify/env)

| Metadata | Value |
| --- | --- |
| Package | `@web/env` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/env
```

```cp
import {
    EnvKind,
    EnvVarSpec,
    EnvKV,
    EnvValidationResult,
    envStrEql,
    envParseI64,
    envIsValidBool,
    envInEnum,
} from "@web/env";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `EnvKind` | `Str, Int, Bool, Enum` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EnvVarSpec` | `class` | Fields: `name: string, kind: EnvKind, required: bool, default_val: string, min_val: i64, max_val: i64, +2 more` |
| `EnvKV` | `class` | Fields: `key: string, value: string` |
| `EnvValidationResult` | `class` | Fields: `ok: bool, missing_count: usize, invalid_count: usize, loaded_count: usize, first_error_key: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `envStrEql` | `(a: string, b: string): bool` | Invokes `envStrEql` and returns `bool`. |
| `envParseI64` | `(s: string): i64?` | Invokes `envParseI64` and returns `i64?`. |
| `envIsValidBool` | `(s: string): bool` | Invokes `envIsValidBool` and returns `bool`. |
| `envInEnum` | `(val: string, list: string): bool` | Invokes `envInEnum` and returns `bool`. |
| `envLookupRaw` | `(entries: &[]EnvKV, key: string): string?` | Invokes `envLookupRaw` and returns `string?`. |
| `registerEnv` | `(app: &mut App, specs: &[]EnvVarSpec, entries: &[]EnvKV): EnvValidationResult` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { envStrEql } from "@web/env";

pub fn main() {
    let result = envStrEql("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
