// Source: cp/cap/test/test_docx_production_deep.cp (Production deep tests for cap:docx)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DOCX_MAIN_CONTENT_TYPE,
    DocxCoreProps,
    DocxParagraph,
    DocxStats,
    DocxParagraphIter,
    isWordDocumentXml,
    isDocxContentTypes,
    parseDocxCoreProps,
    extractDocxParagraphText,
    extractDocxText,
    summarizeDocx,
} from "cap:docx";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. Format detection: isWordDocumentXml and isDocxContentTypes", () => {
    const valid_doc = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<!-- Word doc -->\n<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\"><w:body/></w:document>";
    expect_eq(isWordDocumentXml(valid_doc), true);

    // UTF-8 BOM + unprefixed <document>
    const bom_doc = "\xEF\xBB\xBF<document xmlns=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\"><body/></document>";
    expect_eq(isWordDocumentXml(bom_doc), true);

    // Must NOT false-match prefix-similar tags or non-DOCX XML
    expect_eq(isWordDocumentXml("<w:documentSettings/>"), false);
    expect_eq(isWordDocumentXml("<html><body>not docx</body></html>"), false);
    expect_eq(isWordDocumentXml(""), false);

    // [Content_Types].xml detection
    const ct_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"><Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/></Types>";
    expect_eq(isDocxContentTypes(ct_xml), true);
    expect_eq(bytesEq(DOCX_MAIN_CONTENT_TYPE, "application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"), true);

    const xlsx_ct = "<Types><Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/></Types>";
    expect_eq(isDocxContentTypes(xlsx_ct), false);
});

test("2. Multi-paragraph Heading1 + body + tab + br/cr + prefix disambiguation", () => {
    const doc_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +
        "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">" +
        "<w:body>" +
        "<w:p w:rsidR=\"001\">" +
        "  <w:pPr><w:pStyle w:val=\"Heading1\"/><w:pBdr/></w:pPr>" +
        "  <w:r><w:rPr><w:b/></w:rPr><w:t>Chapter 1</w:t></w:r>" +
        "</w:p>" +
        "<w:p>" +
        "  <w:pPr><w:pStyle w:val=\"Normal\"/></w:pPr>" +
        "  <w:r><w:t xml:space=\"preserve\">ColA</w:t><w:tab/><w:t>ColB</w:t><w:br/><w:t>Line2</w:t><w:cr/><w:t>Line3</w:t></w:r>" +
        "</w:p>" +
        "<w:p/>" +
        "</w:body>" +
        "</w:document>";

    let it = DocxParagraphIter.init(doc_xml);
    const p0 = it.next() ?? unreachable;
    expect_eq(bytesEq(p0.style, "Heading1"), true);
    expect_eq(p0.is_heading, true);

    const p1 = it.next() ?? unreachable;
    expect_eq(bytesEq(p1.style, "Normal"), true);
    expect_eq(p1.is_heading, false);

    const p2 = it.next() ?? unreachable;
    expect_eq(bytesEq(p2.style, ""), true);
    expect_eq(p2.is_heading, false);

    expect_eq(it.next() == null, true);

    // Single paragraph extraction
    let pbuf: [128]u8 = std.mem.zeroes([128]u8);
    const plen = extractDocxParagraphText(p1.raw_xml, pbuf[0..]) ?? unreachable;
    expect_eq(bytesEq(pbuf[0..plen], "ColA\tColB\nLine2\nLine3"), true);

    // Full document extraction (paragraphs joined by '\n')
    let dbuf: [256]u8 = std.mem.zeroes([256]u8);
    const dlen = extractDocxText(doc_xml, dbuf[0..]) ?? unreachable;
    expect_eq(bytesEq(dbuf[0..dlen], "Chapter 1\nColA\tColB\nLine2\nLine3\n"), true);
});

test("3. XML named entities and numeric entities (&#65;, &#x4E2D;, &#x1F600;) + CDATA", () => {
    const doc_xml =
        "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">" +
        "<w:body>" +
        "<w:p><w:r>" +
        "<w:t>A &amp; B &lt; C &gt; &quot;D&quot; &apos;E&apos; </w:t>" +
        "<w:t>&#65;&#x4E2D;&#25991;&#x1F600;</w:t>" +
        "<w:t><![CDATA[ <raw&cdata> ]]></w:t>" +
        "</w:r></w:p>" +
        "</w:body>" +
        "</w:document>";

    let buf: [256]u8 = std.mem.zeroes([256]u8);
    const n = extractDocxText(doc_xml, buf[0..]) ?? unreachable;
    const expected = "A & B < C > \"D\" 'E' A中文😀 <raw&cdata> ";
    expect_eq(bytesEq(buf[0..n], expected), true);
});

test("4. docProps/core.xml full-field metadata parsing with dc:/cp:/dcterms: namespaces", () => {
    const core_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" +
        "<cp:coreProperties " +
        "  xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" " +
        "  xmlns:dc=\"http://purl.org/dc/elements/1.1/\" " +
        "  xmlns:dcterms=\"http://purl.org/dc/terms/\" " +
        "  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">" +
        "  <dc:title>CP Architecture Whitepaper</dc:title>" +
        "  <dc:subject>Systems Programming</dc:subject>" +
        "  <dc:creator>CP Core Team</dc:creator>" +
        "  <cp:keywords>cp, ooxml, zero-alloc</cp:keywords>" +
        "  <dc:description>Zero allocation DOCX parser specification</dc:description>" +
        "  <cp:lastModifiedBy>Build Bot</cp:lastModifiedBy>" +
        "  <cp:revision>7</cp:revision>" +
        "  <dcterms:created xsi:type=\"dcterms:W3CDTF\">2026-11-20T08:30:00Z</dcterms:created>" +
        "  <dcterms:modified xsi:type=\"dcterms:W3CDTF\">2026-11-21T19:00:00Z</dcterms:modified>" +
        "</cp:coreProperties>";

    const props = parseDocxCoreProps(core_xml) ?? unreachable;
    expect_eq(bytesEq(props.title, "CP Architecture Whitepaper"), true);
    expect_eq(bytesEq(props.subject, "Systems Programming"), true);
    expect_eq(bytesEq(props.creator, "CP Core Team"), true);
    expect_eq(bytesEq(props.keywords, "cp, ooxml, zero-alloc"), true);
    expect_eq(bytesEq(props.description, "Zero allocation DOCX parser specification"), true);
    expect_eq(bytesEq(props.last_modified_by, "Build Bot"), true);
    expect_eq(bytesEq(props.revision, "7"), true);
    expect_eq(bytesEq(props.created, "2026-11-20T08:30:00Z"), true);
    expect_eq(bytesEq(props.modified, "2026-11-21T19:00:00Z"), true);

    // Self-closing coreProperties & non-coreProperties rejection
    const empty_props = parseDocxCoreProps("<coreProperties/>") ?? unreachable;
    expect_eq(bytesEq(empty_props.title, ""), true);
    expect_eq(parseDocxCoreProps("<w:document><w:body/></w:document>") == null, true);
});

test("5. summarizeDocx paragraph, heading, table, and run statistics", () => {
    const doc_xml =
        "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">" +
        "<w:body>" +
        "  <w:p><w:pPr><w:pStyle w:val=\"Heading1\"/></w:pPr><w:r><w:rPr><w:b/></w:rPr><w:t>Title</w:t></w:r></w:p>" +
        "  <w:p><w:pPr><w:pStyle w:val=\"Heading2\"/></w:pPr><w:r><w:t>Sub</w:t></w:r></w:p>" +
        "  <w:p><w:r><w:t>Run 1</w:t></w:r><w:r><w:t>Run 2</w:t></w:r></w:p>" +
        "  <w:tbl>" +
        "    <w:tblPr/><w:tblGrid/>" +
        "    <w:tr><w:tc><w:p><w:r><w:t>Cell 1</w:t></w:r></w:p></w:tc>" +
        "          <w:tc><w:p><w:r><w:t>Cell 2</w:t></w:r></w:p></w:tc></w:tr>" +
        "  </w:tbl>" +
        "  <w:tbl><w:tblPr/><w:tr><w:tc><w:p><w:r><w:t>T2</w:t></w:r></w:p></w:tc></w:tr></w:tbl>" +
        "</w:body>" +
        "</w:document>";

    const stats = summarizeDocx(doc_xml);
    expect_eq(stats.paragraph_count, 6);
    expect_eq(stats.heading_count, 2);
    expect_eq(stats.table_count, 2);
    expect_eq(stats.run_count, 7);
});

test("6. Buffer overflow and malformed / non-DOCX XML fault tolerance", () => {
    const doc_xml =
        "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">" +
        "<w:body>" +
        "<w:p><w:r><w:t>Hello</w:t></w:r></w:p>" +
        "<w:p><w:r><w:t>World</w:t></w:r></w:p>" +
        "</w:body>" +
        "</w:document>";

    // Exact fit: "Hello\nWorld" = 11 bytes
    let exact_buf: [11]u8 = std.mem.zeroes([11]u8);
    const ok_len = extractDocxText(doc_xml, exact_buf[0..]) ?? unreachable;
    expect_eq(ok_len, 11);
    expect_eq(bytesEq(exact_buf[0..ok_len], "Hello\nWorld"), true);

    // 1 byte short -> must return null
    let short_buf: [10]u8 = std.mem.zeroes([10]u8);
    expect_eq(extractDocxText(doc_xml, short_buf[0..]) == null, true);

    // Overflow on paragraph separator '\n' (5 bytes fits "Hello", fails on '\n')
    let sep_buf: [5]u8 = std.mem.zeroes([5]u8);
    expect_eq(extractDocxText(doc_xml, sep_buf[0..]) == null, true);

    // Single paragraph overflow
    let tiny_buf: [2]u8 = std.mem.zeroes([2]u8);
    expect_eq(extractDocxParagraphText("<w:p><w:r><w:t>ABCD</w:t></w:r></w:p>", tiny_buf[0..]) == null, true);

    // Non-DOCX XML returns null for extractDocxText and zero stats for summarizeDocx
    let ok_buf: [64]u8 = std.mem.zeroes([64]u8);
    expect_eq(extractDocxText("<html><body>Hello</body></html>", ok_buf[0..]) == null, true);
    expect_eq(extractDocxText("", ok_buf[0..]) == null, true);
    expect_eq(extractDocxText("<w:document unclosed", ok_buf[0..]) == null, true);

    const bad_stats = summarizeDocx("<html><body>Hello</body></html>");
    expect_eq(bad_stats.paragraph_count, 0);
    expect_eq(bad_stats.heading_count, 0);
    expect_eq(bad_stats.table_count, 0);
    expect_eq(bad_stats.run_count, 0);
});
