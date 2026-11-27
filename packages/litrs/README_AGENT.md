# litrs@1.0.0 (Agent Quickref)

> Zero-allocation Rust literal lexer, validator, and string unescaper (Rust litrs parity) for CP | License: MIT OR Apache-2.0
> Import: `import { ... } from "litrs";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum LiteralKind`

## Functions
- `export fn ltrInvalid(src: string): ParsedLiteral;`
- `export fn ltrStrEq(a: string, b: string): bool;`
- `export fn ltrIsIdentStart(c: u8): bool;`
- `export fn ltrIsIdentCont(c: u8): bool;`
- `export fn ltrIsValidSuffix(s: string): bool;`
- `export fn ltrHexVal(c: u8): i32;`
- `export fn ltrDigitVal(c: u8, radix: u64): i32;`
- `export fn ltrParseRawString(src: string, start: usize, kind: LiteralKind): ParsedLiteral;`
- `export fn ltrParseQuotedString(src: string, quote_pos: usize, kind: LiteralKind): ParsedLiteral;`
- `export fn ltrParseCharOrByte(src: string, quote_pos: usize, is_byte: bool): ParsedLiteral;`
- `export fn ltrParseNumber(src: string): ParsedLiteral;`
- `export fn parseLiteral(src: string): ParsedLiteral;`
- `export fn ltrEncodeUtf8(cp: u32, out: []u8, pos: usize): usize?;`
- `export fn unescapeString(lit_raw: string, out: []u8): i64;`
