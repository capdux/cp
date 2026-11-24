// Reference-hardened against RazrFalcon/roxmltree (MIT/Apache-2.0)
// Source: cp/lib/xml/parser.cp (XML 1.0 DOM Parser, Strict Zero-Alloc Validator, 16-Lane SIMD Scanner & Path Query)
// Pure CP W3C XML 1.0 (5th Edition) recursive-descent DOM parser & zero-alloc validator with Arena-backed allocation,
// 5 predefined & numeric (decimal/hex) entity decoding, CDATA/comments/PIs, namespace-prefixed tags,
// duplicate-attribute/control-char/bare-]]>/comment-`--` enforcement, depth/attr/node/entity bounds,
// and 16-lane @Vector(16, u8) SIMD hot-path scanning.

const std = @import("std");
import { Utf8 } from "cap:string";

type XmlVec16u8 = @Vector(16, u8);

pub const XML_MAX_DEPTH: usize = 128;
pub const XML_MAX_ATTRS_PER_ELEMENT: usize = 256;
pub const XML_MAX_NODES: usize = 65536;
pub const XML_MAX_ENTITY_EXPANSIONS: usize = 65536;

pub err XmlError {
    UnexpectedEof,
    MismatchedTag,
    MalformedTag,
    MaxDepthExceeded,
    InvalidEntity,
    DuplicateAttribute,
    InvalidControlChar,
    MaxNodesExceeded,
    MaxAttrsExceeded,
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

// ============================================================================
// 16-Lane @Vector(16, u8) SIMD Scanners + Scalar Differential References
// ============================================================================

fn xmlLoadVec16(data: []const u8, off: usize): XmlVec16u8 {
    const v: XmlVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` SIMD scanner locating the next XML text delimiter (`<`, `&`, or `]`)
/// at or after `start`.
pub fn simdFindXmlTextDelim(data: []const u8, start: usize): usize? {
    if (start >= data.len) return null;
    const lt_v: XmlVec16u8 = @splat('<');
    const amp_v: XmlVec16u8 = @splat('&');
    const rbr_v: XmlVec16u8 = @splat(']');
    let i: usize = start;
    while (i + 16 <= data.len) {
        const v = xmlLoadVec16(data, i);
        if (@reduce(.Or, v == lt_v) || @reduce(.Or, v == amp_v) || @reduce(.Or, v == rbr_v)) {
            let j: usize = 0;
            while (j < 16) : (j += 1) {
                const b = data[i + j];
                if (b == '<' || b == '&' || b == ']') {
                    return i + j;
                }
            }
        }
        i += 16;
    }
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b == '<' || b == '&' || b == ']') {
            return i;
        }
    }
    return null;
}

/// Scalar reference for `simdFindXmlTextDelim` (differential verification).
pub fn scalarFindXmlTextDelim(data: []const u8, start: usize): usize? {
    if (start >= data.len) return null;
    let i: usize = start;
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b == '<' || b == '&' || b == ']') {
            return i;
        }
    }
    return null;
}

/// 16-lane `@Vector(16, u8)` SIMD scanner detecting forbidden W3C XML 1.0 §2.2 ASCII control bytes
/// (`0x00..0x08`, `0x0B`, `0x0C`, `0x0E..0x1F`).
pub fn simdHasInvalidXmlControl(data: []const u8): bool {
    const sp_v: XmlVec16u8 = @splat(0x20);
    let i: usize = 0;
    while (i + 16 <= data.len) {
        const v = xmlLoadVec16(data, i);
        if (@reduce(.Or, v < sp_v)) {
            let j: usize = 0;
            while (j < 16) : (j += 1) {
                const b = data[i + j];
                if (b < 0x20 && b != 0x09 && b != 0x0A && b != 0x0D) {
                    return true;
                }
            }
        }
        i += 16;
    }
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b < 0x20 && b != 0x09 && b != 0x0A && b != 0x0D) {
            return true;
        }
    }
    return false;
}

/// Scalar reference for `simdHasInvalidXmlControl` (differential verification).
pub fn scalarHasInvalidXmlControl(data: []const u8): bool {
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b < 0x20 && b != 0x09 && b != 0x0A && b != 0x0D) {
            return true;
        }
    }
    return false;
}

/// 16-lane `@Vector(16, u8)` SIMD scanner locating the next attribute closing `quote`, `&`, or `<`
/// at or after `start`.
pub fn simdFindAttrQuoteOrAmp(data: []const u8, start: usize, quote: u8): usize? {
    if (start >= data.len) return null;
    const q_v: XmlVec16u8 = @splat(quote);
    const amp_v: XmlVec16u8 = @splat('&');
    const lt_v: XmlVec16u8 = @splat('<');
    let i: usize = start;
    while (i + 16 <= data.len) {
        const v = xmlLoadVec16(data, i);
        if (@reduce(.Or, v == q_v) || @reduce(.Or, v == amp_v) || @reduce(.Or, v == lt_v)) {
            let j: usize = 0;
            while (j < 16) : (j += 1) {
                const b = data[i + j];
                if (b == quote || b == '&' || b == '<') {
                    return i + j;
                }
            }
        }
        i += 16;
    }
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b == quote || b == '&' || b == '<') {
            return i;
        }
    }
    return null;
}

/// Scalar reference for `simdFindAttrQuoteOrAmp` (differential verification).
pub fn scalarFindAttrQuoteOrAmp(data: []const u8, start: usize, quote: u8): usize? {
    if (start >= data.len) return null;
    let i: usize = start;
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b == quote || b == '&' || b == '<') {
            return i;
        }
    }
    return null;
}

fn isXmlTargetCaseInsensitive(s: []const u8): bool {
    if (s.len != 3) return false;
    const c0 = if (s[0] >= 'A' && s[0] <= 'Z') s[0] + 32 else s[0];
    const c1 = if (s[1] >= 'A' && s[1] <= 'Z') s[1] + 32 else s[1];
    const c2 = if (s[2] >= 'A' && s[2] <= 'Z') s[2] + 32 else s[2];
    return c0 == 'x' && c1 == 'm' && c2 == 'l';
}

fn validateEntitySlice(raw: []const u8, expansions: *usize): XmlError!void {
    let i: usize = 0;
    while (i < raw.len) {
        if (raw[i] != '&') {
            i += 1;
            continue;
        }
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
        if (expansions.* >= XML_MAX_ENTITY_EXPANSIONS) {
            return err.InvalidEntity;
        }
        expansions.* += 1;

        if (body[0] == '#') {
            * = XmlParser.decodeNumericEntity(body[1..])?;
        } else if (
            !std.mem.eql(u8, body, "amp") &&
            !std.mem.eql(u8, body, "lt") &&
            !std.mem.eql(u8, body, "gt") &&
            !std.mem.eql(u8, body, "quot") &&
            !std.mem.eql(u8, body, "apos")
        ) {
            return err.InvalidEntity;
        }
        i = semi + 1;
    }
}

pub struct XmlParser {
    src: []const u8 = "",
    pos: usize = 0,
    doc_alloc: std.mem.Allocator = undefined,
    version: []const u8 = "1.0",
    encoding: []const u8 = "",
    node_count: usize = 0,
    entity_expansions: usize = 0,

    pub static const MAX_DEPTH: usize = XML_MAX_DEPTH;

    /// Parses an XML 1.0 document into an arena-backed `XmlDocument`.
    /// Enforces W3C XML 1.0 control-byte, duplicate-attribute, comment `--`, bare `]]>`,
    /// entity/numeric reference, and depth/attr/node limits.
    /// Call `doc.deinit()` when finished to release all document memory.
    pub static fn parse(alloc: std.mem.Allocator, xml_str: []const u8): XmlDocument! {
        let input = xml_str;
        if (input.len >= 3 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            input = input[3..];
        }
        if (simdHasInvalidXmlControl(input)) {
            return err.InvalidControlChar;
        }

        let doc_arena = alloc.create(std.heap.ArenaAllocator)?;
        unsafe {
            doc_arena.* = std.heap.ArenaAllocator.init(alloc);
        }
        errdefer {
            doc_arena.deinit();
            alloc.destroy(doc_arena);
        }

        let p = XmlParser{
            src: input,
            pos: 0,
            doc_alloc: doc_arena.allocator(),
            version: "1.0",
            encoding: "",
            node_count: 0,
            entity_expansions: 0,
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

    static fn decodeNumericEntity(body: []const u8): XmlError!u32 {
        if (body.len == 0 || body.len > 16) {
            return err.InvalidEntity;
        }
        let cp: u64 = 0;
        // W3C XML 1.0 §4.1: CharRef ::= '&#' [0-9]+ ';' | '&#x' [0-9a-fA-F]+ ';'
        if (body[0] == 'x') {
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

    static fn decodeEntitiesTracked(
        alloc: std.mem.Allocator,
        raw: []const u8,
        expansions: *usize,
    ): ([]const u8)! {
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
                if (expansions.* >= XML_MAX_ENTITY_EXPANSIONS) {
                    return err.InvalidEntity;
                }
                expansions.* += 1;

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

    /// Decodes XML 1.0 predefined entities and decimal/hex numeric references.
    /// Rejects unknown entities, invalid codepoints, or unterminated `&` sequences.
    pub static fn decodeEntities(alloc: std.mem.Allocator, raw: []const u8): ([]const u8)! {
        let expansions: usize = 0;
        return XmlParser.decodeEntitiesTracked(alloc, raw, *expansions);
    }

    fn bumpNodeCount(*_): XmlError!void {
        if (_.node_count >= XML_MAX_NODES) {
            return err.MaxNodesExceeded;
        }
        _.node_count += 1;
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

    fn skipComment(*_): XmlError!void {
        // Assumes positioned at "<!--"; W3C XML 1.0 §2.5 forbids "--" anywhere inside a comment body
        _.pos += 4;
        while (_.pos + 1 < _.src.len) {
            if (_.src[_.pos] == '-' && _.src[_.pos + 1] == '-') {
                if (_.pos + 2 < _.src.len && _.src[_.pos + 2] == '>') {
                    _.pos += 3;
                    return;
                }
                return err.MalformedTag;
            }
            _.pos += 1;
        }
        return err.UnexpectedEof;
    }

    fn parseProcessingInstruction(*_, allow_xml_decl: bool): ! {
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
        const is_xml_ci = isXmlTargetCaseInsensitive(target);
        const is_xml_decl = std.mem.eql(u8, target, "xml");
        if (is_xml_ci && (!allow_xml_decl || !is_xml_decl)) {
            return err.MalformedTag;
        }

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
        let seen_version = false;
        let seen_encoding = false;
        let seen_standalone = false;
        while (i < body.len) {
            const had_ws = i < body.len && XmlParser.isWhitespace(body[i]);
            while (i < body.len && XmlParser.isWhitespace(body[i])) {
                i += 1;
            }
            if (i >= body.len) {
                break;
            }
            if (!had_ws) {
                return err.MalformedTag;
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
                if (seen_version || val.len == 0) {
                    return err.MalformedTag;
                }
                seen_version = true;
                _.version = _.doc_alloc.dupe(u8, val)?;
            } else if (std.mem.eql(u8, key, "encoding")) {
                if (seen_encoding || !seen_version || val.len == 0) {
                    return err.MalformedTag;
                }
                seen_encoding = true;
                _.encoding = _.doc_alloc.dupe(u8, val)?;
            } else if (std.mem.eql(u8, key, "standalone")) {
                if (seen_standalone || !seen_version) {
                    return err.MalformedTag;
                }
                if (!std.mem.eql(u8, val, "yes") && !std.mem.eql(u8, val, "no")) {
                    return err.MalformedTag;
                }
                seen_standalone = true;
            } else {
                return err.MalformedTag;
            }
        }
        if (!seen_version) {
            return err.MalformedTag;
        }
    }

    fn skipDoctype(*_): XmlError!void {
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
        // Optional XML declaration may only appear at byte offset 0 (immediately after optional BOM)
        if (_.pos == 0 && _.matchPrefix("<?xml")) {
            _.parseProcessingInstruction(true)?;
        }
        let seen_doctype = false;
        while (_.pos < _.src.len) {
            _.skipWhitespace();
            if (_.matchPrefix("<!--")) {
                _.skipComment()?;
            } else if (_.matchPrefix("<?")) {
                _.parseProcessingInstruction(false)?;
            } else if (_.matchPrefix("<!DOCTYPE")) {
                if (seen_doctype) {
                    return err.MalformedTag;
                }
                seen_doctype = true;
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

        // Only whitespace, comments, and non-xml PIs are permitted after root closes
        while (_.pos < _.src.len) {
            _.skipWhitespace();
            if (_.pos >= _.src.len) {
                break;
            }
            if (_.matchPrefix("<!--")) {
                _.skipComment()?;
            } else if (_.matchPrefix("<?")) {
                _.parseProcessingInstruction(false)?;
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
        _.bumpNodeCount()?;
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

            if (attrs.items.len >= XML_MAX_ATTRS_PER_ELEMENT) {
                return err.MaxAttrsExceeded;
            }

            const attr_name = _.parseName()?;
            for (existing of attrs.items) {
                if (std.mem.eql(u8, existing.name, attr_name)) {
                    return err.DuplicateAttribute;
                }
            }

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
            let has_amp = false;
            while (true) {
                const hit_opt = simdFindAttrQuoteOrAmp(_.src, _.pos, quote);
                if (hit_opt == null) {
                    return err.UnexpectedEof;
                }
                const hit = hit_opt ?? _.src.len;
                const ch = _.src[hit];
                if (ch == '<') {
                    return err.MalformedTag;
                }
                if (ch == '&') {
                    has_amp = true;
                    _.pos = hit + 1;
                    continue;
                }
                _.pos = hit;
                break;
            }
            const raw_val = _.src[val_start.._.pos];
            _.pos += 1;

            const decoded_val = if (has_amp)
                XmlParser.decodeEntitiesTracked(_.doc_alloc, raw_val, *_.entity_expansions)?
            else
                _.doc_alloc.dupe(u8, raw_val)?;
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
                    _.bumpNodeCount()?;
                    const cdata_txt = _.parseCData()?;
                    raw_children.append(_.doc_alloc, XmlNode{
                        kind: .CData,
                        tag: "",
                        text: cdata_txt,
                        attrs: [],
                        children: [],
                    })?;
                } else if (_.matchPrefix("<?")) {
                    _.parseProcessingInstruction(false)?;
                } else if (_.matchPrefix("<!")) {
                    return err.MalformedTag;
                } else {
                    const child_elem = _.parseElement(depth + 1)?;
                    has_element_child = true;
                    raw_children.append(_.doc_alloc, child_elem)?;
                }
            } else {
                const txt_start = _.pos;
                let has_amp = false;
                while (true) {
                    const hit_opt = simdFindXmlTextDelim(_.src, _.pos);
                    if (hit_opt == null) {
                        _.pos = _.src.len;
                        break;
                    }
                    const hit = hit_opt ?? _.src.len;
                    const ch = _.src[hit];
                    if (ch == '<') {
                        _.pos = hit;
                        break;
                    }
                    if (ch == ']') {
                        if (hit + 2 < _.src.len && _.src[hit + 1] == ']' && _.src[hit + 2] == '>') {
                            return err.MalformedTag;
                        }
                        _.pos = hit + 1;
                        continue;
                    }
                    // ch == '&'
                    has_amp = true;
                    _.pos = hit + 1;
                }
                const raw_txt = _.src[txt_start.._.pos];
                const decoded_txt = if (has_amp)
                    XmlParser.decodeEntitiesTracked(_.doc_alloc, raw_txt, *_.entity_expansions)?
                else
                    _.doc_alloc.dupe(u8, raw_txt)?;
                if (decoded_txt.len > 0) {
                    _.bumpNodeCount()?;
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

// ============================================================================
// Zero-Allocation Strict W3C XML 1.0 Validator (roxmltree-aligned)
// ============================================================================

struct XmlNameSpan {
    start: usize = 0;
    end: usize = 0;
}

struct StrictXmlValidator {
    src: []const u8 = "",
    pos: usize = 0,
    node_count: usize = 0,
    entity_expansions: usize = 0,

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

    fn scanName(*_): XmlError!XmlNameSpan {
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
        return XmlNameSpan{ start: start, end: _.pos };
    }

    fn skipComment(*_): XmlError!void {
        _.pos += 4;
        while (_.pos + 1 < _.src.len) {
            if (_.src[_.pos] == '-' && _.src[_.pos + 1] == '-') {
                if (_.pos + 2 < _.src.len && _.src[_.pos + 2] == '>') {
                    _.pos += 3;
                    return;
                }
                return err.MalformedTag;
            }
            _.pos += 1;
        }
        return err.UnexpectedEof;
    }

    static fn validateXmlDeclBody(body: []const u8): XmlError!void {
        let i: usize = 0;
        let seen_version = false;
        let seen_encoding = false;
        let seen_standalone = false;
        while (i < body.len) {
            const had_ws = i < body.len && XmlParser.isWhitespace(body[i]);
            while (i < body.len && XmlParser.isWhitespace(body[i])) {
                i += 1;
            }
            if (i >= body.len) break;
            if (!had_ws) return err.MalformedTag;
            if (!XmlParser.isNameStartChar(body[i])) return err.MalformedTag;
            const k_start = i;
            i += 1;
            while (i < body.len && XmlParser.isNameChar(body[i])) {
                i += 1;
            }
            const key = body[k_start..i];
            while (i < body.len && XmlParser.isWhitespace(body[i])) {
                i += 1;
            }
            if (i >= body.len || body[i] != '=') return err.MalformedTag;
            i += 1;
            while (i < body.len && XmlParser.isWhitespace(body[i])) {
                i += 1;
            }
            if (i >= body.len || (body[i] != '"' && body[i] != '\'')) return err.MalformedTag;
            const quote = body[i];
            i += 1;
            const v_start = i;
            while (i < body.len && body[i] != quote) {
                i += 1;
            }
            if (i >= body.len) return err.MalformedTag;
            const val = body[v_start..i];
            i += 1;
            if (std.mem.eql(u8, key, "version")) {
                if (seen_version || val.len == 0) return err.MalformedTag;
                seen_version = true;
            } else if (std.mem.eql(u8, key, "encoding")) {
                if (seen_encoding || !seen_version || val.len == 0) return err.MalformedTag;
                seen_encoding = true;
            } else if (std.mem.eql(u8, key, "standalone")) {
                if (seen_standalone || !seen_version) return err.MalformedTag;
                if (!std.mem.eql(u8, val, "yes") && !std.mem.eql(u8, val, "no")) return err.MalformedTag;
                seen_standalone = true;
            } else {
                return err.MalformedTag;
            }
        }
        if (!seen_version) return err.MalformedTag;
    }

    fn skipPI(*_, allow_xml_decl: bool): XmlError!void {
        _.pos += 2;
        const target_span = _.scanName()?;
        const target = _.src[target_span.start..target_span.end];
        const is_xml_ci = isXmlTargetCaseInsensitive(target);
        const is_xml_decl = std.mem.eql(u8, target, "xml");
        if (is_xml_ci && (!allow_xml_decl || !is_xml_decl)) {
            return err.MalformedTag;
        }
        if (_.pos >= _.src.len) return err.UnexpectedEof;
        if (!_.matchPrefix("?>") && !XmlParser.isWhitespace(_.src[_.pos])) {
            return err.MalformedTag;
        }
        const body_start = _.pos;
        while (_.pos + 1 < _.src.len) {
            if (_.src[_.pos] == '?' && _.src[_.pos + 1] == '>') {
                const body = _.src[body_start.._.pos];
                _.pos += 2;
                if (is_xml_decl) {
                    StrictXmlValidator.validateXmlDeclBody(body)?;
                }
                return;
            }
            _.pos += 1;
        }
        return err.UnexpectedEof;
    }

    fn skipDoctype(*_): XmlError!void {
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
                if (_.pos >= _.src.len) return err.UnexpectedEof;
                _.pos += 1;
            } else if (c == '[') {
                bracket_depth += 1;
                _.pos += 1;
            } else if (c == ']') {
                if (bracket_depth > 0) bracket_depth -= 1;
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

    fn skipCData(*_): XmlError!void {
        _.pos += 9;
        while (_.pos + 2 < _.src.len) {
            if (_.src[_.pos] == ']' && _.src[_.pos + 1] == ']' && _.src[_.pos + 2] == '>') {
                _.pos += 3;
                return;
            }
            _.pos += 1;
        }
        return err.UnexpectedEof;
    }

    fn bumpNode(*_): XmlError!void {
        if (_.node_count >= XML_MAX_NODES) {
            return err.MaxNodesExceeded;
        }
        _.node_count += 1;
    }
}

/// Zero-allocation strict W3C XML 1.0 (5th Edition) validator aligned with `RazrFalcon/roxmltree`.
/// Uses fixed stack storage for element nesting (`XML_MAX_DEPTH`) and per-element attribute
/// deduplication (`XML_MAX_ATTRS_PER_ELEMENT`), accelerated with 16-lane SIMD scanning.
pub fn validateXmlStrict(xml_str: []const u8): XmlError!void {
    let input = xml_str;
    if (input.len >= 3 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
        input = input[3..];
    }
    if (simdHasInvalidXmlControl(input)) {
        return err.InvalidControlChar;
    }

    let v = StrictXmlValidator{
        src: input,
        pos: 0,
        node_count: 0,
        entity_expansions: 0,
    };

    // Prolog
    if (v.pos == 0 && v.matchPrefix("<?xml")) {
        v.skipPI(true)?;
    }
    let seen_doctype = false;
    while (v.pos < v.src.len) {
        v.skipWhitespace();
        if (v.matchPrefix("<!--")) {
            v.skipComment()?;
        } else if (v.matchPrefix("<?")) {
            v.skipPI(false)?;
        } else if (v.matchPrefix("<!DOCTYPE")) {
            if (seen_doctype) return err.MalformedTag;
            seen_doctype = true;
            v.skipDoctype()?;
        } else {
            break;
        }
    }

    if (v.pos >= v.src.len) {
        return err.UnexpectedEof;
    }
    if (v.src[v.pos] != '<') {
        return err.MalformedTag;
    }

    let tag_stack: [XML_MAX_DEPTH]XmlNameSpan = [XmlNameSpan{}; XML_MAX_DEPTH];
    let attr_buf: [XML_MAX_ATTRS_PER_ELEMENT]XmlNameSpan = [XmlNameSpan{}; XML_MAX_ATTRS_PER_ELEMENT];
    let depth: usize = 0;
    let parsed_root = false;

    while (true) {
        if (depth == 0 && parsed_root) {
            break;
        }
        if (v.pos >= v.src.len) {
            return err.UnexpectedEof;
        }

        if (v.src[v.pos] == '<') {
            if (v.matchPrefix("</")) {
                if (depth == 0) return err.MalformedTag;
                v.pos += 2;
                const close_span = v.scanName()?;
                v.skipWhitespace();
                if (v.pos >= v.src.len) return err.UnexpectedEof;
                if (v.src[v.pos] != '>') return err.MalformedTag;
                v.pos += 1;
                const open_span = tag_stack[depth - 1];
                const open_name = v.src[open_span.start..open_span.end];
                const close_name = v.src[close_span.start..close_span.end];
                if (!std.mem.eql(u8, open_name, close_name)) {
                    return err.MismatchedTag;
                }
                depth -= 1;
            } else if (v.matchPrefix("<!--")) {
                if (depth == 0) return err.MalformedTag;
                v.skipComment()?;
            } else if (v.matchPrefix("<![CDATA[")) {
                if (depth == 0) return err.MalformedTag;
                v.bumpNode()?;
                v.skipCData()?;
            } else if (v.matchPrefix("<?")) {
                if (depth == 0) return err.MalformedTag;
                v.skipPI(false)?;
            } else if (v.matchPrefix("<!")) {
                return err.MalformedTag;
            } else {
                // Start tag
                if (depth >= XML_MAX_DEPTH) {
                    return err.MaxDepthExceeded;
                }
                v.bumpNode()?;
                v.pos += 1;
                if (v.pos >= v.src.len) return err.UnexpectedEof;
                if (v.src[v.pos] == '/' || v.src[v.pos] == '!' || v.src[v.pos] == '?') {
                    return err.MalformedTag;
                }
                const tag_span = v.scanName()?;
                let attr_count: usize = 0;
                let self_closing = false;

                while (true) {
                    const had_ws = v.pos < v.src.len && XmlParser.isWhitespace(v.src[v.pos]);
                    v.skipWhitespace();
                    if (v.pos >= v.src.len) return err.UnexpectedEof;
                    if (v.matchPrefix("/>")) {
                        v.pos += 2;
                        self_closing = true;
                        break;
                    }
                    if (v.src[v.pos] == '>') {
                        v.pos += 1;
                        break;
                    }
                    if (!had_ws) return err.MalformedTag;
                    if (attr_count >= XML_MAX_ATTRS_PER_ELEMENT) {
                        return err.MaxAttrsExceeded;
                    }
                    const attr_span = v.scanName()?;
                    const attr_name = v.src[attr_span.start..attr_span.end];
                    let ai: usize = 0;
                    while (ai < attr_count) : (ai += 1) {
                        const prev = v.src[attr_buf[ai].start..attr_buf[ai].end];
                        if (std.mem.eql(u8, prev, attr_name)) {
                            return err.DuplicateAttribute;
                        }
                    }
                    attr_buf[attr_count] = attr_span;
                    attr_count += 1;

                    v.skipWhitespace();
                    if (v.pos >= v.src.len) return err.UnexpectedEof;
                    if (v.src[v.pos] != '=') return err.MalformedTag;
                    v.pos += 1;
                    v.skipWhitespace();
                    if (v.pos >= v.src.len) return err.UnexpectedEof;
                    const quote = v.src[v.pos];
                    if (quote != '"' && quote != '\'') return err.MalformedTag;
                    v.pos += 1;

                    const val_start = v.pos;
                    let has_amp = false;
                    while (true) {
                        const hit_opt = simdFindAttrQuoteOrAmp(v.src, v.pos, quote);
                        if (hit_opt == null) return err.UnexpectedEof;
                        const hit = hit_opt ?? v.src.len;
                        const ch = v.src[hit];
                        if (ch == '<') return err.MalformedTag;
                        if (ch == '&') {
                            has_amp = true;
                            v.pos = hit + 1;
                            continue;
                        }
                        v.pos = hit;
                        break;
                    }
                    if (has_amp) {
                        validateEntitySlice(v.src[val_start..v.pos], *v.entity_expansions)?;
                    }
                    v.pos += 1;
                }

                parsed_root = true;
                if (!self_closing) {
                    tag_stack[depth] = tag_span;
                    depth += 1;
                }
            }
        } else {
            if (depth == 0) return err.MalformedTag;
            const txt_start = v.pos;
            let has_amp = false;
            while (true) {
                const hit_opt = simdFindXmlTextDelim(v.src, v.pos);
                if (hit_opt == null) {
                    v.pos = v.src.len;
                    break;
                }
                const hit = hit_opt ?? v.src.len;
                const ch = v.src[hit];
                if (ch == '<') {
                    v.pos = hit;
                    break;
                }
                if (ch == ']') {
                    if (hit + 2 < v.src.len && v.src[hit + 1] == ']' && v.src[hit + 2] == '>') {
                        return err.MalformedTag;
                    }
                    v.pos = hit + 1;
                    continue;
                }
                has_amp = true;
                v.pos = hit + 1;
            }
            const raw_txt = v.src[txt_start..v.pos];
            if (has_amp) {
                validateEntitySlice(raw_txt, *v.entity_expansions)?;
            }
            if (raw_txt.len > 0) {
                v.bumpNode()?;
            }
        }
    }

    // Trailing Misc only (whitespace, comments, non-xml PIs)
    while (v.pos < v.src.len) {
        v.skipWhitespace();
        if (v.pos >= v.src.len) break;
        if (v.matchPrefix("<!--")) {
            v.skipComment()?;
        } else if (v.matchPrefix("<?")) {
            v.skipPI(false)?;
        } else {
            return err.MalformedTag;
        }
    }
}
