// Source: cp/lib/html/extractor.cp (AI Agent HTML Visible Text & Metadata Extractor)
// Single-Pass Streaming HTML5 Metadata, Link, Heading & Clean Text Extractor for RAG / AI Agents

const std = @import("std");
import { HtmlEntities } from "./entities";
import { HtmlToken, HtmlTokenKind, HtmlAttribute, HtmlTokenizer } from "./tokenizer";

pub struct HtmlLink {
    href: []const u8 = "",
    text: []const u8 = "",
}

pub struct HtmlMeta {
    name: []const u8 = "",
    content: []const u8 = "",
}

pub struct HtmlHeading {
    level: u8 = 1,
    text: []const u8 = "",
}

pub struct HtmlDocumentSummary {
    allocator: std.mem.Allocator = undefined,
    title: []const u8 = "",
    description: []const u8 = "",
    canonical_url: []const u8 = "",
    og_title: []const u8 = "",
    og_image: []const u8 = "",
    links: []HtmlLink = [],
    headings: []HtmlHeading = [],
    metas: []HtmlMeta = [],
    plain_text: []const u8 = "",

    pub fn deinit(*_) {
        if (_.title.len > 0) _.allocator.free(_.title);
        if (_.description.len > 0) _.allocator.free(_.description);
        if (_.canonical_url.len > 0) _.allocator.free(_.canonical_url);
        if (_.og_title.len > 0) _.allocator.free(_.og_title);
        if (_.og_image.len > 0) _.allocator.free(_.og_image);
        if (_.plain_text.len > 0) _.allocator.free(_.plain_text);

        for (lnk of _.links) {
            if (lnk.href.len > 0) _.allocator.free(lnk.href);
            if (lnk.text.len > 0) _.allocator.free(lnk.text);
        }
        if (_.links.len > 0) _.allocator.free(_.links);

        for (h of _.headings) {
            if (h.text.len > 0) _.allocator.free(h.text);
        }
        if (_.headings.len > 0) _.allocator.free(_.headings);

        for (m of _.metas) {
            if (m.name.len > 0) _.allocator.free(m.name);
            if (m.content.len > 0) _.allocator.free(m.content);
        }
        if (_.metas.len > 0) _.allocator.free(_.metas);
    }
}

struct SkipTagFrame {
    name: []const u8 = "",
}

struct ExtractorTextNorm {
    buf: std.ArrayList(u8) = .empty,
    pending_space: bool = false,
    pending_newline: bool = false,
    has_emitted: bool = false,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.buf.deinit(alloc);
    }

    pub fn clear(*_) {
        _.buf.clearRetainingCapacity();
        _.pending_space = false;
        _.pending_newline = false;
        _.has_emitted = false;
    }

    pub fn markBlockBreak(*_) {
        if (_.has_emitted) {
            _.pending_newline = true;
            _.pending_space = false;
        }
    }

    pub fn markSpace(*_) {
        if (_.has_emitted && !_.pending_newline) {
            _.pending_space = true;
        }
    }

    pub fn appendText(*_, alloc: std.mem.Allocator, raw_text: []const u8): ! {
        const decoded = HtmlEntities.decode(alloc, raw_text)?;
        defer alloc.free(decoded);

        for (c of decoded) {
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C) {
                if (_.has_emitted && !_.pending_newline) {
                    _.pending_space = true;
                }
            } else {
                if (_.has_emitted) {
                    if (_.pending_newline) {
                        _.buf.append(alloc, '\n')?;
                    } else if (_.pending_space) {
                        _.buf.append(alloc, ' ')?;
                    }
                }
                _.pending_newline = false;
                _.pending_space = false;
                _.buf.append(alloc, c)?;
                _.has_emitted = true;
            }
        }
    }
}

pub struct HtmlExtractor {
    static fn isIgnoredContainerTag(tag: []const u8): bool {
        if (std.ascii.eqlIgnoreCase(tag, "script")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "style")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "noscript")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "template")) return true;
        return false;
    }

    static fn isBlockTag(tag: []const u8): bool {
        if (std.ascii.eqlIgnoreCase(tag, "address")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "article")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "aside")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "blockquote")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "br")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "dd")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "details")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "div")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "dl")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "dt")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "figcaption")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "figure")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "footer")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "form")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "h1")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "h2")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "h3")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "h4")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "h5")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "h6")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "header")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "hr")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "li")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "main")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "nav")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "ol")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "p")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "pre")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "section")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "summary")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "table")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "tbody")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "tfoot")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "thead")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "tr")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "ul")) return true;
        return false;
    }

    static fn parseHeadingLevel(tag: []const u8): u8? {
        if (tag.len == 2 && (tag[0] == 'h' || tag[0] == 'H') && tag[1] >= '1' && tag[1] <= '6') {
            return tag[1] - '0';
        }
        return null;
    }

    static fn relContainsCanonical(rel_val: []const u8): bool {
        let iter = std.mem.tokenizeAny(u8, rel_val, " \t\r\n");
        while (tok := iter.next()) {
            if (std.ascii.eqlIgnoreCase(tok, "canonical")) return true;
        }
        return false;
    }

    /// Single-pass streaming extraction of metadata, headings, links, and normalized visible text.
    pub static fn extract(alloc: std.mem.Allocator, html: []const u8): HtmlDocumentSummary! {
        let summary = HtmlDocumentSummary{
            allocator: alloc,
        };
        errdefer summary.deinit();

        let plain_norm = ExtractorTextNorm{};
        defer plain_norm.deinit(alloc);

        let title_norm = ExtractorTextNorm{};
        defer title_norm.deinit(alloc);

        let link_norm = ExtractorTextNorm{};
        defer link_norm.deinit(alloc);

        let heading_norm = ExtractorTextNorm{};
        defer heading_norm.deinit(alloc);

        let links_list: std.ArrayList(HtmlLink) = .empty;
        errdefer {
            for (l of links_list.items) {
                if (l.href.len > 0) alloc.free(l.href);
                if (l.text.len > 0) alloc.free(l.text);
            }
            links_list.deinit(alloc);
        }

        let headings_list: std.ArrayList(HtmlHeading) = .empty;
        errdefer {
            for (h of headings_list.items) {
                if (h.text.len > 0) alloc.free(h.text);
            }
            headings_list.deinit(alloc);
        }

        let metas_list: std.ArrayList(HtmlMeta) = .empty;
        errdefer {
            for (m of metas_list.items) {
                if (m.name.len > 0) alloc.free(m.name);
                if (m.content.len > 0) alloc.free(m.content);
            }
            metas_list.deinit(alloc);
        }

        let skip_stack: std.ArrayList(SkipTagFrame) = .empty;
        defer skip_stack.deinit(alloc);

        let in_title = false;
        let active_link_href: ([]u8)? = null;
        defer {
            if (href_buf := active_link_href) {
                alloc.free(href_buf);
            }
        }
        let active_heading_level: u8 = 0;

        let tok = HtmlTokenizer.init(html);
        while (t := tok.next(alloc)?) {
            if (t.kind == .StartTag || t.kind == .SelfClosingTag) {
                const is_void = HtmlTokenizer.isVoidElement(t.tag_name);
                const is_self = t.self_closing || is_void;

                if (skip_stack.items.len > 0) {
                    if (!is_self && skip_stack.items.len < 256) {
                        skip_stack.append(alloc, SkipTagFrame{ name: t.tag_name })?;
                    }
                    if (t.attrs.len > 0) alloc.free(t.attrs);
                    continue;
                }

                if (HtmlExtractor.isIgnoredContainerTag(t.tag_name)) {
                    if (!is_self) {
                        skip_stack.append(alloc, SkipTagFrame{ name: t.tag_name })?;
                    }
                    if (t.attrs.len > 0) alloc.free(t.attrs);
                    continue;
                }

                if (std.ascii.eqlIgnoreCase(t.tag_name, "title")) {
                    if (!is_self && summary.title.len == 0) {
                        in_title = true;
                        title_norm.clear();
                    }
                    if (t.attrs.len > 0) alloc.free(t.attrs);
                    continue;
                }

                if (std.ascii.eqlIgnoreCase(t.tag_name, "meta")) {
                    const name_raw = t.getAttribute("name") ?? t.getAttribute("property") ?? t.getAttribute("http-equiv") ?? "";
                    const content_raw = t.getAttribute("content") ?? "";
                    if (name_raw.len > 0 && content_raw.len > 0) {
                        const dec_name = HtmlEntities.decode(alloc, name_raw)?;
                        errdefer alloc.free(dec_name);
                        const dec_content = HtmlEntities.decode(alloc, content_raw)?;
                        errdefer alloc.free(dec_content);

                        if (summary.description.len == 0 && std.ascii.eqlIgnoreCase(dec_name, "description")) {
                            summary.description = alloc.dupe(u8, dec_content)?;
                        } else if (summary.og_title.len == 0 && std.ascii.eqlIgnoreCase(dec_name, "og:title")) {
                            summary.og_title = alloc.dupe(u8, dec_content)?;
                        } else if (summary.og_image.len == 0 && std.ascii.eqlIgnoreCase(dec_name, "og:image")) {
                            summary.og_image = alloc.dupe(u8, dec_content)?;
                        }

                        metas_list.append(alloc, HtmlMeta{
                            name: dec_name,
                            content: dec_content,
                        })?;
                    }
                    if (t.attrs.len > 0) alloc.free(t.attrs);
                    continue;
                }

                if (std.ascii.eqlIgnoreCase(t.tag_name, "link")) {
                    if (rel_val := t.getAttribute("rel")) {
                        if (HtmlExtractor.relContainsCanonical(rel_val)) {
                            if (href_val := t.getAttribute("href")) {
                                if (summary.canonical_url.len == 0 && href_val.len > 0) {
                                    summary.canonical_url = HtmlEntities.decode(alloc, href_val)?;
                                }
                            }
                        }
                    }
                    if (t.attrs.len > 0) alloc.free(t.attrs);
                    continue;
                }

                if (std.ascii.eqlIgnoreCase(t.tag_name, "a")) {
                    // Flush any previous unclosed <a>
                    if (prev_href := active_link_href) {
                        const link_text = link_norm.buf.toOwnedSlice(alloc)?;
                        links_list.append(alloc, HtmlLink{
                            href: prev_href,
                            text: link_text,
                        })?;
                        active_link_href = null;
                        link_norm.clear();
                    }
                    if (!is_self) {
                        if (href_attr := t.getAttribute("href")) {
                            active_link_href = HtmlEntities.decode(alloc, href_attr)?;
                            link_norm.clear();
                        }
                    } else if (href_attr := t.getAttribute("href")) {
                        const dec_href = HtmlEntities.decode(alloc, href_attr)?;
                        links_list.append(alloc, HtmlLink{
                            href: dec_href,
                            text: "",
                        })?;
                    }
                    if (t.attrs.len > 0) alloc.free(t.attrs);
                    continue;
                }

                if (h_lvl := HtmlExtractor.parseHeadingLevel(t.tag_name)) {
                    if (active_heading_level > 0) {
                        const h_text = heading_norm.buf.toOwnedSlice(alloc)?;
                        headings_list.append(alloc, HtmlHeading{
                            level: active_heading_level,
                            text: h_text,
                        })?;
                        heading_norm.clear();
                    }
                    if (!is_self) {
                        active_heading_level = h_lvl;
                        heading_norm.clear();
                    }
                    plain_norm.markBlockBreak();
                    if (t.attrs.len > 0) alloc.free(t.attrs);
                    continue;
                }

                if (std.ascii.eqlIgnoreCase(t.tag_name, "td") || std.ascii.eqlIgnoreCase(t.tag_name, "th")) {
                    plain_norm.markSpace();
                } else if (HtmlExtractor.isBlockTag(t.tag_name)) {
                    plain_norm.markBlockBreak();
                    if (active_link_href != null) link_norm.markSpace();
                }

                if (t.attrs.len > 0) alloc.free(t.attrs);
            } else if (t.kind == .EndTag) {
                if (skip_stack.items.len > 0) {
                    let match_skip: usize? = null;
                    let idx = skip_stack.items.len;
                    while (idx > 0) {
                        idx -= 1;
                        if (std.ascii.eqlIgnoreCase(skip_stack.items[idx].name, t.tag_name)) {
                            match_skip = idx;
                            break;
                        }
                    }
                    if (m := match_skip) {
                        while (skip_stack.items.len > m) {
                            * = skip_stack.pop().?;
                        }
                    }
                    continue;
                }

                if (std.ascii.eqlIgnoreCase(t.tag_name, "title")) {
                    if (in_title) {
                        in_title = false;
                        if (summary.title.len == 0 && title_norm.buf.items.len > 0) {
                            summary.title = title_norm.buf.toOwnedSlice(alloc)?;
                        }
                        title_norm.clear();
                    }
                    continue;
                }

                if (std.ascii.eqlIgnoreCase(t.tag_name, "a")) {
                    if (cur_href := active_link_href) {
                        const link_text = link_norm.buf.toOwnedSlice(alloc)?;
                        links_list.append(alloc, HtmlLink{
                            href: cur_href,
                            text: link_text,
                        })?;
                        active_link_href = null;
                        link_norm.clear();
                    }
                    continue;
                }

                if (h_lvl := HtmlExtractor.parseHeadingLevel(t.tag_name)) {
                    if (active_heading_level > 0 && active_heading_level == h_lvl) {
                        const h_text = heading_norm.buf.toOwnedSlice(alloc)?;
                        headings_list.append(alloc, HtmlHeading{
                            level: active_heading_level,
                            text: h_text,
                        })?;
                        active_heading_level = 0;
                        heading_norm.clear();
                    }
                    plain_norm.markBlockBreak();
                    continue;
                }

                if (std.ascii.eqlIgnoreCase(t.tag_name, "td") || std.ascii.eqlIgnoreCase(t.tag_name, "th")) {
                    plain_norm.markSpace();
                } else if (HtmlExtractor.isBlockTag(t.tag_name)) {
                    plain_norm.markBlockBreak();
                }
            } else if (t.kind == .Text) {
                if (skip_stack.items.len > 0) {
                    continue;
                }
                if (in_title) {
                    title_norm.appendText(alloc, t.text)?;
                    continue;
                }
                plain_norm.appendText(alloc, t.text)?;
                if (active_link_href != null) {
                    link_norm.appendText(alloc, t.text)?;
                }
                if (active_heading_level > 0) {
                    heading_norm.appendText(alloc, t.text)?;
                }
            }
        }

        // Flush any unclosed title, link, or heading at EOF
        if (in_title && summary.title.len == 0 && title_norm.buf.items.len > 0) {
            summary.title = title_norm.buf.toOwnedSlice(alloc)?;
        }
        if (cur_href := active_link_href) {
            const link_text = link_norm.buf.toOwnedSlice(alloc)?;
            links_list.append(alloc, HtmlLink{
                href: cur_href,
                text: link_text,
            })?;
            active_link_href = null;
        }
        if (active_heading_level > 0) {
            const h_text = heading_norm.buf.toOwnedSlice(alloc)?;
            headings_list.append(alloc, HtmlHeading{
                level: active_heading_level,
                text: h_text,
            })?;
            active_heading_level = 0;
        }

        summary.links = links_list.toOwnedSlice(alloc)?;
        summary.headings = headings_list.toOwnedSlice(alloc)?;
        summary.metas = metas_list.toOwnedSlice(alloc)?;
        summary.plain_text = plain_norm.buf.toOwnedSlice(alloc)?;
        return summary;
    }
}
