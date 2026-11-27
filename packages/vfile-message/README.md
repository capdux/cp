[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# vfile-message

Zero-allocation structured lint/AST diagnostic message with unist Point/Position formatting

| Metadata | Value |
| --- | --- |
| Package | `vfile-message` |
| Version | `4.0.2` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add vfile-message
```

```cp
import {
    Severity,
    Point,
    Position,
    VFileMessage,
    makePoint,
    makePosition,
    pointAtOffset,
    createMessage,
} from "vfile-message";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `Severity` | `Info, Warning, Error` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Point` | `class` | Fields: `line: u32, column: u32, offset: u32` |
| `Position` | `class` | Fields: `start: Point, end: Point` |
| `VFileMessage` | `class` | Fields: `reason: string, source: string, rule_id: string, file_path: string, severity: Severity, position: Position` · Methods: `isFatal, hasPosition` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `makePoint` | `(line: u32, column: u32, offset: u32): Point` | Invokes `makePoint` and returns `Point`. |
| `makePosition` | `(start: Point, end: Point): Position` | Invokes `makePosition` and returns `Position`. |
| `pointAtOffset` | `(text: string, offset: usize): Point` | Invokes `pointAtOffset` and returns `Point`. |
| `createMessage` | `(reason: string, file_path: string, source: string, rule_id: string, severity: Severity, at: Point): VFileMessage` | Constructs/initializes an instance via `createMessage`. |
| `stringifyPosition` | `(pos: Position, out: []u8): usize` | Serializes/formats data via `stringifyPosition` returning `usize`. |
| `formatMessage` | `(msg: VFileMessage, out: []u8): usize` | Serializes/formats data via `formatMessage` returning `usize`. |

## Minimal Example

```cp
import { pointAtOffset } from "vfile-message";

pub fn main() {
    let result = pointAtOffset("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
