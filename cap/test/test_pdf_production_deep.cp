// Source: cp/cap/test/test_pdf_production_deep.cp
// Deep Production Verification Suite for cap:pdf & cap.pdf
// Covers:
//   1. Single-page & multi-page PDF 1.4 generation (A4/Letter/Custom, 4 Base14 fonts, paren/backslash escaping, vector lines & rects)
//   2. Exact 10-digit xref byte-offset verification against actual object positions
//   3. PdfInspector.inspect version, page_count, xref/eof flags & /Title /Author /Creator metadata extraction
//   4. Malformed non-PDF & truncated payload boundary guards
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.pdf`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    PdfError,
    PdfFont,
    PageSize,
    PdfPage,
    PdfDocument,
    PdfInfo,
    PdfInspector,
} from "cap:doc";
import cap from "cap";

fn testSingleAndMultiPageGeneration()! {
    const alloc = std.heap.page_allocator;

    // 1. Single-page A4 document with no Info metadata
    let single_doc = PdfDocument.init(alloc);
    defer single_doc.deinit();

    const p0 = single_doc.addPage(PageSize.a4()) catch unreachable;
    p0.drawText(72.0, 750.0, PdfFont.Helvetica, 14.0, "Single Page A4") catch unreachable;

    const single_bytes = single_doc.build(alloc) catch unreachable;
    defer alloc.free(single_bytes);

    expect_eq(std.mem.startsWith(u8, single_bytes, "%PDF-1.4\n"), true);
    expect_eq(std.mem.indexOf(u8, single_bytes, "/MediaBox [0 0 595.28 841.89]") != null, true);
    expect_eq(std.mem.indexOf(u8, single_bytes, "/Count 1") != null, true);
    expect_eq(PdfInspector.isValidPdf(single_bytes), true);

    const single_info = PdfInspector.inspect(single_bytes) catch unreachable;
    expect_eq(single_info.page_count, 1 as usize);
    expect_eq(single_info.title == null, true);
    expect_eq(single_info.author == null, true);
    expect_eq(single_info.creator == null, true);

    // 2. Multi-page document (A4 + Letter + Custom) with all 4 Base14 fonts, escaping & vector ops
    let doc = PdfDocument.init(alloc);
    defer doc.deinit();

    const page1 = doc.addPage(PageSize.a4()) catch unreachable;
    page1.setFillRgb(0.15, 0.35, 0.75) catch unreachable;
    page1.fillRect(50.0, 740.0, 495.0, 44.0) catch unreachable;
    page1.setStrokeRgb(0.8, 0.1, 0.2) catch unreachable;
    page1.setLineWidth(2.5) catch unreachable;
    page1.drawLine(50.0, 730.0, 545.0, 730.0) catch unreachable;
    page1.drawRect(50.0, 620.0, 200.0, 90.0) catch unreachable;
    page1.drawText(60.0, 755.0, PdfFont.Helvetica, 12.0, "Normal (parens) and \\backslash\\") catch unreachable;
    page1.drawText(60.0, 690.0, PdfFont.HelveticaBold, 16.0, "Bold Title") catch unreachable;

    const page2 = doc.addPage(PageSize.letter()) catch unreachable;
    page2.drawText(72.0, 700.0, PdfFont.Courier, 11.0, "const x = fn(a, b);") catch unreachable;

    const page3 = doc.addPage(PageSize.custom(400.0, 300.0)) catch unreachable;
    page3.drawText(36.0, 200.0, PdfFont.TimesRoman, 18.0, "Times Roman Footer") catch unreachable;

    expect_eq(doc.pageCount(), 3 as usize);

    const pdf_bytes = doc.build(alloc) catch unreachable;
    defer alloc.free(pdf_bytes);

    expect_eq(std.mem.startsWith(u8, pdf_bytes, "%PDF-1.4\n"), true);
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "/BaseFont /Helvetica ") != null, true);
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "/BaseFont /Helvetica-Bold ") != null, true);
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "/BaseFont /Courier ") != null, true);
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "/BaseFont /Times-Roman ") != null, true);

    expect_eq(std.mem.indexOf(u8, pdf_bytes, "/MediaBox [0 0 595.28 841.89]") != null, true);
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "/MediaBox [0 0 612 792]") != null, true);
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "/MediaBox [0 0 400 300]") != null, true);
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "/Count 3") != null, true);

    // Text escaping checks
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "(Normal \\(parens\\) and \\\\backslash\\\\) Tj") != null, true);

    // Vector operators checks
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "0.15 0.35 0.75 rg\n") != null, true);
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "0.8 0.1 0.2 RG\n") != null, true);
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "2.5 w\n") != null, true);
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "50 730 m\n545 730 l\nS\n") != null, true);
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "50 620 200 90 re\nS\n") != null, true);
    expect_eq(std.mem.indexOf(u8, pdf_bytes, "50 740 495 44 re\nf\n") != null, true);
}

fn testExactXrefByteOffsets()! {
    const alloc = std.heap.page_allocator;

    let doc = PdfDocument.init(alloc);
    defer doc.deinit();

    doc.setTitle("XRef Precision Check") catch unreachable;
    doc.setAuthor("Subagent 46") catch unreachable;

    const p1 = doc.addPage(PageSize.a4()) catch unreachable;
    p1.drawText(72.0, 720.0, PdfFont.Helvetica, 12.0, "Page 1 (alpha)") catch unreachable;
    const p2 = doc.addPage(PageSize.letter()) catch unreachable;
    p2.fillRect(10.0, 20.0, 100.0, 50.0) catch unreachable;
    p2.drawText(72.0, 680.0, PdfFont.Courier, 10.0, "Page 2 /Type /Page trap inside stream") catch unreachable;

    const pdf = doc.build(alloc) catch unreachable;
    defer alloc.free(pdf);

    // 1. Parse startxref offset and verify it points directly to "xref\n"
    const sxref_pos = std.mem.lastIndexOf(u8, pdf, "startxref\n").?;
    let num_start = sxref_pos + 10;
    let num_end = num_start;
    while (num_end < pdf.len && pdf[num_end] >= '0' && pdf[num_end] <= '9') {
        num_end += 1;
    }
    const xref_off = std.fmt.parseInt(usize, pdf[num_start..num_end], 10) catch unreachable;
    expect_eq(std.mem.startsWith(u8, pdf[xref_off..], "xref\n0 "), true);

    // 2. Parse total entry count from "0 <count>\n"
    const count_start = xref_off + 7;
    const count_nl = std.mem.indexOfPos(u8, pdf, count_start, "\n").?;
    const total_entries = std.fmt.parseInt(usize, pdf[count_start..count_nl], 10) catch unreachable;
    // 6 fixed + 2*2 pages + 1 Info = 11 objects => 12 entries (0..11)
    expect_eq(total_entries, 12 as usize);

    // 3. Entry 0 must be exact 20-byte free entry
    let cursor = count_nl + 1;
    expect_eq(std.mem.eql(u8, pdf[cursor..cursor + 20], "0000000000 65535 f \n"), true);
    cursor += 20;

    // 4. Every object 1..11 must have exact 10-digit offset pointing to "<id> 0 obj\n"
    let obj_id: usize = 1;
    while (obj_id < total_entries) : (obj_id += 1) {
        const entry = pdf[cursor..cursor + 20];
        expect_eq(std.mem.eql(u8, entry[10..20], " 00000 n \n"), true);

        const off = std.fmt.parseInt(usize, entry[0..10], 10) catch unreachable;
        expect_eq(off < xref_off, true);

        let prefix_buf: [32]u8 = undefined;
        const expected_prefix = std.fmt.bufPrint(&prefix_buf, "{d} 0 obj\n", .{obj_id}) catch unreachable;
        expect_eq(std.mem.startsWith(u8, pdf[off..], expected_prefix), true);

        cursor += 20;
    }
}

fn testInspectorMetadataAndPageCount()! {
    const alloc = std.heap.page_allocator;

    let doc = PdfDocument.init(alloc);
    defer doc.deinit();

    doc.setTitle("CP Language Spec") catch unreachable;
    doc.setAuthor("Antigravity Team") catch unreachable;
    doc.setCreator("cap:pdf 1.4 Engine") catch unreachable;

    let i: usize = 0;
    while (i < 4) : (i += 1) {
        const pg = doc.addPage(PageSize.a4()) catch unreachable;
        // Include "/Type /Page" inside a text stream to ensure inspector ignores stream bodies
        pg.drawText(50.0, 700.0, PdfFont.Helvetica, 12.0, "Stream text /Type /Page should not inflate count") catch unreachable;
    }

    const pdf = doc.build(alloc) catch unreachable;
    defer alloc.free(pdf);

    expect_eq(PdfInspector.isValidPdf(pdf), true);
    expect_eq(std.mem.eql(u8, PdfInspector.extractVersion(pdf).?, "1.4"), true);

    const info = PdfInspector.inspect(pdf) catch unreachable;
    expect_eq(std.mem.eql(u8, info.version, "1.4"), true);
    expect_eq(info.page_count, 4 as usize);
    expect_eq(info.has_xref, true);
    expect_eq(info.has_eof, true);
    expect_eq(std.mem.eql(u8, info.title.?, "CP Language Spec"), true);
    expect_eq(std.mem.eql(u8, info.author.?, "Antigravity Team"), true);
    expect_eq(std.mem.eql(u8, info.creator.?, "cap:pdf 1.4 Engine"), true);
}

fn testMalformedAndTruncatedGuards()! {
    const alloc = std.heap.page_allocator;

    // 1. Non-PDF payloads
    expect_eq(PdfInspector.isValidPdf(""), false);
    expect_eq(PdfInspector.isValidPdf("NOT A PDF DOCUMENT"), false);
    expect_eq(PdfInspector.extractVersion("<html></html>") == null, true);
    expect_eq(PdfInspector.extractVersion("%PDF-1.X\n") == null, true);

    let invalid_hit = false;
    * = PdfInspector.inspect("PNG\x89\x50\x4E\x47") catch |e| {
        if (e == PdfError.InvalidPdf) invalid_hit = true;
    };
    expect_eq(invalid_hit, true);

    // 2. Build a real PDF then test truncated prefixes
    let doc = PdfDocument.init(alloc);
    defer doc.deinit();
    const p = doc.addPage(PageSize.a4()) catch unreachable;
    p.drawText(72.0, 720.0, PdfFont.Helvetica, 12.0, "Check truncation") catch unreachable;
    const full = doc.build(alloc) catch unreachable;
    defer alloc.free(full);

    // Truncate inside %%EOF (keep startxref + newline intact, cut "EOF\n")
    const no_eof = full[0 .. full.len - 4];
    expect_eq(PdfInspector.isValidPdf(no_eof), false);
    let eof_hit = false;
    * = PdfInspector.inspect(no_eof) catch |e| {
        if (e == PdfError.MissingEof) eof_hit = true;
    };
    expect_eq(eof_hit, true);

    // Truncate inside startxref offset (corrupts xref pointer)
    const bad_sxref_cut = full[0 .. full.len - 8];
    expect_eq(PdfInspector.isValidPdf(bad_sxref_cut), false);
    let bad_sxref_hit = false;
    * = PdfInspector.inspect(bad_sxref_cut) catch |e| {
        if (e == PdfError.MissingXRef) bad_sxref_hit = true;
    };
    expect_eq(bad_sxref_hit, true);

    // Truncate before startxref (keep header + objects only)
    const xref_pos = std.mem.lastIndexOf(u8, full, "xref\n").?;
    const no_xref = full[0..xref_pos];
    expect_eq(PdfInspector.isValidPdf(no_xref), false);
    let xref_hit = false;
    * = PdfInspector.inspect(no_xref) catch |e| {
        if (e == PdfError.MissingXRef) xref_hit = true;
    };
    expect_eq(xref_hit, true);

    // Header only with bogus startxref pointing out of bounds
    const bogus_startxref = "%PDF-1.4\nxref\n0 1\n0000000000 65535 f \ntrailer\n<< /Size 1 >>\nstartxref\n999999\n%%EOF\n";
    expect_eq(PdfInspector.isValidPdf(bogus_startxref), false);
    let bogus_xref_hit = false;
    * = PdfInspector.inspect(bogus_startxref) catch |e| {
        if (e == PdfError.MissingXRef) bogus_xref_hit = true;
    };
    expect_eq(bogus_xref_hit, true);

    // Invalid page size & font size guards
    let bad_size_hit = false;
    * = doc.addPage(PageSize.custom(0.0, 100.0)) catch |e| {
        if (e == PdfError.InvalidArgument) bad_size_hit = true;
    };
    expect_eq(bad_size_hit, true);

    let bad_font_hit = false;
    p.drawText(10.0, 10.0, PdfFont.Helvetica, 0.0, "bad") catch |e| {
        if (e == PdfError.InvalidArgument) bad_font_hit = true;
    };
    expect_eq(bad_font_hit, true);
}

fn testCapPdfDynamicMount()! {
    const alloc = std.heap.page_allocator;

    let doc = cap.doc.PdfDocument.init(alloc);
    defer doc.deinit();

    doc.setTitle("Dynamic Mount PDF") catch unreachable;
    doc.setAuthor("cap.doc") catch unreachable;
    doc.setCreator("CP Runtime") catch unreachable;

    const p1 = doc.addPage(cap.doc.PageSize.a4()) catch unreachable;
    p1.setStrokeRgb(0.2, 0.4, 0.8) catch unreachable;
    p1.drawRect(40.0, 700.0, 300.0, 60.0) catch unreachable;
    p1.drawText(52.0, 725.0, cap.doc.PdfFont.HelveticaBold, 14.0, "Mounted via cap.doc") catch unreachable;

    const p2 = doc.addPage(cap.doc.PageSize.letter()) catch unreachable;
    p2.drawText(52.0, 700.0, cap.doc.PdfFont.TimesRoman, 12.0, "Second Page") catch unreachable;

    const out = doc.build(alloc) catch unreachable;
    defer alloc.free(out);

    expect_eq(cap.doc.PdfInspector.isValidPdf(out), true);
    expect_eq(std.mem.eql(u8, cap.doc.PdfInspector.extractVersion(out).?, "1.4"), true);

    const info = cap.doc.PdfInspector.inspect(out) catch unreachable;
    expect_eq(info.page_count, 2 as usize);
    expect_eq(info.has_xref, true);
    expect_eq(info.has_eof, true);
    expect_eq(std.mem.eql(u8, info.title.?, "Dynamic Mount PDF"), true);
    expect_eq(std.mem.eql(u8, info.author.?, "cap.doc"), true);
    expect_eq(std.mem.eql(u8, info.creator.?, "CP Runtime"), true);
}

fn main() {
    test("1. Single/Multi-Page PDF 1.4 Generation, Base14 Fonts, Escaping & Vector Ops", () => {
        testSingleAndMultiPageGeneration()?;
    });

    test("2. Exact 10-Digit XRef Byte-Offset Verification", () => {
        testExactXrefByteOffsets()?;
    });

    test("3. PdfInspector Version, Page Count & Metadata Extraction", () => {
        testInspectorMetadataAndPageCount()?;
    });

    test("4. Malformed Non-PDF & Truncated Payload Boundary Guards", () => {
        testMalformedAndTruncatedGuards()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.pdf`", () => {
        testCapPdfDynamicMount()?;
    });
}
