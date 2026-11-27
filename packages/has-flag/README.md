[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# has-flag

Check if CLI argv contains a specific flag (clean-room CP implementation of npm has-flag)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:has-flag` ([https://github.com/sindresorhus/has-flag](https://github.com/sindresorhus/has-flag))

| Metadata | Value |
| --- | --- |
| Package | `has-flag` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add has-flag
```

```cp
import {
    hfEq,
    hfStartsWith,
    hfMatchesFlag,
    hfComputePrefixLen,
    findFlagIndex,
    hasFlag,
    getFlagValue,
    isFlagEnabled,
} from "has-flag";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hfEq` | `(a: string, b: string): bool` | Invokes `hfEq` and returns `bool`. |
| `hfStartsWith` | `(s: string, prefix: string): bool` | Invokes `hfStartsWith` and returns `bool`. |
| `hfMatchesFlag` | `(arg: string, flag: string, prefix_len: usize): bool` | Invokes `hfMatchesFlag` and returns `bool`. |
| `hfComputePrefixLen` | `(flag: string): usize` | Invokes `hfComputePrefixLen` and returns `usize`. |
| `findFlagIndex` | `(flag: string, argv: &[]string): i64` | Queries/retrieves data via `findFlagIndex` returning `i64`. |
| `hasFlag` | `(flag: string, argv: &[]string): bool` | Checks predicate `hasFlag` and returns `bool`. |
| `getFlagValue` | `(flag: string, argv: &[]string, out: []u8): i64` | Queries/retrieves data via `getFlagValue` returning `i64`. |
| `isFlagEnabled` | `(flag: string, argv: &[]string, default_val: bool): bool` | Checks predicate `isFlagEnabled` and returns `bool`. |

## Minimal Example

```cp
import { getFlagValue } from "has-flag";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = getFlagValue("example", /* argv: &[]string */, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
