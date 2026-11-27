# phonenumber@1.0.0 (Agent Quickref)

> Zero-heap RFC 3966 tel: URI parser, E.164 international phone number normalizer/validator, and URI formatter. | License: MIT
> Import: `import { ... } from "phonenumber";` | Full types: `src/index.d.cp`

## Functions
- `export fn isDigit(c: u8): bool;`
- `export fn isVisualSep(c: u8): bool;`
- `export fn isWs(c: u8): bool;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn isValidE164(number: string): bool;`
- `export fn normalizeE164(input: string, out: []u8): usize;`
- `export fn parseTelUri(uri: string): TelUri;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn formatTelUri(e164_number: string, ext: string, out: []u8): usize;`
