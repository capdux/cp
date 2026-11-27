[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# stack-utils

Zero-heap V8 stack trace line parser extracting function name, file path, line, column, and native flag

| Metadata | Value |
| --- | --- |
| Package | `stack-utils` |
| Version | `2.0.6` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add stack-utils
```

```cp
import { StackFrame, stkuInvalid, stkuParseLoc, parseStackLine } from "stack-utils";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `StackFrame` | `class` | Fields: `function_name: string, file: string, line: u32, column: u32, is_native: bool, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `stkuInvalid` | `(): StackFrame` | Invokes `stkuInvalid` and returns `StackFrame`. |
| `stkuParseLoc` | `(fn_name: string, loc: string): StackFrame` | Invokes `stkuParseLoc` and returns `StackFrame`. |
| `parseStackLine` | `(line: string): StackFrame` | Parses input via `parseStackLine` and returns `StackFrame`. |

## Minimal Example

```cp
import { parseStackLine } from "stack-utils";

pub fn main() {
    let result = parseStackLine("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
