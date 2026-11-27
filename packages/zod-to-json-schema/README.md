[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# zod-to-json-schema

Zero-heap JSON Schema (Draft-07 / 2020-12) builder and serializer for structured AI tool definitions

| Metadata | Value |
| --- | --- |
| Package | `zod-to-json-schema` |
| Version | `3.24.1` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add zod-to-json-schema
```

```cp
import {
    SchemaType,
    SchemaProp,
    schemaTypeName,
    prop,
    stringProp,
    intProp,
    arrayProp,
    toJsonSchema,
} from "zod-to-json-schema";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `SchemaType` | `String, Number, Integer, Boolean, Array, Object, Null` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SchemaProp` | `class` | Fields: `name: string, kind: SchemaType, description: string, required: bool, format: string, item_kind: SchemaType, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `schemaTypeName` | `(t: SchemaType): string` | Invokes `schemaTypeName` and returns `string`. |
| `prop` | `(name: string, kind: SchemaType, description: string, required: bool): SchemaProp` | Invokes `prop` and returns `SchemaProp`. |
| `stringProp` | `(name: string, description: string, format: string, required: bool): SchemaProp` | Invokes `stringProp` and returns `SchemaProp`. |
| `intProp` | `(name: string, description: string, required: bool, min_val: i64, max_val: i64): SchemaProp` | Invokes `intProp` and returns `SchemaProp`. |
| `arrayProp` | `(name: string, item_kind: SchemaType, description: string, required: bool): SchemaProp` | Invokes `arrayProp` and returns `SchemaProp`. |
| `toJsonSchema` | `(title: string, props: &[]SchemaProp, additional_properties: bool, out: []u8): usize` | Invokes `toJsonSchema` and returns `usize`. |

## Minimal Example

```cp
import { prop } from "zod-to-json-schema";

pub fn main() {
    let result = prop("example", /* kind: SchemaType */, "example", false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
