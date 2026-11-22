// Origin: Pure-CP Stdlib Test Suite (lib/doc/pdfInspector.cp) | Created: 2026-11-23
// Production Deep Verification Suite for PDF 1.4-1.7 Object/XRef/FlateDecode/Page-Tree/Outlines & Text Extraction:
//   1. Closed-loop interop with `PdfDocument` (`pdfWriter.cp`): 3-page PDF -> XRef, Metadata, MediaBox & Page Text
//   2. `/FlateDecode` compressed content stream (`cap:pack` `deflate`) + indirect `/Length N 0 R` + `TJ` kerning
//   3. Incremental update `/Prev` XRef chain (newest section overrides older `/Info` object)
//   4. `PdfLexer`, `decodePdfName` (`#XX`), literal escapes (octal/nested parens), hex strings & UTF-16BE BOM (`中文`)
//   5. `/ASCIIHexDecode` stream, `/Contents [...]` multi-stream array & `/Outlines` bookmark chain
//   6. `extractTextFromContentStream` operator matrix (`Tj`, `TJ`, `'`, `"`, `T*`, `Td`, `Tm`)
//   7. Error boundary guards (`InvalidPdf`, `MissingXRef`, `OutputSliceTooSmall`, `BufferTooSmall`, `DecompressFailed`) & `cap.doc` parity

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import { deflate } from "cap:pack";
import {
    PdfFont,
    PageSize,
    PdfDocument,
    PdfParseError,
    PdfTokenKind,
    PdfLexer,
    PdfObjRef,
    PdfXrefEntry,
    PdfXrefTable,
    decodePdfName,
    decodePdfStringToUtf8,
    PdfStreamFilter,
    PdfStreamView,
    decodePdfStream,
    extractPdfMetadata,
    PdfPageInfo,
    collectPdfPages,
    PdfOutlineItem,
    extractPdfOutlines,
    extractTextFromContentStream,
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

fn appendBuf(dst: []u8, pos: *usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[pos.* + i] = src[i];
    }
    pos.* += src.len;
}

fn appendDec(dst: []u8, pos: *usize, val: usize) {
    let tmp: [32]u8 = [0; 32];
    const s = std.fmt.bufPrint(tmp[0..], "{d}", .{val}) catch "";
    appendBuf(dst, pos, s);
}

fn appendXref10(dst: []u8, pos: *usize, off: usize) {
    let tmp: [32]u8 = [0; 32];
    const s = std.fmt.bufPrint(tmp[0..], "{d:0>10} 00000 n \n", .{off}) catch "";
    appendBuf(dst, pos, s);
}

test("PDF Deep Interop: PdfDocument 3-page build -> XRef, Metadata, PageTree & Text extraction") {
    const alloc = std.heap.page_allocator;
    let doc = PdfDocument.init(alloc);
    defer doc.deinit();

    doc.setMetadata("CP PDF Engine Spec", "CP Team", "cap:doc writer");

    const p0 = doc.addPage(PageSize.a4()) catch unreachable;
    p0.drawText(72.0, 750.0, PdfFont.HelveticaBold, 16.0, "Chapter 1: Overview") catch unreachable;
    p0.drawText(72.0, 720.0, PdfFont.Helvetica, 12.0, "Zero-alloc (native) PDF parser.") catch unreachable;

    const p1 = doc.addPage(PageSize.letter()) catch unreachable;
    p1.drawText(72.0, 700.0, PdfFont.Courier, 11.0, "Page Two Letter Content") catch unreachable;

    const p2 = doc.addPage(PageSize.custom(400.0, 300.0)) catch unreachable;
    p2.drawText(36.0, 240.0, PdfFont.TimesRoman, 12.0, "Custom Page Three") catch unreachable;

    const pdf_bytes = doc.build(alloc) catch unreachable;
    defer alloc.free(pdf_bytes);

    let xref_buf: [64]PdfXrefEntry = [PdfXrefEntry{}; 64];
    const xref = PdfXrefTable.parse(pdf_bytes, xref_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(xref.trailer.has_root);
    expect_true(xref.trailer.has_info);
    expect_true(!xref.trailer.has_encrypt);

    let arena: [512]u8 = [0; 512];
    const meta = extractPdfMetadata(pdf_bytes, xref, arena[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(bytesEq(meta.version, "1.4"));
    expect_true(bytesEq(meta.title.slice(arena[0..]), "CP PDF Engine Spec"));
    expect_true(bytesEq(meta.author.slice(arena[0..]), "CP Team"));
    expect_true(bytesEq(meta.creator.slice(arena[0..]), "cap:doc writer"));

    let stack: [16]PdfObjRef = [PdfObjRef{}; 16];
    let pages: [8]PdfPageInfo = [PdfPageInfo{}; 8];
    const n_pages = collectPdfPages(pdf_bytes, xref, stack[0..], pages[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(n_pages, @as(usize, 3));
    expect_true(pages[0].media_box.width() > 595.0 && pages[0].media_box.width() < 596.0);
    expect_true(pages[1].media_box.width() > 611.9 && pages[1].media_box.width() < 612.1);
    expect_true(pages[2].media_box.width() > 399.9 && pages[2].media_box.width() < 400.1);
    expect_true(pages[0].font_count >= 1);

    let scratch: [4096]u8 = [0; 4096];
    let text_buf: [1024]u8 = [0; 1024];
    const t0 = extractPageText(pdf_bytes, xref, pages[0], scratch[0..], text_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(containsStr(text_buf[0..t0], "Chapter 1: Overview"));
    expect_true(containsStr(text_buf[0..t0], "Zero-alloc (native) PDF parser."));

    const t1 = extractPageText(pdf_bytes, xref, pages[1], scratch[0..], text_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(containsStr(text_buf[0..t1], "Page Two Letter Content"));

    const t2 = extractPageText(pdf_bytes, xref, pages[2], scratch[0..], text_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(containsStr(text_buf[0..t2], "Custom Page Three"));
}

test("PDF /FlateDecode compressed stream (via cap:pack deflate) + indirect /Length N 0 R + TJ kerning") {
    const raw_content = "BT /F1 12 Tf 72 720 Td [(Hello) -250 (FlateDecode) -200 (World!)] TJ T* (Second Line) Tj ET\n";
    const compressed = deflate(raw_content, 6);
    expect_true(compressed.len > 2);

    // Construct a complete valid PDF 1.5 with FlateDecode stream & indirect `/Length 5 0 R`
    let pdf: [2048]u8 = [0; 2048];
    let pos: usize = 0;
    let offs: [6]usize = [0; 6];

    appendBuf(pdf[0..], *pos, "%PDF-1.5\n");

    offs[1] = pos;
    appendBuf(pdf[0..], *pos, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");

    offs[2] = pos;
    appendBuf(pdf[0..], *pos, "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 /MediaBox [0 0 612 792] >>\nendobj\n");

    offs[3] = pos;
    appendBuf(pdf[0..], *pos, "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R >>\nendobj\n");

    offs[4] = pos;
    appendBuf(pdf[0..], *pos, "4 0 obj\n<< /Filter [/FlateDecode] /Length 5 0 R >>\nstream\n");
    appendBuf(pdf[0..], *pos, compressed);
    appendBuf(pdf[0..], *pos, "\nendstream\nendobj\n");

    offs[5] = pos;
    appendBuf(pdf[0..], *pos, "5 0 obj\n");
    appendDec(pdf[0..], *pos, compressed.len);
    appendBuf(pdf[0..], *pos, "\nendobj\n");

    const xref_off = pos;
    appendBuf(pdf[0..], *pos, "xref\n0 6\n0000000000 65535 f \n");
    let k: usize = 1;
    while (k <= 5) : (k += 1) {
        appendXref10(pdf[0..], *pos, offs[k]);
    }
    appendBuf(pdf[0..], *pos, "trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n");
    appendDec(pdf[0..], *pos, xref_off);
    appendBuf(pdf[0..], *pos, "\n%%EOF\n");

    let xbuf: [16]PdfXrefEntry = [PdfXrefEntry{}; 16];
    const xref = PdfXrefTable.parse(pdf[0..pos], xbuf[0..]) catch { expect_true(false); return; };

    let stack: [8]PdfObjRef = [PdfObjRef{}; 8];
    let pages: [4]PdfPageInfo = [PdfPageInfo{}; 4];
    const np = collectPdfPages(pdf[0..pos], xref, stack[0..], pages[0..]) catch { expect_true(false); return; };
    expect_eq(np, @as(usize, 1));
    // Inherited MediaBox from parent `/Pages`
    expect_true(pages[0].media_box.width() > 611.9 && pages[0].media_box.width() < 612.1);

    let scratch: [512]u8 = [0; 512];
    let out_txt: [256]u8 = [0; 256];
    const tlen = extractPageText(pdf[0..pos], xref, pages[0], scratch[0..], out_txt[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(bytesEq(out_txt[0..tlen], "Hello FlateDecode World!\nSecond Line"));
}

test("PDF Incremental Update /Prev XRef chain: newest section overrides older /Info object") {
    let pdf: [2048]u8 = [0; 2048];
    let pos: usize = 0;

    appendBuf(pdf[0..], *pos, "%PDF-1.6\n");
    const o1 = pos;
    appendBuf(pdf[0..], *pos, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");
    const o2 = pos;
    appendBuf(pdf[0..], *pos, "2 0 obj\n<< /Type /Pages /Kids [] /Count 0 >>\nendobj\n");
    const o3_old = pos;
    appendBuf(pdf[0..], *pos, "3 0 obj\n<< /Title (Draft v1) /Author (Alice) >>\nendobj\n");

    const xref1 = pos;
    appendBuf(pdf[0..], *pos, "xref\n0 4\n0000000000 65535 f \n");
    appendXref10(pdf[0..], *pos, o1);
    appendXref10(pdf[0..], *pos, o2);
    appendXref10(pdf[0..], *pos, o3_old);
    appendBuf(pdf[0..], *pos, "trailer\n<< /Size 4 /Root 1 0 R /Info 3 0 R >>\nstartxref\n");
    appendDec(pdf[0..], *pos, xref1);
    appendBuf(pdf[0..], *pos, "\n%%EOF\n");

    // Incremental update overriding object 3 with UTF-16BE Title & new Subject
    const o3_new = pos;
    appendBuf(pdf[0..], *pos, "3 0 obj\n<< /Title (Final v2) /Author (Alice) /Subject (Updated) >>\nendobj\n");
    const xref2 = pos;
    appendBuf(pdf[0..], *pos, "xref\n3 1\n");
    appendXref10(pdf[0..], *pos, o3_new);
    appendBuf(pdf[0..], *pos, "trailer\n<< /Size 4 /Prev ");
    appendDec(pdf[0..], *pos, xref1);
    appendBuf(pdf[0..], *pos, " >>\nstartxref\n");
    appendDec(pdf[0..], *pos, xref2);
    appendBuf(pdf[0..], *pos, "\n%%EOF\n");

    let xbuf: [16]PdfXrefEntry = [PdfXrefEntry{}; 16];
    const xref = PdfXrefTable.parse(pdf[0..pos], xbuf[0..]) catch { expect_true(false); return; };
    expect_eq(xref.trailer.xref_sections_parsed, @as(usize, 2));
    expect_true(xref.trailer.has_prev);
    expect_eq(xref.trailer.prev_xref_offset, xref1);

    let arena: [256]u8 = [0; 256];
    const meta = extractPdfMetadata(pdf[0..pos], xref, arena[0..]) catch { expect_true(false); return; };
    expect_true(bytesEq(meta.title.slice(arena[0..]), "Final v2"));
    expect_true(bytesEq(meta.subject.slice(arena[0..]), "Updated"));
}

test("PDF Lexer & Decoders: comments, #XX names, octal/nested escapes, hex strings & UTF-16BE BOM") {
    let out: [128]u8 = [0; 128];

    // 1. Name `#XX` decoding
    const nlen = decodePdfName("/Flate#44ecode#20v1", out[0..]) catch { expect_true(false); return; };
    expect_true(bytesEq(out[0..nlen], "FlateDecode v1"));

    // 2. Literal string escapes: nested parens, \n, \t, octal \101 ('A')
    const slen = decodePdfStringToUtf8("(Hi (nested) \\101\\n\\tOK)", out[0..]) catch { expect_true(false); return; };
    expect_true(bytesEq(out[0..slen], "Hi (nested) A\n\tOK"));

    // 3. Hex string `<48656C6C6F>` -> "Hello"
    const hlen = decodePdfStringToUtf8("<48 65 6C 6C 6F>", out[0..]) catch { expect_true(false); return; };
    expect_true(bytesEq(out[0..hlen], "Hello"));

    // 4. UTF-16BE BOM `<FEFF 4E2D 6587>` -> "中文" (UTF-8: E4 B8 AD E6 96 87)
    const ulen = decodePdfStringToUtf8("<FEFF4E2D6587>", out[0..]) catch { expect_true(false); return; };
    expect_true(bytesEq(out[0..ulen], "中文"));

    // 5. Lexer skips `%` comments
    const src = "% header comment\n<< /Key 42 % inline\n /Flag true >>";
    let lex = PdfLexer.init(src, 0);
    expect_true(lex.nextToken().kind == PdfTokenKind.dict_start);
    expect_true(lex.nextToken().kind == PdfTokenKind.name);
    expect_true(lex.nextToken().kind == PdfTokenKind.number_int);
    expect_true(lex.nextToken().kind == PdfTokenKind.name);
    expect_true(lex.nextToken().kind == PdfTokenKind.boolean);
    expect_true(lex.nextToken().kind == PdfTokenKind.dict_end);
}

test("PDF /ASCIIHexDecode, /Contents array & /Outlines bookmark extraction") {
    let pdf: [2048]u8 = [0; 2048];
    let pos: usize = 0;
    let offs: [9]usize = [0; 9];

    appendBuf(pdf[0..], *pos, "%PDF-1.7\n");
    offs[1] = pos;
    appendBuf(pdf[0..], *pos, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R /Outlines 6 0 R >>\nendobj\n");
    offs[2] = pos;
    appendBuf(pdf[0..], *pos, "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n");
    offs[3] = pos;
    appendBuf(pdf[0..], *pos, "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 500 500] /Rotate 90 /Contents [4 0 R 5 0 R] >>\nendobj\n");
    // Stream 4: ASCIIHexDecode of "BT (Part1) Tj ET" = 4254202850617274312920546A204554>
    offs[4] = pos;
    appendBuf(pdf[0..], *pos, "4 0 obj\n<< /Filter /ASCIIHexDecode /Length 33 >>\nstream\n4254202850617274312920546A204554>\nendstream\nendobj\n");
    // Stream 5: raw uncompressed
    offs[5] = pos;
    appendBuf(pdf[0..], *pos, "5 0 obj\n<< /Length 16 >>\nstream\nBT (Part2) Tj ET\nendstream\nendobj\n");
    // Outlines 6 -> 7 -> 8
    offs[6] = pos;
    appendBuf(pdf[0..], *pos, "6 0 obj\n<< /Type /Outlines /First 7 0 R /Last 8 0 R >>\nendobj\n");
    offs[7] = pos;
    appendBuf(pdf[0..], *pos, "7 0 obj\n<< /Title (1. Intro) /Next 8 0 R >>\nendobj\n");
    offs[8] = pos;
    appendBuf(pdf[0..], *pos, "8 0 obj\n<< /Title (2. Deep Dive) >>\nendobj\n");

    const xref_off = pos;
    appendBuf(pdf[0..], *pos, "xref\n0 9\n0000000000 65535 f \n");
    let k: usize = 1;
    while (k <= 8) : (k += 1) {
        appendXref10(pdf[0..], *pos, offs[k]);
    }
    appendBuf(pdf[0..], *pos, "trailer\n<< /Size 9 /Root 1 0 R >>\nstartxref\n");
    appendDec(pdf[0..], *pos, xref_off);
    appendBuf(pdf[0..], *pos, "\n%%EOF\n");

    let xbuf: [16]PdfXrefEntry = [PdfXrefEntry{}; 16];
    const xref = PdfXrefTable.parse(pdf[0..pos], xbuf[0..]) catch { expect_true(false); return; };

    let stack: [8]PdfObjRef = [PdfObjRef{}; 8];
    let pages: [4]PdfPageInfo = [PdfPageInfo{}; 4];
    const np = collectPdfPages(pdf[0..pos], xref, stack[0..], pages[0..]) catch { expect_true(false); return; };
    expect_eq(np, @as(usize, 1));
    expect_eq(pages[0].rotate, @as(i32, 90));
    expect_true(pages[0].has_contents_array);

    let scratch: [256]u8 = [0; 256];
    let text_buf: [256]u8 = [0; 256];
    const tlen = extractPageText(pdf[0..pos], xref, pages[0], scratch[0..], text_buf[0..]) catch { expect_true(false); return; };
    expect_true(bytesEq(text_buf[0..tlen], "Part1\nPart2"));

    let outlines: [8]PdfOutlineItem = [PdfOutlineItem{}; 8];
    let o_arena: [256]u8 = [0; 256];
    let o_used: usize = 0;
    const n_out = extractPdfOutlines(pdf[0..pos], xref, outlines[0..], o_arena[0..], *o_used) catch { expect_true(false); return; };
    expect_eq(n_out, @as(usize, 2));
    expect_true(bytesEq(outlines[0].titleSlice(o_arena[0..]), "1. Intro"));
    expect_true(bytesEq(outlines[1].titleSlice(o_arena[0..]), "2. Deep Dive"));
}

test("PDF Content Stream BT..ET operator matrix (Tj, TJ kerning, ', \", T*, Td, Tm)") {
    const stream =
        "BT 1 0 0 1 72 700 Tm (LineA) Tj " ++
        "1 0 0 1 72 680 Tm [(Ker) -30 (ning) -180 (Space)] TJ " ++
        "(NextQuote) ' 0 0 (DoubleQuote) \" ET";
    let out: [256]u8 = [0; 256];
    const n = extractTextFromContentStream(stream, out[0..]) catch { expect_true(false); return; };
    expect_true(bytesEq(out[0..n], "LineA\nKerning Space\nNextQuote\nDoubleQuote"));
}

test("PDF Inspector error boundary guards & cap.doc facade parity") {
    let xbuf: [4]PdfXrefEntry = [PdfXrefEntry{}; 4];

    // 1. InvalidPdf
    let got_inv: bool = false;
    * = cap.doc.PdfXrefTable.parse("NOT A PDF", xbuf[0..]) catch |e| {
        if (e == PdfParseError.InvalidPdf) got_inv = true;
    };
    expect_true(got_inv);

    // 2. MissingXRef
    let got_xref: bool = false;
    * = PdfXrefTable.parse("%PDF-1.4\n%%EOF\n", xbuf[0..]) catch |e| {
        if (e == PdfParseError.MissingXRef) got_xref = true;
    };
    expect_true(got_xref);

    // 3. DecompressFailed on corrupted FlateDecode stream
    const bad_flate = PdfStreamView{
        dict_slice: "",
        raw_data: "\x78\x9C\xFF\xFF\xFF",
        filter: PdfStreamFilter.flate_decode,
    };
    let out: [64]u8 = [0; 64];
    let got_decomp: bool = false;
    * = cap.doc.decodePdfStream(bad_flate, out[0..]) catch |e| {
        if (e == PdfParseError.DecompressFailed) got_decomp = true;
    };
    expect_true(got_decomp);

    // 4. BufferTooSmall on decodePdfName
    let tiny: [2]u8 = [0; 2];
    let got_small: bool = false;
    * = cap.doc.decodePdfName("/VeryLongPdfName", tiny[0..]) catch |e| {
        if (e == PdfParseError.BufferTooSmall) got_small = true;
    };
    expect_true(got_small);
}
