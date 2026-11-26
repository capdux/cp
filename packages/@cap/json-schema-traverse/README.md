# @cap/json-schema-traverse

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:json-schema-traverse` ([https://github.com/epoberezkin/json-schema-traverse](https://github.com/epoberezkin/json-schema-traverse))

Zero-allocation JSON Schema traversal engine with RFC 6901 JSON Pointer generation for CP — clean-room MIT parity with npm `json-schema-traverse` (477M+/wk).

纯 CP 零堆分配 JSON Schema 递归遍历引擎，完整支持 `properties`、`patternProperties`、`items`（对象与数组元组）、`additionalProperties`、`allOf`/`anyOf`/`oneOf`/`not` 及 `$defs`/`definitions`，自动生成符合 RFC 6901 规范的 JSON Pointer（含 `~0`/`~1` 转义）。

## Install / 安装

```bash
cap add @cap/json-schema-traverse
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `SchemaNodeKind` | `enum` | `Root`, `Property`, `Items`, `AdditionalProperties`, `AllOf`, `AnyOf`, `OneOf`, `Not`, `Defs` |
| `SchemaVisitEvent` | `struct` | Visit event with `pointer_buf`, `pointer_len`, `kind`, `schema_start`, `schema_end`, `depth`, `.pointer()`, `.schemaSlice(src)` |
| `traverseJsonSchema` | `(schema_json: string, out: []SchemaVisitEvent): usize` | Recursively traverse schema into caller buffer and return event count |
| `countSchemaNodes` | `(schema_json: string): usize` | Count total schema nodes including root without allocating |
| `findSubschemaByPointer` | `(schema_json: string, pointer: string): string?` | Locate subschema JSON slice by RFC 6901 JSON Pointer |

## Usage / 使用示例

```cp
import { traverseJsonSchema, findSubschemaByPointer, SchemaVisitEvent } from "@cap/json-schema-traverse";

const schema = "{\"type\":\"object\",\"properties\":{\"name\":{\"type\":\"string\"}}}";
let events: [16]SchemaVisitEvent = undefined;
const n = traverseJsonSchema(schema, events[0..16]); // 2 ("", "/properties/name")
const sub = findSubschemaByPointer(schema, "/properties/name"); // "{\"type\":\"string\"}"
```
