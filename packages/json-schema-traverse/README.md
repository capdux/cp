[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# json-schema-traverse

Zero-allocation JSON Schema traversal engine with RFC 6901 JSON Pointer generation for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:json-schema-traverse` ([https://github.com/epoberezkin/json-schema-traverse](https://github.com/epoberezkin/json-schema-traverse))

| Metadata | Value |
| --- | --- |
| Package | `json-schema-traverse` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add json-schema-traverse
```

```cp
import {
    SchemaNodeKind,
    SchemaVisitEvent,
    JstvSpan,
    JstvTraverseState,
    jstvIsWs,
    jstvSkipWs,
    jstvScanString,
    jstvScanValue,
} from "json-schema-traverse";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `SchemaNodeKind` | `Root, Property, Items, AdditionalProperties, AllOf, AnyOf, OneOf, Not ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SchemaVisitEvent` | `class` | Fields: `pointer_start: usize, pointer_end: usize, pointer_buf: [128]u8, pointer_len: usize, kind: SchemaNodeKind, schema_start: usize, +2 more` · Methods: `pointer, schemaSlice` |
| `JstvSpan` | `class` | Fields: `start: usize, end: usize, ok: bool` |
| `JstvTraverseState` | `class` | Fields: `src: string, out: []SchemaVisitEvent, written: usize, total: usize, target_ptr: string, find_mode: bool, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `jstvIsWs` | `(b: u8): bool` | Invokes `jstvIsWs` and returns `bool`. |
| `jstvSkipWs` | `(src: string, start: usize): usize` | Invokes `jstvSkipWs` and returns `usize`. |
| `jstvScanString` | `(src: string, start: usize): JstvSpan` | Invokes `jstvScanString` and returns `JstvSpan`. |
| `jstvScanValue` | `(src: string, start: usize): JstvSpan` | Invokes `jstvScanValue` and returns `JstvSpan`. |
| `jstvStrEq` | `(a: string, b: string): bool` | Invokes `jstvStrEq` and returns `bool`. |
| `jstvCopyBuf` | `(src: [128]u8, len: usize): [128]u8` | Invokes `jstvCopyBuf` and returns `[128]u8`. |
| `jstvAppendByte` | `(buf: &mut [128]u8, cur_len: usize, b: u8): usize` | Invokes `jstvAppendByte` and returns `usize`. |
| `jstvAppendEscapedSegment` | `(buf: &mut [128]u8, cur_len: usize, seg: string): usize` | Invokes `jstvAppendEscapedSegment` and returns `usize`. |
| `jstvAppendIndexSegment` | `(buf: &mut [128]u8, cur_len: usize, idx: usize): usize` | Invokes `jstvAppendIndexSegment` and returns `usize`. |
| `jstvEmitEvent` | `(st: &mut JstvTraverseState, ptr_buf: [128]u8, ptr_len: usize, kind: SchemaNodeKind, schema_start: usize, schema_end: usize, depth: usize)` | Executes `jstvEmitEvent` in-place. |
| `jstvWalkMapKeyword` | `(st: &mut JstvTraverseState, obj_start: usize, obj_end: usize, base_buf: [128]u8, base_len: usize, kw: string, kind: SchemaNodeKind, depth: usize)` | Executes `jstvWalkMapKeyword` in-place. |
| `jstvWalkArrayKeyword` | `(st: &mut JstvTraverseState, arr_start: usize, arr_end: usize, base_buf: [128]u8, base_len: usize, kw: string, kind: SchemaNodeKind, depth: usize)` | Executes `jstvWalkArrayKeyword` in-place. |
| `jstvWalkSchema` | `(st: &mut JstvTraverseState, schema_start: usize, schema_end: usize, ptr_buf: [128]u8, ptr_len: usize, kind: SchemaNodeKind, depth: usize)` | Executes `jstvWalkSchema` in-place. |
| `traverseJsonSchema` | `(schema_json: string, out: []SchemaVisitEvent): usize` | Invokes `traverseJsonSchema` and returns `usize`. |
| `countSchemaNodes` | `(schema_json: string): usize` | Invokes `countSchemaNodes` and returns `usize`. |
| `findSubschemaByPointer` | `(schema_json: string, pointer: string): string?` | Queries/retrieves data via `findSubschemaByPointer` returning `string?`. |

## Minimal Example

```cp
import { jstvSkipWs } from "json-schema-traverse";

pub fn main() {
    let result = jstvSkipWs("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
