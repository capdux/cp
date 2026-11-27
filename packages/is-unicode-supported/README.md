[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# is-unicode-supported

Zero-allocation pure-logic terminal Unicode support detector (npm is-unicode-supported v2 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-unicode-supported` ([https://github.com/sindresorhus/is-unicode-supported](https://github.com/sindresorhus/is-unicode-supported))

| Metadata | Value |
| --- | --- |
| Package | `is-unicode-supported` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add is-unicode-supported
```

```cp
import {
    EnvInfo,
    iusStrEq,
    isUnicodeSupportedTerm,
    isUnicodeSupportedTermProgram,
    isUnicodeSupported,
    isUnicodeSupportedPlatform,
    selectFallback,
} from "is-unicode-supported";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EnvInfo` | `class` | Fields: `is_windows: bool, ci: bool, wt_session: bool, terminus_sublime: bool, conemu_task: string, term_program: string, +2 more` · Methods: `posix, posixWithTerm, linuxConsole, windows, windowsTerminal, vscodeWindows` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `iusStrEq` | `(a: string, b: string): bool` | Invokes `iusStrEq` and returns `bool`. |
| `isUnicodeSupportedTerm` | `(term: string): bool` | Checks predicate `isUnicodeSupportedTerm` and returns `bool`. |
| `isUnicodeSupportedTermProgram` | `(term_program: string): bool` | Checks predicate `isUnicodeSupportedTermProgram` and returns `bool`. |
| `isUnicodeSupported` | `(env: EnvInfo): bool` | Checks predicate `isUnicodeSupported` and returns `bool`. |
| `isUnicodeSupportedPlatform` | `(is_windows: bool, term: string): bool` | Checks predicate `isUnicodeSupportedPlatform` and returns `bool`. |
| `selectFallback` | `(env: EnvInfo, unicode_val: string, ascii_val: string): string` | Invokes `selectFallback` and returns `string`. |

## Minimal Example

```cp
import { isUnicodeSupportedTerm } from "is-unicode-supported";

pub fn main() {
    let result = isUnicodeSupportedTerm("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
