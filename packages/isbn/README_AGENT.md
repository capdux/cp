# isbn@1.0.0 (Agent Quickref)

> Zero-allocation ISO 2108 ISBN-10, ISBN-13, ISSN, and EAN-13 validator and converter for CP. | License: MIT
> Import: `import { ... } from "isbn";` | Full types: `src/index.d.cp`

## Functions
- `export fn stripHyphens(input: string, out: []u8): usize;`
- `export fn computeIsbn13CheckDigit(prefix12: string): u8;`
- `export fn isValidIsbn10(input: string): bool;`
- `export fn isValidEan13(input: string): bool;`
- `export fn isValidIsbn13(input: string): bool;`
- `export fn isValidIssn(input: string): bool;`
- `export fn isValidIsbn(input: string): bool;`
- `export fn isbn10To13(isbn10: string, out: []u8): usize;`
- `export fn isbn13To10(isbn13: string, out: []u8): usize;`
