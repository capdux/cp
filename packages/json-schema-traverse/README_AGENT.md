# json-schema-traverse@1.0.0 (Agent Quickref)

> Zero-allocation JSON Schema traversal engine with RFC 6901 JSON Pointer generation for CP | License: MIT
> Import: `import { ... } from "json-schema-traverse";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum SchemaNodeKind`

## Functions
- `export fn jstvIsWs(b: u8): bool;`
- `export fn jstvSkipWs(src: string, start: usize): usize;`
- `export fn jstvScanString(src: string, start: usize): JstvSpan;`
- `export fn jstvScanValue(src: string, start: usize): JstvSpan;`
- `export fn jstvStrEq(a: string, b: string): bool;`
- `export fn jstvCopyBuf(src: [128]u8, len: usize): [128]u8;`
- `export fn jstvAppendByte(buf: &mut [128]u8, cur_len: usize, b: u8): usize;`
- `export fn jstvAppendEscapedSegment(buf: &mut [128]u8, cur_len: usize, seg: string): usize;`
- `export fn jstvAppendIndexSegment(buf: &mut [128]u8, cur_len: usize, idx: usize): usize;`
- `export fn jstvEmitEvent(st: &mut JstvTraverseState, ptr_buf: [128]u8, ptr_len: usize, kind: SchemaNodeKind, schema_start: usize, schema_end: usize, depth: usize);`
- `export fn jstvWalkMapKeyword(st: &mut JstvTraverseState, obj_start: usize, obj_end: usize, base_buf: [128]u8, base_len: usize, kw: string, kind: SchemaNodeKind, depth: usize);`
- `export fn jstvWalkArrayKeyword(st: &mut JstvTraverseState, arr_start: usize, arr_end: usize, base_buf: [128]u8, base_len: usize, kw: string, kind: SchemaNodeKind, depth: usize);`
- `export fn jstvWalkSchema(st: &mut JstvTraverseState, schema_start: usize, schema_end: usize, ptr_buf: [128]u8, ptr_len: usize, kind: SchemaNodeKind, depth: usize);`
- `export fn traverseJsonSchema(schema_json: string, out: []SchemaVisitEvent): usize;`
- `export fn countSchemaNodes(schema_json: string): usize;`
- `export fn findSubschemaByPointer(schema_json: string, pointer: string): string?;`
