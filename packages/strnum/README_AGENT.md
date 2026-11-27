# strnum@1.0.0 (Agent Quickref)

> Zero-heap configurable string-to-number parser for CP (clean-room MIT implementation of npm strnum). | License: MIT
> Import: `import { ... } from "strnum";` | Full types: `src/index.d.cp`

## Functions
- `export fn snumInvalid(): StrNumResult;`
- `export fn snumIsSpace(ch: u8): bool;`
- `export fn snumHexDigit(ch: u8): i32;`
- `export fn snumScalePow10(val: f64, exp10: i32): f64;`
- `export fn parseStrNum(input: string, opts: StrNumOptions): StrNumResult;`
