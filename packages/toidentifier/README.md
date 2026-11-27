[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# toidentifier

Zero-heap converter from free-form words and HTTP status phrases to PascalCase, camelCase, `CONSTANT_CASE`, `snake_case`, and safe identifiers, plus identifier validators in pure CP.

- **Package**: `toidentifier` (`v1.0.0`)
- **Upstream Alignment**: Clean-room CP port of `npm/toidentifier` (`MIT`, `dougwilson/toidentifier`), extended with checked buffer writes, safe leading-digit prefixing, camel/const/snake variants, and identifier validation.

## Installation & Import

```bash
cap add toidentifier
```

```typescript
import {
    toIdentifierLen, toIdentifier, toIdentifierChecked,
    toSafeIdentifier, toCamelIdentifier, toConstIdentifier, toSnakeIdentifier,
    isValidIdentifier, isValidJsIdentifier,
} from "toidentifier";
```

## Core API

| Function | Signature | Description |
| --- | --- | --- |
| `toIdentifierLen` | `(input: string): usize` | Computes exact output length for `toIdentifier(input, out)`. |
| `toIdentifier` | `(input: string, out: []u8): usize` | Converts words to PascalCase identifier (e.g. `"Bad Request"` -> `"BadRequest"`), stripping non-alphanumeric chars. |
| `toIdentifierChecked` | `(input: string, out: []u8): usize?` | Same as `toIdentifier`, returning `null` if `out.len < toIdentifierLen(input)`. |
| `toSafeIdentifier` | `(input: string, out: []u8): usize` | Produces a valid identifier by prepending `_` when the result would start with a digit (or `"_"` if empty). |
| `toCamelIdentifier` | `(input: string, out: []u8): usize` | Converts words to `camelCase` identifier (e.g. `"Bad Request"` -> `"badRequest"`). |
| `toConstIdentifier` | `(input: string, out: []u8): usize` | Converts words to `UPPER_SNAKE_CASE` constant identifier (e.g. `"Bad Request"` -> `"BAD_REQUEST"`). |
| `toSnakeIdentifier` | `(input: string, out: []u8): usize` | Converts words to `lower_snake_case` identifier (e.g. `"Bad Request"` -> `"bad_request"`). |
| `isValidIdentifier` | `(s: string): bool` | Checks standard ASCII identifier syntax (`[A-Za-z_][A-Za-z0-9_]*`). |
| `isValidJsIdentifier` | `(s: string): bool` | Checks JavaScript ASCII identifier syntax (`[A-Za-z_$][A-Za-z0-9_$]*`). |

## Examples

### 1. HTTP Status Phrase to PascalCase Identifier

```typescript
import { toIdentifier, toIdentifierChecked } from "toidentifier";

fn main() {
    let buf: [64]u8 = undefined;
    let n = toIdentifier("I'm a Teapot", buf[0..]);
    // buf[0..n] == "ImATeapot"

    if (let m = toIdentifierChecked("Bad Gateway", buf[0..])) {
        // buf[0..m] == "BadGateway"
    }
}
```

### 2. Multi-Style Identifier Generation

```typescript
import { toCamelIdentifier, toConstIdentifier, toSnakeIdentifier } from "toidentifier";

fn main() {
    let camel: [64]u8 = undefined;
    let c_len = toCamelIdentifier("Not Found", camel[0..]); // "notFound"

    let cnst: [64]u8 = undefined;
    let k_len = toConstIdentifier("Not Found", cnst[0..]);  // "NOT_FOUND"

    let snake: [64]u8 = undefined;
    let s_len = toSnakeIdentifier("Not Found", snake[0..]); // "not_found"
}
```

### 3. Safe Prefixing & Validation

```typescript
import { toSafeIdentifier, isValidIdentifier, isValidJsIdentifier } from "toidentifier";

fn main() {
    let buf: [32]u8 = undefined;
    let n = toSafeIdentifier("404 Not Found", buf[0..]); // "_404NotFound"
    let ok = isValidIdentifier(buf[0..n]);               // true
    let js_ok = isValidJsIdentifier("$value_1");         // true
}
```

## Differences from Upstream & Highlights

- **Zero Heap Allocation**: Writes directly to caller-supplied `out: []u8` slices instead of constructing regexes and intermediate JS arrays.
- **Full Naming-Convention Suite**: Extends upstream PascalCase-only `toIdentifier` with `toCamelIdentifier`, `toConstIdentifier`, `toSnakeIdentifier`, `toSafeIdentifier`, and `isValidIdentifier` / `isValidJsIdentifier`.
- **Optional Overflow Guard**: `toIdentifierChecked` returns `usize?` (`null` on insufficient buffer) without truncating silently.
