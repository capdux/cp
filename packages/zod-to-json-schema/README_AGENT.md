# zod-to-json-schema — AI Agent Quick Ref

Zero-heap JSON Schema (Draft-07 / 2020-12) builder and serializer for structured AI tool definitions (`v3.24.1`, `MIT`)

## Import

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

## Core Types & Constants

```cp
enum SchemaType { String, Number, Integer, Boolean, Array, Object, ... }
class SchemaProp { name: string, kind: SchemaType, description: string, required: bool, /* +6 fields */ }
```

## API Signatures

```cp
fn schemaTypeName(t: SchemaType): string;
fn prop(name: string, kind: SchemaType, description: string, required: bool): SchemaProp;
fn stringProp(name: string, description: string, format: string, required: bool): SchemaProp;
fn intProp(name: string, description: string, required: bool, min_val: i64, max_val: i64): SchemaProp;
fn arrayProp(name: string, item_kind: SchemaType, description: string, required: bool): SchemaProp;
fn toJsonSchema(title: string, props: &[]SchemaProp, additional_properties: bool, out: []u8): usize;
```

## Minimal Usage

```cp
import { prop } from "zod-to-json-schema";

pub fn main() {
    let result = prop("example", /* kind: SchemaType */, "example", false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"zod-to-json-schema"`; install via `cap add zod-to-json-schema`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
