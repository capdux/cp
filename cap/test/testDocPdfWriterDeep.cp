// Origin: Pure-CP Stdlib Test Suite (lib/doc/pdfWriter.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Heap PDF 1.4-1.7 Layout, Table, Vector, Image XObject,
// Outlines, UTF-16BE Metadata & FlateDecode Serializer (Full Bidirectional Closed Loop with pdfInspector.cp):
//   1. Multi-page auto-wrapped paragraphs (left/center/right), header/footer & structured table -> pdfInspector readback
//   2. Vector graphics (rounded rect, dashed lines, polygon) & 4x4 DeviceRGB Image XObject embedding
//   3. UTF-16BE Unicode metadata (Chinese/ASCII) & `/Outlines` bookmark chain -> exact `extractPdfMetadata` / `extractPdfOutlines`
//   4. `/FlateDecode` compressed streams vs uncompressed size reduction + 100% exact `extractPageText` recovery
//   5. Compressed `/DeviceGray` Image XObject pixel-for-pixel roundtrip via `extractStreamFromObject` + `decodePdfStream`
//   6. Landscape `PageSize` & `/Rotate 90` page metadata verification
//   7. Error boundary guards (`BufferTooSmall`, `OutputSliceTooSmall`, `InvalidImage`, `InvalidPageIndex`) & `cap.doc` parity

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    PdfFont,
    PageSize,
    PdfWriteError,
    PdfTextAlign,
    PdfColorSpace,
    PdfRgb,
    PdfMargins,
    PdfPoint,
    PdfTableStyle,
    PdfImageSpec,
    PdfBookmarkSpec,
    PdfMetadataSpec,
    PdfPageSpec,
    PdfPageCanvas,
    pdfWriterXrefWorkLen,
    writePdfDocument,
    PdfInspector,
    PdfObjRef,
    PdfXrefEntry,
    PdfXrefTable,
    extractPdfMetadata,
    PdfPageInfo,
    collectPdfPages,
    PdfOutlineItem,
    extractPdfOutlines,
    extractStreamFromObject,
    decodePdfStream,
    extractPageText,
} from "cap:doc";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsStr(haystack: []const u8, needle: []const u8): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

test("PDF Writer Deep: multi-page auto-wrap, header/footer & structured table -> pdfInspector readback") {
    // Page 0: A4 portrait with header/footer, wrapped paragraph & 3x2 table
    let p0_buf: [8192]u8 = [0; 8192];
    let c0 = PdfPageCanvas.init(p0_buf[0..]);
    const margins = PdfMargins{};

    c0.drawHeaderFooter(PageSize.a4(), margins, "Quarterly Engineering Report", "Page", 1, 2, PdfFont.Helvetica, 9.0) catch { expect_true(false); return; };
    const y1 = c0.drawWrappedText(
        72.0,
        740.0,
        240.0,
        PdfFont.Helvetica,
        11.0,
        15.0,
        PdfTextAlign.left,
        "CP standard library delivers zero-allocation native PDF layout and text extraction across multiple pages.",
    ) catch { expect_true(false); return; };
    expect_true(y1 < 710.0);

    const col_w: [3]f64 = [140.0, 120.0, 120.0];
    const headers: [3][]const u8 = ["Module", "Latency", "Status"];
    const rows: [6][]const u8 = [
        "pdfWriter", "0.4ms", "PASS",
        "pdfInspector", "0.6ms", "PASS",
    ];
    const y2 = c0.drawTable(72.0, y1 - 16.0, col_w[0..], headers[0..], rows[0..], 2, PdfTableStyle{}) catch {
        expect_true(false);
        return;
    };
    expect_true(y2 < y1 - 60.0);

    // Page 1: A4 landscape with centered & right-aligned text
    let p1_buf: [4096]u8 = [0; 4096];
    let c1 = PdfPageCanvas.init(p1_buf[0..]);
    c1.drawHeaderFooter(PageSize.a4Landscape(), margins, "Appendix", "Page", 2, 2, PdfFont.Courier, 9.0) catch { expect_true(false); return; };
    * = c1.drawWrappedText(72.0, 500.0, 400.0, PdfFont.HelveticaBold, 14.0, 18.0, PdfTextAlign.center, "Centered Summary Heading") catch { expect_true(false); return; };
    * = c1.drawWrappedText(72.0, 460.0, 400.0, PdfFont.TimesRoman, 11.0, 15.0, PdfTextAlign.right, "Right Aligned Footer Note") catch { expect_true(false); return; };

    const pages: [2]PdfPageSpec = [
        PdfPageSpec{ size: PageSize.a4(), content_stream: c0.bytes() },
        PdfPageSpec{ size: PageSize.a4Landscape(), content_stream: c1.bytes() },
    ];
    const no_imgs: [0]PdfImageSpec = [];
    const no_bms: [0]PdfBookmarkSpec = [];
    let xref_work: [32]usize = [0; 32];
    let pdf_out: [16384]u8 = [0; 16384];

    const pdf_len = writePdfDocument(pages[0..], no_imgs[0..], no_bms[0..], PdfMetadataSpec{}, false, xref_work[0..], pdf_out[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(PdfInspector.isValidPdf(pdf_out[0..pdf_len]));

    // Closed-loop verification via pdfInspector.cp
    let xentries: [32]PdfXrefEntry = [PdfXrefEntry{}; 32];
    const xref = PdfXrefTable.parse(pdf_out[0..pdf_len], xentries[0..]) catch { expect_true(false); return; };
    let stack: [8]PdfObjRef = [PdfObjRef{}; 8];
    let pinfo: [4]PdfPageInfo = [PdfPageInfo{}; 4];
    const np = collectPdfPages(pdf_out[0..pdf_len], xref, stack[0..], pinfo[0..]) catch { expect_true(false); return; };
    expect_eq(np, @as(usize, 2));
    // Page 1 is A4 landscape (841.89 x 595.28)
    expect_true(pinfo[1].media_box.width() > 841.0 && pinfo[1].media_box.height() < 596.0);

    let scratch: [8192]u8 = [0; 8192];
    let txt: [4096]u8 = [0; 4096];
    const t0 = extractPageText(pdf_out[0..pdf_len], xref, pinfo[0], scratch[0..], txt[0..]) catch { expect_true(false); return; };
    expect_true(containsStr(txt[0..t0], "Quarterly Engineering Report"));
    expect_true(containsStr(txt[0..t0], "pdfWriter"));
    expect_true(containsStr(txt[0..t0], "pdfInspector"));
    expect_true(containsStr(txt[0..t0], "Page 1/2"));

    const t1 = extractPageText(pdf_out[0..pdf_len], xref, pinfo[1], scratch[0..], txt[0..]) catch { expect_true(false); return; };
    expect_true(containsStr(txt[0..t1], "Centered Summary Heading"));
    expect_true(containsStr(txt[0..t1], "Right Aligned Footer Note"));
    expect_true(containsStr(txt[0..t1], "Page 2/2"));
}

test("PDF Writer Deep: vector primitives (rounded rect, dash, polygon) & RGB Image XObject") {
    let pbuf: [4096]u8 = [0; 4096];
    let canvas = PdfPageCanvas.init(pbuf[0..]);

    canvas.setStrokeRgb(PdfRgb.init(0.1, 0.2, 0.8)) catch { expect_true(false); return; };
    canvas.setFillRgb(PdfRgb.init(0.95, 0.97, 1.0)) catch { expect_true(false); return; };
    canvas.setLineWidth(1.5) catch { expect_true(false); return; };
    canvas.setDashPattern(4.0, 2.0) catch { expect_true(false); return; };
    canvas.drawRoundedRect(50.0, 600.0, 200.0, 80.0, 12.0, true) catch { expect_true(false); return; };
    canvas.clearDashPattern() catch { expect_true(false); return; };

    const tri: [3]PdfPoint = [
        PdfPoint{ x: 300.0, y: 600.0 },
        PdfPoint{ x: 360.0, y: 680.0 },
        PdfPoint{ x: 420.0, y: 600.0 },
    ];
    canvas.drawPolygon(tri[0..], true, true) catch { expect_true(false); return; };
    canvas.drawImageXObject(0, 50.0, 420.0, 120.0, 120.0) catch { expect_true(false); return; };

    // 4x4 DeviceRGB 8-bit image (48 bytes)
    let rgb_pixels: [48]u8 = [0; 48];
    let i: usize = 0;
    while (i < 48) : (i += 1) {
        rgb_pixels[i] = @as(u8, @intCast((i * 5) & 0xFF));
    }

    const pages: [1]PdfPageSpec = [PdfPageSpec{ size: PageSize.letter(), content_stream: canvas.bytes() }];
    const imgs: [1]PdfImageSpec = [PdfImageSpec{ width: 4, height: 4, color_space: PdfColorSpace.device_rgb, pixels: rgb_pixels[0..], compress: false }];
    const no_bms: [0]PdfBookmarkSpec = [];
    let xref_work: [32]usize = [0; 32];
    let pdf_out: [8192]u8 = [0; 8192];

    const pdf_len = writePdfDocument(pages[0..], imgs[0..], no_bms[0..], PdfMetadataSpec{}, false, xref_work[0..], pdf_out[0..]) catch {
        expect_true(false);
        return;
    };

    expect_true(containsStr(pdf_out[0..pdf_len], "/Subtype /Image"));
    expect_true(containsStr(pdf_out[0..pdf_len], "/ColorSpace /DeviceRGB"));
    expect_true(containsStr(pdf_out[0..pdf_len], "/Im1 Do"));
    expect_true(containsStr(pdf_out[0..pdf_len], "[4 2] 0 d"));
    expect_true(PdfInspector.isValidPdf(pdf_out[0..pdf_len]));
}

test("PDF Writer Deep: UTF-16BE Unicode metadata & /Outlines bookmarks closed-loop readback") {
    let p0_buf: [512]u8 = [0; 512];
    let c0 = PdfPageCanvas.init(p0_buf[0..]);
    c0.drawText(72.0, 720.0, PdfFont.HelveticaBold, 14.0, "Chapter 1") catch { expect_true(false); return; };

    let p1_buf: [512]u8 = [0; 512];
    let c1 = PdfPageCanvas.init(p1_buf[0..]);
    c1.drawText(72.0, 720.0, PdfFont.HelveticaBold, 14.0, "Chapter 2") catch { expect_true(false); return; };

    const pages: [2]PdfPageSpec = [
        PdfPageSpec{ size: PageSize.a4(), content_stream: c0.bytes() },
        PdfPageSpec{ size: PageSize.a4(), content_stream: c1.bytes() },
    ];
    const no_imgs: [0]PdfImageSpec = [];
    const bms: [2]PdfBookmarkSpec = [
        PdfBookmarkSpec{ title: "第一章：总览", page_index: 0 },
        PdfBookmarkSpec{ title: "Chapter 2: Details", page_index: 1 },
    ];
    const meta = PdfMetadataSpec{
        title: "CP 标准库架构报告",
        author: "CP 架构组",
        subject: "PDF 1.5 Roundtrip",
        keywords: "cp,pdf,unicode",
        creator: "cap:doc",
        producer: "CP pdfWriter",
    };

    let xref_work: [32]usize = [0; 32];
    let pdf_out: [8192]u8 = [0; 8192];
    const pdf_len = writePdfDocument(pages[0..], no_imgs[0..], bms[0..], meta, false, xref_work[0..], pdf_out[0..]) catch {
        expect_true(false);
        return;
    };

    let xentries: [32]PdfXrefEntry = [PdfXrefEntry{}; 32];
    const xref = PdfXrefTable.parse(pdf_out[0..pdf_len], xentries[0..]) catch { expect_true(false); return; };

    let arena: [512]u8 = [0; 512];
    const got_meta = extractPdfMetadata(pdf_out[0..pdf_len], xref, arena[0..]) catch { expect_true(false); return; };
    expect_true(bytesEq(got_meta.title.slice(arena[0..]), "CP 标准库架构报告"));
    expect_true(bytesEq(got_meta.author.slice(arena[0..]), "CP 架构组"));
    expect_true(bytesEq(got_meta.subject.slice(arena[0..]), "PDF 1.5 Roundtrip"));
    expect_true(bytesEq(got_meta.keywords.slice(arena[0..]), "cp,pdf,unicode"));

    let out_bms: [8]PdfOutlineItem = [PdfOutlineItem{}; 8];
    let bm_arena: [256]u8 = [0; 256];
    let bm_used: usize = 0;
    const n_bm = extractPdfOutlines(pdf_out[0..pdf_len], xref, out_bms[0..], bm_arena[0..], *bm_used) catch { expect_true(false); return; };
    expect_eq(n_bm, @as(usize, 2));
    expect_true(bytesEq(out_bms[0].titleSlice(bm_arena[0..]), "第一章：总览"));
    expect_true(bytesEq(out_bms[1].titleSlice(bm_arena[0..]), "Chapter 2: Details"));
}

test("PDF Writer Deep: /FlateDecode compression shrinks size & pdfInspector restores 100% exact text") {
    let pbuf: [8192]u8 = [0; 8192];
    let canvas = PdfPageCanvas.init(pbuf[0..]);
    let line: usize = 0;
    while (line < 24) : (line += 1) {
        const y: f64 = 760.0 - (line as f64) * 20.0;
        canvas.drawText(72.0, y, PdfFont.Helvetica, 10.0, "Repeated structured report line for FlateDecode compression verification.") catch {
            expect_true(false);
            return;
        };
    }

    const pages: [1]PdfPageSpec = [PdfPageSpec{ size: PageSize.a4(), content_stream: canvas.bytes() }];
    const no_imgs: [0]PdfImageSpec = [];
    const no_bms: [0]PdfBookmarkSpec = [];
    let xref_work: [32]usize = [0; 32];
    let raw_pdf: [16384]u8 = [0; 16384];
    let cmp_pdf: [16384]u8 = [0; 16384];

    const raw_len = writePdfDocument(pages[0..], no_imgs[0..], no_bms[0..], PdfMetadataSpec{}, false, xref_work[0..], raw_pdf[0..]) catch { expect_true(false); return; };
    const cmp_len = writePdfDocument(pages[0..], no_imgs[0..], no_bms[0..], PdfMetadataSpec{}, true, xref_work[0..], cmp_pdf[0..]) catch { expect_true(false); return; };

    // FlateDecode stream must substantially reduce overall PDF size
    expect_true(cmp_len < raw_len / 2);
    expect_true(containsStr(cmp_pdf[0..cmp_len], "/Filter /FlateDecode"));

    // Read back both raw and FlateDecode PDFs with pdfInspector and verify identical extracted text
    let x1: [16]PdfXrefEntry = [PdfXrefEntry{}; 16];
    let x2: [16]PdfXrefEntry = [PdfXrefEntry{}; 16];
    const xr1 = PdfXrefTable.parse(raw_pdf[0..raw_len], x1[0..]) catch { expect_true(false); return; };
    const xr2 = PdfXrefTable.parse(cmp_pdf[0..cmp_len], x2[0..]) catch { expect_true(false); return; };
    let st1: [8]PdfObjRef = [PdfObjRef{}; 8];
    let st2: [8]PdfObjRef = [PdfObjRef{}; 8];
    let pg1: [2]PdfPageInfo = [PdfPageInfo{}; 2];
    let pg2: [2]PdfPageInfo = [PdfPageInfo{}; 2];
    * = collectPdfPages(raw_pdf[0..raw_len], xr1, st1[0..], pg1[0..]) catch { expect_true(false); return; };
    * = collectPdfPages(cmp_pdf[0..cmp_len], xr2, st2[0..], pg2[0..]) catch { expect_true(false); return; };

    let scr: [8192]u8 = [0; 8192];
    let txt1: [4096]u8 = [0; 4096];
    let txt2: [4096]u8 = [0; 4096];
    const n1 = extractPageText(raw_pdf[0..raw_len], xr1, pg1[0], scr[0..], txt1[0..]) catch { expect_true(false); return; };
    const n2 = extractPageText(cmp_pdf[0..cmp_len], xr2, pg2[0], scr[0..], txt2[0..]) catch { expect_true(false); return; };
    expect_eq(n1, n2);
    expect_true(bytesEq(txt1[0..n1], txt2[0..n2]));
    expect_true(containsStr(txt2[0..n2], "Repeated structured report line"));
}

test("PDF Writer Deep: compressed /DeviceGray Image XObject pixel-for-pixel decode roundtrip") {
    let gray_pixels: [64]u8 = [0; 64]; // 8x8 grayscale ramp
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        gray_pixels[i] = @as(u8, @intCast(i * 4));
    }

    const pages: [1]PdfPageSpec = [PdfPageSpec{ size: PageSize.letter(), rotate: 90, content_stream: "" }];
    const imgs: [1]PdfImageSpec = [PdfImageSpec{ width: 8, height: 8, color_space: PdfColorSpace.device_gray, pixels: gray_pixels[0..], compress: true }];
    const no_bms: [0]PdfBookmarkSpec = [];
    let xref_work: [32]usize = [0; 32];
    let pdf_out: [4096]u8 = [0; 4096];

    const pdf_len = writePdfDocument(pages[0..], imgs[0..], no_bms[0..], PdfMetadataSpec{}, true, xref_work[0..], pdf_out[0..]) catch {
        expect_true(false);
        return;
    };

    let xbuf: [16]PdfXrefEntry = [PdfXrefEntry{}; 16];
    const xref = PdfXrefTable.parse(pdf_out[0..pdf_len], xbuf[0..]) catch { expect_true(false); return; };

    // Verify /Rotate 90 on page 0
    let st: [4]PdfObjRef = [PdfObjRef{}; 4];
    let pgs: [2]PdfPageInfo = [PdfPageInfo{}; 2];
    * = collectPdfPages(pdf_out[0..pdf_len], xref, st[0..], pgs[0..]) catch { expect_true(false); return; };
    expect_eq(pgs[0].rotate, @as(i32, 90));

    // Image XObject is object 7 -> extract & decode FlateDecode pixel stream
    const img_opt = xref.locateObjectBody(pdf_out[0..pdf_len], PdfObjRef{ obj_num: 7, gen_num: 0 });
    expect_true(img_opt != null);
    const img_body = img_opt ?? "";
    const sv = extractStreamFromObject(pdf_out[0..pdf_len], xref, img_body) catch { expect_true(false); return; };
    let dec_pixels: [128]u8 = [0; 128];
    const dlen = decodePdfStream(sv, dec_pixels[0..]) catch { expect_true(false); return; };
    expect_eq(dlen, @as(usize, 64));
    expect_true(bytesEq(dec_pixels[0..64], gray_pixels[0..]));
}

test("PDF Writer Deep: error boundary guards & cap.doc facade parity") {
    const pages: [1]PdfPageSpec = [PdfPageSpec{ size: PageSize.a4(), content_stream: "" }];
    const no_imgs: [0]PdfImageSpec = [];
    const no_bms: [0]PdfBookmarkSpec = [];
    let xref_work: [32]usize = [0; 32];
    let pdf_out: [2048]u8 = [0; 2048];

    // 1. InvalidImage (mismatched pixel length)
    const bad_pix: [10]u8 = [0; 10];
    const bad_imgs: [1]PdfImageSpec = [PdfImageSpec{ width: 4, height: 4, color_space: PdfColorSpace.device_rgb, pixels: bad_pix[0..] }];
    let got_img: bool = false;
    * = cap.doc.writePdfDocument(pages[0..], bad_imgs[0..], no_bms[0..], PdfMetadataSpec{}, false, xref_work[0..], pdf_out[0..]) catch |e| {
        if (e == PdfWriteError.InvalidImage) got_img = true;
    };
    expect_true(got_img);

    // 2. InvalidPageIndex in bookmark
    const bad_bms: [1]PdfBookmarkSpec = [PdfBookmarkSpec{ title: "OOB", page_index: 5 }];
    let got_bm: bool = false;
    * = writePdfDocument(pages[0..], no_imgs[0..], bad_bms[0..], PdfMetadataSpec{}, false, xref_work[0..], pdf_out[0..]) catch |e| {
        if (e == PdfWriteError.InvalidPageIndex) got_bm = true;
    };
    expect_true(got_bm);

    // 3. OutputSliceTooSmall on xref_offsets_work
    let tiny_xref: [2]usize = [0; 2];
    let got_xsmall: bool = false;
    * = writePdfDocument(pages[0..], no_imgs[0..], no_bms[0..], PdfMetadataSpec{}, false, tiny_xref[0..], pdf_out[0..]) catch |e| {
        if (e == PdfWriteError.OutputSliceTooSmall) got_xsmall = true;
    };
    expect_true(got_xsmall);

    // 4. BufferTooSmall on out_buf
    let tiny_out: [32]u8 = [0; 32];
    let got_bsmall: bool = false;
    * = writePdfDocument(pages[0..], no_imgs[0..], no_bms[0..], PdfMetadataSpec{}, false, xref_work[0..], tiny_out[0..]) catch |e| {
        if (e == PdfWriteError.BufferTooSmall) got_bsmall = true;
    };
    expect_true(got_bsmall);

    // 5. Facade parity
    expect_eq(cap.doc.pdfWriterXrefWorkLen(2, 1, 2, true), pdfWriterXrefWorkLen(2, 1, 2, true));
}
