// @cap/css-select — Zero-heap CSS selector parser, W3C Selectors Level 4
// specificity calculator, and element matching engine.
// Clean-room MIT CP implementation modeled on `css-select` + `css-what`.
// Reuses `eqAscii` from `@cap/unicase` for case-insensitive HTML tag/attribute matching.

import { isSpace, isAlpha, isDigit, strEq, trim } from "cap:ascii";
import { eqAscii } from "../../unicase/src/index.cp";

fn csssIsCombinator(c: u8): bool {
    return c == 62 || c == 43 || c == 126; // '>', '+', '~'
}

fn csssIsIdentStart(c: u8): bool {
    return isAlpha(c)
        || c == 95
        || c == 45
        || c >= 128;
}

fn csssIsIdentChar(c: u8): bool {
    return csssIsIdentStart(c) || isDigit(c);
}

fn csssEqMaybeCi(a: string, b: string, ci: bool): bool {
    if (ci) { return eqAscii(a, b); }
    return strEq(a, b);
}

fn csssSkipIdent(s: string, pos: usize): usize {
    let i = pos;
    while (i < s.len && csssIsIdentChar(s[i])) {
        i = i + 1;
    }
    return i;
}

// Finds index of matching ']' starting at '['; returns s.len if unclosed.
fn csssFindBracketEnd(s: string, pos: usize): usize {
    let i = pos + 1;
    let quote: u8 = 0;
    while (i < s.len) {
        let c = s[i];
        if (quote != 0) {
            if (c == 92 && i + 1 < s.len) {
                i = i + 2;
                continue;
            }
            if (c == quote) { quote = 0; }
        } else {
            if (c == 34 || c == 39) {
                quote = c;
            } else if (c == 93) {
                return i;
            }
        }
        i = i + 1;
    }
    return s.len;
}

// Skips balanced '(...)' starting at '('; returns index immediately after ')'.
fn csssSkipParen(s: string, pos: usize): usize {
    if (pos >= s.len || s[pos] != 40) { return pos; }
    let i = pos + 1;
    let depth: usize = 1;
    let quote: u8 = 0;
    while (i < s.len && depth > 0) {
        let c = s[i];
        if (quote != 0) {
            if (c == 92 && i + 1 < s.len) {
                i = i + 2;
                continue;
            }
            if (c == quote) { quote = 0; }
        } else {
            if (c == 34 || c == 39) {
                quote = c;
            } else if (c == 40) {
                depth = depth + 1;
            } else if (c == 41) {
                depth = depth - 1;
            }
        }
        i = i + 1;
    }
    return i;
}

// Whitespace-separated token membership check (used for .class and [attr~=val]).
fn csssHasToken(list: string, target: string, ci: bool): bool {
    if (target.len == 0) { return false; }
    let i: usize = 0;
    while (i < list.len) {
        while (i < list.len && isSpace(list[i])) {
            i = i + 1;
        }
        let start = i;
        while (i < list.len && !isSpace(list[i])) {
            i = i + 1;
        }
        if (i > start) {
            if (csssEqMaybeCi(list[start..i], target, ci)) {
                return true;
            }
        }
    }
    return false;
}

fn csssContains(haystack: string, needle: string, ci: bool): bool {
    if (needle.len == 0 || needle.len > haystack.len) { return false; }
    let limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        if (csssEqMaybeCi(haystack[i..i + needle.len], needle, ci)) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

fn csssIsLegacyPseudoElement(name: string): bool {
    return eqAscii(name, "before")
        || eqAscii(name, "after")
        || eqAscii(name, "first-line")
        || eqAscii(name, "first-letter");
}

/// W3C Selectors Level 4 specificity triple `(ids, classes, tags)`.
pub struct Specificity {
    pub ids: u32,
    pub classes: u32,
    pub tags: u32,

    pub static fn new(ids: u32, classes: u32, tags: u32): Specificity {
        return Specificity { ids: ids, classes: classes, tags: tags };
    }

    /// Lexicographical comparison: returns -1 if lower, 0 if equal, 1 if higher.
    pub fn cmp(other: Specificity): i32 {
        if (_.ids < other.ids) { return -1; }
        if (_.ids > other.ids) { return 1; }
        if (_.classes < other.classes) { return -1; }
        if (_.classes > other.classes) { return 1; }
        if (_.tags < other.tags) { return -1; }
        if (_.tags > other.tags) { return 1; }
        return 0;
    }

    /// Monotonic 64-bit packed specificity score: `(ids << 40) | (classes << 20) | tags`.
    pub fn toScore(): u64 {
        return ((_.ids as u64) << 40)
            | (((_.classes as u64) & 0xFFFFF) << 20)
            | ((_.tags as u64) & 0xFFFFF);
    }
}

fn csssComputeSingleSpecificity(s: string, depth: u32): Specificity {
    let ids: u32 = 0;
    let classes: u32 = 0;
    let tags: u32 = 0;
    let i: usize = 0;

    while (i < s.len) {
        let c = s[i];
        if (isSpace(c) || csssIsCombinator(c) || c == 124) {
            i = i + 1;
            continue;
        }
        if (c == 42) {
            // Universal selector '*' contributes (0, 0, 0)
            i = i + 1;
            continue;
        }
        if (c == 35) {
            // '#id'
            i = i + 1;
            let start = i;
            i = csssSkipIdent(s, i);
            if (i > start) { ids = ids + 1; }
            continue;
        }
        if (c == 46) {
            // '.class'
            i = i + 1;
            let start = i;
            i = csssSkipIdent(s, i);
            if (i > start) { classes = classes + 1; }
            continue;
        }
        if (c == 91) {
            // '[attr...]'
            let end_idx = csssFindBracketEnd(s, i);
            classes = classes + 1;
            if (end_idx < s.len) {
                i = end_idx + 1;
            } else {
                i = s.len;
            }
            continue;
        }
        if (c == 58) {
            // '::pseudo-element' or ':pseudo-class'
            if (i + 1 < s.len && s[i + 1] == 58) {
                i = i + 2;
                let start = i;
                i = csssSkipIdent(s, i);
                if (i > start) { tags = tags + 1; }
                if (i < s.len && s[i] == 40) {
                    i = csssSkipParen(s, i);
                }
            } else {
                i = i + 1;
                let name_start = i;
                i = csssSkipIdent(s, i);
                let name = s[name_start..i];
                if (i < s.len && s[i] == 40) {
                    let end_paren = csssSkipParen(s, i);
                    let inner_end = if (end_paren > i + 1 && s[end_paren - 1] == 41) {
                        end_paren - 1
                    } else {
                        end_paren
                    };
                    let inner_s = s[i + 1..inner_end];
                    i = end_paren;
                    if (eqAscii(name, "where")) {
                        // ':where(...)' always has zero specificity
                    } else if (eqAscii(name, "is") || eqAscii(name, "not") || eqAscii(name, "has")) {
                        if (depth < 8) {
                            let inner_spec = csssComputeListSpecificity(inner_s, depth + 1);
                            ids = ids + inner_spec.ids;
                            classes = classes + inner_spec.classes;
                            tags = tags + inner_spec.tags;
                        }
                    } else if (name.len > 0) {
                        classes = classes + 1;
                    }
                } else if (name.len > 0) {
                    if (csssIsLegacyPseudoElement(name)) {
                        tags = tags + 1;
                    } else {
                        classes = classes + 1;
                    }
                }
            }
            continue;
        }
        if (csssIsIdentStart(c)) {
            i = csssSkipIdent(s, i);
            // Skip namespace prefix 'ns|tag' without counting 'ns'
            if (i < s.len && s[i] == 124) {
                i = i + 1;
            } else {
                tags = tags + 1;
            }
            continue;
        }
        i = i + 1;
    }

    return Specificity.new(ids, classes, tags);
}

fn csssComputeListSpecificity(selector: string, depth: u32): Specificity {
    let best = Specificity.new(0, 0, 0);
    let has_any = false;
    let start: usize = 0;
    let i: usize = 0;
    let bracket_depth: usize = 0;
    let paren_depth: usize = 0;
    let quote: u8 = 0;

    while (i < selector.len) {
        let c = selector[i];
        if (quote != 0) {
            if (c == 92 && i + 1 < selector.len) {
                i = i + 2;
                continue;
            }
            if (c == quote) { quote = 0; }
        } else {
            if (c == 34 || c == 39) {
                quote = c;
            } else if (c == 91) {
                bracket_depth = bracket_depth + 1;
            } else if (c == 93 && bracket_depth > 0) {
                bracket_depth = bracket_depth - 1;
            } else if (c == 40) {
                paren_depth = paren_depth + 1;
            } else if (c == 41 && paren_depth > 0) {
                paren_depth = paren_depth - 1;
            } else if (c == 44 && bracket_depth == 0 && paren_depth == 0) {
                let part = trim(selector[start..i]);
                if (part.len > 0) {
                    let sp = csssComputeSingleSpecificity(part, depth);
                    if (!has_any || sp.cmp(best) > 0) {
                        best = sp;
                        has_any = true;
                    }
                }
                start = i + 1;
            }
        }
        i = i + 1;
    }

    let tail = trim(selector[start..selector.len]);
    if (tail.len > 0) {
        let sp = csssComputeSingleSpecificity(tail, depth);
        if (!has_any || sp.cmp(best) > 0) {
            best = sp;
        }
    }
    return best;
}

/// Computes W3C Selectors Level 4 specificity for a selector or selector list.
pub fn computeSpecificity(selector: string): Specificity {
    return csssComputeListSpecificity(selector, 0);
}

/// Zero-heap lightweight element descriptor for CSS selector matching.
pub struct CssElement {
    pub tag: string,
    pub id: string,
    pub class_list: string,
    pub attr_name: string,
    pub attr_val: string,

    pub static fn new(
        tag: string,
        id: string,
        class_list: string,
        attr_name: string,
        attr_val: string,
    ): CssElement {
        return CssElement {
            tag: tag,
            id: id,
            class_list: class_list,
            attr_name: attr_name,
            attr_val: attr_val,
        };
    }
}

fn csssMatchAttr(raw_inner: string, el: CssElement): bool {
    let s = trim(raw_inner);
    if (s.len == 0) { return false; }

    let i: usize = 0;
    let name_start = i;
    i = csssSkipIdent(s, i);
    if (i == name_start) { return false; }
    let attr_name = s[name_start..i];

    // HTML attribute names are matched ASCII case-insensitively.
    if (el.attr_name.len == 0 || !eqAscii(attr_name, el.attr_name)) {
        return false;
    }

    while (i < s.len && isSpace(s[i])) {
        i = i + 1;
    }
    // Presence-only selector: [attr]
    if (i >= s.len) {
        return true;
    }

    // Parse attribute matcher operator: =, ^=, $=, *=, ~=, |=
    let op: u8 = 0;
    if (s[i] == 61) {
        op = 0;
        i = i + 1;
    } else if (i + 1 < s.len && s[i + 1] == 61) {
        let ch = s[i];
        if (ch == 94) { op = 1; }       // ^=
        else if (ch == 36) { op = 2; }  // $=
        else if (ch == 42) { op = 3; }  // *=
        else if (ch == 126) { op = 4; } // ~=
        else if (ch == 124) { op = 5; } // |=
        else { return false; }
        i = i + 2;
    } else {
        return false;
    }

    while (i < s.len && isSpace(s[i])) {
        i = i + 1;
    }
    if (i >= s.len) { return false; }

    // Parse quoted or unquoted attribute value
    let val_start: usize = i;
    let val_end: usize = i;
    if (s[i] == 34 || s[i] == 39) {
        let q = s[i];
        i = i + 1;
        val_start = i;
        while (i < s.len && s[i] != q) {
            if (s[i] == 92 && i + 1 < s.len) {
                i = i + 2;
                continue;
            }
            i = i + 1;
        }
        if (i >= s.len) { return false; }
        val_end = i;
        i = i + 1;
    } else {
        val_start = i;
        while (i < s.len && !isSpace(s[i])) {
            i = i + 1;
        }
        val_end = i;
    }
    let val = s[val_start..val_end];

    // Optional ASCII case-sensitivity flag ('i'/'I' or 's'/'S')
    while (i < s.len && isSpace(s[i])) {
        i = i + 1;
    }
    let ci = false;
    if (i < s.len) {
        if (s[i] == 105 || s[i] == 73) {
            ci = true;
            i = i + 1;
        } else if (s[i] == 115 || s[i] == 83) {
            ci = false;
            i = i + 1;
        } else {
            return false;
        }
        while (i < s.len && isSpace(s[i])) {
            i = i + 1;
        }
        if (i < s.len) { return false; }
    }

    if (op == 0) {
        return csssEqMaybeCi(el.attr_val, val, ci);
    }
    if (op == 1) {
        // ^= prefix match (empty value never matches per W3C spec)
        if (val.len == 0 || val.len > el.attr_val.len) { return false; }
        return csssEqMaybeCi(el.attr_val[0..val.len], val, ci);
    }
    if (op == 2) {
        // $= suffix match
        if (val.len == 0 || val.len > el.attr_val.len) { return false; }
        let start = el.attr_val.len - val.len;
        return csssEqMaybeCi(el.attr_val[start..el.attr_val.len], val, ci);
    }
    if (op == 3) {
        // *= substring match
        return csssContains(el.attr_val, val, ci);
    }
    if (op == 4) {
        // ~= whitespace-separated word match
        return csssHasToken(el.attr_val, val, ci);
    }
    // |= dash-separated prefix match
    if (csssEqMaybeCi(el.attr_val, val, ci)) { return true; }
    if (el.attr_val.len > val.len && el.attr_val[val.len] == 45) {
        return csssEqMaybeCi(el.attr_val[0..val.len], val, ci);
    }
    return false;
}

/// Matches a single compound selector (e.g. `div#app.card.active[data-role^="adm"]`) against `el`.
pub fn matchesCompoundSelector(selector: string, el: CssElement): bool {
    let sel = trim(selector);
    if (sel.len == 0) { return false; }

    let i: usize = 0;
    let matched_any = false;

    // Optional leading universal '*' or type selector (case-insensitive via @cap/unicase)
    if (sel[i] == 42) {
        i = i + 1;
        matched_any = true;
    } else if (csssIsIdentStart(sel[i])) {
        let start = i;
        i = csssSkipIdent(sel, i);
        let tag_sel = sel[start..i];
        if (!eqAscii(tag_sel, el.tag)) { return false; }
        matched_any = true;
    }

    while (i < sel.len) {
        let c = sel[i];
        if (c == 35) {
            // '#id'
            i = i + 1;
            let start = i;
            i = csssSkipIdent(sel, i);
            if (i == start) { return false; }
            if (!strEq(sel[start..i], el.id)) { return false; }
            matched_any = true;
        } else if (c == 46) {
            // '.class'
            i = i + 1;
            let start = i;
            i = csssSkipIdent(sel, i);
            if (i == start) { return false; }
            if (!csssHasToken(el.class_list, sel[start..i], false)) { return false; }
            matched_any = true;
        } else if (c == 91) {
            // '[attr...]'
            let end_idx = csssFindBracketEnd(sel, i);
            if (end_idx >= sel.len) { return false; }
            if (!csssMatchAttr(sel[i + 1..end_idx], el)) { return false; }
            i = end_idx + 1;
            matched_any = true;
        } else {
            return false;
        }
    }

    return matched_any;
}

/// Matches a comma-separated selector list (e.g. `h1, .title, #main`) against `el`.
pub fn matchesSelectorList(selector_list: string, el: CssElement): bool {
    let start: usize = 0;
    let i: usize = 0;
    let bracket_depth: usize = 0;
    let paren_depth: usize = 0;
    let quote: u8 = 0;

    while (i < selector_list.len) {
        let c = selector_list[i];
        if (quote != 0) {
            if (c == 92 && i + 1 < selector_list.len) {
                i = i + 2;
                continue;
            }
            if (c == quote) { quote = 0; }
        } else {
            if (c == 34 || c == 39) {
                quote = c;
            } else if (c == 91) {
                bracket_depth = bracket_depth + 1;
            } else if (c == 93 && bracket_depth > 0) {
                bracket_depth = bracket_depth - 1;
            } else if (c == 40) {
                paren_depth = paren_depth + 1;
            } else if (c == 41 && paren_depth > 0) {
                paren_depth = paren_depth - 1;
            } else if (c == 44 && bracket_depth == 0 && paren_depth == 0) {
                if (matchesCompoundSelector(selector_list[start..i], el)) {
                    return true;
                }
                start = i + 1;
            }
        }
        i = i + 1;
    }

    return matchesCompoundSelector(selector_list[start..selector_list.len], el);
}

/// Counts the number of compound selector segments separated by combinators
/// (whitespace, `>`, `+`, `~`) in a selector chain.
pub fn countSelectorParts(selector: string): usize {
    let parts: usize = 0;
    let in_part = false;
    let i: usize = 0;
    let bracket_depth: usize = 0;
    let paren_depth: usize = 0;
    let quote: u8 = 0;

    while (i < selector.len) {
        let c = selector[i];
        if (quote != 0) {
            if (c == 92 && i + 1 < selector.len) {
                i = i + 2;
                continue;
            }
            if (c == quote) { quote = 0; }
            i = i + 1;
            continue;
        }
        if (bracket_depth > 0 || paren_depth > 0) {
            if (c == 34 || c == 39) {
                quote = c;
            } else if (c == 91) {
                bracket_depth = bracket_depth + 1;
            } else if (c == 93 && bracket_depth > 0) {
                bracket_depth = bracket_depth - 1;
            } else if (c == 40) {
                paren_depth = paren_depth + 1;
            } else if (c == 41 && paren_depth > 0) {
                paren_depth = paren_depth - 1;
            }
            i = i + 1;
            continue;
        }

        if (isSpace(c) || csssIsCombinator(c)) {
            in_part = false;
        } else {
            if (!in_part) {
                parts = parts + 1;
                in_part = true;
            }
            if (c == 34 || c == 39) {
                quote = c;
            } else if (c == 91) {
                bracket_depth = 1;
            } else if (c == 40) {
                paren_depth = 1;
            }
        }
        i = i + 1;
    }

    return parts;
}
