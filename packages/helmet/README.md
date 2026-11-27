[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# helmet

Zero-heap HTTP security response headers policy generator (CSP, HSTS, COOP, CORP, X-Frame-Options, Referrer-Policy) with CRLF-injection defense in pure CP (clean-room MIT counterpart to npm helmet v8).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:helmet` ([https://github.com/helmetjs/helmet](https://github.com/helmetjs/helmet))（原库名 `helmet` → CP 包名 `helmet`）

| Metadata | Value |
| --- | --- |
| Package | `helmet` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add helmet
```

```cp
import {
    HelmetOptions,
    hlmWriteByte,
    hlmWriteStr,
    hlmWriteU64,
    hlmWriteHeaderLine,
    hlmAppendCspDirective,
    isSafeHeaderValue,
    formatHstsHeader,
} from "helmet";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HelmetOptions` | `class` | Fields: `csp_enabled: bool, csp_value: string, hsts_enabled: bool, hsts_max_age: u64, hsts_include_subdomains: bool, hsts_preload: bool, +8 more` · Methods: `defaultStrict, minimal` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hlmWriteByte` | `(out: []u8, pos: &mut usize, b: u8)` | Executes `hlmWriteByte` in-place. |
| `hlmWriteStr` | `(out: []u8, pos: &mut usize, s: string)` | Executes `hlmWriteStr` in-place. |
| `hlmWriteU64` | `(out: []u8, pos: &mut usize, val: u64)` | Executes `hlmWriteU64` in-place. |
| `hlmWriteHeaderLine` | `(out: []u8, pos: &mut usize, name: string, val: string)` | Executes `hlmWriteHeaderLine` in-place. |
| `hlmAppendCspDirective` | `(out: []u8, pos: &mut usize, has_any: &mut bool, dir_name: string, val: string)` | Executes `hlmAppendCspDirective` in-place. |
| `isSafeHeaderValue` | `(val: string): bool` | Checks predicate `isSafeHeaderValue` and returns `bool`. |
| `formatHstsHeader` | `(max_age: u64, include_subdomains: bool, preload: bool, out: []u8): usize` | Serializes/formats data via `formatHstsHeader` returning `usize`. |
| `formatCspHeader` | `(default_src: string, script_src: string, style_src: string, img_src: string, connect_src: string, out: []u8): usize` | Serializes/formats data via `formatCspHeader` returning `usize`. |
| `formatHelmetHeaders` | `(opts: HelmetOptions, out: []u8): usize` | Serializes/formats data via `formatHelmetHeaders` returning `usize`. |

## Minimal Example

```cp
import { isSafeHeaderValue } from "helmet";

pub fn main() {
    let result = isSafeHeaderValue("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
