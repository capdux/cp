# parse-json — AI Agent Quick Ref

Zero-allocation RFC 8259 JSON diagnostic validator with line/column locations and codeFrame rendering (npm parse-json v8 parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    JsonDiagKind,
    JsonDiagResult,
    PjsParser,
    PjsLoc,
    PjsLineSpan,
    PjsWriter,
    pjsIsWs,
    pjsIsDigit,
} from "parse-json";
```

## Core Types & Constants

```cp
enum JsonDiagKind { Ok, EmptyInput, UnexpectedToken, UnexpectedEnd, TrailingComma, SingleQuotedString, ... }
class JsonDiagResult { ok: bool, kind: JsonDiagKind, offset: usize, line: usize, /* +2 fields */ }
class PjsParser { input: string, pos: usize, has_err: bool, err_kind: JsonDiagKind, /* +1 fields */ }
class PjsLoc { line: usize, column: usize }
class PjsLineSpan { found: bool, start: usize, end: usize }
class PjsWriter { buf: []u8, pos: usize }
```

## API Signatures

```cp
fn pjsIsWs(ch: u8): bool;
fn pjsIsDigit(ch: u8): bool;
fn pjsIsHex(ch: u8): bool;
fn pjsIsIdentStart(ch: u8): bool;
fn pjsSkipWs(p: &mut PjsParser);
fn pjsSetErr(p: &mut PjsParser, kind: JsonDiagKind, offset: usize);
fn pjsLineCol(input: string, offset: usize): PjsLoc;
fn pjsParseString(p: &mut PjsParser);
fn pjsParseNumber(p: &mut PjsParser);
fn pjsExpectKeyword(p: &mut PjsParser, kw: string);
fn pjsParseValue(p: &mut PjsParser, depth: usize): u8;
fn validateJsonWithDiag(input: string): JsonDiagResult;
fn pjsDiagMessage(kind: JsonDiagKind): string;
fn pjsWriteByte(w: &mut PjsWriter, b: u8);
fn pjsWriteStr(w: &mut PjsWriter, s: string);
fn pjsNumWidth(n: usize): usize;
fn pjsWriteUsize(w: &mut PjsWriter, n: usize);
fn pjsWriteSpaces(w: &mut PjsWriter, count: usize);
fn pjsFindLineSpan(input: string, target_line: usize): PjsLineSpan;
fn pjsWriteFrameLine(w: &mut PjsWriter, input: string, line_no: usize, span: PjsLineSpan, gutter_w: usize, is_err: bool);
fn formatJsonError(input: string, diag: JsonDiagResult, filename: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { validateJsonWithDiag } from "parse-json";

pub fn main() {
    let result = validateJsonWithDiag("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"parse-json"`; install via `cap add parse-json`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
