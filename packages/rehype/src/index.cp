// @cap/rehype-ast — Zero-allocation HAST (HTML AST) builder & HTML5 serializer for CP
// Clean-room MIT implementation with parity to npm `rehype` / `hast`.
// Composes `@cap/escape-html` for XSS-safe text and attribute escaping with zero duplication.

import { strEq, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { escapeHtml } from "../../escape-html/src/index.cp";

pub const HAST_ROOT: u8 = 0;
pub const HAST_ELEMENT: u8 = 1;
pub const HAST_TEXT: u8 = 2;
pub const HAST_COMMENT: u8 = 3;
pub const HAST_DOCTYPE: u8 = 4;

const RH_MAX_NODES: usize = 64;
const RH_MAX_ATTRS: usize = 64;
const RH_TEXT_CAP: usize = 256;
const RH_ATTR_NAME_CAP: usize = 48;
const RH_ATTR_VAL_CAP: usize = 192;
const RH_MAX_DEPTH: usize = 48;

fn rhWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

/// Return `true` if `tag` is one of the 14 standard HTML5 void elements
/// (`area`, `base`, `br`, `col`, `embed`, `hr`, `img`, `input`, `link`, `meta`, `param`, `source`, `track`, `wbr`).
pub fn isVoidElement(tag: string): bool {
    if (eqIgnoreCase(tag, "area")) { return true; }
    if (eqIgnoreCase(tag, "base")) { return true; }
    if (eqIgnoreCase(tag, "br")) { return true; }
    if (eqIgnoreCase(tag, "col")) { return true; }
    if (eqIgnoreCase(tag, "embed")) { return true; }
    if (eqIgnoreCase(tag, "hr")) { return true; }
    if (eqIgnoreCase(tag, "img")) { return true; }
    if (eqIgnoreCase(tag, "input")) { return true; }
    if (eqIgnoreCase(tag, "link")) { return true; }
    if (eqIgnoreCase(tag, "meta")) { return true; }
    if (eqIgnoreCase(tag, "param")) { return true; }
    if (eqIgnoreCase(tag, "source")) { return true; }
    if (eqIgnoreCase(tag, "track")) { return true; }
    if (eqIgnoreCase(tag, "wbr")) { return true; }
    return false;
}

/// Element attribute stored in the fixed-size `HastTree` attribute pool.
pub struct HastAttr {
    pub name: [48]u8,
    pub name_len: usize,
    pub value: [192]u8,
    pub value_len: usize,
    pub next_attr: i32,
}

/// Node stored in the fixed-size `HastTree` node pool.
pub struct HastNode {
    pub kind: u8,
    pub text: [256]u8,
    pub text_len: usize,
    pub first_child: i32,
    pub last_child: i32,
    pub next_sibling: i32,
    pub first_attr: i32,
    pub last_attr: i32,
}

fn rhMakeNode(kind: u8, text: string): HastNode {
    let tbuf: [256]u8 = undefined;
    tbuf[0] = 0;
    const tlen = writeStr(text, tbuf[0..RH_TEXT_CAP], 0);
    return HastNode {
        kind: kind,
        text: tbuf,
        text_len: tlen,
        first_child: -1,
        last_child: -1,
        next_sibling: -1,
        first_attr: -1,
        last_attr: -1,
    };
}

fn rhSerializeHtml(
    nodes: []const HastNode,
    attrs: []const HastAttr,
    node_id: usize,
    out: []u8,
    pos: usize,
    depth: usize,
): usize {
    if (depth > RH_MAX_DEPTH || pos >= out.len) {
        return pos;
    }
    const node = nodes[node_id];
    let w = pos;

    if (node.kind == HAST_ROOT) {
        let child = node.first_child;
        while (child >= 0 && w < out.len) {
            w = rhSerializeHtml(nodes, attrs, child as usize, out, w, depth + 1);
            child = nodes[child as usize].next_sibling;
        }
        return w;
    }

    if (node.kind == HAST_DOCTYPE) {
        return writeStr("<!doctype html>", out, w);
    }

    if (node.kind == HAST_TEXT) {
        if (node.text_len == 0) {
            return w;
        }
        const n = escapeHtml(node.text[0..node.text_len], out[w..out.len]);
        return w + n;
    }

    if (node.kind == HAST_COMMENT) {
        w = writeStr("<!--", out, w);
        w = writeStr(node.text[0..node.text_len], out, w);
        w = writeStr("-->", out, w);
        return w;
    }

    if (node.kind == HAST_ELEMENT) {
        const tag = node.text[0..node.text_len];
        w = rhWriteByte(out, w, 60); // '<'
        w = writeStr(tag, out, w);

        let ai = node.first_attr;
        while (ai >= 0 && w < out.len) {
            const attr = attrs[ai as usize];
            w = rhWriteByte(out, w, 32); // ' '
            w = writeStr(attr.name[0..attr.name_len], out, w);
            w = writeStr("=\"", out, w);
            if (attr.value_len > 0 && w < out.len) {
                const en = escapeHtml(attr.value[0..attr.value_len], out[w..out.len]);
                w = w + en;
            }
            w = rhWriteByte(out, w, 34); // '"'
            ai = attr.next_attr;
        }

        w = rhWriteByte(out, w, 62); // '>'

        if (!isVoidElement(tag)) {
            let child = node.first_child;
            while (child >= 0 && w < out.len) {
                w = rhSerializeHtml(nodes, attrs, child as usize, out, w, depth + 1);
                child = nodes[child as usize].next_sibling;
            }
            w = writeStr("</", out, w);
            w = writeStr(tag, out, w);
            w = rhWriteByte(out, w, 62); // '>'
        }
        return w;
    }

    return w;
}

fn rhExtractText(
    nodes: []const HastNode,
    node_id: usize,
    out: []u8,
    pos: usize,
    depth: usize,
): usize {
    if (depth > RH_MAX_DEPTH || pos >= out.len) {
        return pos;
    }
    const node = nodes[node_id];
    if (node.kind == HAST_TEXT) {
        return writeStr(node.text[0..node.text_len], out, pos);
    }
    if (node.kind == HAST_ROOT || node.kind == HAST_ELEMENT) {
        let w = pos;
        let child = node.first_child;
        while (child >= 0 && w < out.len) {
            w = rhExtractText(nodes, child as usize, out, w, depth + 1);
            child = nodes[child as usize].next_sibling;
        }
        return w;
    }
    return pos;
}

/// Fixed-pool zero-allocation HAST tree (up to 64 nodes and 64 attributes).
pub struct HastTree {
    nodes: [64]HastNode,
    pub node_len: usize,
    attrs: [64]HastAttr,
    pub attr_len: usize,

    /// Create a new `HastTree` initialized with a root node at ID `0`.
    pub static fn new(): HastTree {
        let nodes: [64]HastNode = undefined;
        let attrs: [64]HastAttr = undefined;
        nodes[0] = rhMakeNode(HAST_ROOT, "");
        return HastTree {
            nodes: nodes,
            node_len: 1,
            attrs: attrs,
            attr_len: 0,
        };
    }

    /// Append a new element child `<tag>` under `parent_id`. Returns new node ID or `-1`.
    pub fn addElement(*_, parent_id: usize, tag: string): i32 {
        if (parent_id >= _.node_len || _.node_len >= RH_MAX_NODES || tag.len == 0 || tag.len > RH_TEXT_CAP) {
            return -1;
        }
        const pkind = _.nodes[parent_id].kind;
        if (pkind != HAST_ROOT && pkind != HAST_ELEMENT) {
            return -1;
        }
        if (pkind == HAST_ELEMENT && isVoidElement(_.nodes[parent_id].text[0.._.nodes[parent_id].text_len])) {
            return -1;
        }
        const id = _.node_len as i32;
        _.nodes[_.node_len] = rhMakeNode(HAST_ELEMENT, tag);
        _.node_len = _.node_len + 1;

        if (_.nodes[parent_id].first_child < 0) {
            _.nodes[parent_id].first_child = id;
            _.nodes[parent_id].last_child = id;
        } else {
            const last = _.nodes[parent_id].last_child as usize;
            _.nodes[last].next_sibling = id;
            _.nodes[parent_id].last_child = id;
        }
        return id;
    }

    /// Append a new text child under `parent_id`. Returns new node ID or `-1`.
    pub fn addText(*_, parent_id: usize, text: string): i32 {
        if (parent_id >= _.node_len || _.node_len >= RH_MAX_NODES || text.len > RH_TEXT_CAP) {
            return -1;
        }
        const pkind = _.nodes[parent_id].kind;
        if (pkind != HAST_ROOT && pkind != HAST_ELEMENT) {
            return -1;
        }
        const id = _.node_len as i32;
        _.nodes[_.node_len] = rhMakeNode(HAST_TEXT, text);
        _.node_len = _.node_len + 1;

        if (_.nodes[parent_id].first_child < 0) {
            _.nodes[parent_id].first_child = id;
            _.nodes[parent_id].last_child = id;
        } else {
            const last = _.nodes[parent_id].last_child as usize;
            _.nodes[last].next_sibling = id;
            _.nodes[parent_id].last_child = id;
        }
        return id;
    }

    /// Append a new HTML comment child `<!--comment-->` under `parent_id`. Returns new node ID or `-1`.
    pub fn addComment(*_, parent_id: usize, comment: string): i32 {
        if (parent_id >= _.node_len || _.node_len >= RH_MAX_NODES || comment.len > RH_TEXT_CAP) {
            return -1;
        }
        const pkind = _.nodes[parent_id].kind;
        if (pkind != HAST_ROOT && pkind != HAST_ELEMENT) {
            return -1;
        }
        const id = _.node_len as i32;
        _.nodes[_.node_len] = rhMakeNode(HAST_COMMENT, comment);
        _.node_len = _.node_len + 1;

        if (_.nodes[parent_id].first_child < 0) {
            _.nodes[parent_id].first_child = id;
            _.nodes[parent_id].last_child = id;
        } else {
            const last = _.nodes[parent_id].last_child as usize;
            _.nodes[last].next_sibling = id;
            _.nodes[parent_id].last_child = id;
        }
        return id;
    }

    /// Append an HTML5 `<!doctype html>` node under `parent_id`. Returns new node ID or `-1`.
    pub fn addDoctype(*_, parent_id: usize): i32 {
        if (parent_id >= _.node_len || _.node_len >= RH_MAX_NODES) {
            return -1;
        }
        const pkind = _.nodes[parent_id].kind;
        if (pkind != HAST_ROOT && pkind != HAST_ELEMENT) {
            return -1;
        }
        const id = _.node_len as i32;
        _.nodes[_.node_len] = rhMakeNode(HAST_DOCTYPE, "");
        _.node_len = _.node_len + 1;

        if (_.nodes[parent_id].first_child < 0) {
            _.nodes[parent_id].first_child = id;
            _.nodes[parent_id].last_child = id;
        } else {
            const last = _.nodes[parent_id].last_child as usize;
            _.nodes[last].next_sibling = id;
            _.nodes[parent_id].last_child = id;
        }
        return id;
    }

    /// Set or overwrite attribute `name="value"` on element `node_id`.
    pub fn setAttr(*_, node_id: usize, name: string, value: string): bool {
        if (node_id >= _.node_len
            || _.nodes[node_id].kind != HAST_ELEMENT
            || name.len == 0
            || name.len > RH_ATTR_NAME_CAP
            || value.len > RH_ATTR_VAL_CAP) {
            return false;
        }

        // Update existing attribute with same name
        let cur = _.nodes[node_id].first_attr;
        while (cur >= 0) {
            const idx = cur as usize;
            if (strEq(_.attrs[idx].name[0.._.attrs[idx].name_len], name)) {
                _.attrs[idx].value_len = writeStr(value, _.attrs[idx].value[0..RH_ATTR_VAL_CAP], 0);
                return true;
            }
            cur = _.attrs[idx].next_attr;
        }

        if (_.attr_len >= RH_MAX_ATTRS) {
            return false;
        }

        let nbuf: [48]u8 = undefined;
        let vbuf: [192]u8 = undefined;
        nbuf[0] = 0;
        vbuf[0] = 0;
        const nlen = writeStr(name, nbuf[0..RH_ATTR_NAME_CAP], 0);
        const vlen = writeStr(value, vbuf[0..RH_ATTR_VAL_CAP], 0);

        const aid = _.attr_len as i32;
        _.attrs[_.attr_len] = HastAttr {
            name: nbuf,
            name_len: nlen,
            value: vbuf,
            value_len: vlen,
            next_attr: -1,
        };
        _.attr_len = _.attr_len + 1;

        if (_.nodes[node_id].first_attr < 0) {
            _.nodes[node_id].first_attr = aid;
            _.nodes[node_id].last_attr = aid;
        } else {
            const last = _.nodes[node_id].last_attr as usize;
            _.attrs[last].next_attr = aid;
            _.nodes[node_id].last_attr = aid;
        }
        return true;
    }

    /// Serialize the entire HAST tree to canonical HTML5 into `out`.
    /// All text nodes and attribute values are safely escaped via `@cap/escape-html`.
    /// Returns bytes written.
    pub fn toHtml(out: []u8): usize {
        if (out.len == 0 || _.node_len == 0) {
            return 0;
        }
        return rhSerializeHtml(_.nodes[0.._.node_len], _.attrs[0.._.attr_len], 0, out, 0, 0);
    }

    /// Extract concatenated plain text content (ignoring tags, attributes, comments, and doctype).
    /// Returns bytes written.
    pub fn toText(out: []u8): usize {
        if (out.len == 0 || _.node_len == 0) {
            return 0;
        }
        return rhExtractText(_.nodes[0.._.node_len], 0, out, 0, 0);
    }

    /// Return total number of nodes in the tree (including root node `0`).
    pub fn nodeCount(): usize {
        return _.node_len;
    }

    /// Count element nodes matching `tag` (ASCII case-insensitive).
    pub fn countByTag(tag: string): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.node_len) {
            const node = _.nodes[i];
            if (node.kind == HAST_ELEMENT && eqIgnoreCase(node.text[0..node.text_len], tag)) {
                total = total + 1;
            }
            i = i + 1;
        }
        return total;
    }
}

export {
    HAST_ROOT,
    HAST_ELEMENT,
    HAST_TEXT,
    HAST_COMMENT,
    HAST_DOCTYPE,
    HastAttr,
    HastNode,
    HastTree,
    isVoidElement,
};
