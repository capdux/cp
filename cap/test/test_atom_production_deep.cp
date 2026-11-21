// Source: cp/cap/test/test_atom_production_deep.cp (Production deep tests for cap:atom)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    AtomFeed,
    AtomEntry,
    isAtomFeed,
    parseAtomFeed,
    AtomEntryIter,
    formatAtomEntry,
} from "cap:atom";

const SAMPLE_ATOM: []const u8 =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" ++
    "<feed xmlns=\"http://www.w3.org/2005/Atom\">\n" ++
    "  <!-- Feed header comment -->\n" ++
    "  <title><![CDATA[CP Lang Blog]]></title>\n" ++
    "  <subtitle>Systems &amp; Agent Engineering</subtitle>\n" ++
    "  <link rel=\"self\" href=\"https://example.org/atom.xml\"/>\n" ++
    "  <link rel=\"alternate\" href=\"https://example.org/\"/>\n" ++
    "  <updated>2026-11-21T12:00:00Z</updated>\n" ++
    "  <id>urn:uuid:60a76c80-d399-11d9-b93C-0003939e0af6</id>\n" ++
    "  <entry>\n" ++
    "    <title>Atom-Powered Robots Run Amok</title>\n" ++
    "    <link href=\"https://example.org/2026/11/robots\"/>\n" ++
    "    <id>urn:uuid:1225c695-cfb8-4ebb-aaaa-80da344efa6a</id>\n" ++
    "    <updated>2026-11-21T11:30:00Z</updated>\n" ++
    "    <author><name>Ada Lovelace</name></author>\n" ++
    "    <summary><![CDATA[Zero-alloc parser shipped!]]></summary>\n" ++
    "  </entry>\n" ++
    "  <entry>\n" ++
    "    <title>Second Release</title>\n" ++
    "    <link rel='alternate' href='https://example.org/2026/11/second'/>\n" ++
    "    <id>urn:uuid:2225c695-cfb8-4ebb-bbbb-80da344efa6b</id>\n" ++
    "    <updated>2026-11-21T10:00:00Z</updated>\n" ++
    "  </entry>\n" ++
    "</feed>";

test("1. isAtomFeed & parseAtomFeed extract feed metadata, CDATA, and alternate link", () => {
    expect_eq(isAtomFeed(SAMPLE_ATOM), true);
    const feed = parseAtomFeed(SAMPLE_ATOM) ?? unreachable;

    expect_eq(std.mem.eql(u8, feed.id, "urn:uuid:60a76c80-d399-11d9-b93C-0003939e0af6"), true);
    expect_eq(std.mem.eql(u8, feed.title, "CP Lang Blog"), true);
    expect_eq(std.mem.eql(u8, feed.updated, "2026-11-21T12:00:00Z"), true);
    expect_eq(std.mem.eql(u8, feed.link_href ?? "", "https://example.org/"), true);
    expect_eq(std.mem.eql(u8, feed.subtitle ?? "", "Systems &amp; Agent Engineering"), true);
});

test("2. parseAtomFeed skips entry child tags even when feed metadata appears after entry", () => {
    const reversed =
        "<feed xmlns=\"http://www.w3.org/2005/Atom\">" ++
        "<entry><id>entry-id</id><title>Entry Title</title><updated>2026-01-01T00:00:00Z</updated><link href=\"https://entry.example/\"/></entry>" ++
        "<id>feed-id</id><title>Feed Title</title><updated>2026-11-21T00:00:00Z</updated><link href=\"https://feed.example/\"/>" ++
        "</feed>";
    const feed = parseAtomFeed(reversed) ?? unreachable;
    expect_eq(std.mem.eql(u8, feed.id, "feed-id"), true);
    expect_eq(std.mem.eql(u8, feed.title, "Feed Title"), true);
    expect_eq(std.mem.eql(u8, feed.updated, "2026-11-21T00:00:00Z"), true);
    expect_eq(std.mem.eql(u8, feed.link_href ?? "", "https://feed.example/"), true);
});

test("3. AtomEntryIter iterates multiple entries with author, summary, CDATA, and single-quoted links", () => {
    let it = AtomEntryIter.init(SAMPLE_ATOM);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.id, "urn:uuid:1225c695-cfb8-4ebb-aaaa-80da344efa6a"), true);
    expect_eq(std.mem.eql(u8, e1.title, "Atom-Powered Robots Run Amok"), true);
    expect_eq(std.mem.eql(u8, e1.updated, "2026-11-21T11:30:00Z"), true);
    expect_eq(std.mem.eql(u8, e1.link_href ?? "", "https://example.org/2026/11/robots"), true);
    expect_eq(std.mem.eql(u8, e1.author_name ?? "", "Ada Lovelace"), true);
    expect_eq(std.mem.eql(u8, e1.summary ?? "", "Zero-alloc parser shipped!"), true);

    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.id, "urn:uuid:2225c695-cfb8-4ebb-bbbb-80da344efa6b"), true);
    expect_eq(std.mem.eql(u8, e2.title, "Second Release"), true);
    expect_eq(std.mem.eql(u8, e2.link_href ?? "", "https://example.org/2026/11/second"), true);
    expect_eq(e2.author_name == null, true);
    expect_eq(e2.summary == null, true);

    expect_eq(it.next() == null, true);
});

test("4. formatAtomEntry escapes XML special characters and round-trips through AtomEntryIter", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    const entry = AtomEntry {
        id: "tag:example.org,2026:1",
        title: "A & B <C> \"D\" 'E'",
        updated: "2026-11-21T15:00:00Z",
        link_href: "https://example.org/p?a=1&b=2",
        summary: "x < y && z > 0",
        author_name: "O'Connor & Co",
    };

    const xml = formatAtomEntry(entry, buf[0..]) ?? unreachable;
    const expected =
        "<entry><id>tag:example.org,2026:1</id>" ++
        "<title>A &amp; B &lt;C&gt; &quot;D&quot; &apos;E&apos;</title>" ++
        "<updated>2026-11-21T15:00:00Z</updated>" ++
        "<link rel=\"alternate\" href=\"https://example.org/p?a=1&amp;b=2\"/>" ++
        "<author><name>O&apos;Connor &amp; Co</name></author>" ++
        "<summary>x &lt; y &amp;&amp; z &gt; 0</summary></entry>";
    expect_eq(std.mem.eql(u8, xml, expected), true);

    // Round-trip parse check
    let it = AtomEntryIter.init(xml);
    const parsed = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, parsed.id, "tag:example.org,2026:1"), true);
    expect_eq(std.mem.eql(u8, parsed.updated, "2026-11-21T15:00:00Z"), true);
    expect_eq(std.mem.eql(u8, parsed.author_name ?? "", "O&apos;Connor &amp; Co"), true);
});

test("5. Malformed feed rejection and formatAtomEntry buffer overflow protection", () => {
    expect_eq(isAtomFeed(""), false);
    expect_eq(isAtomFeed("<rss><channel></channel></rss>"), false);
    expect_eq(isAtomFeed("<feed><title>unclosed"), false);
    expect_eq(isAtomFeed("<feed/>"), false);
    expect_eq(parseAtomFeed("<feed><id>only-id</id></feed>") == null, true);

    // Buffer too small returns null cleanly
    let tiny: [32]u8 = std.mem.zeroes([32]u8);
    const entry = AtomEntry {
        id: "id-1",
        title: "Title",
        updated: "2026-11-21T00:00:00Z",
    };
    expect_eq(formatAtomEntry(entry, tiny[0..]) == null, true);
    expect_eq(formatAtomEntry(entry, tiny[0..0]) == null, true);
});
