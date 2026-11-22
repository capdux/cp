// Source: cp/cap/test/test_pptx_production_deep.cp (Production deep tests for cap:pptx)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PPTX_MAIN_CONTENT_TYPE,
    EMU_PER_INCH,
    PptxSlideSize,
    PptxSlideRef,
    PptxSlideStats,
    PptxSlideRefIter,
    PptxParagraphIter,
    isPresentationXml,
    isSlideXml,
    isNotesSlideXml,
    parsePptxSlideSize,
    extractSlideText,
    summarizeSlide,
} from "cap:doc";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. presentation.xml 16:9 widescreen EMU size conversion and <p:sldId> manifest iteration", () => {
    const pres_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" +
        "<p:presentation xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" " +
        "  xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" " +
        "  xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">" +
        "  <p:sldMasterIdLst><p:sldMasterId id=\"2147483648\" r:id=\"rId1\"/></p:sldMasterIdLst>" +
        "  <p:sldIdLst>" +
        "    <p:sldId id=\"256\" r:id=\"rId2\"/>" +
        "    <p:sldId id=\"257\" r:id=\"rId3\"/>" +
        "    <p:sldId id=\"258\" r:id=\"rId4\"/>" +
        "  </p:sldIdLst>" +
        "  <p:sldSz cx=\"12192000\" cy=\"6858000\" type=\"screen16x9\"/>" +
        "  <p:notesSz cx=\"6858000\" cy=\"9144000\"/>" +
        "</p:presentation>";

    expect_eq(isPresentationXml(pres_xml), true);
    expect_eq(isSlideXml(pres_xml), false);
    expect_eq(isNotesSlideXml(pres_xml), false);

    const sz = parsePptxSlideSize(pres_xml) ?? unreachable;
    expect_eq(sz.cx, 12192000);
    expect_eq(sz.cy, 6858000);
    expect_eq(bytesEq(sz.size_type, "screen16x9"), true);
    // 12192000 / 914400 = 13.3333... inches; 6858000 / 914400 = 7.5 inches
    const w_in = sz.widthInches();
    const h_in = sz.heightInches();
    expect_eq(w_in > 13.33 && w_in < 13.34, true);
    expect_eq(h_in > 7.49 && h_in < 7.51, true);
    expect_eq(EMU_PER_INCH > 914399.0 && EMU_PER_INCH < 914401.0, true);

    let it = PptxSlideRefIter.init(pres_xml);
    const s0 = it.next() ?? unreachable;
    expect_eq(s0.id, 256);
    expect_eq(bytesEq(s0.r_id, "rId2"), true);

    const s1 = it.next() ?? unreachable;
    expect_eq(s1.id, 257);
    expect_eq(bytesEq(s1.r_id, "rId3"), true);

    const s2 = it.next() ?? unreachable;
    expect_eq(s2.id, 258);
    expect_eq(bytesEq(s2.r_id, "rId4"), true);

    expect_eq(it.next() == null, true);
});

test("2. slide1.xml multi-shape, paragraphs, <a:br/>, <a:tab/>, and XML entity text extraction", () => {
    const slide_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" +
        "<p:sld xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" " +
        "  xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">" +
        "  <p:cSld><p:spTree>" +
        "    <p:sp><p:txBody>" +
        "      <a:p><a:pPr algn=\"ctr\"/><a:pBdr/><a:r><a:rPr b=\"1\"/><a:t>Q4 &amp; Roadmap</a:t></a:r></a:p>" +
        "    </p:txBody></p:sp>" +
        "    <p:sp><p:txBody>" +
        "      <a:p><a:r><a:t>Item1</a:t><a:tab/><a:t>&lt;OK&gt;</a:t><a:br/><a:t>&quot;Quoted&quot; &apos;S&apos;</a:t></a:r></a:p>" +
        "      <a:p><a:r><a:t>&#65;&#x4E2D;&#25991;&#x1F680;</a:t></a:r></a:p>" +
        "    </p:txBody></p:sp>" +
        "  </p:spTree></p:cSld>" +
        "</p:sld>";

    expect_eq(isSlideXml(slide_xml), true);

    // Verify PptxParagraphIter accurately distinguishes <a:p> from <a:pPr> / <a:pBdr>
    let pit = PptxParagraphIter.init(slide_xml);
    let pcount: usize = 0;
    while (p := pit.next()) {
        * = p;
        pcount += 1;
    }
    expect_eq(pcount, 3);

    let buf: [256]u8 = std.mem.zeroes([256]u8);
    const n = extractSlideText(slide_xml, buf[0..]) ?? unreachable;
    const expected = "Q4 & Roadmap\nItem1\t<OK>\n\"Quoted\" 'S'\nA中文🚀";
    expect_eq(bytesEq(buf[0..n], expected), true);
});

test("3. notesSlide1.xml speaker notes detection and pure text extraction", () => {
    const notes_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +
        "<p:notes xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" " +
        "  xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">" +
        "  <p:cSld><p:spTree>" +
        "    <p:sp><p:txBody>" +
        "      <a:p><a:r><a:t>Remember to demo live benchmark.</a:t></a:r></a:p>" +
        "      <a:p><a:r><a:t>Pause for Q&amp;A.</a:t></a:r></a:p>" +
        "    </p:txBody></p:sp>" +
        "  </p:spTree></p:cSld>" +
        "</p:notes>";

    expect_eq(isNotesSlideXml(notes_xml), true);
    expect_eq(isSlideXml(notes_xml), false);
    expect_eq(isPresentationXml(notes_xml), false);

    // Prefix-similar <p:notesMaster> must NOT match isNotesSlideXml
    expect_eq(isNotesSlideXml("<p:notesMaster/>"), false);

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const n = extractSlideText(notes_xml, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n], "Remember to demo live benchmark.\nPause for Q&A."), true);
});

test("4. summarizeSlide shape, paragraph, text_run, table, and picture statistics", () => {
    const slide_xml =
        "<p:sld xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" " +
        "  xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">" +
        "  <p:cSld><p:spTree>" +
        "    <p:sp><p:spPr/><p:txBody>" +
        "      <a:p><a:pPr/><a:r><a:rPr/><a:t>Title</a:t></a:r></a:p>" +
        "      <a:p><a:r><a:t>Sub1</a:t></a:r><a:r><a:t>Sub2</a:t></a:r></a:p>" +
        "    </p:txBody></p:sp>" +
        "    <p:sp><p:spPr/><p:txBody><a:p><a:r><a:t>Footer</a:t></a:r></a:p></p:txBody></p:sp>" +
        "    <p:pic><p:nvPicPr><p:cNvPicPr><a:picLocks/></p:cNvPicPr></p:nvPicPr></p:pic>" +
        "    <p:pic><p:spPr/></p:pic>" +
        "    <p:graphicFrame><a:graphic><a:graphicData><a:tbl>" +
        "      <a:tblPr/><a:tblGrid/>" +
        "      <a:tr><a:tc><a:txBody><a:p><a:r><a:t>C1</a:t></a:r></a:p></a:txBody></a:tc>" +
        "            <a:tc><a:txBody><a:p><a:r><a:t>C2</a:t></a:r></a:p></a:txBody></a:tc></a:tr>" +
        "    </a:tbl></a:graphicData></a:graphic></p:graphicFrame>" +
        "  </p:spTree></p:cSld>" +
        "</p:sld>";

    const stats = summarizeSlide(slide_xml);
    expect_eq(stats.shape_count, 2);
    expect_eq(stats.paragraph_count, 5);
    expect_eq(stats.text_run_count, 6);
    expect_eq(stats.table_count, 1);
    expect_eq(stats.pic_count, 2);
});

test("5. Format detection edge cases (BOM, unprefixed tags, prefix-similar rejection, Content-Type)", () => {
    expect_eq(bytesEq(PPTX_MAIN_CONTENT_TYPE, "application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"), true);

    expect_eq(isPresentationXml("\xEF\xBB\xBF<presentation><sldSz cx=\"9144000\" cy=\"6858000\"/></presentation>"), true);
    expect_eq(isSlideXml("\xEF\xBB\xBF<!-- comment --><sld/>"), true);
    expect_eq(isNotesSlideXml("<notes/>"), true);

    // Prefix-similar OOXML tags must NOT false-match
    expect_eq(isPresentationXml("<p:presentationPr/>"), false);
    expect_eq(isSlideXml("<p:sldMaster/>"), false);
    expect_eq(isSlideXml("<p:sldLayout/>"), false);
    expect_eq(isNotesSlideXml("<p:notesMaster/>"), false);

    // 4:3 standard slide size without `type` attribute
    const sz = parsePptxSlideSize("<presentation><sldSz cx=\"9144000\" cy=\"6858000\"/></presentation>") ?? unreachable;
    expect_eq(sz.widthInches() > 9.99 && sz.widthInches() < 10.01, true);
    expect_eq(sz.heightInches() > 7.49 && sz.heightInches() < 7.51, true);
    expect_eq(bytesEq(sz.size_type, ""), true);
});

test("6. Non-PPTX XML and buffer overflow fault tolerance", () => {
    const slide_xml =
        "<p:sld><p:cSld><p:spTree><p:sp><p:txBody>" +
        "<a:p><a:r><a:t>Hello</a:t></a:r></a:p>" +
        "<a:p><a:r><a:t>Slide</a:t></a:r></a:p>" +
        "</p:txBody></p:sp></p:spTree></p:cSld></p:sld>";

    // Exact fit: "Hello\nSlide" = 11 bytes
    let exact_buf: [11]u8 = std.mem.zeroes([11]u8);
    const n = extractSlideText(slide_xml, exact_buf[0..]) ?? unreachable;
    expect_eq(n, 11);
    expect_eq(bytesEq(exact_buf[0..n], "Hello\nSlide"), true);

    // 1 byte short -> null
    let short_buf: [10]u8 = std.mem.zeroes([10]u8);
    expect_eq(extractSlideText(slide_xml, short_buf[0..]) == null, true);

    // Overflow on paragraph separator '\n' -> null
    let sep_buf: [5]u8 = std.mem.zeroes([5]u8);
    expect_eq(extractSlideText(slide_xml, sep_buf[0..]) == null, true);

    // Non-PPTX XML rejections
    let ok_buf: [64]u8 = std.mem.zeroes([64]u8);
    expect_eq(extractSlideText("<w:document><w:body/></w:document>", ok_buf[0..]) == null, true);
    expect_eq(extractSlideText("", ok_buf[0..]) == null, true);
    expect_eq(parsePptxSlideSize("<html></html>") == null, true);
    expect_eq(parsePptxSlideSize("<p:presentation><p:sldSz cx=\"bad\" cy=\"100\"/></p:presentation>") == null, true);

    const empty_stats = summarizeSlide("<w:document/>");
    expect_eq(empty_stats.shape_count, 0);
    expect_eq(empty_stats.paragraph_count, 0);
    expect_eq(empty_stats.text_run_count, 0);
    expect_eq(empty_stats.table_count, 0);
    expect_eq(empty_stats.pic_count, 0);

    // Empty self-closing <p:sldIdLst/>
    let empty_refs = PptxSlideRefIter.init("<p:presentation><p:sldIdLst/></p:presentation>");
    expect_eq(empty_refs.next() == null, true);
});
