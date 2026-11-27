# romans@1.0.0 (Agent Quickref)

> Zero-allocation strict Roman numeral (1..3999) bidirectional parser, validator, and formatter in pure CP | License: MIT
> Import: `import { ... } from "romans";` | Full types: `src/index.d.cp`

## Functions
- `export fn toUpperAscii(b: u8): u8;`
- `export fn romanCharVal(b: u8): i32;`
- `export fn toRoman(num: u16, out: []u8): usize;`
- `export fn parseRoman(input: string): i32;`
- `export fn isValidRoman(input: string): bool;`
