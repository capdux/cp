// Source: Absorbed from /data/dev/bun/src/md/html_renderer.cp | Synced: 2026-11-20T09:23:05Z
// lib/md/html_renderer.cp
// Industrial-Grade CommonMark + GFM HTML Renderer (Heading IDs, Autolink Headings, Tag Filter, Escaping)

const std = @import("std");
import {
    HeadingIdTracker,
    decodeEntityToUtf8,
    findEntity,
    isBlank,
    isNewline,
} from "./helpers.cp";
import {
    BLOCK_FENCED_CODE,
    BlockType,
    RenderOptions,
    Renderer,
    RendererVTable,
    SpanDetail,
    SpanType,
    TextType,
    alignmentFromData,
    alignmentName,
    isTaskChecked,
    taskMarkFromData,
} from "./types.cp";

pub struct HtmlOutputBuffer {
    list: std.ArrayListUnmanaged(u8) = .empty,
    allocator: std.mem.Allocator,
    oom: bool = false,

    pub fn write(*_, data: []const u8) {
        if (_.oom) return;
        _.list.appendSlice(_.allocator, data) catch {
            _.oom = true;
        };
    }

    pub fn writeByte(*_, b: u8) {
        if (_.oom) return;
        _.list.append(_.allocator, b) catch {
            _.oom = true;
        };
    }
}

pub struct HtmlRenderer {
    out: HtmlOutputBuffer,
    allocator: std.mem.Allocator,
    src_text: []const u8 = "",
    image_nesting_level: u32 = 0,
    saved_img_title: []const u8 = "",
    tag_filter: bool = false,
    tag_filter_raw_depth: u32 = 0,
    autolink_headings: bool = false,
    sanitize_html: bool = false,
    heading_buf: std.ArrayListUnmanaged(u8) = .empty,
    heading_tracker: HeadingIdTracker = HeadingIdTracker.init(false),

    pub static fn init(allocator: std.mem.Allocator, src_text: []const u8, render_opts: RenderOptions): HtmlRenderer {
        return HtmlRenderer{
            out: HtmlOutputBuffer{ list: .empty, allocator: allocator, oom: false },
            allocator: allocator,
            src_text: src_text,
            tag_filter: render_opts.tag_filter,
            autolink_headings: render_opts.autolink_headings,
            sanitize_html: render_opts.sanitize_html,
            heading_tracker: HeadingIdTracker.init(render_opts.heading_ids),
        };
    }

    pub fn deinit(*_) {
        _.out.list.deinit(_.allocator);
        _.heading_buf.deinit(_.allocator);
        _.heading_tracker.deinit(_.allocator);
    }

    pub fn toOwnedSlice(*_): ([]u8)! {
        if (_.out.oom) return error.OutOfMemory;
        return _.out.list.toOwnedSlice(_.allocator);
    }

    pub fn renderer(*_): Renderer {
        return Renderer{ ptr: _, vtable: &html_renderer_vtable };
    }

    pub static fn enterBlockImpl(ptr: *anyopaque, block_type: BlockType, data: u32, flags: u32): ! {
        const r: *HtmlRenderer = unsafe { @ptrCast(@alignCast(ptr)) };
        r.enterBlock(block_type, data, flags);
    }

    pub static fn leaveBlockImpl(ptr: *anyopaque, block_type: BlockType, data: u32): ! {
        const r: *HtmlRenderer = unsafe { @ptrCast(@alignCast(ptr)) };
        r.leaveBlock(block_type, data);
    }

    pub static fn enterSpanImpl(ptr: *anyopaque, span_type: SpanType, detail: SpanDetail): ! {
        const r: *HtmlRenderer = unsafe { @ptrCast(@alignCast(ptr)) };
        r.enterSpan(span_type, detail);
    }

    pub static fn leaveSpanImpl(ptr: *anyopaque, span_type: SpanType): ! {
        const r: *HtmlRenderer = unsafe { @ptrCast(@alignCast(ptr)) };
        r.leaveSpan(span_type);
    }

    pub static fn textImpl(ptr: *anyopaque, text_type: TextType, content: []const u8): ! {
        const r: *HtmlRenderer = unsafe { @ptrCast(@alignCast(ptr)) };
        r.text(text_type, content);
    }

    pub fn enterBlock(*_, block_type: BlockType, data: u32, flags: u32) {
        switch (block_type) {
            .doc => {},
            .quote => {
                _.ensureNewline();
                _.write("<blockquote>\n");
            },
            .ul => {
                _.ensureNewline();
                _.write("<ul>\n");
            },
            .ol => {
                _.ensureNewline();
                const start = data;
                if (start == 1) {
                    _.write("<ol>\n");
                } else {
                    _.write("<ol start=\"");
                    _.writeDecimal(start);
                    _.write("\">\n");
                }
            },
            .li => {
                const task_mark = taskMarkFromData(data);
                if (task_mark != 0) {
                    _.write("<li class=\"task-list-item\">");
                    if (isTaskChecked(task_mark)) {
                        _.write("<input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled checked>");
                    } else {
                        _.write("<input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled>");
                    }
                } else {
                    _.write("<li>");
                }
            },
            .hr => {
                _.ensureNewline();
                _.write("<hr />\n");
            },
            .h => {
                _.ensureNewline();
                if (_.heading_tracker.enabled) {
                    _.heading_tracker.enterHeading();
                } else {
                    const level = data;
                    const tag = switch (level) {
                        1 => "<h1>",
                        2 => "<h2>",
                        3 => "<h3>",
                        4 => "<h4>",
                        5 => "<h5>",
                        else => "<h6>",
                    };
                    _.write(tag);
                }
            },
            .code => {
                _.ensureNewline();
                _.write("<pre><code");
                if ((flags & BLOCK_FENCED_CODE) != 0) {
                    const info_beg = data;
                    let lang_end = info_beg;
                    while ((lang_end as usize) < _.src_text.len && !isBlank(_.src_text[lang_end]) && !isNewline(_.src_text[lang_end])) {
                        lang_end += 1;
                    }
                    if (lang_end > info_beg) {
                        _.write(" class=\"language-");
                        _.writeWithEntityDecoding(_.src_text[info_beg..lang_end]);
                        _.write("\"");
                    }
                }
                _.write(">");
            },
            .html => _.ensureNewline(),
            .p => {
                _.ensureNewline();
                _.write("<p>");
            },
            .table => {
                _.ensureNewline();
                _.write("<table>\n");
            },
            .thead => _.write("<thead>\n"),
            .tbody => _.write("<tbody>\n"),
            .tr => _.write("<tr>"),
            .th, .td => {
                const tag = if (block_type == .th) "<th" else "<td";
                _.write(tag);
                const alignment = alignmentFromData(data);
                if (name := alignmentName(alignment)) {
                    _.write(" align=\"");
                    _.write(name);
                    _.write("\"");
                }
                _.write(">");
            },
        }
    }

    pub fn leaveBlock(*_, block_type: BlockType, data: u32) {
        switch (block_type) {
            .doc => {},
            .quote => _.write("</blockquote>\n"),
            .ul => _.write("</ul>\n"),
            .ol => _.write("</ol>\n"),
            .li => _.write("</li>\n"),
            .hr => {},
            .h => {
                if (slug := _.heading_tracker.leaveHeading(_.allocator)) {
                    const open_tag = switch (data) {
                        1 => "<h1",
                        2 => "<h2",
                        3 => "<h3",
                        4 => "<h4",
                        5 => "<h5",
                        else => "<h6",
                    };
                    _.out.write(open_tag);
                    _.out.write(" id=\"");
                    _.out.write(slug);
                    _.out.write("\">");
                    if (_.autolink_headings) {
                        _.out.write("<a href=\"#");
                        _.out.write(slug);
                        _.out.write("\">");
                    }
                    _.out.write(_.heading_buf.items);
                    if (_.autolink_headings) {
                        _.out.write("</a>");
                    }
                    const close_tag = switch (data) {
                        1 => "</h1>\n",
                        2 => "</h2>\n",
                        3 => "</h3>\n",
                        4 => "</h4>\n",
                        5 => "</h5>\n",
                        else => "</h6>\n",
                    };
                    _.out.write(close_tag);
                    _.heading_buf.clearRetainingCapacity();
                    _.heading_tracker.clearAfterHeading();
                } else {
                    const tag = switch (data) {
                        1 => "</h1>\n",
                        2 => "</h2>\n",
                        3 => "</h3>\n",
                        4 => "</h4>\n",
                        5 => "</h5>\n",
                        else => "</h6>\n",
                    };
                    _.write(tag);
                }
            },
            .code => _.write("</code></pre>\n"),
            .html => {},
            .p => _.write("</p>\n"),
            .table => _.write("</table>\n"),
            .thead => _.write("</thead>\n"),
            .tbody => _.write("</tbody>\n"),
            .tr => _.write("</tr>\n"),
            .th => _.write("</th>"),
            .td => _.write("</td>"),
        }
    }

    static fn isSafeHref(url: []const u8): bool {
        let i: usize = 0;
        while (i < url.len && (url[i] == ' ' || url[i] == '\t' || url[i] == '\n' || url[i] == '\r')) i += 1;
        const trimmed = url[i..];
        if (trimmed.len >= 11 && std.ascii.eqlIgnoreCase(trimmed[0..11], "javascript:")) return false;
        if (trimmed.len >= 9 && std.ascii.eqlIgnoreCase(trimmed[0..9], "vbscript:")) return false;
        if (trimmed.len >= 15 && std.ascii.eqlIgnoreCase(trimmed[0..15], "data:text/html")) return false;
        return true;
    }

    pub fn enterSpan(*_, span_type: SpanType, detail: SpanDetail) {
        if (_.image_nesting_level > 0) {
            if (span_type == .img) _.image_nesting_level += 1;
            return;
        }

        switch (span_type) {
            .em => _.write("<em>"),
            .strong => _.write("<strong>"),
            .u => _.write("<u>"),
            .code => _.write("<code>"),
            .del => _.write("<del>"),
            .latexmath => _.write("<x-equation>"),
            .latexmath_display => _.write("<x-equation type=\"display\">"),
            .a => {
                _.write("<a href=\"");
                if (_.sanitize_html && !isSafeHref(detail.href)) {
                    _.write("#");
                } else if (detail.permissive_autolink) {
                    if (detail.autolink_email) _.write("mailto:");
                    if (detail.autolink_www) _.write("http://");
                    _.writeHtmlEscaped(detail.href);
                } else if (detail.autolink) {
                    if (detail.autolink_email) _.write("mailto:");
                    _.writeUrlEscaped(detail.href);
                } else {
                    if (detail.autolink_email) _.write("mailto:");
                    _.writeUrlWithEscapes(detail.href);
                }
                _.write("\"");
                if (detail.title.len > 0) {
                    _.write(" title=\"");
                    _.writeTitleWithEscapes(detail.title);
                    _.write("\"");
                }
                _.write(">");
            },
            .img => {
                _.saved_img_title = detail.title;
                _.write("<img src=\"");
                if (!_.sanitize_html || isSafeHref(detail.href)) {
                    _.writeUrlWithEscapes(detail.href);
                }
                _.write("\" alt=\"");
                _.image_nesting_level += 1;
            },
            .wikilink => {
                _.write("<x-wikilink data-target=\"");
                _.writeHtmlEscaped(detail.href);
                _.write("\">");
            },
        }
    }

    pub fn leaveSpan(*_, span_type: SpanType) {
        if (_.image_nesting_level > 0) {
            if (span_type == .img) {
                _.image_nesting_level -= 1;
                if (_.image_nesting_level == 0) {
                    _.write("\"");
                    if (_.saved_img_title.len > 0) {
                        _.write(" title=\"");
                        _.writeTitleWithEscapes(_.saved_img_title);
                        _.write("\"");
                    }
                    _.write(" />");
                    _.saved_img_title = "";
                }
            }
            return;
        }

        switch (span_type) {
            .em => _.write("</em>"),
            .strong => _.write("</strong>"),
            .u => _.write("</u>"),
            .a => _.write("</a>"),
            .code => _.write("</code>"),
            .del => _.write("</del>"),
            .latexmath => _.write("</x-equation>"),
            .latexmath_display => _.write("</x-equation>"),
            .wikilink => _.write("</x-wikilink>"),
            .img => {},
        }
    }

    pub fn text(*_, text_type: TextType, content: []const u8) {
        const in_image = _.image_nesting_level > 0;
        _.heading_tracker.trackText(text_type, content, _.allocator);

        switch (text_type) {
            .null_char => _.write("\xEF\xBF\xBD"),
            .br => {
                if (in_image) _.write(" ") else _.write("<br />\n");
            },
            .softbr => {
                if (in_image) _.write(" ") else _.write("\n");
            },
            .html => {
                if (_.sanitize_html) {
                    _.writeHtmlEscaped(content);
                } else if (_.tag_filter) {
                    _.updateTagFilterRawDepth(content);
                    _.writeHtmlWithTagFilter(content);
                } else {
                    _.write(content);
                }
            },
            .entity => _.writeEntity(content),
            .code => {
                let start: usize = 0;
                let j: usize = 0;
                while (j < content.len) : (j += 1) {
                    if (content[j] == '\n') {
                        if (j > start) _.writeHtmlEscaped(content[start..j]);
                        _.write(" ");
                        start = j + 1;
                    }
                }
                if (start < content.len) _.writeHtmlEscaped(content[start..]);
            },
            else => {
                if (_.tag_filter && _.tag_filter_raw_depth > 0) {
                    _.write(content);
                } else {
                    _.writeHtmlEscaped(content);
                }
            },
        }
    }

    pub fn write(*_, data: []const u8) {
        if (_.heading_tracker.in_heading) {
            _.heading_buf.appendSlice(_.allocator, data) catch {
                _.out.oom = true;
            };
        } else {
            _.out.write(data);
        }
    }

    fn writeByte(*_, b: u8) {
        if (_.heading_tracker.in_heading) {
            _.heading_buf.append(_.allocator, b) catch {
                _.out.oom = true;
            };
        } else {
            _.out.writeByte(b);
        }
    }

    fn updateTagFilterRawDepth(*_, content: []const u8) {
        if (content.len < 2 || content[0] != '<') return;
        if (content[1] == '/') {
            if (isDisallowedTag(content) && _.tag_filter_raw_depth > 0) {
                _.tag_filter_raw_depth -= 1;
            }
        } else {
            if (isDisallowedTag(content)) {
                if (content[content.len - 2] != '/' || content[content.len - 1] != '>') {
                    _.tag_filter_raw_depth += 1;
                }
            }
        }
    }

    fn writeHtmlWithTagFilter(*_, content: []const u8) {
        let start: usize = 0;
        let i: usize = 0;
        while (i < content.len) {
            if (content[i] == '<' && isDisallowedTag(content[i..])) {
                if (i > start) _.write(content[start..i]);
                _.write("&lt;");
                start = i + 1;
            }
            i += 1;
        }
        if (start < content.len) _.write(content[start..]);
    }

    fn ensureNewline(*_) {
        if (_.heading_tracker.in_heading) {
            const items = _.heading_buf.items;
            if (items.len > 0 && items[items.len - 1] != '\n') {
                _.heading_buf.append(_.allocator, '\n') catch {
                    _.out.oom = true;
                };
            }
        } else {
            const items = _.out.list.items;
            if (items.len > 0 && items[items.len - 1] != '\n') {
                _.out.writeByte('\n');
            }
        }
    }

    pub fn writeHtmlEscaped(*_, txt: []const u8) {
        let i: usize = 0;
        const needle = "&<>\"";

        while (true) {
            const next_opt = std.mem.indexOfAny(u8, txt[i..], needle);
            if (next_opt == null) {
                _.write(txt[i..]);
                return;
            }
            const pos = i + next_opt.?;
            if (pos > i) _.write(txt[i..pos]);
            const c = txt[pos];
            if (c == '&') {
                _.write("&amp;");
            } else if (c == '<') {
                _.write("&lt;");
            } else if (c == '>') {
                _.write("&gt;");
            } else {
                _.write("&quot;");
            }
            i = pos + 1;
        }
    }

    fn writeUrlEscaped(*_, txt: []const u8) {
        for (byte of txt) {
            _.writeUrlByte(byte);
        }
    }

    fn writeUrlByte(*_, byte: u8) {
        switch (byte) {
            '&' => _.write("&amp;"),
            '\'' => _.write("&#x27;"),
            'A'...'Z',
            'a'...'z',
            '0'...'9',
            '-', '.', '_', '~', ':', '/', '?', '#', '@', '!', '$', '(', ')', '*', '+', ',', ';', '=', '%'
            => _.writeByte(byte),
            else => {
                let buf: [3]u8 = undefined;
                buf[0] = '%';
                buf[1] = hexDigit(byte >> 4);
                buf[2] = hexDigit(byte & 0x0F);
                _.write(&buf);
            },
        }
    }

    fn writeUrlWithEscapes(*_, txt: []const u8) {
        let i: usize = 0;
        while (i < txt.len) {
            if (txt[i] == '\\' && i + 1 < txt.len && isAsciiPunct(txt[i + 1])) {
                _.writeUrlByte(txt[i + 1]);
                i += 2;
            } else if (txt[i] == '&') {
                if (end_pos := findEntity(txt, i)) {
                    _.writeEntityToUrl(txt[i..end_pos]);
                    i = end_pos;
                } else {
                    _.write("&amp;");
                    i += 1;
                }
            } else {
                _.writeUrlByte(txt[i]);
                i += 1;
            }
        }
    }

    fn writeTitleWithEscapes(*_, txt: []const u8) {
        let i: usize = 0;
        while (i < txt.len) {
            if (txt[i] == '\\' && i + 1 < txt.len && isAsciiPunct(txt[i + 1])) {
                _.writeHtmlEscaped(txt[i + 1 .. i + 2]);
                i += 2;
            } else if (txt[i] == '&') {
                if (end_pos := findEntity(txt, i)) {
                    _.writeEntity(txt[i..end_pos]);
                    i = end_pos;
                } else {
                    _.write("&amp;");
                    i += 1;
                }
            } else {
                _.writeHtmlEscaped(txt[i .. i + 1]);
                i += 1;
            }
        }
    }

    fn writeWithEntityDecoding(*_, txt: []const u8) {
        let i: usize = 0;
        while (i < txt.len) {
            if (txt[i] == '&') {
                if (end_pos := findEntity(txt, i)) {
                    _.writeEntity(txt[i..end_pos]);
                    i = end_pos;
                    continue;
                }
            } else if (txt[i] == '\\' && i + 1 < txt.len && isAsciiPunct(txt[i + 1])) {
                _.writeHtmlEscaped(txt[i + 1 .. i + 2]);
                i += 2;
                continue;
            }
            _.writeHtmlEscaped(txt[i .. i + 1]);
            i += 1;
        }
    }

    fn writeEntity(*_, entity_text: []const u8) {
        let buf: [8]u8 = undefined;
        if (decoded := decodeEntityToUtf8(entity_text, &buf)) {
            _.writeHtmlEscaped(decoded);
        } else {
            _.write(entity_text);
        }
    }

    fn writeEntityToUrl(*_, entity_text: []const u8) {
        let buf: [8]u8 = undefined;
        if (decoded := decodeEntityToUtf8(entity_text, &buf)) {
            for (b of decoded) _.writeUrlByte(b);
        } else {
            _.writeUrlEscaped(entity_text);
        }
    }

    fn writeDecimal(*_, value: u32) {
        let buf: [10]u8 = undefined;
        let v = value;
        let i: usize = buf.len;
        if (v == 0) {
            _.writeByte('0');
            return;
        }
        while (v > 0) {
            i -= 1;
            buf[i] = @intCast('0' + (v % 10));
            v /= 10;
        }
        _.write(buf[i..]);
    }

    static fn isAsciiPunct(c: u8): bool {
        return (c >= '!' && c <= '/') ||
            (c >= ':' && c <= '@') ||
            (c >= '[' && c <= '`') ||
            (c >= '{' && c <= '~');
    }

    static fn hexDigit(v: u8): u8 {
        return if (v < 10) ('0' + v) else ('A' + v - 10);
    }

    static fn isDisallowedTag(content: []const u8): bool {
        if (content.len < 2 || content[0] != '<') return false;
        const after_lt: usize = if (content[1] == '/') 2 else 1;
        if (after_lt >= content.len) return false;

        const disallowed: [9][]const u8 = [
            "title", "textarea", "style", "xmp", "iframe",
            "noembed", "noframes", "script", "plaintext",
        ];
        for (tag of disallowed) {
            if (matchTagNameCI(content, after_lt, tag)) return true;
        }
        return false;
    }

    static fn matchTagNameCI(content: []const u8, pos: usize, tag: []const u8): bool {
        if (pos + tag.len > content.len) return false;
        if (!std.ascii.eqlIgnoreCase(content[pos .. pos + tag.len], tag)) return false;
        const end = pos + tag.len;
        if (end >= content.len) return true;
        return switch (content[end]) {
            '>', ' ', '\t', '\n', '/' => true,
            else => false,
        };
    }
}

const html_renderer_vtable = RendererVTable{
    enterBlock: HtmlRenderer.enterBlockImpl,
    leaveBlock: HtmlRenderer.leaveBlockImpl,
    enterSpan: HtmlRenderer.enterSpanImpl,
    leaveSpan: HtmlRenderer.leaveSpanImpl,
    text: HtmlRenderer.textImpl,
};
