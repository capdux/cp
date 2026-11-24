// Source: Absorbed from /data/dev/bun/src/css/selectors/parser.cp | Synced: 2026-11-23T22:57:42Z
// lib/css/selector.cp
// CSS Selectors Level 4 Parser, Specificity Calculator, SelectorList & Element Matcher
// Absorbed from Bun src/css/selectors/{parser,selector,builder}.cp

const std = @import("std");

fn isIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' ||
        c == '_';
}

pub struct CssSpecificity {
    ids: u32 = 0;
    classes: u32 = 0;
    elements: u32 = 0;

    pub static fn init(ids: u32, classes: u32, elements: u32): CssSpecificity {
        return CssSpecificity { ids: ids, classes: classes, elements: elements };
    }

    /// Returns a 32-bit packed score `(ids << 16) | (classes << 8) | elements`.
    pub fn score(): u32 {
        return (_.ids << 16) | (_.classes << 8) | _.elements;
    }

    pub fn eql(other: CssSpecificity): bool {
        return _.ids == other.ids && _.classes == other.classes && _.elements == other.elements;
    }

    /// Compares two CSS specificities (`-1` if `<`, `0` if `==`, `1` if `>`).
    pub fn compare(other: CssSpecificity): i32 {
        if (_.ids != other.ids) {
            return if (_.ids > other.ids) 1 else -1;
        }
        if (_.classes != other.classes) {
            return if (_.classes > other.classes) 1 else -1;
        }
        if (_.elements != other.elements) {
            return if (_.elements > other.elements) 1 else -1;
        }
        return 0;
    }

    pub fn add(other: CssSpecificity): CssSpecificity {
        return CssSpecificity {
            ids: _.ids + other.ids,
            classes: _.classes + other.classes,
            elements: _.elements + other.elements,
        };
    }
}

fn maxArgSpecificity(arg_list: []const u8, depth: usize): CssSpecificity {
    if (depth >= 8) return CssSpecificity {};
    let best = CssSpecificity {};
    let i: usize = 0;
    let start: usize = 0;
    let paren_depth: usize = 0;
    let bracket_depth: usize = 0;
    let in_quote: u8 = 0;

    while (i <= arg_list.len) {
        let is_split = false;
        if (i == arg_list.len) {
            is_split = true;
        } else {
            const c = arg_list[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < arg_list.len) {
                    i += 2;
                    continue;
                }
                if (c == in_quote) in_quote = 0;
            } else {
                if (c == '"' || c == '\'') in_quote = c;
                else if (c == '(') paren_depth += 1;
                else if (c == ')' && paren_depth > 0) paren_depth -= 1;
                else if (c == '[') bracket_depth += 1;
                else if (c == ']' && bracket_depth > 0) bracket_depth -= 1;
                else if (c == ',' && paren_depth == 0 && bracket_depth == 0) is_split = true;
            }
        }

        if (is_split) {
            const part = std.mem.trim(u8, arg_list[start..i], " \t\r\n");
            if (part.len > 0) {
                const sub = parseSelectorWithDepth(part, depth + 1);
                if (sub.specificity.compare(best) > 0) {
                    best = sub.specificity;
                }
            }
            start = i + 1;
        }
        i += 1;
    }
    return best;
}

fn parseSelectorWithDepth(sel: []const u8, depth: usize): CssSelector {
    const trimmed = std.mem.trim(u8, sel, " \t\r\n");
    let res = CssSelector { raw: trimmed };
    if (trimmed.len == 0) {
        return res;
    }

    let ids: u32 = 0;
    let classes: u32 = 0;
    let elements: u32 = 0;
    let combinators: u32 = 0;

    let i: usize = 0;
    let expect_type_selector: bool = true;

    while (i < trimmed.len) {
        const c = trimmed[i];

        // Whitespace / combinators (' ', '>', '+', '~')
        if (c == ' ' || c == '\t' || c == '>' || c == '+' || c == '~') {
            while (i < trimmed.len && (trimmed[i] == ' ' || trimmed[i] == '\t' || trimmed[i] == '>' || trimmed[i] == '+' || trimmed[i] == '~')) {
                i += 1;
            }
            combinators += 1;
            expect_type_selector = true;
            // Reset subject fields so subject reflects the rightmost compound selector
            res.tag_name = "";
            res.id_name = "";
            res.class_name = "";
            res.class_count = 0;
            res.attr_name = "";
            res.attr_op = "";
            res.attr_val = "";
            continue;
        }

        // Universal selector '*'
        if (c == '*') {
            res.tag_name = "*";
            expect_type_selector = false;
            i += 1;
            continue;
        }

        // Nesting selector '&'
        if (c == '&') {
            expect_type_selector = false;
            i += 1;
            continue;
        }

        // ID selector '#id'
        if (c == '#') {
            i += 1;
            const start = i;
            while (i < trimmed.len && isIdentChar(trimmed[i])) {
                i += 1;
            }
            if (i > start) {
                res.id_name = trimmed[start..i];
                ids += 1;
            }
            expect_type_selector = false;
            continue;
        }

        // Class selector '.class'
        if (c == '.') {
            i += 1;
            const start = i;
            while (i < trimmed.len && isIdentChar(trimmed[i])) {
                i += 1;
            }
            if (i > start) {
                const cls_name = trimmed[start..i];
                if (res.class_name.len == 0) {
                    res.class_name = cls_name;
                }
                if (res.class_count < 8) {
                    res.classes[res.class_count] = cls_name;
                    res.class_count += 1;
                }
                classes += 1;
            }
            expect_type_selector = false;
            continue;
        }

        // Attribute selector '[attr op val]'
        if (c == '[') {
            i += 1;
            const start = i;
            let in_q: u8 = 0;
            while (i < trimmed.len) {
                const ch = trimmed[i];
                if (in_q != 0) {
                    if (ch == '\\' && i + 1 < trimmed.len) {
                        i += 2;
                        continue;
                    }
                    if (ch == in_q) in_q = 0;
                } else {
                    if (ch == '"' || ch == '\'') in_q = ch;
                    else if (ch == ']') break;
                }
                i += 1;
            }
            const attr_expr = trimmed[start..i];
            if (eq_idx := std.mem.indexOfScalar(u8, attr_expr, '=')) {
                let attr_lhs = attr_expr[0..eq_idx];
                if (attr_lhs.len > 0 && (attr_lhs[attr_lhs.len - 1] == '~' || attr_lhs[attr_lhs.len - 1] == '|' || attr_lhs[attr_lhs.len - 1] == '^' || attr_lhs[attr_lhs.len - 1] == '$' || attr_lhs[attr_lhs.len - 1] == '*')) {
                    res.attr_op = attr_lhs[attr_lhs.len - 1 .. attr_lhs.len];
                    attr_lhs = attr_lhs[0 .. attr_lhs.len - 1];
                } else {
                    res.attr_op = "=";
                }
                res.attr_name = std.mem.trim(u8, attr_lhs, " \t");
                res.attr_val = std.mem.trim(u8, attr_expr[eq_idx + 1 ..], " \t\"'");
            } else {
                res.attr_name = std.mem.trim(u8, attr_expr, " \t");
                res.attr_op = "";
                res.attr_val = "";
            }
            if (i < trimmed.len && trimmed[i] == ']') {
                i += 1;
            }
            classes += 1;
            expect_type_selector = false;
            continue;
        }

        // Pseudo-element '::before' or Pseudo-class ':hover' / ':is(...)' / ':not(...)' / ':where(...)' / ':has(...)'
        if (c == ':') {
            if (i + 1 < trimmed.len && trimmed[i + 1] == ':') {
                i += 2;
                const start = i;
                while (i < trimmed.len && isIdentChar(trimmed[i])) {
                    i += 1;
                }
                if (i > start) {
                    res.pseudo_element = trimmed[start..i];
                    elements += 1;
                }
                if (i < trimmed.len && trimmed[i] == '(') {
                    i += 1;
                    let p_depth: usize = 1;
                    while (i < trimmed.len && p_depth > 0) {
                        if (trimmed[i] == '(') p_depth += 1;
                        if (trimmed[i] == ')') p_depth -= 1;
                        i += 1;
                    }
                }
            } else {
                i += 1;
                const start = i;
                while (i < trimmed.len && isIdentChar(trimmed[i])) {
                    i += 1;
                }
                const p_name = trimmed[start..i];
                res.pseudo_class = p_name;
                let fn_args: []const u8 = "";
                let has_args = false;
                if (i < trimmed.len && trimmed[i] == '(') {
                    has_args = true;
                    i += 1;
                    const args_start = i;
                    let p_depth: usize = 1;
                    while (i < trimmed.len && p_depth > 0) {
                        if (trimmed[i] == '(') p_depth += 1;
                        if (trimmed[i] == ')') p_depth -= 1;
                        i += 1;
                    }
                    const args_end = if (p_depth == 0 && i > args_start) (i - 1) else i;
                    fn_args = trimmed[args_start..args_end];
                }

                if (std.ascii.eqlIgnoreCase(p_name, "where")) {
                    // :where() always contributes 0 specificity
                } else if (has_args && (std.ascii.eqlIgnoreCase(p_name, "is") || std.ascii.eqlIgnoreCase(p_name, "not") || std.ascii.eqlIgnoreCase(p_name, "has") || std.ascii.eqlIgnoreCase(p_name, "matches"))) {
                    const arg_spec = maxArgSpecificity(fn_args, depth);
                    ids += arg_spec.ids;
                    classes += arg_spec.classes;
                    elements += arg_spec.elements;
                } else {
                    classes += 1;
                }
            }
            expect_type_selector = false;
            continue;
        }

        // Type / tag selector (e.g. `div`, `button`, `h1`)
        if (isIdentChar(c)) {
            const start = i;
            while (i < trimmed.len && isIdentChar(trimmed[i])) {
                i += 1;
            }
            if (expect_type_selector) {
                res.tag_name = trimmed[start..i];
                elements += 1;
            }
            expect_type_selector = false;
            continue;
        }

        i += 1;
    }

    res.combinator_count = combinators;
    res.specificity = CssSpecificity {
        ids: ids,
        classes: classes,
        elements: elements,
    };
    return res;
}

pub struct CssSelector {
    raw: []const u8 = "";
    tag_name: []const u8 = "";
    id_name: []const u8 = "";
    class_name: []const u8 = "";
    classes: [8][]const u8 = ["", "", "", "", "", "", "", ""];
    class_count: usize = 0;
    attr_name: []const u8 = "";
    attr_op: []const u8 = "";
    attr_val: []const u8 = "";
    pseudo_class: []const u8 = "";
    pseudo_element: []const u8 = "";
    combinator_count: u32 = 0;
    specificity: CssSpecificity = CssSpecificity {};

    /// Parses a CSS selector (compound or complex), extracting the subject's primary tag,
    /// id, classes, attribute, pseudo-class/element, and computing full W3C Level 4 specificity.
    pub static fn parse(sel: []const u8): CssSelector {
        return parseSelectorWithDepth(sel, 0);
    }

    pub fn matchesTag(tag: []const u8): bool {
        if (_.tag_name.len == 0 || std.mem.eql(u8, _.tag_name, "*")) {
            return true;
        }
        return std.ascii.eqlIgnoreCase(_.tag_name, tag);
    }

    /// Checks whether an element's attribute value satisfies this selector's `[attr op val]` constraint.
    pub fn matchesAttribute(attr_name: []const u8, attr_val: []const u8): bool {
        if (_.attr_name.len == 0) {
            return true;
        }
        if (!std.ascii.eqlIgnoreCase(_.attr_name, attr_name)) {
            return false;
        }
        if (_.attr_op.len == 0) {
            return true; // presence check `[attr]`
        }
        if (std.mem.eql(u8, _.attr_op, "=")) {
            return std.mem.eql(u8, attr_val, _.attr_val);
        }
        if (std.mem.eql(u8, _.attr_op, "^")) {
            return std.mem.startsWith(u8, attr_val, _.attr_val);
        }
        if (std.mem.eql(u8, _.attr_op, "$")) {
            return std.mem.endsWith(u8, attr_val, _.attr_val);
        }
        if (std.mem.eql(u8, _.attr_op, "*")) {
            return _.attr_val.len > 0 && std.mem.indexOf(u8, attr_val, _.attr_val) != null;
        }
        if (std.mem.eql(u8, _.attr_op, "~")) {
            let it = std.mem.tokenizeAny(u8, attr_val, " \t\r\n");
            while (tok := it.next()) {
                if (std.mem.eql(u8, tok, _.attr_val)) return true;
            }
            return false;
        }
        if (std.mem.eql(u8, _.attr_op, "|")) {
            return std.mem.eql(u8, attr_val, _.attr_val) ||
                (attr_val.len > _.attr_val.len && std.mem.startsWith(u8, attr_val, _.attr_val) && attr_val[_.attr_val.len] == '-');
        }
        return false;
    }

    /// Checks whether an element with `tag`, `id`, and space-separated `class_list` matches this selector's primary subject.
    pub fn matchesElement(tag: []const u8, id: []const u8, class_list: []const u8): bool {
        if (!_.matchesTag(tag)) {
            return false;
        }
        if (_.id_name.len > 0 && !std.mem.eql(u8, _.id_name, id)) {
            return false;
        }
        if (_.class_name.len > 0) {
            let found_class = false;
            let it = std.mem.tokenizeAny(u8, class_list, " \t\r\n");
            while (cls := it.next()) {
                if (std.mem.eql(u8, cls, _.class_name)) {
                    found_class = true;
                    break;
                }
            }
            if (!found_class) {
                return false;
            }
        }
        return true;
    }

    /// Strict multi-class element matcher requiring ALL classes on the subject selector to be present.
    pub fn matchesElementAllClasses(tag: []const u8, id: []const u8, class_list: []const u8): bool {
        if (!_.matchesTag(tag)) {
            return false;
        }
        if (_.id_name.len > 0 && !std.mem.eql(u8, _.id_name, id)) {
            return false;
        }
        if (_.class_count > 0) {
            let idx: usize = 0;
            while (idx < _.class_count) {
                const target_cls = _.classes[idx];
                let found_class = false;
                let it = std.mem.tokenizeAny(u8, class_list, " \t\r\n");
                while (cls := it.next()) {
                    if (std.mem.eql(u8, cls, target_cls)) {
                        found_class = true;
                        break;
                    }
                }
                if (!found_class) {
                    return false;
                }
                idx += 1;
            }
        }
        return true;
    }
}

pub struct CssSelectorList {
    selectors: [16]CssSelector = undefined;
    count: usize = 0;

    /// Parses a comma-separated CSS selector list (respecting parentheses and brackets).
    pub static fn parse(sel_list: []const u8): CssSelectorList {
        let list = CssSelectorList {
            selectors: std.mem.zeroes([16]CssSelector),
            count: 0,
        };
        let i: usize = 0;
        let start: usize = 0;
        let p_depth: usize = 0;
        let b_depth: usize = 0;
        let in_q: u8 = 0;

        while (i <= sel_list.len && list.count < 16) {
            let is_split = false;
            if (i == sel_list.len) {
                is_split = true;
            } else {
                const c = sel_list[i];
                if (in_q != 0) {
                    if (c == '\\' && i + 1 < sel_list.len) {
                        i += 2;
                        continue;
                    }
                    if (c == in_q) in_q = 0;
                } else {
                    if (c == '"' || c == '\'') in_q = c;
                    else if (c == '(') p_depth += 1;
                    else if (c == ')' && p_depth > 0) p_depth -= 1;
                    else if (c == '[') b_depth += 1;
                    else if (c == ']' && b_depth > 0) b_depth -= 1;
                    else if (c == ',' && p_depth == 0 && b_depth == 0) is_split = true;
                }
            }

            if (is_split) {
                const part = std.mem.trim(u8, sel_list[start..i], " \t\r\n");
                if (part.len > 0) {
                    list.selectors[list.count] = CssSelector.parse(part);
                    list.count += 1;
                }
                start = i + 1;
            }
            i += 1;
        }
        return list;
    }

    pub fn maxSpecificity(): CssSpecificity {
        let best = CssSpecificity {};
        let i: usize = 0;
        while (i < _.count) {
            if (_.selectors[i].specificity.compare(best) > 0) {
                best = _.selectors[i].specificity;
            }
            i += 1;
        }
        return best;
    }

    pub fn matchesElement(tag: []const u8, id: []const u8, class_list: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (_.selectors[i].matchesElement(tag, id, class_list)) {
                return true;
            }
            i += 1;
        }
        return false;
    }
}
