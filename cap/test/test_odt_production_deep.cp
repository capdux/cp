// Source: cp/cap/test/test_odt_production_deep.cp (Production deep tests for cap:odt)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ODT_MIMETYPE,
    ODS_MIMETYPE,
    ODP_MIMETYPE,
    OdtMeta,
    OdtManifestEntry,
    OdtBlock,
    detectOdfMimetype,
    isOdtZip,
    parseOdtMeta,
    OdtManifestIter,
    OdtBlockIter,
    extractOdtText,
} from "cap:doc";

fn writeU16LE(buf: []u8, offset: usize, val: u16) {
    buf[offset] = (val & 0xFF) as u8;
    buf[offset + 1] = ((val >> 8) & 0xFF) as u8;
}

fn writeU32LE(buf: []u8, offset: usize, val: u32) {
    buf[offset] = (val & 0xFF) as u8;
    buf[offset + 1] = ((val >> 8) & 0xFF) as u8;
    buf[offset + 2] = ((val >> 16) & 0xFF) as u8;
    buf[offset + 3] = ((val >> 24) & 0xFF) as u8;
}

fn copySlice(dst: []u8, offset: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[offset + i] = src[i];
        i += 1;
    }
}

fn buildOdfZipHeader(buf: []u8, mime: []const u8): usize {
    buf[0] = 0x50;
    buf[1] = 0x4B;
    buf[2] = 0x03;
    buf[3] = 0x04;
    writeU16LE(buf, 4, 20);
    writeU16LE(buf, 6, 0);
    writeU16LE(buf, 8, 0);
    writeU32LE(buf, 18, mime.len as u32);
    writeU32LE(buf, 22, mime.len as u32);
    writeU16LE(buf, 26, 8);
    writeU16LE(buf, 28, 0);
    copySlice(buf, 30, "mimetype");
    copySlice(buf, 38, mime);
    return 38 + mime.len;
}

const SAMPLE_META_XML: []const u8 =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
    "<office:document-meta xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" " ++
    "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:meta=\"urn:oasis:names:tc:opendocument:xmlns:meta:1.0\">\n" ++
    "  <office:meta>\n" ++
    "    <!-- document metadata -->\n" ++
    "    <meta:generator>LibreOffice/24.2.5</meta:generator>\n" ++
    "    <dc:title>CP Systems Architecture</dc:title>\n" ++
    "    <dc:description>Zero-allocation OpenDocument parser specification.</dc:description>\n" ++
    "    <dc:subject>Systems Engineering</dc:subject>\n" ++
    "    <meta:initial-creator>Ada Lovelace</meta:initial-creator>\n" ++
    "    <dc:creator>Grace Hopper</dc:creator>\n" ++
    "    <meta:creation-date>2026-01-15T09:00:00Z</meta:creation-date>\n" ++
    "    <dc:date>2026-11-21T18:30:00Z</dc:date>\n" ++
    "    <dc:language>en-US</dc:language>\n" ++
    "    <meta:document-statistic meta:table-count=\"3\" meta:page-count=\"42\" meta:word-count=\"12345\" meta:character-count=\"78901\"/>\n" ++
    "  </office:meta>\n" ++
    "</office:document-meta>";

const SAMPLE_MANIFEST_XML: []const u8 =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
    "<manifest:manifest xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\" manifest:version=\"1.3\">\n" ++
    "  <manifest:file-entry manifest:full-path=\"/\" manifest:version=\"1.3\" manifest:media-type=\"application/vnd.oasis.opendocument.text\"/>\n" ++
    "  <manifest:file-entry manifest:full-path=\"content.xml\" manifest:media-type=\"text/xml\"/>\n" ++
    "  <manifest:file-entry manifest:media-type='text/xml' manifest:full-path='meta.xml'/>\n" ++
    "  <manifest:file-entry manifest:full-path=\"Pictures/logo.png\" manifest:media-type=\"image/png\"/>\n" ++
    "</manifest:manifest>";

const SAMPLE_CONTENT_XML: []const u8 =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
    "<office:document-content xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" " ++
    "xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">\n" ++
    "  <office:body>\n" ++
    "    <office:text>\n" ++
    "      <text:h text:style-name=\"Heading_20_1\" text:outline-level=\"1\">Chapter 1: Intro &amp; Goals</text:h>\n" ++
    "      <text:p text:style-name=\"Text_20_body\">Hello<text:s text:c=\"3\"/>World<text:tab/>Tab<text:line-break/>Line2</text:p>\n" ++
    "      <text:p text:style-name=\"Text_20_body\"><text:span text:style-name=\"T1\">A &lt; B &gt; C</text:span><text:s/>&quot;quoted&quot; &apos;ok&apos;</text:p>\n" ++
    "      <text:p text:style-name=\"Empty_20_Para\"/>\n" ++
    "    </office:text>\n" ++
    "  </office:body>\n" ++
    "</office:document-content>";

test("1. detectOdfMimetype & isOdtZip validate ODT/ODS/ODP OCF headers", () => {
    let odt_buf: [128]u8 = std.mem.zeroes([128]u8);
    const odt_len = buildOdfZipHeader(odt_buf[0..], ODT_MIMETYPE);
    expect_eq(isOdtZip(odt_buf[0..odt_len]), true);
    const detected_odt = detectOdfMimetype(odt_buf[0..odt_len]) ?? unreachable;
    expect_eq(std.mem.eql(u8, detected_odt, ODT_MIMETYPE), true);

    let ods_buf: [128]u8 = std.mem.zeroes([128]u8);
    const ods_len = buildOdfZipHeader(ods_buf[0..], ODS_MIMETYPE);
    expect_eq(isOdtZip(ods_buf[0..ods_len]), false);
    const detected_ods = detectOdfMimetype(ods_buf[0..ods_len]) ?? unreachable;
    expect_eq(std.mem.eql(u8, detected_ods, ODS_MIMETYPE), true);

    let odp_buf: [128]u8 = std.mem.zeroes([128]u8);
    const odp_len = buildOdfZipHeader(odp_buf[0..], ODP_MIMETYPE);
    expect_eq(isOdtZip(odp_buf[0..odp_len]), false);
    const detected_odp = detectOdfMimetype(odp_buf[0..odp_len]) ?? unreachable;
    expect_eq(std.mem.eql(u8, detected_odp, ODP_MIMETYPE), true);
});

test("2. parseOdtMeta extracts all dc:*/meta:* fields and document-statistic counters", () => {
    const meta = parseOdtMeta(SAMPLE_META_XML) ?? unreachable;
    expect_eq(std.mem.eql(u8, meta.generator, "LibreOffice/24.2.5"), true);
    expect_eq(std.mem.eql(u8, meta.title, "CP Systems Architecture"), true);
    expect_eq(std.mem.eql(u8, meta.description, "Zero-allocation OpenDocument parser specification."), true);
    expect_eq(std.mem.eql(u8, meta.subject, "Systems Engineering"), true);
    expect_eq(std.mem.eql(u8, meta.initial_creator, "Ada Lovelace"), true);
    expect_eq(std.mem.eql(u8, meta.creator, "Grace Hopper"), true);
    expect_eq(std.mem.eql(u8, meta.creation_date, "2026-01-15T09:00:00Z"), true);
    expect_eq(std.mem.eql(u8, meta.date, "2026-11-21T18:30:00Z"), true);
    expect_eq(std.mem.eql(u8, meta.language, "en-US"), true);
    expect_eq(meta.page_count, 42);
    expect_eq(meta.word_count, 12345);
    expect_eq(meta.character_count, 78901);
});

test("3. OdtManifestIter iterates all manifest:file-entry items with path, media-type, and version", () => {
    let it = OdtManifestIter.init(SAMPLE_MANIFEST_XML);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.full_path, "/"), true);
    expect_eq(std.mem.eql(u8, e1.media_type, ODT_MIMETYPE), true);
    expect_eq(std.mem.eql(u8, e1.version, "1.3"), true);

    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.full_path, "content.xml"), true);
    expect_eq(std.mem.eql(u8, e2.media_type, "text/xml"), true);
    expect_eq(std.mem.eql(u8, e2.version, ""), true);

    const e3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.full_path, "meta.xml"), true);
    expect_eq(std.mem.eql(u8, e3.media_type, "text/xml"), true);

    const e4 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e4.full_path, "Pictures/logo.png"), true);
    expect_eq(std.mem.eql(u8, e4.media_type, "image/png"), true);

    expect_eq(it.next() == null, true);
});

test("4. OdtBlockIter & extractOdtText handle headings, spans, text:s, text:tab, text:line-break, and XML entities", () => {
    let bit = OdtBlockIter.init(SAMPLE_CONTENT_XML);

    const b1 = bit.next() ?? unreachable;
    expect_eq(b1.is_heading, true);
    expect_eq(b1.outline_level, 1);
    expect_eq(std.mem.eql(u8, b1.style_name, "Heading_20_1"), true);

    const b2 = bit.next() ?? unreachable;
    expect_eq(b2.is_heading, false);
    expect_eq(b2.outline_level, 0);
    expect_eq(std.mem.eql(u8, b2.style_name, "Text_20_body"), true);

    const b3 = bit.next() ?? unreachable;
    expect_eq(b3.is_heading, false);

    const b4 = bit.next() ?? unreachable;
    expect_eq(b4.is_heading, false);
    expect_eq(std.mem.eql(u8, b4.raw_xml, ""), true);

    expect_eq(bit.next() == null, true);

    let out: [512]u8 = std.mem.zeroes([512]u8);
    const n = extractOdtText(SAMPLE_CONTENT_XML, out[0..]) ?? unreachable;
    const expected =
        "Chapter 1: Intro & Goals\n" ++
        "Hello   World\tTab\nLine2\n" ++
        "A < B > C \"quoted\" 'ok'\n";
    expect_eq(std.mem.eql(u8, out[0..n], expected), true);
});

test("5. Malformed ZIP headers, compressed mimetype, truncated XML, and output buffer overflow protection", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const valid_len = buildOdfZipHeader(buf[0..], ODT_MIMETYPE);

    // Truncated payload
    expect_eq(detectOdfMimetype(buf[0 .. valid_len - 1]) == null, true);
    expect_eq(isOdtZip(buf[0..30]), false);

    // Compressed mimetype (method = 8)
    let compressed: [128]u8 = buf;
    writeU16LE(compressed[0..], 8, 8);
    expect_eq(detectOdfMimetype(compressed[0..valid_len]) == null, true);
    expect_eq(isOdtZip(compressed[0..valid_len]), false);

    // Non-zero extra field
    let with_extra: [128]u8 = buf;
    writeU16LE(with_extra[0..], 28, 4);
    expect_eq(detectOdfMimetype(with_extra[0..valid_len]) == null, true);

    // Wrong filename
    let bad_name: [128]u8 = buf;
    bad_name[30] = 'X';
    expect_eq(detectOdfMimetype(bad_name[0..valid_len]) == null, true);

    // Mismatched comp_size vs uncomp_size
    let bad_sizes: [128]u8 = buf;
    writeU32LE(bad_sizes[0..], 22, 99);
    expect_eq(detectOdfMimetype(bad_sizes[0..valid_len]) == null, true);

    // Non-ODF / malformed XML
    expect_eq(parseOdtMeta("") == null, true);
    expect_eq(parseOdtMeta("<html><body>No meta</body></html>") == null, true);
    expect_eq(parseOdtMeta("<office:document-meta><office:meta><dc:title>Cut") == null, true);

    // Output buffer overflow protection in extractOdtText
    let tiny: [10]u8 = std.mem.zeroes([10]u8);
    expect_eq(extractOdtText(SAMPLE_CONTENT_XML, tiny[0..]) == null, true);
});
