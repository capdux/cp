// Source: Absorbed from /data/dev/bun/src/css/rules/rules.cp | Synced: 2026-11-21T23:51:49Z
// lib/css/engine/rules.cp
// Modern CSS At-Rule AST, Evaluation & Range Lowering:
// SupportsAst, MediaRangeQuery (Level 4 -> legacy), ContainerQuery, CascadeLayer/LayerManager, FontFaceRule, PropertyRule, ScopeRule

const std = @import("std");
import { CssLength, CssUnit } from "../length.cp";
import { Ratio } from "./values.cp";
import { engAppendBytes, engFindMatchingParen } from "./_utils.cp";

fn formatPxCompact(px: f64, out_buf: []u8): []const u8 {
    if (@abs(px) < 0.000001) return std.fmt.bufPrint(out_buf, "0px", .{}) catch "0px";
    const ri: i64 = @intFromFloat(@round(px));
    const rf: f64 = @floatFromInt(ri);
    if (@abs(px - rf) < 0.0001) {
        return std.fmt.bufPrint(out_buf, "{d}px", .{ri}) catch "0px";
    }
    let tmp: [64]u8 = [0; 64];
    const s = std.fmt.bufPrint(tmp[0..64], "{d:.2}", .{px}) catch return "0px";
    let end = s.len;
    while (end > 0 && s[end - 1] == '0') end -= 1;
    if (end > 0 && s[end - 1] == '.') end -= 1;
    return std.fmt.bufPrint(out_buf, "{s}px", .{s[0..end]}) catch "0px";
}

// ============================================================================
// 1. SupportsCondition & SupportsAst (@supports)
// ============================================================================

pub enum SupportsNodeKind {
    Declaration,
    Selector,
    Not,
    And,
    Or,
    Unknown,
}

pub struct SupportsNode {
    kind: SupportsNodeKind = SupportsNodeKind.Unknown;
    prop_name: []const u8 = "";
    prop_val: []const u8 = "";
    text: []const u8 = "";
    children: [8]usize = [0; 8];
    child_count: usize = 0;
}

struct ParseSpan {
    idx: usize = 0;
    next_pos: usize = 0;
}

struct OpMatch {
    start: usize = 0;
    end: usize = 0;
    code: usize = 0;
}

pub struct SupportsAst {
    nodes: [32]SupportsNode = std.mem.zeroes([32]SupportsNode);
    node_count: usize = 0;
    root: usize = 0;

    fn addNode(*_, node: SupportsNode): usize? {
        if (_.node_count >= 32) return null;
        const idx = _.node_count;
        _.nodes[idx] = node;
        _.node_count += 1;
        return idx;
    }

    fn skipWs(s: []const u8, pos_in: usize): usize {
        let p = pos_in;
        while (p < s.len && (s[p] == ' ' || s[p] == '\t' || s[p] == '\r' || s[p] == '\n')) p += 1;
        return p;
    }

    fn matchWord(s: []const u8, pos: usize, word: []const u8): usize? {
        const p = SupportsAst.skipWs(s, pos);
        if (p + word.len > s.len) return null;
        if (!std.ascii.eqlIgnoreCase(s[p .. p + word.len], word)) return null;
        const after = p + word.len;
        if (after < s.len && (std.ascii.isAlphanumeric(s[after]) || s[after] == '-' || s[after] == '_')) return null;
        return after;
    }

    fn parseInParens(*_, s: []const u8, pos_in: usize): ParseSpan? {
        const p = SupportsAst.skipWs(s, pos_in);
        if (p >= s.len) return null;

        // `selector(...)`
        if (p + 9 <= s.len && std.ascii.startsWithIgnoreCase(s[p..], "selector(")) {
            const close = engFindMatchingParen(s, p + 8) ?? return null;
            const sel_text = std.mem.trim(u8, s[p + 9 .. close], " \t\r\n");
            const idx = _.addNode(SupportsNode { kind: SupportsNodeKind.Selector, text: sel_text }) ?? return null;
            return ParseSpan { idx: idx, next_pos: close + 1 };
        }

        if (s[p] != '(') return null;
        const close = engFindMatchingParen(s, p) ?? return null;
        const inner = std.mem.trim(u8, s[p + 1 .. close], " \t\r\n");
        if (inner.len == 0) return null;

        // Nested compound condition inside parens: `(not (...))` or `((a: 1) and (b: 2))`
        if (inner[0] == '(' || SupportsAst.matchWord(inner, 0, "not") != null || std.ascii.startsWithIgnoreCase(inner, "selector(")) {
            const sub = _.parseExpr(inner, 0) ?? return null;
            return ParseSpan { idx: sub.idx, next_pos: close + 1 };
        }

        // Declaration `(prop: val)`
        if (colon := std.mem.indexOfScalar(u8, inner, ':')) {
            const prop = std.mem.trim(u8, inner[0..colon], " \t\r\n");
            const val = std.mem.trim(u8, inner[colon + 1 ..], " \t\r\n");
            const idx = _.addNode(SupportsNode {
                kind: SupportsNodeKind.Declaration,
                prop_name: prop,
                prop_val: val,
            }) ?? return null;
            return ParseSpan { idx: idx, next_pos: close + 1 };
        }

        const idx = _.addNode(SupportsNode { kind: SupportsNodeKind.Unknown, text: inner }) ?? return null;
        return ParseSpan { idx: idx, next_pos: close + 1 };
    }

    fn parseExpr(*_, s: []const u8, pos_in: usize): ParseSpan? {
        let p = SupportsAst.skipWs(s, pos_in);
        if (after_not := SupportsAst.matchWord(s, p, "not")) {
            const inner = _.parseInParens(s, after_not) ?? return null;
            let n = SupportsNode { kind: SupportsNodeKind.Not };
            n.children[0] = inner.idx;
            n.child_count = 1;
            const idx = _.addNode(n) ?? return null;
            return ParseSpan { idx: idx, next_pos: inner.next_pos };
        }

        const first = _.parseInParens(s, p) ?? return null;
        p = first.next_pos;

        let op_kind: SupportsNodeKind? = null;
        let children: [8]usize = [0; 8];
        children[0] = first.idx;
        let count: usize = 1;

        while (count < 8) {
            if (after_and := SupportsAst.matchWord(s, p, "and")) {
                if (op_kind != null && op_kind.? != SupportsNodeKind.And) return null;
                op_kind = SupportsNodeKind.And;
                const next_term = _.parseInParens(s, after_and) ?? return null;
                children[count] = next_term.idx;
                count += 1;
                p = next_term.next_pos;
            } else if (after_or := SupportsAst.matchWord(s, p, "or")) {
                if (op_kind != null && op_kind.? != SupportsNodeKind.Or) return null;
                op_kind = SupportsNodeKind.Or;
                const next_term = _.parseInParens(s, after_or) ?? return null;
                children[count] = next_term.idx;
                count += 1;
                p = next_term.next_pos;
            } else {
                break;
            }
        }

        if (count == 1) return ParseSpan { idx: first.idx, next_pos: p };
        let comb = SupportsNode { kind: op_kind.? };
        comb.children = children;
        comb.child_count = count;
        const idx = _.addNode(comb) ?? return null;
        return ParseSpan { idx: idx, next_pos: p };
    }

    pub static fn parse(raw: []const u8): SupportsAst? {
        let s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.startsWithIgnoreCase(s, "@supports")) {
            s = std.mem.trim(u8, s[9..], " \t\r\n");
        }
        if (s.len == 0) return null;
        let ast = SupportsAst {};
        const r = ast.parseExpr(s, 0) ?? return null;
        ast.root = r.idx;
        return ast;
    }

    pub fn evalNode(idx: usize, decl_fn: fn ([]const u8, []const u8) bool, sel_fn: fn ([]const u8) bool): bool {
        if (idx >= _.node_count) return false;
        const n = _.nodes[idx];
        return switch (n.kind) {
            .Declaration => decl_fn(n.prop_name, n.prop_val),
            .Selector => sel_fn(n.text),
            .Not => !_.evalNode(n.children[0], decl_fn, sel_fn),
            .And => {
                let i: usize = 0;
                while (i < n.child_count) {
                    if (!_.evalNode(n.children[i], decl_fn, sel_fn)) return false;
                    i += 1;
                }
                return true;
            },
            .Or => {
                let i: usize = 0;
                while (i < n.child_count) {
                    if (_.evalNode(n.children[i], decl_fn, sel_fn)) return true;
                    i += 1;
                }
                return false;
            },
            .Unknown => false,
        };
    }

    pub fn eval(decl_fn: fn ([]const u8, []const u8) bool, sel_fn: fn ([]const u8) bool): bool {
        return _.evalNode(_.root, decl_fn, sel_fn);
    }

    fn formatNode(idx: usize, out_buf: []u8, pos_in: usize, parent_kind: SupportsNodeKind): usize {
        if (idx >= _.node_count) return pos_in;
        let pos = pos_in;
        const n = _.nodes[idx];
        switch (n.kind) {
            .Declaration => {
                pos = engAppendBytes(out_buf, pos, "(");
                pos = engAppendBytes(out_buf, pos, n.prop_name);
                pos = engAppendBytes(out_buf, pos, ": ");
                pos = engAppendBytes(out_buf, pos, n.prop_val);
                pos = engAppendBytes(out_buf, pos, ")");
            },
            .Selector => {
                pos = engAppendBytes(out_buf, pos, "selector(");
                pos = engAppendBytes(out_buf, pos, n.text);
                pos = engAppendBytes(out_buf, pos, ")");
            },
            .Not => {
                const need_p = (parent_kind == SupportsNodeKind.And || parent_kind == SupportsNodeKind.Or);
                if (need_p) pos = engAppendBytes(out_buf, pos, "(");
                pos = engAppendBytes(out_buf, pos, "not ");
                pos = _.formatNode(n.children[0], out_buf, pos, SupportsNodeKind.Not);
                if (need_p) pos = engAppendBytes(out_buf, pos, ")");
            },
            .And => {
                const need_p = (parent_kind == SupportsNodeKind.Or || parent_kind == SupportsNodeKind.Not);
                if (need_p) pos = engAppendBytes(out_buf, pos, "(");
                let i: usize = 0;
                while (i < n.child_count) {
                    if (i > 0) pos = engAppendBytes(out_buf, pos, " and ");
                    pos = _.formatNode(n.children[i], out_buf, pos, SupportsNodeKind.And);
                    i += 1;
                }
                if (need_p) pos = engAppendBytes(out_buf, pos, ")");
            },
            .Or => {
                const need_p = (parent_kind == SupportsNodeKind.And || parent_kind == SupportsNodeKind.Not);
                if (need_p) pos = engAppendBytes(out_buf, pos, "(");
                let i: usize = 0;
                while (i < n.child_count) {
                    if (i > 0) pos = engAppendBytes(out_buf, pos, " or ");
                    pos = _.formatNode(n.children[i], out_buf, pos, SupportsNodeKind.Or);
                    i += 1;
                }
                if (need_p) pos = engAppendBytes(out_buf, pos, ")");
            },
            .Unknown => {
                pos = engAppendBytes(out_buf, pos, "(");
                pos = engAppendBytes(out_buf, pos, n.text);
                pos = engAppendBytes(out_buf, pos, ")");
            },
        }
        return pos;
    }

    pub fn format(out_buf: []u8): []const u8 {
        const end = _.formatNode(_.root, out_buf, 0, SupportsNodeKind.Unknown);
        return out_buf[0..end];
    }
}

// ============================================================================
// 2. MediaRangeQuery (Media Queries Level 4 Range & Interval -> Legacy Min/Max)
// ============================================================================

pub enum MediaComparison {
    Eq,
    Gt,
    Gte,
    Lt,
    Lte,

    fn opposite(): MediaComparison {
        return switch (_) {
            .Gt => MediaComparison.Lt,
            .Gte => MediaComparison.Lte,
            .Lt => MediaComparison.Gt,
            .Lte => MediaComparison.Gte,
            .Eq => MediaComparison.Eq,
        };
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Eq => "=",
            .Gt => ">",
            .Gte => ">=",
            .Lt => "<",
            .Lte => "<=",
        };
    }
}

fn findComparisonOp(s: []const u8, start: usize): OpMatch? {
    let i = start;
    while (i < s.len) {
        const c = s[i];
        if (c == '>') {
            if (i + 1 < s.len && s[i + 1] == '=') return OpMatch { start: i, end: i + 2, code: 2 };
            return OpMatch { start: i, end: i + 1, code: 1 };
        }
        if (c == '<') {
            if (i + 1 < s.len && s[i + 1] == '=') return OpMatch { start: i, end: i + 2, code: 4 };
            return OpMatch { start: i, end: i + 1, code: 3 };
        }
        if (c == '=') return OpMatch { start: i, end: i + 1, code: 0 };
        i += 1;
    }
    return null;
}

fn codeToComparison(code: usize): MediaComparison {
    return switch (code) {
        0 => MediaComparison.Eq,
        1 => MediaComparison.Gt,
        2 => MediaComparison.Gte,
        3 => MediaComparison.Lt,
        else => MediaComparison.Lte,
    };
}

pub struct MediaRangeQuery {
    feature: []const u8 = "width";
    has_start: bool = false;
    start_op: MediaComparison = MediaComparison.Gte;
    start_val: []const u8 = "";
    has_end: bool = false;
    end_op: MediaComparison = MediaComparison.Lte;
    end_val: []const u8 = "";
    is_interval: bool = false;

    pub static fn parse(raw: []const u8): MediaRangeQuery? {
        let s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len >= 2 && s[0] == '(' && s[s.len - 1] == ')') {
            s = std.mem.trim(u8, s[1 .. s.len - 1], " \t\r\n");
        }
        if (s.len == 0) return null;

        // Legacy min-*/max-* support
        if (colon := std.mem.indexOfScalar(u8, s, ':')) {
            const fname = std.mem.trim(u8, s[0..colon], " \t");
            const fval = std.mem.trim(u8, s[colon + 1 ..], " \t");
            if (std.ascii.startsWithIgnoreCase(fname, "min-")) {
                return MediaRangeQuery { feature: fname[4..], has_start: true, start_op: MediaComparison.Gte, start_val: fval };
            }
            if (std.ascii.startsWithIgnoreCase(fname, "max-")) {
                return MediaRangeQuery { feature: fname[4..], has_end: true, end_op: MediaComparison.Lte, end_val: fval };
            }
            return MediaRangeQuery { feature: fname, has_start: true, start_op: MediaComparison.Eq, start_val: fval };
        }

        const op1 = findComparisonOp(s, 0) ?? return null;
        const lhs = std.mem.trim(u8, s[0..op1.start], " \t");
        const cmp1 = codeToComparison(op1.code);

        if (op2 := findComparisonOp(s, op1.end)) {
            // Interval syntax: `400px <= width < 900px`
            const mid = std.mem.trim(u8, s[op1.end..op2.start], " \t");
            const rhs = std.mem.trim(u8, s[op2.end..], " \t");
            const cmp2 = codeToComparison(op2.code);
            // Normalize so `start_op` is the lower bound on `mid` (`lhs <= mid` -> `mid >= lhs`)
            return MediaRangeQuery {
                feature: mid,
                has_start: true,
                start_op: cmp1.opposite(),
                start_val: lhs,
                has_end: true,
                end_op: cmp2,
                end_val: rhs,
                is_interval: true,
            };
        }

        const rhs = std.mem.trim(u8, s[op1.end..], " \t");
        // Determine whether lhs or rhs is the feature name
        const is_lhs_feature = std.ascii.isAlphabetic(lhs[0]);
        if (is_lhs_feature) {
            if (cmp1 == MediaComparison.Lt || cmp1 == MediaComparison.Lte) {
                return MediaRangeQuery { feature: lhs, has_end: true, end_op: cmp1, end_val: rhs };
            }
            return MediaRangeQuery { feature: lhs, has_start: true, start_op: cmp1, start_val: rhs };
        } else {
            const flipped = cmp1.opposite();
            if (flipped == MediaComparison.Lt || flipped == MediaComparison.Lte) {
                return MediaRangeQuery { feature: rhs, has_end: true, end_op: flipped, end_val: lhs };
            }
            return MediaRangeQuery { feature: rhs, has_start: true, start_op: flipped, start_val: lhs };
        }
    }

    fn writeBoundLegacy(feature: []const u8, op: MediaComparison, val_str: []const u8, out_buf: []u8, pos_in: usize): usize {
        let pos = pos_in;
        pos = engAppendBytes(out_buf, pos, "(");
        let adj_buf: [64]u8 = [0; 64];
        let out_val = val_str;

        if (op == MediaComparison.Gt || op == MediaComparison.Gte) {
            pos = engAppendBytes(out_buf, pos, "min-");
            if (op == MediaComparison.Gt) {
                if (l := CssLength.parse(val_str)) {
                    const px = l.toPx(16.0, 1920.0, 1080.0) + 0.02;
                    out_val = formatPxCompact(px, adj_buf[0..64]);
                }
            }
        } else if (op == MediaComparison.Lt || op == MediaComparison.Lte) {
            pos = engAppendBytes(out_buf, pos, "max-");
            if (op == MediaComparison.Lt) {
                if (l := CssLength.parse(val_str)) {
                    const px = l.toPx(16.0, 1920.0, 1080.0) - 0.02;
                    out_val = formatPxCompact(px, adj_buf[0..64]);
                }
            }
        }
        pos = engAppendBytes(out_buf, pos, feature);
        pos = engAppendBytes(out_buf, pos, ": ");
        pos = engAppendBytes(out_buf, pos, out_val);
        pos = engAppendBytes(out_buf, pos, ")");
        return pos;
    }

    /// Lowers Level 4 range/interval syntax into legacy `(min-*: ...) and (max-*: ...)`.
    pub fn lowerToLegacy(out_buf: []u8): []const u8 {
        let pos: usize = 0;
        if (_.has_start) {
            pos = MediaRangeQuery.writeBoundLegacy(_.feature, _.start_op, _.start_val, out_buf, pos);
        }
        if (_.has_end) {
            if (_.has_start) pos = engAppendBytes(out_buf, pos, " and ");
            pos = MediaRangeQuery.writeBoundLegacy(_.feature, _.end_op, _.end_val, out_buf, pos);
        }
        return out_buf[0..pos];
    }

    fn evalOp(actual: f64, op: MediaComparison, target: f64): bool {
        return switch (op) {
            .Eq => @abs(actual - target) < 0.0001,
            .Gt => actual > target,
            .Gte => actual >= target,
            .Lt => actual < target,
            .Lte => actual <= target,
        };
    }

    pub fn matches(viewport_w: f64, viewport_h: f64): bool {
        let actual: f64 = viewport_w;
        if (std.ascii.eqlIgnoreCase(_.feature, "height") || std.ascii.eqlIgnoreCase(_.feature, "block-size")) {
            actual = viewport_h;
        } else if (std.ascii.eqlIgnoreCase(_.feature, "aspect-ratio")) {
            if (viewport_h <= 0.0) return false;
            actual = viewport_w / viewport_h;
            if (_.has_start) {
                const r = Ratio.parse(_.start_val) ?? return false;
                if (!MediaRangeQuery.evalOp(actual, _.start_op, r.value())) return false;
            }
            if (_.has_end) {
                const r = Ratio.parse(_.end_val) ?? return false;
                if (!MediaRangeQuery.evalOp(actual, _.end_op, r.value())) return false;
            }
            return true;
        }
        if (_.has_start) {
            const l = CssLength.parse(_.start_val) ?? return false;
            if (!MediaRangeQuery.evalOp(actual, _.start_op, l.toPx(16.0, viewport_w, viewport_h))) return false;
        }
        if (_.has_end) {
            const l = CssLength.parse(_.end_val) ?? return false;
            if (!MediaRangeQuery.evalOp(actual, _.end_op, l.toPx(16.0, viewport_w, viewport_h))) return false;
        }
        return true;
    }
}

// ============================================================================
// 3. ContainerQuery (@container [name] <condition>)
// ============================================================================

pub struct ContainerQuery {
    name: []const u8 = "";
    raw_condition: []const u8 = "";
    clauses: [6]MediaRangeQuery = std.mem.zeroes([6]MediaRangeQuery);
    clause_count: usize = 0;
    is_or: bool = false;
    negated: bool = false;
    style_prop: []const u8 = "";
    style_val: []const u8 = "";

    pub static fn parse(raw: []const u8): ContainerQuery? {
        let s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.startsWithIgnoreCase(s, "@container")) {
            s = std.mem.trim(u8, s[10..], " \t\r\n");
        }
        if (s.len == 0) return null;

        let cq = ContainerQuery {};
        // Optional container name before `(` or `not ` or `style(`
        if (s[0] != '(' && !std.ascii.startsWithIgnoreCase(s, "not ") && !std.ascii.startsWithIgnoreCase(s, "style(")) {
            let sp: usize = 0;
            while (sp < s.len && s[sp] != ' ' && s[sp] != '\t' && s[sp] != '(') sp += 1;
            cq.name = s[0..sp];
            s = std.mem.trim(u8, s[sp..], " \t\r\n");
        }
        cq.raw_condition = s;

        if (std.ascii.startsWithIgnoreCase(s, "not ")) {
            cq.negated = true;
            s = std.mem.trim(u8, s[4..], " \t\r\n");
        }

        if (std.mem.indexOf(u8, s, " or ") != null || std.mem.indexOf(u8, s, " OR ") != null) {
            cq.is_or = true;
        }

        let pos: usize = 0;
        while (pos < s.len) {
            if (pos + 6 <= s.len && std.ascii.startsWithIgnoreCase(s[pos..], "style(")) {
                const close = engFindMatchingParen(s, pos + 5) ?? break;
                const inner = std.mem.trim(u8, s[pos + 6 .. close], " \t");
                if (col := std.mem.indexOfScalar(u8, inner, ':')) {
                    cq.style_prop = std.mem.trim(u8, inner[0..col], " \t");
                    cq.style_val = std.mem.trim(u8, inner[col + 1 ..], " \t");
                }
                pos = close + 1;
                continue;
            }
            if (s[pos] == '(') {
                const close = engFindMatchingParen(s, pos) ?? break;
                const chunk = s[pos .. close + 1];
                if (mrq := MediaRangeQuery.parse(chunk)) {
                    if (cq.clause_count < 6) {
                        cq.clauses[cq.clause_count] = mrq;
                        cq.clause_count += 1;
                    }
                }
                pos = close + 1;
            } else {
                pos += 1;
            }
        }
        return cq;
    }

    pub fn matches(target_name: []const u8, width: f64, height: f64): bool {
        if (_.name.len > 0 && target_name.len > 0 && !std.mem.eql(u8, _.name, target_name)) {
            return false;
        }
        if (_.name.len > 0 && target_name.len == 0) return false;
        if (_.clause_count == 0) return !_.negated;

        let ok = if (_.is_or) false else true;
        let i: usize = 0;
        while (i < _.clause_count) {
            const m = _.clauses[i].matches(width, height);
            if (_.is_or) {
                if (m) { ok = true; break; }
            } else {
                if (!m) { ok = false; break; }
            }
            i += 1;
        }
        return if (_.negated) !ok else ok;
    }

    pub fn format(out_buf: []u8, lower_ranges: bool): []const u8 {
        let pos: usize = 0;
        pos = engAppendBytes(out_buf, pos, "@container ");
        if (_.name.len > 0) {
            pos = engAppendBytes(out_buf, pos, _.name);
            pos = engAppendBytes(out_buf, pos, " ");
        }
        if (lower_ranges && _.clause_count > 0) {
            if (_.negated) pos = engAppendBytes(out_buf, pos, "not ");
            const joiner: []const u8 = if (_.is_or) " or " else " and ";
            let lb: [128]u8 = [0; 128];
            let i: usize = 0;
            while (i < _.clause_count) {
                if (i > 0) pos = engAppendBytes(out_buf, pos, joiner);
                pos = engAppendBytes(out_buf, pos, _.clauses[i].lowerToLegacy(lb[0..128]));
                i += 1;
            }
            return out_buf[0..pos];
        }
        pos = engAppendBytes(out_buf, pos, _.raw_condition);
        return out_buf[0..pos];
    }
}

// ============================================================================
// 4. CascadeLayer & LayerManager (@layer)
// ============================================================================

pub struct CascadeLayer {
    name: []const u8 = "";
    order: usize = 0;
    is_anonymous: bool = false;
}

pub struct LayerManager {
    layers: [32]CascadeLayer = std.mem.zeroes([32]CascadeLayer);
    count: usize = 0;
    anon_seq: usize = 0;

    pub static fn init(): LayerManager {
        return LayerManager {};
    }

    pub fn registerLayer(*_, name_raw: []const u8): usize {
        const name = std.mem.trim(u8, name_raw, " \t\r\n");
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.layers[i].name, name)) return _.layers[i].order;
            i += 1;
        }
        if (_.count < 32) {
            const ord = _.count + 1;
            _.layers[_.count] = CascadeLayer { name: name, order: ord, is_anonymous: false };
            _.count += 1;
            return ord;
        }
        return 0;
    }

    pub fn registerStatement(*_, stmt_raw: []const u8): usize {
        let s = std.mem.trim(u8, stmt_raw, " \t\r\n;");
        if (std.ascii.startsWithIgnoreCase(s, "@layer")) {
            s = std.mem.trim(u8, s[6..], " \t\r\n");
        }
        let added: usize = 0;
        let it = std.mem.splitScalar(u8, s, ',');
        while (part := it.next()) {
            const trimmed = std.mem.trim(u8, part, " \t\r\n");
            if (trimmed.len > 0) {
                * = _.registerLayer(trimmed);
                added += 1;
            }
        }
        return added;
    }

    pub fn registerAnonymous(*_): usize {
        _.anon_seq += 1;
        if (_.count < 32) {
            const ord = _.count + 1;
            _.layers[_.count] = CascadeLayer { name: "<anonymous>", order: ord, is_anonymous: true };
            _.count += 1;
            return ord;
        }
        return 0;
    }

    pub fn getOrder(name: []const u8): usize? {
        if (name.len == 0) return null;
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.layers[i].name, name)) return _.layers[i].order;
            i += 1;
        }
        return null;
    }

    /// Compares cascade layer priority: unlayered (`""`) always beats any named layer.
    /// Returns -1 if `a < b`, 0 if equal, 1 if `a > b`.
    pub fn compareWeight(layer_a: []const u8, layer_b: []const u8): i32 {
        if (layer_a.len == 0 && layer_b.len == 0) return 0;
        if (layer_a.len == 0) return 1;
        if (layer_b.len == 0) return -1;
        const oa = _.getOrder(layer_a) ?? 0;
        const ob = _.getOrder(layer_b) ?? 0;
        if (oa < ob) return -1;
        if (oa > ob) return 1;
        return 0;
    }
}

// ============================================================================
// 5. FontFaceRule, PropertyRule, ScopeRule
// ============================================================================

pub struct FontFaceRule {
    font_family: []const u8 = "";
    src: []const u8 = "";
    font_weight: []const u8 = "";
    font_style: []const u8 = "";
    font_display: []const u8 = "";
    unicode_range: []const u8 = "";

    pub static fn parse(raw_rule: []const u8): FontFaceRule? {
        const open = std.mem.indexOfScalar(u8, raw_rule, '{') ?? return null;
        const close = std.mem.lastIndexOfScalar(u8, raw_rule, '}') ?? return null;
        if (close <= open) return null;
        const body = raw_rule[open + 1 .. close];

        let ff = FontFaceRule {};
        let it = std.mem.splitScalar(u8, body, ';');
        while (decl_raw := it.next()) {
            const decl = std.mem.trim(u8, decl_raw, " \t\r\n");
            if (decl.len == 0) continue;
            const col = std.mem.indexOfScalar(u8, decl, ':') ?? continue;
            const k = std.mem.trim(u8, decl[0..col], " \t");
            const v = std.mem.trim(u8, decl[col + 1 ..], " \t");
            if (std.ascii.eqlIgnoreCase(k, "font-family")) ff.font_family = v
            else if (std.ascii.eqlIgnoreCase(k, "src")) ff.src = v
            else if (std.ascii.eqlIgnoreCase(k, "font-weight")) ff.font_weight = v
            else if (std.ascii.eqlIgnoreCase(k, "font-style")) ff.font_style = v
            else if (std.ascii.eqlIgnoreCase(k, "font-display")) ff.font_display = v
            else if (std.ascii.eqlIgnoreCase(k, "unicode-range")) ff.unicode_range = v;
        }
        return ff;
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        let pos: usize = 0;
        pos = engAppendBytes(out_buf, pos, if (minify) "@font-face{" else "@font-face {\n");
        const pfx: []const u8 = if (minify) "" else "  ";
        const sep: []const u8 = if (minify) ";" else ";\n";
        const kv_sep: []const u8 = if (minify) ":" else ": ";

        let first = true;
        if (_.font_family.len > 0) {
            pos = engAppendBytes(out_buf, pos, pfx);
            pos = engAppendBytes(out_buf, pos, "font-family");
            pos = engAppendBytes(out_buf, pos, kv_sep);
            pos = engAppendBytes(out_buf, pos, _.font_family);
            first = false;
        }
        if (_.src.len > 0) {
            if (!first) pos = engAppendBytes(out_buf, pos, sep);
            pos = engAppendBytes(out_buf, pos, pfx);
            pos = engAppendBytes(out_buf, pos, "src");
            pos = engAppendBytes(out_buf, pos, kv_sep);
            pos = engAppendBytes(out_buf, pos, _.src);
            first = false;
        }
        if (_.font_weight.len > 0) {
            if (!first) pos = engAppendBytes(out_buf, pos, sep);
            pos = engAppendBytes(out_buf, pos, pfx);
            pos = engAppendBytes(out_buf, pos, "font-weight");
            pos = engAppendBytes(out_buf, pos, kv_sep);
            pos = engAppendBytes(out_buf, pos, _.font_weight);
            first = false;
        }
        if (_.font_style.len > 0) {
            if (!first) pos = engAppendBytes(out_buf, pos, sep);
            pos = engAppendBytes(out_buf, pos, pfx);
            pos = engAppendBytes(out_buf, pos, "font-style");
            pos = engAppendBytes(out_buf, pos, kv_sep);
            pos = engAppendBytes(out_buf, pos, _.font_style);
            first = false;
        }
        if (_.font_display.len > 0) {
            if (!first) pos = engAppendBytes(out_buf, pos, sep);
            pos = engAppendBytes(out_buf, pos, pfx);
            pos = engAppendBytes(out_buf, pos, "font-display");
            pos = engAppendBytes(out_buf, pos, kv_sep);
            pos = engAppendBytes(out_buf, pos, _.font_display);
            first = false;
        }
        if (_.unicode_range.len > 0) {
            if (!first) pos = engAppendBytes(out_buf, pos, sep);
            pos = engAppendBytes(out_buf, pos, pfx);
            pos = engAppendBytes(out_buf, pos, "unicode-range");
            pos = engAppendBytes(out_buf, pos, kv_sep);
            pos = engAppendBytes(out_buf, pos, _.unicode_range);
        }
        pos = engAppendBytes(out_buf, pos, if (minify) "}" else ";\n}");
        return out_buf[0..pos];
    }
}

pub struct PropertyRule {
    name: []const u8 = "";
    syntax: []const u8 = "\"*\"";
    inherits: bool = false;
    has_inherits: bool = false;
    initial_value: []const u8 = "";

    pub static fn parse(raw_rule: []const u8): PropertyRule? {
        const open = std.mem.indexOfScalar(u8, raw_rule, '{') ?? return null;
        const close = std.mem.lastIndexOfScalar(u8, raw_rule, '}') ?? return null;
        if (close <= open) return null;

        let prelude = std.mem.trim(u8, raw_rule[0..open], " \t\r\n");
        if (std.ascii.startsWithIgnoreCase(prelude, "@property")) {
            prelude = std.mem.trim(u8, prelude[9..], " \t\r\n");
        }
        if (!std.mem.startsWith(u8, prelude, "--")) return null;

        let rule = PropertyRule { name: prelude };
        let has_syntax = false;
        const body = raw_rule[open + 1 .. close];
        let it = std.mem.splitScalar(u8, body, ';');
        while (decl_raw := it.next()) {
            const decl = std.mem.trim(u8, decl_raw, " \t\r\n");
            if (decl.len == 0) continue;
            const col = std.mem.indexOfScalar(u8, decl, ':') ?? continue;
            const k = std.mem.trim(u8, decl[0..col], " \t");
            const v = std.mem.trim(u8, decl[col + 1 ..], " \t");
            if (std.ascii.eqlIgnoreCase(k, "syntax")) {
                rule.syntax = v;
                has_syntax = true;
            } else if (std.ascii.eqlIgnoreCase(k, "inherits")) {
                if (std.ascii.eqlIgnoreCase(v, "true")) {
                    rule.inherits = true;
                    rule.has_inherits = true;
                } else if (std.ascii.eqlIgnoreCase(v, "false")) {
                    rule.inherits = false;
                    rule.has_inherits = true;
                } else {
                    return null;
                }
            } else if (std.ascii.eqlIgnoreCase(k, "initial-value")) {
                rule.initial_value = v;
            }
        }
        if (!has_syntax || !rule.has_inherits) return null;
        const is_universal = std.mem.eql(u8, rule.syntax, "\"*\"") || std.mem.eql(u8, rule.syntax, "'*'") || std.mem.eql(u8, rule.syntax, "*");
        if (!is_universal && rule.initial_value.len == 0) return null;
        return rule;
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        const inh: []const u8 = if (_.inherits) "true" else "false";
        if (_.initial_value.len > 0) {
            if (minify) {
                return std.fmt.bufPrint(out_buf, "@property {s}{{syntax:{s};inherits:{s};initial-value:{s}}}", .{ _.name, _.syntax, inh, _.initial_value }) catch "";
            }
            return std.fmt.bufPrint(out_buf, "@property {s} {{\n  syntax: {s};\n  inherits: {s};\n  initial-value: {s};\n}}", .{ _.name, _.syntax, inh, _.initial_value }) catch "";
        }
        if (minify) {
            return std.fmt.bufPrint(out_buf, "@property {s}{{syntax:{s};inherits:{s}}}", .{ _.name, _.syntax, inh }) catch "";
        }
        return std.fmt.bufPrint(out_buf, "@property {s} {{\n  syntax: {s};\n  inherits: {s};\n}}", .{ _.name, _.syntax, inh }) catch "";
    }
}

pub struct ScopeRule {
    scope_start: []const u8 = "";
    scope_end: []const u8 = "";
    body: []const u8 = "";

    pub static fn parse(raw_rule: []const u8): ScopeRule? {
        const open = std.mem.indexOfScalar(u8, raw_rule, '{') ?? return null;
        const close = std.mem.lastIndexOfScalar(u8, raw_rule, '}') ?? return null;
        if (close <= open) return null;

        let prelude = std.mem.trim(u8, raw_rule[0..open], " \t\r\n");
        if (!std.ascii.startsWithIgnoreCase(prelude, "@scope")) return null;
        prelude = std.mem.trim(u8, prelude[6..], " \t\r\n");

        let sr = ScopeRule { body: std.mem.trim(u8, raw_rule[open + 1 .. close], " \t\r\n") };
        let pos: usize = 0;
        if (pos < prelude.len && prelude[pos] == '(') {
            const p_end = engFindMatchingParen(prelude, pos) ?? return null;
            sr.scope_start = std.mem.trim(u8, prelude[pos + 1 .. p_end], " \t\r\n");
            pos = p_end + 1;
        }
        const rest = std.mem.trim(u8, prelude[pos..], " \t\r\n");
        if (std.ascii.startsWithIgnoreCase(rest, "to")) {
            const after_to = std.mem.trim(u8, rest[2..], " \t\r\n");
            if (after_to.len > 0 && after_to[0] == '(') {
                const p_end = engFindMatchingParen(after_to, 0) ?? return null;
                sr.scope_end = std.mem.trim(u8, after_to[1..p_end], " \t\r\n");
            }
        }
        return sr;
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        let pos: usize = 0;
        pos = engAppendBytes(out_buf, pos, "@scope");
        if (_.scope_start.len > 0) {
            pos = engAppendBytes(out_buf, pos, if (minify) "(" else " (");
            pos = engAppendBytes(out_buf, pos, _.scope_start);
            pos = engAppendBytes(out_buf, pos, ")");
        }
        if (_.scope_end.len > 0) {
            pos = engAppendBytes(out_buf, pos, if (minify) " to (" else " to (");
            pos = engAppendBytes(out_buf, pos, _.scope_end);
            pos = engAppendBytes(out_buf, pos, ")");
        }
        pos = engAppendBytes(out_buf, pos, if (minify) "{" else " {\n  ");
        pos = engAppendBytes(out_buf, pos, _.body);
        pos = engAppendBytes(out_buf, pos, if (minify) "}" else "\n}");
        return out_buf[0..pos];
    }
}
