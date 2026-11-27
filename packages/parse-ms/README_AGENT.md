# parse-ms@1.0.0 (Agent Quickref)

> CP package parse-ms | License: MIT
> Import: `import { ... } from "parse-ms";` | Full types: `src/index.d.cp`

## Functions
- `export fn signedPart(mag: u64, is_neg: bool): i64;`
- `export fn splitWholeMs(whole_ms: u64, us: u64, ns: u64, is_neg: bool): ParsedMs;`
- `export fn parseMs(ms: i64): ParsedMs;`
- `export fn parseMsF64(ms: f64): ParsedMs;`
- `export fn toMilliseconds(p: ParsedMs): i64;`
