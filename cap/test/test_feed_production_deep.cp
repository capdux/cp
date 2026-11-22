// Source: cp/cap/test/test_feed_production_deep.cp
// Deep Production Test Suite for cap:feed
// Covers:
//   1. RSS 2.0 full parsing (CDATA, multiple <category> tags, guid/pubDate/author/content:encoded)
//   2. Atom 1.0 full parsing (<link href=...>, <author><name>, <updated>, <summary>) & detectFormat
//   3. JSON Feed 1.1 parsing (tags, content_html, date_published, authors)
//   4. Cross-format round-trip (toRss2 / toAtom1 / toJsonFeed -> parse) & UnsupportedFormat rejection
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.feed`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { FeedError, FeedFormat, FeedItem, Feed, FeedCodec } from "cap:web";
import cap from "cap";

fn testRss2FullParsing()! {
    const alloc = std.heap.page_allocator;
    const rss =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
        "<rss version=\"2.0\" xmlns:content=\"http://purl.org/rss/1.0/modules/content/\">\n" ++
        "  <channel>\n" ++
        "    <title>CP Systems Engineering</title>\n" ++
        "    <link>https://cp-lang.org</link>\n" ++
        "    <description>Low-latency AI Agent runtime updates</description>\n" ++
        "    <language>en-us</language>\n" ++
        "    <lastBuildDate>Thu, 20 Nov 2026 12:00:00 GMT</lastBuildDate>\n" ++
        "    <item>\n" ++
        "      <title><![CDATA[Zero-Copy <CPS> Scheduling]]></title>\n" ++
        "      <link>https://cp-lang.org/posts/cps</link>\n" ++
        "      <guid>post-cps-001</guid>\n" ++
        "      <author>alice@cp-lang.org</author>\n" ++
        "      <pubDate>Thu, 20 Nov 2026 10:30:00 GMT</pubDate>\n" ++
        "      <description><![CDATA[Summary with & ampersand <tag>]]></description>\n" ++
        "      <content:encoded><![CDATA[<p>Full <strong>HTML</strong> body</p>]]></content:encoded>\n" ++
        "      <category>systems</category>\n" ++
        "      <category>beam</category>\n" ++
        "      <category>zig</category>\n" ++
        "    </item>\n" ++
        "  </channel>\n" ++
        "</rss>";

    let feed = FeedCodec.parse(alloc, rss) catch unreachable;
    defer feed.deinit();

    expect_eq(feed.format == .Rss2, true);
    expect_eq(std.mem.eql(u8, feed.title, "CP Systems Engineering"), true);
    expect_eq(std.mem.eql(u8, feed.link, "https://cp-lang.org"), true);
    expect_eq(std.mem.eql(u8, feed.description, "Low-latency AI Agent runtime updates"), true);
    expect_eq(std.mem.eql(u8, feed.language, "en-us"), true);
    expect_eq(std.mem.eql(u8, feed.updated, "Thu, 20 Nov 2026 12:00:00 GMT"), true);
    expect_eq(feed.items.len, 1 as usize);

    const item = feed.items[0];
    expect_eq(std.mem.eql(u8, item.id, "post-cps-001"), true);
    expect_eq(std.mem.eql(u8, item.title, "Zero-Copy <CPS> Scheduling"), true);
    expect_eq(std.mem.eql(u8, item.link, "https://cp-lang.org/posts/cps"), true);
    expect_eq(std.mem.eql(u8, item.author, "alice@cp-lang.org"), true);
    expect_eq(std.mem.eql(u8, item.published, "Thu, 20 Nov 2026 10:30:00 GMT"), true);
    expect_eq(std.mem.eql(u8, item.summary, "Summary with & ampersand <tag>"), true);
    expect_eq(std.mem.eql(u8, item.content_html, "<p>Full <strong>HTML</strong> body</p>"), true);
    expect_eq(item.tags.len, 3 as usize);
    expect_eq(std.mem.eql(u8, item.tags[0], "systems"), true);
    expect_eq(std.mem.eql(u8, item.tags[1], "beam"), true);
    expect_eq(std.mem.eql(u8, item.tags[2], "zig"), true);
}

fn testAtom1AndDetectFormat()! {
    const alloc = std.heap.page_allocator;
    const atom =
        "\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" ++
        "<!-- Atom 1.0 feed -->\n" ++
        "<feed xmlns=\"http://www.w3.org/2005/Atom\" xml:lang=\"zh-CN\">\n" ++
        "  <title>CP 架构周报</title>\n" ++
        "  <subtitle>纯 CP 标准库演进</subtitle>\n" ++
        "  <id>urn:cp:weekly</id>\n" ++
        "  <updated>2026-11-20T08:00:00Z</updated>\n" ++
        "  <link rel=\"self\" href=\"https://cp-lang.org/atom.xml\"/>\n" ++
        "  <link rel=\"alternate\" href=\"https://cp-lang.org/weekly\"/>\n" ++
        "  <entry>\n" ++
        "    <title>cap:feed 发布</title>\n" ++
        "    <id>urn:cp:weekly:42</id>\n" ++
        "    <link rel=\"alternate\" href=\"https://cp-lang.org/weekly/42\"/>\n" ++
        "    <published>2026-11-20T07:00:00Z</published>\n" ++
        "    <updated>2026-11-20T08:00:00Z</updated>\n" ++
        "    <author><name>Bob</name><email>bob@cp-lang.org</email></author>\n" ++
        "    <summary>统一解析与生成引擎</summary>\n" ++
        "    <content type=\"html\">&lt;p&gt;Pure CP&lt;/p&gt;</content>\n" ++
        "    <category term=\"stdlib\"/>\n" ++
        "    <category term=\"feed\"/>\n" ++
        "  </entry>\n" ++
        "</feed>";

    const detected = FeedCodec.detectFormat(atom) catch unreachable;
    expect_eq(detected == .Atom1, true);

    let feed = FeedCodec.parse(alloc, atom) catch unreachable;
    defer feed.deinit();

    expect_eq(feed.format == .Atom1, true);
    expect_eq(std.mem.eql(u8, feed.title, "CP 架构周报"), true);
    expect_eq(std.mem.eql(u8, feed.link, "https://cp-lang.org/weekly"), true);
    expect_eq(std.mem.eql(u8, feed.description, "纯 CP 标准库演进"), true);
    expect_eq(std.mem.eql(u8, feed.language, "zh-CN"), true);
    expect_eq(std.mem.eql(u8, feed.updated, "2026-11-20T08:00:00Z"), true);
    expect_eq(feed.items.len, 1 as usize);

    const entry = feed.items[0];
    expect_eq(std.mem.eql(u8, entry.id, "urn:cp:weekly:42"), true);
    expect_eq(std.mem.eql(u8, entry.title, "cap:feed 发布"), true);
    expect_eq(std.mem.eql(u8, entry.link, "https://cp-lang.org/weekly/42"), true);
    expect_eq(std.mem.eql(u8, entry.author, "Bob"), true);
    expect_eq(std.mem.eql(u8, entry.published, "2026-11-20T07:00:00Z"), true);
    expect_eq(std.mem.eql(u8, entry.summary, "统一解析与生成引擎"), true);
    expect_eq(std.mem.eql(u8, entry.content_html, "<p>Pure CP</p>"), true);
    expect_eq(entry.tags.len, 2 as usize);
    expect_eq(std.mem.eql(u8, entry.tags[0], "stdlib"), true);
    expect_eq(std.mem.eql(u8, entry.tags[1], "feed"), true);
}

fn testJsonFeed11Parsing()! {
    const alloc = std.heap.page_allocator;
    const jf =
        "{\n" ++
        "  \"version\": \"https://jsonfeed.org/version/1.1\",\n" ++
        "  \"title\": \"Agent Knowledge Sync\",\n" ++
        "  \"home_page_url\": \"https://sync.cp-lang.org\",\n" ++
        "  \"description\": \"Incremental KB feed\",\n" ++
        "  \"language\": \"en\",\n" ++
        "  \"items\": [\n" ++
        "    {\n" ++
        "      \"id\": \"kb-99\",\n" ++
        "      \"url\": \"https://sync.cp-lang.org/kb/99\",\n" ++
        "      \"title\": \"RAG Chunking\",\n" ++
        "      \"summary\": \"Fast chunking\",\n" ++
        "      \"content_html\": \"<article>Chunking details</article>\",\n" ++
        "      \"date_published\": \"2026-11-20T09:15:00Z\",\n" ++
        "      \"authors\": [{\"name\": \"Carol\"}],\n" ++
        "      \"tags\": [\"ai\", \"rag\"]\n" ++
        "    }\n" ++
        "  ]\n" ++
        "}";

    const detected = FeedCodec.detectFormat(jf) catch unreachable;
    expect_eq(detected == .JsonFeed1, true);

    let feed = FeedCodec.parse(alloc, jf) catch unreachable;
    defer feed.deinit();

    expect_eq(feed.format == .JsonFeed1, true);
    expect_eq(std.mem.eql(u8, feed.title, "Agent Knowledge Sync"), true);
    expect_eq(std.mem.eql(u8, feed.link, "https://sync.cp-lang.org"), true);
    expect_eq(std.mem.eql(u8, feed.description, "Incremental KB feed"), true);
    expect_eq(std.mem.eql(u8, feed.language, "en"), true);
    expect_eq(feed.items.len, 1 as usize);

    const it = feed.items[0];
    expect_eq(std.mem.eql(u8, it.id, "kb-99"), true);
    expect_eq(std.mem.eql(u8, it.title, "RAG Chunking"), true);
    expect_eq(std.mem.eql(u8, it.link, "https://sync.cp-lang.org/kb/99"), true);
    expect_eq(std.mem.eql(u8, it.summary, "Fast chunking"), true);
    expect_eq(std.mem.eql(u8, it.content_html, "<article>Chunking details</article>"), true);
    expect_eq(std.mem.eql(u8, it.author, "Carol"), true);
    expect_eq(std.mem.eql(u8, it.published, "2026-11-20T09:15:00Z"), true);
    expect_eq(it.tags.len, 2 as usize);
    expect_eq(std.mem.eql(u8, it.tags[0], "ai"), true);
    expect_eq(std.mem.eql(u8, it.tags[1], "rag"), true);
}

fn testCrossFormatRoundTripAndErrors()! {
    const alloc = std.heap.page_allocator;

    let tags_arr: [2][]const u8 = ["cp", "feed"];
    let items_arr: [1]FeedItem = [FeedItem{
        id: "item-1",
        title: "RoundTrip & Escape <Test>",
        link: "https://cp-lang.org/item-1",
        summary: "Short summary & notes",
        content_html: "<p>Hello <b>World</b></p>",
        author: "Dana",
        published: "2026-11-20T12:00:00Z",
        tags: tags_arr[0..2],
    }];

    let src_feed = Feed.init(.Rss2);
    src_feed.title = "Unified Feed";
    src_feed.link = "https://cp-lang.org";
    src_feed.description = "Cross-format test";
    src_feed.language = "en";
    src_feed.updated = "2026-11-20T12:00:00Z";
    src_feed.items = items_arr[0..1];

    // 1. RSS 2.0 round-trip
    const rss_bytes = FeedCodec.toRss2(alloc, src_feed) catch unreachable;
    defer alloc.free(rss_bytes);
    let parsed_rss = FeedCodec.parse(alloc, rss_bytes) catch unreachable;
    defer parsed_rss.deinit();
    expect_eq(parsed_rss.format == .Rss2, true);
    expect_eq(std.mem.eql(u8, parsed_rss.title, "Unified Feed"), true);
    expect_eq(parsed_rss.items.len, 1 as usize);
    expect_eq(std.mem.eql(u8, parsed_rss.items[0].title, "RoundTrip & Escape <Test>"), true);
    expect_eq(std.mem.eql(u8, parsed_rss.items[0].content_html, "<p>Hello <b>World</b></p>"), true);
    expect_eq(parsed_rss.items[0].tags.len, 2 as usize);

    // 2. Atom 1.0 round-trip
    const atom_bytes = FeedCodec.toAtom1(alloc, parsed_rss) catch unreachable;
    defer alloc.free(atom_bytes);
    let parsed_atom = FeedCodec.parse(alloc, atom_bytes) catch unreachable;
    defer parsed_atom.deinit();
    expect_eq(parsed_atom.format == .Atom1, true);
    expect_eq(std.mem.eql(u8, parsed_atom.title, "Unified Feed"), true);
    expect_eq(parsed_atom.items.len, 1 as usize);
    expect_eq(std.mem.eql(u8, parsed_atom.items[0].author, "Dana"), true);
    expect_eq(std.mem.eql(u8, parsed_atom.items[0].tags[1], "feed"), true);

    // 3. JSON Feed 1.1 round-trip
    const jf_bytes = FeedCodec.toJsonFeed(alloc, parsed_atom) catch unreachable;
    defer alloc.free(jf_bytes);
    let parsed_jf = FeedCodec.parse(alloc, jf_bytes) catch unreachable;
    defer parsed_jf.deinit();
    expect_eq(parsed_jf.format == .JsonFeed1, true);
    expect_eq(std.mem.eql(u8, parsed_jf.title, "Unified Feed"), true);
    expect_eq(parsed_jf.items.len, 1 as usize);
    expect_eq(std.mem.eql(u8, parsed_jf.items[0].id, "item-1"), true);
    expect_eq(std.mem.eql(u8, parsed_jf.items[0].content_html, "<p>Hello <b>World</b></p>"), true);

    // 4. UnsupportedFormat rejection
    let bad_html_err = false;
    * = FeedCodec.detectFormat("<html><body>Not a feed</body></html>") catch e => {
        if (e == FeedError.UnsupportedFormat) {
            bad_html_err = true;
        }
    };
    expect_eq(bad_html_err, true);

    let bad_json_err = false;
    * = FeedCodec.parse(alloc, "{\"hello\": \"world\"}") catch e => {
        if (e == FeedError.UnsupportedFormat) {
            bad_json_err = true;
        }
    };
    expect_eq(bad_json_err, true);
}

fn testDynamicCapFeedMount()! {
    const alloc = std.heap.page_allocator;
    const minimal_rss =
        "<rss version=\"2.0\"><channel><title>Cap Dynamic</title>" ++
        "<item><title>Mounted</title><link>https://cp-lang.org</link></item>" ++
        "</channel></rss>";

    const fmt = cap.web.FeedCodec.detectFormat(minimal_rss) catch unreachable;
    expect_eq(fmt == cap.web.FeedFormat.Rss2, true);

    let f = cap.web.FeedCodec.parse(alloc, minimal_rss) catch unreachable;
    defer f.deinit();
    expect_eq(std.mem.eql(u8, f.title, "Cap Dynamic"), true);
    expect_eq(f.items.len, 1 as usize);
    expect_eq(std.mem.eql(u8, f.items[0].title, "Mounted"), true);
}

test("feed_rss2_full_parsing", testRss2FullParsing);
test("feed_atom1_and_detect_format", testAtom1AndDetectFormat);
test("feed_json_feed_1_1_parsing", testJsonFeed11Parsing);
test("feed_cross_format_roundtrip_and_errors", testCrossFormatRoundTripAndErrors);
test("feed_dynamic_cap_mount", testDynamicCapFeedMount);
