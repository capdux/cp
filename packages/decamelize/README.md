[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# decamelize

Zero-heap, single-pass ASCII converter that turns `camelCase` / `PascalCase` strings into separated lowercase (or `CONSTANT_CASE`) strings, matching npm [`decamelize`](https://github.com/sindresorhus/decamelize) (120M+/wk).

## Source Alignment

- **Upstream**: npm `decamelize` (`sindresorhus/decamelize`, MIT)
- **CP Package**: `decamelize` v1.0.0 (clean-room MIT in pure CP)
- **Design**: Zero heap allocations, supports custom separators, acronym / consecutive uppercase preservation (`preserve_consecutive_uppercase`), exact length pre-calculation, and convenience helpers for `snake_case`, `kebab-case`, space-separated, and `CONSTANT_CASE`.

## Installation & Import

```bash
cap add decamelize
```

```cp
import {
    DecamelizeOptions,
    decamelizedLen,
    decamelizeWithOptions,
    decamelizeOpts,
    decamelize,
    decamelizeDefault,
    toSnakeCase,
    toKebabCase,
    toSpaceCase,
    toConstantCase,
} from "decamelize";
```

## Core API

### Struct

| Symbol | Definition | Description |
| :--- | :--- | :--- |
| `DecamelizeOptions` | `struct { separator: string, preserve_consecutive_uppercase: bool }` | Options struct; call `DecamelizeOptions.defaults()` for `{ separator: "_", preserve_consecutive_uppercase: false }` |

### Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `decamelize` | `fn decamelize(input: string, separator: string, out: []u8): usize` | Converts `input` using `separator` with `preserve_consecutive_uppercase = false`; returns bytes written |
| `decamelizeDefault` | `fn decamelizeDefault(input: string, out: []u8): usize` | Converts `input` using the default `"_"` separator |
| `decamelizeWithOptions` | `fn decamelizeWithOptions(input: string, separator: string, preserve_consecutive_uppercase: bool, out: []u8): usize` | Full converter supporting acronym preservation (e.g. `"testURL"` -> `"test_URL"`) |
| `decamelizeOpts` | `fn decamelizeOpts(input: string, opts: DecamelizeOptions, out: []u8): usize` | Structured-options wrapper over `decamelizeWithOptions` |
| `decamelizedLen` | `fn decamelizedLen(input: string, separator: string, preserve_consecutive_uppercase: bool): usize` | Exact byte length produced by `decamelizeWithOptions` |
| `toSnakeCase` | `fn toSnakeCase(input: string, out: []u8): usize` | Converts `camelCase` / `PascalCase` to `snake_case` |
| `toKebabCase` | `fn toKebabCase(input: string, out: []u8): usize` | Converts `camelCase` / `PascalCase` to `kebab-case` |
| `toSpaceCase` | `fn toSpaceCase(input: string, out: []u8): usize` | Converts `camelCase` / `PascalCase` to `"space separated"` |
| `toConstantCase` | `fn toConstantCase(input: string, out: []u8): usize` | Converts `camelCase` / `PascalCase` to `UPPER_SNAKE_CASE` |

## Examples

### 1. Basic Case Conversions

```cp
import { decamelize, toKebabCase, toConstantCase } from "decamelize";
import { print } from "cap:io";

pub fn main() {
    let buf: [64]u8 = [0; 64];

    const n1 = decamelize("unicornRainbow", "_", buf[0..64]);
    print(buf[0..n1] as string); // "unicorn_rainbow"

    const n2 = toKebabCase("xmlHttpRequest", buf[0..64]);
    print(buf[0..n2] as string); // "xml-http-request"

    const n3 = toConstantCase("maxRetryCount", buf[0..64]);
    print(buf[0..n3] as string); // "MAX_RETRY_COUNT"
}
```

### 2. Preserving Consecutive Uppercase Acronyms

```cp
import { DecamelizeOptions, decamelizeOpts } from "decamelize";

pub fn formatAcronym(input: string, out: []u8): usize {
    const opts = DecamelizeOptions {
        separator: "_",
        preserve_consecutive_uppercase: true,
    };
    return decamelizeOpts(input, opts, out); // "testURL" -> "test_URL"
}
```

### 3. Pre-Sizing Before Writing

```cp
import { decamelizedLen, decamelizeWithOptions } from "decamelize";

pub fn safeDecamelize(input: string, sep: string, out: []u8): usize {
    if (decamelizedLen(input, sep, false) > out.len) {
        return 0;
    }
    return decamelizeWithOptions(input, sep, false, out);
}
```

## Differences from npm `decamelize` & Safety Highlights

- **Zero-Heap Buffer Model**: Writes directly into caller-owned `[]u8` slices in a single pass with zero regex or heap allocations.
- **ASCII Case Logic**: Optimized for ASCII identifiers (`A-Z`, `a-z`, `0-9`), matching CLI flag, JSON key, and env-var conversion workloads.
- **Convenience Helpers**: Includes `toSnakeCase`, `toKebabCase`, `toSpaceCase`, `toConstantCase`, and `decamelizedLen` out of the box.
