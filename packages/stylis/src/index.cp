// @cap/stylis — Zero-heap CSS minifier & nested selector preprocessor

fn stylIsWs(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13 || c == 12;
}

fn stylTrimSlice(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && stylIsWs(s[start])) : (start += 1) {}
    while (end > start && stylIsWs(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn stylPushByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn stylPushSlice(out: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn stylIsStripAfter(c: u8): bool {
    // { } ; , : > + ~
    return c == 123 || c == 125 || c == 59 || c == 44 || c == 58 || c == 62 || c == 43 || c == 126;
}

fn stylIsStripBefore(c: u8): bool {
    // { } ; , > + ~
    return c == 123 || c == 125 || c == 59 || c == 44 || c == 62 || c == 43 || c == 126;
}

pub fn minifyCss(css: string, out: []u8): usize {
    const n: usize = css.len;
    let i: usize = 0;
    let pos: usize = 0;
    let pending_space: bool = false;
    let quote: u8 = 0;
    let paren_depth: usize = 0;

    while (i < n) {
        const c: u8 = css[i];

        // Inside string literal: copy verbatim including escapes
        if (quote != 0) {
            pos = stylPushByte(out, pos, c);
            if (c == 92 && i + 1 < n) {
                i += 1;
                pos = stylPushByte(out, pos, css[i]);
            } else if (c == quote) {
                quote = 0;
            }
            i += 1;
            continue;
        }

        // Start of string literal
        if (c == 34 || c == 39) {
            if (pending_space && pos > 0 && !stylIsStripAfter(out[pos - 1])) {
                pos = stylPushByte(out, pos, 32);
            }
            pending_space = false;
            quote = c;
            pos = stylPushByte(out, pos, c);
            i += 1;
            continue;
        }

        // Block comment: /* ... */
        if (c == 47 && i + 1 < n && css[i + 1] == 42) {
            i += 2;
            while (i < n) {
                if (css[i] == 42 && i + 1 < n && css[i + 1] == 47) {
                    i += 2;
                    break;
                }
                i += 1;
            }
            continue;
        }

        // Single-line comment: // ... (skip inside url(...) parens)
        if (c == 47 && i + 1 < n && css[i + 1] == 47 && paren_depth == 0) {
            i += 2;
            while (i < n && css[i] != 10) : (i += 1) {}
            continue;
        }

        // Whitespace
        if (stylIsWs(c)) {
            pending_space = true;
            i += 1;
            continue;
        }

        if (c == 40) {
            paren_depth += 1;
        } else if (c == 41 && paren_depth > 0) {
            paren_depth -= 1;
        }

        // Emit single space if needed before this token
        if (pending_space && pos > 0) {
            const prev: u8 = out[pos - 1];
            if (!stylIsStripAfter(prev) && !stylIsStripBefore(c)) {
                // Also avoid space right after '(' or right before ')'
                if (prev != 40 && c != 41) {
                    pos = stylPushByte(out, pos, 32);
                }
            }
        }
        pending_space = false;

        pos = stylPushByte(out, pos, c);
        i += 1;
    }

    return pos;
}

fn stylContainsAmp(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == 38) {
            return true;
        }
    }
    return false;
}

fn stylExpandSingle(parent: string, child: string, out: []u8, start_pos: usize): usize {
    let pos: usize = start_pos;
    if (parent.len == 0) {
        return stylPushSlice(out, pos, child);
    }
    if (child.len == 0) {
        return stylPushSlice(out, pos, parent);
    }
    if (stylContainsAmp(child)) {
        let i: usize = 0;
        while (i < child.len) : (i += 1) {
            if (child[i] == 38) {
                pos = stylPushSlice(out, pos, parent);
            } else {
                pos = stylPushByte(out, pos, child[i]);
            }
        }
        return pos;
    }
    pos = stylPushSlice(out, pos, parent);
    pos = stylPushByte(out, pos, 32);
    pos = stylPushSlice(out, pos, child);
    return pos;
}

pub fn expandNestedSelector(parent: string, child: string, out: []u8): usize {
    const p_trim: string = stylTrimSlice(parent);
    const c_trim: string = stylTrimSlice(child);
    if (p_trim.len == 0) {
        return stylPushSlice(out, 0, c_trim);
    }
    if (c_trim.len == 0) {
        return stylPushSlice(out, 0, p_trim);
    }

    let pos: usize = 0;
    let first_out: bool = true;

    let p_start: usize = 0;
    let pi: usize = 0;
    while (pi <= p_trim.len) : (pi += 1) {
        if (pi == p_trim.len || p_trim[pi] == 44) {
            const p_part: string = stylTrimSlice(p_trim[p_start..pi]);
            if (p_part.len > 0) {
                let c_start: usize = 0;
                let ci: usize = 0;
                while (ci <= c_trim.len) : (ci += 1) {
                    if (ci == c_trim.len || c_trim[ci] == 44) {
                        const c_part: string = stylTrimSlice(c_trim[c_start..ci]);
                        if (c_part.len > 0) {
                            if (!first_out) {
                                pos = stylPushByte(out, pos, 44);
                            }
                            pos = stylExpandSingle(p_part, c_part, out, pos);
                            first_out = false;
                        }
                        c_start = ci + 1;
                    }
                }
            }
            p_start = pi + 1;
        }
    }

    return pos;
}

fn stylSkipCommentOrQuote(s: string, idx: usize): usize {
    const n: usize = s.len;
    const c: u8 = s[idx];
    if (c == 34 || c == 39) {
        let j: usize = idx + 1;
        while (j < n) {
            if (s[j] == 92 && j + 1 < n) {
                j += 2;
                continue;
            }
            if (s[j] == c) {
                return j + 1;
            }
            j += 1;
        }
        return n;
    }
    if (c == 47 && idx + 1 < n && s[idx + 1] == 42) {
        let j: usize = idx + 2;
        while (j < n) {
            if (s[j] == 42 && j + 1 < n && s[j + 1] == 47) {
                return j + 2;
            }
            j += 1;
        }
        return n;
    }
    if (c == 47 && idx + 1 < n && s[idx + 1] == 47) {
        let j: usize = idx + 2;
        while (j < n && s[j] != 10) : (j += 1) {}
        return j;
    }
    return idx + 1;
}

pub fn compileNestedCss(parent_sel: string, body_css: string, out: []u8): usize {
    const p_sel: string = stylTrimSlice(parent_sel);
    const n: usize = body_css.len;
    let pos: usize = 0;

    // Pass 1: collect base declarations at depth 0
    let base_opened: bool = false;
    let seg_start: usize = 0;
    let i: usize = 0;
    while (i < n) {
        const c: u8 = body_css[i];
        if (c == 34 || c == 39 || (c == 47 && i + 1 < n && (body_css[i + 1] == 42 || body_css[i + 1] == 47))) {
            i = stylSkipCommentOrQuote(body_css, i);
            continue;
        }
        if (c == 59) {
            // Base declaration ending with ';'
            let tmp: [512]u8 = [0; 512];
            const dlen: usize = minifyCss(body_css[seg_start..i + 1], tmp[0..512]);
            if (dlen > 1) {
                if (!base_opened) {
                    pos = stylPushSlice(out, pos, p_sel);
                    pos = stylPushByte(out, pos, 123);
                    base_opened = true;
                }
                pos = stylPushSlice(out, pos, tmp[0..dlen] as string);
            }
            seg_start = i + 1;
            i += 1;
            continue;
        }
        if (c == 123) {
            // Skip nested block in Pass 1
            let depth: usize = 1;
            i += 1;
            while (i < n && depth > 0) {
                const ch: u8 = body_css[i];
                if (ch == 34 || ch == 39 || (ch == 47 && i + 1 < n && (body_css[i + 1] == 42 || body_css[i + 1] == 47))) {
                    i = stylSkipCommentOrQuote(body_css, i);
                    continue;
                }
                if (ch == 123) {
                    depth += 1;
                } else if (ch == 125) {
                    depth -= 1;
                }
                i += 1;
            }
            seg_start = i;
            continue;
        }
        i += 1;
    }
    // Trailing base declaration without semicolon
    if (seg_start < n) {
        let tmp: [512]u8 = [0; 512];
        const dlen: usize = minifyCss(body_css[seg_start..n], tmp[0..512]);
        if (dlen > 0) {
            if (!base_opened) {
                pos = stylPushSlice(out, pos, p_sel);
                pos = stylPushByte(out, pos, 123);
                base_opened = true;
            }
            pos = stylPushSlice(out, pos, tmp[0..dlen] as string);
        }
    }
    if (base_opened) {
        pos = stylPushByte(out, pos, 125);
    }

    // Pass 2: emit single-level nested child blocks
    let seg2: usize = 0;
    let j: usize = 0;
    while (j < n) {
        const c: u8 = body_css[j];
        if (c == 34 || c == 39 || (c == 47 && j + 1 < n && (body_css[j + 1] == 42 || body_css[j + 1] == 47))) {
            j = stylSkipCommentOrQuote(body_css, j);
            continue;
        }
        if (c == 59) {
            seg2 = j + 1;
            j += 1;
            continue;
        }
        if (c == 123) {
            const raw_child_sel: string = body_css[seg2..j];
            const block_start: usize = j + 1;
            let depth: usize = 1;
            j += 1;
            while (j < n && depth > 0) {
                const ch: u8 = body_css[j];
                if (ch == 34 || ch == 39 || (ch == 47 && j + 1 < n && (body_css[j + 1] == 42 || body_css[j + 1] == 47))) {
                    j = stylSkipCommentOrQuote(body_css, j);
                    continue;
                }
                if (ch == 123) {
                    depth += 1;
                } else if (ch == 125) {
                    depth -= 1;
                }
                j += 1;
            }
            const block_end: usize = if (j > block_start) j - 1 else block_start;

            let clean_child: [256]u8 = [0; 256];
            const clen: usize = minifyCss(raw_child_sel, clean_child[0..256]);
            let expanded: [512]u8 = [0; 512];
            const elen: usize = expandNestedSelector(p_sel, clean_child[0..clen] as string, expanded[0..512]);
            let inner: [1024]u8 = [0; 1024];
            const ilen: usize = minifyCss(body_css[block_start..block_end], inner[0..1024]);

            if (elen > 0 && ilen > 0) {
                pos = stylPushSlice(out, pos, expanded[0..elen] as string);
                pos = stylPushByte(out, pos, 123);
                pos = stylPushSlice(out, pos, inner[0..ilen] as string);
                pos = stylPushByte(out, pos, 125);
            }
            seg2 = j;
            continue;
        }
        j += 1;
    }

    return pos;
}
