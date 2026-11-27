[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# es-module-lexer

Zero-allocation ES module static/dynamic import and export lexer for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:es-module-lexer` ([https://github.com/guybedford/es-module-lexer](https://github.com/guybedford/es-module-lexer))

| Metadata | Value |
| --- | --- |
| Package | `es-module-lexer` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add es-module-lexer
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `ImportKind` | `Static, Dynamic, ImportMeta` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ImportSpecifier` | `class` | Fields: `kind: ImportKind, spec_start: usize, spec_end: usize, stmt_start: usize, stmt_end: usize` · Methods: `specifier` |
| `ExportSpecifier` | `class` | Fields: `name_start: usize, name_end: usize, local_start: usize, local_end: usize` · Methods: `name, localName` |
| `EsmParseResult` | `class` | Fields: `import_count: usize, export_count: usize, facade: bool, has_module_syntax: bool` |
| `EsmlSpan` | `class` | Fields: `start: usize, end: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `esmlIsWs` | `(c: u8): bool` | Invokes `esmlIsWs` and returns `bool`. |
| `esmlIsIdentStart` | `(c: u8): bool` | Invokes `esmlIsIdentStart` and returns `bool`. |
| `esmlIsIdentPart` | `(c: u8): bool` | Invokes `esmlIsIdentPart` and returns `bool`. |
| `esmlStrEq` | `(a: string, b: string): bool` | Invokes `esmlStrEq` and returns `bool`. |
| `esmlSkipWsAndComments` | `(src: string, start: usize): usize` | Invokes `esmlSkipWsAndComments` and returns `usize`. |
| `esmlScanString` | `(src: string, start: usize): EsmlSpan` | Invokes `esmlScanString` and returns `EsmlSpan`. |
| `esmlSkipTemplate` | `(src: string, start: usize): usize` | Invokes `esmlSkipTemplate` and returns `usize`. |
| `esmlScanIdent` | `(src: string, start: usize): EsmlSpan` | Invokes `esmlScanIdent` and returns `EsmlSpan`. |
| `esmlPrevNonWsChar` | `(src: string, pos: usize): u8` | Invokes `esmlPrevNonWsChar` and returns `u8`. |
| `esmlPushImport` | `(out: []ImportSpecifier, count: usize, kind: ImportKind, spec_start: usize, spec_end: usize, stmt_start: usize, stmt_end: usize): usize` | Invokes `esmlPushImport` and returns `usize`. |
| `esmlPushExport` | `(out: []ExportSpecifier, count: usize, name_start: usize, name_end: usize, local_start: usize, local_end: usize): usize` | Invokes `esmlPushExport` and returns `usize`. |
| `parseEsm` | `(src: string, imports_out: []ImportSpecifier, exports_out: []ExportSpecifier): EsmParseResult` | Parses input via `parseEsm` and returns `EsmParseResult`. |
| `isFacade` | `(src: string): bool` | Checks predicate `isFacade` and returns `bool`. |

## Minimal Example

```cp
import { parseEsm } from "es-module-lexer";

pub fn main() {
    let result = parseEsm("example", /* imports_out: []ImportSpecifier */, /* exports_out: []ExportSpecifier */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
