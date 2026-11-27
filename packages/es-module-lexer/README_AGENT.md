# es-module-lexer@1.0.0 (Agent Quickref)

> Zero-allocation ES module static/dynamic import and export lexer for CP | License: MIT
> Import: `import { ... } from "es-module-lexer";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum ImportKind`

## Functions
- `export fn esmlIsWs(c: u8): bool;`
- `export fn esmlIsIdentStart(c: u8): bool;`
- `export fn esmlIsIdentPart(c: u8): bool;`
- `export fn esmlStrEq(a: string, b: string): bool;`
- `export fn esmlSkipWsAndComments(src: string, start: usize): usize;`
- `export fn esmlScanString(src: string, start: usize): EsmlSpan;`
- `export fn esmlSkipTemplate(src: string, start: usize): usize;`
- `export fn esmlScanIdent(src: string, start: usize): EsmlSpan;`
- `export fn esmlPrevNonWsChar(src: string, pos: usize): u8;`
- `export fn esmlPushImport(out: []ImportSpecifier, count: usize, kind: ImportKind, spec_start: usize, spec_end: usize, stmt_start: usize, stmt_end: usize): usize;`
- `export fn esmlPushExport(out: []ExportSpecifier, count: usize, name_start: usize, name_end: usize, local_start: usize, local_end: usize): usize;`
- `export fn parseEsm(src: string, imports_out: []ImportSpecifier, exports_out: []ExportSpecifier): EsmParseResult;`
- `export fn isFacade(src: string): bool;`
