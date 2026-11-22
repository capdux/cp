// Source: Absorbed from /data/dev/bun/src/css/rules/nesting.cp | Synced: 2026-11-22T17:24:54Z
// Pure-CP zero-allocation CSS Nesting (`@nest` and implicit nested style rules):
// mirrors Bun's `NestingRule.toCss` context-aware `@nest ` emission, validates
// `&` nesting selector placement, and lowers nested selectors (including comma-separated
// selector lists) into flat browser-compatible CSS without heap allocation.

pub const CSS_NESTING_MAX_DECLS: usize = 32;

pub error CssNestingError {
    EmptySelector,
    InvalidAtNestSelector,
    TooManyDecls,
    BufferTooSmall,
}

pub enum CssNestingSyntax: u8 {
    AtNest = 0,
    Implicit = 1,
}

pub struct CssNestingLoc {
    line: u32 = 0,
    column: u32 = 0,
}

pub struct CssNestingDecl {
    property: []const u8 = "",
    value: []const u8 = "",
    important: bool = false,
}

/// Returns `true` if `selector` contains at least one unquoted `&` nesting selector token.
pub fn hasNestingSelectorAmp(selector: []const u8): bool {
    let in_quote: u8 = 0;
    let i: usize = 0;
    while (i < selector.len) {
        const c = selector[i];
        if (in_quote != 0) {
            if (c == '\\' && i + 1 < selector.len) {
                i += 2;
                continue;
            }
            if (c == in_quote) in_quote = 0;
        } else {
            if (c == '"' || c == '\'') {
                in_quote = c;
            } else if (c == '&') {
                return true;
            }
        }
        i += 1;
    }
    return false;
}

/// Validates a W3C `@nest` selector list: every comma-separated item must be non-empty and contain `&`.
pub fn isValidAtNestSelector(selector: []const u8): bool {
    const trimmed = trimAsciiWs(selector);
    if (trimmed.len == 0) return false;
    let start: usize = 0;
    let i: usize = 0;
    let paren_depth: usize = 0;
    let in_quote: u8 = 0;
    while (i <= trimmed.len) {
        const at_end = i == trimmed.len;
        let is_comma = false;
        if (!at_end) {
            const c = trimmed[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < trimmed.len) {
                    i += 2;
                    continue;
                }
                if (c == in_quote) in_quote = 0;
            } else if (c == '"' || c == '\'') {
                in_quote = c;
            } else if (c == '(') {
                paren_depth += 1;
            } else if (c == ')' && paren_depth > 0) {
                paren_depth -= 1;
            } else if (c == ',' && paren_depth == 0) {
                is_comma = true;
            }
        }
        if (at_end || is_comma) {
            const part = trimAsciiWs(trimmed[start..i]);
            if (part.len == 0 || !hasNestingSelectorAmp(part)) {
                return false;
            }
            start = i + 1;
        }
        i += 1;
    }
    return true;
}

/// Lowers a nested `child` selector under `parent` into a flat CSS selector list in `out`.
/// - Expands comma-separated parent/child lists via Cartesian product (e.g. `.a, .b` + `&:hover` -> `.a:hover, .b:hover`)
/// - Replaces every unquoted `&` in `child` with the parent item; if `child` has no `&`, joins as `parent + " " + child`.
pub fn resolveNestedSelector(
    parent: []const u8,
    child: []const u8,
    minify: bool,
    out: []u8,
): []const u8 ! CssNestingError {
    const p_trim = trimAsciiWs(parent);
    const c_trim = trimAsciiWs(child);
    if (p_trim.len == 0 || c_trim.len == 0) return err.EmptySelector;

    let pos: usize = 0;
    let pair_count: usize = 0;

    let p_start: usize = 0;
    let pi: usize = 0;
    let p_depth: usize = 0;
    while (pi <= p_trim.len) {
        const p_end = pi == p_trim.len;
        let p_comma = false;
        if (!p_end) {
            if (p_trim[pi] == '(') p_depth += 1
            else if (p_trim[pi] == ')' && p_depth > 0) p_depth -= 1
            else if (p_trim[pi] == ',' && p_depth == 0) p_comma = true;
        }
        if (p_end || p_comma) {
            const p_item = trimAsciiWs(p_trim[p_start..pi]);
            if (p_item.len == 0) return err.EmptySelector;

            let c_start: usize = 0;
            let ci: usize = 0;
            let c_depth: usize = 0;
            while (ci <= c_trim.len) {
                const c_end = ci == c_trim.len;
                let c_comma = false;
                if (!c_end) {
                    if (c_trim[ci] == '(') c_depth += 1
                    else if (c_trim[ci] == ')' && c_depth > 0) c_depth -= 1
                    else if (c_trim[ci] == ',' && c_depth == 0) c_comma = true;
                }
                if (c_end || c_comma) {
                    const c_item = trimAsciiWs(c_trim[c_start..ci]);
                    if (c_item.len == 0) return err.EmptySelector;

                    if (pair_count > 0) {
                        pos = writeBytes(out, pos, if (minify) "," else ", ")?;
                    }
                    if (hasNestingSelectorAmp(c_item)) {
                        pos = writeWithAmpSubst(out, pos, p_item, c_item)?;
                    } else {
                        pos = writeBytes(out, pos, p_item)?;
                        pos = writeBytes(out, pos, " ")?;
                        pos = writeBytes(out, pos, c_item)?;
                    }
                    pair_count += 1;
                    c_start = ci + 1;
                }
                ci += 1;
            }
            p_start = pi + 1;
        }
        pi += 1;
    }

    return out[0..pos];
}

/// Pure-CP representation of a CSS `@nest` or implicit nested style rule.
pub struct CssNestingRule {
    syntax: CssNestingSyntax = .AtNest,
    selector: []const u8 = "",
    loc: CssNestingLoc = CssNestingLoc{},
    decls: [CSS_NESTING_MAX_DECLS]CssNestingDecl = [CssNestingDecl{}; CSS_NESTING_MAX_DECLS],
    decl_count: usize = 0,

    static fn initAtNest(selector: []const u8, loc: CssNestingLoc): CssNestingRule ! CssNestingError {
        const s = trimAsciiWs(selector);
        if (s.len == 0) return err.EmptySelector;
        if (!isValidAtNestSelector(s)) return err.InvalidAtNestSelector;
        return CssNestingRule{
            syntax: .AtNest,
            selector: s,
            loc: loc,
        };
    }

    static fn initImplicit(selector: []const u8, loc: CssNestingLoc): CssNestingRule ! CssNestingError {
        const s = trimAsciiWs(selector);
        if (s.len == 0) return err.EmptySelector;
        return CssNestingRule{
            syntax: .Implicit,
            selector: s,
            loc: loc,
        };
    }

    fn addDecl(*_, property: []const u8, value: []const u8, important: bool): usize ! CssNestingError {
        if (_.decl_count >= CSS_NESTING_MAX_DECLS) {
            return err.TooManyDecls;
        }
        _.decls[_.decl_count] = CssNestingDecl{
            property: property,
            value: value,
            important: important,
        };
        _.decl_count += 1;
        return _.decl_count;
    }

    fn declCount(): usize {
        return _.decl_count;
    }

    /// Serializes the rule into `out`.
    /// Mirrors Bun `NestingRule.toCss`: emits `"@nest "` iff `!in_parent_context` (and syntax is `.AtNest`).
    fn toCss(in_parent_context: bool, minify: bool, out: []u8): []const u8 ! CssNestingError {
        let pos: usize = 0;
        if (!in_parent_context && _.syntax == .AtNest) {
            pos = writeBytes(out, pos, "@nest ")?;
        }
        pos = writeBytes(out, pos, _.selector)?;
        pos = _.writeDeclBlock(minify, out, pos)?;
        return out[0..pos];
    }

    /// Lowers this nested rule against `parent_selector` into a flat browser-compatible CSS rule.
    fn toLoweredCss(parent_selector: []const u8, minify: bool, out: []u8): []const u8 ! CssNestingError {
        const lowered_sel = resolveNestedSelector(parent_selector, _.selector, minify, out)?;
        const pos = _.writeDeclBlock(minify, out, lowered_sel.len)?;
        return out[0..pos];
    }

    // --- Internal helper ---
    fn writeDeclBlock(minify: bool, out: []u8, start_pos: usize): usize ! CssNestingError {
        let pos = start_pos;
        pos = writeBytes(out, pos, if (minify) "{" else " { ")?;
        let i: usize = 0;
        while (i < _.decl_count) {
            if (i > 0) {
                pos = writeBytes(out, pos, if (minify) ";" else "; ")?;
            }
            const d = _.decls[i];
            pos = writeBytes(out, pos, d.property)?;
            pos = writeBytes(out, pos, if (minify) ":" else ": ")?;
            pos = writeBytes(out, pos, d.value)?;
            if (d.important) {
                pos = writeBytes(out, pos, if (minify) "!important" else " !important")?;
            }
            i += 1;
        }
        pos = writeBytes(out, pos, if (minify) "}" else " }")?;
        return pos;
    }
}

fn trimAsciiWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) start += 1;
    let end = s.len;
    while (end > start && isWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn writeBytes(out: []u8, pos: usize, src: []const u8): usize ! CssNestingError {
    if (pos + src.len > out.len) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeWithAmpSubst(out: []u8, start_pos: usize, parent: []const u8, child: []const u8): usize ! CssNestingError {
    let pos = start_pos;
    let in_quote: u8 = 0;
    let i: usize = 0;
    while (i < child.len) {
        const c = child[i];
        if (in_quote != 0) {
            if (pos >= out.len) return err.BufferTooSmall;
            out[pos] = c;
            pos += 1;
            if (c == '\\' && i + 1 < child.len) {
                i += 1;
                if (pos >= out.len) return err.BufferTooSmall;
                out[pos] = child[i];
                pos += 1;
            } else if (c == in_quote) {
                in_quote = 0;
            }
        } else if (c == '"' || c == '\'') {
            in_quote = c;
            if (pos >= out.len) return err.BufferTooSmall;
            out[pos] = c;
            pos += 1;
        } else if (c == '&') {
            pos = writeBytes(out, pos, parent)?;
        } else {
            if (pos >= out.len) return err.BufferTooSmall;
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    return pos;
}
