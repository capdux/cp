// Source: cp/lib/html/sanitizer.cp (Allowlist HTML5 Content Sanitizer)
// Allowlist-Based Streaming HTML5 Sanitizer with URL Protocol & Event Handler Defense

const std = @import("std");
import { HtmlEntities } from "./entities";
import { HtmlToken, HtmlTokenKind, HtmlAttribute, HtmlTokenizer } from "./tokenizer";

const DEFAULT_ALL_SAFE_TAGS: [62][]const u8 = [
    "a", "abbr", "article", "aside", "b", "blockquote", "br", "caption",
    "cite", "code", "col", "colgroup", "dd", "del", "details", "div",
    "dl", "dt", "em", "figcaption", "figure", "footer", "h1", "h2",
    "h3", "h4", "h5", "h6", "header", "hr", "i", "img",
    "ins", "kbd", "li", "main", "mark", "nav", "ol", "p",
    "pre", "q", "s", "samp", "section", "small", "span", "strong",
    "sub", "summary", "sup", "table", "tbody", "td", "tfoot", "th",
    "thead", "tr", "u", "ul", "var", "wbr",
];

const DEFAULT_SAFE_ATTRS: [19][]const u8 = [
    "alt", "title", "class", "id", "dir", "lang", "width", "height",
    "colspan", "rowspan", "align", "valign", "target", "rel", "start",
    "type", "open", "datetime", "name",
];

const DEFAULT_URL_ATTRS: [4][]const u8 = [
    "href", "src", "cite", "poster",
];

const EMPTY_TAG_LIST: []const []const u8 = [];

pub struct SanitizerPolicy {
    allowed_tags: []const []const u8 = DEFAULT_ALL_SAFE_TAGS[0..DEFAULT_ALL_SAFE_TAGS.len],
    allowed_attrs: []const []const u8 = DEFAULT_SAFE_ATTRS[0..DEFAULT_SAFE_ATTRS.len],
    url_attrs: []const []const u8 = DEFAULT_URL_ATTRS[0..DEFAULT_URL_ATTRS.len],
    strip_comments: bool = true,
    drop_dangerous_content: bool = true,
    allow_data_attrs: bool = false,
    max_nesting_depth: usize = 256,

    /// Standard safe HTML5 policy allowing formatting, lists, tables, headings, links, and images.
    pub static fn defaultSafe(): SanitizerPolicy {
        return SanitizerPolicy{};
    }

    /// Strict policy stripping all HTML tags and returning safe escaped plain text only.
    pub static fn strictTextOnly(): SanitizerPolicy {
        return SanitizerPolicy{
            allowed_tags: EMPTY_TAG_LIST,
            allowed_attrs: EMPTY_TAG_LIST,
            url_attrs: EMPTY_TAG_LIST,
            strip_comments: true,
            drop_dangerous_content: true,
            allow_data_attrs: false,
            max_nesting_depth: 256,
        };
    }

    /// Creates a custom allowlist policy with caller-provided tag, attribute, and URL attribute lists.
    pub static fn custom(
        tags: []const []const u8,
        attrs: []const []const u8,
        url_attrs: []const []const u8,
    ): SanitizerPolicy {
        return SanitizerPolicy{
            allowed_tags: tags,
            allowed_attrs: attrs,
            url_attrs: url_attrs,
            strip_comments: true,
            drop_dangerous_content: true,
            allow_data_attrs: false,
            max_nesting_depth: 256,
        };
    }

    pub fn isTagAllowed(tag: []const u8): bool {
        if (HtmlSanitizer.isForbiddenTag(tag)) return false;
        for (allowed of _.allowed_tags) {
            if (std.ascii.eqlIgnoreCase(allowed, tag)) return true;
        }
        return false;
    }

    pub fn isUrlAttr(attr: []const u8): bool {
        for (u of _.url_attrs) {
            if (std.ascii.eqlIgnoreCase(u, attr)) return true;
        }
        return false;
    }

    pub fn isSafeNonUrlAttr(attr: []const u8): bool {
        if (attr.len >= 2 && (attr[0] == 'o' || attr[0] == 'O') && (attr[1] == 'n' || attr[1] == 'N')) {
            return false;
        }
        if (_.allow_data_attrs && attr.len > 5 && std.ascii.eqlIgnoreCase(attr[0..5], "data-")) {
            return true;
        }
        for (a of _.allowed_attrs) {
            if (std.ascii.eqlIgnoreCase(a, attr)) return true;
        }
        return false;
    }
}

struct SanitizerTagFrame {
    tag_name: []const u8 = "",
    emitted: bool = false,
}

struct DropTagFrame {
    name: []const u8 = "",
}

pub struct HtmlSanitizer {
    /// Returns true for inherently dangerous tags whose content must always be dropped.
    pub static fn isForbiddenTag(tag: []const u8): bool {
        if (std.ascii.eqlIgnoreCase(tag, "script")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "style")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "iframe")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "object")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "embed")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "svg")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "math")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "applet")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "frame")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "frameset")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "base")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "noscript")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "template")) return true;
        return false;
    }

    static fn isControlOrSpace(c: u8): bool {
        return c <= 0x20 || c == 0x7F;
    }

    static fn isValidSchemeChar(c: u8): bool {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
    }

    /// Validates a URL against allowlisted protocols (`http://`, `https://`, `mailto:`, `#`, or relative paths)
    /// while strictly rejecting control-character and whitespace obfuscation bypasses.
    pub static fn isSafeUrl(url: []const u8): bool {
        let start: usize = 0;
        let end: usize = url.len;
        while (start < end && HtmlSanitizer.isControlOrSpace(url[start])) {
            start += 1;
        }
        while (end > start && HtmlSanitizer.isControlOrSpace(url[end - 1])) {
            end -= 1;
        }
        if (start >= end) return false;
        const trimmed = url[start..end];

        // Reject any embedded C0/DEL control character (prevents java\tscript:, java\nscript:, \x00 bypasses)
        for (c of trimmed) {
            if (c < 0x20 || c == 0x7F) return false;
        }

        // Scan up to first ':', '/', '?', or '#'
        let i: usize = 0;
        while (i < trimmed.len) : (i += 1) {
            const c = trimmed[i];
            if (c == '/' || c == '?' || c == '#') {
                return true;
            }
            if (c == ':') {
                const scheme = trimmed[0..i];
                if (scheme.len == 0) return false;
                for (sc of scheme) {
                    if (!HtmlSanitizer.isValidSchemeChar(sc)) return false;
                }
                if (std.ascii.eqlIgnoreCase(scheme, "http") || std.ascii.eqlIgnoreCase(scheme, "https")) {
                    return i + 3 <= trimmed.len && std.mem.eql(u8, trimmed[i .. i + 3], "://");
                }
                if (std.ascii.eqlIgnoreCase(scheme, "mailto")) {
                    return i + 1 < trimmed.len;
                }
                return false;
            }
        }

        // No scheme delimiter present -> relative URL path
        return true;
    }

    static fn appendLowerAscii(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): ! {
        for (c of s) {
            out.append(alloc, std.ascii.toLower(c))?;
        }
    }

    static fn isEventHandlerAttr(name: []const u8): bool {
        return name.len >= 2 && (name[0] == 'o' || name[0] == 'O') && (name[1] == 'n' || name[1] == 'N');
    }

    /// Sanitizes HTML using an allowlist streaming state machine. Caller owns returned slice.
    pub static fn sanitize(alloc: std.mem.Allocator, html: []const u8, policy: SanitizerPolicy): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let open_stack: std.ArrayList(SanitizerTagFrame) = .empty;
        defer open_stack.deinit(alloc);

        let drop_stack: std.ArrayList(DropTagFrame) = .empty;
        defer drop_stack.deinit(alloc);

        const depth_limit: usize = if (policy.max_nesting_depth > 0) policy.max_nesting_depth else 256;

        let tok = HtmlTokenizer.init(html);
        while (t := tok.next(alloc)?) {
            if (t.kind == .StartTag || t.kind == .SelfClosingTag) {
                const is_void = HtmlTokenizer.isVoidElement(t.tag_name);
                const is_self = t.self_closing || is_void;

                if (drop_stack.items.len > 0) {
                    if (!is_self && drop_stack.items.len < depth_limit) {
                        drop_stack.append(alloc, DropTagFrame{ name: t.tag_name })?;
                    }
                    if (t.attrs.len > 0) alloc.free(t.attrs);
                    continue;
                }

                if (policy.drop_dangerous_content && HtmlSanitizer.isForbiddenTag(t.tag_name)) {
                    if (!is_self) {
                        drop_stack.append(alloc, DropTagFrame{ name: t.tag_name })?;
                    }
                    if (t.attrs.len > 0) alloc.free(t.attrs);
                    continue;
                }

                if (!policy.isTagAllowed(t.tag_name)) {
                    if (!is_self && open_stack.items.len < depth_limit) {
                        open_stack.append(alloc, SanitizerTagFrame{
                            tag_name: t.tag_name,
                            emitted: false,
                        })?;
                    }
                    if (t.attrs.len > 0) alloc.free(t.attrs);
                    continue;
                }

                if (!is_self && open_stack.items.len >= depth_limit) {
                    if (t.attrs.len > 0) alloc.free(t.attrs);
                    continue;
                }

                out.append(alloc, '<')?;
                HtmlSanitizer.appendLowerAscii(&out, alloc, t.tag_name)?;

                for (a of t.attrs) {
                    if (HtmlSanitizer.isEventHandlerAttr(a.name)) {
                        continue;
                    }
                    if (policy.isUrlAttr(a.name)) {
                        if (!a.has_value) continue;
                        const decoded_url = HtmlEntities.decode(alloc, a.value)?;
                        defer alloc.free(decoded_url);
                        if (!HtmlSanitizer.isSafeUrl(decoded_url)) {
                            continue;
                        }
                        const safe_url = HtmlEntities.encodeAttr(alloc, decoded_url)?;
                        defer alloc.free(safe_url);
                        out.append(alloc, ' ')?;
                        HtmlSanitizer.appendLowerAscii(&out, alloc, a.name)?;
                        out.appendSlice(alloc, "=\"")?;
                        out.appendSlice(alloc, safe_url)?;
                        out.append(alloc, '"')?;
                    } else if (policy.isSafeNonUrlAttr(a.name)) {
                        out.append(alloc, ' ')?;
                        HtmlSanitizer.appendLowerAscii(&out, alloc, a.name)?;
                        if (a.has_value) {
                            const decoded_val = HtmlEntities.decode(alloc, a.value)?;
                            defer alloc.free(decoded_val);
                            const safe_val = HtmlEntities.encodeAttr(alloc, decoded_val)?;
                            defer alloc.free(safe_val);
                            out.appendSlice(alloc, "=\"")?;
                            out.appendSlice(alloc, safe_val)?;
                            out.append(alloc, '"')?;
                        }
                    }
                }

                if (t.attrs.len > 0) alloc.free(t.attrs);

                if (t.self_closing && !is_void) {
                    out.appendSlice(alloc, "/>")?;
                } else {
                    out.append(alloc, '>')?;
                }

                if (!is_self) {
                    open_stack.append(alloc, SanitizerTagFrame{
                        tag_name: t.tag_name,
                        emitted: true,
                    })?;
                }
            } else if (t.kind == .EndTag) {
                if (drop_stack.items.len > 0) {
                    let match_drop: usize? = null;
                    let idx = drop_stack.items.len;
                    while (idx > 0) {
                        idx -= 1;
                        if (std.ascii.eqlIgnoreCase(drop_stack.items[idx].name, t.tag_name)) {
                            match_drop = idx;
                            break;
                        }
                    }
                    if (m := match_drop) {
                        while (drop_stack.items.len > m) {
                            * = drop_stack.pop().?;
                        }
                    }
                    continue;
                }

                if (HtmlTokenizer.isVoidElement(t.tag_name)) {
                    continue;
                }

                let match_idx: usize? = null;
                let idx = open_stack.items.len;
                while (idx > 0) {
                    idx -= 1;
                    if (std.ascii.eqlIgnoreCase(open_stack.items[idx].tag_name, t.tag_name)) {
                        match_idx = idx;
                        break;
                    }
                }

                if (m := match_idx) {
                    while (open_stack.items.len > m) {
                        const fr = open_stack.pop().?;
                        if (fr.emitted) {
                            out.appendSlice(alloc, "</")?;
                            HtmlSanitizer.appendLowerAscii(&out, alloc, fr.tag_name)?;
                            out.append(alloc, '>')?;
                        }
                    }
                }
            } else if (t.kind == .Text) {
                if (drop_stack.items.len > 0) {
                    continue;
                }
                const decoded_text = HtmlEntities.decode(alloc, t.text)?;
                defer alloc.free(decoded_text);
                const safe_text = HtmlEntities.encodeText(alloc, decoded_text)?;
                defer alloc.free(safe_text);
                out.appendSlice(alloc, safe_text)?;
            } else {
                // Comment, Doctype, and CDATA tokens are stripped in sanitized output
                if (!policy.strip_comments && t.kind == .Comment && drop_stack.items.len == 0) {
                    out.appendSlice(alloc, "<!--")?;
                    const safe_comment = HtmlEntities.encodeText(alloc, t.text)?;
                    defer alloc.free(safe_comment);
                    out.appendSlice(alloc, safe_comment)?;
                    out.appendSlice(alloc, "-->")?;
                }
            }
        }

        // Auto-close any unclosed allowed tags cleanly at EOF
        while (open_stack.items.len > 0) {
            const fr = open_stack.pop().?;
            if (fr.emitted) {
                out.appendSlice(alloc, "</")?;
                HtmlSanitizer.appendLowerAscii(&out, alloc, fr.tag_name)?;
                out.append(alloc, '>')?;
            }
        }

        return out.toOwnedSlice(alloc);
    }
}
