# utf8parse@1.0.0 (Agent Quickref)

> Zero-allocation RFC 3629 strict UTF-8 validator, WHATWG lossy repair (U+FFFD), ASCII fast-path, and UTF-16 code-unit length calculator for CP | License: MIT
> Import: `import { ... } from "utf8parse";` | Full types: `src/index.d.cp`

## Functions
- `export fn isCont(b: u8): bool;`
- `export fn checkSeqAt(data: string, pos: usize): StepResult;`
- `export fn isAscii(data: string): bool;`
- `export fn validateUtf8(data: string): Utf8CheckResult;`
- `export fn isValidUtf8(data: string): bool;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn toLossyUtf8(data: string, out: []u8): string;`
- `export fn utf16Len(data: string): usize;`
