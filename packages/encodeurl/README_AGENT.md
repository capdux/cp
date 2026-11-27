# encodeurl@1.0.0 (Agent Quickref)

> Zero-heap idempotent URL encoder that percent-encodes unsafe characters without double-encoding existing %XX sequences (clean-room port of npm encodeurl) | License: MIT
> Import: `import { ... } from "encodeurl";` | Full types: `src/index.d.cp`

## Functions
- `export fn euIsHex(b: u8): bool;`
- `export fn euNeedsEncode(b: u8): bool;`
- `export fn encodeUrl(url: string, out: []u8): usize;`
- `export fn isEncodedUrl(url: string): bool;`
