// Type definitions for CP

import {  } from "std";
export fn FPDF_InitLibrary();
export fn FPDF_DestroyLibrary();
export fn FPDF_LoadMemDocument(data_buf: *auto!?, size: i32, password: [*:0]&u8!?): *auto!?;
export fn FPDF_CloseDocument(document: *auto!?);
export fn FPDF_GetPageCount(document: *auto!?): i32;
export fn FPDF_LoadPage(document: *auto!?, page_index: i32): *auto!?;
export fn FPDF_ClosePage(page: *auto!?);
export fn FPDF_GetPageWidth(page: *auto!?): f64;
export fn FPDF_GetPageHeight(page: *auto!?): f64;
export fn FPDFText_LoadPage(page: *auto!?): *auto!?;
export fn FPDFText_ClosePage(text_page: *auto!?);
export fn FPDFText_CountChars(text_page: *auto!?): i32;
export fn FPDFText_GetText(text_page: *auto!?, start_index: i32, count: i32, result: [*]u16!): i32;
export error PdfError { OpenFailed, InvalidPage, OutOfMemory }

export fn initLibrary();
export fn destroyLibrary();
export class PageSize {
    width: f64,
    height: f64,
}

export class PdfDoc {
    doc_ptr: *auto!?,
    fn loadFromMemory(bytes: &[]u8): !PdfDoc;
    fn pageCount(): i32;
    fn pageSize(page_index: i32): !PageSize;
    fn pageCharCount(page_index: i32): !i32;
    fn __cp_deinit(_: &mut PdfDoc);
}

