// Origin: Pure-CP Stdlib Test Suite (lib/doc/docx.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Upgraded Zero-Heap OOXML DOCX Engine:
//   1. `parseDocxRelsXml` & `buildDocxRelsXml` roundtrip + external hyperlink Target resolution
//   2. `parseDocxDocumentXml`: Heading1..6, list `<w:numPr>`, alignment `<w:jc>`, rich runs (bold/italic/u/strike/sz/tab/br/entities) & hyperlinks
//   3. Structured `<w:tbl>` parsing with multi-row cells, header bold & `gridSpan` column merge
//   4. `parseDocxCommentsXml` review comment extraction (`id`/`author`/`date`/unescaped text)
//   5. `buildDocxDocumentXml` + `buildDocxRelsXml` -> `parseDocxDocumentXml` full bidirectional roundtrip
//   6. `docxXmlToMarkdown` & `markdownToDocxXml` bidirectional conversion + `computeDocxWordStats` (English + CJK)
//   7. Malformed XML boundary guards & `cap.doc` namespace parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    DOCX_HYPERLINK_REL_TYPE,
    DocxError,
    DocxRel,
    parseDocxRelsXml,
    lookupDocxRelTarget,
    buildDocxRelsXml,
    DocxAlign,
    DocxRun,
    DocxRichParagraph,
    DocxTableCell,
    DocxTable,
    DocxBlockKind,
    DocxBlockRef,
    DocxComment,
    parseDocxDocumentXml,
    parseDocxCommentsXml,
    buildDocxDocumentXml,
    docxXmlToMarkdown,
    markdownToDocxXml,
    computeDocxWordStats,
} from "cap:doc";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("DOCX relationships (`document.xml.rels`) build -> parse roundtrip & Target lookup") {
    const rels_in: [2]DocxRel = [
        DocxRel{ id: "rId1", rel_type: DOCX_HYPERLINK_REL_TYPE, target: "https://cp-lang.org", is_external: true },
        DocxRel{ id: "rId2", rel_type: "http://schemas.openxmlformats.org/officeDocument/2006/relationships/header", target: "header1.xml", is_external: false },
    ];
    let xml: [1024]u8 = [0; 1024];
    const xlen = buildDocxRelsXml(rels_in[0..], xml[0..]) catch unreachable;

    let rels_out: [4]DocxRel = [DocxRel{}; 4];
    const cnt = parseDocxRelsXml(xml[0..xlen], rels_out[0..]) catch unreachable;
    expect_eq(cnt, 2);
    expect_true(rels_out[0].is_external);
    expect_true(!rels_out[1].is_external);
    expect_true(bytesEq(lookupDocxRelTarget(rels_out[0..cnt], "rId1") orelse "", "https://cp-lang.org"));
    expect_true(bytesEq(lookupDocxRelTarget(rels_out[0..cnt], "rId2") orelse "", "header1.xml"));
}

test("DOCX rich document parsing: Heading, list, alignment, styled runs, entities & hyperlinks") {
    const rels: [1]DocxRel = [
        DocxRel{ id: "rId5", rel_type: DOCX_HYPERLINK_REL_TYPE, target: "https://example.com/docs", is_external: true },
    ];
    const doc_xml = "<w:document><w:body>"
        ++ "<w:p><w:pPr><w:pStyle w:val=\"Heading2\"/><w:jc w:val=\"center\"/></w:pPr>"
        ++ "<w:r><w:rPr><w:b/><w:sz w:val=\"32\"/></w:rPr><w:t>Spec &amp; API</w:t></w:r></w:p>"
        ++ "<w:p><w:pPr><w:numPr><w:ilvl w:val=\"1\"/><w:numId w:val=\"2\"/></w:numPr></w:pPr>"
        ++ "<w:r><w:rPr><w:i/><w:u w:val=\"single\"/><w:strike/></w:rPr><w:t>Read</w:t><w:tab/><w:t>more</w:t></w:r>"
        ++ "<w:hyperlink r:id=\"rId5\"><w:r><w:t>Docs</w:t></w:r></w:hyperlink></w:p>"
        ++ "</w:body></w:document>";

    let blocks: [4]DocxBlockRef = [DocxBlockRef{}; 4];
    let paras: [4]DocxRichParagraph = [DocxRichParagraph{}; 4];
    let runs: [8]DocxRun = [DocxRun{}; 8];
    let tables: [2]DocxTable = [DocxTable{}; 2];
    let cells: [8]DocxTableCell = [DocxTableCell{}; 8];
    let slab: [512]u8 = [0; 512];

    const parsed = parseDocxDocumentXml(doc_xml, rels[0..], blocks[0..], paras[0..], runs[0..], tables[0..], cells[0..], slab[0..]) catch unreachable;
    expect_eq(parsed.paragraph_count, 2);
    expect_eq(parsed.run_count, 3);

    expect_eq(paras[0].heading_level, 2);
    expect_eq(paras[0].alignment, DocxAlign.center);
    expect_true(runs[0].bold);
    expect_eq(runs[0].font_size_half_pt, 32);
    expect_true(bytesEq(runs[0].text, "Spec & API"));

    expect_true(paras[1].is_list_item);
    expect_eq(paras[1].list_level, 1);
    expect_eq(paras[1].list_num_id, 2);
    expect_true(runs[1].italic && runs[1].underline && runs[1].strike);
    expect_true(bytesEq(runs[1].text, "Read\tmore"));
    expect_true(bytesEq(runs[2].text, "Docs"));
    expect_true(bytesEq(runs[2].link_url, "https://example.com/docs"));
}

test("DOCX structured table parsing with multi-row cells, header bold & gridSpan") {
    const tbl_xml = "<w:document><w:body><w:tbl>"
        ++ "<w:tr><w:tc><w:tcPr><w:gridSpan w:val=\"2\"/></w:tcPr><w:p><w:r><w:rPr><w:b/></w:rPr><w:t>Merged Header</w:t></w:r></w:p></w:tc></w:tr>"
        ++ "<w:tr><w:tc><w:p><w:r><w:t>Left</w:t></w:r></w:p></w:tc><w:tc><w:p><w:r><w:t>Right</w:t></w:r></w:p></w:tc></w:tr>"
        ++ "</w:tbl></w:body></w:document>";

    let blocks: [4]DocxBlockRef = [DocxBlockRef{}; 4];
    let paras: [4]DocxRichParagraph = [DocxRichParagraph{}; 4];
    let runs: [4]DocxRun = [DocxRun{}; 4];
    let tables: [2]DocxTable = [DocxTable{}; 2];
    let cells: [8]DocxTableCell = [DocxTableCell{}; 8];
    let slab: [256]u8 = [0; 256];
    let empty_rels: [0]DocxRel = [];

    const parsed = parseDocxDocumentXml(tbl_xml, empty_rels[0..], blocks[0..], paras[0..], runs[0..], tables[0..], cells[0..], slab[0..]) catch unreachable;
    expect_eq(parsed.table_count, 1);
    expect_eq(tables[0].row_count, 2);
    expect_eq(tables[0].col_count, 2);
    expect_eq(parsed.cell_count, 3);
    expect_eq(cells[0].grid_span, 2);
    expect_true(cells[0].is_header_bold);
    expect_true(bytesEq(cells[0].text, "Merged Header"));
    expect_eq(cells[1].row_index, 1);
    expect_eq(cells[1].col_index, 0);
    expect_true(bytesEq(cells[2].text, "Right"));
}

test("DOCX review comments (`word/comments.xml`) parsing") {
    const cm_xml = "<w:comments>"
        ++ "<w:comment w:id=\"1\" w:author=\"Alice\" w:date=\"2026-11-23T10:00:00Z\">"
        ++ "<w:p><w:r><w:t>Check &lt;section 2&gt;</w:t></w:r></w:p></w:comment>"
        ++ "</w:comments>";

    let comments: [4]DocxComment = [DocxComment{}; 4];
    let slab: [128]u8 = [0; 128];
    const cnt = parseDocxCommentsXml(cm_xml, comments[0..], slab[0..]) catch unreachable;
    expect_eq(cnt, 1);
    expect_true(bytesEq(comments[0].id, "1"));
    expect_true(bytesEq(comments[0].author, "Alice"));
    expect_true(bytesEq(comments[0].date, "2026-11-23T10:00:00Z"));
    expect_true(bytesEq(comments[0].text, "Check <section 2>"));
}

test("DOCX builder -> parser full bidirectional roundtrip (headings, links, styled runs & tables)") {
    const runs_in: [3]DocxRun = [
        DocxRun{ text: "Report Title", bold: true, font_size_half_pt: 36 },
        DocxRun{ text: "Visit ", italic: false },
        DocxRun{ text: "CP Docs", link_url: "https://cp-lang.org/doc" },
    ];
    const paras_in: [2]DocxRichParagraph = [
        DocxRichParagraph{ heading_level: 1, run_start: 0, run_count: 1 },
        DocxRichParagraph{ is_list_item: true, list_level: 0, run_start: 1, run_count: 2 },
    ];
    const cells_in: [4]DocxTableCell = [
        DocxTableCell{ row_index: 0, col_index: 0, text: "Key", is_header_bold: true },
        DocxTableCell{ row_index: 0, col_index: 1, text: "Val", is_header_bold: true },
        DocxTableCell{ row_index: 1, col_index: 0, text: "A" },
        DocxTableCell{ row_index: 1, col_index: 1, text: "100" },
    ];
    const tables_in: [1]DocxTable = [
        DocxTable{ row_count: 2, col_count: 2, cell_start: 0, cell_count: 4 },
    ];
    const blocks_in: [3]DocxBlockRef = [
        DocxBlockRef{ kind: DocxBlockKind.paragraph, index: 0 },
        DocxBlockRef{ kind: DocxBlockKind.paragraph, index: 1 },
        DocxBlockRef{ kind: DocxBlockKind.table, index: 0 },
    ];

    let rels_built: [4]DocxRel = [DocxRel{}; 4];
    let rel_cnt: usize = 0;
    let rid_slab: [64]u8 = [0; 64];
    let doc_xml: [4096]u8 = [0; 4096];

    const xlen = buildDocxDocumentXml(
        blocks_in[0..], paras_in[0..], runs_in[0..], tables_in[0..], cells_in[0..],
        rels_built[0..], *rel_cnt, rid_slab[0..], doc_xml[0..],
    ) catch unreachable;
    expect_eq(rel_cnt, 1);

    // Parse back and render to Markdown
    let b_out: [8]DocxBlockRef = [DocxBlockRef{}; 8];
    let p_out: [8]DocxRichParagraph = [DocxRichParagraph{}; 8];
    let r_out: [16]DocxRun = [DocxRun{}; 16];
    let t_out: [4]DocxTable = [DocxTable{}; 4];
    let c_out: [16]DocxTableCell = [DocxTableCell{}; 16];
    let slab: [512]u8 = [0; 512];
    let md_out: [1024]u8 = [0; 1024];

    const md_len = docxXmlToMarkdown(
        doc_xml[0..xlen], rels_built[0..rel_cnt],
        b_out[0..], p_out[0..], r_out[0..], t_out[0..], c_out[0..], slab[0..], md_out[0..],
    ) catch unreachable;
    const expected_md = "# **Report Title**\n\n- Visit [CP Docs](https://cp-lang.org/doc)\n\n| Key | Val |\n| --- | --- |\n| A | 100 |\n";
    expect_true(bytesEq(md_out[0..md_len], expected_md));
}

test("Markdown -> DOCX XML conversion & English + CJK word/char statistics") {
    const md = "# Hello 世界\n- **Bold** item with [link](https://cp-lang.org)\n";
    let rels: [4]DocxRel = [DocxRel{}; 4];
    let rcnt: usize = 0;
    let rid_slab: [64]u8 = [0; 64];
    let xml_out: [2048]u8 = [0; 2048];
    const xlen = markdownToDocxXml(md, rels[0..], *rcnt, rid_slab[0..], xml_out[0..]) catch unreachable;
    expect_eq(rcnt, 1);
    expect_true(bytesEq(rels[0].target, "https://cp-lang.org"));

    let scratch: [512]u8 = [0; 512];
    const ws = computeDocxWordStats(xml_out[0..xlen], scratch[0..]) catch unreachable;
    expect_eq(ws.paragraph_count, 2);
    expect_eq(ws.table_count, 0);
    expect_eq(ws.english_word_count, 5); // Hello, Bold, item, with, link
    expect_eq(ws.cjk_char_count, 2);     // 世, 界
}

test("DOCX malformed XML guards & cap.doc namespace parity") {
    let rels: [2]DocxRel = [DocxRel{}; 2];
    let got_bad_rels: bool = false;
    * = parseDocxRelsXml("not xml", rels[0..]) catch |e| {
        if (e == DocxError.InvalidRels) got_bad_rels = true;
    };
    expect_true(got_bad_rels);

    let comments: [2]DocxComment = [DocxComment{}; 2];
    let slab: [64]u8 = [0; 64];
    let got_bad_cm: bool = false;
    * = parseDocxCommentsXml("garbage", comments[0..], slab[0..]) catch |e| {
        if (e == DocxError.InvalidComments) got_bad_cm = true;
    };
    expect_true(got_bad_cm);

    expect_true(bytesEq(cap.doc.DOCX_HYPERLINK_REL_TYPE, DOCX_HYPERLINK_REL_TYPE));
}
