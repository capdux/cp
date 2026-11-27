[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# clean-stack

Zero-allocation stack trace cleaner removing internal runtime frames and normalizing base paths (clean-room MIT implementation of npm clean-stack v5)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:clean-stack` ([https://github.com/sindresorhus/clean-stack](https://github.com/sindresorhus/clean-stack)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `clean-stack` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add clean-stack
```

```cp
import {
    CleanStackOptions,
    csContains,
    csStartsWith,
    csAppend,
    isInternalStackFrame,
    csWriteCleanedLine,
    cleanStackWithOptions,
    cleanStack,
} from "clean-stack";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CleanStackOptions` | `class` | Fields: `pretty: bool, base_path: string, home_dir: string` · Methods: `defaults` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `csContains` | `(hay: string, needle: string): bool` | Invokes `csContains` and returns `bool`. |
| `csStartsWith` | `(s: string, prefix: string): bool` | Invokes `csStartsWith` and returns `bool`. |
| `csAppend` | `(out: []u8, pos: usize, s: string): usize` | Invokes `csAppend` and returns `usize`. |
| `isInternalStackFrame` | `(line: string): bool` | Checks predicate `isInternalStackFrame` and returns `bool`. |
| `csWriteCleanedLine` | `(line: string, opts: CleanStackOptions, out: []u8, pos: usize): usize` | Invokes `csWriteCleanedLine` and returns `usize`. |
| `cleanStackWithOptions` | `(stack: string, opts: CleanStackOptions, out: []u8): usize` | Invokes `cleanStackWithOptions` and returns `usize`. |
| `cleanStack` | `(stack: string, out: []u8): usize` | Invokes `cleanStack` and returns `usize`. |

## Minimal Example

```cp
import { isInternalStackFrame } from "clean-stack";

pub fn main() {
    let result = isInternalStackFrame("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
