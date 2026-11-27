import { test, expect_eq } from "cap:test";
import {
    DomNodeKind,
    DomAttr,
    SerializeOptions,
    isVoidElement,
    serializeText,
    serializeOpenTag,
    serializeElement,
    serializeComment,
    serializeCdata,
} from "../../packages/dom-serializer/src/index.cp";

fn main() {
    test("1. @cap/dom-serializer — identifies HTML void elements case-insensitively", () => {
        expect_eq(isVoidElement("br"), true);
        expect_eq(isVoidElement("IMG"), true);
        expect_eq(isVoidElement("input"), true);
        expect_eq(isVoidElement("meta"), true);
        expect_eq(isVoidElement("wbr"), true);
        expect_eq(isVoidElement("div"), false);
        expect_eq(isVoidElement("span"), false);
        const kind = DomNodeKind.Element;
        expect_eq(kind == DomNodeKind.Element, true);
    });

    test("2. @cap/dom-serializer — serializeText escapes &, <, >", () => {
        let buf: [128]u8 = [0 as u8; 128];
        const n = serializeText("a < b & c > d", buf[0..128]);
        expect_eq(buf[0..n] as string, "a &lt; b &amp; c &gt; d");
    });

    test("3. @cap/dom-serializer — serializeOpenTag escapes attribute values", () => {
        const attrs: [2]DomAttr = [
            DomAttr{ name: "class", value: "btn primary" },
            DomAttr{ name: "title", value: "a & \"b\"" },
        ];
        let buf: [128]u8 = [0 as u8; 128];
        const n = serializeOpenTag("button", attrs[0..2], false, buf[0..128]);
        expect_eq(buf[0..n] as string, "<button class=\"btn primary\" title=\"a &amp; &quot;b&quot;\">");
    });

    test("4. @cap/dom-serializer — serializeElement handles normal & void HTML elements", () => {
        const html_opts = SerializeOptions{ xml_mode: false, self_closing_tags: false };
        const attrs: [1]DomAttr = [DomAttr{ name: "src", value: "logo.png" }];
        let buf: [128]u8 = [0 as u8; 128];

        const n1 = serializeElement("img", attrs[0..1], "", html_opts, buf[0..128]);
        expect_eq(buf[0..n1] as string, "<img src=\"logo.png\">");

        const self_opts = SerializeOptions{ xml_mode: false, self_closing_tags: true };
        const n2 = serializeElement("img", attrs[0..1], "", self_opts, buf[0..128]);
        expect_eq(buf[0..n2] as string, "<img src=\"logo.png\"/>");

        const n3 = serializeElement("p", attrs[0..0], "hello", html_opts, buf[0..128]);
        expect_eq(buf[0..n3] as string, "<p>hello</p>");
    });

    test("5. @cap/dom-serializer — serializeElement supports xml_mode self-closing empty tags", () => {
        const xml_opts = SerializeOptions{ xml_mode: true, self_closing_tags: false };
        const attrs: [1]DomAttr = [DomAttr{ name: "id", value: "root" }];
        let buf: [128]u8 = [0 as u8; 128];

        const n1 = serializeElement("node", attrs[0..1], "", xml_opts, buf[0..128]);
        expect_eq(buf[0..n1] as string, "<node id=\"root\"/>");

        const n2 = serializeElement("node", attrs[0..1], "body", xml_opts, buf[0..128]);
        expect_eq(buf[0..n2] as string, "<node id=\"root\">body</node>");
    });

    test("6. @cap/dom-serializer — serializeComment and serializeCdata", () => {
        let buf: [128]u8 = [0 as u8; 128];
        const n1 = serializeComment(" note ", buf[0..128]);
        expect_eq(buf[0..n1] as string, "<!-- note -->");

        const n2 = serializeCdata("<raw>&data</raw>", buf[0..128]);
        expect_eq(buf[0..n2] as string, "<![CDATA[<raw>&data</raw>]]>");
    });
}
