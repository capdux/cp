[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ident_case

Zero-heap identifier case conversion engine supporting `lowercase`, `UPPERCASE`, `camelCase`, `PascalCase`, `snake_case`, `SCREAMING_SNAKE_CASE`, `kebab-case`, and `SCREAMING-KEBAB-CASE` with smart acronym splitting.

Aligned with Rust crates.io [`ident_case`](https://github.com/TedDriggs/ident_case) (MIT).

## Installation & Import

```bash
cap add ident_case
```

```cp
import {
    RenameRule,
    toLowerCase,
    toUpperCase,
    toCamelCase,
    toPascalCase,
    toSnakeCase,
    toKebabCase,
    toScreamingSnakeCase,
    toScreamingKebabCase,
    renameIdent,
    parseRenameRule,
} from "ident_case";
```

## Core API

### Enum

| Symbol | Variants | Description |
| --- | --- | --- |
| `RenameRule` | `LowerCase`, `UpperCase`, `CamelCase`, `PascalCase`, `SnakeCase`, `ScreamingSnakeCase`, `KebabCase`, `ScreamingKebabCase` | Serde-compatible identifier rename rule enum. |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `toLowerCase` | `(input: string, out: []u8): usize` | Converts `input` to lowercase into `out`; returns bytes written. |
| `toUpperCase` | `(input: string, out: []u8): usize` | Converts `input` to uppercase into `out`; returns bytes written. |
| `toCamelCase` | `(input: string, out: []u8): usize` | Converts `input` to `camelCase` with acronym awareness (e.g. `HTTPResponse` -> `httpResponse`). |
| `toPascalCase` | `(input: string, out: []u8): usize` | Converts `input` to `PascalCase`. |
| `toSnakeCase` | `(input: string, out: []u8): usize` | Converts `input` to `snake_case`. |
| `toKebabCase` | `(input: string, out: []u8): usize` | Converts `input` to `kebab-case`. |
| `toScreamingSnakeCase` | `(input: string, out: []u8): usize` | Converts `input` to `SCREAMING_SNAKE_CASE`. |
| `toScreamingKebabCase` | `(input: string, out: []u8): usize` | Converts `input` to `SCREAMING-KEBAB-CASE`. |
| `renameIdent` | `(rule: RenameRule, input: string, out: []u8): usize` | Applies a `RenameRule` dynamically to `input`, writing into `out`. |
| `parseRenameRule` | `(name: string): RenameRule?` | Parses Serde-style rule name (`"camelCase"`, `"snake_case"`, `"SCREAMING_SNAKE_CASE"`, etc.) or returns `null`. |

## Examples

### 1. Converting Identifiers with Acronym Splitting

```cp
import { print } from "cap:io";
import { toSnakeCase, toCamelCase, toKebabCase } from "ident_case";

pub fn main() {
    let out: [64]u8 = undefined;
    let n1 = toSnakeCase("HTTPResponseCode", out[0..64]);
    print("snake: {}\n", out[0..n1]); // "http_response_code"

    let n2 = toCamelCase("user_profile_id", out[0..64]);
    print("camel: {}\n", out[0..n2]); // "userProfileId"

    let n3 = toKebabCase("getJSONData", out[0..64]);
    print("kebab: {}\n", out[0..n3]); // "get-json-data"
}
```

### 2. Dynamic Rule Dispatch (Serde-Style)

```cp
import { print } from "cap:io";
import { parseRenameRule, renameIdent } from "ident_case";

pub fn main() {
    let rule = parseRenameRule("SCREAMING-KEBAB-CASE");
    if (rule != null) {
        let out: [64]u8 = undefined;
        let n = renameIdent(rule!, "maxRetryCount", out[0..64]);
        print("renamed: {}\n", out[0..n]); // "MAX-RETRY-COUNT"
    }
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Zero heap allocation**: Writes directly into a caller-provided `out: []u8` buffer and returns the exact written length `usize` instead of allocating a `String`.
- **Unified `RenameRule` dispatcher**: Adds `RenameRule`, `parseRenameRule`, and `renameIdent` for macro/serialization field renaming workflows.
- **Smart acronym boundaries**: Correctly splits consecutive uppercase runs such as `HTTPServer` -> `http_server` and `XMLHttpRequest` -> `xml_http_request`.
