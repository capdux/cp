// Source: cp/cap/test/test_xml_production_deep.cp
// Deep Production Test Suite for cap:xml
// Covers:
//   1. XML declaration, comments, CDATA, self-closing tags, namespaces, single/double quotes & entity decoding
//   2. Real RSS & S3 XML DOM queries: findByPath, findAllChildren, getAttr, innerText
//   3. XmlWriter compact/pretty serialization & XmlSerializer.stringify lossless round-trip
//   4. Boundary guards: MismatchedTag, UnexpectedEof, MaxDepthExceeded (>128), InvalidEntity & CDATA `]]>` rejection
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.xml`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    XmlError,
    XmlAttr,
    XmlNodeKind,
    XmlNode,
    XmlDocument,
    XmlParser,
    XmlWriter,
    XmlSerializer,
} from "cap:xml";
import cap from "cap";

fn testDeclCdataNamespacesAndEntities()! {
    const alloc = std.heap.page_allocator;

    const xml_input =
        "\xEF\xBB\xBF" ++
        "<?xml version='1.0' encoding='UTF-8'?>\n" ++
        "<!-- leading comment with <fake> tags & entities -->\n" ++
        "<svg:svg xmlns:svg=\"http://www.w3.org/2000/svg\" viewBox='0 0 100 100' title=\"A &amp; B &lt;1&gt; &quot;hi&quot; &apos;ok&apos;\">\n" ++
        "  <?custom-pi target=\"ignored\"?>\n" ++
        "  <svg:rect width=\"10\" height='20'/>\n" ++
        "  <!-- mid comment -->\n" ++
        "  <svg:desc>Hex &#x41;&#x1F600; Dec &#66;&#20013; &amp;&lt;&gt;&quot;&apos;</svg:desc>\n" ++
        "  <svg:script><![CDATA[if (a < b && c > d) { return \"raw &amp;\"; }]]></svg:script>\n" ++
        "</svg:svg>\n" ++
        "<!-- trailing comment -->";

    let doc = XmlParser.parse(alloc, xml_input) catch unreachable;
    defer doc.deinit();

    expect_eq(std.mem.eql(u8, doc.version, "1.0"), true);
    expect_eq(std.mem.eql(u8, doc.encoding, "UTF-8"), true);
    expect_eq(std.mem.eql(u8, doc.root.tag, "svg:svg"), true);
    expect_eq(doc.root.kind == .Element, true);

    expect_eq(std.mem.eql(u8, doc.root.getAttr("xmlns:svg").?, "http://www.w3.org/2000/svg"), true);
    expect_eq(std.mem.eql(u8, doc.root.getAttr("viewBox").?, "0 0 100 100"), true);
    expect_eq(std.mem.eql(u8, doc.root.getAttr("title").?, "A & B <1> \"hi\" 'ok'"), true);
    expect_eq(doc.root.getAttr("missing") == null, true);

    const rect = doc.root.findChild("svg:rect").?;
    expect_eq(rect.children.len, 0 as usize);
    expect_eq(std.mem.eql(u8, rect.getAttr("width").?, "10"), true);
    expect_eq(std.mem.eql(u8, rect.getAttr("height").?, "20"), true);

    const desc = doc.root.findChild("svg:desc").?;
    const desc_txt = desc.innerText(alloc) catch unreachable;
    defer alloc.free(desc_txt);
    expect_eq(std.mem.eql(u8, desc_txt, "Hex A\xF0\x9F\x98\x80 Dec B\xE4\xB8\xAD &<>\"'"), true);

    const script = doc.root.findChild("svg:script").?;
    expect_eq(script.children.len, 1 as usize);
    expect_eq(script.children[0].kind == .CData, true);
    const script_txt = script.innerText(alloc) catch unreachable;
    defer alloc.free(script_txt);
    expect_eq(std.mem.eql(u8, script_txt, "if (a < b && c > d) { return \"raw &amp;\"; }"), true);
}

fn testRssAndS3DomQueries()! {
    const alloc = std.heap.page_allocator;

    const rss_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
        "<rss version=\"2.0\" xmlns:atom=\"http://www.w3.org/2005/Atom\">\n" ++
        "  <channel>\n" ++
        "    <title>CP Releases</title>\n" ++
        "    <atom:link href=\"https://cp-lang.org/feed.xml\" rel=\"self\"/>\n" ++
        "    <item>\n" ++
        "      <title>v0.9 &amp; Runtime</title>\n" ++
        "      <link>https://cp-lang.org/v0.9</link>\n" ++
        "      <description><![CDATA[Fast <b>Native</b> Build]]></description>\n" ++
        "    </item>\n" ++
        "    <item>\n" ++
        "      <title>v1.0 GA</title>\n" ++
        "      <link>https://cp-lang.org/v1.0</link>\n" ++
        "    </item>\n" ++
        "  </channel>\n" ++
        "</rss>";

    let rss_doc = XmlParser.parse(alloc, rss_xml) catch unreachable;
    defer rss_doc.deinit();

    expect_eq(std.mem.eql(u8, rss_doc.root.getAttr("version").?, "2.0"), true);

    const first_item_title = rss_doc.root.findByPath("channel/item/title").?;
    const first_title_str = first_item_title.innerText(alloc) catch unreachable;
    defer alloc.free(first_title_str);
    expect_eq(std.mem.eql(u8, first_title_str, "v0.9 & Runtime"), true);

    const atom_link = rss_doc.root.findByPath("channel/atom:link").?;
    expect_eq(std.mem.eql(u8, atom_link.getAttr("href").?, "https://cp-lang.org/feed.xml"), true);
    expect_eq(std.mem.eql(u8, atom_link.getAttr("rel").?, "self"), true);
    expect_eq(rss_doc.root.findByPath("channel/missing/title") == null, true);

    const channel = rss_doc.root.findChild("channel").?;
    const items = channel.findAllChildren(alloc, "item") catch unreachable;
    defer alloc.free(items);
    expect_eq(items.len, 2 as usize);

    const second_title = items[1].findChild("title").?.innerText(alloc) catch unreachable;
    defer alloc.free(second_title);
    expect_eq(std.mem.eql(u8, second_title, "v1.0 GA"), true);

    // S3 ListObjectsV2 response query
    const s3_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
        "<ListBucketResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n" ++
        "  <Name>cp-artifacts</Name>\n" ++
        "  <IsTruncated>false</IsTruncated>\n" ++
        "  <Contents><Key>builds/linux-x64.tar.zst</Key><Size>4096</Size></Contents>\n" ++
        "  <Contents><Key>builds/macos-arm64.tar.zst</Key><Size>4120</Size></Contents>\n" ++
        "</ListBucketResult>";

    let s3_doc = XmlParser.parse(alloc, s3_xml) catch unreachable;
    defer s3_doc.deinit();

    const bucket_name = s3_doc.root.findByPath("Name").?.innerText(alloc) catch unreachable;
    defer alloc.free(bucket_name);
    expect_eq(std.mem.eql(u8, bucket_name, "cp-artifacts"), true);

    const objects = s3_doc.root.findAllChildren(alloc, "Contents") catch unreachable;
    defer alloc.free(objects);
    expect_eq(objects.len, 2 as usize);

    const key1 = objects[1].findChild("Key").?.innerText(alloc) catch unreachable;
    defer alloc.free(key1);
    expect_eq(std.mem.eql(u8, key1, "builds/macos-arm64.tar.zst"), true);
}

fn testWriterAndRoundTrip()! {
    const alloc = std.heap.page_allocator;

    // 1. Compact XmlWriter
    let cw = XmlWriter.init(alloc);
    defer cw.deinit();
    cw.writeDeclaration() catch unreachable;
    cw.openTag("testsuites") catch unreachable;
    cw.attr("name", "unit & \"core\"") catch unreachable;
    cw.openTag("testsuite") catch unreachable;
    cw.attr("tests", "2") catch unreachable;
    cw.openTag("testcase") catch unreachable;
    cw.attr("name", "a<b") catch unreachable;
    cw.text("5 > 3 & 1 < 2") catch unreachable;
    cw.closeTag() catch unreachable;
    cw.openTag("note") catch unreachable;
    cw.cdata("raw <tag> & data") catch unreachable;
    cw.closeTag() catch unreachable;
    cw.emptyTag("skipped") catch unreachable;
    cw.closeTag() catch unreachable;
    cw.closeTag() catch unreachable;

    const compact_xml = cw.toOwnedSlice() catch unreachable;
    defer alloc.free(compact_xml);

    const expected_compact =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" ++
        "<testsuites name=\"unit &amp; &quot;core&quot;\">" ++
        "<testsuite tests=\"2\">" ++
        "<testcase name=\"a&lt;b\">5 &gt; 3 &amp; 1 &lt; 2</testcase>" ++
        "<note><![CDATA[raw <tag> & data]]></note>" ++
        "<skipped/>" ++
        "</testsuite>" ++
        "</testsuites>";
    expect_eq(std.mem.eql(u8, compact_xml, expected_compact), true);

    // 2. Pretty XmlWriter
    let pw = XmlWriter.initPretty(alloc, 2);
    defer pw.deinit();
    pw.writeDeclaration() catch unreachable;
    pw.openTag("root") catch unreachable;
    pw.attr("env", "prod") catch unreachable;
    pw.openTag("item") catch unreachable;
    pw.attr("id", "1") catch unreachable;
    pw.text("A & B") catch unreachable;
    pw.closeTag() catch unreachable;
    pw.emptyTag("flag") catch unreachable;
    pw.closeTag() catch unreachable;

    const pretty_xml = pw.toOwnedSlice() catch unreachable;
    defer alloc.free(pretty_xml);

    const expected_pretty =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
        "<root env=\"prod\">\n" ++
        "  <item id=\"1\">A &amp; B</item>\n" ++
        "  <flag/>\n" ++
        "</root>";
    expect_eq(std.mem.eql(u8, pretty_xml, expected_pretty), true);

    // 3. Round-trip via XmlParser -> XmlSerializer.stringify(pretty & compact) -> XmlParser
    let parsed = XmlParser.parse(alloc, compact_xml) catch unreachable;
    defer parsed.deinit();

    const rt_pretty = XmlSerializer.stringify(alloc, parsed.root, true) catch unreachable;
    defer alloc.free(rt_pretty);

    let reparsed = XmlParser.parse(alloc, rt_pretty) catch unreachable;
    defer reparsed.deinit();

    const rt_compact_1 = XmlSerializer.stringify(alloc, parsed.root, false) catch unreachable;
    defer alloc.free(rt_compact_1);
    const rt_compact_2 = XmlSerializer.stringify(alloc, reparsed.root, false) catch unreachable;
    defer alloc.free(rt_compact_2);

    expect_eq(std.mem.eql(u8, rt_compact_1, rt_compact_2), true);
    expect_eq(std.mem.eql(u8, reparsed.root.getAttr("name").?, "unit & \"core\""), true);

    const tc = reparsed.root.findByPath("testsuite/testcase").?;
    expect_eq(std.mem.eql(u8, tc.getAttr("name").?, "a<b"), true);
    const tc_txt = tc.innerText(alloc) catch unreachable;
    defer alloc.free(tc_txt);
    expect_eq(std.mem.eql(u8, tc_txt, "5 > 3 & 1 < 2"), true);
}

fn testErrorAndBoundaryGuards()! {
    const alloc = std.heap.page_allocator;

    // 1. MismatchedTag
    let mismatch_hit = false;
    * = XmlParser.parse(alloc, "<root><child></other></root>") catch |e| {
        if (e == @as(anyerror, err.MismatchedTag)) {
            mismatch_hit = true;
        }
    };
    expect_eq(mismatch_hit, true);

    // 2. UnexpectedEof on unclosed tag & unclosed CDATA
    let eof_tag_hit = false;
    * = XmlParser.parse(alloc, "<root><child>text") catch |e| {
        if (e == @as(anyerror, err.UnexpectedEof)) {
            eof_tag_hit = true;
        }
    };
    expect_eq(eof_tag_hit, true);

    let eof_cdata_hit = false;
    * = XmlParser.parse(alloc, "<root><![CDATA[unclosed") catch |e| {
        if (e == @as(anyerror, err.UnexpectedEof)) {
            eof_cdata_hit = true;
        }
    };
    expect_eq(eof_cdata_hit, true);

    // 3. InvalidEntity
    let bad_entity_hit = false;
    * = XmlParser.parse(alloc, "<root>&unknown;</root>") catch |e| {
        if (e == @as(anyerror, err.InvalidEntity)) {
            bad_entity_hit = true;
        }
    };
    expect_eq(bad_entity_hit, true);

    let bare_amp_hit = false;
    * = XmlParser.parse(alloc, "<root a=\"x & y\"/>") catch |e| {
        if (e == @as(anyerror, err.InvalidEntity)) {
            bare_amp_hit = true;
        }
    };
    expect_eq(bare_amp_hit, true);

    let zero_cp_hit = false;
    * = XmlParser.parse(alloc, "<root>&#0;</root>") catch |e| {
        if (e == @as(anyerror, err.InvalidEntity)) {
            zero_cp_hit = true;
        }
    };
    expect_eq(zero_cp_hit, true);

    // 4. MaxDepthExceeded (> 128) vs valid depth == 128
    let deep_ok_buf: std.ArrayList(u8) = .empty;
    defer deep_ok_buf.deinit(alloc);
    let i: usize = 0;
    while (i < 128) : (i += 1) {
        deep_ok_buf.appendSlice(alloc, "<a>") catch unreachable;
    }
    i = 0;
    while (i < 128) : (i += 1) {
        deep_ok_buf.appendSlice(alloc, "</a>") catch unreachable;
    }
    let deep_ok_doc = XmlParser.parse(alloc, deep_ok_buf.items) catch unreachable;
    deep_ok_doc.deinit();

    let too_deep_buf: std.ArrayList(u8) = .empty;
    defer too_deep_buf.deinit(alloc);
    i = 0;
    while (i < 129) : (i += 1) {
        too_deep_buf.appendSlice(alloc, "<a>") catch unreachable;
    }
    i = 0;
    while (i < 129) : (i += 1) {
        too_deep_buf.appendSlice(alloc, "</a>") catch unreachable;
    }
    let depth_hit = false;
    * = XmlParser.parse(alloc, too_deep_buf.items) catch |e| {
        if (e == @as(anyerror, err.MaxDepthExceeded)) {
            depth_hit = true;
        }
    };
    expect_eq(depth_hit, true);

    // 5. XmlWriter CDATA embedded `]]>` rejection
    let w = XmlWriter.init(alloc);
    defer w.deinit();
    w.openTag("data") catch unreachable;
    let cdata_reject_hit = false;
    w.cdata("bad ]]> payload") catch |e| {
        if (e == @as(anyerror, err.MalformedTag)) {
            cdata_reject_hit = true;
        }
    };
    expect_eq(cdata_reject_hit, true);
}

fn testCapXmlLazyMount()! {
    const alloc = std.heap.page_allocator;

    const payload = "<feed><entry><id>cp-42</id><title>Dynamic Mount</title></entry></feed>";
    let doc = cap.xml.XmlParser.parse(alloc, payload) catch unreachable;
    defer doc.deinit();

    const id_txt = doc.root.findByPath("entry/id").?.innerText(alloc) catch unreachable;
    defer alloc.free(id_txt);
    expect_eq(std.mem.eql(u8, id_txt, "cp-42"), true);

    const serialized = cap.xml.XmlSerializer.stringify(alloc, doc.root, false) catch unreachable;
    defer alloc.free(serialized);
    expect_eq(std.mem.eql(u8, serialized, payload), true);

    let w = cap.xml.XmlWriter.initPretty(alloc, 2);
    defer w.deinit();
    w.openTag("ok") catch unreachable;
    w.attr("v", "1") catch unreachable;
    w.closeTag() catch unreachable;
    const out = w.toOwnedSlice() catch unreachable;
    defer alloc.free(out);
    expect_eq(std.mem.eql(u8, out, "<ok v=\"1\"/>"), true);
}

fn main() {
    test("1. XML Declaration, Comments, CDATA, Self-Closing, Namespaces & Entity Decoding", () => {
        testDeclCdataNamespacesAndEntities()?;
    });

    test("2. Real RSS & S3 XML DOM Queries (findByPath, findAllChildren, getAttr, innerText)", () => {
        testRssAndS3DomQueries()?;
    });

    test("3. XmlWriter Compact/Pretty Serialization & XmlSerializer Lossless Round-Trip", () => {
        testWriterAndRoundTrip()?;
    });

    test("4. Boundary Guards: MismatchedTag, UnexpectedEof, MaxDepthExceeded (>128), InvalidEntity & CDATA ]]>", () => {
        testErrorAndBoundaryGuards()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.xml`", () => {
        testCapXmlLazyMount()?;
    });
}
