// Source: cp/lib/xml/parser.cp (XML 1.0 DOM Parser, Entity Decoder & Path Query)
// Pure CP XML 1.0 recursive-descent DOM parser with Arena-backed allocation,
// 5 predefined & numeric (decimal/hex) entity decoding, CDATA/comments/PIs,
// namespace-prefixed tags, max-depth DoS guard (128), and path/DOM queries.

const std = @import("std");
import { Utf8 } from "cap:string";

pub err XmlError {
    UnexpectedEof,
    MismatchedTag,
    MalformedTag,
    MaxDepthExceeded,
    InvalidEntity,
    OutOfMemory,
}

pub struct XmlAttr {
    name: []const u8 = "",
    value: []const u8 = "",
}

pub enum XmlNodeKind {
    Element,
    Text,
    CData,
}

pub struct XmlNode {
    kind: XmlNodeKind = .Element,
    tag: []const u8 = "",
    text: []const u8 = "",
    attrs: []XmlAttr = [],
    children: []XmlNode = [],

    /// Returns the decoded value of the attribute matching `name`, or `null` if absent.
    pub fn getAttr(name: []const u8): ([]const u8)? {
        if (_.kind != .Element) {
            return null;
        }
        for (a of _.attrs) {
            if (std.mem.eql(u8, a.name, name)) {
                return a.value;
            }
        }
        return null;
    }

    /// Returns the first direct child element whose tag name matches `tag`, or `null`.
    pub fn findChild(tag: []const u8): XmlNode? {
        if (_.kind != .Element) {
            return null;
        }
        for (c of _.children) {
            if (c.kind == .Element && std.mem.eql(u8, c.tag, tag)) {
                return c;
            }
        }
        return null;
    }

    /// Returns all direct child elements whose tag name matches `tag`.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn findAllChildren(alloc: std.mem.Allocator, tag: []const u8): ([]XmlNode)! {
        let out: std.ArrayList(XmlNode) = .empty;
        errdefer out.deinit(alloc);

        if (_.kind == .Element) {
            for (c of _.children) {
                if (c.kind == .Element && std.mem.eql(u8, c.tag, tag)) {
                    out.append(alloc, c)?;
                }
            }
        }
        return out.toOwnedSlice(alloc);
    }

    /// Traverses a slash-separated relative element path such as `"channel/item/title"`.
    pub fn findByPath(path: []const u8): XmlNode? {
        let cur = XmlNode{
            kind: _.kind,
            tag: _.tag,
            text: _.text,
            attrs: _.attrs,
            children: _.children,
        };
        if (path.len == 0 || (path.len == 1 && path[0] == '/')) {
            return cur;
        }

        let pos: usize = 0;
        if (path[0] == '/') {
            pos = 1;
        }

        while (pos < path.len) {
            let end = pos;
            while (end < path.len && path[end] != '/') {
                end += 1;
            }
            const seg = path[pos..end];
            if (seg.len == 0) {
                return null;
            }
            if (next_node := cur.findChild(seg)) {
                cur = next_node;
            } else {
                return null;
            }
            pos = end + 1;
        }
        return cur;
    }

    static fn appendInnerText(out: *std.ArrayList(u8), alloc: std.mem.Allocator, node: XmlNode): ! {
        switch (node.kind) {
            .Text | .CData => {
                if (node.text.len > 0) {
                    out.appendSlice(alloc, node.text)?;
                }
            },
            .Element => {
                for (c of node.children) {
                    XmlNode.appendInnerText(out, alloc, c)?;
                }
            },
        }
    }

    /// Concatenates all Text and CData descendant contents in document order.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn innerText(alloc: std.mem.Allocator): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        const self_copy = XmlNode{
            kind: _.kind,
            tag: _.tag,
            text: _.text,
            attrs: _.attrs,
            children: _.children,
        };
        XmlNode.appendInnerText(&out, alloc, self_copy)?;
        return out.toOwnedSlice(alloc);
    }
}

pub struct XmlDocument {
    version: []const u8 = "1.0",
    encoding: []const u8 = "",
    root: XmlNode = XmlNode{},
    owned_arena: (*std.heap.ArenaAllocator)? = null,
    parent_alloc: std.mem.Allocator? = null,

    /// Releases all arena-allocated nodes, attributes, and decoded strings in O(1).
    pub fn deinit(*_) {
        if (owned := _.owned_arena) {
            owned.deinit();
            if (p := _.parent_alloc) {
                p.destroy(owned);
            }
            _.owned_arena = null;
            _.parent_alloc = null;
        }
        _.root = XmlNode{};
    }
}

pub struct XmlParser {
    src: []const u8 = "",
    pos: usize = 0,
    doc_alloc: std.mem.Allocator = undefined,
    version: []const u8 = "1.0",
    encoding: []const u8 = "",

    pub static const MAX_DEPTH: usize = 128;

    /// Parses an XML 1.0 document into an arena-backed `XmlDocument`.
    /// Call `doc.deinit()` when finished to release all document memory.
    pub static fn parse(alloc: std.mem.Allocator, xml_str: []const u8): XmlDocument! {
        let doc_arena = alloc.create(std.heap.ArenaAllocator)?;
        unsafe {
            doc_arena.* = std.heap.ArenaAllocator.init(alloc);
        }
        errdefer {
            doc_arena.deinit();
            alloc.destroy(doc_arena);
        }

        let input = xml_str;
        if (input.len >= 3 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            input = input[3..];
        }

        let p = XmlParser{
            src: input,
            pos: 0,
            doc_alloc: doc_arena.allocator(),
            version: "1.0",
            encoding: "",
        };

        const root_node = p.parseDocument()?;
        return XmlDocument{
            version: p.version,
            encoding: p.encoding,
            root: root_node,
            owned_arena: doc_arena,
            parent_alloc: alloc,
        };
    }

    static fn isWhitespace(c: u8): bool {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    static fn isAllWhitespace(s: []const u8): bool {
        for (c of s) {
            if (!XmlParser.isWhitespace(c)) {
                return false;
            }
        }
        return true;
    }

    static fn isNameStartChar(c: u8): bool {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == ':') {
            return true;
        }
        return c >= 0x80;
    }

    static fn isNameChar(c: u8): bool {
        if (XmlParser.isNameStartChar(c)) {
            return true;
        }
        return (c >= '0' && c <= '9') || c == '-' || c == '.';
    }

    static fn isValidXmlCodepoint(cp: u64): bool {
        if (cp == 0x09 || cp == 0x0A || cp == 0x0D) {
            return true;
        }
        if (cp >= 0x20 && cp <= 0xD7FF) {
            return true;
        }
        if (cp >= 0xE000 && cp <= 0xFFFD) {
            return true;
        }
        if (cp >= 0x10000 && cp <= 0x10FFFF) {
            return true;
        }
        return false;
    }

    static fn decodeNumericEntity(body: []const u8): u32! {
        if (body.len == 0) {
            return err.InvalidEntity;
        }
        let cp: u64 = 0;
        if (body[0] == 'x' || body[0] == 'X') {
            if (body.len == 1) {
                return err.InvalidEntity;
            }
            let i: usize = 1;
            while (i < body.len) : (i += 1) {
                const c = body[i];
                let digit: u64 = 0;
                if (c >= '0' && c <= '9') {
                    digit = (c - '0') as u64;
                } else if (c >= 'a' && c <= 'f') {
                    digit = (c - 'a' + 10) as u64;
                } else if (c >= 'A' && c <= 'F') {
                    digit = (c - 'A' + 10) as u64;
                } else {
                    return err.InvalidEntity;
                }
                cp = (cp << 4) | digit;
                if (cp > 0x10FFFF) {
                    return err.InvalidEntity;
                }
            }
        } else {
            for (c of body) {
                if (c < '0' || c > '9') {
                    return err.InvalidEntity;
                }
                cp = cp * 10 + ((c - '0') as u64);
                if (cp > 0x10FFFF) {
                    return err.InvalidEntity;
                }
            }
        }

        if (!XmlParser.isValidXmlCodepoint(cp)) {
            return err.InvalidEntity;
        }
        return cp as u32;
    }

    /// Decodes XML 1.0 predefined entities and decimal/hex numeric references.
    /// Rejects unknown entities, invalid codepoints, or unterminated `&` sequences.
    pub static fn decodeEntities(alloc: std.mem.Allocator, raw: []const u8): ([]const u8)! {
        if (std.mem.indexOfScalar(u8, raw, '&') == null) {
            return alloc.dupe(u8, raw);
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        out.ensureTotalCapacity(alloc, raw.len)?;

        let i: usize = 0;
        while (i < raw.len) {
            if (raw[i] == '&') {
                let semi = i + 1;
                let found_semi = false;
                while (semi < raw.len) : (semi += 1) {
                    const ch = raw[semi];
                    if (ch == ';') {
                        found_semi = true;
                        break;
                    }
                    if (ch == '&' || ch == '<' || XmlParser.isWhitespace(ch) || semi - i > 32) {
                        break;
                    }
                }
                if (!found_semi) {
                    return err.InvalidEntity;
                }

                const body = raw[i + 1 .. semi];
                if (body.len == 0) {
                    return err.InvalidEntity;
                }

                if (body[0] == '#') {
                    const cp = XmlParser.decodeNumericEntity(body[1..])?;
                    let utf8_buf: [4]u8 = undefined;
                    const n = Utf8.encodeCodepoint(cp, &utf8_buf);
                    if (n == 0) {
                        return err.InvalidEntity;
                    }
                    out.appendSlice(alloc, utf8_buf[0..n])?;
                } else if (std.mem.eql(u8, body, "amp")) {
                    out.append(alloc, '&')?;
                } else if (std.mem.eql(u8, body, "lt")) {
                    out.append(alloc, '<')?;
                } else if (std.mem.eql(u8, body, "gt")) {
                    out.append(alloc, '>')?;
                } else if (std.mem.eql(u8, body, "quot")) {
                    out.append(alloc, '"')?;
                } else if (std.mem.eql(u8, body, "apos")) {
                    out.append(alloc, '\'')?;
                } else {
                    return err.InvalidEntity;
                }
                i = semi + 1;
            } else {
                const start = i;
                while (i < raw.len && raw[i] != '&') {
                    i += 1;
                }
                out.appendSlice(alloc, raw[start..i])?;
            }
        }
        return out.toOwnedSlice(alloc);
    }

    fn skipWhitespace(*_) {
        while (_.pos < _.src.len && XmlParser.isWhitespace(_.src[_.pos])) {
            _.pos += 1;
        }
    }

    fn matchPrefix(prefix: []const u8): bool {
        if (_.pos + prefix.len > _.src.len) {
            return false;
        }
        return std.mem.eql(u8, _.src[_.pos .. _.pos + prefix.len], prefix);
    }

    fn parseName(*_): ([]const u8)! {
        if (_.pos >= _.src.len) {
            return err.UnexpectedEof;
        }
        if (!XmlParser.isNameStartChar(_.src[_.pos])) {
            return err.MalformedTag;
        }
        const start = _.pos;
        _.pos += 1;
        while (_.pos < _.src.len && XmlParser.isNameChar(_.src[_.pos])) {
            _.pos += 1;
        }
        return _.doc_alloc.dupe(u8, _.src[start.._.pos]);
    }

    fn skipComment(*_): ! {
        // Assumes positioned at "<!--"
        _.pos += 4;
        while (_.pos + 2 < _.src.len) {
            if (_.src[_.pos] == '-' && _.src[_.pos + 1] == '-' && _.src[_.pos + 2] == '>') {
                _.pos += 3;
                return;
            }
            _.pos += 1;
        }
        return err.UnexpectedEof;
    }

    fn parseProcessingInstruction(*_): ! {
        // Assumes positioned at "<?"
        _.pos += 2;
        const target_start = _.pos;
        if (_.pos >= _.src.len) {
            return err.UnexpectedEof;
        }
        if (!XmlParser.isNameStartChar(_.src[_.pos])) {
            return err.MalformedTag;
        }
        _.pos += 1;
        while (_.pos < _.src.len && XmlParser.isNameChar(_.src[_.pos])) {
            _.pos += 1;
        }
        const target = _.src[target_start.._.pos];
        const is_xml_decl = std.mem.eql(u8, target, "xml");

        if (_.pos >= _.src.len) {
            return err.UnexpectedEof;
        }
        if (!_.matchPrefix("?>") && !XmlParser.isWhitespace(_.src[_.pos])) {
            return err.MalformedTag;
        }

        const body_start = _.pos;
        while (_.pos + 1 < _.src.len) {
            if (_.src[_.pos] == '?' && _.src[_.pos + 1] == '>') {
                const body = _.src[body_start.._.pos];
                _.pos += 2;
                if (is_xml_decl) {
                    _.extractXmlDeclMetadata(body)?;
                }
                return;
            }
            _.pos += 1;
        }
        return err.UnexpectedEof;
    }

    fn extractXmlDeclMetadata(*_, body: []const u8): ! {
        let i: usize = 0;
        while (i < body.len) {
            while (i < body.len && XmlParser.isWhitespace(body[i])) {
                i += 1;
            }
            if (i >= body.len) {
                break;
            }
            if (!XmlParser.isNameStartChar(body[i])) {
                return err.MalformedTag;
            }
            const k_start = i;
            i += 1;
            while (i < body.len && XmlParser.isNameChar(body[i])) {
                i += 1;
            }
            const key = body[k_start..i];
            while (i < body.len && XmlParser.isWhitespace(body[i])) {
                i += 1;
            }
            if (i >= body.len || body[i] != '=') {
                return err.MalformedTag;
            }
            i += 1;
            while (i < body.len && XmlParser.isWhitespace(body[i])) {
                i += 1;
            }
            if (i >= body.len || (body[i] != '"' && body[i] != '\'')) {
                return err.MalformedTag;
            }
            const quote = body[i];
            i += 1;
            const v_start = i;
            while (i < body.len && body[i] != quote) {
                i += 1;
            }
            if (i >= body.len) {
                return err.MalformedTag;
            }
            const val = body[v_start..i];
            i += 1;
            if (std.mem.eql(u8, key, "version")) {
                _.version = _.doc_alloc.dupe(u8, val)?;
            } else if (std.mem.eql(u8, key, "encoding")) {
                _.encoding = _.doc_alloc.dupe(u8, val)?;
            }
        }
    }

    fn skipDoctype(*_): ! {
        // Assumes positioned at "<!DOCTYPE"
        _.pos += 9;
        let bracket_depth: usize = 0;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '"' || c == '\'') {
                const q = c;
                _.pos += 1;
                while (_.pos < _.src.len && _.src[_.pos] != q) {
                    _.pos += 1;
                }
                if (_.pos >= _.src.len) {
                    return err.UnexpectedEof;
                }
                _.pos += 1;
            } else if (c == '[') {
                bracket_depth += 1;
                _.pos += 1;
            } else if (c == ']') {
                if (bracket_depth > 0) {
                    bracket_depth -= 1;
                }
                _.pos += 1;
            } else if (c == '>' && bracket_depth == 0) {
                _.pos += 1;
                return;
            } else {
                _.pos += 1;
            }
        }
        return err.UnexpectedEof;
    }

    fn parseCData(*_): ([]const u8)! {
        // Assumes positioned at "<![CDATA["
        _.pos += 9;
        const start = _.pos;
        while (_.pos + 2 < _.src.len) {
            if (_.src[_.pos] == ']' && _.src[_.pos + 1] == ']' && _.src[_.pos + 2] == '>') {
                const raw = _.src[start.._.pos];
                _.pos += 3;
                return _.doc_alloc.dupe(u8, raw);
            }
            _.pos += 1;
        }
        return err.UnexpectedEof;
    }

    fn skipPrologAndMisc(*_): ! {
        while (_.pos < _.src.len) {
            _.skipWhitespace();
            if (_.matchPrefix("<!--")) {
                _.skipComment()?;
            } else if (_.matchPrefix("<?")) {
                _.parseProcessingInstruction()?;
            } else if (_.matchPrefix("<!DOCTYPE")) {
                _.skipDoctype()?;
            } else {
                break;
            }
        }
    }

    fn parseDocument(*_): XmlNode! {
        _.skipPrologAndMisc()?;
        if (_.pos >= _.src.len) {
            return err.UnexpectedEof;
        }
        if (_.src[_.pos] != '<') {
            return err.MalformedTag;
        }
        const root_node = _.parseElement(1)?;

        // Only whitespace, comments, and PIs are permitted after root closes
        while (_.pos < _.src.len) {
            _.skipWhitespace();
            if (_.pos >= _.src.len) {
                break;
            }
            if (_.matchPrefix("<!--")) {
                _.skipComment()?;
            } else if (_.matchPrefix("<?")) {
                _.parseProcessingInstruction()?;
            } else {
                return err.MalformedTag;
            }
        }
        return root_node;
    }

    fn parseElement(*_, depth: usize): XmlNode! {
        if (depth > XmlParser.MAX_DEPTH) {
            return err.MaxDepthExceeded;
        }
        if (_.pos >= _.src.len) {
            return err.UnexpectedEof;
        }
        if (_.src[_.pos] != '<') {
            return err.MalformedTag;
        }
        _.pos += 1;

        if (_.pos >= _.src.len) {
            return err.UnexpectedEof;
        }
        if (_.src[_.pos] == '/' || _.src[_.pos] == '!' || _.src[_.pos] == '?') {
            return err.MalformedTag;
        }

        const tag_name = _.parseName()?;

        let attrs: std.ArrayList(XmlAttr) = .empty;
        let self_closing = false;

        while (true) {
            const had_ws = _.pos < _.src.len && XmlParser.isWhitespace(_.src[_.pos]);
            _.skipWhitespace();
            if (_.pos >= _.src.len) {
                return err.UnexpectedEof;
            }
            if (_.matchPrefix("/>")) {
                _.pos += 2;
                self_closing = true;
                break;
            }
            if (_.src[_.pos] == '>') {
                _.pos += 1;
                break;
            }
            if (!had_ws) {
                return err.MalformedTag;
            }

            const attr_name = _.parseName()?;
            _.skipWhitespace();
            if (_.pos >= _.src.len) {
                return err.UnexpectedEof;
            }
            if (_.src[_.pos] != '=') {
                return err.MalformedTag;
            }
            _.pos += 1;
            _.skipWhitespace();
            if (_.pos >= _.src.len) {
                return err.UnexpectedEof;
            }
            const quote = _.src[_.pos];
            if (quote != '"' && quote != '\'') {
                return err.MalformedTag;
            }
            _.pos += 1;

            const val_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != quote) {
                if (_.src[_.pos] == '<') {
                    return err.MalformedTag;
                }
                _.pos += 1;
            }
            if (_.pos >= _.src.len) {
                return err.UnexpectedEof;
            }
            const raw_val = _.src[val_start.._.pos];
            _.pos += 1;

            const decoded_val = XmlParser.decodeEntities(_.doc_alloc, raw_val)?;
            attrs.append(_.doc_alloc, XmlAttr{
                name: attr_name,
                value: decoded_val,
            })?;
        }

        const attr_slice = attrs.toOwnedSlice(_.doc_alloc)?;
        if (self_closing) {
            return XmlNode{
                kind: .Element,
                tag: tag_name,
                text: "",
                attrs: attr_slice,
                children: [],
            };
        }

        let raw_children: std.ArrayList(XmlNode) = .empty;
        let has_element_child = false;

        while (true) {
            if (_.pos >= _.src.len) {
                return err.UnexpectedEof;
            }
            if (_.src[_.pos] == '<') {
                if (_.matchPrefix("</")) {
                    _.pos += 2;
                    const close_name = _.parseName()?;
                    _.skipWhitespace();
                    if (_.pos >= _.src.len) {
                        return err.UnexpectedEof;
                    }
                    if (_.src[_.pos] != '>') {
                        return err.MalformedTag;
                    }
                    _.pos += 1;
                    if (!std.mem.eql(u8, tag_name, close_name)) {
                        return err.MismatchedTag;
                    }
                    break;
                } else if (_.matchPrefix("<!--")) {
                    _.skipComment()?;
                } else if (_.matchPrefix("<![CDATA[")) {
                    const cdata_txt = _.parseCData()?;
                    raw_children.append(_.doc_alloc, XmlNode{
                        kind: .CData,
                        tag: "",
                        text: cdata_txt,
                        attrs: [],
                        children: [],
                    })?;
                } else if (_.matchPrefix("<?")) {
                    _.parseProcessingInstruction()?;
                } else if (_.matchPrefix("<!")) {
                    return err.MalformedTag;
                } else {
                    const child_elem = _.parseElement(depth + 1)?;
                    has_element_child = true;
                    raw_children.append(_.doc_alloc, child_elem)?;
                }
            } else {
                const txt_start = _.pos;
                while (_.pos < _.src.len && _.src[_.pos] != '<') {
                    _.pos += 1;
                }
                const raw_txt = _.src[txt_start.._.pos];
                const decoded_txt = XmlParser.decodeEntities(_.doc_alloc, raw_txt)?;
                if (decoded_txt.len > 0) {
                    raw_children.append(_.doc_alloc, XmlNode{
                        kind: .Text,
                        tag: "",
                        text: decoded_txt,
                        attrs: [],
                        children: [],
                    })?;
                }
            }
        }

        let final_children: []XmlNode = [];
        if (has_element_child) {
            let filtered: std.ArrayList(XmlNode) = .empty;
            for (c of raw_children.items) {
                if (c.kind == .Text && XmlParser.isAllWhitespace(c.text)) {
                    continue;
                }
                filtered.append(_.doc_alloc, c)?;
            }
            final_children = filtered.toOwnedSlice(_.doc_alloc)?;
        } else {
            final_children = raw_children.toOwnedSlice(_.doc_alloc)?;
        }

        return XmlNode{
            kind: .Element,
            tag: tag_name,
            text: "",
            attrs: attr_slice,
            children: final_children,
        };
    }
}
