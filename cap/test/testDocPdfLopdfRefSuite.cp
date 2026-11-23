// lopdf (MIT) Reference-Hardened PDF 1.5+ XRef Stream, PNG/TIFF Predictor & ObjStm Suite
// Covers PDF 1.5+ /Type /XRef streams, /W & /Index, PNG Predictor 10..15 & TIFF Predictor 2,
// /Type /ObjStm compressed object transparent resolution, hybrid /XRefStm + /Prev chains,
// real lopdf/assets/ PDF samples, and 1,000-iteration deterministic malformed fuzzing.

const std = @import("std");
import { test, expect_eq, expect_true, expect_false } from "cap:test";
import { deflate } from "cap:pack";
import {
    PdfInspector,
    PdfParseError,
    PdfObjRef,
    PdfXrefEntry,
    PdfXrefTable,
    PdfDecodeParms,
    applyPdfPredictorDecode,
    locateObjectBodyFromObjStm,
    extractPdfMetadata,
    PdfPageInfo,
    collectPdfPages,
    extractPageText,
} from "../../lib/doc/index.cp";

fn appendBytes(buf: []u8, pos: *usize, s: []const u8) {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
}

fn appendUsizeAscii(buf: []u8, pos: *usize, v: usize) {
    let tmp: [24]u8 = [0; 24];
    const s = std.fmt.bufPrint(tmp[0..], "{d}", .{v}) catch unreachable;
    appendBytes(buf, pos, s);
}

/// Builds a complete valid PDF 1.5 document where objects 1 (/Catalog), 2 (/Pages),
/// 3 (/Page), and 5 (/Info) are compressed inside Object Stream 6 (/Type /ObjStm, /FlateDecode),
/// object 4 is an uncompressed content stream, and object 7 is a /Type /XRef stream
/// encoded with /Filter /FlateDecode + /DecodeParms << /Predictor 12 /Columns 4 >> (PNG Up).
fn buildModernPdf15WithXrefStmAndObjStm(out: []u8): usize {
    let pos: usize = 0;
    appendBytes(out, &pos, "%PDF-1.5\n%\xE2\xE3\xCF\xD3\n");

    // Object 4: Page content stream (uncompressed normal object, type 1)
    const off_obj4: usize = pos;
    const content = "BT /F1 12 Tf 72 720 Td (Hello PDF 1.5 ObjStm!) Tj ET";
    appendBytes(out, &pos, "4 0 obj\n<< /Length ");
    appendUsizeAscii(out, &pos, content.len);
    appendBytes(out, &pos, " >>\nstream\n");
    appendBytes(out, &pos, content);
    appendBytes(out, &pos, "\nendstream\nendobj\n");

    // Object 6: /Type /ObjStm holding compressed objects 1, 2, 3, 5
    const obj1 = "<< /Type /Catalog /Pages 2 0 R >>";
    const obj2 = "<< /Type /Pages /Kids [3 0 R] /Count 1 /MediaBox [0 0 612 792] >>";
    const obj3 = "<< /Type /Page /Parent 2 0 R /Contents 4 0 R /Resources << /Font << /F1 << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >> >> >> >>";
    const obj5 = "<< /Title (Modern PDF 1.5 Title) /Author (CP Stdlib) >>";

    let objstm_raw: [1024]u8 = [0; 1024];
    let op: usize = 0;
    const o1_rel: usize = 0;
    const o2_rel: usize = obj1.len + 1;
    const o3_rel: usize = o2_rel + obj2.len + 1;
    const o5_rel: usize = o3_rel + obj3.len + 1;

    // Header pairs: "1 <o1> 2 <o2> 3 <o3> 5 <o5> "
    appendBytes(objstm_raw[0..], &op, "1 ");
    appendUsizeAscii(objstm_raw[0..], &op, o1_rel);
    appendBytes(objstm_raw[0..], &op, " 2 ");
    appendUsizeAscii(objstm_raw[0..], &op, o2_rel);
    appendBytes(objstm_raw[0..], &op, " 3 ");
    appendUsizeAscii(objstm_raw[0..], &op, o3_rel);
    appendBytes(objstm_raw[0..], &op, " 5 ");
    appendUsizeAscii(objstm_raw[0..], &op, o5_rel);
    appendBytes(objstm_raw[0..], &op, " ");
    const first_off: usize = op;

    appendBytes(objstm_raw[0..], &op, obj1);
    appendBytes(objstm_raw[0..], &op, "\n");
    appendBytes(objstm_raw[0..], &op, obj2);
    appendBytes(objstm_raw[0..], &op, "\n");
    appendBytes(objstm_raw[0..], &op, obj3);
    appendBytes(objstm_raw[0..], &op, "\n");
    appendBytes(objstm_raw[0..], &op, obj5);

    const objstm_comp = deflate(objstm_raw[0..op]).?;
    const off_obj6: usize = pos;
    appendBytes(out, &pos, "6 0 obj\n<< /Type /ObjStm /N 4 /First ");
    appendUsizeAscii(out, &pos, first_off);
    appendBytes(out, &pos, " /Filter /FlateDecode /Length ");
    appendUsizeAscii(out, &pos, objstm_comp.len);
    appendBytes(out, &pos, " >>\nstream\n");
    appendBytes(out, &pos, objstm_comp);
    appendBytes(out, &pos, "\nendstream\nendobj\n");

    // Object 7: /Type /XRef stream for objects 0..7 (/Size 8, /W [1 2 1]) with PNG Up Predictor (12)
    const off_obj7: usize = pos;
    let raw_rows: [8 * 4]u8 = [0; 8 * 4];
    // Entry 0: type 0 (free)
    raw_rows[0] = 0; raw_rows[1] = 0; raw_rows[2] = 0; raw_rows[3] = 255;
    // Entry 1: type 2, objstm=6, idx=0
    raw_rows[4] = 2; raw_rows[5] = 0; raw_rows[6] = 6; raw_rows[7] = 0;
    // Entry 2: type 2, objstm=6, idx=1
    raw_rows[8] = 2; raw_rows[9] = 0; raw_rows[10] = 6; raw_rows[11] = 1;
    // Entry 3: type 2, objstm=6, idx=2
    raw_rows[12] = 2; raw_rows[13] = 0; raw_rows[14] = 6; raw_rows[15] = 2;
    // Entry 4: type 1, offset=off_obj4, gen=0
    raw_rows[16] = 1; raw_rows[17] = @as(u8, @intCast((off_obj4 >> 8) & 0xFF)); raw_rows[18] = @as(u8, @intCast(off_obj4 & 0xFF)); raw_rows[19] = 0;
    // Entry 5: type 2, objstm=6, idx=3
    raw_rows[20] = 2; raw_rows[21] = 0; raw_rows[22] = 6; raw_rows[23] = 3;
    // Entry 6: type 1, offset=off_obj6, gen=0
    raw_rows[24] = 1; raw_rows[25] = @as(u8, @intCast((off_obj6 >> 8) & 0xFF)); raw_rows[26] = @as(u8, @intCast(off_obj6 & 0xFF)); raw_rows[27] = 0;
    // Entry 7: type 1, offset=off_obj7, gen=0
    raw_rows[28] = 1; raw_rows[29] = @as(u8, @intCast((off_obj7 >> 8) & 0xFF)); raw_rows[30] = @as(u8, @intCast(off_obj7 & 0xFF)); raw_rows[31] = 0;

    // Encode rows with PNG Up filter (filter byte = 2)
    let pred_rows: [8 * 5]u8 = [0; 8 * 5];
    let r: usize = 0;
    while (r < 8) : (r += 1) {
        pred_rows[r * 5] = 2; // PNG Up
        let c: usize = 0;
        while (c < 4) : (c += 1) {
            const up: u8 = if (r == 0) 0 else raw_rows[(r - 1) * 4 + c];
            pred_rows[r * 5 + 1 + c] = raw_rows[r * 4 + c] -% up;
        }
    }

    const xref_comp = deflate(pred_rows[0..]).?;
    appendBytes(out, &pos, "7 0 obj\n<< /Type /XRef /Size 8 /W [1 2 1] /Root 1 0 R /Info 5 0 R /Filter /FlateDecode /DecodeParms << /Predictor 12 /Columns 4 >> /Length ");
    appendUsizeAscii(out, &pos, xref_comp.len);
    appendBytes(out, &pos, " >>\nstream\n");
    appendBytes(out, &pos, xref_comp);
    appendBytes(out, &pos, "\nendstream\nendobj\nstartxref\n");
    appendUsizeAscii(out, &pos, off_obj7);
    appendBytes(out, &pos, "\n%%EOF\n");
    return pos;
}

// ============================================================================
// Part A: PNG Predictor (10..15) & TIFF Predictor 2 Reverse Filtering
// ============================================================================

test("lopdf_ref_01_png_predictor_all_5_filter_modes_none_sub_up_avg_paeth") {
    // 5 rows x 4 columns (bpp=1), exercising all 5 PNG filter types (0, 1, 2, 3, 4)
    const expected: [20]u8 = [
        10, 20, 30, 40,       // Row 0: None (0)
        10, 25, 35, 50,       // Row 1: Sub  (1)
        12, 27, 40, 60,       // Row 2: Up   (2)
        20, 30, 50, 70,       // Row 3: Avg  (3)
        18, 32, 48, 72,       // Row 4: Paeth(4)
    ];

    let encoded: [25]u8 = [0; 25];
    // Row 0: None (0)
    encoded[0] = 0;
    encoded[1] = 10; encoded[2] = 20; encoded[3] = 30; encoded[4] = 40;
    // Row 1: Sub (1) -> [10, 25-10=15, 35-25=10, 50-35=15]
    encoded[5] = 1;
    encoded[6] = 10; encoded[7] = 15; encoded[8] = 10; encoded[9] = 15;
    // Row 2: Up (2) -> [12-10=2, 27-25=2, 40-35=5, 60-50=10]
    encoded[10] = 2;
    encoded[11] = 2; encoded[12] = 2; encoded[13] = 5; encoded[14] = 10;
    // Row 3: Avg (3):
    //   c=0: pred=(0+12)/2=6 -> 20-6=14
    //   c=1: pred=(20+27)/2=23 -> 30-23=7
    //   c=2: pred=(30+40)/2=35 -> 50-35=15
    //   c=3: pred=(50+60)/2=55 -> 70-55=15
    encoded[15] = 3;
    encoded[16] = 14; encoded[17] = 7; encoded[18] = 15; encoded[19] = 15;
    // Row 4: Paeth (4):
    //   c=0: paeth(0,20,0)=20 -> 18-20 = 254 (-2 mod 256)
    //   c=1: paeth(18,30,20)=28? p=28, |28-18|=10, |28-30|=2, |28-20|=8 -> above=30 -> 32-30=2
    //   c=2: paeth(32,50,30)=52, |52-32|=20, |52-50|=2, |52-30|=22 -> above=50 -> 48-50=254
    //   c=3: paeth(48,70,50)=68, |68-48|=20, |68-70|=2, |68-50|=18 -> above=70 -> 72-70=2
    encoded[20] = 4;
    encoded[21] = 254; encoded[22] = 2; encoded[23] = 254; encoded[24] = 2;

    let out: [32]u8 = [0; 32];
    const n = applyPdfPredictorDecode(
        encoded[0..],
        PdfDecodeParms{ predictor: 15, columns: 4, colors: 1, bits_per_component: 8 },
        out[0..],
    ) catch unreachable;
    expect_eq(n, 20);
    expect_true(std.mem.eql(u8, out[0..n], expected[0..]));
}

test("lopdf_ref_02_tiff_predictor_2_multichannel_rgb") {
    // 2 rows x 3 pixels x 3 colors (RGB, bpp=3, row_bytes=9)
    const raw_diff: [18]u8 = [
        10, 20, 30,  5, 10, 15,  5, 5, 5,
        100, 50, 25,  10, 20, 30,  250, 250, 250,
    ];
    let out: [18]u8 = [0; 18];
    const n = applyPdfPredictorDecode(
        raw_diff[0..],
        PdfDecodeParms{ predictor: 2, columns: 3, colors: 3, bits_per_component: 8 },
        out[0..],
    ) catch unreachable;
    expect_eq(n, 18);
    expect_eq(out[0], 10);
    expect_eq(out[3], 15);
    expect_eq(out[6], 20);
    expect_eq(out[9], 100);
    expect_eq(out[12], 110);
    expect_eq(out[15], 104); // (110 + 250) mod 256 = 104
}

test("lopdf_ref_03_predictor_malformed_filter_and_truncation_guards") {
    // Invalid PNG filter byte 9 -> InvalidStream
    const bad_filter: [5]u8 = [9, 1, 2, 3, 4];
    let out: [16]u8 = [0; 16];
    let err1 = false;
    * = applyPdfPredictorDecode(bad_filter[0..], PdfDecodeParms{ predictor: 12, columns: 4 }, out[0..]) catch |e| {
        if (e == PdfParseError.InvalidStream) err1 = true;
    };
    expect_true(err1);

    // Incomplete row (only 3 bytes instead of 1+4=5) -> InvalidStream
    const trunc_row: [3]u8 = [2, 1, 2];
    let err2 = false;
    * = applyPdfPredictorDecode(trunc_row[0..], PdfDecodeParms{ predictor: 12, columns: 4 }, out[0..]) catch |e| {
        if (e == PdfParseError.InvalidStream) err2 = true;
    };
    expect_true(err2);

    // BufferTooSmall guard
    const valid_row: [5]u8 = [0, 1, 2, 3, 4];
    let tiny: [2]u8 = [0; 2];
    let err3 = false;
    * = applyPdfPredictorDecode(valid_row[0..], PdfDecodeParms{ predictor: 12, columns: 4 }, tiny[0..]) catch |e| {
        if (e == PdfParseError.BufferTooSmall) err3 = true;
    };
    expect_true(err3);
}

// ============================================================================
// Part B: PDF 1.5+ Cross-Reference Stream (/Type /XRef) & Object Stream (/Type /ObjStm)
// ============================================================================

test("lopdf_ref_04_pdf15_xref_stream_png_up_predictor_and_all_entry_types") {
    let pdf_buf: [4096]u8 = [0; 4096];
    const pdf_len = buildModernPdf15WithXrefStmAndObjStm(pdf_buf[0..]);
    const pdf = pdf_buf[0..pdf_len];

    expect_true(PdfInspector.isValidPdf(pdf));
    let entries: [32]PdfXrefEntry = [PdfXrefEntry{}; 32];
    const xref = PdfXrefTable.parse(pdf, entries[0..]) catch unreachable;

    expect_true(xref.trailer.is_xref_stream);
    expect_eq(xref.trailer.size, 8);
    expect_true(xref.trailer.has_root);
    expect_eq(xref.trailer.root.obj_num, 1);
    expect_true(xref.trailer.has_info);
    expect_eq(xref.trailer.info.obj_num, 5);

    // Type 0 (free)
    const e0 = xref.findEntry(0).?;
    expect_false(e0.in_use);
    expect_false(e0.is_compressed);

    // Type 2 (compressed in ObjStm 6)
    const e1 = xref.findEntry(1).?;
    expect_true(e1.in_use);
    expect_true(e1.is_compressed);
    expect_eq(e1.obj_stm_num, 6);
    expect_eq(e1.obj_stm_index, 0);

    const e5 = xref.findEntry(5).?;
    expect_true(e5.is_compressed);
    expect_eq(e5.obj_stm_num, 6);
    expect_eq(e5.obj_stm_index, 3);

    // Type 1 (normal uncompressed)
    const e4 = xref.findEntry(4).?;
    expect_true(e4.in_use);
    expect_false(e4.is_compressed);
    expect_true(e4.offset > 0);
}

test("lopdf_ref_05_objstm_transparent_metadata_pages_and_text_extraction") {
    let pdf_buf: [4096]u8 = [0; 4096];
    const pdf_len = buildModernPdf15WithXrefStmAndObjStm(pdf_buf[0..]);
    const pdf = pdf_buf[0..pdf_len];

    let entries: [32]PdfXrefEntry = [PdfXrefEntry{}; 32];
    const xref = PdfXrefTable.parse(pdf, entries[0..]) catch unreachable;

    // 1. Direct locateObjectBodyFromObjStm test
    let obj_out: [512]u8 = [0; 512];
    const olen = locateObjectBodyFromObjStm(pdf, xref, 6, 5, obj_out[0..]) catch unreachable;
    expect_true(std.mem.indexOf(u8, obj_out[0..olen], "Modern PDF 1.5 Title") != null);

    // 2. Transparent extractPdfMetadata (Info obj 5 is inside ObjStm 6!)
    let arena: [512]u8 = [0; 512];
    const meta = extractPdfMetadata(pdf, xref, arena[0..]) catch unreachable;
    expect_true(std.mem.eql(u8, meta.title.slice(arena[0..]), "Modern PDF 1.5 Title"));
    expect_true(std.mem.eql(u8, meta.author.slice(arena[0..]), "CP Stdlib"));

    // 3. Transparent collectPdfPages (Catalog 1, Pages 2, Page 3 are ALL inside ObjStm 6!)
    let stack: [16]PdfObjRef = [PdfObjRef{}; 16];
    let pages: [8]PdfPageInfo = [PdfPageInfo{}; 8];
    const n_pages = collectPdfPages(pdf, xref, stack[0..], pages[0..]) catch unreachable;
    expect_eq(n_pages, 1);
    expect_eq(pages[0].obj_ref.obj_num, 3);
    expect_eq(pages[0].media_box.urx, 612.0);
    expect_eq(pages[0].media_box.ury, 792.0);
    expect_true(pages[0].has_contents_ref);
    expect_eq(pages[0].contents_ref.obj_num, 4);

    // 4. Transparent extractPageText
    let scratch: [1024]u8 = [0; 1024];
    let text: [256]u8 = [0; 256];
    const tlen = extractPageText(pdf, xref, pages[0], scratch[0..], text[0..]) catch unreachable;
    expect_true(std.mem.eql(u8, text[0..tlen], "Hello PDF 1.5 ObjStm!"));
}

test("lopdf_ref_06_xref_stream_w0_zero_default_type1_and_multi_index") {
    // /W [0 2 1] omits field 0 (type defaults to 1 per ISO 32000-1 §7.5.8.2)
    // /Index [1 1 5 1] defines two disjoint subsections
    let out: [2048]u8 = [0; 2048];
    let pos: usize = 0;
    appendBytes(out[0..], &pos, "%PDF-1.5\n");
    const off1: usize = pos;
    appendBytes(out[0..], &pos, "1 0 obj\n<< /Type /Catalog /Pages 5 0 R >>\nendobj\n");
    const off5: usize = pos;
    appendBytes(out[0..], &pos, "5 0 obj\n<< /Type /Pages /Kids [] /Count 0 >>\nendobj\n");

    const off_xref: usize = pos;
    let raw_xref: [6]u8 = [
        @as(u8, @intCast((off1 >> 8) & 0xFF)), @as(u8, @intCast(off1 & 0xFF)), 0,
        @as(u8, @intCast((off5 >> 8) & 0xFF)), @as(u8, @intCast(off5 & 0xFF)), 0,
    ];
    appendBytes(out[0..], &pos, "6 0 obj\n<< /Type /XRef /Size 7 /W [0 2 1] /Index [1 1 5 1] /Root 1 0 R /Length 6 >>\nstream\n");
    appendBytes(out[0..], &pos, raw_xref[0..]);
    appendBytes(out[0..], &pos, "\nendstream\nendobj\nstartxref\n");
    appendUsizeAscii(out[0..], &pos, off_xref);
    appendBytes(out[0..], &pos, "\n%%EOF\n");

    let entries: [16]PdfXrefEntry = [PdfXrefEntry{}; 16];
    const xref = PdfXrefTable.parse(out[0..pos], entries[0..]) catch unreachable;
    expect_eq(xref.count, 2);
    expect_eq(xref.findEntry(1).?.offset, off1);
    expect_true(xref.findEntry(1).?.in_use);
    expect_eq(xref.findEntry(5).?.offset, off5);
    expect_true(xref.findEntry(2) == null);
}

test("lopdf_ref_07_hybrid_xref_trailer_with_xrefstm") {
    // Traditional xref + trailer with /XRefStm pointing to compressed object entries
    let out: [2048]u8 = [0; 2048];
    let pos: usize = 0;
    appendBytes(out[0..], &pos, "%PDF-1.5\n");
    const off1: usize = pos;
    appendBytes(out[0..], &pos, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");

    // XRefStm at obj 3 containing compressed entry for obj 2
    const off_xstm: usize = pos;
    const xraw: [4]u8 = [2, 0, 9, 0]; // obj 2 compressed in objstm 9 index 0
    appendBytes(out[0..], &pos, "3 0 obj\n<< /Type /XRef /Size 4 /W [1 2 1] /Index [2 1] /Length 4 >>\nstream\n");
    appendBytes(out[0..], &pos, xraw[0..]);
    appendBytes(out[0..], &pos, "\nendstream\nendobj\n");

    const off_xref: usize = pos;
    appendBytes(out[0..], &pos, "xref\n0 2\n0000000000 65535 f \n");
    let off_str: [10]u8 = [0; 10];
    const fs = std.fmt.bufPrint(off_str[0..], "{d:0>10}", .{off1}) catch unreachable;
    appendBytes(out[0..], &pos, fs);
    appendBytes(out[0..], &pos, " 00000 n \ntrailer\n<< /Size 4 /Root 1 0 R /XRefStm ");
    appendUsizeAscii(out[0..], &pos, off_xstm);
    appendBytes(out[0..], &pos, " >>\nstartxref\n");
    appendUsizeAscii(out[0..], &pos, off_xref);
    appendBytes(out[0..], &pos, "\n%%EOF\n");

    let entries: [16]PdfXrefEntry = [PdfXrefEntry{}; 16];
    const xref = PdfXrefTable.parse(out[0..pos], entries[0..]) catch unreachable;
    expect_true(xref.trailer.has_xref_stm);
    expect_eq(xref.trailer.xref_stm_offset, off_xstm);
    expect_eq(xref.findEntry(1).?.offset, off1);
    const e2 = xref.findEntry(2).?;
    expect_true(e2.is_compressed);
    expect_eq(e2.obj_stm_num, 9);
}

test("lopdf_ref_08_xref_stream_prev_incremental_chain_newest_wins") {
    let out: [2048]u8 = [0; 2048];
    let pos: usize = 0;
    appendBytes(out[0..], &pos, "%PDF-1.5\n");
    const off1_old: usize = pos;
    appendBytes(out[0..], &pos, "1 0 obj\n<< /Title (Old) >>\nendobj\n");

    // Old XRef stream at off_xref1
    const off_xref1: usize = pos;
    const x1: [4]u8 = [1, @as(u8, @intCast((off1_old >> 8) & 0xFF)), @as(u8, @intCast(off1_old & 0xFF)), 0];
    appendBytes(out[0..], &pos, "2 0 obj\n<< /Type /XRef /Size 3 /W [1 2 1] /Index [1 1] /Info 1 0 R /Length 4 >>\nstream\n");
    appendBytes(out[0..], &pos, x1[0..]);
    appendBytes(out[0..], &pos, "\nendstream\nendobj\n");

    // Incremental update: new obj 1 + new XRef stream with /Prev off_xref1
    const off1_new: usize = pos;
    appendBytes(out[0..], &pos, "1 0 obj\n<< /Title (Updated) >>\nendobj\n");
    const off_xref2: usize = pos;
    const x2: [4]u8 = [1, @as(u8, @intCast((off1_new >> 8) & 0xFF)), @as(u8, @intCast(off1_new & 0xFF)), 0];
    appendBytes(out[0..], &pos, "3 0 obj\n<< /Type /XRef /Size 4 /W [1 2 1] /Index [1 1] /Info 1 0 R /Prev ");
    appendUsizeAscii(out[0..], &pos, off_xref1);
    appendBytes(out[0..], &pos, " /Length 4 >>\nstream\n");
    appendBytes(out[0..], &pos, x2[0..]);
    appendBytes(out[0..], &pos, "\nendstream\nendobj\nstartxref\n");
    appendUsizeAscii(out[0..], &pos, off_xref2);
    appendBytes(out[0..], &pos, "\n%%EOF\n");

    let entries: [16]PdfXrefEntry = [PdfXrefEntry{}; 16];
    const xref = PdfXrefTable.parse(out[0..pos], entries[0..]) catch unreachable;
    expect_true(xref.trailer.has_prev);
    expect_eq(xref.trailer.xref_sections_parsed, 2);
    expect_eq(xref.findEntry(1).?.offset, off1_new); // Newest wins!

    let arena: [128]u8 = [0; 128];
    const meta = extractPdfMetadata(out[0..pos], xref, arena[0..]) catch unreachable;
    expect_true(std.mem.eql(u8, meta.title.slice(arena[0..]), "Updated"));
}

// ============================================================================
// Part C: Real lopdf/assets/ PDF Samples
// ============================================================================

fn readPdfAsset(alloc: std.mem.Allocator, path: []const u8): ([]u8)? {
    const f = std.fs.openFileAbsolute(path, .{}) catch return null;
    defer f.close();
    return f.readToEndAlloc(alloc, 2 * 1024 * 1024) catch return null;
}

test("lopdf_ref_09_real_asset_example_pdf") {
    const alloc = std.heap.page_allocator;
    const pdf = readPdfAsset(alloc, "/data/dev/cp/_ref/lopdf/assets/example.pdf") ?? return;
    defer alloc.free(pdf);

    expect_true(PdfInspector.isValidPdf(pdf));
    let entries: [64]PdfXrefEntry = [PdfXrefEntry{}; 64];
    const xref = PdfXrefTable.parse(pdf, entries[0..]) catch unreachable;
    expect_true(xref.trailer.has_root);

    let stack: [32]PdfObjRef = [PdfObjRef{}; 32];
    let pages: [16]PdfPageInfo = [PdfPageInfo{}; 16];
    const n_pages = collectPdfPages(pdf, xref, stack[0..], pages[0..]) catch unreachable;
    expect_eq(n_pages, 1);

    let scratch: [4096]u8 = [0; 4096];
    let text: [1024]u8 = [0; 1024];
    const tlen = extractPageText(pdf, xref, pages[0], scratch[0..], text[0..]) catch unreachable;
    expect_true(tlen > 0);
}

test("lopdf_ref_10_real_asset_incremental_pdf") {
    const alloc = std.heap.page_allocator;
    const pdf = readPdfAsset(alloc, "/data/dev/cp/_ref/lopdf/assets/Incremental.pdf") ?? return;
    defer alloc.free(pdf);

    expect_true(PdfInspector.isValidPdf(pdf));
    let entries: [128]PdfXrefEntry = [PdfXrefEntry{}; 128];
    const xref = PdfXrefTable.parse(pdf, entries[0..]) catch unreachable;
    expect_true(xref.trailer.has_root);
    expect_true(xref.trailer.xref_sections_parsed >= 2);
}

test("lopdf_ref_11_real_asset_unicode_pdf") {
    const alloc = std.heap.page_allocator;
    const pdf = readPdfAsset(alloc, "/data/dev/cp/_ref/lopdf/assets/unicode.pdf") ?? return;
    defer alloc.free(pdf);

    expect_true(PdfInspector.isValidPdf(pdf));
    let entries: [128]PdfXrefEntry = [PdfXrefEntry{}; 128];
    const xref = PdfXrefTable.parse(pdf, entries[0..]) catch unreachable;
    expect_true(xref.trailer.has_root);

    let stack: [32]PdfObjRef = [PdfObjRef{}; 32];
    let pages: [16]PdfPageInfo = [PdfPageInfo{}; 16];
    const n_pages = collectPdfPages(pdf, xref, stack[0..], pages[0..]) catch unreachable;
    expect_true(n_pages >= 1);
}

// ============================================================================
// Part D: Malformed XRef Stream Amplification Guard & 1,000-Iter Fuzzing
// ============================================================================

test("lopdf_ref_12_malformed_xref_stream_zero_width_and_amplification_guard") {
    // /W [0 0 0] must be rejected immediately (anti-infinite-loop guard per lopdf)
    const bad_w0 =
        "%PDF-1.5\n1 0 obj\n<< /Type /XRef /Size 100 /W [0 0 0] /Length 4 >>\nstream\n0000\nendstream\nendobj\nstartxref\n9\n%%EOF\n";
    let entries: [16]PdfXrefEntry = [PdfXrefEntry{}; 16];
    let rejected = false;
    * = PdfXrefTable.parse(bad_w0, entries[0..]) catch { rejected = true; };
    expect_true(rejected);

    // /Index claiming 10000 entries for a 4-byte stream -> rejected before loop
    const bad_amp =
        "%PDF-1.5\n1 0 obj\n<< /Type /XRef /Size 10000 /W [1 2 1] /Index [0 10000] /Length 4 >>\nstream\n\x01\x00\x09\x00\nendstream\nendobj\nstartxref\n9\n%%EOF\n";
    let amp_rejected = false;
    * = PdfXrefTable.parse(bad_amp, entries[0..]) catch { amp_rejected = true; };
    expect_true(amp_rejected);
}

test("lopdf_ref_13_1000_iter_malformed_xref_stream_predictor_objstm_fuzz") {
    let base_pdf: [4096]u8 = [0; 4096];
    const base_len = buildModernPdf15WithXrefStmAndObjStm(base_pdf[0..]);

    let mut_buf: [4096]u8 = [0; 4096];
    let entries: [32]PdfXrefEntry = [PdfXrefEntry{}; 32];
    let stack: [16]PdfObjRef = [PdfObjRef{}; 16];
    let pages: [8]PdfPageInfo = [PdfPageInfo{}; 8];
    let pred_out: [256]u8 = [0; 256];
    let obj_out: [256]u8 = [0; 256];

    let rng: u32 = 0xCAFE_BABE;
    let iter: usize = 0;
    while (iter < 1000) : (iter += 1) {
        let i: usize = 0;
        while (i < base_len) : (i += 1) {
            mut_buf[i] = base_pdf[i];
        }

        rng = rng *% 1664525 +% 1013904223;
        const mode: u32 = rng & 3;
        let test_len: usize = base_len;

        if (mode == 0) {
            rng = rng *% 1664525 +% 1013904223;
            test_len = @as(usize, rng % @as(u32, @intCast(base_len)));
        } else if (mode == 1) {
            let flips: usize = @as(usize, 1) + @as(usize, (rng >> 8) & 3);
            let f: usize = 0;
            while (f < flips) : (f += 1) {
                rng = rng *% 1664525 +% 1013904223;
                const pos: usize = @as(usize, rng % @as(u32, @intCast(base_len)));
                mut_buf[pos] ^= @as(u8, @intCast((rng >> 16) & 0xFF));
            }
        } else if (mode == 2) {
            // Fuzz PNG/TIFF predictor directly with random filter bytes & column counts
            const plen: usize = @as(usize, (rng >> 12) & 63);
            const cols: u32 = @as(u32, 1) + ((rng >> 20) & 7);
            const pred: u32 = if ((rng & 4) != 0) @as(u32, 2) else (@as(u32, 10) + ((rng >> 4) % 6));
            * = applyPdfPredictorDecode(
                mut_buf[0..plen],
                PdfDecodeParms{ predictor: pred, columns: cols, colors: 1, bits_per_component: 8 },
                pred_out[0..],
            ) catch {};
            continue;
        } else {
            // Corrupt XRef stream body bytes near the end
            if (base_len > 64) {
                const pos: usize = base_len - 64 + @as(usize, (rng >> 16) & 63);
                mut_buf[pos] ^= 0xFF;
            }
        }

        const pdf_slice = mut_buf[0..test_len];
        const xr = PdfXrefTable.parse(pdf_slice, entries[0..]) catch continue;
        * = collectPdfPages(pdf_slice, xr, stack[0..], pages[0..]) catch {};
        * = locateObjectBodyFromObjStm(pdf_slice, xr, 6, 1, obj_out[0..]) catch {};
    }
    expect_eq(iter, 1000);
}
