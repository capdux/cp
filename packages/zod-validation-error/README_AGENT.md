# zod-validation-error@3.4.0 (Agent Quickref)

> Zero-heap human-readable validation error aggregator and formatter (npm zod-validation-error equivalent) | License: MIT
> Import: `import { ... } from "zod-validation-error";` | Full types: `src/index.d.cp`

## Functions
- `export fn defaultFormatErrorOptions(): FormatErrorOptions;`
- `export fn zverWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn zverWriteByte(out: []u8, pos: usize, ch: u8): usize;`
- `export fn formatValidationIssues(issues: &[]ValidationIssue, opts: FormatErrorOptions, out: []u8): usize;`
