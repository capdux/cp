# @cap/pdfium — AI Agent Quick Ref

Google PDFium PDF rendering & text extraction engine (prebuilt dynamic library) for CP (`v131.0.0`, `Apache-2.0`)

## Import

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

## Core Types & Constants

```cp
error PdfError { OpenFailed, InvalidPage, OutOfMemory }
class PageSize { width: f64, height: f64 }
class PdfDoc { doc_ptr: *auto!? } // methods: loadFromMemory(...) +4
```

## API Signatures

```cp
fn FPDF_InitLibrary();
fn FPDF_DestroyLibrary();
fn FPDF_LoadMemDocument(data_buf: *auto!?, size: i32, password: [*:0]&u8!?): *auto!?;
fn FPDF_CloseDocument(document: *auto!?);
fn FPDF_GetPageCount(document: *auto!?): i32;
fn FPDF_LoadPage(document: *auto!?, page_index: i32): *auto!?;
fn FPDF_ClosePage(page: *auto!?);
fn FPDF_GetPageWidth(page: *auto!?): f64;
fn FPDF_GetPageHeight(page: *auto!?): f64;
fn FPDFText_LoadPage(page: *auto!?): *auto!?;
fn FPDFText_ClosePage(text_page: *auto!?);
fn FPDFText_CountChars(text_page: *auto!?): i32;
fn FPDFText_GetText(text_page: *auto!?, start_index: i32, count: i32, result: [*]u16!): i32;
fn initLibrary();
fn destroyLibrary();
```

## Minimal Usage

```cp
import { FPDF_InitLibrary } from "@cap/pdfium";

pub fn main() {
    FPDF_InitLibrary();
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/pdfium"`; install via `cap add @cap/pdfium`.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
