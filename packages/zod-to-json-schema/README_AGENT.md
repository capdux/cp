# zod-to-json-schema@3.24.1 (Agent Quickref)

> Zero-heap JSON Schema (Draft-07 / 2020-12) builder and serializer for structured AI tool definitions | License: MIT
> Import: `import { ... } from "zod-to-json-schema";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum SchemaType`

## Functions
- `export fn schemaTypeName(t: SchemaType): string;`
- `export fn prop(name: string, kind: SchemaType, description: string, required: bool): SchemaProp;`
- `export fn stringProp(name: string, description: string, format: string, required: bool): SchemaProp;`
- `export fn intProp(name: string, description: string, required: bool, min_val: i64, max_val: i64): SchemaProp;`
- `export fn arrayProp(name: string, item_kind: SchemaType, description: string, required: bool): SchemaProp;`
- `export fn toJsonSchema(title: string, props: &[]SchemaProp, additional_properties: bool, out: []u8): usize;`
