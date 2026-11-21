// Source: cp/cap/test/test_opml_production_deep.cp (Production deep tests for cap:opml)

import { test, expect_eq } from "cap:test";
import {
    OpmlHead,
    OpmlOutline,
    isOpmlDocument,
    parseOpmlHead,
    OpmlOutlineIter,
    formatOpmlFeedOutline,
} from "cap:opml";

const SAMPLE_OPML =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
    "<!-- Subscription Export -->\n" ++
    "<opml version=\"2.0\">\n" ++
    "  <head>\n" ++
    "    <title>Engineering Feeds</title>\n" ++
    "    <dateCreated>Sat, 21 Nov 2026 10:00:00 GMT</dateCreated>\n" ++
    "    <ownerName><![CDATA[CP Systems Team]]></ownerName>\n" ++
    "  </head>\n" ++
    "  <body>\n" ++
    "    <outline text=\"Tech\" title=\"Technology Folder\">\n" ++
    "      <outline type=\"rss\" text=\"CP Blog\" title=\"CP Official\" xmlUrl=\"https://cp-lang.org/rss.xml\" htmlUrl=\"https://cp-lang.org\"/>\n" ++
    "      <outline text='Subfolder'>\n" ++
    "        <outline type='rss' text='Deep Feed' xmlUrl='https://deep.example/feed.xml' />\n" ++
    "      </outline>\n" ++
    "    </outline>\n" ++
    "    <outline type=\"rss\" text=\"Top Level Feed\" xmlUrl=\"https://top.example/rss\"/>\n" ++
    "  </body>\n" ++
    "</opml>\n";

fn main() {
    test("1. isOpmlDocument validates OPML with BOM/PI/comments and rejects RSS/HTML/malformed", () => {
        expect_eq(isOpmlDocument(SAMPLE_OPML), true);
        expect_eq(isOpmlDocument("\xEF\xBB\xBF<opml version=\"2.0\"><body/></opml>"), true);
        expect_eq(isOpmlDocument("<rss version=\"2.0\"><channel></channel></rss>"), false);
        expect_eq(isOpmlDocument("<html><body>not opml</body></html>"), false);
        expect_eq(isOpmlDocument("<opml version=\"2.0\"><head></head></opml>"), false);
        expect_eq(isOpmlDocument(""), false);
    });

    test("2. parseOpmlHead extracts title, dateCreated and CDATA ownerName", () => {
        const head_opt = parseOpmlHead(SAMPLE_OPML);
        expect_eq(head_opt != null, true);
        if (head := head_opt) {
            expect_eq(head.title != null, true);
            expect_eq(head.title.?, "Engineering Feeds");
            expect_eq(head.date_created != null, true);
            expect_eq(head.date_created.?, "Sat, 21 Nov 2026 10:00:00 GMT");
            expect_eq(head.owner_name != null, true);
            expect_eq(head.owner_name.?, "CP Systems Team");
        }
        expect_eq(parseOpmlHead("<opml><body></body></opml>") == null, true);
    });

    test("3. OpmlOutlineIter tracks nested depth, self-closing tags, single/double quotes and isFeed", () => {
        let it = OpmlOutlineIter.init(SAMPLE_OPML);

        // 0: Tech folder (depth 0)
        const n0 = it.next();
        expect_eq(n0 != null, true);
        if (o := n0) {
            expect_eq(o.text, "Tech");
            expect_eq(o.title.?, "Technology Folder");
            expect_eq(o.depth, 0 as u16);
            expect_eq(o.isFeed(), false);
        }

        // 1: CP Blog feed (depth 1, self-closing)
        const n1 = it.next();
        expect_eq(n1 != null, true);
        if (o := n1) {
            expect_eq(o.text, "CP Blog");
            expect_eq(o.kind.?, "rss");
            expect_eq(o.xml_url.?, "https://cp-lang.org/rss.xml");
            expect_eq(o.html_url.?, "https://cp-lang.org");
            expect_eq(o.depth, 1 as u16);
            expect_eq(o.isFeed(), true);
        }

        // 2: Subfolder (depth 1, single quotes)
        const n2 = it.next();
        expect_eq(n2 != null, true);
        if (o := n2) {
            expect_eq(o.text, "Subfolder");
            expect_eq(o.depth, 1 as u16);
            expect_eq(o.isFeed(), false);
        }

        // 3: Deep Feed (depth 2, self-closing with space before />)
        const n3 = it.next();
        expect_eq(n3 != null, true);
        if (o := n3) {
            expect_eq(o.text, "Deep Feed");
            expect_eq(o.xml_url.?, "https://deep.example/feed.xml");
            expect_eq(o.html_url == null, true);
            expect_eq(o.depth, 2 as u16);
            expect_eq(o.isFeed(), true);
        }

        // 4: Top Level Feed (back to depth 0 after two </outline>)
        const n4 = it.next();
        expect_eq(n4 != null, true);
        if (o := n4) {
            expect_eq(o.text, "Top Level Feed");
            expect_eq(o.depth, 0 as u16);
            expect_eq(o.isFeed(), true);
        }

        expect_eq(it.next() == null, true);
    });

    test("4. formatOpmlFeedOutline escapes XML attributes, handles optional htmlUrl and small buffers", () => {
        let buf: [512]u8 = undefined;
        const with_html = formatOpmlFeedOutline(
            "R&D <News> \"2026\" 'A'",
            "https://example.com/rss?a=1&b=2",
            "https://example.com/?x=1&y=2",
            &buf,
        );
        expect_eq(with_html != null, true);
        if (line := with_html) {
            const expected =
                "<outline type=\"rss\" text=\"R&amp;D &lt;News&gt; &quot;2026&quot; &apos;A&apos;\" " ++
                "xmlUrl=\"https://example.com/rss?a=1&amp;b=2\" " ++
                "htmlUrl=\"https://example.com/?x=1&amp;y=2\"/>";
            expect_eq(line, expected);
        }

        const no_html = formatOpmlFeedOutline("Minimal", "https://min.example/rss", null, &buf);
        expect_eq(no_html != null, true);
        if (line := no_html) {
            expect_eq(line, "<outline type=\"rss\" text=\"Minimal\" xmlUrl=\"https://min.example/rss\"/>");
        }

        let tiny: [24]u8 = undefined;
        expect_eq(formatOpmlFeedOutline("Minimal", "https://min.example/rss", null, &tiny) == null, true);
    });

    test("5. Malformed OPML, extra closing tags, comments and '>' inside attribute quotes are handled safely", () => {
        // Extra </outline> must not underflow depth; '>' inside attribute value must not truncate tag
        const tricky =
            "<opml version=\"2.0\"><body>" ++
            "</outline></outline>" ++
            "<!-- <outline text=\"commented\"/> -->" ++
            "<outline text=\"A > B\" xmlUrl=\"https://ok.example/rss\"/>" ++
            "<outline text=\"Unclosed" ++
            "</body></opml>";
        let it = OpmlOutlineIter.init(tricky);
        const first = it.next();
        expect_eq(first != null, true);
        if (o := first) {
            expect_eq(o.text, "A > B");
            expect_eq(o.xml_url.?, "https://ok.example/rss");
            expect_eq(o.depth, 0 as u16);
        }
        // Unclosed outline terminates cleanly
        expect_eq(it.next() == null, true);
    });
}
