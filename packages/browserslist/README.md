[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# browserslist

Zero-heap Browserslist query clause parser and browser version matcher

| Metadata | Value |
| --- | --- |
| Package | `browserslist` |
| Version | `4.24.4` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add browserslist
```

```cp
import {
    BrowserOp,
    BrowserClause,
    blqStartsWithWord,
    blqParseU32,
    blqIsDeadBrowser,
    blqCmpVersion,
    parseBrowserClause,
    matchBrowserVersion,
} from "browserslist";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `BrowserOp` | `Ge, Gt, Le, Lt, Eq, LastN, Defaults, Dead ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BrowserClause` | `class` | Fields: `negated: bool, is_and: bool, browser: string, op: BrowserOp, version_major: u32, version_minor: u32, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `blqStartsWithWord` | `(s: string, word: string): bool` | Invokes `blqStartsWithWord` and returns `bool`. |
| `blqParseU32` | `(s: string): u32` | Invokes `blqParseU32` and returns `u32`. |
| `blqIsDeadBrowser` | `(browser: string): bool` | Invokes `blqIsDeadBrowser` and returns `bool`. |
| `blqCmpVersion` | `(maj_a: u32, min_a: u32, maj_b: u32, min_b: u32): i32` | Invokes `blqCmpVersion` and returns `i32`. |
| `parseBrowserClause` | `(clause: string): BrowserClause` | Parses input via `parseBrowserClause` and returns `BrowserClause`. |
| `matchBrowserVersion` | `(clause: BrowserClause, browser: string, major: u32, minor: u32, latest_major: u32): bool` | Invokes `matchBrowserVersion` and returns `bool`. |

## Minimal Example

```cp
import { parseBrowserClause } from "browserslist";

pub fn main() {
    let result = parseBrowserClause("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
