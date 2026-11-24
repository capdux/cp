// Reference-hardened against RazrFalcon/roxmltree (MIT/Apache-2.0)
// Source: cp/lib/xml/writer.cp (XML 1.0 Compact & Pretty Serializer)
// Pure CP streaming XML 1.0 writer and DOM tree serializer with automatic
// attribute/text entity escaping, CDATA `]]>` validation, and compact/pretty modes.

const std = @import("std");
import { XmlNode, XmlNodeKind, XmlError } from "./parser.cp";

struct WriterFrame {
    name: []const u8 = "",
    start_tag_open: bool = true,
    has_child_elements: bool = false,
    has_inline_content: bool = false,
}

pub struct XmlWriter {
    allocator: std.mem.Allocator = undefined,
    buf: std.ArrayList(u8) = .empty,
    stack: std.ArrayList(WriterFrame) = .empty,
    pretty: bool = false,
    indent: usize = 2,

    /// Initializes a compact XML 1.0 writer (no extra whitespace or indentation).
    pub static fn init(alloc: std.mem.Allocator): XmlWriter {
        return XmlWriter{
            allocator: alloc,
            buf: .empty,
            stack: .empty,
            pretty: false,
            indent: 0,
        };
    }

    /// Initializes a pretty-printing XML 1.0 writer using `indent` spaces per nesting level.
    pub static fn initPretty(alloc: std.mem.Allocator, indent: usize): XmlWriter {
        return XmlWriter{
            allocator: alloc,
            buf: .empty,
            stack: .empty,
            pretty: true,
            indent: indent,
        };
    }

    /// Releases all internal buffers held by the writer.
    pub fn deinit(*_) {
        for (frame of _.stack.items) {
            if (frame.name.len > 0) {
                _.allocator.free(frame.name);
            }
        }
        _.stack.deinit(_.allocator);
        _.buf.deinit(_.allocator);
    }

    /// Returns a read-only view of the currently written XML bytes.
    pub fn slice(): []const u8 {
        return _.buf.items;
    }

    /// Transfers ownership of the generated XML buffer to the caller.
    pub fn toOwnedSlice(*_): ([]u8)! {
        return _.buf.toOwnedSlice(_.allocator);
    }

    fn appendIndent(*_, depth: usize): ! {
        let count = depth * _.indent;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            _.buf.append(_.allocator, ' ')?;
        }
    }

    fn flushOpenStartTag(*_): ! {
        if (_.stack.items.len > 0) {
            const top_idx = _.stack.items.len - 1;
            if (_.stack.items[top_idx].start_tag_open) {
                _.buf.append(_.allocator, '>')?;
                _.stack.items[top_idx].start_tag_open = false;
            }
        }
    }

    fn appendEscapedText(*_, s: []const u8): ! {
        let i: usize = 0;
        let flush_start: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c == '&' || c == '<' || c == '>') {
                if (i > flush_start) {
                    _.buf.appendSlice(_.allocator, s[flush_start..i])?;
                }
                switch (c) {
                    '&' => _.buf.appendSlice(_.allocator, "&amp;")?,
                    '<' => _.buf.appendSlice(_.allocator, "&lt;")?,
                    '>' => _.buf.appendSlice(_.allocator, "&gt;")?,
                    else => {},
                }
                flush_start = i + 1;
            }
        }
        if (i > flush_start) {
            _.buf.appendSlice(_.allocator, s[flush_start..i])?;
        }
    }

    fn appendEscapedAttr(*_, s: []const u8): ! {
        let i: usize = 0;
        let flush_start: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c == '&' || c == '<' || c == '>' || c == '"' || c == '\'' || c == '\n' || c == '\r' || c == '\t') {
                if (i > flush_start) {
                    _.buf.appendSlice(_.allocator, s[flush_start..i])?;
                }
                switch (c) {
                    '&' => _.buf.appendSlice(_.allocator, "&amp;")?,
                    '<' => _.buf.appendSlice(_.allocator, "&lt;")?,
                    '>' => _.buf.appendSlice(_.allocator, "&gt;")?,
                    '"' => _.buf.appendSlice(_.allocator, "&quot;")?,
                    '\'' => _.buf.appendSlice(_.allocator, "&apos;")?,
                    '\n' => _.buf.appendSlice(_.allocator, "&#10;")?,
                    '\r' => _.buf.appendSlice(_.allocator, "&#13;")?,
                    '\t' => _.buf.appendSlice(_.allocator, "&#9;")?,
                    else => {},
                }
                flush_start = i + 1;
            }
        }
        if (i > flush_start) {
            _.buf.appendSlice(_.allocator, s[flush_start..i])?;
        }
    }

    /// Writes the standard XML 1.0 UTF-8 declaration `<?xml version="1.0" encoding="UTF-8"?>`.
    pub fn writeDeclaration(*_): ! {
        _.buf.appendSlice(_.allocator, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>")?;
    }

    /// Opens a new XML element tag `<name`. Attributes may follow via `attr()`.
    pub fn openTag(*_, name: []const u8): ! {
        if (name.len == 0) {
            return err.MalformedTag;
        }
        if (_.stack.items.len > 0) {
            _.flushOpenStartTag()?;
            const parent_idx = _.stack.items.len - 1;
            _.stack.items[parent_idx].has_child_elements = true;
            if (_.pretty && !_.stack.items[parent_idx].has_inline_content) {
                _.buf.append(_.allocator, '\n')?;
                _.appendIndent(_.stack.items.len)?;
            }
        } else if (_.pretty && _.buf.items.len > 0) {
            _.buf.append(_.allocator, '\n')?;
        }

        const owned_name = _.allocator.dupe(u8, name)?;
        errdefer _.allocator.free(owned_name);

        _.stack.append(_.allocator, WriterFrame{
            name: owned_name,
            start_tag_open: true,
            has_child_elements: false,
            has_inline_content: false,
        })?;

        _.buf.append(_.allocator, '<')?;
        _.buf.appendSlice(_.allocator, name)?;
    }

    /// Appends an XML attribute `name="value"` to the currently open start tag.
    pub fn attr(*_, name: []const u8, value: []const u8): ! {
        if (name.len == 0 || _.stack.items.len == 0) {
            return err.MalformedTag;
        }
        const top_idx = _.stack.items.len - 1;
        if (!_.stack.items[top_idx].start_tag_open) {
            return err.MalformedTag;
        }
        _.buf.append(_.allocator, ' ')?;
        _.buf.appendSlice(_.allocator, name)?;
        _.buf.appendSlice(_.allocator, "=\"")?;
        _.appendEscapedAttr(value)?;
        _.buf.append(_.allocator, '"')?;
    }

    /// Writes escaped character data (`&`, `<`, `>` escaped) inside the current element.
    pub fn text(*_, content: []const u8): ! {
        if (_.stack.items.len > 0) {
            _.flushOpenStartTag()?;
            const top_idx = _.stack.items.len - 1;
            _.stack.items[top_idx].has_inline_content = true;
        }
        _.appendEscapedText(content)?;
    }

    /// Writes a `<![CDATA[...]]>` section. Rejects content containing embedded `]]>`.
    pub fn cdata(*_, content: []const u8): ! {
        if (std.mem.indexOf(u8, content, "]]>") != null) {
            return err.MalformedTag;
        }
        if (_.stack.items.len > 0) {
            _.flushOpenStartTag()?;
            const top_idx = _.stack.items.len - 1;
            _.stack.items[top_idx].has_inline_content = true;
        }
        _.buf.appendSlice(_.allocator, "<![CDATA[")?;
        _.buf.appendSlice(_.allocator, content)?;
        _.buf.appendSlice(_.allocator, "]]>")?;
    }

    /// Writes a self-closing empty element `<name/>`.
    pub fn emptyTag(*_, name: []const u8): ! {
        if (name.len == 0) {
            return err.MalformedTag;
        }
        if (_.stack.items.len > 0) {
            _.flushOpenStartTag()?;
            const parent_idx = _.stack.items.len - 1;
            _.stack.items[parent_idx].has_child_elements = true;
            if (_.pretty && !_.stack.items[parent_idx].has_inline_content) {
                _.buf.append(_.allocator, '\n')?;
                _.appendIndent(_.stack.items.len)?;
            }
        } else if (_.pretty && _.buf.items.len > 0) {
            _.buf.append(_.allocator, '\n')?;
        }

        _.buf.append(_.allocator, '<')?;
        _.buf.appendSlice(_.allocator, name)?;
        _.buf.appendSlice(_.allocator, "/>")?;
    }

    /// Closes the most recently opened tag (`/>` if empty, or `</name>` otherwise).
    pub fn closeTag(*_): ! {
        if (_.stack.items.len == 0) {
            return err.MismatchedTag;
        }
        const frame = _.stack.pop().?;
        defer {
            if (frame.name.len > 0) {
                _.allocator.free(frame.name);
            }
        }

        if (frame.start_tag_open) {
            _.buf.appendSlice(_.allocator, "/>")?;
        } else {
            if (_.pretty && frame.has_child_elements && !frame.has_inline_content) {
                _.buf.append(_.allocator, '\n')?;
                _.appendIndent(_.stack.items.len)?;
            }
            _.buf.appendSlice(_.allocator, "</")?;
            _.buf.appendSlice(_.allocator, frame.name)?;
            _.buf.append(_.allocator, '>')?;
        }
    }
}

pub struct XmlSerializer {
    static fn writeNode(w: *XmlWriter, node: XmlNode): ! {
        switch (node.kind) {
            .Text => w.text(node.text)?,
            .CData => w.cdata(node.text)?,
            .Element => {
                if (node.children.len == 0 && node.attrs.len == 0) {
                    w.emptyTag(node.tag)?;
                    return;
                }
                w.openTag(node.tag)?;
                for (a of node.attrs) {
                    w.attr(a.name, a.value)?;
                }
                for (c of node.children) {
                    XmlSerializer.writeNode(w, c)?;
                }
                w.closeTag()?;
            },
        }
    }

    /// Serializes an `XmlNode` DOM subtree back to XML 1.0 text (compact or pretty-printed).
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub static fn stringify(alloc: std.mem.Allocator, node: XmlNode, pretty: bool): ([]u8)! {
        let w = if (pretty) XmlWriter.initPretty(alloc, 2) else XmlWriter.init(alloc);
        defer w.deinit();
        XmlSerializer.writeNode(&w, node)?;
        return w.toOwnedSlice();
    }
}
