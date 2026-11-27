# did-resolver@1.0.0 (Agent Quickref)

> Zero-heap W3C DID Core 1.0 Decentralized Identifier & DID URL parser, strict character validator, and formatter. | License: MIT
> Import: `import { ... } from "did-resolver";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValidMethod(method: string): bool;`
- `export fn isValidMethodId(id: string): bool;`
- `export fn parseDid(input: string): DidUrl;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn formatDid(method: string, id: string, fragment: string, out: []u8): usize;`
