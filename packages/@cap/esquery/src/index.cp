// @cap/esquery — Zero-allocation ESTree AST CSS-like selector parser & matcher
// Clean-room BSD-3-Clause implementation inspired by npm:esquery (ESLint core)

pub const MAX_SELECTOR_NODES: usize = 64;

pub enum SelectorKind {
    Wildcard,
    NodeType,
    Attribute,
    Compound,
    Descendant,
    Child,
    AdjacentSibling,
    Matches,
}

pub enum AttrOp {
    Exists,
    Eq,
    NotEq,
    Lt,
    Gt,
}

pub struct AstNodeInfo {
    node_type: string,
    name: string,
    operator: string,
    parent_idx: i32,
    prev_sibling_idx: i32,
}

pub struct SelectorNode {
    kind: SelectorKind,
    attr_op: AttrOp,
    text: string,
    attr_name: string,
    attr_val: string,
    left: i32,
    right: i32,
}

pub struct ParsedSelector {
    ok: bool,
    root: i32,
    count: usize,
    error_pos: usize,
    nodes: [64]SelectorNode,
}

struct EsqParser {
    src: string,
    pos: usize,
    has_err: bool,
    err_pos: usize,
    count: usize,
    nodes: [64]SelectorNode,
}

struct EsqIntParse {
    ok: bool,
    val: i64,
}

fn esqEmptyNode(): SelectorNode {
    return SelectorNode{
        kind: SelectorKind.Wildcard,
        attr_op: AttrOp.Exists,
        text: "",
        attr_name: "",
        attr_val: "",
        left: -1,
        right: -1,
    };
}

fn esqIsWs(ch: u8): bool {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

fn esqIsIdentStart(ch: u8): bool {
    if (ch >= 'a' && ch <= 'z') return true;
    if (ch >= 'A' && ch <= 'Z') return true;
    return ch == '_' || ch == '$';
}

fn esqIsIdentChar(ch: u8): bool {
    if (esqIsIdentStart(ch)) return true;
    return ch >= '0' && ch <= '9';
}

fn esqCanStartSimple(ch: u8): bool {
    return ch == '*' || ch == '[' || esqIsIdentStart(ch);
}

fn esqStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn esqStrCmp(a: string, b: string): i32 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) : (i += 1) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

fn esqParseI64(s: string): EsqIntParse {
    if (s.len == 0) return EsqIntParse{ ok: false, val: 0 };
    let i: usize = 0;
    let neg: bool = false;
    if (s[0] == '-') {
        if (s.len == 1) return EsqIntParse{ ok: false, val: 0 };
        neg = true;
        i = 1;
    }
    let acc: i64 = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch < '0' || ch > '9') return EsqIntParse{ ok: false, val: 0 };
        acc = acc * 10 + i64(ch - '0');
    }
    if (neg) {
        return EsqIntParse{ ok: true, val: -acc };
    }
    return EsqIntParse{ ok: true, val: acc };
}

fn esqSetErr(p: *EsqParser, pos: usize) {
    if (!p.has_err) {
        p.has_err = true;
        p.err_pos = pos;
    }
}

fn esqSkipWs(p: *EsqParser): bool {
    const start = p.pos;
    while (p.pos < p.src.len && esqIsWs(p.src[p.pos])) {
        p.pos += 1;
    }
    return p.pos > start;
}

fn esqAddNode(p: *EsqParser, node: SelectorNode): i32 {
    if (p.count >= MAX_SELECTOR_NODES) {
        esqSetErr(p, p.pos);
        return -1;
    }
    const idx = p.count;
    p.nodes[idx] = node;
    p.count += 1;
    return i32(idx);
}

fn esqParseAttr(p: *EsqParser): i32 {
    // consume '['
    p.pos += 1;
    * = esqSkipWs(p);

    const name_start = p.pos;
    while (p.pos < p.src.len) {
        const ch = p.src[p.pos];
        if (esqIsIdentChar(ch) || ch == '.') {
            p.pos += 1;
        } else {
            break;
        }
    }
    if (p.pos == name_start) {
        esqSetErr(p, p.pos);
        return -1;
    }
    const attr_name = p.src[name_start..p.pos];
    * = esqSkipWs(p);

    if (p.pos >= p.src.len) {
        esqSetErr(p, p.pos);
        return -1;
    }

    if (p.src[p.pos] == ']') {
        p.pos += 1;
        return esqAddNode(p, SelectorNode{
            kind: SelectorKind.Attribute,
            attr_op: AttrOp.Exists,
            text: "",
            attr_name: attr_name,
            attr_val: "",
            left: -1,
            right: -1,
        });
    }

    let op = AttrOp.Eq;
    const op_ch = p.src[p.pos];
    if (op_ch == '=') {
        op = AttrOp.Eq;
        p.pos += 1;
        if (p.pos < p.src.len && p.src[p.pos] == '=') {
            p.pos += 1;
        }
    } else if (op_ch == '!') {
        if (p.pos + 1 < p.src.len && p.src[p.pos + 1] == '=') {
            op = AttrOp.NotEq;
            p.pos += 2;
        } else {
            esqSetErr(p, p.pos);
            return -1;
        }
    } else if (op_ch == '<') {
        op = AttrOp.Lt;
        p.pos += 1;
    } else if (op_ch == '>') {
        op = AttrOp.Gt;
        p.pos += 1;
    } else {
        esqSetErr(p, p.pos);
        return -1;
    }

    * = esqSkipWs(p);
    if (p.pos >= p.src.len) {
        esqSetErr(p, p.pos);
        return -1;
    }

    let attr_val: string = "";
    const q = p.src[p.pos];
    if (q == '"' || q == '\'') {
        p.pos += 1;
        const val_start = p.pos;
        let closed = false;
        while (p.pos < p.src.len) {
            const c = p.src[p.pos];
            if (c == '\\' && p.pos + 1 < p.src.len) {
                p.pos += 2;
                continue;
            }
            if (c == q) {
                attr_val = p.src[val_start..p.pos];
                p.pos += 1;
                closed = true;
                break;
            }
            p.pos += 1;
        }
        if (!closed) {
            esqSetErr(p, p.pos);
            return -1;
        }
    } else {
        const val_start = p.pos;
        while (p.pos < p.src.len) {
            const c = p.src[p.pos];
            if (c == ']' || esqIsWs(c)) break;
            p.pos += 1;
        }
        if (p.pos == val_start) {
            esqSetErr(p, p.pos);
            return -1;
        }
        attr_val = p.src[val_start..p.pos];
    }

    * = esqSkipWs(p);
    if (p.pos >= p.src.len || p.src[p.pos] != ']') {
        esqSetErr(p, p.pos);
        return -1;
    }
    p.pos += 1;

    return esqAddNode(p, SelectorNode{
        kind: SelectorKind.Attribute,
        attr_op: op,
        text: "",
        attr_name: attr_name,
        attr_val: attr_val,
        left: -1,
        right: -1,
    });
}

fn esqParseSimple(p: *EsqParser): i32 {
    if (p.pos >= p.src.len) {
        esqSetErr(p, p.pos);
        return -1;
    }
    const ch = p.src[p.pos];
    if (ch == '*') {
        p.pos += 1;
        return esqAddNode(p, SelectorNode{
            kind: SelectorKind.Wildcard,
            attr_op: AttrOp.Exists,
            text: "*",
            attr_name: "",
            attr_val: "",
            left: -1,
            right: -1,
        });
    }
    if (ch == '[') {
        return esqParseAttr(p);
    }
    if (esqIsIdentStart(ch)) {
        const start = p.pos;
        p.pos += 1;
        while (p.pos < p.src.len && esqIsIdentChar(p.src[p.pos])) {
            p.pos += 1;
        }
        const ident = p.src[start..p.pos];
        return esqAddNode(p, SelectorNode{
            kind: SelectorKind.NodeType,
            attr_op: AttrOp.Exists,
            text: ident,
            attr_name: "",
            attr_val: "",
            left: -1,
            right: -1,
        });
    }
    esqSetErr(p, p.pos);
    return -1;
}

fn esqParseCompound(p: *EsqParser): i32 {
    let left = esqParseSimple(p);
    if (p.has_err || left < 0) return -1;

    while (p.pos < p.src.len && esqCanStartSimple(p.src[p.pos])) {
        const right = esqParseSimple(p);
        if (p.has_err || right < 0) return -1;
        left = esqAddNode(p, SelectorNode{
            kind: SelectorKind.Compound,
            attr_op: AttrOp.Exists,
            text: "",
            attr_name: "",
            attr_val: "",
            left: left,
            right: right,
        });
        if (p.has_err || left < 0) return -1;
    }
    return left;
}

fn esqParseComplex(p: *EsqParser): i32 {
    let left = esqParseCompound(p);
    if (p.has_err || left < 0) return -1;

    while (p.pos < p.src.len) {
        const had_ws = esqSkipWs(p);
        if (p.pos >= p.src.len) break;

        const ch = p.src[p.pos];
        if (ch == ',') break;

        if (ch == '>') {
            p.pos += 1;
            * = esqSkipWs(p);
            const right = esqParseCompound(p);
            if (p.has_err || right < 0) return -1;
            left = esqAddNode(p, SelectorNode{
                kind: SelectorKind.Child,
                attr_op: AttrOp.Exists,
                text: "",
                attr_name: "",
                attr_val: "",
                left: left,
                right: right,
            });
            if (p.has_err || left < 0) return -1;
        } else if (ch == '+') {
            p.pos += 1;
            * = esqSkipWs(p);
            const right = esqParseCompound(p);
            if (p.has_err || right < 0) return -1;
            left = esqAddNode(p, SelectorNode{
                kind: SelectorKind.AdjacentSibling,
                attr_op: AttrOp.Exists,
                text: "",
                attr_name: "",
                attr_val: "",
                left: left,
                right: right,
            });
            if (p.has_err || left < 0) return -1;
        } else if (had_ws && esqCanStartSimple(ch)) {
            const right = esqParseCompound(p);
            if (p.has_err || right < 0) return -1;
            left = esqAddNode(p, SelectorNode{
                kind: SelectorKind.Descendant,
                attr_op: AttrOp.Exists,
                text: "",
                attr_name: "",
                attr_val: "",
                left: left,
                right: right,
            });
            if (p.has_err || left < 0) return -1;
        } else {
            esqSetErr(p, p.pos);
            return -1;
        }
    }
    return left;
}

fn esqParseList(p: *EsqParser): i32 {
    * = esqSkipWs(p);
    if (p.pos >= p.src.len) {
        esqSetErr(p, p.pos);
        return -1;
    }

    let left = esqParseComplex(p);
    if (p.has_err || left < 0) return -1;

    while (p.pos < p.src.len) {
        * = esqSkipWs(p);
        if (p.pos >= p.src.len) break;
        if (p.src[p.pos] == ',') {
            p.pos += 1;
            * = esqSkipWs(p);
            const right = esqParseComplex(p);
            if (p.has_err || right < 0) return -1;
            left = esqAddNode(p, SelectorNode{
                kind: SelectorKind.Matches,
                attr_op: AttrOp.Exists,
                text: "",
                attr_name: "",
                attr_val: "",
                left: left,
                right: right,
            });
            if (p.has_err || left < 0) return -1;
        } else {
            esqSetErr(p, p.pos);
            return -1;
        }
    }
    return left;
}

pub fn parseSelector(sel: string): ParsedSelector {
    let empty_nodes: [64]SelectorNode = undefined;
    let i: usize = 0;
    while (i < MAX_SELECTOR_NODES) : (i += 1) {
        empty_nodes[i] = esqEmptyNode();
    }

    let p = EsqParser{
        src: sel,
        pos: 0,
        has_err: false,
        err_pos: 0,
        count: 0,
        nodes: empty_nodes,
    };

    const root = esqParseList(*p);
    if (p.has_err || root < 0 || p.pos != sel.len) {
        return ParsedSelector{
            ok: false,
            root: -1,
            count: p.count,
            error_pos: p.err_pos,
            nodes: p.nodes,
        };
    }

    return ParsedSelector{
        ok: true,
        root: root,
        count: p.count,
        error_pos: 0,
        nodes: p.nodes,
    };
}

fn esqGetAttrVal(ast_node: AstNodeInfo, attr_name: string): string {
    if (esqStrEq(attr_name, "name")) return ast_node.name;
    if (esqStrEq(attr_name, "operator")) return ast_node.operator;
    if (esqStrEq(attr_name, "type") || esqStrEq(attr_name, "node_type")) return ast_node.node_type;
    return "";
}

fn esqMatchAttr(ast_node: AstNodeInfo, sel_node: SelectorNode): bool {
    const val = esqGetAttrVal(ast_node, sel_node.attr_name);
    if (sel_node.attr_op == AttrOp.Exists) {
        return val.len > 0;
    }
    if (sel_node.attr_op == AttrOp.Eq) {
        return esqStrEq(val, sel_node.attr_val);
    }
    if (sel_node.attr_op == AttrOp.NotEq) {
        return !esqStrEq(val, sel_node.attr_val);
    }
    const lhs_num = esqParseI64(val);
    const rhs_num = esqParseI64(sel_node.attr_val);
    if (lhs_num.ok && rhs_num.ok) {
        if (sel_node.attr_op == AttrOp.Lt) return lhs_num.val < rhs_num.val;
        return lhs_num.val > rhs_num.val;
    }
    const cmp = esqStrCmp(val, sel_node.attr_val);
    if (sel_node.attr_op == AttrOp.Lt) return cmp < 0;
    return cmp > 0;
}

fn esqMatchRec(sel_nodes: []const SelectorNode, sel_idx: i32, nodes: []const AstNodeInfo, idx: usize, depth: usize): bool {
    if (depth >= 64) return false;
    if (sel_idx < 0) return false;
    const s_idx = usize(sel_idx);
    if (s_idx >= sel_nodes.len || idx >= nodes.len) return false;

    const sn = sel_nodes[s_idx];
    const an = nodes[idx];

    if (sn.kind == SelectorKind.Wildcard) {
        return true;
    }
    if (sn.kind == SelectorKind.NodeType) {
        return esqStrEq(an.node_type, sn.text);
    }
    if (sn.kind == SelectorKind.Attribute) {
        return esqMatchAttr(an, sn);
    }
    if (sn.kind == SelectorKind.Compound) {
        if (!esqMatchRec(sel_nodes, sn.left, nodes, idx, depth + 1)) return false;
        return esqMatchRec(sel_nodes, sn.right, nodes, idx, depth + 1);
    }
    if (sn.kind == SelectorKind.Matches) {
        if (esqMatchRec(sel_nodes, sn.left, nodes, idx, depth + 1)) return true;
        return esqMatchRec(sel_nodes, sn.right, nodes, idx, depth + 1);
    }
    if (sn.kind == SelectorKind.Child) {
        if (!esqMatchRec(sel_nodes, sn.right, nodes, idx, depth + 1)) return false;
        if (an.parent_idx < 0) return false;
        const p_idx = usize(an.parent_idx);
        if (p_idx >= nodes.len) return false;
        return esqMatchRec(sel_nodes, sn.left, nodes, p_idx, depth + 1);
    }
    if (sn.kind == SelectorKind.AdjacentSibling) {
        if (!esqMatchRec(sel_nodes, sn.right, nodes, idx, depth + 1)) return false;
        if (an.prev_sibling_idx < 0) return false;
        const prev_idx = usize(an.prev_sibling_idx);
        if (prev_idx >= nodes.len) return false;
        return esqMatchRec(sel_nodes, sn.left, nodes, prev_idx, depth + 1);
    }
    if (sn.kind == SelectorKind.Descendant) {
        if (!esqMatchRec(sel_nodes, sn.right, nodes, idx, depth + 1)) return false;
        let cur_parent = an.parent_idx;
        let hops: usize = 0;
        while (cur_parent >= 0 && hops < 128) : (hops += 1) {
            const p_idx = usize(cur_parent);
            if (p_idx >= nodes.len) break;
            if (esqMatchRec(sel_nodes, sn.left, nodes, p_idx, depth + 1)) return true;
            cur_parent = nodes[p_idx].parent_idx;
        }
        return false;
    }
    return false;
}

pub fn matchesSelector(sel: ParsedSelector, nodes: []const AstNodeInfo, idx: usize): bool {
    if (!sel.ok || idx >= nodes.len) return false;
    const local_nodes: [64]SelectorNode = sel.nodes;
    return esqMatchRec(local_nodes[0..sel.count], sel.root, nodes, idx, 0);
}

pub fn queryAll(sel_str: string, nodes: []const AstNodeInfo, out_indices: []usize): usize {
    const parsed = parseSelector(sel_str);
    if (!parsed.ok) return 0;

    const local_nodes: [64]SelectorNode = parsed.nodes;
    let found: usize = 0;
    let i: usize = 0;
    while (i < nodes.len) : (i += 1) {
        if (esqMatchRec(local_nodes[0..parsed.count], parsed.root, nodes, i, 0)) {
            if (found < out_indices.len) {
                out_indices[found] = i;
            }
            found += 1;
        }
    }
    return found;
}
