# strip-bom@1.0.0 (Agent Quickref)

> Pure CP zero-allocation UTF-8/UTF-16/UTF-32 BOM detection, zero-copy stripping and idempotent prepending (npm strip-bom parity) | License: MIT
> Import: `import { ... } from "strip-bom";` | Full types: `src/index.d.cp`

## Functions
- `export fn detectBom(data: []u8): BomInfo;`
- `export fn hasUtf8Bom(s: string): bool;`
- `export fn stripUtf8Bom(s: string): string;`
- `export fn stripAnyBomOffset(data: []u8): usize;`
- `export fn prependUtf8Bom(s: string, out: []u8): usize;`
