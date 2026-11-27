# lodash@1.0.0 (Agent Quickref)

> Zero-allocation dot-path parser and deep JSON path reader (dot-prop / lodash.get / dlv parity) for CP | License: MIT
> Import: `import { ... } from "lodash";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseDotPath(path: string, key_buf: []u8): ParsedDotPath;`
- `export fn getJsonPath(json: string, path: string): string?;`
- `export fn getJsonString(json: string, path: string): string?;`
- `export fn hasJsonPath(json: string, path: string): bool;`
- `export fn escapePathSegment(seg: string, out: []u8): string;`
