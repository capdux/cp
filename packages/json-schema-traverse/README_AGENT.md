# json-schema-traverse — AI Agent Quick Ref

Zero-allocation JSON Schema traversal engine with RFC 6901 JSON Pointer generation for CP (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
enum SchemaNodeKind { Root, Property, Items, AdditionalProperties, AllOf, AnyOf, ... }
class SchemaVisitEvent { pointer_start: usize, pointer_end: usize, pointer_buf: [128]u8, pointer_len: usize, /* +4 fields */ } // methods: pointer(...) +1
class JstvSpan { start: usize, end: usize, ok: bool }
class JstvTraverseState { src: string, out: []SchemaVisitEvent, written: usize, total: usize, /* +5 fields */ }
```

## API Signatures

```cp
fn jstvIsWs(b: u8): bool;
fn jstvSkipWs(src: string, start: usize): usize;
fn jstvScanString(src: string, start: usize): JstvSpan;
fn jstvScanValue(src: string, start: usize): JstvSpan;
fn jstvStrEq(a: string, b: string): bool;
fn jstvCopyBuf(src: [128]u8, len: usize): [128]u8;
fn jstvAppendByte(buf: &mut [128]u8, cur_len: usize, b: u8): usize;
fn jstvAppendEscapedSegment(buf: &mut [128]u8, cur_len: usize, seg: string): usize;
fn jstvAppendIndexSegment(buf: &mut [128]u8, cur_len: usize, idx: usize): usize;
fn jstvEmitEvent(st: &mut JstvTraverseState, ptr_buf: [128]u8, ptr_len: usize, kind: SchemaNodeKind, schema_start: usize, schema_end: usize, depth: usize);
fn jstvWalkMapKeyword(st: &mut JstvTraverseState, obj_start: usize, obj_end: usize, base_buf: [128]u8, base_len: usize, kw: string, kind: SchemaNodeKind, depth: usize);
fn jstvWalkArrayKeyword(st: &mut JstvTraverseState, arr_start: usize, arr_end: usize, base_buf: [128]u8, base_len: usize, kw: string, kind: SchemaNodeKind, depth: usize);
fn jstvWalkSchema(st: &mut JstvTraverseState, schema_start: usize, schema_end: usize, ptr_buf: [128]u8, ptr_len: usize, kind: SchemaNodeKind, depth: usize);
fn traverseJsonSchema(schema_json: string, out: []SchemaVisitEvent): usize;
fn countSchemaNodes(schema_json: string): usize;
fn findSubschemaByPointer(schema_json: string, pointer: string): string?;
```

## Minimal Usage

```cp
import { jstvSkipWs } from "json-schema-traverse";

pub fn main() {
    let result = jstvSkipWs("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"json-schema-traverse"`; install via `cap add json-schema-traverse`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
