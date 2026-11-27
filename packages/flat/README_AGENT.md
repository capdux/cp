# flat@1.0.1 (Agent Quickref)

> Zero-allocation JSON flattening (a.b.0.c), dot-prop path lookup & flat-to-JSON reconstruction engine (flat / dot-prop parity) for CP | License: MIT
> Import: `import { ... } from "flat";` | Full types: `src/index.d.cp`

## Functions
- `export fn flattenJsonWith(json_str: string, sep: u8): FlatResult;`
- `export fn flattenJson(json_str: string): FlatResult;`
- `export fn getByPath(json_str: string, dot_path: string, out: []u8): string?;`
- `export fn hasPath(json_str: string, dot_path: string): bool;`
- `export fn formatFlatJsonObject(flat: FlatResult, out: []u8): string;`
