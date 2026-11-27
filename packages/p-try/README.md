[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# p-try

Zero-heap structured sync/async execution outcome wrapper preventing escaping errors, with safe integer parsing, division, and short-circuit chaining (clean-room port of npm p-try)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-try` ([https://github.com/sindresorhus/p-try](https://github.com/sindresorhus/p-try))

| Metadata | Value |
| --- | --- |
| Package | `p-try` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add p-try
```

```cp
import {
    TryOutcome,
    ptryIsSpace,
    ptryIsDigit,
    ptryI64Min,
    ptryAbsU64,
    tryOkInt,
    tryOkStr,
    tryErr,
} from "p-try";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TryOutcome` | `class` | Fields: `ok: bool, value_i64: i64, value_str: string, err_code: i32, err_msg: string` · Methods: `okInt, okStr, err, isOk, isErr` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ptryIsSpace` | `(ch: u8): bool` | Invokes `ptryIsSpace` and returns `bool`. |
| `ptryIsDigit` | `(ch: u8): bool` | Invokes `ptryIsDigit` and returns `bool`. |
| `ptryI64Min` | `(): i64` | Invokes `ptryI64Min` and returns `i64`. |
| `ptryAbsU64` | `(v: i64): u64` | Invokes `ptryAbsU64` and returns `u64`. |
| `tryOkInt` | `(v: i64): TryOutcome` | Invokes `tryOkInt` and returns `TryOutcome`. |
| `tryOkStr` | `(s: string): TryOutcome` | Invokes `tryOkStr` and returns `TryOutcome`. |
| `tryErr` | `(code: i32, msg: string): TryOutcome` | Invokes `tryErr` and returns `TryOutcome`. |
| `unwrapOrInt` | `(o: TryOutcome, fallback: i64): i64` | Invokes `unwrapOrInt` and returns `i64`. |
| `unwrapOrStr` | `(o: TryOutcome, fallback: string): string` | Invokes `unwrapOrStr` and returns `string`. |
| `tryParseI64` | `(s: string): TryOutcome` | Invokes `tryParseI64` and returns `TryOutcome`. |
| `tryDivI64` | `(a: i64, b: i64): TryOutcome` | Invokes `tryDivI64` and returns `TryOutcome`. |
| `tryChain` | `(first: TryOutcome, next_ok: bool, next_val: i64, next_err_code: i32, next_err_msg: string): TryOutcome` | Invokes `tryChain` and returns `TryOutcome`. |

## Minimal Example

```cp
import { tryOkStr } from "p-try";

pub fn main() {
    let result = tryOkStr("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
