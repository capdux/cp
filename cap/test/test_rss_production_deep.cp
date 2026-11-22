// Source: cp/cap/test/test_rss_production_deep.cp (Production deep tests for cap:rss)

import { test, expect_eq } from "cap:test";
import {
    RssChannel,
    RssItem,
    isRss2Feed,
    parseRssChannel,
    RssItemIter,
    formatRssItem,
} from "cap:doc";

const SAMPLE_FEED =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
    "<!-- Leading feed comment -->\n" ++
    "<rss version=\"2.0\" xmlns:atom=\"http://www.w3.org/2005/Atom\">\n" ++
    "  <channel>\n" ++
    "    <title><![CDATA[CP Standard Library News]]></title>\n" ++
    "    <link>https://cp-lang.org/news</link>\n" ++
    "    <description>Zero-alloc systems programming updates</description>\n" ++
    "    <image>\n" ++
    "      <title>Logo Title Should Not Override</title>\n" ++
    "      <link>https://cp-lang.org/logo.png</link>\n" ++
    "    </image>\n" ++
    "    <item>\n" ++
    "      <title>Release 1.0</title>\n" ++
    "      <link>https://cp-lang.org/news/1-0</link>\n" ++
    "      <description><![CDATA[<p>Hello & <b>World</b></p>]]></description>\n" ++
    "      <guid isPermaLink=\"false\">urn:cp:release:1.0</guid>\n" ++
    "      <pubDate>Sat, 21 Nov 2026 12:00:00 GMT</pubDate>\n" ++
    "      <author>dev@cp-lang.org</author>\n" ++
    "    </item>\n" ++
    "    <item xml:lang=\"en\">\n" ++
    "      <title>  Zero-Copy Streams  </title>\n" ++
    "      <link>https://cp-lang.org/news/streams</link>\n" ++
    "      <description>Fast IO</description>\n" ++
    "    </item>\n" ++
    "    <language>en-us</language>\n" ++
    "    <lastBuildDate>Sat, 21 Nov 2026 18:00:00 GMT</lastBuildDate>\n" ++
    "  </channel>\n" ++
    "</rss>\n";

fn main() {
    test("1. isRss2Feed detects valid RSS 2.0 with BOM/PI/comments and rejects Atom/HTML/malformed", () => {
        expect_eq(isRss2Feed(SAMPLE_FEED), true);
        expect_eq(isRss2Feed("\xEF\xBB\xBF<rss version=\"2.0\"><channel></channel></rss>"), true);
        expect_eq(isRss2Feed("<?xml version=\"1.0\"?><feed xmlns=\"http://www.w3.org/2005/Atom\"></feed>"), false);
        expect_eq(isRss2Feed("<html><body><channel>nope</channel></body></html>"), false);
        expect_eq(isRss2Feed("<rss version=\"2.0\">no channel here</rss>"), false);
        expect_eq(isRss2Feed(""), false);
    });

    test("2. parseRssChannel extracts channel metadata, skips item/image subtrees, strips CDATA", () => {
        const ch_opt = parseRssChannel(SAMPLE_FEED);
        expect_eq(ch_opt != null, true);
        if (ch := ch_opt) {
            expect_eq(ch.title, "CP Standard Library News");
            expect_eq(ch.link, "https://cp-lang.org/news");
            expect_eq(ch.description, "Zero-alloc systems programming updates");
            expect_eq(ch.language != null, true);
            expect_eq(ch.language.?, "en-us");
            expect_eq(ch.last_build_date != null, true);
            expect_eq(ch.last_build_date.?, "Sat, 21 Nov 2026 18:00:00 GMT");
        }
    });

    test("3. parseRssChannel ignores item fields even when channel has no own title/link", () => {
        const feed =
            "<rss version=\"2.0\"><channel>" ++
            "<item><title>Item Only</title><link>https://item.example</link></item>" ++
            "<description>Only Desc</description>" ++
            "</channel></rss>";
        const ch_opt = parseRssChannel(feed);
        expect_eq(ch_opt != null, true);
        if (ch := ch_opt) {
            expect_eq(ch.title, "");
            expect_eq(ch.link, "");
            expect_eq(ch.description, "Only Desc");
            expect_eq(ch.language == null, true);
            expect_eq(ch.last_build_date == null, true);
        }
    });

    test("4. RssItemIter iterates items with attributes, CDATA, optional fields and termination", () => {
        let it = RssItemIter.init(SAMPLE_FEED);

        const first_opt = it.next();
        expect_eq(first_opt != null, true);
        if (item1 := first_opt) {
            expect_eq(item1.title, "Release 1.0");
            expect_eq(item1.link, "https://cp-lang.org/news/1-0");
            expect_eq(item1.description, "<p>Hello & <b>World</b></p>");
            expect_eq(item1.guid != null, true);
            expect_eq(item1.guid.?, "urn:cp:release:1.0");
            expect_eq(item1.pub_date != null, true);
            expect_eq(item1.pub_date.?, "Sat, 21 Nov 2026 12:00:00 GMT");
            expect_eq(item1.author != null, true);
            expect_eq(item1.author.?, "dev@cp-lang.org");
        }

        const second_opt = it.next();
        expect_eq(second_opt != null, true);
        if (item2 := second_opt) {
            expect_eq(item2.title, "Zero-Copy Streams");
            expect_eq(item2.link, "https://cp-lang.org/news/streams");
            expect_eq(item2.description, "Fast IO");
            expect_eq(item2.guid == null, true);
            expect_eq(item2.pub_date == null, true);
            expect_eq(item2.author == null, true);
        }

        expect_eq(it.next() == null, true);
        expect_eq(it.next() == null, true);
    });

    test("5. formatRssItem XML-escapes special chars, omits null optionals, and guards small buffers", () => {
        const item = RssItem{
            title: "A & B <C> \"D\" 'E'",
            link: "https://example.com/?a=1&b=2",
            description: "1 < 2 && 3 > 2",
            guid: "id-42",
            pub_date: "Sat, 21 Nov 2026 12:00:00 GMT",
            author: null,
        };
        let buf: [512]u8 = undefined;
        const out_opt = formatRssItem(item, &buf);
        expect_eq(out_opt != null, true);
        if (xml := out_opt) {
            const expected =
                "<item>" ++
                "<title>A &amp; B &lt;C&gt; &quot;D&quot; &apos;E&apos;</title>" ++
                "<link>https://example.com/?a=1&amp;b=2</link>" ++
                "<description>1 &lt; 2 &amp;&amp; 3 &gt; 2</description>" ++
                "<guid>id-42</guid>" ++
                "<pubDate>Sat, 21 Nov 2026 12:00:00 GMT</pubDate>" ++
                "</item>";
            expect_eq(xml, expected);
        }

        // Small buffer protection
        let tiny: [16]u8 = undefined;
        expect_eq(formatRssItem(item, &tiny) == null, true);
    });

    test("6. Malformed XML, unclosed tags, self-closing tags and CDATA fake close tags are handled safely", () => {
        // Unclosed channel / item
        expect_eq(parseRssChannel("<rss><channel><title>Unclosed") == null, true);
        let bad_it = RssItemIter.init("<rss><channel><item><title>Oops");
        expect_eq(bad_it.next() == null, true);

        // Self-closing tag & fake closing tag inside CDATA & '>' inside attribute quotes
        const tricky =
            "<rss version=\"2.0\"><channel>" ++
            "<item data-expr=\"a > b\">" ++
            "<title/>" ++
            "<link>https://example.com</link>" ++
            "<description><![CDATA[contains </description> and </item> safely]]></description>" ++
            "</item>" ++
            "</channel></rss>";
        let it = RssItemIter.init(tricky);
        const item_opt = it.next();
        expect_eq(item_opt != null, true);
        if (item := item_opt) {
            expect_eq(item.title, "");
            expect_eq(item.link, "https://example.com");
            expect_eq(item.description, "contains </description> and </item> safely");
        }
        expect_eq(it.next() == null, true);
    });
}
