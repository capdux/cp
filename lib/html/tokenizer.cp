// Source: Absorbed from /data/dev/bun/src/lolhtml_sys/lol_html.cp | Synced: 2026-11-23T22:01:50Z
// lib/html/tokenizer.cp
// HTML5 Spec-Compliant Streaming Tokenizer (StartTag, EndTag, SelfClosing, Comment, Doctype, CDATA, RawText)

const std = @import("std");

pub enum HtmlTokenKind: u8 {
    StartTag = 0,
    EndTag = 1,
    SelfClosingTag = 2,
    Text = 3,
    Comment = 4,
    Doctype = 5,
    Cdata = 6,
}

pub struct HtmlAttribute {
    name: []const u8 = "",
    value: []const u8 = "",
    has_value: bool = false,
}

pub struct HtmlToken {
    kind: HtmlTokenKind = .Text,
    tag_name: []const u8 = "",
    raw: []const u8 = "",
    text: []const u8 = "",
    attrs: []HtmlAttribute = [],
    self_closing: bool = false,

    pub fn getAttribute(name: []const u8): ([]const u8)? {
        for (a of _.attrs) {
            if (std.ascii.eqlIgnoreCase(a.name, name)) {
                return a.value;
            }
        }
        return null;
    }

    pub fn hasAttribute(name: []const u8): bool {
        for (a of _.attrs) {
            if (std.ascii.eqlIgnoreCase(a.name, name)) {
                return true;
            }
        }
        return false;
    }
}

pub struct HtmlTokenizer {
    src: []const u8 = "",
    pos: usize = 0,
    raw_text_tag: []const u8 = "",

    pub static fn init(html: []const u8): HtmlTokenizer {
        return HtmlTokenizer{
            src: html,
            pos: 0,
            raw_text_tag: "",
        };
    }

    static fn isSpace(c: u8): bool {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
    }

    static fn isTagNameStart(c: u8): bool {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    static fn isTagNameChar(c: u8): bool {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == ':';
    }

    pub static fn isVoidElement(tag: []const u8): bool {
        if (std.ascii.eqlIgnoreCase(tag, "area")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "base")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "br")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "col")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "embed")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "hr")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "img")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "input")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "link")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "meta")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "param")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "source")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "track")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "wbr")) return true;
        return false;
    }

    pub static fn isRawTextElement(tag: []const u8): bool {
        if (std.ascii.eqlIgnoreCase(tag, "script")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "style")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "textarea")) return true;
        if (std.ascii.eqlIgnoreCase(tag, "title")) return true;
        return false;
    }

    fn isTokenStartAt(idx: usize): bool {
        if (idx + 1 >= _.src.len || _.src[idx] != '<') return false;
        const c1 = _.src[idx + 1];
        if (HtmlTokenizer.isTagNameStart(c1)) return true;
        if (c1 == '/' && idx + 2 < _.src.len && HtmlTokenizer.isTagNameStart(_.src[idx + 2])) return true;
        if (c1 == '!') return true;
        return false;
    }

    fn scanRawText(*_): HtmlToken? {
        const tag = _.raw_text_tag;
        const start = _.pos;
        while (_.pos < _.src.len) {
            if (_.src[_.pos] == '<' && _.pos + 2 + tag.len <= _.src.len && _.src[_.pos + 1] == '/') {
                if (std.ascii.eqlIgnoreCase(_.src[_.pos + 2 .. _.pos + 2 + tag.len], tag)) {
                    const after = _.pos + 2 + tag.len;
                    if (after < _.src.len && (_.src[after] == '>' || _.src[after] == '/' || HtmlTokenizer.isSpace(_.src[after]))) {
                        _.raw_text_tag = "";
                        if (_.pos > start) {
                            const slice = _.src[start.._.pos];
                            return HtmlToken{
                                kind: .Text,
                                raw: slice,
                                text: slice,
                            };
                        }
                        return null;
                    }
                }
            }
            _.pos += 1;
        }
        _.raw_text_tag = "";
        if (_.pos > start) {
            const slice = _.src[start.._.pos];
            return HtmlToken{
                kind: .Text,
                raw: slice,
                text: slice,
            };
        }
        return null;
    }

    /// Reads the next HTML5 token, allocating attribute slices via `allocator`.
    /// Returns `null` at end of input.
    pub fn next(*_, allocator: std.mem.Allocator): (HtmlToken?)! {
        while (_.pos < _.src.len) {
            if (_.raw_text_tag.len > 0) {
                if (raw_tok := _.scanRawText()) {
                    return raw_tok;
                }
                if (_.pos >= _.src.len) return null;
            }

            if (_.src[_.pos] == '<') {
                // 1. Comment: <!-- ... -->
                if (_.pos + 4 <= _.src.len && std.mem.eql(u8, _.src[_.pos .. _.pos + 4], "<!--")) {
                    const start = _.pos;
                    const body_start = _.pos + 4;
                    if (end_idx := std.mem.indexOf(u8, _.src[body_start..], "-->")) {
                        _.pos = body_start + end_idx + 3;
                        return HtmlToken{
                            kind: .Comment,
                            raw: _.src[start.._.pos],
                            text: _.src[body_start .. body_start + end_idx],
                        };
                    }
                    _.pos = _.src.len;
                    return HtmlToken{
                        kind: .Comment,
                        raw: _.src[start.._.pos],
                        text: _.src[body_start.._.pos],
                    };
                }

                // 2. CDATA: <![CDATA[ ... ]]>
                if (_.pos + 9 <= _.src.len && std.mem.eql(u8, _.src[_.pos .. _.pos + 9], "<![CDATA[")) {
                    const start = _.pos;
                    const body_start = _.pos + 9;
                    if (end_idx := std.mem.indexOf(u8, _.src[body_start..], "]]>")) {
                        _.pos = body_start + end_idx + 3;
                        return HtmlToken{
                            kind: .Cdata,
                            raw: _.src[start.._.pos],
                            text: _.src[body_start .. body_start + end_idx],
                        };
                    }
                    _.pos = _.src.len;
                    return HtmlToken{
                        kind: .Cdata,
                        raw: _.src[start.._.pos],
                        text: _.src[body_start.._.pos],
                    };
                }

                // 3. DOCTYPE: <!DOCTYPE ...>
                if (_.pos + 9 <= _.src.len && std.ascii.eqlIgnoreCase(_.src[_.pos .. _.pos + 9], "<!DOCTYPE")) {
                    const start = _.pos;
                    const body_start = _.pos + 9;
                    if (gt := std.mem.indexOfScalar(u8, _.src[body_start..], '>')) {
                        _.pos = body_start + gt + 1;
                        return HtmlToken{
                            kind: .Doctype,
                            raw: _.src[start.._.pos],
                            text: std.mem.trim(u8, _.src[body_start .. body_start + gt], " \t\r\n"),
                        };
                    }
                    _.pos = _.src.len;
                    return HtmlToken{
                        kind: .Doctype,
                        raw: _.src[start.._.pos],
                        text: std.mem.trim(u8, _.src[body_start.._.pos], " \t\r\n"),
                    };
                }

                // 4. EndTag: </tag ...>
                if (_.pos + 2 < _.src.len && _.src[_.pos + 1] == '/' && HtmlTokenizer.isTagNameStart(_.src[_.pos + 2])) {
                    const start = _.pos;
                    _.pos += 2;
                    const name_start = _.pos;
                    while (_.pos < _.src.len && HtmlTokenizer.isTagNameChar(_.src[_.pos])) {
                        _.pos += 1;
                    }
                    const tag_name = _.src[name_start.._.pos];
                    while (_.pos < _.src.len && _.src[_.pos] != '>') {
                        _.pos += 1;
                    }
                    if (_.pos < _.src.len && _.src[_.pos] == '>') {
                        _.pos += 1;
                    }
                    return HtmlToken{
                        kind: .EndTag,
                        tag_name: tag_name,
                        raw: _.src[start.._.pos],
                    };
                }

                // 5. StartTag / SelfClosingTag: <tag ...>
                if (_.pos + 1 < _.src.len && HtmlTokenizer.isTagNameStart(_.src[_.pos + 1])) {
                    const start = _.pos;
                    _.pos += 1;
                    const name_start = _.pos;
                    while (_.pos < _.src.len && HtmlTokenizer.isTagNameChar(_.src[_.pos])) {
                        _.pos += 1;
                    }
                    const tag_name = _.src[name_start.._.pos];

                    let attr_list: std.ArrayList(HtmlAttribute) = .empty;
                    errdefer attr_list.deinit(allocator);
                    let self_closing = false;

                    while (_.pos < _.src.len) {
                        while (_.pos < _.src.len && HtmlTokenizer.isSpace(_.src[_.pos])) {
                            _.pos += 1;
                        }
                        if (_.pos >= _.src.len) break;
                        if (_.src[_.pos] == '>') {
                            _.pos += 1;
                            break;
                        }
                        if (_.src[_.pos] == '/') {
                            if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '>') {
                                self_closing = true;
                                _.pos += 2;
                                break;
                            }
                            _.pos += 1;
                            continue;
                        }

                        // Attribute name
                        const aname_start = _.pos;
                        while (_.pos < _.src.len && !HtmlTokenizer.isSpace(_.src[_.pos]) && _.src[_.pos] != '=' && _.src[_.pos] != '>' && _.src[_.pos] != '/') {
                            _.pos += 1;
                        }
                        const aname = _.src[aname_start.._.pos];
                        if (aname.len == 0) {
                            _.pos += 1;
                            continue;
                        }

                        while (_.pos < _.src.len && HtmlTokenizer.isSpace(_.src[_.pos])) {
                            _.pos += 1;
                        }

                        let aval: []const u8 = "";
                        let has_val = false;

                        if (_.pos < _.src.len && _.src[_.pos] == '=') {
                            has_val = true;
                            _.pos += 1;
                            while (_.pos < _.src.len && HtmlTokenizer.isSpace(_.src[_.pos])) {
                                _.pos += 1;
                            }
                            if (_.pos < _.src.len && (_.src[_.pos] == '"' || _.src[_.pos] == '\'')) {
                                const q = _.src[_.pos];
                                _.pos += 1;
                                const v_start = _.pos;
                                while (_.pos < _.src.len && _.src[_.pos] != q) {
                                    _.pos += 1;
                                }
                                aval = _.src[v_start.._.pos];
                                if (_.pos < _.src.len && _.src[_.pos] == q) {
                                    _.pos += 1;
                                }
                            } else {
                                const v_start = _.pos;
                                while (_.pos < _.src.len && !HtmlTokenizer.isSpace(_.src[_.pos]) && _.src[_.pos] != '>') {
                                    _.pos += 1;
                                }
                                aval = _.src[v_start.._.pos];
                            }
                        }

                        attr_list.append(allocator, HtmlAttribute{
                            name: aname,
                            value: aval,
                            has_value: has_val,
                        })?;
                    }

                    const is_void = HtmlTokenizer.isVoidElement(tag_name);
                    if (!self_closing && !is_void && HtmlTokenizer.isRawTextElement(tag_name)) {
                        _.raw_text_tag = tag_name;
                    }

                    return HtmlToken{
                        kind: if (self_closing) HtmlTokenKind.SelfClosingTag else HtmlTokenKind.StartTag,
                        tag_name: tag_name,
                        raw: _.src[start.._.pos],
                        attrs: attr_list.toOwnedSlice(allocator)?,
                        self_closing: self_closing || is_void,
                    };
                }
            }

            // 6. Text node: consume until next valid tag/comment/doctype/cdata start
            const text_start = _.pos;
            _.pos += 1;
            while (_.pos < _.src.len && !_.isTokenStartAt(_.pos)) {
                _.pos += 1;
            }
            const text_slice = _.src[text_start.._.pos];
            return HtmlToken{
                kind: .Text,
                raw: text_slice,
                text: text_slice,
            };
        }
        return null;
    }

    pub static fn tokenizeAll(allocator: std.mem.Allocator, html: []const u8): ([]HtmlToken)! {
        let tok = HtmlTokenizer.init(html);
        let list: std.ArrayList(HtmlToken) = .empty;
        errdefer list.deinit(allocator);
        while (t := tok.next(allocator)?) {
            list.append(allocator, t)?;
        }
        return list.toOwnedSlice(allocator);
    }
}
