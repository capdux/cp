# detect-newline@1.0.0 (Agent Quickref)

> Zero-allocation dominant newline detector and cross-platform LF/CRLF normalizer for CP | License: MIT
> Import: `import { ... } from "detect-newline";` | Full types: `src/index.d.cp`

## Functions
- `export fn analyzeNewlines(input: string): NewlineStats;`
- `export fn detectNewline(input: string): string;`
- `export fn detectNewlineGraceful(input: string): string;`
- `export fn normalizeToLf(input: string, out: []u8): usize;`
- `export fn normalizeToCrlf(input: string, out: []u8): usize;`
