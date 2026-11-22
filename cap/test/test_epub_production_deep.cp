// Source: cp/cap/test/test_epub_production_deep.cp (Production deep tests for cap:epub)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    EPUB_MIMETYPE,
    EPUB_OPF_MEDIA_TYPE,
    EpubRootfile,
    EpubMetadata,
    EpubManifestItem,
    EpubSpineItem,
    isEpubZip,
    parseContainerXml,
    parseOpfMetadata,
    EpubManifestIter,
    findManifestItem,
    EpubSpineIter,
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

fn buildValidEpubZipHeader(buf: []u8) {
    // PK\x03\x04
    buf[0] = 0x50;
    buf[1] = 0x4B;
    buf[2] = 0x03;
    buf[3] = 0x04;
    // version needed = 20
    writeU16LE(buf, 4, 20);
    // flags = 0
    writeU16LE(buf, 6, 0);
    // compression method = 0 (Stored)
    writeU16LE(buf, 8, 0);
    // compressed & uncompressed size = 20
    writeU32LE(buf, 18, 20);
    writeU32LE(buf, 22, 20);
    // filename len = 8 ("mimetype"), extra len = 0
    writeU16LE(buf, 26, 8);
    writeU16LE(buf, 28, 0);
    // offset 30..38 = "mimetype", 38..58 = "application/epub+zip"
    copySlice(buf, 30, "mimetype");
    copySlice(buf, 38, EPUB_MIMETYPE);
}

const SAMPLE_CONTAINER_XML: []const u8 =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
    "<container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">\n" ++
    "  <!-- Comment with fake <rootfile full-path=\"fake.opf\" media-type=\"application/oebps-package+xml\"/> -->\n" ++
    "  <rootfiles>\n" ++
    "    <rootfile media-type='application/pdf' full-path='alt/book.pdf'/>\n" ++
    "    <rootfile media-type=\"application/oebps-package+xml\" full-path=\"OEBPS/content.opf\"/>\n" ++
    "  </rootfiles>\n" ++
    "</container>";

const SAMPLE_OPF_EPUB3: []const u8 =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
    "<package xmlns=\"http://www.idpf.org/2007/opf\" version=\"3.0\" unique-identifier=\"book-id\">\n" ++
    "  <metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n" ++
    "    <!-- metadata comment -->\n" ++
    "    <dc:identifier id=\"isbn-id\">urn:isbn:9780123456789</dc:identifier>\n" ++
    "    <dc:identifier id=\"book-id\">urn:uuid:a1b2c3d4-e5f6-7890-abcd-ef1234567890</dc:identifier>\n" ++
    "    <dc:title id=\"main-title\"><![CDATA[Systems Programming in CP]]></dc:title>\n" ++
    "    <dc:creator id=\"creator1\">Ada Lovelace</dc:creator>\n" ++
    "    <dc:language>en-US</dc:language>\n" ++
    "    <dc:publisher>CP Foundation Press</dc:publisher>\n" ++
    "    <dc:date>2026-11-21</dc:date>\n" ++
    "    <dc:description>Zero-allocation systems design guide.</dc:description>\n" ++
    "    <meta property=\"dcterms:modified\">2026-11-21T12:00:00Z</meta>\n" ++
    "  </metadata>\n" ++
    "  <manifest>\n" ++
    "    <!-- manifest items -->\n" ++
    "    <item id=\"nav\" href=\"nav.xhtml\" media-type=\"application/xhtml+xml\" properties=\"nav\"/>\n" ++
    "    <item id=\"cover-img\" href=\"images/cover.png\" media-type=\"image/png\" properties=\"cover-image\"/>\n" ++
    "    <item media-type='application/xhtml+xml' href='text/ch01.xhtml' id='ch1'/>\n" ++
    "    <item id=\"ch2\" href=\"text/ch02.xhtml\" media-type=\"application/xhtml+xml\"></item>\n" ++
    "  </manifest>\n" ++
    "  <spine toc=\"ncx\">\n" ++
    "    <itemref idref=\"nav\" linear=\"no\"/>\n" ++
    "    <itemref idref=\"ch1\"/>\n" ++
    "    <itemref idref=\"ch2\" linear=\"yes\"/>\n" ++
    "  </spine>\n" ++
    "</package>";

test("1. isEpubZip validates OCF uncompressed mimetype header and rejects compressed or altered headers", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    buildValidEpubZipHeader(buf[0..]);
    expect_eq(isEpubZip(buf[0..58]), true);
    expect_eq(isEpubZip(buf[0..64]), true);

    // Truncated (< 58 bytes)
    expect_eq(isEpubZip(buf[0..57]), false);
    expect_eq(isEpubZip(""), false);

    // Compressed (method = 8 Deflate) -> must reject
    let deflated: [64]u8 = buf;
    writeU16LE(deflated[0..], 8, 8);
    expect_eq(isEpubZip(deflated[0..]), false);

    // Extra field present (extra_len = 4) -> must reject
    let with_extra: [64]u8 = buf;
    writeU16LE(with_extra[0..], 28, 4);
    expect_eq(isEpubZip(with_extra[0..]), false);

    // Wrong filename ("mimetypx") -> must reject
    let bad_name: [64]u8 = buf;
    bad_name[37] = 'x';
    expect_eq(isEpubZip(bad_name[0..]), false);

    // Wrong mimetype payload -> must reject
    let bad_mime: [64]u8 = buf;
    bad_mime[57] = 'x';
    expect_eq(isEpubZip(bad_mime[0..]), false);

    // Wrong signature -> must reject
    let bad_sig: [64]u8 = buf;
    bad_sig[0] = 0x00;
    expect_eq(isEpubZip(bad_sig[0..]), false);

    // Wrong compressed size -> must reject
    let bad_size: [64]u8 = buf;
    writeU32LE(bad_size[0..], 18, 25);
    expect_eq(isEpubZip(bad_size[0..]), false);
});

test("2. parseContainerXml extracts OPF rootfile path with arbitrary attribute order, quotes, and comments", () => {
    const rf = parseContainerXml(SAMPLE_CONTAINER_XML) ?? unreachable;
    expect_eq(std.mem.eql(u8, rf.full_path, "OEBPS/content.opf"), true);
    expect_eq(std.mem.eql(u8, rf.media_type, EPUB_OPF_MEDIA_TYPE), true);

    // Single quotes and reversed order, fallback when media-type omitted
    const minimal = "<container><rootfiles><rootfile full-path='EPUB/package.opf'/></rootfiles></container>";
    const rf2 = parseContainerXml(minimal) ?? unreachable;
    expect_eq(std.mem.eql(u8, rf2.full_path, "EPUB/package.opf"), true);
    expect_eq(std.mem.eql(u8, rf2.media_type, ""), true);

    // Missing rootfile or empty full-path -> null
    expect_eq(parseContainerXml("<container><rootfiles></rootfiles></container>") == null, true);
    expect_eq(parseContainerXml("<container><rootfile full-path=\"\"/></container>") == null, true);
});

test("3. Full EPUB 3 OPF metadata, manifest iteration, findManifestItem, and spine traversal", () => {
    const meta = parseOpfMetadata(SAMPLE_OPF_EPUB3) ?? unreachable;
    expect_eq(std.mem.eql(u8, meta.version, "3.0"), true);
    expect_eq(std.mem.eql(u8, meta.unique_identifier, "book-id"), true);
    expect_eq(std.mem.eql(u8, meta.identifier, "urn:uuid:a1b2c3d4-e5f6-7890-abcd-ef1234567890"), true);
    expect_eq(std.mem.eql(u8, meta.title, "Systems Programming in CP"), true);
    expect_eq(std.mem.eql(u8, meta.creator, "Ada Lovelace"), true);
    expect_eq(std.mem.eql(u8, meta.language, "en-US"), true);
    expect_eq(std.mem.eql(u8, meta.publisher, "CP Foundation Press"), true);
    expect_eq(std.mem.eql(u8, meta.date, "2026-11-21"), true);
    expect_eq(std.mem.eql(u8, meta.description, "Zero-allocation systems design guide."), true);

    // Manifest iteration
    let mit = EpubManifestIter.init(SAMPLE_OPF_EPUB3);
    const m1 = mit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m1.id, "nav"), true);
    expect_eq(std.mem.eql(u8, m1.href, "nav.xhtml"), true);
    expect_eq(std.mem.eql(u8, m1.media_type, "application/xhtml+xml"), true);
    expect_eq(std.mem.eql(u8, m1.properties, "nav"), true);

    const m2 = mit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m2.id, "cover-img"), true);
    expect_eq(std.mem.eql(u8, m2.properties, "cover-image"), true);

    const m3 = mit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m3.id, "ch1"), true);
    expect_eq(std.mem.eql(u8, m3.href, "text/ch01.xhtml"), true);
    expect_eq(std.mem.eql(u8, m3.properties, ""), true);

    const m4 = mit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m4.id, "ch2"), true);
    expect_eq(std.mem.eql(u8, m4.href, "text/ch02.xhtml"), true);

    expect_eq(mit.next() == null, true);

    // findManifestItem
    const found = findManifestItem(SAMPLE_OPF_EPUB3, "ch1") ?? unreachable;
    expect_eq(std.mem.eql(u8, found.href, "text/ch01.xhtml"), true);
    expect_eq(findManifestItem(SAMPLE_OPF_EPUB3, "nonexistent") == null, true);
    expect_eq(findManifestItem(SAMPLE_OPF_EPUB3, "") == null, true);

    // Spine iteration joined with manifest
    let sit = EpubSpineIter.init(SAMPLE_OPF_EPUB3);
    const s1 = sit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, s1.idref, "nav"), true);
    expect_eq(s1.linear, false);

    const s2 = sit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, s2.idref, "ch1"), true);
    expect_eq(s2.linear, true);

    const s3 = sit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, s3.idref, "ch2"), true);
    expect_eq(s3.linear, true);

    expect_eq(sit.next() == null, true);
});

test("4. Namespace compatibility (unprefixed Dublin Core, opf: prefixed tags, linear='NO', self-closing tags)", () => {
    const prefixed_opf: []const u8 =
        "<opf:package version='2.0' unique-identifier='uid'>" ++
        "  <opf:metadata>" ++
        "    <title/>" ++
        "    <title>Plain Title Without Prefix</title>" ++
        "    <creator>Grace Hopper</creator>" ++
        "    <language>en</language>" ++
        "    <identifier>urn:uuid:plain-001</identifier>" ++
        "  </opf:metadata>" ++
        "  <opf:manifest>" ++
        "    <opf:item id='p1' href='p1.xhtml' media-type='application/xhtml+xml'/>" ++
        "    <opf:item id='p2' href='p2.xhtml' media-type='application/xhtml+xml'/>" ++
        "  </opf:manifest>" ++
        "  <opf:spine>" ++
        "    <opf:itemref idref='p1' linear='NO'/>" ++
        "    <opf:itemref idref='p2'/>" ++
        "  </opf:spine>" ++
        "</opf:package>";

    const meta = parseOpfMetadata(prefixed_opf) ?? unreachable;
    expect_eq(std.mem.eql(u8, meta.version, "2.0"), true);
    expect_eq(std.mem.eql(u8, meta.title, "Plain Title Without Prefix"), true);
    expect_eq(std.mem.eql(u8, meta.creator, "Grace Hopper"), true);
    expect_eq(std.mem.eql(u8, meta.language, "en"), true);
    expect_eq(std.mem.eql(u8, meta.identifier, "urn:uuid:plain-001"), true);

    const p2 = findManifestItem(prefixed_opf, "p2") ?? unreachable;
    expect_eq(std.mem.eql(u8, p2.href, "p2.xhtml"), true);

    let sit = EpubSpineIter.init(prefixed_opf);
    const s1 = sit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, s1.idref, "p1"), true);
    expect_eq(s1.linear, false);
    const s2 = sit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, s2.idref, "p2"), true);
    expect_eq(s2.linear, true);
    expect_eq(sit.next() == null, true);
});

test("5. Malformed, non-OPF, and truncated XML inputs fail safely without panic or infinite loop", () => {
    // Non-OPF XML
    expect_eq(parseOpfMetadata("") == null, true);
    expect_eq(parseOpfMetadata("<html><body>Not an OPF</body></html>") == null, true);

    // Package without metadata
    expect_eq(parseOpfMetadata("<package version=\"3.0\"><manifest/></package>") == null, true);

    // Truncated unclosed package / metadata / quotes / comments
    expect_eq(parseOpfMetadata("<package version=\"3.0\"><metadata><dc:title>Cut") == null, true);
    expect_eq(parseContainerXml("<container><rootfile full-path=\"unterminated") == null, true);
    expect_eq(parseContainerXml("<!-- unterminated comment <rootfile full-path=\"a.opf\"/>") == null, true);

    // Iterators on malformed/truncated OPF terminate cleanly
    let mit = EpubManifestIter.init("<package><manifest><item id=\"x\" href=\"unterminated");
    expect_eq(mit.next() == null, true);

    let sit = EpubSpineIter.init("<not-package><spine><itemref idref=\"ch1\"/></spine></not-package>");
    expect_eq(sit.next() == null, true);
});
