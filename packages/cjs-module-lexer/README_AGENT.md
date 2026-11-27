# cjs-module-lexer — AI Agent Quick Ref

Zero-allocation CommonJS static named export and re-export lexer (npm cjs-module-lexer parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CjsParseResult,
    CjslLexer,
    CjslStrRes,
    cjslIsWs,
    cjslIsIdentStart,
    cjslIsIdentChar,
    cjslStrEq,
    cjslAddExport,
} from "cjs-module-lexer";
```

## Core Types & Constants

```cp
class CjsParseResult { exports: [32]string, export_count: usize, reexports: [16]string, reexport_count: usize }
class CjslLexer { src: string, pos: usize }
class CjslStrRes { ok: bool, val: string }
```

## API Signatures

```cp
fn cjslIsWs(ch: u8): bool;
fn cjslIsIdentStart(ch: u8): bool;
fn cjslIsIdentChar(ch: u8): bool;
fn cjslStrEq(a: string, b: string): bool;
fn cjslAddExport(res: &mut CjsParseResult, name: string);
fn cjslAddReexport(res: &mut CjsParseResult, spec: string);
fn cjslSkipString(lex: &mut CjslLexer);
fn cjslSkipWsAndComments(lex: &mut CjslLexer);
fn cjslReadIdent(lex: &mut CjslLexer): string;
fn cjslReadStringLit(lex: &mut CjslLexer): CjslStrRes;
fn cjslMatchRequireCall(lex: &mut CjslLexer): CjslStrRes;
fn cjslSkipPropValue(lex: &mut CjslLexer);
fn cjslParseObjectExports(lex: &mut CjslLexer, res: &mut CjsParseResult);
fn cjslParseExportsMemberAssign(lex: &mut CjslLexer, res: &mut CjsParseResult);
fn cjslParseDefineProperty(lex: &mut CjslLexer, res: &mut CjsParseResult);
fn cjslParseExportStar(lex: &mut CjslLexer, res: &mut CjsParseResult);
fn parseCjs(src: string): CjsParseResult;
```

## Minimal Usage

```cp
import { parseCjs } from "cjs-module-lexer";

pub fn main() {
    let result = parseCjs("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cjs-module-lexer"`; install via `cap add cjs-module-lexer`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
