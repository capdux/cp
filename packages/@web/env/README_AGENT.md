# @web/env — AI Agent Quick Ref

Official startup environment schema validation & App state injection plugin for cap:web (inspired by @fastify/env) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
enum EnvKind { Str, Int, Bool, Enum }
class EnvVarSpec { name: string, kind: EnvKind, required: bool, default_val: string, /* +4 fields */ }
class EnvKV { key: string, value: string }
class EnvValidationResult { ok: bool, missing_count: usize, invalid_count: usize, loaded_count: usize, /* +1 fields */ }
```

## API Signatures

```cp
fn envStrEql(a: string, b: string): bool;
fn envParseI64(s: string): i64?;
fn envIsValidBool(s: string): bool;
fn envInEnum(val: string, list: string): bool;
fn envLookupRaw(entries: &[]EnvKV, key: string): string?;
fn registerEnv(app: &mut App, specs: &[]EnvVarSpec, entries: &[]EnvKV): EnvValidationResult;
```

## Minimal Usage

```cp
import { envStrEql } from "@web/env";

pub fn main() {
    let result = envStrEql("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/env"`; install via `cap add @web/env`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
