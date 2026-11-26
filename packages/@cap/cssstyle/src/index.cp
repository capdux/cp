// @cap/cssstyle — Zero-allocation CSS inline style declaration parser & serializer in pure CP
// Clean-room MIT implementation inspired by npm `cssstyle`.

pub const CSS_STYLE_CAP: usize = 64;

/// A single CSS declaration property (`name: value [!important]`).
pub struct CssProp {
    name: string,
    value: string,
    important: bool,
}

fn cstToLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn cstIsSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
}

fn cstEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (cstToLower(a[i]) != cstToLower(b[i])) {
            return false;
        }
    }
    return true;
}

fn cstTrim(s: string): string {
    let start: usize = 0;
    while (start < s.len && cstIsSpace(s[start])) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && cstIsSpace(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn cstSkipSpaceAndComments(s: string, pos: usize): usize {
    let i: usize = pos;
    while (i < s.len) {
        if (cstIsSpace(s[i])) {
            i += 1;
        } else if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < s.len && !(s[i] == '*' && s[i + 1] == '/')) : (i += 1) {}
            if (i + 1 < s.len) {
                i += 2;
            } else {
                i = s.len;
            }
        } else {
            break;
        }
    }
    return i;
}

fn cstStripTrailingComments(s: string): string {
    let cur: string = cstTrim(s);
    while (cur.len >= 4 && cur[cur.len - 2] == '*' && cur[cur.len - 1] == '/') {
        // Find matching opening "/*"
        let k: usize = cur.len - 2;
        let found: bool = false;
        while (k >= 2) : (k -= 1) {
            if (cur[k - 2] == '/' && cur[k - 1] == '*') {
                cur = cstTrim(cur[0..(k - 2)]);
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
        if (cstEqIgnoreCase(clean[tail_start..clean.len], imp_word)) {
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

fn cstWriteBytes(out: []u8, pos: usize, src: string, lowercase: bool): usize {
    let cur: usize = pos;
    let i: usize = 0;
    while (i < src.len && cur < out.len) : (i += 1) {
        if (lowercase) {
            out[cur] = cstToLower(src[i]);
        } else {
            out[cur] = src[i];
        }
        cur += 1;
    }
    return cur;
}

/// Fixed-capacity (64-item) zero-heap CSS inline style declaration container.
pub struct CssStyleDecl {
    props: [64]CssProp,
    count: usize,

    /// Create an empty `CssStyleDecl`.
    static fn new(): CssStyleDecl {
        let decl: CssStyleDecl = CssStyleDecl {
            props: undefined,
            count: 0,
        };
        let i: usize = 0;
        while (i < CSS_STYLE_CAP) : (i += 1) {
            decl.props[i] = CssProp {
                name: "",
                value: "",
                important: false,
            };
        }
        return decl;
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
            while (i < css_text.len && css_text[i] != ':' && css_text[i] != ';') {
                if (i + 1 < css_text.len && css_text[i] == '/' && css_text[i + 1] == '*') {
                    break;
                }
                i += 1;
            }
            const name: string = cstTrim(css_text[name_start..i]);
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
                while (i < css_text.len && css_text[i] != ';') : (i += 1) {}
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
                const c: u8 = css_text[i];
                if (quote != 0) {
                    if (c == '\\' && i + 1 < css_text.len) {
                        i += 2;
                        continue;
                    }
                    if (c == quote) {
                        quote = 0;
                    }
                    i += 1;
                } else {
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
        const key: string = cstTrim(name);
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (cstEqIgnoreCase(_.props[i].name, key)) {
                return _.props[i].value;
            }
        }
        return "";
    }

    /// Look up a property priority by name (case-insensitive). Returns `"important"` or `""`.
    fn getPropertyPriority(name: string): string {
        const key: string = cstTrim(name);
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (cstEqIgnoreCase(_.props[i].name, key)) {
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
        const clean_name: string = cstTrim(name);
        if (clean_name.len == 0) {
            return false;
        }
        const clean_val: string = cstTrim(value);
        if (clean_val.len == 0) {
            * = _.removeProperty(clean_name);
            return true;
        }

        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (cstEqIgnoreCase(_.props[i].name, clean_name)) {
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
        const key: string = cstTrim(name);
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (cstEqIgnoreCase(_.props[i].name, key)) {
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
                pos = cstWriteBytes(out, pos, " ", false);
            }
            pos = cstWriteBytes(out, pos, _.props[i].name, true);
            pos = cstWriteBytes(out, pos, ": ", false);
            pos = cstWriteBytes(out, pos, _.props[i].value, false);
            if (_.props[i].important) {
                pos = cstWriteBytes(out, pos, " !important", false);
            }
            pos = cstWriteBytes(out, pos, ";", false);
        }
        return pos;
    }
}
