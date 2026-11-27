[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# json-schema-traverse

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation JSON Schema traversal engine with RFC 6901 JSON Pointer generation for CP

## 安装与导入

```bash
cap add json-schema-traverse
```

```cp
import { jstvIsWs } from "json-schema-traverse";
```

## 核心导出 API

- `export enum SchemaNodeKind`
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

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`json-schema-traverse@1.0.0`)，许可证：MIT
