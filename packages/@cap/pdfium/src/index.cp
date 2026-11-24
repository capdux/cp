// @cap/pdfium — Google PDFium PDF parsing, rendering & text extraction for CP
// License: Apache-2.0 (Upstream: Google PDFium / bblanchon pdfium-binaries)

import std from "std";

pub @c {
    fn FPDF_InitLibrary();
    fn FPDF_DestroyLibrary();
    fn FPDF_LoadMemDocument(data_buf: *!?, size: i32, password: [*:0]const u8?): *!?;
    fn FPDF_CloseDocument(document: *!?);
    fn FPDF_GetPageCount(document: *!?): i32;
    fn FPDF_LoadPage(document: *!?, page_index: i32): *!?;
    fn FPDF_ClosePage(page: *!?);
    fn FPDF_GetPageWidth(page: *!?): f64;
    fn FPDF_GetPageHeight(page: *!?): f64;
    fn FPDFText_LoadPage(page: *!?): *!?;
    fn FPDFText_ClosePage(text_page: *!?);
    fn FPDFText_CountChars(text_page: *!?): i32;
    fn FPDFText_GetText(text_page: *!?, start_index: i32, count: i32, result: [*]u16!): i32;
}

pub err PdfError {
    OpenFailed,
    InvalidPage,
    OutOfMemory,
}

/// Initializes the global PDFium library (call once before loading documents).
pub fn initLibrary() {
    unsafe { FPDF_InitLibrary(); }
}

/// Releases global PDFium resources.
pub fn destroyLibrary() {
    unsafe { FPDF_DestroyLibrary(); }
}

pub struct PageSize {
    pub width: f64,
    pub height: f64,
}

/// Owned RAII PDF document handle.
pub struct PdfDoc {
    doc_ptr: *!? = null,

    pub static fn loadFromMemory(bytes: []const u8): PdfDoc! {
        let p: *!? = null;
        unsafe {
            p = FPDF_LoadMemDocument(@constCast(@ptrCast(bytes.ptr)), @intCast(bytes.len), null);
        }
        if (p == null) return PdfError.OpenFailed;
        return PdfDoc{ .doc_ptr = p };
    }

    pub fn pageCount(): i32 {
        if (_.doc_ptr == null) return 0;
        unsafe { return FPDF_GetPageCount(_.doc_ptr); }
    }

    pub fn pageSize(page_index: i32): PageSize! {
        if (_.doc_ptr == null) return PdfError.InvalidPage;
        let page: *!? = null;
        unsafe { page = FPDF_LoadPage(_.doc_ptr, page_index); }
        if (page == null) return PdfError.InvalidPage;
        defer { unsafe { FPDF_ClosePage(page); } }
        let w: f64 = 0.0;
        let h: f64 = 0.0;
        unsafe {
            w = FPDF_GetPageWidth(page);
            h = FPDF_GetPageHeight(page);
        }
        return PageSize{ .width = w, .height = h };
    }

    pub fn pageCharCount(page_index: i32): i32! {
        if (_.doc_ptr == null) return PdfError.InvalidPage;
        let page: *!? = null;
        unsafe { page = FPDF_LoadPage(_.doc_ptr, page_index); }
        if (page == null) return PdfError.InvalidPage;
        defer { unsafe { FPDF_ClosePage(page); } }
        let tp: *!? = null;
        unsafe { tp = FPDFText_LoadPage(page); }
        if (tp == null) return 0;
        defer { unsafe { FPDFText_ClosePage(tp); } }
        unsafe { return FPDFText_CountChars(tp); }
    }

    ~init() {
        if (d := _.doc_ptr) {
            _.doc_ptr = null;
            unsafe { FPDF_CloseDocument(d); }
        }
    }
}
