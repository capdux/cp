[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/pdfium

Google PDFium PDF rendering & text extraction engine (prebuilt dynamic library) for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/pdfium` |
| Version | `131.0.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/pdfium
```

```cp
import {
    PdfError,
    PageSize,
    PdfDoc,
    FPDF_InitLibrary,
    FPDF_DestroyLibrary,
    FPDF_LoadMemDocument,
    FPDF_CloseDocument,
    FPDF_GetPageCount,
} from "@cap/pdfium";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `PdfError` | `OpenFailed, InvalidPage, OutOfMemory` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PageSize` | `class` | Fields: `width: f64, height: f64` |
| `PdfDoc` | `class` | Fields: `doc_ptr: *auto!?` · Methods: `loadFromMemory, pageCount, pageSize, pageCharCount, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `FPDF_InitLibrary` | `()` | Executes `FPDF_InitLibrary` in-place. |
| `FPDF_DestroyLibrary` | `()` | Executes `FPDF_DestroyLibrary` in-place. |
| `FPDF_LoadMemDocument` | `(data_buf: *auto!?, size: i32, password: [*:0]&u8!?): *auto!?` | Invokes `FPDF_LoadMemDocument` and returns `*auto!?`. |
| `FPDF_CloseDocument` | `(document: *auto!?)` | Executes `FPDF_CloseDocument` in-place. |
| `FPDF_GetPageCount` | `(document: *auto!?): i32` | Invokes `FPDF_GetPageCount` and returns `i32`. |
| `FPDF_LoadPage` | `(document: *auto!?, page_index: i32): *auto!?` | Invokes `FPDF_LoadPage` and returns `*auto!?`. |
| `FPDF_ClosePage` | `(page: *auto!?)` | Executes `FPDF_ClosePage` in-place. |
| `FPDF_GetPageWidth` | `(page: *auto!?): f64` | Invokes `FPDF_GetPageWidth` and returns `f64`. |
| `FPDF_GetPageHeight` | `(page: *auto!?): f64` | Invokes `FPDF_GetPageHeight` and returns `f64`. |
| `FPDFText_LoadPage` | `(page: *auto!?): *auto!?` | Invokes `FPDFText_LoadPage` and returns `*auto!?`. |
| `FPDFText_ClosePage` | `(text_page: *auto!?)` | Executes `FPDFText_ClosePage` in-place. |
| `FPDFText_CountChars` | `(text_page: *auto!?): i32` | Invokes `FPDFText_CountChars` and returns `i32`. |
| `FPDFText_GetText` | `(text_page: *auto!?, start_index: i32, count: i32, result: [*]u16!): i32` | Invokes `FPDFText_GetText` and returns `i32`. |
| `initLibrary` | `()` | Constructs/initializes an instance via `initLibrary`. |
| `destroyLibrary` | `()` | Releases native resources allocated by this module. |

## Minimal Example

```cp
import { FPDF_InitLibrary } from "@cap/pdfium";

pub fn main() {
    FPDF_InitLibrary();
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
