# lexical-parse-float@1.0.0 (Agent Quickref)

> Zero-allocation pure CP strict floating-point and radix integer string parser (rust-lexical lexical-parse-float / lexical-parse-integer / lexical-util parity) | License: MIT
> Import: `import { ... } from "lexical-parse-float";` | Full types: `src/index.d.cp`

## Functions
- `export fn lexpToLower(c: u8): u8;`
- `export fn lexpEqIgnoreCase(a: string, b: string): bool;`
- `export fn lexpDecodeDigit(c: u8): i32;`
- `export fn lexpScalePow10(val_in: f64, exp10: i32): f64;`
- `export fn parseFloat(s: string): ParseFloatResult;`
- `export fn parseIntRadix(s: string, radix: u32): ParseIntResult;`
