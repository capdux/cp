import { test, expect_eq } from "cap:test";
import {
    XmlEventKind,
    XmlEvent,
    SaxesParser,
} from "../../packages/saxes/src/index.cp";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn main() {
    test("1. @cap/saxes — OpenTag, attributes, Text, CloseTag & SelfClosingTag", () => {
        let p = SaxesParser.new();
        let events: [16]XmlEvent = undefined;
        const xml = "<book id=\"42\" lang='en'><title>CP Lang</title><br/></book>";
        const n = p.parseAll(xml, events[0..16]);

        expect_eq(n, 6);
        expect_eq(events[0].kind, XmlEventKind.OpenTag);
        expect_eq(strEq(events[0].name, "book"), true);
        expect_eq(events[0].attr_count, 2);
        expect_eq(strEq(events[0].attrs[0].name, "id"), true);
        expect_eq(strEq(events[0].attrs[0].value, "42"), true);
        expect_eq(strEq(events[0].attrs[1].name, "lang"), true);
        expect_eq(strEq(events[0].attrs[1].value, "en"), true);

        expect_eq(events[1].kind, XmlEventKind.OpenTag);
        expect_eq(strEq(events[1].name, "title"), true);
        expect_eq(events[2].kind, XmlEventKind.Text);
        expect_eq(strEq(events[2].text, "CP Lang"), true);
        expect_eq(events[3].kind, XmlEventKind.CloseTag);
        expect_eq(strEq(events[3].name, "title"), true);
        expect_eq(events[4].kind, XmlEventKind.SelfClosingTag);
        expect_eq(strEq(events[4].name, "br"), true);
        expect_eq(events[5].kind, XmlEventKind.CloseTag);
        expect_eq(strEq(events[5].name, "book"), true);
    });

    test("2. @cap/saxes — CDATA, Comment & ProcessingInstruction events", () => {
        let p = SaxesParser.new();
        let events: [16]XmlEvent = undefined;
        const xml = "<?xml version=\"1.0\"?><!-- header --><doc><![CDATA[<raw & text>]]></doc>";
        const n = p.parseAll(xml, events[0..16]);

        expect_eq(n, 5);
        expect_eq(events[0].kind, XmlEventKind.ProcessingInstruction);
        expect_eq(strEq(events[0].name, "xml"), true);
        expect_eq(strEq(events[0].text, "version=\"1.0\""), true);

        expect_eq(events[1].kind, XmlEventKind.Comment);
        expect_eq(strEq(events[1].text, " header "), true);

        expect_eq(events[2].kind, XmlEventKind.OpenTag);
        expect_eq(events[3].kind, XmlEventKind.Cdata);
        expect_eq(strEq(events[3].text, "<raw & text>"), true);
        expect_eq(events[4].kind, XmlEventKind.CloseTag);
    });

    test("3. @cap/saxes — isWellFormed accepts valid XML & rejects malformed XML", () => {
        let p = SaxesParser.new();
        expect_eq(p.isWellFormed("<root><a/><b x=\"1\">ok</b></root>"), true);
        expect_eq(p.isWellFormed("<root><a></b></root>"), false);
        expect_eq(p.isWellFormed("<root><a>unclosed</root>"), false);
        expect_eq(p.isWellFormed("<a/><b/>"), false);
        expect_eq(p.isWellFormed("<a x=\"1\" x=\"2\"/>"), false);
        expect_eq(p.isWellFormed("   "), false);
    });

    test("4. @cap/saxes — Mismatched & unclosed tags emit Error event", () => {
        let p = SaxesParser.new();
        let events: [16]XmlEvent = undefined;

        const n1 = p.parseAll("<a><b></a>", events[0..16]);
        expect_eq(n1 >= 3, true);
        expect_eq(events[n1 - 1].kind, XmlEventKind.Error);

        const n2 = p.parseAll("<a><b>text</b>", events[0..16]);
        expect_eq(events[n2 - 1].kind, XmlEventKind.Error);
    });

    test("5. @cap/saxes — decodeEntities named entities (&lt; &gt; &amp; &quot; &apos;)", () => {
        let p = SaxesParser.new();
        let buf: [128]u8 = undefined;

        const n = p.decodeEntities("1 &lt; 2 &amp;&amp; 3 &gt; 2 &quot;hi&quot; &apos;ok&apos;", buf[0..128]);
        expect_eq(strEq(buf[0..n], "1 < 2 && 3 > 2 \"hi\" 'ok'"), true);
    });

    test("6. @cap/saxes — decodeEntities numeric decimal &#NNN; & hex &#xHHH;", () => {
        let p = SaxesParser.new();
        let buf: [128]u8 = undefined;

        const n1 = p.decodeEntities("&#65;&#66;&#x43;&#x21;", buf[0..128]);
        expect_eq(strEq(buf[0..n1], "ABC!"), true);

        // UTF-8 multi-byte code point: U+00E9 ('é' = 0xC3 0xA9)
        const n2 = p.decodeEntities("caf&#xE9;", buf[0..128]);
        expect_eq(n2, 5);
        expect_eq(buf[3], 0xC3);
        expect_eq(buf[4], 0xA9);
    });
}
