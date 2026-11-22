// Source: Absorbed from /data/dev/bun/src/css/rules/supports.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP CSS Conditional Rules `@supports` condition AST, recursive-descent
// parser, evaluator, and canonical serializer (zero heap allocation via fixed arena).

pub enum SupportsKind: u8 {
    declaration = 0,
    selector = 1,
    unknown = 2,
    not_cond = 3,
    and_cond = 4,
    or_cond = 5,
}

pub struct SupportsNode {
    kind: SupportsKind,
    /// For `.declaration`: property name slice. For `.selector`/`.unknown`: raw inner slice.
    prop: []const u8,
    /// For `.declaration`: property value slice.
    value: []const u8,
    /// For `.not_cond`: child index in `SupportsTree.nodes`.
    /// For `.and_cond` / `.or_cond`: start offset in `SupportsTree.children`.
    first_child: u16,
    /// For `.and_cond` / `.or_cond`: number of children.
    child_count: u16,
}

/// Fixed-capacity arena representing a parsed `@supports` condition tree.
pub struct SupportsTree {
    nodes: [64]SupportsNode,
    node_len: u16,
    children: [64]u16,
    child_len: u16,
    root: u16,

    pub static fn initEmpty(): SupportsTree {
        return SupportsTree{
            nodes: [SupportsNode{ kind: .unknown, prop: "", value: "", first_child: 0, child_count: 0 }; 64],
            node_len: 0,
            children: [0; 64],
            child_len: 0,
            root: 0,
        };
    }

    fn pushNode(*_, node: SupportsNode): u16? {
        if (_.node_len >= 64) return null;
        const idx = _.node_len;
        _.nodes[idx] = node;
        _.node_len += 1;
        return idx;
    }

    /// Parse a CSS `<supports-condition>` string (without leading `@supports`).
    /// Supports `not (...)`, `(...) and (...)`, `(...) or (...)`, `(prop: val)`,
    /// `selector(...)`, nested parens, and unknown general-enclosed conditions.
    pub static fn parse(input: []const u8): SupportsTree? {
        let tree = SupportsTree.initEmpty();
        let pos: usize = 0;
        const root = parseCondition(&tree, input, &pos) ?? return null;
        skipWs(input, &pos);
        if (pos != input.len) return null;
        tree.root = root;
        return tree;
    }

    /// Serialize the condition tree back to canonical CSS into `out`.
    pub fn toCss(out: []u8): []const u8 {
        const len = writeNode(_, _.root, null, out, 0);
        return out[0..len];
    }

    /// Evaluate the condition tree given a declaration predicate and selector predicate.
    pub fn eval(
        decl_fn: fn ([]const u8, []const u8) bool,
        sel_fn: fn ([]const u8) bool,
    ): bool {
        return evalNode(_, _.root, decl_fn, sel_fn);
    }
}

fn evalNode(
    tree: SupportsTree,
    idx: u16,
    decl_fn: fn ([]const u8, []const u8) bool,
    sel_fn: fn ([]const u8) bool,
): bool {
    const n = tree.nodes[idx];
    switch (n.kind) {
        .declaration => return decl_fn(n.prop, n.value),
        .selector => return sel_fn(n.prop),
        .unknown => return false,
        .not_cond => return !evalNode(tree, n.first_child, decl_fn, sel_fn),
        .and_cond => {
            let i: u16 = 0;
            while (i < n.child_count) : (i += 1) {
                const c = tree.children[n.first_child + i];
                if (!evalNode(tree, c, decl_fn, sel_fn)) return false;
            }
            return true;
        },
        .or_cond => {
            let i: u16 = 0;
            while (i < n.child_count) : (i += 1) {
                const c = tree.children[n.first_child + i];
                if (evalNode(tree, c, decl_fn, sel_fn)) return true;
            }
            return false;
        },
    }
}

fn parseCondition(tree: *SupportsTree, s: []const u8, pos: *usize): u16? {
    skipWs(s, pos);
    if (consumeKeyword(s, pos, "not")) {
        skipWs(s, pos);
        const child = parseInParens(tree, s, pos) ?? return null;
        return tree.pushNode(SupportsNode{
            kind: .not_cond,
            prop: "",
            value: "",
            first_child: child,
            child_count: 1,
        });
    }

    const first = parseInParens(tree, s, pos) ?? return null;
    let op: u8 = 0; // 0=none, 1=and, 2=or
    let list: [32]u16 = [0; 32];
    let list_len: u16 = 1;
    list[0] = first;

    while (true) {
        const saved = pos.*;
        skipWs(s, pos);
        let cur_op: u8 = 0;
        if (consumeKeyword(s, pos, "and")) {
            cur_op = 1;
        } else if (consumeKeyword(s, pos, "or")) {
            cur_op = 2;
        } else {
            pos.* = saved;
            break;
        }

        // CSS Conditional Rules §3: cannot mix `and` and `or` at the same level
        if (op != 0 && op != cur_op) return null;
        op = cur_op;

        skipWs(s, pos);
        const next_node = parseInParens(tree, s, pos) ?? return null;
        if (list_len >= 32) return null;
        list[list_len] = next_node;
        list_len += 1;
    }

    if (op == 0) return first;
    if (tree.child_len + list_len > 64) return null;
    const start_child = tree.child_len;
    let i: u16 = 0;
    while (i < list_len) : (i += 1) {
        tree.children[tree.child_len] = list[i];
        tree.child_len += 1;
    }
    const kind: SupportsKind = if (op == 1) .and_cond else .or_cond;
    return tree.pushNode(SupportsNode{
        kind: kind,
        prop: "",
        value: "",
        first_child: start_child,
        child_count: list_len,
    });
}

fn parseInParens(tree: *SupportsTree, s: []const u8, pos: *usize): u16? {
    skipWs(s, pos);
    if (pos.* >= s.len) return null;

    // `selector(...)`
    if (startsWithFunc(s, pos.*, "selector")) {
        const open_p = pos.* + 8;
        const close_p = findMatchingParen(s, open_p) ?? return null;
        const inner = trimAscii(s[open_p + 1 .. close_p]);
        pos.* = close_p + 1;
        if (inner.len == 0) return null;
        return tree.pushNode(SupportsNode{
            kind: .selector,
            prop: inner,
            value: "",
            first_child: 0,
            child_count: 0,
        });
    }

    // `(` ... `)`
    if (s[pos.*] == '(') {
        const open_p = pos.*;
        const close_p = findMatchingParen(s, open_p) ?? return null;
        const inner = trimAscii(s[open_p + 1 .. close_p]);
        pos.* = close_p + 1;
        if (inner.len == 0) return null;

        // Try nested condition first if inner starts with `not ` or `(` or `selector(`
        if (startsWithNestedCondition(inner)) {
            let sub_pos: usize = 0;
            if (nested := parseCondition(tree, inner, &sub_pos)) {
                skipWs(inner, &sub_pos);
                if (sub_pos == inner.len) return nested;
            }
        }

        // Try declaration `prop: value`
        if (colon := findTopLevelColon(inner)) {
            const prop = trimAscii(inner[0..colon]);
            const val = trimAscii(inner[colon + 1 ..]);
            if (isValidPropIdent(prop) && val.len > 0) {
                return tree.pushNode(SupportsNode{
                    kind: .declaration,
                    prop: prop,
                    value: val,
                    first_child: 0,
                    child_count: 0,
                });
            }
        }

        // Fallback: general-enclosed unknown condition
        return tree.pushNode(SupportsNode{
            kind: .unknown,
            prop: s[open_p .. close_p + 1],
            value: "",
            first_child: 0,
            child_count: 0,
        });
    }

    // General-enclosed function `foo(...)`
    const ident_start = pos.*;
    while (pos.* < s.len && isIdentChar(s[pos.*])) : (pos.* += 1) {}
    if (pos.* > ident_start && pos.* < s.len && s[pos.*] == '(') {
        const close_p = findMatchingParen(s, pos.*) ?? return null;
        const raw = s[ident_start .. close_p + 1];
        pos.* = close_p + 1;
        return tree.pushNode(SupportsNode{
            kind: .unknown,
            prop: raw,
            value: "",
            first_child: 0,
            child_count: 0,
        });
    }

    return null;
}

fn needsParens(child_kind: SupportsKind, parent_kind: SupportsKind?): bool {
    if (parent_kind == null) return false;
    const pk = parent_kind.?;
    return switch (child_kind) {
        .not_cond => true,
        .and_cond => pk != .and_cond,
        .or_cond => pk != .or_cond,
        else => false,
    };
}

fn writeNode(tree: SupportsTree, idx: u16, parent_kind: SupportsKind?, out: []u8, pos: usize): usize {
    const n = tree.nodes[idx];
    const wrap = needsParens(n.kind, parent_kind);
    let p = pos;
    if (wrap) p = appendStr(out, p, "(");
    switch (n.kind) {
        .not_cond => {
            p = appendStr(out, p, "not ");
            p = writeNode(tree, n.first_child, .not_cond, out, p);
        },
        .and_cond => {
            let i: u16 = 0;
            while (i < n.child_count) : (i += 1) {
                if (i > 0) p = appendStr(out, p, " and ");
                p = writeNode(tree, tree.children[n.first_child + i], .and_cond, out, p);
            }
        },
        .or_cond => {
            let i: u16 = 0;
            while (i < n.child_count) : (i += 1) {
                if (i > 0) p = appendStr(out, p, " or ");
                p = writeNode(tree, tree.children[n.first_child + i], .or_cond, out, p);
            }
        },
        .declaration => {
            p = appendStr(out, p, "(");
            p = appendStr(out, p, n.prop);
            p = appendStr(out, p, ": ");
            p = appendStr(out, p, n.value);
            p = appendStr(out, p, ")");
        },
        .selector => {
            p = appendStr(out, p, "selector(");
            p = appendStr(out, p, n.prop);
            p = appendStr(out, p, ")");
        },
        .unknown => {
            p = appendStr(out, p, n.prop);
        },
    }
    if (wrap) p = appendStr(out, p, ")");
    return p;
}

// ---------- Helpers ----------

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn skipWs(s: []const u8, pos: *usize) {
    while (pos.* < s.len && isWs(s[pos.*])) : (pos.* += 1) {}
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) : (start += 1) {}
    let end = s.len;
    while (end > start && isWs(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLower(a[i]) != toLower(b[i])) return false;
    }
    return true;
}

fn isIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
}

fn isValidPropIdent(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (!isIdentChar(s[i])) return false;
    }
    return true;
}

fn consumeKeyword(s: []const u8, pos: *usize, kw: []const u8): bool {
    if (pos.* + kw.len > s.len) return false;
    if (!eqIgnoreCase(s[pos.* .. pos.* + kw.len], kw)) return false;
    if (pos.* + kw.len < s.len && isIdentChar(s[pos.* + kw.len])) return false;
    pos.* += kw.len;
    return true;
}

fn startsWithFunc(s: []const u8, pos: usize, name: []const u8): bool {
    if (pos + name.len + 1 > s.len) return false;
    return eqIgnoreCase(s[pos .. pos + name.len], name) && s[pos + name.len] == '(';
}

fn startsWithNestedCondition(inner: []const u8): bool {
    if (inner.len == 0) return false;
    if (inner[0] == '(') return true;
    if (startsWithFunc(inner, 0, "selector")) return true;
    if (inner.len >= 4 && eqIgnoreCase(inner[0..3], "not") && (isWs(inner[3]) || inner[3] == '(')) return true;
    return false;
}

fn findMatchingParen(s: []const u8, open_p: usize): usize? {
    if (open_p >= s.len || s[open_p] != '(') return null;
    let depth: u32 = 0;
    let i = open_p;
    while (i < s.len) : (i += 1) {
        if (s[i] == '(') {
            depth += 1;
        } else if (s[i] == ')') {
            depth -= 1;
            if (depth == 0) return i;
        }
    }
    return null;
}

fn findTopLevelColon(s: []const u8): usize? {
    let depth: u32 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '(') {
            depth += 1;
        } else if (s[i] == ')') {
            if (depth > 0) depth -= 1;
        } else if (s[i] == ':' && depth == 0) {
            return i;
        }
    }
    return null;
}

fn appendStr(out: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}
