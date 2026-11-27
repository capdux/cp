[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# error-stack-parser

Zero-allocation multi-language stack trace parser & cleaner supporting V8/Node/Bun, SpiderMonkey/Safari, and Rust/Zig/CP formats (error-stack-parser / stacktrace-parser / clean-stack parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:error-stack-parser` ([https://github.com/stacktracejs/error-stack-parser](https://github.com/stacktracejs/error-stack-parser))（原库名 `error-stack-parser` → CP 包名 `error-stack-parser`）

| Metadata | Value |
| --- | --- |
| Package | `error-stack-parser` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add error-stack-parser
```

```cp
import {
    StackFrame,
    ParsedStack,
    LocationInfo,
    isInternalLocation,
    parseLocation,
    parseFrameLine,
    parseStack,
    pushByte,
} from "error-stack-parser";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `StackFrame` | `class` | Fields: `func_name: string, file: string, line: u32, column: u32, is_native: bool, is_internal: bool` · Methods: `empty` |
| `ParsedStack` | `class` | Fields: `frames: [MAX_FRAMES]StackFrame, count: usize, message: string` · Methods: `empty, frameAt` |
| `LocationInfo` | `class` | Fields: `file: string, line: u32, column: u32, is_native: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isInternalLocation` | `(file: string, is_native: bool): bool` | Checks predicate `isInternalLocation` and returns `bool`. |
| `parseLocation` | `(raw_loc: string): LocationInfo` | Parses input via `parseLocation` and returns `LocationInfo`. |
| `parseFrameLine` | `(raw_line: string): StackFrame?` | Parses input via `parseFrameLine` and returns `StackFrame?`. |
| `parseStack` | `(raw: string): ParsedStack` | Parses input via `parseStack` and returns `ParsedStack`. |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | Mutates or appends state via `pushByte`. |
| `formatFrameAt` | `(f: StackFrame, out: []u8, start_pos: usize): usize` | Serializes/formats data via `formatFrameAt` returning `usize`. |
| `formatFrame` | `(f: StackFrame, out: []u8): string` | Serializes/formats data via `formatFrame` returning `string`. |
| `cleanStack` | `(raw: string, out: []u8): string` | Invokes `cleanStack` and returns `string`. |

## Minimal Example

```cp
import { isInternalLocation } from "error-stack-parser";

pub fn main() {
    let result = isInternalLocation("example", false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
