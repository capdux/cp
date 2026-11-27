# http-content-range@1.0.0 (Agent Quickref)

> Zero-allocation RFC 9110 HTTP Content-Range header parser and formatter (clean-room MIT implementation of crates.io http-content-range) | License: MIT
> Import: `import { ... } from "http-content-range";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum ContentRangeKind`

## Functions
- `export fn parseContentRange(header: string): ContentRange;`
- `export fn formatContentRangeHeader(cr: ContentRange, out: []u8): usize;`
