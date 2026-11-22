// Source: cp/lib/feed/feed.cp (RSS 2.0 / Atom 1.0 / JSON Feed 1.1 Parser & Codec)
// Pure CP, zero-C-dependency unified syndication feed engine for RSS 2.0,
// Atom 1.0, and JSON Feed 1.1: format auto-detection, DOM/AST parsing,
// and lossless cross-format serialization.

const std = @import("std");
import { XmlParser, XmlNode, XmlWriter } from "cap:xml";
import { JsonParser, JsonValue, JsonField, JsonSerializer } from "cap:json";

pub err FeedError {
    UnsupportedFormat,
    InvalidXml,
    InvalidJson,
    MissingTitle,
    OutOfMemory,
}

pub enum FeedFormat {
    Rss2,
    Atom1,
    JsonFeed1,
}

pub struct FeedItem {
    id: []const u8 = "",
    title: []const u8 = "",
    link: []const u8 = "",
    summary: []const u8 = "",
    content_html: []const u8 = "",
    author: []const u8 = "",
    published: []const u8 = "",
    tags: [][]const u8 = [],
}

pub struct Feed {
    format: FeedFormat = .Rss2,
    title: []const u8 = "",
    link: []const u8 = "",
    description: []const u8 = "",
    language: []const u8 = "",
    updated: []const u8 = "",
    items: []FeedItem = [],
    owned_arena: (*std.heap.ArenaAllocator)? = null,
    parent_alloc: std.mem.Allocator? = null,

    /// Initializes an empty `Feed` with the specified format.
    pub static fn init(format: FeedFormat): Feed {
        return Feed{ format: format };
    }

    /// Releases all arena-allocated strings, tags, and items in O(1).
    pub fn deinit(*_) {
        if (owned := _.owned_arena) {
            owned.deinit();
            if (p := _.parent_alloc) {
                p.destroy(owned);
            }
            _.owned_arena = null;
            _.parent_alloc = null;
        }
        _.items = [];
        _.title = "";
        _.link = "";
        _.description = "";
        _.language = "";
        _.updated = "";
    }
}

pub struct FeedCodec {
    static fn stripBomAndSpace(raw: []const u8): []const u8 {
        let s = raw;
        if (s.len >= 3 && s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
            s = s[3..];
        }
        return std.mem.trim(u8, s, " \t\r\n");
    }

    static fn isSpace(c: u8): bool {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    static fn findXmlRootTag(trimmed: []const u8): []const u8 {
        let pos: usize = 0;
        while (pos < trimmed.len) {
            while (pos < trimmed.len && FeedCodec.isSpace(trimmed[pos])) {
                pos += 1;
            }
            if (pos >= trimmed.len || trimmed[pos] != '<') {
                return "";
            }
            if (pos + 1 < trimmed.len && trimmed[pos + 1] == '?') {
                if (end_idx := std.mem.indexOfPos(u8, trimmed, pos + 2, "?>")) {
                    pos = end_idx + 2;
                    continue;
                }
                return "";
            }
            if (pos + 3 < trimmed.len && std.mem.eql(u8, trimmed[pos .. pos + 4], "<!--")) {
                if (end_idx := std.mem.indexOfPos(u8, trimmed, pos + 4, "-->")) {
                    pos = end_idx + 3;
                    continue;
                }
                return "";
            }
            if (pos + 1 < trimmed.len && trimmed[pos + 1] == '!') {
                if (end_idx := std.mem.indexOfScalarPos(u8, trimmed, pos + 2, '>')) {
                    pos = end_idx + 1;
                    continue;
                }
                return "";
            }
            if (pos + 1 < trimmed.len && trimmed[pos + 1] == '/') {
                return "";
            }
            const name_start = pos + 1;
            let name_end = name_start;
            while (name_end < trimmed.len) {
                const c = trimmed[name_end];
                if (FeedCodec.isSpace(c) || c == '>' || c == '/') {
                    break;
                }
                name_end += 1;
            }
            return trimmed[name_start..name_end];
        }
        return "";
    }

    /// Automatically detects whether `raw` is RSS 2.0, Atom 1.0, or JSON Feed 1.1.
    pub static fn detectFormat(raw: []const u8): FeedFormat ! FeedError {
        const trimmed = FeedCodec.stripBomAndSpace(raw);
        if (trimmed.len == 0) {
            return FeedError.UnsupportedFormat;
        }
        if (trimmed[0] == '{') {
            if (std.mem.indexOf(u8, trimmed, "https://jsonfeed.org/version/") != null ||
                std.mem.indexOf(u8, trimmed, "http://jsonfeed.org/version/") != null)
            {
                return .JsonFeed1;
            }
            return FeedError.UnsupportedFormat;
        }
        if (trimmed[0] == '<') {
            const root_tag = FeedCodec.findXmlRootTag(trimmed);
            if (std.mem.eql(u8, root_tag, "rss") || std.mem.eql(u8, root_tag, "rdf:RDF")) {
                return .Rss2;
            }
            if (std.mem.eql(u8, root_tag, "feed") || std.mem.eql(u8, root_tag, "atom:feed")) {
                return .Atom1;
            }
            return FeedError.UnsupportedFormat;
        }
        return FeedError.UnsupportedFormat;
    }

    static fn dupeStr(aa: std.mem.Allocator, s: []const u8): ([]const u8) ! FeedError {
        if (s.len == 0) {
            return "";
        }
        return aa.dupe(u8, s) catch return FeedError.OutOfMemory;
    }

    static fn childText(aa: std.mem.Allocator, parent: XmlNode, tag: []const u8): ([]const u8) ! FeedError {
        if (child := parent.findChild(tag)) {
            const txt = child.innerText(aa) catch return FeedError.OutOfMemory;
            return std.mem.trim(u8, txt, " \t\r\n");
        }
        return "";
    }

    static fn parseRss2(aa: std.mem.Allocator, parent_alloc: std.mem.Allocator, raw: []const u8): Feed ! FeedError {
        let doc = XmlParser.parse(parent_alloc, raw) catch return FeedError.InvalidXml;
        defer doc.deinit();

        const channel = doc.root.findChild("channel") ?? return FeedError.InvalidXml;
        const title = FeedCodec.childText(aa, channel, "title")?;
        if (title.len == 0) {
            return FeedError.MissingTitle;
        }

        const link = FeedCodec.childText(aa, channel, "link")?;
        const description = FeedCodec.childText(aa, channel, "description")?;
        const language = FeedCodec.childText(aa, channel, "language")?;
        let updated = FeedCodec.childText(aa, channel, "lastBuildDate")?;
        if (updated.len == 0) {
            updated = FeedCodec.childText(aa, channel, "pubDate")?;
        }

        let items_list: std.ArrayList(FeedItem) = .empty;
        for (c of channel.children) {
            if (c.kind != .Element || !std.mem.eql(u8, c.tag, "item")) {
                continue;
            }
            const item_title = FeedCodec.childText(aa, c, "title")?;
            let item_link = FeedCodec.childText(aa, c, "link")?;
            if (item_link.len == 0) {
                if (enc := c.findChild("enclosure")) {
                    if (enc_url := enc.getAttr("url")) {
                        item_link = FeedCodec.dupeStr(aa, enc_url)?;
                    }
                }
            }
            let item_id = FeedCodec.childText(aa, c, "guid")?;
            if (item_id.len == 0) {
                item_id = item_link;
            }
            const item_summary = FeedCodec.childText(aa, c, "description")?;
            const item_content = FeedCodec.childText(aa, c, "content:encoded")?;
            let item_author = FeedCodec.childText(aa, c, "author")?;
            if (item_author.len == 0) {
                item_author = FeedCodec.childText(aa, c, "dc:creator")?;
            }
            let item_pub = FeedCodec.childText(aa, c, "pubDate")?;
            if (item_pub.len == 0) {
                item_pub = FeedCodec.childText(aa, c, "dc:date")?;
            }

            let tags_list: std.ArrayList(string) = .empty;
            for (sub of c.children) {
                if (sub.kind == .Element && std.mem.eql(u8, sub.tag, "category")) {
                    const raw_tag = sub.innerText(aa) catch return FeedError.OutOfMemory;
                    const trimmed_tag = std.mem.trim(u8, raw_tag, " \t\r\n");
                    if (trimmed_tag.len > 0) {
                        tags_list.append(aa, trimmed_tag) catch return FeedError.OutOfMemory;
                    }
                }
            }
            const tags_slice = tags_list.toOwnedSlice(aa) catch return FeedError.OutOfMemory;

            items_list.append(aa, FeedItem{
                id: item_id,
                title: item_title,
                link: item_link,
                summary: item_summary,
                content_html: item_content,
                author: item_author,
                published: item_pub,
                tags: tags_slice,
            }) catch return FeedError.OutOfMemory;
        }

        const items_slice = items_list.toOwnedSlice(aa) catch return FeedError.OutOfMemory;
        return Feed{
            format: .Rss2,
            title: title,
            link: link,
            description: description,
            language: language,
            updated: updated,
            items: items_slice,
        };
    }

    static fn extractAtomLink(aa: std.mem.Allocator, parent: XmlNode): ([]const u8) ! FeedError {
        let fallback_href: []const u8 = "";
        for (c of parent.children) {
            if (c.kind != .Element || !std.mem.eql(u8, c.tag, "link")) {
                continue;
            }
            if (href := c.getAttr("href")) {
                const rel = c.getAttr("rel") ?? "alternate";
                if (std.mem.eql(u8, rel, "alternate")) {
                    return FeedCodec.dupeStr(aa, href);
                }
                if (fallback_href.len == 0) {
                    fallback_href = href;
                }
            }
        }
        if (fallback_href.len > 0) {
            return FeedCodec.dupeStr(aa, fallback_href);
        }
        return FeedCodec.childText(aa, parent, "link");
    }

    static fn parseAtom1(aa: std.mem.Allocator, parent_alloc: std.mem.Allocator, raw: []const u8): Feed ! FeedError {
        let doc = XmlParser.parse(parent_alloc, raw) catch return FeedError.InvalidXml;
        defer doc.deinit();

        const root = doc.root;
        const title = FeedCodec.childText(aa, root, "title")?;
        if (title.len == 0) {
            return FeedError.MissingTitle;
        }

        const link = FeedCodec.extractAtomLink(aa, root)?;
        const description = FeedCodec.childText(aa, root, "subtitle")?;
        const language = FeedCodec.dupeStr(aa, root.getAttr("xml:lang") ?? "")?;
        const updated = FeedCodec.childText(aa, root, "updated")?;

        let items_list: std.ArrayList(FeedItem) = .empty;
        for (c of root.children) {
            if (c.kind != .Element || !std.mem.eql(u8, c.tag, "entry")) {
                continue;
            }
            const item_title = FeedCodec.childText(aa, c, "title")?;
            const item_link = FeedCodec.extractAtomLink(aa, c)?;
            let item_id = FeedCodec.childText(aa, c, "id")?;
            if (item_id.len == 0) {
                item_id = item_link;
            }
            const item_summary = FeedCodec.childText(aa, c, "summary")?;
            const item_content = FeedCodec.childText(aa, c, "content")?;

            let item_author: []const u8 = "";
            if (author_node := c.findChild("author")) {
                item_author = FeedCodec.childText(aa, author_node, "name")?;
                if (item_author.len == 0) {
                    item_author = FeedCodec.childText(aa, author_node, "email")?;
                }
                if (item_author.len == 0) {
                    const raw_a = author_node.innerText(aa) catch return FeedError.OutOfMemory;
                    item_author = std.mem.trim(u8, raw_a, " \t\r\n");
                }
            }
            if (item_author.len == 0) {
                item_author = FeedCodec.childText(aa, c, "dc:creator")?;
            }

            let item_pub = FeedCodec.childText(aa, c, "published")?;
            if (item_pub.len == 0) {
                item_pub = FeedCodec.childText(aa, c, "updated")?;
            }

            let tags_list: std.ArrayList(string) = .empty;
            for (sub of c.children) {
                if (sub.kind == .Element && std.mem.eql(u8, sub.tag, "category")) {
                    let tag_val: []const u8 = "";
                    if (term := sub.getAttr("term")) {
                        tag_val = FeedCodec.dupeStr(aa, std.mem.trim(u8, term, " \t\r\n"))?;
                    } else if (lbl := sub.getAttr("label")) {
                        tag_val = FeedCodec.dupeStr(aa, std.mem.trim(u8, lbl, " \t\r\n"))?;
                    } else {
                        const raw_t = sub.innerText(aa) catch return FeedError.OutOfMemory;
                        tag_val = std.mem.trim(u8, raw_t, " \t\r\n");
                    }
                    if (tag_val.len > 0) {
                        tags_list.append(aa, tag_val) catch return FeedError.OutOfMemory;
                    }
                }
            }
            const tags_slice = tags_list.toOwnedSlice(aa) catch return FeedError.OutOfMemory;

            items_list.append(aa, FeedItem{
                id: item_id,
                title: item_title,
                link: item_link,
                summary: item_summary,
                content_html: item_content,
                author: item_author,
                published: item_pub,
                tags: tags_slice,
            }) catch return FeedError.OutOfMemory;
        }

        const items_slice = items_list.toOwnedSlice(aa) catch return FeedError.OutOfMemory;
        return Feed{
            format: .Atom1,
            title: title,
            link: link,
            description: description,
            language: language,
            updated: updated,
            items: items_slice,
        };
    }

    static fn jsonFieldStr(aa: std.mem.Allocator, obj: JsonValue, key: []const u8): ([]const u8) ! FeedError {
        if (v := obj.get(key)) {
            if (s := v.asString()) {
                return FeedCodec.dupeStr(aa, s);
            }
        }
        return "";
    }

    static fn parseJsonFeed1(aa: std.mem.Allocator, raw: []const u8): Feed ! FeedError {
        const root = JsonParser.parse(aa, raw) catch return FeedError.InvalidJson;
        if (!root.isObject()) {
            return FeedError.InvalidJson;
        }

        const ver = root.get("version")?.asString() ?? "";
        if (std.mem.indexOf(u8, ver, "jsonfeed.org/version/") == null) {
            return FeedError.UnsupportedFormat;
        }

        const title = FeedCodec.jsonFieldStr(aa, root, "title")?;
        if (title.len == 0) {
            return FeedError.MissingTitle;
        }

        let link = FeedCodec.jsonFieldStr(aa, root, "home_page_url")?;
        if (link.len == 0) {
            link = FeedCodec.jsonFieldStr(aa, root, "feed_url")?;
        }
        const description = FeedCodec.jsonFieldStr(aa, root, "description")?;
        const language = FeedCodec.jsonFieldStr(aa, root, "language")?;

        let items_list: std.ArrayList(FeedItem) = .empty;
        if (items_val := root.get("items")) {
            if (arr := items_val.asArray()) {
                for (item_val of arr) {
                    if (!item_val.isObject()) {
                        continue;
                    }
                    const item_title = FeedCodec.jsonFieldStr(aa, item_val, "title")?;
                    let item_link = FeedCodec.jsonFieldStr(aa, item_val, "url")?;
                    if (item_link.len == 0) {
                        item_link = FeedCodec.jsonFieldStr(aa, item_val, "external_url")?;
                    }
                    let item_id = FeedCodec.jsonFieldStr(aa, item_val, "id")?;
                    if (item_id.len == 0) {
                        item_id = item_link;
                    }
                    const item_summary = FeedCodec.jsonFieldStr(aa, item_val, "summary")?;
                    let item_content = FeedCodec.jsonFieldStr(aa, item_val, "content_html")?;
                    if (item_content.len == 0) {
                        item_content = FeedCodec.jsonFieldStr(aa, item_val, "content_text")?;
                    }

                    let item_author: []const u8 = "";
                    if (authors_val := item_val.get("authors")) {
                        if (authors_arr := authors_val.asArray()) {
                            if (authors_arr.len > 0 && authors_arr[0].isObject()) {
                                item_author = FeedCodec.jsonFieldStr(aa, authors_arr[0], "name")?;
                            }
                        }
                    }
                    if (item_author.len == 0) {
                        if (author_obj := item_val.get("author")) {
                            if (author_obj.isObject()) {
                                item_author = FeedCodec.jsonFieldStr(aa, author_obj, "name")?;
                            }
                        }
                    }

                    let item_pub = FeedCodec.jsonFieldStr(aa, item_val, "date_published")?;
                    if (item_pub.len == 0) {
                        item_pub = FeedCodec.jsonFieldStr(aa, item_val, "date_modified")?;
                    }

                    let tags_list: std.ArrayList(string) = .empty;
                    if (tags_val := item_val.get("tags")) {
                        if (tags_arr := tags_val.asArray()) {
                            for (t of tags_arr) {
                                if (ts := t.asString()) {
                                    if (ts.len > 0) {
                                        const duped = FeedCodec.dupeStr(aa, ts)?;
                                        tags_list.append(aa, duped) catch return FeedError.OutOfMemory;
                                    }
                                }
                            }
                        }
                    }
                    const tags_slice = tags_list.toOwnedSlice(aa) catch return FeedError.OutOfMemory;

                    items_list.append(aa, FeedItem{
                        id: item_id,
                        title: item_title,
                        link: item_link,
                        summary: item_summary,
                        content_html: item_content,
                        author: item_author,
                        published: item_pub,
                        tags: tags_slice,
                    }) catch return FeedError.OutOfMemory;
                }
            }
        }

        const items_slice = items_list.toOwnedSlice(aa) catch return FeedError.OutOfMemory;
        return Feed{
            format: .JsonFeed1,
            title: title,
            link: link,
            description: description,
            language: language,
            updated: "",
            items: items_slice,
        };
    }

    /// Parses an RSS 2.0, Atom 1.0, or JSON Feed 1.1 document into a unified `Feed`.
    /// Caller owns the returned `Feed` and must call `feed.deinit()`.
    pub static fn parse(alloc: std.mem.Allocator, raw: []const u8): Feed ! FeedError {
        const fmt = FeedCodec.detectFormat(raw)?;

        let arena_ptr = alloc.create(std.heap.ArenaAllocator) catch return FeedError.OutOfMemory;
        unsafe {
            arena_ptr.* = std.heap.ArenaAllocator.init(alloc);
        }
        errdefer {
            arena_ptr.deinit();
            alloc.destroy(arena_ptr);
        }
        const aa = arena_ptr.allocator();

        let feed = switch (fmt) {
            .Rss2 => FeedCodec.parseRss2(aa, alloc, raw)?,
            .Atom1 => FeedCodec.parseAtom1(aa, alloc, raw)?,
            .JsonFeed1 => FeedCodec.parseJsonFeed1(aa, raw)?,
        };
        feed.owned_arena = arena_ptr;
        feed.parent_alloc = alloc;
        return feed;
    }

    static fn writeXmlTextOrCdata(w: *XmlWriter, content: []const u8): !FeedError {
        if (content.len == 0) {
            return;
        }
        const has_markup = std.mem.indexOfAny(u8, content, "<>&") != null;
        if (has_markup && std.mem.indexOf(u8, content, "]]>") == null) {
            w.cdata(content) catch return FeedError.InvalidXml;
        } else {
            w.text(content) catch return FeedError.InvalidXml;
        }
    }

    static fn writeXmlElementText(w: *XmlWriter, tag: []const u8, content: []const u8): !FeedError {
        w.openTag(tag) catch return FeedError.InvalidXml;
        w.text(content) catch return FeedError.InvalidXml;
        w.closeTag() catch return FeedError.InvalidXml;
    }

    /// Serializes a `Feed` into a compliant RSS 2.0 XML document.
    pub static fn toRss2(alloc: std.mem.Allocator, feed: Feed): ([]u8) ! FeedError {
        if (feed.title.len == 0) {
            return FeedError.MissingTitle;
        }
        let w = XmlWriter.initPretty(alloc, 2);
        defer w.deinit();

        w.writeDeclaration() catch return FeedError.OutOfMemory;
        w.openTag("rss") catch return FeedError.InvalidXml;
        w.attr("version", "2.0") catch return FeedError.InvalidXml;
        w.attr("xmlns:content", "http://purl.org/rss/1.0/modules/content/") catch return FeedError.InvalidXml;

        w.openTag("channel") catch return FeedError.InvalidXml;
        FeedCodec.writeXmlElementText(&w, "title", feed.title)?;
        FeedCodec.writeXmlElementText(&w, "link", feed.link)?;
        FeedCodec.writeXmlElementText(&w, "description", feed.description)?;
        if (feed.language.len > 0) {
            FeedCodec.writeXmlElementText(&w, "language", feed.language)?;
        }
        if (feed.updated.len > 0) {
            FeedCodec.writeXmlElementText(&w, "lastBuildDate", feed.updated)?;
        }

        for (item of feed.items) {
            w.openTag("item") catch return FeedError.InvalidXml;
            if (item.title.len > 0) {
                FeedCodec.writeXmlElementText(&w, "title", item.title)?;
            }
            if (item.link.len > 0) {
                FeedCodec.writeXmlElementText(&w, "link", item.link)?;
            }
            if (item.id.len > 0) {
                FeedCodec.writeXmlElementText(&w, "guid", item.id)?;
            }
            if (item.summary.len > 0) {
                w.openTag("description") catch return FeedError.InvalidXml;
                FeedCodec.writeXmlTextOrCdata(&w, item.summary)?;
                w.closeTag() catch return FeedError.InvalidXml;
            }
            if (item.content_html.len > 0) {
                w.openTag("content:encoded") catch return FeedError.InvalidXml;
                FeedCodec.writeXmlTextOrCdata(&w, item.content_html)?;
                w.closeTag() catch return FeedError.InvalidXml;
            }
            if (item.author.len > 0) {
                FeedCodec.writeXmlElementText(&w, "author", item.author)?;
            }
            if (item.published.len > 0) {
                FeedCodec.writeXmlElementText(&w, "pubDate", item.published)?;
            }
            for (tag of item.tags) {
                if (tag.len > 0) {
                    FeedCodec.writeXmlElementText(&w, "category", tag)?;
                }
            }
            w.closeTag() catch return FeedError.InvalidXml;
        }

        w.closeTag() catch return FeedError.InvalidXml;
        w.closeTag() catch return FeedError.InvalidXml;
        return w.toOwnedSlice() catch return FeedError.OutOfMemory;
    }

    /// Serializes a `Feed` into a compliant Atom 1.0 XML document.
    pub static fn toAtom1(alloc: std.mem.Allocator, feed: Feed): ([]u8) ! FeedError {
        if (feed.title.len == 0) {
            return FeedError.MissingTitle;
        }
        let w = XmlWriter.initPretty(alloc, 2);
        defer w.deinit();

        w.writeDeclaration() catch return FeedError.OutOfMemory;
        w.openTag("feed") catch return FeedError.InvalidXml;
        w.attr("xmlns", "http://www.w3.org/2005/Atom") catch return FeedError.InvalidXml;
        if (feed.language.len > 0) {
            w.attr("xml:lang", feed.language) catch return FeedError.InvalidXml;
        }

        FeedCodec.writeXmlElementText(&w, "title", feed.title)?;
        const feed_id = if (feed.link.len > 0) feed.link else "urn:cp:feed";
        FeedCodec.writeXmlElementText(&w, "id", feed_id)?;
        if (feed.link.len > 0) {
            w.openTag("link") catch return FeedError.InvalidXml;
            w.attr("rel", "alternate") catch return FeedError.InvalidXml;
            w.attr("href", feed.link) catch return FeedError.InvalidXml;
            w.closeTag() catch return FeedError.InvalidXml;
        }
        if (feed.description.len > 0) {
            FeedCodec.writeXmlElementText(&w, "subtitle", feed.description)?;
        }
        if (feed.updated.len > 0) {
            FeedCodec.writeXmlElementText(&w, "updated", feed.updated)?;
        }

        for (item of feed.items) {
            w.openTag("entry") catch return FeedError.InvalidXml;
            if (item.title.len > 0) {
                FeedCodec.writeXmlElementText(&w, "title", item.title)?;
            }
            const entry_id = if (item.id.len > 0) item.id else (if (item.link.len > 0) item.link else item.title);
            FeedCodec.writeXmlElementText(&w, "id", entry_id)?;
            if (item.link.len > 0) {
                w.openTag("link") catch return FeedError.InvalidXml;
                w.attr("rel", "alternate") catch return FeedError.InvalidXml;
                w.attr("href", item.link) catch return FeedError.InvalidXml;
                w.closeTag() catch return FeedError.InvalidXml;
            }
            if (item.published.len > 0) {
                FeedCodec.writeXmlElementText(&w, "published", item.published)?;
                FeedCodec.writeXmlElementText(&w, "updated", item.published)?;
            }
            if (item.author.len > 0) {
                w.openTag("author") catch return FeedError.InvalidXml;
                FeedCodec.writeXmlElementText(&w, "name", item.author)?;
                w.closeTag() catch return FeedError.InvalidXml;
            }
            if (item.summary.len > 0) {
                FeedCodec.writeXmlElementText(&w, "summary", item.summary)?;
            }
            if (item.content_html.len > 0) {
                w.openTag("content") catch return FeedError.InvalidXml;
                w.attr("type", "html") catch return FeedError.InvalidXml;
                FeedCodec.writeXmlTextOrCdata(&w, item.content_html)?;
                w.closeTag() catch return FeedError.InvalidXml;
            }
            for (tag of item.tags) {
                if (tag.len > 0) {
                    w.openTag("category") catch return FeedError.InvalidXml;
                    w.attr("term", tag) catch return FeedError.InvalidXml;
                    w.closeTag() catch return FeedError.InvalidXml;
                }
            }
            w.closeTag() catch return FeedError.InvalidXml;
        }

        w.closeTag() catch return FeedError.InvalidXml;
        return w.toOwnedSlice() catch return FeedError.OutOfMemory;
    }

    static fn jsonStrVal(s: []const u8): JsonValue {
        return JsonValue{ tag: .String, str_val: s };
    }

    /// Serializes a `Feed` into a compliant JSON Feed 1.1 document.
    pub static fn toJsonFeed(alloc: std.mem.Allocator, feed: Feed): ([]u8) ! FeedError {
        if (feed.title.len == 0) {
            return FeedError.MissingTitle;
        }

        let tmp_arena = std.heap.ArenaAllocator.init(alloc);
        defer tmp_arena.deinit();
        const aa = tmp_arena.allocator();

        let root_fields: std.ArrayList(JsonField) = .empty;
        root_fields.append(aa, JsonField{
            key: "version",
            value: FeedCodec.jsonStrVal("https://jsonfeed.org/version/1.1"),
        }) catch return FeedError.OutOfMemory;
        root_fields.append(aa, JsonField{
            key: "title",
            value: FeedCodec.jsonStrVal(feed.title),
        }) catch return FeedError.OutOfMemory;
        if (feed.link.len > 0) {
            root_fields.append(aa, JsonField{
                key: "home_page_url",
                value: FeedCodec.jsonStrVal(feed.link),
            }) catch return FeedError.OutOfMemory;
        }
        if (feed.description.len > 0) {
            root_fields.append(aa, JsonField{
                key: "description",
                value: FeedCodec.jsonStrVal(feed.description),
            }) catch return FeedError.OutOfMemory;
        }
        if (feed.language.len > 0) {
            root_fields.append(aa, JsonField{
                key: "language",
                value: FeedCodec.jsonStrVal(feed.language),
            }) catch return FeedError.OutOfMemory;
        }

        let json_items: std.ArrayList(JsonValue) = .empty;
        for (item of feed.items) {
            let fields: std.ArrayList(JsonField) = .empty;
            const item_id = if (item.id.len > 0) item.id else (if (item.link.len > 0) item.link else item.title);
            fields.append(aa, JsonField{ key: "id", value: FeedCodec.jsonStrVal(item_id) }) catch return FeedError.OutOfMemory;
            if (item.title.len > 0) {
                fields.append(aa, JsonField{ key: "title", value: FeedCodec.jsonStrVal(item.title) }) catch return FeedError.OutOfMemory;
            }
            if (item.link.len > 0) {
                fields.append(aa, JsonField{ key: "url", value: FeedCodec.jsonStrVal(item.link) }) catch return FeedError.OutOfMemory;
            }
            if (item.summary.len > 0) {
                fields.append(aa, JsonField{ key: "summary", value: FeedCodec.jsonStrVal(item.summary) }) catch return FeedError.OutOfMemory;
            }
            if (item.content_html.len > 0) {
                fields.append(aa, JsonField{ key: "content_html", value: FeedCodec.jsonStrVal(item.content_html) }) catch return FeedError.OutOfMemory;
            }
            if (item.published.len > 0) {
                fields.append(aa, JsonField{ key: "date_published", value: FeedCodec.jsonStrVal(item.published) }) catch return FeedError.OutOfMemory;
            }
            if (item.author.len > 0) {
                let author_fields = aa.alloc(JsonField, 1) catch return FeedError.OutOfMemory;
                author_fields[0] = JsonField{ key: "name", value: FeedCodec.jsonStrVal(item.author) };
                let authors_arr = aa.alloc(JsonValue, 1) catch return FeedError.OutOfMemory;
                authors_arr[0] = JsonValue{ tag: .Object, obj_val: author_fields };
                fields.append(aa, JsonField{
                    key: "authors",
                    value: JsonValue{ tag: .Array, arr_val: authors_arr },
                }) catch return FeedError.OutOfMemory;
            }
            if (item.tags.len > 0) {
                let tags_arr: std.ArrayList(JsonValue) = .empty;
                for (tag of item.tags) {
                    if (tag.len > 0) {
                        tags_arr.append(aa, FeedCodec.jsonStrVal(tag)) catch return FeedError.OutOfMemory;
                    }
                }
                if (tags_arr.items.len > 0) {
                    const tags_slice = tags_arr.toOwnedSlice(aa) catch return FeedError.OutOfMemory;
                    fields.append(aa, JsonField{
                        key: "tags",
                        value: JsonValue{ tag: .Array, arr_val: tags_slice },
                    }) catch return FeedError.OutOfMemory;
                }
            }
            const fields_slice = fields.toOwnedSlice(aa) catch return FeedError.OutOfMemory;
            json_items.append(aa, JsonValue{ tag: .Object, obj_val: fields_slice }) catch return FeedError.OutOfMemory;
        }

        const items_slice = json_items.toOwnedSlice(aa) catch return FeedError.OutOfMemory;
        root_fields.append(aa, JsonField{
            key: "items",
            value: JsonValue{ tag: .Array, arr_val: items_slice },
        }) catch return FeedError.OutOfMemory;

        const root_slice = root_fields.toOwnedSlice(aa) catch return FeedError.OutOfMemory;
        const root_val = JsonValue{ tag: .Object, obj_val: root_slice };
        return JsonSerializer.stringifyPretty(alloc, root_val, 2) catch return FeedError.OutOfMemory;
    }
}
