// Source: Absorbed from /data/dev/bun/src/lolhtml_sys/lol_html.cp | Synced: 2026-11-21T12:28:08Z
// lib/html/selector.cp
// Compound CSS Selector Parser & Full HTMLRewriter Element/Tag/Attribute Streaming Engine
// Absorbed from Bun src/lolhtml_sys/lol_html.cp & src/css/selectors/

const std = @import("std");
import { HtmlEscape } from "./escape";
import { HtmlToken, HtmlTokenKind, HtmlAttribute, HtmlTokenizer } from "./tokenizer";

pub enum AttrMatchOp: u8 {
    Exists = 0,
    Equals = 1,
    Prefix = 2,
    Suffix = 3,
    Contains = 4,
}

pub struct AttrSelector {
    name: []const u8 = "",
    value: []const u8 = "",
    op: AttrMatchOp = .Exists,
}

pub struct CssSelector {
    tag_name: []const u8 = "",
    id_name: []const u8 = "",
    class_name: []const u8 = "",
    attr_name: []const u8 = "",
    attr_val: []const u8 = "",
    classes: [8][]const u8 = ["", "", "", "", "", "", "", ""],
    class_count: usize = 0,
    attrs: [8]AttrSelector = [AttrSelector{}, AttrSelector{}, AttrSelector{}, AttrSelector{}, AttrSelector{}, AttrSelector{}, AttrSelector{}, AttrSelector{}],
    attr_count: usize = 0,

    /// Parses compound CSS selectors such as `div`, `a#main`, `div.card#hero`, `a.btn.primary[href][type="email"]`.
    pub static fn parse(sel: []const u8): CssSelector {
        const trimmed = std.mem.trim(u8, sel, " \t\r\n");
        let res = CssSelector{};
        if (trimmed.len == 0) return res;

        let i: usize = 0;
        // Optional leading tag name or `*`
        while (i < trimmed.len && trimmed[i] != '#' && trimmed[i] != '.' && trimmed[i] != '[') {
            i += 1;
        }
        if (i > 0) {
            res.tag_name = trimmed[0..i];
        }

        while (i < trimmed.len) {
            const ch = trimmed[i];
            if (ch == '#') {
                i += 1;
                const start = i;
                while (i < trimmed.len && trimmed[i] != '#' && trimmed[i] != '.' && trimmed[i] != '[') {
                    i += 1;
                }
                res.id_name = trimmed[start..i];
            } else if (ch == '.') {
                i += 1;
                const start = i;
                while (i < trimmed.len && trimmed[i] != '#' && trimmed[i] != '.' && trimmed[i] != '[') {
                    i += 1;
                }
                const cls = trimmed[start..i];
                if (cls.len > 0) {
                    if (res.class_name.len == 0) {
                        res.class_name = cls;
                    }
                    if (res.class_count < 8) {
                        res.classes[res.class_count] = cls;
                        res.class_count += 1;
                    }
                }
            } else if (ch == '[') {
                i += 1;
                const expr_start = i;
                let in_quote: u8 = 0;
                while (i < trimmed.len) {
                    const c = trimmed[i];
                    if (in_quote != 0) {
                        if (c == in_quote) in_quote = 0;
                    } else {
                        if (c == '"' || c == '\'') {
                            in_quote = c;
                        } else if (c == ']') {
                            break;
                        }
                    }
                    i += 1;
                }
                const expr = std.mem.trim(u8, trimmed[expr_start..i], " \t");
                if (i < trimmed.len && trimmed[i] == ']') {
                    i += 1;
                }

                if (expr.len > 0) {
                    let op: AttrMatchOp = .Exists;
                    let aname = expr;
                    let aval: []const u8 = "";

                    if (eq_idx := std.mem.indexOfScalar(u8, expr, '=')) {
                        let name_end = eq_idx;
                        if (eq_idx > 0) {
                            const prev = expr[eq_idx - 1];
                            if (prev == '^') {
                                op = .Prefix;
                                name_end = eq_idx - 1;
                            } else if (prev == '$') {
                                op = .Suffix;
                                name_end = eq_idx - 1;
                            } else if (prev == '*') {
                                op = .Contains;
                                name_end = eq_idx - 1;
                            } else {
                                op = .Equals;
                            }
                        } else {
                            op = .Equals;
                        }
                        aname = std.mem.trim(u8, expr[0..name_end], " \t");
                        let raw_val = std.mem.trim(u8, expr[eq_idx + 1 ..], " \t");
                        if (raw_val.len >= 2 && ((raw_val[0] == '"' && raw_val[raw_val.len - 1] == '"') || (raw_val[0] == '\'' && raw_val[raw_val.len - 1] == '\''))) {
                            raw_val = raw_val[1 .. raw_val.len - 1];
                        }
                        aval = raw_val;
                    }

                    if (res.attr_name.len == 0) {
                        res.attr_name = aname;
                        res.attr_val = aval;
                    }
                    if (res.attr_count < 8) {
                        res.attrs[res.attr_count] = AttrSelector{
                            name: aname,
                            value: aval,
                            op: op,
                        };
                        res.attr_count += 1;
                    }
                }
            } else {
                i += 1;
            }
        }
        return res;
    }

    pub fn matchesTag(tag: []const u8): bool {
        if (_.tag_name.len == 0 || std.mem.eql(u8, _.tag_name, "*")) {
            return true;
        }
        return std.ascii.eqlIgnoreCase(_.tag_name, tag);
    }

    static fn classListContains(class_attr: []const u8, target: []const u8): bool {
        let iter = std.mem.tokenizeAny(u8, class_attr, " \t\r\n");
        while (tok := iter.next()) {
            if (std.mem.eql(u8, tok, target)) return true;
        }
        return false;
    }

    static fn findAttr(attrs: []const HtmlAttribute, name: []const u8): HtmlAttribute? {
        for (a of attrs) {
            if (std.ascii.eqlIgnoreCase(a.name, name)) return a;
        }
        return null;
    }

    /// Matches a tag name + attribute slice against all tag, ID, class, and attribute rules.
    pub fn matchesAttributes(tag: []const u8, tok_attrs: []const HtmlAttribute): bool {
        if (!_.matchesTag(tag)) return false;

        if (_.id_name.len > 0) {
            const id_attr = CssSelector.findAttr(tok_attrs, "id") ?? return false;
            if (!std.mem.eql(u8, id_attr.value, _.id_name)) return false;
        }

        if (_.class_count > 0) {
            const cls_attr = CssSelector.findAttr(tok_attrs, "class") ?? return false;
            let ci: usize = 0;
            while (ci < _.class_count) : (ci += 1) {
                if (!CssSelector.classListContains(cls_attr.value, _.classes[ci])) {
                    return false;
                }
            }
        }

        let ai: usize = 0;
        while (ai < _.attr_count) : (ai += 1) {
            const req = _.attrs[ai];
            const found = CssSelector.findAttr(tok_attrs, req.name) ?? return false;
            switch (req.op) {
                .Exists => {},
                .Equals => {
                    if (!std.mem.eql(u8, found.value, req.value)) return false;
                },
                .Prefix => {
                    if (!std.mem.startsWith(u8, found.value, req.value)) return false;
                },
                .Suffix => {
                    if (!std.mem.endsWith(u8, found.value, req.value)) return false;
                },
                .Contains => {
                    if (std.mem.indexOf(u8, found.value, req.value) == null) return false;
                },
            }
        }

        return true;
    }

    pub fn matchesToken(tok: HtmlToken): bool {
        if (tok.kind != .StartTag && tok.kind != .SelfClosingTag) return false;
        return _.matchesAttributes(tok.tag_name, tok.attrs);
    }
}

pub struct HtmlElement {
    allocator: std.mem.Allocator = undefined,
    orig_tag: []const u8 = "",
    current_tag: []const u8 = "",
    attrs: std.ArrayList(HtmlAttribute) = .empty,
    self_closing: bool = false,
    removed: bool = false,
    keep_content: bool = false,
    has_replace: bool = false,
    has_inner: bool = false,
    before_buf: std.ArrayList(u8) = .empty,
    after_buf: std.ArrayList(u8) = .empty,
    prepend_buf: std.ArrayList(u8) = .empty,
    append_buf: std.ArrayList(u8) = .empty,
    inner_buf: std.ArrayList(u8) = .empty,
    replace_buf: std.ArrayList(u8) = .empty,

    pub static fn init(allocator: std.mem.Allocator, tag_name: []const u8, initial_attrs: []const HtmlAttribute, self_closing: bool): HtmlElement! {
        let el = HtmlElement{
            allocator: allocator,
            orig_tag: tag_name,
            current_tag: tag_name,
            self_closing: self_closing,
        };
        for (a of initial_attrs) {
            el.attrs.append(allocator, a)?;
        }
        return el;
    }

    pub fn deinit(*_) {
        _.attrs.deinit(_.allocator);
        _.before_buf.deinit(_.allocator);
        _.after_buf.deinit(_.allocator);
        _.prepend_buf.deinit(_.allocator);
        _.append_buf.deinit(_.allocator);
        _.inner_buf.deinit(_.allocator);
        _.replace_buf.deinit(_.allocator);
    }

    pub fn tagName(): []const u8 {
        return _.current_tag;
    }

    pub fn setTagName(*_, new_name: []const u8) {
        _.current_tag = new_name;
    }

    pub fn getAttribute(name: []const u8): ([]const u8)? {
        for (a of _.attrs.items) {
            if (std.ascii.eqlIgnoreCase(a.name, name)) {
                return a.value;
            }
        }
        return null;
    }

    pub fn hasAttribute(name: []const u8): bool {
        for (a of _.attrs.items) {
            if (std.ascii.eqlIgnoreCase(a.name, name)) {
                return true;
            }
        }
        return false;
    }

    pub fn setAttribute(*_, name: []const u8, value: []const u8) {
        let i: usize = 0;
        while (i < _.attrs.items.len) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(_.attrs.items[i].name, name)) {
                _.attrs.items[i].value = value;
                _.attrs.items[i].has_value = true;
                return;
            }
        }
        _.attrs.append(_.allocator, HtmlAttribute{
            name: name,
            value: value,
            has_value: true,
        }) catch {};
    }

    pub fn removeAttribute(*_, name: []const u8) {
        let i: usize = 0;
        while (i < _.attrs.items.len) {
            if (std.ascii.eqlIgnoreCase(_.attrs.items[i].name, name)) {
                * = _.attrs.orderedRemove(i);
                continue;
            }
            i += 1;
        }
    }

    fn appendContent(*_, target: *std.ArrayList(u8), content: []const u8, is_html: bool) {
        if (is_html) {
            target.appendSlice(_.allocator, content) catch {};
        } else {
            const esc = HtmlEscape.escapeAlloc(_.allocator, content) catch return;
            defer _.allocator.free(esc);
            target.appendSlice(_.allocator, esc) catch {};
        }
    }

    pub fn before(*_, content: []const u8, is_html: bool) {
        _.appendContent(&_.before_buf, content, is_html);
    }

    pub fn after(*_, content: []const u8, is_html: bool) {
        _.appendContent(&_.after_buf, content, is_html);
    }

    pub fn prepend(*_, content: []const u8, is_html: bool) {
        _.appendContent(&_.prepend_buf, content, is_html);
    }

    pub fn append(*_, content: []const u8, is_html: bool) {
        _.appendContent(&_.append_buf, content, is_html);
    }

    pub fn setInnerContent(*_, content: []const u8, is_html: bool) {
        _.inner_buf.clearRetainingCapacity();
        _.has_inner = true;
        _.appendContent(&_.inner_buf, content, is_html);
    }

    pub fn replaceWith(*_, content: []const u8, is_html: bool) {
        _.replace_buf.clearRetainingCapacity();
        _.has_replace = true;
        _.removed = true;
        _.appendContent(&_.replace_buf, content, is_html);
    }

    pub fn replaceHtml(*_, content: []const u8) {
        _.replaceWith(content, true);
    }

    pub fn replaceText(*_, content: []const u8) {
        _.replaceWith(content, false);
    }

    pub fn remove(*_) {
        _.removed = true;
        _.has_replace = false;
    }

    pub fn removeAndKeepContent(*_) {
        _.keep_content = true;
    }
}

pub struct ElementHandlerRule {
    selector: CssSelector,
    handler: (fn (*HtmlElement) void)? = null,
    handler_ctx: (fn (*HtmlElement, (*anyopaque)?) void)? = null,
    ctx: (*anyopaque)? = null,
}

struct OpenElementFrame {
    orig_tag: []const u8 = "",
    new_tag: []const u8 = "",
    suppress_inner: bool = false,
    remove_end_tag: bool = false,
    append_bytes: []u8 = [],
    after_bytes: []u8 = [],
}

pub struct HtmlRewriter {
    allocator: std.mem.Allocator = undefined,
    rules: std.ArrayList(ElementHandlerRule) = .empty,

    pub static fn init(allocator: std.mem.Allocator): HtmlRewriter {
        return HtmlRewriter{
            allocator: allocator,
            rules: .empty,
        };
    }

    pub fn deinit(*_) {
        _.rules.deinit(_.allocator);
    }

    /// Registers a CSS selector and element handler callback on the rewriter.
    pub fn on(*_, selector: []const u8, handler: fn (*HtmlElement) void): ! {
        _.rules.append(_.allocator, ElementHandlerRule{
            selector: CssSelector.parse(selector),
            handler: handler,
        })?;
    }

    /// Registers a CSS selector and stateful element handler callback with context pointer.
    pub fn onWithCtx(*_, selector: []const u8, handler: fn (*HtmlElement, (*anyopaque)?) void, ctx: (*anyopaque)?): ! {
        _.rules.append(_.allocator, ElementHandlerRule{
            selector: CssSelector.parse(selector),
            handler_ctx: handler,
            ctx: ctx,
        })?;
    }

    static fn writeAttrValueEscaped(out: *std.ArrayList(u8), allocator: std.mem.Allocator, val: []const u8): ! {
        for (c of val) {
            if (c == '"') {
                out.appendSlice(allocator, "&quot;")?;
            } else {
                out.append(allocator, c)?;
            }
        }
    }

    /// Streams `html` through the tokenizer and applies all matching selector handlers, returning rewritten HTML.
    pub fn transform(*_, html: []const u8): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.allocator);

        let stack: std.ArrayList(OpenElementFrame) = .empty;
        defer {
            for (fr of stack.items) {
                if (fr.append_bytes.len > 0) _.allocator.free(fr.append_bytes);
                if (fr.after_bytes.len > 0) _.allocator.free(fr.after_bytes);
            }
            stack.deinit(_.allocator);
        }

        let suppress_depth: usize = 0;
        let tok = HtmlTokenizer.init(html);

        while (t := tok.next(_.allocator)?) {
            if (t.kind == .StartTag || t.kind == .SelfClosingTag) {
                const is_void = HtmlTokenizer.isVoidElement(t.tag_name);
                const is_self = t.self_closing || is_void;

                if (suppress_depth > 0) {
                    if (!is_self) {
                        stack.append(_.allocator, OpenElementFrame{
                            orig_tag: t.tag_name,
                            new_tag: t.tag_name,
                            suppress_inner: true,
                            remove_end_tag: true,
                        })?;
                        suppress_depth += 1;
                    }
                    if (t.attrs.len > 0) _.allocator.free(t.attrs);
                    continue;
                }

                let el = HtmlElement.init(_.allocator, t.tag_name, t.attrs, t.self_closing)?;
                if (t.attrs.len > 0) _.allocator.free(t.attrs);
                defer el.deinit();

                for (rule of _.rules.items) {
                    if (rule.selector.matchesAttributes(el.tagName(), el.attrs.items)) {
                        if (cb := rule.handler) {
                            cb(&el);
                        }
                        if (cb_ctx := rule.handler_ctx) {
                            cb_ctx(&el, rule.ctx);
                        }
                    }
                }

                if (el.before_buf.items.len > 0) {
                    out.appendSlice(_.allocator, el.before_buf.items)?;
                }

                if (el.removed) {
                    if (el.has_replace) {
                        out.appendSlice(_.allocator, el.replace_buf.items)?;
                    }
                    if (is_self) {
                        if (el.after_buf.items.len > 0) {
                            out.appendSlice(_.allocator, el.after_buf.items)?;
                        }
                    } else {
                        let after_copy: []u8 = [];
                        if (el.after_buf.items.len > 0) {
                            after_copy = _.allocator.dupe(u8, el.after_buf.items)?;
                        }
                        stack.append(_.allocator, OpenElementFrame{
                            orig_tag: t.tag_name,
                            new_tag: el.current_tag,
                            suppress_inner: true,
                            remove_end_tag: true,
                            after_bytes: after_copy,
                        })?;
                        suppress_depth += 1;
                    }
                    continue;
                }

                if (!el.keep_content) {
                    out.append(_.allocator, '<')?;
                    out.appendSlice(_.allocator, el.current_tag)?;
                    for (a of el.attrs.items) {
                        out.append(_.allocator, ' ')?;
                        out.appendSlice(_.allocator, a.name)?;
                        if (a.has_value) {
                            out.appendSlice(_.allocator, "=\"")?;
                            HtmlRewriter.writeAttrValueEscaped(&out, _.allocator, a.value)?;
                            out.append(_.allocator, '"')?;
                        }
                    }
                    if (t.self_closing) {
                        out.appendSlice(_.allocator, "/>")?;
                    } else {
                        out.append(_.allocator, '>')?;
                    }
                }

                if (el.prepend_buf.items.len > 0) {
                    out.appendSlice(_.allocator, el.prepend_buf.items)?;
                }
                if (el.has_inner) {
                    out.appendSlice(_.allocator, el.inner_buf.items)?;
                }

                if (is_self) {
                    if (el.append_buf.items.len > 0) {
                        out.appendSlice(_.allocator, el.append_buf.items)?;
                    }
                    if (el.after_buf.items.len > 0) {
                        out.appendSlice(_.allocator, el.after_buf.items)?;
                    }
                } else {
                    let app_copy: []u8 = [];
                    if (el.append_buf.items.len > 0) {
                        app_copy = _.allocator.dupe(u8, el.append_buf.items)?;
                    }
                    let aft_copy: []u8 = [];
                    if (el.after_buf.items.len > 0) {
                        aft_copy = _.allocator.dupe(u8, el.after_buf.items)?;
                    }
                    if (el.has_inner) suppress_depth += 1;
                    stack.append(_.allocator, OpenElementFrame{
                        orig_tag: t.tag_name,
                        new_tag: el.current_tag,
                        suppress_inner: el.has_inner,
                        remove_end_tag: el.keep_content,
                        append_bytes: app_copy,
                        after_bytes: aft_copy,
                    })?;
                }
            } else if (t.kind == .EndTag) {
                // Find matching open tag from top of stack
                let match_idx: usize? = null;
                let idx = stack.items.len;
                while (idx > 0) {
                    idx -= 1;
                    if (std.ascii.eqlIgnoreCase(stack.items[idx].orig_tag, t.tag_name)) {
                        match_idx = idx;
                        break;
                    }
                }

                if (m := match_idx) {
                    while (stack.items.len > m) {
                        const fr = stack.pop().?;
                        if (fr.suppress_inner && suppress_depth > 0) {
                            suppress_depth -= 1;
                        }
                        if (suppress_depth == 0) {
                            if (fr.append_bytes.len > 0) {
                                out.appendSlice(_.allocator, fr.append_bytes)?;
                            }
                            if (!fr.remove_end_tag) {
                                out.appendSlice(_.allocator, "</")?;
                                out.appendSlice(_.allocator, fr.new_tag)?;
                                out.append(_.allocator, '>')?;
                            }
                            if (fr.after_bytes.len > 0) {
                                out.appendSlice(_.allocator, fr.after_bytes)?;
                            }
                        }
                        if (fr.append_bytes.len > 0) _.allocator.free(fr.append_bytes);
                        if (fr.after_bytes.len > 0) _.allocator.free(fr.after_bytes);
                    }
                } else if (suppress_depth == 0) {
                    out.appendSlice(_.allocator, t.raw)?;
                }
            } else {
                // Text / Comment / Doctype / Cdata
                if (suppress_depth == 0) {
                    out.appendSlice(_.allocator, t.raw)?;
                }
            }
        }

        return out.toOwnedSlice(_.allocator);
    }

    /// Strips all `<...>` HTML tags from `html` (properly ignoring comments and quoted `>` inside attributes) into `out_buf`.
    pub static fn stripTags(html: []const u8, out_buf: []u8): []const u8 {
        let i: usize = 0;
        let out_pos: usize = 0;

        while (i < html.len && out_pos < out_buf.len) {
            if (html[i] == '<') {
                if (i + 4 <= html.len && std.mem.eql(u8, html[i .. i + 4], "<!--")) {
                    if (end_idx := std.mem.indexOf(u8, html[i + 4 ..], "-->")) {
                        i = i + 4 + end_idx + 3;
                        continue;
                    }
                    break;
                }
                i += 1;
                let quote: u8 = 0;
                while (i < html.len) {
                    const ch = html[i];
                    if (quote != 0) {
                        if (ch == quote) quote = 0;
                    } else {
                        if (ch == '"' || ch == '\'') {
                            quote = ch;
                        } else if (ch == '>') {
                            i += 1;
                            break;
                        }
                    }
                    i += 1;
                }
                continue;
            }
            out_buf[out_pos] = html[i];
            out_pos += 1;
            i += 1;
        }
        return out_buf[0..out_pos];
    }

    /// Rewrites all occurrences of `<old_tag>` and `</old_tag>` to `<new_tag>` and `</new_tag>`.
    pub static fn renameTag(html: []const u8, old_tag: []const u8, new_tag: []const u8, out_buf: []u8): []const u8 {
        let i: usize = 0;
        let out_pos: usize = 0;

        while (i < html.len && out_pos < out_buf.len) {
            if (html[i] == '<') {
                const is_close = (i + 1 < html.len && html[i + 1] == '/');
                const tag_start = if (is_close) (i + 2) else (i + 1);
                if (tag_start + old_tag.len < html.len &&
                    std.mem.eql(u8, html[tag_start .. tag_start + old_tag.len], old_tag) &&
                    (html[tag_start + old_tag.len] == '>' || html[tag_start + old_tag.len] == ' ' || html[tag_start + old_tag.len] == '/'))
                {
                    out_buf[out_pos] = '<';
                    out_pos += 1;
                    if (is_close) {
                        out_buf[out_pos] = '/';
                        out_pos += 1;
                    }
                    let k: usize = 0;
                    while (k < new_tag.len && out_pos < out_buf.len) {
                        out_buf[out_pos] = new_tag[k];
                        out_pos += 1;
                        k += 1;
                    }
                    i = tag_start + old_tag.len;
                    continue;
                }
            }
            out_buf[out_pos] = html[i];
            out_pos += 1;
            i += 1;
        }
        return out_buf[0..out_pos];
    }
}
