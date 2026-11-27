[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cookie

Zero-allocation Rust-style CookieBuilder, percent-encoded Set-Cookie helpers, and removal cookie generator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cookie` ([https://github.com/SergioBenitez/cookie-rs](https://github.com/SergioBenitez/cookie-rs))（原库名 `cookie` → CP 包名 `cookie`）

| Metadata | Value |
| --- | --- |
| Package | `cookie` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cookie
```

```cp
import {
    EncodedCookieMeta,
    CookieBuilder,
    ckcDefaultCookie,
    parseEncodedCookie,
    formatEncodedCookie,
    makeRemovalCookie,
} from "cookie";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EncodedCookieMeta` | `class` | Fields: `name_len: usize, value_len: usize, domain: string, path: string, max_age: i64, has_max_age: bool, +4 more` |
| `CookieBuilder` | `class` | Fields: `cookie: Cookie` · Methods: `new, domain, path, maxAge, secure, httpOnly, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ckcDefaultCookie` | `(name: string, value: string): Cookie` | Invokes `ckcDefaultCookie` and returns `Cookie`. |
| `parseEncodedCookie` | `(header: string, name_out: []u8, val_out: []u8): EncodedCookieMeta` | Parses input via `parseEncodedCookie` and returns `EncodedCookieMeta`. |
| `formatEncodedCookie` | `(c: Cookie, out: []u8): usize` | Serializes/formats data via `formatEncodedCookie` returning `usize`. |
| `makeRemovalCookie` | `(name: string, path: string, out: []u8): usize` | Invokes `makeRemovalCookie` and returns `usize`. |

## Minimal Example

```cp
import { parseEncodedCookie } from "cookie";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = parseEncodedCookie("example", out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
