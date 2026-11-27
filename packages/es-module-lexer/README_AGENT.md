# es-module-lexer — AI Agent Quick Ref

Zero-allocation ES module static/dynamic import and export lexer for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ImportKind,
    ImportSpecifier,
    ExportSpecifier,
    EsmParseResult,
    EsmlSpan,
    esmlIsWs,
    esmlIsIdentStart,
    esmlIsIdentPart,
} from "es-module-lexer";
```

## Core Types & Constants

```cp
enum ImportKind { Static, Dynamic, ImportMeta }
class ImportSpecifier { kind: ImportKind, spec_start: usize, spec_end: usize, stmt_start: usize, /* +1 fields */ } // methods: specifier(...)
class ExportSpecifier { name_start: usize, name_end: usize, local_start: usize, local_end: usize } // methods: name(...) +1
class EsmParseResult { import_count: usize, export_count: usize, facade: bool, has_module_syntax: bool }
class EsmlSpan { start: usize, end: usize, ok: bool }
```

## API Signatures

```cp
fn esmlIsWs(c: u8): bool;
fn esmlIsIdentStart(c: u8): bool;
fn esmlIsIdentPart(c: u8): bool;
fn esmlStrEq(a: string, b: string): bool;
fn esmlSkipWsAndComments(src: string, start: usize): usize;
fn esmlScanString(src: string, start: usize): EsmlSpan;
fn esmlSkipTemplate(src: string, start: usize): usize;
fn esmlScanIdent(src: string, start: usize): EsmlSpan;
fn esmlPrevNonWsChar(src: string, pos: usize): u8;
fn esmlPushImport(out: []ImportSpecifier, count: usize, kind: ImportKind, spec_start: usize, spec_end: usize, stmt_start: usize, stmt_end: usize): usize;
fn esmlPushExport(out: []ExportSpecifier, count: usize, name_start: usize, name_end: usize, local_start: usize, local_end: usize): usize;
fn parseEsm(src: string, imports_out: []ImportSpecifier, exports_out: []ExportSpecifier): EsmParseResult;
fn isFacade(src: string): bool;
```

## Minimal Usage

```cp
import { parseEsm } from "es-module-lexer";

pub fn main() {
    let result = parseEsm("example", /* imports_out: []ImportSpecifier */, /* exports_out: []ExportSpecifier */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"es-module-lexer"`; install via `cap add es-module-lexer`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
