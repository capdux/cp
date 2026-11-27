# error-ex@1.3.2 (Agent Quickref)

> Zero-heap structured error and context formatter with template interpolation | License: MIT
> Import: `import { ... } from "error-ex";` | Full types: `src/index.d.cp`

## Functions
- `export fn erexWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn erexWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn erexWriteI32(out: []u8, pos: usize, val: i32): usize;`
- `export fn formatStructuredError(e: StructuredError, fields: &[]ErrorField, out: []u8): usize;`
- `export fn erexMatchesPlaceholder(tmpl: string, start: usize, key: string): bool;`
- `export fn formatErrorTemplate(tmpl: string, key: string, val: string, out: []u8): usize;`
