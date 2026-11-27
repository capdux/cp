# unflatten@1.0.0 (Agent Quickref)

> Zero-allocation nested JSON flatten & unflatten library (npm flat/unflatten parity) with custom delimiter, max_depth, safe array mode, and round-trip reconstruction for CP | License: MIT
> Import: `import { ... } from "unflatten";` | Full types: `src/index.d.cp`

## Functions
- `export fn flattenJson(json_in: string, opts: FlatOptions, out_table: &mut FlatTable): i64;`
- `export fn flattenToJson(json_in: string, opts: FlatOptions, out: []u8): i64;`
- `export fn unflattenJson(flat_json: string, opts: FlatOptions, out: []u8): i64;`
