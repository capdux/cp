# strip-final-newline@1.0.0 (Agent Quickref)

> Zero-copy final LF/CRLF newline stripper and normalizer matching npm strip-final-newline (250M+/wk) in pure CP | License: MIT
> Import: `import { ... } from "strip-final-newline";` | Full types: `src/index.d.cp`

## Functions
- `export fn stripFinalNewline(input: string): string;`
- `export fn stripAllTrailingNewlines(input: string): string;`
- `export fn hasFinalNewline(input: string): bool;`
- `export fn hasFinalCrlf(input: string): bool;`
- `export fn ensureFinalNewline(input: string, use_crlf: bool, out: []u8): usize;`
