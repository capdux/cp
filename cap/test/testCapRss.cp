import { test, expect_eq } from "cap:test";
import { RssChannel, RssItem, RssWriter, extractTagValue, countItems } from "../../packages/rss/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn containsSlice(haystack: []const u8, needle: []const u8) -> bool {
    if (needle.len == 0 || needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (testEq(haystack[i..i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn main() {
    test("1. @cap/rss — Single & multi-item RSS 2.0 generation & countItems", () => {
        let buf: [1024]u8 = undefined;
        let w = RssWriter.init(buf[0..1024], RssChannel {
            title: "CP Lang Blog",
            link: "https://cp.dev",
            description: "Systems language updates",
            language: "en-US",
        });
        expect_eq(w.addItem(RssItem {
            title: "Release v1.0",
            link: "https://cp.dev/blog/v1",
            description: "First stable release",
            guid: "post-1",
            pub_date: "Thu, 26 Nov 2026 12:00:00 GMT",
        }), true);
        expect_eq(w.addItem(RssItem {
            title: "Zero-Alloc Guide",
            link: "https://cp.dev/blog/zero-alloc",
            description: "Writing fast packages",
            guid: "post-2",
            pub_date: "Thu, 26 Nov 2026 13:00:00 GMT",
        }), true);

        expect_eq(w.itemCount(), 2 as usize);
        const total = w.finish();
        expect_eq(total > 0, true);
        const xml = w.written();
        expect_eq(countItems(xml), 2 as usize);
        expect_eq(containsSlice(xml, "<rss version=\"2.0\">"), true);
        expect_eq(containsSlice(xml, "</channel>\n</rss>\n"), true);
    });

    test("2. @cap/rss — Strict XML entity escaping for <script>&\"'", () => {
        let buf: [512]u8 = undefined;
        let w = RssWriter.init(buf[0..512], RssChannel {
            title: "R&D <News>",
            link: "https://cp.dev",
            description: "Quotes \"double\" & 'single'",
            language: "",
        });
        * = w.addItem(RssItem {
            title: "<script>alert(\"x\")</script>",
            link: "https://cp.dev/?a=1&b=2",
            description: "",
            guid: "",
            pub_date: "",
        });
        expect_eq(w.finish() > 0, true);
        const xml = w.written();
        expect_eq(containsSlice(xml, "R&amp;D &lt;News&gt;"), true);
        expect_eq(containsSlice(xml, "&lt;script&gt;alert(&quot;x&quot;)&lt;/script&gt;"), true);
        expect_eq(containsSlice(xml, "&apos;single&apos;"), true);
    });

    test("3. @cap/rss — Empty optional fields are omitted", () => {
        let buf: [512]u8 = undefined;
        let w = RssWriter.init(buf[0..512], RssChannel {
            title: "Minimal",
            link: "https://cp.dev",
            description: "Desc",
            language: "",
        });
        * = w.addItem(RssItem {
            title: "Only Title",
            link: "",
            description: "",
            guid: "",
            pub_date: "",
        });
        expect_eq(w.finish() > 0, true);
        const xml = w.written();
        expect_eq(containsSlice(xml, "<language>"), false);
        expect_eq(containsSlice(xml, "<guid>"), false);
        expect_eq(containsSlice(xml, "<pubDate>"), false);
    });

    test("4. @cap/rss — extractTagValue handles plain tags & CDATA unwrapping", () => {
        const xml = "<rss><channel><title>Plain Title</title><item><title><![CDATA[<b>Bold & Fast</b>]]></title><guid isPermaLink=\"false\">id-99</guid></item></channel></rss>";
        const ch_title = extractTagValue(xml, "title", 0);
        expect_eq(testEq(ch_title, "Plain Title"), true);

        const item_title = extractTagValue(xml, "title", 35);
        expect_eq(testEq(item_title, "<b>Bold & Fast</b>"), true);

        const guid = extractTagValue(xml, "guid", 0);
        expect_eq(testEq(guid, "id-99"), true);

        expect_eq(testEq(extractTagValue(xml, "missing", 0), ""), true);
    });

    test("5. @cap/rss — Empty channel & rejecting addItem after finish()", () => {
        let buf: [256]u8 = undefined;
        let w = RssWriter.init(buf[0..256], RssChannel {
            title: "Empty",
            link: "https://cp.dev",
            description: "No items",
            language: "",
        });
        expect_eq(w.finish() > 0, true);
        expect_eq(countItems(w.written()), 0 as usize);

        // Appending after finish() must be rejected
        expect_eq(w.addItem(RssItem {
            title: "Late",
            link: "",
            description: "",
            guid: "",
            pub_date: "",
        }), false);
        expect_eq(w.itemCount(), 0 as usize);
    });

    test("6. @cap/rss — Undersized buffer returns -1 on finish()", () => {
        let tiny: [32]u8 = undefined;
        let w = RssWriter.init(tiny[0..32], RssChannel {
            title: "Too Big For Buffer",
            link: "https://cp.dev",
            description: "Overflow",
            language: "en",
        });
        expect_eq(w.finish(), -1 as i64);
    });
}
