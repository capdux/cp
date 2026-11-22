// Origin: Pure-CP Stdlib Test Suite (lib/doc/pptx.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Upgraded Zero-Heap OOXML PPTX Engine:
//   1. EMU/pt conversion (`emuToPt`/`ptToEmu`) & `buildPptxPresentationXml` -> `parsePptxPresentationXml` 16:9 roundtrip
//   2. `parsePptxSlideXml`: placeholders, EMU geometry, multilevel bullets, bold/italic/sz/srgbClr & XML entity decoding
//   3. Native DrawingML `<a:tbl>` multi-row/multi-col table parsing
//   4. `buildPptxNotesXml` -> `parsePptxNotesXml` speaker notes roundtrip
//   5. `buildPptxSlideXml` -> `parsePptxSlideXml` full bidirectional roundtrip (shapes + styled runs + table)
//   6. `pptxSlideXmlToMarkdown` outline export & `computePptxSlideStats` (English + CJK + speaker notes)
//   7. Malformed XML guards & `cap.doc` namespace parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    EMU_PER_PT,
    PPTX_SLIDE_16_9_CX,
    PPTX_SLIDE_16_9_CY,
    PptxError,
    emuToPt,
    ptToEmu,
    PptxSlideSize,
    PptxSlideRef,
    parsePptxPresentationXml,
    buildPptxPresentationXml,
    PptxPlaceholderKind,
    PptxRectEmu,
    PptxTextRun,
    PptxTextParagraph,
    PptxShape,
    PptxTableCell,
    PptxTable,
    parsePptxSlideXml,
    parsePptxNotesXml,
    buildPptxSlideXml,
    buildPptxNotesXml,
    pptxSlideXmlToMarkdown,
    computePptxSlideStats,
} from "cap:doc";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("PPTX EMU/pt conversion & 16:9 presentation.xml build -> parse roundtrip") {
    expect_eq(EMU_PER_PT, 12700);
    expect_eq(emuToPt(PPTX_SLIDE_16_9_CX), 960.0);
    expect_eq(emuToPt(PPTX_SLIDE_16_9_CY), 540.0);
    expect_eq(ptToEmu(72.0), 914400);

    const slides_in: [3]PptxSlideRef = [
        PptxSlideRef{ id: 256, r_id: "rId1" },
        PptxSlideRef{ id: 257, r_id: "rId2" },
        PptxSlideRef{ id: 258, r_id: "rId3" },
    ];
    let xml: [1024]u8 = [0; 1024];
    const xlen = buildPptxPresentationXml(
        PptxSlideSize{ cx: PPTX_SLIDE_16_9_CX, cy: PPTX_SLIDE_16_9_CY, size_type: "screen16x9" },
        slides_in[0..],
        xml[0..],
    ) catch unreachable;

    let slides_out: [4]PptxSlideRef = [PptxSlideRef{ id: 0, r_id: "" }; 4];
    const info = parsePptxPresentationXml(xml[0..xlen], slides_out[0..]) catch unreachable;
    expect_true(info.has_slide_size);
    expect_eq(info.slide_size.cx, PPTX_SLIDE_16_9_CX);
    expect_eq(info.slide_size.cy, PPTX_SLIDE_16_9_CY);
    expect_eq(info.slide_count, 3);
    expect_eq(slides_out[0].id, 256);
    expect_true(bytesEq(slides_out[2].r_id, "rId3"));
}

test("PPTX rich slide parsing: placeholders, EMU geometry, bullets, color, font-size & entities") {
    const slide_xml = "<p:sld><p:cSld><p:spTree>"
        ++ "<p:sp><p:nvSpPr><p:cNvPr id=\"2\" name=\"Title\"/><p:nvPr><p:ph type=\"ctrTitle\"/></p:nvPr></p:nvSpPr>"
        ++ "<p:spPr><a:xfrm><a:off x=\"457200\" y=\"274638\"/><a:ext cx=\"8229600\" cy=\"1143000\"/></a:xfrm></p:spPr>"
        ++ "<p:txBody><a:p><a:pPr algn=\"ctr\"/><a:r><a:rPr b=\"1\" sz=\"4400\"><a:solidFill><a:srgbClr val=\"1F4E79\"/></a:solidFill></a:rPr>"
        ++ "<a:t>Q4 &amp; Roadmap</a:t></a:r></a:p></p:txBody></p:sp>"
        ++ "<p:sp><p:nvSpPr><p:cNvPr id=\"3\" name=\"Body\"/><p:nvPr><p:ph type=\"body\"/></p:nvPr></p:nvSpPr>"
        ++ "<p:txBody><a:p><a:pPr lvl=\"1\"/><a:r><a:rPr i=\"1\" sz=\"2000\"/><a:t>Sub-item</a:t></a:r></a:p></p:txBody></p:sp>"
        ++ "</p:spTree></p:cSld></p:sld>";

    let shapes: [4]PptxShape = [PptxShape{}; 4];
    let paras: [8]PptxTextParagraph = [PptxTextParagraph{}; 8];
    let runs: [8]PptxTextRun = [PptxTextRun{}; 8];
    let tables: [2]PptxTable = [PptxTable{}; 2];
    let cells: [8]PptxTableCell = [PptxTableCell{}; 8];
    let slab: [256]u8 = [0; 256];

    const parsed = parsePptxSlideXml(slide_xml, shapes[0..], paras[0..], runs[0..], tables[0..], cells[0..], slab[0..]) catch unreachable;
    expect_eq(parsed.shape_count, 2);
    expect_eq(shapes[0].placeholder, PptxPlaceholderKind.ctr_title);
    expect_true(shapes[0].has_rect);
    expect_eq(shapes[0].rect.x, 457200);
    expect_eq(shapes[0].rect.cx, 8229600);
    expect_true(bytesEq(paras[0].alignment, "ctr"));
    expect_true(runs[0].bold);
    expect_eq(runs[0].font_size_centipt, 4400);
    expect_true(runs[0].has_color);
    expect_true(bytesEq(runs[0].color_hex[0..6], "1F4E79"));
    expect_true(bytesEq(runs[0].text, "Q4 & Roadmap"));

    expect_eq(shapes[1].placeholder, PptxPlaceholderKind.body);
    expect_eq(paras[1].level, 1);
    expect_true(runs[1].italic);
    expect_true(bytesEq(runs[1].text, "Sub-item"));
}

test("PPTX native DrawingML `<a:tbl>` table parsing") {
    const tbl_slide = "<p:sld><p:cSld><p:spTree><p:graphicFrame><a:graphic><a:graphicData><a:tbl>"
        ++ "<a:tr><a:tc><a:txBody><a:p><a:r><a:t>Metric</a:t></a:r></a:p></a:txBody></a:tc>"
        ++ "<a:tc><a:txBody><a:p><a:r><a:t>Value</a:t></a:r></a:p></a:txBody></a:tc></a:tr>"
        ++ "<a:tr><a:tc><a:txBody><a:p><a:r><a:t>Latency</a:t></a:r></a:p></a:txBody></a:tc>"
        ++ "<a:tc><a:txBody><a:p><a:r><a:t>1.2ms</a:t></a:r></a:p></a:txBody></a:tc></a:tr>"
        ++ "</a:tbl></a:graphicData></a:graphic></p:graphicFrame></p:spTree></p:cSld></p:sld>";

    let shapes: [2]PptxShape = [PptxShape{}; 2];
    let paras: [4]PptxTextParagraph = [PptxTextParagraph{}; 4];
    let runs: [4]PptxTextRun = [PptxTextRun{}; 4];
    let tables: [2]PptxTable = [PptxTable{}; 2];
    let cells: [8]PptxTableCell = [PptxTableCell{}; 8];
    let slab: [256]u8 = [0; 256];

    const parsed = parsePptxSlideXml(tbl_slide, shapes[0..], paras[0..], runs[0..], tables[0..], cells[0..], slab[0..]) catch unreachable;
    expect_eq(parsed.table_count, 1);
    expect_eq(tables[0].row_count, 2);
    expect_eq(tables[0].col_count, 2);
    expect_eq(parsed.cell_count, 4);
    expect_true(bytesEq(cells[0].text, "Metric"));
    expect_true(bytesEq(cells[3].text, "1.2ms"));
}

test("PPTX speaker notes (`notesSlideN.xml`) build -> parse roundtrip") {
    let notes_xml: [1024]u8 = [0; 1024];
    const nlen = buildPptxNotesXml("Explain Q4 & highlight <latency> wins.", notes_xml[0..]) catch unreachable;
    let out_text: [256]u8 = [0; 256];
    const tlen = parsePptxNotesXml(notes_xml[0..nlen], out_text[0..]) catch unreachable;
    expect_true(bytesEq(out_text[0..tlen], "Explain Q4 & highlight <latency> wins."));
}

test("PPTX slide builder -> parser -> Markdown outline & CJK/English/Notes stats closed loop") {
    const runs_in: [3]PptxTextRun = [
        PptxTextRun{ text: "CP 架构", bold: true, font_size_centipt: 3600 },
        PptxTextRun{ text: "Zero-heap", bold: true },
        PptxTextRun{ text: "Fast", italic: true },
    ];
    const paras_in: [3]PptxTextParagraph = [
        PptxTextParagraph{ level: 0, run_start: 0, run_count: 1 },
        PptxTextParagraph{ level: 0, run_start: 1, run_count: 1 },
        PptxTextParagraph{ level: 1, run_start: 2, run_count: 1 },
    ];
    const shapes_in: [2]PptxShape = [
        PptxShape{
            placeholder: PptxPlaceholderKind.title,
            rect: PptxRectEmu{ x: 100, y: 200, cx: 5000000, cy: 800000 },
            has_rect: true,
            para_start: 0,
            para_count: 1,
        },
        PptxShape{
            placeholder: PptxPlaceholderKind.body,
            para_start: 1,
            para_count: 2,
        },
    ];
    const cells_in: [4]PptxTableCell = [
        PptxTableCell{ row_index: 0, col_index: 0, text: "A" },
        PptxTableCell{ row_index: 0, col_index: 1, text: "B" },
        PptxTableCell{ row_index: 1, col_index: 0, text: "1" },
        PptxTableCell{ row_index: 1, col_index: 1, text: "2" },
    ];
    const tables_in: [1]PptxTable = [
        PptxTable{ row_count: 2, col_count: 2, cell_start: 0, cell_count: 4 },
    ];

    let sld_xml: [4096]u8 = [0; 4096];
    const slen = buildPptxSlideXml(shapes_in[0..], paras_in[0..], runs_in[0..], tables_in[0..], cells_in[0..], sld_xml[0..]) catch unreachable;

    let notes_xml: [1024]u8 = [0; 1024];
    const nlen = buildPptxNotesXml("Demo note here", notes_xml[0..]) catch unreachable;

    // Export Markdown outline
    let s_sh: [4]PptxShape = [PptxShape{}; 4];
    let s_p: [8]PptxTextParagraph = [PptxTextParagraph{}; 8];
    let s_r: [8]PptxTextRun = [PptxTextRun{}; 8];
    let s_t: [2]PptxTable = [PptxTable{}; 2];
    let s_c: [8]PptxTableCell = [PptxTableCell{}; 8];
    let slab: [512]u8 = [0; 512];
    let n_scr: [256]u8 = [0; 256];
    let md_out: [1024]u8 = [0; 1024];

    const md_len = pptxSlideXmlToMarkdown(
        1, sld_xml[0..slen], notes_xml[0..nlen],
        s_sh[0..], s_p[0..], s_r[0..], s_t[0..], s_c[0..], slab[0..], n_scr[0..], md_out[0..],
    ) catch unreachable;
    const exp_md = "## Slide 1: CP 架构\n- **Zero-heap**\n  - *Fast*\n| A | B |\n| --- | --- |\n| 1 | 2 |\n> Notes: Demo note here\n";
    expect_true(bytesEq(md_out[0..md_len], exp_md));
}

test("PPTX computePptxSlideStats counts shapes, tables, English words, CJK chars & notes") {
    const slide_xml = "<p:sld><p:cSld><p:spTree>"
        ++ "<p:sp><p:txBody><a:p><a:r><a:t>Hello 世界 CP</a:t></a:r></a:p></p:txBody></p:sp>"
        ++ "<p:graphicFrame><a:graphic><a:graphicData><a:tbl><a:tr><a:tc><a:txBody><a:p><a:r><a:t>Cell</a:t></a:r></a:p></a:txBody></a:tc></a:tr></a:tbl></a:graphicData></a:graphic></p:graphicFrame>"
        ++ "</p:spTree></p:cSld></p:sld>";
    let notes_xml: [512]u8 = [0; 512];
    const nlen = buildPptxNotesXml("Three speaker words", notes_xml[0..]) catch unreachable;

    let stat_scr: [512]u8 = [0; 512];
    const stats = computePptxSlideStats(slide_xml, notes_xml[0..nlen], stat_scr[0..]) catch unreachable;
    expect_eq(stats.shape_count, 1);
    expect_eq(stats.table_count, 1);
    expect_eq(stats.english_word_count, 3); // Hello, CP, Cell
    expect_eq(stats.cjk_char_count, 2);     // 世, 界
    expect_eq(stats.notes_word_count, 3);   // Three, speaker, words
}

test("PPTX malformed XML guards & cap.doc namespace parity") {
    let slides: [2]PptxSlideRef = [PptxSlideRef{ id: 0, r_id: "" }; 2];
    let got_bad_pres: bool = false;
    * = parsePptxPresentationXml("not xml", slides[0..]) catch |e| {
        if (e == PptxError.InvalidPresentation) got_bad_pres = true;
    };
    expect_true(got_bad_pres);

    let out: [32]u8 = [0; 32];
    let got_bad_notes: bool = false;
    * = parsePptxNotesXml("<p:sld/>", out[0..]) catch |e| {
        if (e == PptxError.InvalidNotes) got_bad_notes = true;
    };
    expect_true(got_bad_notes);

    expect_eq(cap.doc.EMU_PER_PT, 12700);
    expect_eq(cap.doc.emuToPt(127000), 10.0);
}
