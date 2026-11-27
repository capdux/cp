// @cap/cssstyle — Zero-allocation CSS inline style declaration parser & serializer in pure CP
// Clean-room MIT implementation inspired by npm `cssstyle`.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning, comptime 256-entry LUT, @memcpy, and undefined stack buffers.

import { eqIgnoreCase, trim, toLower } from "cap:ascii";

pub const CSS_STYLE_CAP: usize = 64;

type CstVec16 = @Vector(16, u8);

fn cstBuildWsLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[32] = 1; // ' '
    lut[9] = 1;  // '\t'
    lut[10] = 1; // '\n'
    lut[11] = 1; // '\v'
    lut[12] = 1; // '\f'
    lut[13] = 1; // '\r'
    return lut;
}

const CST_WS_LUT: [256]u8 = cstBuildWsLut();

fn cstIsSpace(b: u8): bool {
    return CST_WS_LUT[b as usize] != 0;
}

fn cstFindStar(s: string, start: usize): usize {
    let i = start;
    const len = s.len;
    const v_star: CstVec16 = @splat(42);
    while (i + 16 <= len) {
        const v: CstVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_star)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == 42) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && s[i] != 42) {
        i += 1;
    }
    return i;
}

fn cstFindNameStop(s: string, start: usize): usize {
    let i = start;
    const len = s.len;
    const v_col: CstVec16 = @splat(58);  // ':'
    const v_semi: CstVec16 = @splat(59); // ';'
    const v_sl: CstVec16 = @splat(47);   // '/'
    while (i + 16 <= len) {
        const v: CstVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_col) || @reduce(.Or, v == v_semi) || @reduce(.Or, v == v_sl)) {
            let k: usize = 0;
            while (k < 16) {
                const c = s[i + k];
                if (c == 58 || c == 59 || c == 47) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = s[i];
        if (c == 58 || c == 59 || c == 47) { return i; }
        i += 1;
    }
    return len;
}

fn cstFindSemi(s: string, start: usize): usize {
    let i = start;
    const len = s.len;
    const v_semi: CstVec16 = @splat(59);
    while (i + 16 <= len) {
        const v: CstVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_semi)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == 59) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && s[i] != 59) {
        i += 1;
    }
    return i;
}

fn cstFindValSpecial(s: string, start: usize): usize {
    let i = start;
    const len = s.len;
    const v_dq: CstVec16 = @splat(34);   // '"'
    const v_sq: CstVec16 = @splat(39);   // '\''
    const v_lp: CstVec16 = @splat(40);   // '('
    const v_rp: CstVec16 = @splat(41);   // ')'
    const v_sl: CstVec16 = @splat(47);   // '/'
    const v_semi: CstVec16 = @splat(59); // ';'
    while (i + 16 <= len) {
        const v: CstVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_semi) || @reduce(.Or, v == v_dq) || @reduce(.Or, v == v_sq)
            || @reduce(.Or, v == v_lp) || @reduce(.Or, v == v_rp) || @reduce(.Or, v == v_sl)) {
            let k: usize = 0;
            while (k < 16) {
                const c = s[i + k];
                if (c == 59 || c == 34 || c == 39 || c == 40 || c == 41 || c == 47) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = s[i];
        if (c == 59 || c == 34 || c == 39 || c == 40 || c == 41 || c == 47) { return i; }
        i += 1;
    }
    return len;
}

fn cstWriteStr(out: []u8, pos: usize, s: string): usize {
    if (pos >= out.len || s.len == 0) {
        return pos;
    }
    const avail = out.len - pos;
    const n = if (s.len < avail) { s.len } else { avail };
    @memcpy(out[pos .. pos + n], s[0..n]);
    return pos + n;
}

/// A single CSS declaration property (`name: value [!important]`).
pub struct CssProp {
    name: string,
    value: string,
    important: bool,
}

fn cstSkipSpaceAndComments(s: string, pos: usize): usize {
    let i: usize = pos;
    while (i < s.len) {
        if (cstIsSpace(s[i])) {
            i += 1;
        } else if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            let closed = false;
            while (i + 1 < s.len) {
                i = cstFindStar(s, i);
                if (i + 1 >= s.len) {
                    break;
                }
                if (s[i + 1] == '/') {
                    i += 2;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) {
                i = s.len;
            }
        } else {
            break;
        }
    }
    return i;
}

fn cstStripTrailingComments(s: string): string {
    let cur: string = trim(s);
    while (cur.len >= 4 && cur[cur.len - 2] == '*' && cur[cur.len - 1] == '/') {
        // Find matching opening "/*"
        let k: usize = cur.len - 2;
        let found: bool = false;
        while (k >= 2) : (k -= 1) {
            if (cur[k - 2] == '/' && cur[k - 1] == '*') {
                cur = trim(cur[0..(k - 2)]);
                found = true;
                break;
            }
        }
        if (!found) {
            break;
        }
    }
    return cur;
}

struct CstParsedValue {
    value: string,
    important: bool,
}

fn cstExtractImportant(raw: string): CstParsedValue {
    const clean: string = cstStripTrailingComments(raw);
    const imp_word: string = "important";
    if (clean.len >= 10) {
        const tail_start: usize = clean.len - imp_word.len;
        if (eqIgnoreCase(clean[tail_start..clean.len], imp_word)) {
            let bang_pos: usize = tail_start;
            while (bang_pos > 0 && cstIsSpace(clean[bang_pos - 1])) : (bang_pos -= 1) {}
            if (bang_pos > 0 && clean[bang_pos - 1] == '!') {
                const val: string = cstStripTrailingComments(clean[0..(bang_pos - 1)]);
                return CstParsedValue {
                    value: val,
                    important: true,
                };
            }
        }
    }
    return CstParsedValue {
        value: clean,
        important: false,
    };
}

/// Fixed-capacity (64-item) zero-heap CSS inline style declaration container.
pub struct CssStyleDecl {
    props: [64]CssProp,
    count: usize,

    /// Create an empty `CssStyleDecl`.
    static fn new(): CssStyleDecl {
        return CssStyleDecl {
            props: undefined,
            count: 0,
        };
    }

    /// Number of stored CSS properties.
    fn len(): usize {
        return _.count;
    }

    /// Parse inline CSS text (replacing existing declarations), ignoring `/* ... */` comments.
    fn parse(*_, css_text: string) {
        _.count = 0;
        let i: usize = 0;
        while (i < css_text.len) {
            i = cstSkipSpaceAndComments(css_text, i);
            if (i >= css_text.len) {
                break;
            }
            if (css_text[i] == ';') {
                i += 1;
                continue;
            }

            // Scan property name up to ':', ';', or '/*'
            const name_start: usize = i;
            while (i < css_text.len) {
                i = cstFindNameStop(css_text, i);
                if (i >= css_text.len || css_text[i] == ':' || css_text[i] == ';') {
                    break;
                }
                if (i + 1 < css_text.len && css_text[i] == '/' && css_text[i + 1] == '*') {
                    break;
                }
                i += 1;
            }
            const name: string = trim(css_text[name_start..i]);
            i = cstSkipSpaceAndComments(css_text, i);
            if (i >= css_text.len) {
                break;
            }
            if (css_text[i] == ';') {
                i += 1;
                continue;
            }
            if (css_text[i] != ':') {
                // Malformed declaration: advance to next ';'
                i = cstFindSemi(css_text, i);
                if (i < css_text.len) {
                    i += 1;
                }
                continue;
            }

            // Skip ':' and leading whitespace/comments before value
            i += 1;
            i = cstSkipSpaceAndComments(css_text, i);
            const val_start: usize = i;
            let quote: u8 = 0;
            let paren_depth: usize = 0;
            while (i < css_text.len) {
                if (quote != 0) {
                    const c: u8 = css_text[i];
                    if (c == '\\' && i + 1 < css_text.len) {
                        i += 2;
                        continue;
                    }
                    if (c == quote) {
                        quote = 0;
                    }
                    i += 1;
                } else {
                    i = cstFindValSpecial(css_text, i);
                    if (i >= css_text.len) {
                        break;
                    }
                    const c: u8 = css_text[i];
                    if (c == '"' || c == '\'') {
                        quote = c;
                        i += 1;
                    } else if (c == '(') {
                        paren_depth += 1;
                        i += 1;
                    } else if (c == ')' && paren_depth > 0) {
                        paren_depth -= 1;
                        i += 1;
                    } else if (i + 1 < css_text.len && c == '/' && css_text[i + 1] == '*') {
                        i = cstSkipSpaceAndComments(css_text, i);
                    } else if (c == ';' && paren_depth == 0) {
                        break;
                    } else {
                        i += 1;
                    }
                }
            }
            const raw_val: string = css_text[val_start..i];
            if (i < css_text.len && css_text[i] == ';') {
                i += 1;
            }

            if (name.len > 0) {
                const parsed: CstParsedValue = cstExtractImportant(raw_val);
                if (parsed.value.len > 0) {
                    * = _.setProperty(name, parsed.value, parsed.important);
                }
            }
        }
    }

    /// Look up a property value by name (case-insensitive). Returns `""` if absent.
    fn getPropertyValue(name: string): string {
        const key: string = trim(name);
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (eqIgnoreCase(_.props[i].name, key)) {
                return _.props[i].value;
            }
        }
        return "";
    }

    /// Look up a property priority by name (case-insensitive). Returns `"important"` or `""`.
    fn getPropertyPriority(name: string): string {
        const key: string = trim(name);
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (eqIgnoreCase(_.props[i].name, key)) {
                if (_.props[i].important) {
                    return "important";
                }
                return "";
            }
        }
        return "";
    }

    /// Set or update a CSS property (case-insensitive overwrite). Returns `false` if invalid name or full.
    fn setProperty(*_, name: string, value: string, important: bool): bool {
        const clean_name: string = trim(name);
        if (clean_name.len == 0) {
            return false;
        }
        const clean_val: string = trim(value);
        if (clean_val.len == 0) {
            * = _.removeProperty(clean_name);
            return true;
        }

        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (eqIgnoreCase(_.props[i].name, clean_name)) {
                _.props[i].value = clean_val;
                _.props[i].important = important;
                return true;
            }
        }
        if (_.count >= CSS_STYLE_CAP) {
            return false;
        }
        _.props[_.count] = CssProp {
            name: clean_name,
            value: clean_val,
            important: important,
        };
        _.count += 1;
        return true;
    }

    /// Remove a CSS property by name (case-insensitive). Returns `true` if removed.
    fn removeProperty(*_, name: string): bool {
        const key: string = trim(name);
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (eqIgnoreCase(_.props[i].name, key)) {
                let j: usize = i;
                while (j + 1 < _.count) : (j += 1) {
                    _.props[j] = _.props[j + 1];
                }
                _.count -= 1;
                _.props[_.count] = CssProp {
                    name: "",
                    value: "",
                    important: false,
                };
                return true;
            }
        }
        return false;
    }

    /// Serialize declarations into `out` buffer (e.g. `color: red; font-size: 14px !important;`).
    /// Returns the number of bytes written.
    fn toCssText(out: []u8): usize {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (i > 0) {
                pos = cstWriteStr(out, pos, " ");
            }
            pos += toLower(_.props[i].name, out[pos..out.len]);
            pos = cstWriteStr(out, pos, ": ");
            pos = cstWriteStr(out, pos, _.props[i].value);
            if (_.props[i].important) {
                pos = cstWriteStr(out, pos, " !important");
            }
            pos = cstWriteStr(out, pos, ";");
        }
        return pos;
    }
}
