// lib/css/selector.cp
// CSS Selectors Level 4 Parser, Specificity Calculator & Element Matcher
// Absorbed and unified from Bun src/css/selectors/parser.cp, selector.cp, builder.cp

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
}

pub struct CssSelector {
    raw: []const u8 = "";
    tag_name: []const u8 = "";
    id_name: []const u8 = "";
    class_name: []const u8 = "";
    attr_name: []const u8 = "";
    attr_val: []const u8 = "";
    pseudo_class: []const u8 = "";
    pseudo_element: []const u8 = "";
    combinator_count: u32 = 0;
    specificity: CssSpecificity = CssSpecificity {};

    /// Parses a CSS selector (compound or complex, e.g., `ul#nav > li.active.selected[data-role="tab"]:hover::after`),
    /// extracting the subject's primary tag, id, class, attribute, pseudo-class/element, and computing full W3C specificity.
    pub static fn parse(sel: []const u8): CssSelector {
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
                // Reset subject tag/id/class so subject reflects the rightmost compound selector
                res.tag_name = "";
                res.id_name = "";
                res.class_name = "";
                continue;
            }

            // Universal selector '*'
            if (c == '*') {
                res.tag_name = "*";
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
                    if (res.class_name.len == 0) {
                        res.class_name = trimmed[start..i];
                    }
                    classes += 1;
                }
                expect_type_selector = false;
                continue;
            }

            // Attribute selector '[attr=val]'
            if (c == '[') {
                i += 1;
                const start = i;
                while (i < trimmed.len && trimmed[i] != ']') {
                    i += 1;
                }
                const attr_expr = trimmed[start..i];
                if (eq_idx := std.mem.indexOfScalar(u8, attr_expr, '=')) {
                    let attr_lhs = attr_expr[0..eq_idx];
                    if (attr_lhs.len > 0 && (attr_lhs[attr_lhs.len - 1] == '~' || attr_lhs[attr_lhs.len - 1] == '|' || attr_lhs[attr_lhs.len - 1] == '^' || attr_lhs[attr_lhs.len - 1] == '$' || attr_lhs[attr_lhs.len - 1] == '*')) {
                        attr_lhs = attr_lhs[0 .. attr_lhs.len - 1];
                    }
                    res.attr_name = std.mem.trim(u8, attr_lhs, " \t");
                    res.attr_val = std.mem.trim(u8, attr_expr[eq_idx + 1 ..], " \t\"'");
                } else {
                    res.attr_name = std.mem.trim(u8, attr_expr, " \t");
                }
                if (i < trimmed.len && trimmed[i] == ']') {
                    i += 1;
                }
                classes += 1;
                expect_type_selector = false;
                continue;
            }

            // Pseudo-element '::before' / '::after' or Pseudo-class ':hover' / ':not(...)'
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
                } else {
                    i += 1;
                    const start = i;
                    while (i < trimmed.len && isIdentChar(trimmed[i])) {
                        i += 1;
                    }
                    const p_name = trimmed[start..i];
                    res.pseudo_class = p_name;
                    if (i < trimmed.len && trimmed[i] == '(') {
                        i += 1;
                        let depth: usize = 1;
                        while (i < trimmed.len && depth > 0) {
                            if (trimmed[i] == '(') depth += 1;
                            if (trimmed[i] == ')') depth -= 1;
                            i += 1;
                        }
                    }
                    // :where() has 0 specificity; standard pseudo-classes count as 1 class
                    if (!std.ascii.eqlIgnoreCase(p_name, "where")) {
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

    pub fn matchesTag(tag: []const u8): bool {
        if (_.tag_name.len == 0 || std.mem.eql(u8, _.tag_name, "*")) {
            return true;
        }
        return std.ascii.eqlIgnoreCase(_.tag_name, tag);
    }

    /// Checks whether an element with `tag`, `id`, and space-separated `class_list` matches this selector's subject.
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
}
