# parse-json@1.0.0 (Agent Quickref)

> Zero-allocation RFC 8259 JSON diagnostic validator with line/column locations and codeFrame rendering (npm parse-json v8 parity) for CP | License: MIT
> Import: `import { ... } from "parse-json";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum JsonDiagKind`

## Functions
- `export fn pjsIsWs(ch: u8): bool;`
- `export fn pjsIsDigit(ch: u8): bool;`
- `export fn pjsIsHex(ch: u8): bool;`
- `export fn pjsIsIdentStart(ch: u8): bool;`
- `export fn pjsSkipWs(p: &mut PjsParser);`
- `export fn pjsSetErr(p: &mut PjsParser, kind: JsonDiagKind, offset: usize);`
- `export fn pjsLineCol(input: string, offset: usize): PjsLoc;`
- `export fn pjsParseString(p: &mut PjsParser);`
- `export fn pjsParseNumber(p: &mut PjsParser);`
- `export fn pjsExpectKeyword(p: &mut PjsParser, kw: string);`
- `export fn pjsParseValue(p: &mut PjsParser, depth: usize): u8;`
- `export fn validateJsonWithDiag(input: string): JsonDiagResult;`
- `export fn pjsDiagMessage(kind: JsonDiagKind): string;`
- `export fn pjsWriteByte(w: &mut PjsWriter, b: u8);`
- `export fn pjsWriteStr(w: &mut PjsWriter, s: string);`
- `export fn pjsNumWidth(n: usize): usize;`
- `export fn pjsWriteUsize(w: &mut PjsWriter, n: usize);`
- `export fn pjsWriteSpaces(w: &mut PjsWriter, count: usize);`
- `export fn pjsFindLineSpan(input: string, target_line: usize): PjsLineSpan;`
- `export fn pjsWriteFrameLine(w: &mut PjsWriter, input: string, line_no: usize, span: PjsLineSpan, gutter_w: usize, is_err: bool);`
- `export fn formatJsonError(input: string, diag: JsonDiagResult, filename: string, out: []u8): usize;`
