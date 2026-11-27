[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# nopt

Zero-allocation CLI option parser with shorthands, --no-flag negation, and -- terminator (npm nopt parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`nopt`](https://github.com/npm/nopt)（`ISC`）（包名 `nopt`）。

| Metadata | Value |
| --- | --- |
| Package | `nopt` |
| Version | `1.0.0` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add nopt
```

```cp
import {
    NoptFlag,
    NoptShorthand,
    NoptResult,
    nptStrEq,
    nptHasPrefix,
    nptIndexOfByte,
    nptStripLeadingDashes,
    nptFindShorthand,
} from "nopt";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `NoptFlag` | `class` | Fields: `key: string, value: string, is_bool: bool, bool_val: bool` |
| `NoptShorthand` | `class` | Fields: `short_key: string, long_key: string` |
| `NoptResult` | `class` | Fields: `flags: [64]NoptFlag, flag_count: usize, remain: [64]string, remain_count: usize` · Methods: `new, getString, getBool, has, remainCount, remainAt, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `nptStrEq` | `(a: string, b: string): bool` | Invokes `nptStrEq` and returns `bool`. |
| `nptHasPrefix` | `(s: string, prefix: string): bool` | Invokes `nptHasPrefix` and returns `bool`. |
| `nptIndexOfByte` | `(s: string, ch: u8): usize?` | Invokes `nptIndexOfByte` and returns `usize?`. |
| `nptStripLeadingDashes` | `(s: string): string` | Invokes `nptStripLeadingDashes` and returns `string`. |
| `nptFindShorthand` | `(shorthands: &[]NoptShorthand, short_name: string): string?` | Invokes `nptFindShorthand` and returns `string?`. |
| `nptIsOptionToken` | `(arg: string): bool` | Invokes `nptIsOptionToken` and returns `bool`. |
| `parseNopt` | `(args: &[]string, shorthands: &[]NoptShorthand): NoptResult` | Parses input via `parseNopt` and returns `NoptResult`. |

## Minimal Example

```cp
import { parseNopt } from "nopt";

pub fn main() {
    let result = parseNopt(/* args: &[]string */, /* shorthands: &[]NoptShorthand */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
