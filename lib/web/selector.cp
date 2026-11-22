// Source: cp/lib/selector/selector.cp (CSS3 Selector Compiler & XML/DOM Query Engine)
// Pure CP, zero-C-dependency CSS3 selector compiler and DOM query engine for cap:xml.
// Supports: tag, #id, .class, *, [attr], [attr=v], [attr^=v], [attr$=v], [attr*=v], [attr~=v],
// combinators (' ', '>', '+', '~'), :first-child, :last-child, :only-child, :empty,
// :nth-child(odd/even/an+b), :not(...), and comma-separated selector lists in document order.

const std = @import("std");
import { XmlNode, XmlNodeKind, XmlDocument, XmlParser } from "cap:xml";

pub err SelectorError {
    EmptySelector,
    UnexpectedToken,
    UnclosedBracket,
    UnclosedParen,
    InvalidNthExpr,
    UnsupportedPseudo,
    OutOfMemory,
}

pub enum AttrOp {
    Exists,
    Equals,
    Prefix,
    Suffix,
    Substring,
    Word,
}

pub enum Combinator {
    Descendant,
    Child,
    AdjacentSibling,
    GeneralSibling,
}

pub type NodePtr = &XmlNode;

pub struct AttrSelector {
    name: []const u8 = "",
    op: AttrOp = .Exists,
    value: []const u8 = "",
    case_insensitive: bool = false,
}

pub enum PseudoKind {
    FirstChild,
    LastChild,
    OnlyChild,
    Empty,
    NthChild,
    Not,
}

pub struct PseudoSelector {
    kind: PseudoKind = .FirstChild,
    nth_a: i32 = 0,
    nth_b: i32 = 0,
    not_sel: (*CompoundSelector!)? = null,
}

pub struct CompoundSelector {
    tag: ([]const u8)? = null,
    has_universal: bool = false,
    ids: [][]const u8 = [],
    classes: [][]const u8 = [],
    attrs: []AttrSelector = [],
    pseudos: []PseudoSelector = [],
}

pub struct ComplexPart {
    combinator: Combinator = .Descendant,
    compound: CompoundSelector = CompoundSelector{},
}

pub struct ComplexSelector {
    parts: []ComplexPart = [],
}

struct ElemPosInfo {
    pos: usize = 1,
    total: usize = 1,
}

struct NthPair {
    a: i32 = 0,
    b: i32 = 0,
}

struct ElemFrame {
    node: NodePtr,
    parent_children: []XmlNode = [],
    child_idx: usize = 0,
}

pub struct CssSelector {
    selectors: []ComplexSelector = [],
    owned_arena: (*std.heap.ArenaAllocator!)? = null,
    parent_alloc: std.mem.Allocator? = null,

    /// Compiles a CSS3 selector expression (supporting comma-separated lists) into an AST.
    pub static fn compile(alloc: std.mem.Allocator, expr: []const u8): CssSelector ! SelectorError {
        let sel_arena = alloc.create(std.heap.ArenaAllocator) catch {
            return SelectorError.OutOfMemory;
        };
        unsafe {
            sel_arena.* = std.heap.ArenaAllocator.init(alloc);
        }
        errdefer {
            sel_arena.deinit();
            alloc.destroy(sel_arena);
        }

        const arena_alloc = sel_arena.allocator();
        let p = SelectorParser{
            src: expr,
            pos: 0,
            alloc: arena_alloc,
        };
        const sels = p.parseSelectorList()?;

        return CssSelector{
            selectors: sels,
            owned_arena: sel_arena,
            parent_alloc: alloc,
        };
    }

    /// Releases all memory owned by the compiled selector AST in O(1).
    pub fn deinit(*_) {
        if (owned := _.owned_arena) {
            owned.deinit();
            if (p := _.parent_alloc) {
                p.destroy(owned);
            }
            _.owned_arena = null;
            _.parent_alloc = null;
        }
        _.selectors = [];
    }

    /// Returns true if `node` (located within `root`) matches any selector in this list.
    pub fn matches(node: NodePtr, root: NodePtr): bool {
        if (node.kind != .Element) {
            return false;
        }
        let stack: [128]ElemFrame = undefined;
        let depth: usize = 0;
        let cur_frame = ElemFrame{
            node: root,
            parent_children: [],
            child_idx: 0,
        };
        if (!CssSelector.findNodePath(node, root, &stack, *depth, *cur_frame)) {
            return false;
        }
        const anc = stack[0..depth];
        for (complex of _.selectors) {
            if (complex.parts.len > 0) {
                if (CssSelector.matchComplexAt(complex.parts, complex.parts.len - 1, anc, anc.len, cur_frame)) {
                    return true;
                }
            }
        }
        return false;
    }

    /// Queries all matching element nodes in `root` in document order, deduplicated across `,` lists.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn queryAll(alloc: std.mem.Allocator, root: NodePtr): ([]NodePtr) ! SelectorError {
        let out: std.ArrayList(NodePtr) = .empty;
        errdefer out.deinit(alloc);

        if (root.kind != .Element) {
            return out.toOwnedSlice(alloc) catch {
                return SelectorError.OutOfMemory;
            };
        }

        let stack: [128]ElemFrame = undefined;
        const root_frame = ElemFrame{
            node: root,
            parent_children: [],
            child_idx: 0,
        };
        * = _.collectMatches(alloc, root_frame, &stack, 0, &out)?;

        return out.toOwnedSlice(alloc) catch {
            return SelectorError.OutOfMemory;
        };
    }

    /// Queries the first matching element node in `root` in document order, or `null` if none match.
    pub fn queryFirst(root: NodePtr): NodePtr? {
        if (root.kind != .Element) {
            return null;
        }
        let stack: [128]ElemFrame = undefined;
        const root_frame = ElemFrame{
            node: root,
            parent_children: [],
            child_idx: 0,
        };
        return _.findFirstMatch(root_frame, &stack, 0);
    }

    /// Parses `xml_text` via `cap:xml` and returns `innerText` for each element matching `css_expr` in document order.
    /// Caller owns each string and the outer slice; free with `CssSelector.freeTexts(alloc, list)`.
    pub static fn selectXml(alloc: std.mem.Allocator, xml_text: []const u8, css_expr: []const u8): ([][]u8)! {
        let doc = XmlParser.parse(alloc, xml_text)?;
        defer doc.deinit();

        let sel = CssSelector.compile(alloc, css_expr)?;
        defer sel.deinit();

        const nodes = sel.queryAll(alloc, &doc.root)?;
        defer alloc.free(nodes);

        let texts: std.ArrayList([]u8) = .empty;
        errdefer {
            for (t of texts.items) {
                alloc.free(t);
            }
            texts.deinit(alloc);
        }

        for (n of nodes) {
            const txt = n.innerText(alloc)?;
            texts.append(alloc, txt)?;
        }
        return texts.toOwnedSlice(alloc);
    }

    /// Parses `xml_text` via `cap:xml` and returns `innerText` of the first element matching `css_expr`, or `null`.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub static fn selectXmlFirst(alloc: std.mem.Allocator, xml_text: []const u8, css_expr: []const u8): ([]u8)?! {
        let doc = XmlParser.parse(alloc, xml_text)?;
        defer doc.deinit();

        let sel = CssSelector.compile(alloc, css_expr)?;
        defer sel.deinit();

        if (first := sel.queryFirst(&doc.root)) {
            return first.innerText(alloc)?;
        }
        return null;
    }

    /// Frees a slice of strings returned by `selectXml`.
    pub static fn freeTexts(alloc: std.mem.Allocator, texts: [][]u8) {
        for (t of texts) {
            alloc.free(t);
        }
        alloc.free(texts);
    }

    fn collectMatches(
        alloc: std.mem.Allocator,
        cur: ElemFrame,
        stack: *[128]ElemFrame,
        depth: usize,
        out: *std.ArrayList(NodePtr),
    ): bool ! SelectorError {
        const anc = stack[0..depth];
        for (complex of _.selectors) {
            if (complex.parts.len > 0) {
                if (CssSelector.matchComplexAt(complex.parts, complex.parts.len - 1, anc, depth, cur)) {
                    out.append(alloc, cur.node) catch {
                        return SelectorError.OutOfMemory;
                    };
                    break;
                }
            }
        }

        if (depth >= 127) {
            return true;
        }
        stack[depth] = cur;
        let i: usize = 0;
        while (i < cur.node.children.len) : (i += 1) {
            const child_ptr: NodePtr = &cur.node.children[i];
            if (child_ptr.kind == .Element) {
                const next_frame = ElemFrame{
                    node: child_ptr,
                    parent_children: cur.node.children,
                    child_idx: i,
                };
                * = _.collectMatches(alloc, next_frame, stack, depth + 1, out)?;
            }
        }
        return true;
    }

    fn findFirstMatch(cur: ElemFrame, stack: *[128]ElemFrame, depth: usize): NodePtr? {
        const anc = stack[0..depth];
        for (complex of _.selectors) {
            if (complex.parts.len > 0) {
                if (CssSelector.matchComplexAt(complex.parts, complex.parts.len - 1, anc, depth, cur)) {
                    return cur.node;
                }
            }
        }

        if (depth >= 127) {
            return null;
        }
        stack[depth] = cur;
        let i: usize = 0;
        while (i < cur.node.children.len) : (i += 1) {
            const child_ptr: NodePtr = &cur.node.children[i];
            if (child_ptr.kind == .Element) {
                const next_frame = ElemFrame{
                    node: child_ptr,
                    parent_children: cur.node.children,
                    child_idx: i,
                };
                if (hit := _.findFirstMatch(next_frame, stack, depth + 1)) {
                    return hit;
                }
            }
        }
        return null;
    }

    static fn findNodePath(
        target: NodePtr,
        cur: NodePtr,
        stack: *[128]ElemFrame,
        depth: *usize,
        out_frame: *ElemFrame,
    ): bool {
        const is_same = unsafe { @intFromPtr(cur) == @intFromPtr(target) };
        if (is_same) {
            return true;
        }
        if (depth.* >= 127) {
            return false;
        }
        const saved_depth = depth.*;
        const parent_frame = out_frame.*;
        let i: usize = 0;
        while (i < cur.children.len) : (i += 1) {
            const child_ptr: NodePtr = &cur.children[i];
            if (child_ptr.kind == .Element) {
                stack[saved_depth] = parent_frame;
                depth.* = saved_depth + 1;
                out_frame.* = ElemFrame{
                    node: child_ptr,
                    parent_children: cur.children,
                    child_idx: i,
                };
                if (CssSelector.findNodePath(target, child_ptr, stack, depth, out_frame)) {
                    return true;
                }
            }
        }
        depth.* = saved_depth;
        out_frame.* = parent_frame;
        return false;
    }

    static fn matchComplexAt(
        parts: []ComplexPart,
        part_idx: usize,
        anc: []const ElemFrame,
        anc_len: usize,
        cur: ElemFrame,
    ): bool {
        if (!CssSelector.matchCompound(parts[part_idx].compound, cur)) {
            return false;
        }
        if (part_idx == 0) {
            return true;
        }

        const comb = parts[part_idx].combinator;
        const prev_part = part_idx - 1;

        switch (comb) {
            .Child => {
                if (anc_len == 0) {
                    return false;
                }
                const parent_frame = anc[anc_len - 1];
                return CssSelector.matchComplexAt(parts, prev_part, anc, anc_len - 1, parent_frame);
            },
            .Descendant => {
                let idx = anc_len;
                while (idx > 0) {
                    idx -= 1;
                    const anc_frame = anc[idx];
                    if (CssSelector.matchComplexAt(parts, prev_part, anc, idx, anc_frame)) {
                        return true;
                    }
                }
                return false;
            },
            .AdjacentSibling => {
                if (cur.parent_children.len == 0 || cur.child_idx == 0) {
                    return false;
                }
                let si = cur.child_idx;
                while (si > 0) {
                    si -= 1;
                    const sib_ptr: NodePtr = &cur.parent_children[si];
                    if (sib_ptr.kind == .Element) {
                        const sib_frame = ElemFrame{
                            node: sib_ptr,
                            parent_children: cur.parent_children,
                            child_idx: si,
                        };
                        return CssSelector.matchComplexAt(parts, prev_part, anc, anc_len, sib_frame);
                    }
                }
                return false;
            },
            .GeneralSibling => {
                if (cur.parent_children.len == 0 || cur.child_idx == 0) {
                    return false;
                }
                let si = cur.child_idx;
                while (si > 0) {
                    si -= 1;
                    const sib_ptr: NodePtr = &cur.parent_children[si];
                    if (sib_ptr.kind == .Element) {
                        const sib_frame = ElemFrame{
                            node: sib_ptr,
                            parent_children: cur.parent_children,
                            child_idx: si,
                        };
                        if (CssSelector.matchComplexAt(parts, prev_part, anc, anc_len, sib_frame)) {
                            return true;
                        }
                    }
                }
                return false;
            },
        }
    }

    static fn matchCompound(comp: CompoundSelector, frame: ElemFrame): bool {
        const node = frame.node;
        if (node.kind != .Element) {
            return false;
        }

        if (expected_tag := comp.tag) {
            if (!std.mem.eql(u8, node.tag, expected_tag)) {
                return false;
            }
        }

        for (expected_id of comp.ids) {
            if (actual_id := node.getAttr("id")) {
                if (!std.mem.eql(u8, actual_id, expected_id)) {
                    return false;
                }
            } else {
                return false;
            }
        }

        if (comp.classes.len > 0) {
            const class_attr = node.getAttr("class") orelse return false;
            for (expected_cls of comp.classes) {
                if (!CssSelector.containsWhitespaceWord(class_attr, expected_cls)) {
                    return false;
                }
            }
        }

        for (attr of comp.attrs) {
            if (!CssSelector.matchAttr(attr, node)) {
                return false;
            }
        }

        for (pseudo of comp.pseudos) {
            if (!CssSelector.matchPseudo(pseudo, frame)) {
                return false;
            }
        }

        return true;
    }

    static fn containsWhitespaceWord(haystack: []const u8, needle: []const u8): bool {
        if (needle.len == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < haystack.len) {
            while (i < haystack.len && isAsciiSpace(haystack[i])) {
                i += 1;
            }
            if (i >= haystack.len) {
                break;
            }
            const start = i;
            while (i < haystack.len && !isAsciiSpace(haystack[i])) {
                i += 1;
            }
            if (std.mem.eql(u8, haystack[start..i], needle)) {
                return true;
            }
        }
        return false;
    }

    static fn eqlSlice(a: []const u8, b: []const u8, case_insensitive: bool): bool {
        if (a.len != b.len) {
            return false;
        }
        if (!case_insensitive) {
            return std.mem.eql(u8, a, b);
        }
        return std.ascii.eqlIgnoreCase(a, b);
    }

    static fn matchAttr(attr: AttrSelector, node: NodePtr): bool {
        const actual = node.getAttr(attr.name) orelse return false;
        switch (attr.op) {
            .Exists => return true,
            .Equals => return CssSelector.eqlSlice(actual, attr.value, attr.case_insensitive),
            .Prefix => {
                if (attr.value.len == 0 || actual.len < attr.value.len) {
                    return false;
                }
                return CssSelector.eqlSlice(actual[0..attr.value.len], attr.value, attr.case_insensitive);
            },
            .Suffix => {
                if (attr.value.len == 0 || actual.len < attr.value.len) {
                    return false;
                }
                const start = actual.len - attr.value.len;
                return CssSelector.eqlSlice(actual[start..], attr.value, attr.case_insensitive);
            },
            .Substring => {
                if (attr.value.len == 0 || actual.len < attr.value.len) {
                    return false;
                }
                if (!attr.case_insensitive) {
                    return std.mem.indexOf(u8, actual, attr.value) != null;
                }
                const limit = actual.len - attr.value.len;
                let i: usize = 0;
                while (i <= limit) : (i += 1) {
                    if (std.ascii.eqlIgnoreCase(actual[i..(i + attr.value.len)], attr.value)) {
                        return true;
                    }
                }
                return false;
            },
            .Word => {
                if (attr.value.len == 0) {
                    return false;
                }
                for (c of attr.value) {
                    if (isAsciiSpace(c)) {
                        return false;
                    }
                }
                if (!attr.case_insensitive) {
                    return CssSelector.containsWhitespaceWord(actual, attr.value);
                }
                let i: usize = 0;
                while (i < actual.len) {
                    while (i < actual.len && isAsciiSpace(actual[i])) {
                        i += 1;
                    }
                    if (i >= actual.len) {
                        break;
                    }
                    const start = i;
                    while (i < actual.len && !isAsciiSpace(actual[i])) {
                        i += 1;
                    }
                    if (std.ascii.eqlIgnoreCase(actual[start..i], attr.value)) {
                        return true;
                    }
                }
                return false;
            },
        }
    }

    static fn elementPositionInfo(frame: ElemFrame): ElemPosInfo {
        if (frame.parent_children.len == 0) {
            return ElemPosInfo{ pos: 1, total: 1 };
        }
        let pos: usize = 0;
        let total: usize = 0;
        let i: usize = 0;
        while (i < frame.parent_children.len) : (i += 1) {
            if (frame.parent_children[i].kind == .Element) {
                total += 1;
                if (i == frame.child_idx) {
                    pos = total;
                }
            }
        }
        return ElemPosInfo{ pos: pos, total: total };
    }

    static fn matchPseudo(pseudo: PseudoSelector, frame: ElemFrame): bool {
        switch (pseudo.kind) {
            .FirstChild => {
                const info = CssSelector.elementPositionInfo(frame);
                return info.pos == 1;
            },
            .LastChild => {
                const info = CssSelector.elementPositionInfo(frame);
                return info.pos == info.total && info.total > 0;
            },
            .OnlyChild => {
                const info = CssSelector.elementPositionInfo(frame);
                return info.pos == 1 && info.total == 1;
            },
            .Empty => {
                for (c of frame.node.children) {
                    switch (c.kind) {
                        .Element => return false,
                        .Text | .CData => {
                            if (c.text.len > 0) {
                                return false;
                            }
                        },
                    }
                }
                return true;
            },
            .NthChild => {
                const info = CssSelector.elementPositionInfo(frame);
                const idx: i32 = @intCast(info.pos);
                const a = pseudo.nth_a;
                const b = pseudo.nth_b;
                if (a == 0) {
                    return b >= 1 && idx == b;
                }
                if (a > 0) {
                    return idx >= b && @rem(idx - b, a) == 0;
                }
                return idx <= b && @rem(b - idx, -a) == 0;
            },
            .Not => {
                if (not_ptr := pseudo.not_sel) {
                    unsafe {
                        return !CssSelector.matchCompound(not_ptr.*, frame);
                    }
                }
                return true;
            },
        }
    }
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0C;
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-' || c >= 0x80;
}

fn isIdentChar(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

struct SelectorParser {
    src: []const u8 = "",
    pos: usize = 0,
    alloc: std.mem.Allocator = undefined,

    fn skipWs(*_) {
        while (_.pos < _.src.len && isAsciiSpace(_.src[_.pos])) {
            _.pos += 1;
        }
    }

    fn parseSelectorList(*_): ([]ComplexSelector) ! SelectorError {
        _.skipWs();
        if (_.pos >= _.src.len) {
            return SelectorError.EmptySelector;
        }

        let list: std.ArrayList(ComplexSelector) = .empty;
        while (true) {
            _.skipWs();
            if (_.pos >= _.src.len) {
                return SelectorError.EmptySelector;
            }
            if (_.src[_.pos] == ',') {
                return SelectorError.EmptySelector;
            }

            const complex = _.parseComplexSelector()?;
            list.append(_.alloc, complex) catch {
                return SelectorError.OutOfMemory;
            };

            _.skipWs();
            if (_.pos >= _.src.len) {
                break;
            }
            if (_.src[_.pos] == ',') {
                _.pos += 1;
                _.skipWs();
                if (_.pos >= _.src.len) {
                    return SelectorError.EmptySelector;
                }
                continue;
            }
            return SelectorError.UnexpectedToken;
        }

        return list.toOwnedSlice(_.alloc) catch {
            return SelectorError.OutOfMemory;
        };
    }

    fn parseComplexSelector(*_): ComplexSelector ! SelectorError {
        let parts: std.ArrayList(ComplexPart) = .empty;

        const first = _.parseCompoundSelector()?;
        parts.append(_.alloc, ComplexPart{
            combinator: .Descendant,
            compound: first,
        }) catch {
            return SelectorError.OutOfMemory;
        };

        while (_.pos < _.src.len) {
            let had_ws = false;
            while (_.pos < _.src.len && isAsciiSpace(_.src[_.pos])) {
                had_ws = true;
                _.pos += 1;
            }
            if (_.pos >= _.src.len || _.src[_.pos] == ',') {
                break;
            }

            let comb: Combinator = .Descendant;
            let explicit_comb = false;
            const c = _.src[_.pos];
            if (c == '>') {
                comb = .Child;
                explicit_comb = true;
                _.pos += 1;
            } else if (c == '+') {
                comb = .AdjacentSibling;
                explicit_comb = true;
                _.pos += 1;
            } else if (c == '~') {
                comb = .GeneralSibling;
                explicit_comb = true;
                _.pos += 1;
            } else if (!had_ws) {
                return SelectorError.UnexpectedToken;
            }

            _.skipWs();
            if (_.pos >= _.src.len || _.src[_.pos] == ',') {
                if (explicit_comb) {
                    return SelectorError.UnexpectedToken;
                }
                break;
            }
            const next_c = _.src[_.pos];
            if (next_c == '>' || next_c == '+' || next_c == '~') {
                return SelectorError.UnexpectedToken;
            }

            const comp = _.parseCompoundSelector()?;
            parts.append(_.alloc, ComplexPart{
                combinator: comb,
                compound: comp,
            }) catch {
                return SelectorError.OutOfMemory;
            };
        }

        const owned = parts.toOwnedSlice(_.alloc) catch {
            return SelectorError.OutOfMemory;
        };
        return ComplexSelector{ parts: owned };
    }

    fn parseCompoundSelector(*_): CompoundSelector ! SelectorError {
        let comp = CompoundSelector{};
        let ids: std.ArrayList(string) = .empty;
        let classes: std.ArrayList(string) = .empty;
        let attrs: std.ArrayList(AttrSelector) = .empty;
        let pseudos: std.ArrayList(PseudoSelector) = .empty;

        let parsed_any = false;

        if (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '*') {
                comp.has_universal = true;
                _.pos += 1;
                parsed_any = true;
            } else if (isIdentStart(c)) {
                comp.tag = _.readIdent();
                parsed_any = true;
            }
        }

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (isAsciiSpace(c) || c == ',' || c == '>' || c == '+' || c == '~' || c == ')') {
                break;
            }
            if (c == '#') {
                _.pos += 1;
                if (_.pos >= _.src.len || !isIdentStart(_.src[_.pos])) {
                    return SelectorError.UnexpectedToken;
                }
                const id_name = _.readIdent();
                ids.append(_.alloc, id_name) catch {
                    return SelectorError.OutOfMemory;
                };
                parsed_any = true;
            } else if (c == '.') {
                _.pos += 1;
                if (_.pos >= _.src.len || !isIdentStart(_.src[_.pos])) {
                    return SelectorError.UnexpectedToken;
                }
                const cls_name = _.readIdent();
                classes.append(_.alloc, cls_name) catch {
                    return SelectorError.OutOfMemory;
                };
                parsed_any = true;
            } else if (c == '[') {
                const attr = _.parseAttrSelector()?;
                attrs.append(_.alloc, attr) catch {
                    return SelectorError.OutOfMemory;
                };
                parsed_any = true;
            } else if (c == ':') {
                const pseudo = _.parsePseudoSelector()?;
                pseudos.append(_.alloc, pseudo) catch {
                    return SelectorError.OutOfMemory;
                };
                parsed_any = true;
            } else {
                return SelectorError.UnexpectedToken;
            }
        }

        if (!parsed_any) {
            return SelectorError.UnexpectedToken;
        }

        comp.ids = ids.toOwnedSlice(_.alloc) catch {
            return SelectorError.OutOfMemory;
        };
        comp.classes = classes.toOwnedSlice(_.alloc) catch {
            return SelectorError.OutOfMemory;
        };
        comp.attrs = attrs.toOwnedSlice(_.alloc) catch {
            return SelectorError.OutOfMemory;
        };
        comp.pseudos = pseudos.toOwnedSlice(_.alloc) catch {
            return SelectorError.OutOfMemory;
        };
        return comp;
    }

    fn readIdent(*_): []const u8 {
        const start = _.pos;
        while (_.pos < _.src.len && isIdentChar(_.src[_.pos])) {
            _.pos += 1;
        }
        return _.src[start.._.pos];
    }

    fn parseAttrSelector(*_): AttrSelector ! SelectorError {
        _.pos += 1; // skip '['
        _.skipWs();
        if (_.pos >= _.src.len) {
            return SelectorError.UnclosedBracket;
        }
        if (!isIdentStart(_.src[_.pos])) {
            return SelectorError.UnexpectedToken;
        }
        const name = _.readIdent();
        _.skipWs();
        if (_.pos >= _.src.len) {
            return SelectorError.UnclosedBracket;
        }
        if (_.src[_.pos] == ']') {
            _.pos += 1;
            return AttrSelector{ name: name, op: .Exists, value: "" };
        }

        let op: AttrOp = .Equals;
        const c = _.src[_.pos];
        if (c == '=') {
            op = .Equals;
            _.pos += 1;
        } else if (c == '^' || c == '$' || c == '*' || c == '~') {
            if (_.pos + 1 >= _.src.len) {
                return SelectorError.UnclosedBracket;
            }
            if (_.src[_.pos + 1] != '=') {
                return SelectorError.UnexpectedToken;
            }
            if (c == '^') op = .Prefix;
            else if (c == '$') op = .Suffix;
            else if (c == '*') op = .Substring;
            else op = .Word;
            _.pos += 2;
        } else {
            return SelectorError.UnexpectedToken;
        }

        _.skipWs();
        if (_.pos >= _.src.len) {
            return SelectorError.UnclosedBracket;
        }

        let val: []const u8 = "";
        const vc = _.src[_.pos];
        if (vc == '"' || vc == '\'') {
            const quote = vc;
            _.pos += 1;
            const vstart = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != quote) {
                _.pos += 1;
            }
            if (_.pos >= _.src.len) {
                return SelectorError.UnclosedBracket;
            }
            val = _.src[vstart.._.pos];
            _.pos += 1; // skip closing quote
        } else {
            if (vc == ']') {
                return SelectorError.UnexpectedToken;
            }
            const vstart = _.pos;
            while (_.pos < _.src.len && !isAsciiSpace(_.src[_.pos]) && _.src[_.pos] != ']') {
                _.pos += 1;
            }
            val = _.src[vstart.._.pos];
            if (val.len == 0) {
                return SelectorError.UnexpectedToken;
            }
        }

        _.skipWs();
        let ci = false;
        if (_.pos < _.src.len && (_.src[_.pos] == 'i' || _.src[_.pos] == 'I')) {
            ci = true;
            _.pos += 1;
            _.skipWs();
        } else if (_.pos < _.src.len && (_.src[_.pos] == 's' || _.src[_.pos] == 'S')) {
            _.pos += 1;
            _.skipWs();
        }

        if (_.pos >= _.src.len) {
            return SelectorError.UnclosedBracket;
        }
        if (_.src[_.pos] != ']') {
            return SelectorError.UnexpectedToken;
        }
        _.pos += 1;

        return AttrSelector{
            name: name,
            op: op,
            value: val,
            case_insensitive: ci,
        };
    }

    fn parsePseudoSelector(*_): PseudoSelector ! SelectorError {
        _.pos += 1; // skip ':'
        if (_.pos >= _.src.len || !isIdentStart(_.src[_.pos])) {
            return SelectorError.UnexpectedToken;
        }
        const name = _.readIdent();

        if (_.pos < _.src.len && _.src[_.pos] == '(') {
            _.pos += 1; // skip '('
            if (std.mem.eql(u8, name, "nth-child")) {
                const start = _.pos;
                while (_.pos < _.src.len && _.src[_.pos] != ')') {
                    _.pos += 1;
                }
                if (_.pos >= _.src.len) {
                    return SelectorError.UnclosedParen;
                }
                const arg = std.mem.trim(u8, _.src[start.._.pos], " \t\r\n");
                _.pos += 1; // skip ')'
                const ab = SelectorParser.parseNthExpr(arg)?;
                return PseudoSelector{
                    kind: .NthChild,
                    nth_a: ab.a,
                    nth_b: ab.b,
                };
            } else if (std.mem.eql(u8, name, "not")) {
                _.skipWs();
                if (_.pos >= _.src.len) {
                    return SelectorError.UnclosedParen;
                }
                if (_.src[_.pos] == ')') {
                    return SelectorError.EmptySelector;
                }
                const inner = _.parseCompoundSelector()?;
                _.skipWs();
                if (_.pos >= _.src.len) {
                    return SelectorError.UnclosedParen;
                }
                if (_.src[_.pos] != ')') {
                    return SelectorError.UnexpectedToken;
                }
                _.pos += 1; // skip ')'
                let ptr = _.alloc.create(CompoundSelector) catch {
                    return SelectorError.OutOfMemory;
                };
                unsafe {
                    ptr.* = inner;
                }
                return PseudoSelector{
                    kind: .Not,
                    not_sel: ptr,
                };
            } else {
                // Consume until ')' to check for UnclosedParen vs UnsupportedPseudo
                while (_.pos < _.src.len && _.src[_.pos] != ')') {
                    _.pos += 1;
                }
                if (_.pos >= _.src.len) {
                    return SelectorError.UnclosedParen;
                }
                return SelectorError.UnsupportedPseudo;
            }
        }

        if (std.mem.eql(u8, name, "first-child")) {
            return PseudoSelector{ kind: .FirstChild };
        }
        if (std.mem.eql(u8, name, "last-child")) {
            return PseudoSelector{ kind: .LastChild };
        }
        if (std.mem.eql(u8, name, "only-child")) {
            return PseudoSelector{ kind: .OnlyChild };
        }
        if (std.mem.eql(u8, name, "empty")) {
            return PseudoSelector{ kind: .Empty };
        }
        return SelectorError.UnsupportedPseudo;
    }

    static fn parseNthExpr(raw: []const u8): NthPair ! SelectorError {
        if (raw.len == 0) {
            return SelectorError.InvalidNthExpr;
        }
        if (std.ascii.eqlIgnoreCase(raw, "odd")) {
            return NthPair{ a: 2, b: 1 };
        }
        if (std.ascii.eqlIgnoreCase(raw, "even")) {
            return NthPair{ a: 2, b: 0 };
        }

        // Find 'n' or 'N'
        let n_pos: usize? = null;
        let i: usize = 0;
        while (i < raw.len) : (i += 1) {
            if (raw[i] == 'n' || raw[i] == 'N') {
                if (n_pos != null) {
                    return SelectorError.InvalidNthExpr;
                }
                n_pos = i;
            }
        }

        if (np := n_pos) {
            const a_str = std.mem.trim(u8, raw[0..np], " \t");
            let a: i32 = 1;
            if (a_str.len == 0 || std.mem.eql(u8, a_str, "+")) {
                a = 1;
            } else if (std.mem.eql(u8, a_str, "-")) {
                a = -1;
            } else {
                a = std.fmt.parseInt(i32, a_str, 10) catch {
                    return SelectorError.InvalidNthExpr;
                };
            }

            const rest = std.mem.trim(u8, raw[(np + 1)..], " \t");
            if (rest.len == 0) {
                return NthPair{ a: a, b: 0 };
            }
            if (rest[0] != '+' && rest[0] != '-') {
                return SelectorError.InvalidNthExpr;
            }
            const sign: i32 = if (rest[0] == '-') -1 else 1;
            const b_str = std.mem.trim(u8, rest[1..], " \t");
            if (b_str.len == 0) {
                return SelectorError.InvalidNthExpr;
            }
            const b_mag = std.fmt.parseInt(i32, b_str, 10) catch {
                return SelectorError.InvalidNthExpr;
            };
            if (b_mag < 0) {
                return SelectorError.InvalidNthExpr;
            }
            return NthPair{ a: a, b: sign * b_mag };
        }

        const b_only = std.fmt.parseInt(i32, raw, 10) catch {
            return SelectorError.InvalidNthExpr;
        };
        return NthPair{ a: 0, b: b_only };
    }
}
