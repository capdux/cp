// @cap/terser-minify — Zero-allocation lexer-safe JS/TS/CSS minifier for CP
// Clean-room MIT implementation inspired by npm `terser`.

/// Minification options for `minifyJs`.
pub struct MinifyOptions {
    pub strip_comments: bool,
    pub preserve_legal_comments: bool,
    pub collapse_whitespace: bool,
    pub keep_newlines: bool,

    /// Default options: strip comments, preserve `/*! ... */` & `@license` legal comments,
    /// collapse whitespace, do not keep newlines.
    pub static fn defaults(): MinifyOptions {
        return MinifyOptions {
            strip_comments: true,
            preserve_legal_comments: true,
            collapse_whitespace: true,
            keep_newlines: false,
        };
    }
}

/// Result metrics returned by `minifyJs` and `minifyCss`.
pub struct MinifyStats {
    pub input_len: usize,
    pub output_len: usize,
    pub comments_removed: usize,
    pub ok: bool,
}

fn tmIsSpace(ch: u8): bool {
    return ch == 32 || ch == 9 || ch == 10 || ch == 13 || ch == 12;
}

fn tmIsWord(ch: u8): bool {
    if (ch >= 48 && ch <= 57) { return true; }   // 0-9
    if (ch >= 65 && ch <= 90) { return true; }   // A-Z
    if (ch >= 97 && ch <= 122) { return true; }  // a-z
    if (ch == 95 || ch == 36) { return true; }   // '_', '$'
    if (ch >= 128) { return true; }              // Non-ASCII UTF-8 identifier bytes
    return false;
}

fn tmNeedsJsSpace(prev: u8, next: u8): bool {
    if (prev == 0) {
        return false;
    }
    // Never merge adjacent identifiers / keywords / numbers (e.g. `return x`, `let a`)
    if (tmIsWord(prev) && tmIsWord(next)) {
        return true;
    }
    // Never merge `a + +b` into `a++b` or `a - -b` into `a--b`
    if (prev == 43 && next == 43) {
        return true;
    }
    if (prev == 45 && next == 45) {
        return true;
    }
    // Never merge `/ /` into line comment `//`
    if (prev == 47 && next == 47) {
        return true;
    }
    return false;
}

fn tmContainsSubstr(s: string, needle: string): bool {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > s.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= s.len) {
        let j: usize = 0;
        let matched = true;
        while (j < needle.len) {
            if (s[i + j] != needle[j]) {
                matched = false;
                break;
            }
            j = j + 1;
        }
        if (matched) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

fn tmIsLegalComment(comment: string): bool {
    // `/*! ... */`
    if (comment.len >= 3 && comment[2] == 33) {
        return true;
    }
    if (tmContainsSubstr(comment, "@license") || tmContainsSubstr(comment, "@preserve")) {
        return true;
    }
    return false;
}

fn tmIsCssPunct(ch: u8): bool {
    return ch == 123 || ch == 125 || ch == 58 || ch == 59
        || ch == 44 || ch == 62 || ch == 43 || ch == 126;
    // '{', '}', ':', ';', ',', '>', '+', '~'
}

/// Minify JavaScript / TypeScript source code in a single lexer-safe pass without heap allocation.
/// Preserves string literals (`"..."`, `'...'`), template literals (`` `...` ``), legal comments,
/// keyword/identifier boundaries, and unary `+ +` / `- -` separation.
pub fn minifyJs(input: string, opts: MinifyOptions, out: []u8): MinifyStats {
    if (input.len == 0) {
        return MinifyStats {
            input_len: 0,
            output_len: 0,
            comments_removed: 0,
            ok: true,
        };
    }

    let w: usize = 0;
    let i: usize = 0;
    let last_out: u8 = 0;
    let pending_space = false;
    let pending_newline = false;
    let comments_removed: usize = 0;

    while (i < input.len) {
        const ch = input[i];

        // 1. String & template literals: `"..."`, `'...'`, `` `...` ``
        if (ch == 34 || ch == 39 || ch == 96) {
            if (opts.collapse_whitespace) {
                if (pending_newline && opts.keep_newlines && last_out != 0 && last_out != 10) {
                    if (w >= out.len) {
                        return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
                    }
                    out[w] = 10;
                    w = w + 1;
                    last_out = 10;
                } else if (pending_space && tmNeedsJsSpace(last_out, ch)) {
                    if (w >= out.len) {
                        return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
                    }
                    out[w] = 32;
                    w = w + 1;
                    last_out = 32;
                }
                pending_space = false;
                pending_newline = false;
            }

            const quote = ch;
            if (w >= out.len) {
                return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
            }
            out[w] = quote;
            w = w + 1;
            i = i + 1;

            while (i < input.len) {
                const sc = input[i];
                if (w >= out.len) {
                    return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
                }
                out[w] = sc;
                w = w + 1;
                i = i + 1;

                if (sc == 92 && i < input.len) { // '\\' escape
                    if (w >= out.len) {
                        return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
                    }
                    out[w] = input[i];
                    w = w + 1;
                    i = i + 1;
                    continue;
                }
                if (sc == quote) {
                    break;
                }
            }
            last_out = quote;
            continue;
        }

        // 2. Single-line comment `// ...`
        if (ch == 47 && i + 1 < input.len && input[i + 1] == 47) {
            let end = i + 2;
            while (end < input.len && input[end] != 10) {
                end = end + 1;
            }
            if (opts.strip_comments) {
                comments_removed = comments_removed + 1;
                pending_space = true;
                i = end;
                continue;
            }
            // Keep comment verbatim
            let k = i;
            while (k < end) {
                if (w >= out.len) {
                    return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
                }
                out[w] = input[k];
                w = w + 1;
                k = k + 1;
            }
            last_out = input[end - 1];
            i = end;
            continue;
        }

        // 3. Block comment `/* ... */`
        if (ch == 47 && i + 1 < input.len && input[i + 1] == 42) {
            let end = i + 2;
            let has_nl = false;
            while (end + 1 < input.len) {
                if (input[end] == 10) {
                    has_nl = true;
                }
                if (input[end] == 42 && input[end + 1] == 47) {
                    end = end + 2;
                    break;
                }
                end = end + 1;
            }
            if (end < input.len && !(end >= 2 && input[end - 2] == 42 && input[end - 1] == 47)) {
                end = input.len;
            }
            const cslice = input[i..end];
            const keep_it = !opts.strip_comments || (opts.preserve_legal_comments && tmIsLegalComment(cslice));

            if (!keep_it) {
                comments_removed = comments_removed + 1;
                pending_space = true;
                if (has_nl) {
                    pending_newline = true;
                }
                i = end;
                continue;
            }

            if (opts.collapse_whitespace) {
                if (pending_newline && opts.keep_newlines && last_out != 0 && last_out != 10) {
                    if (w >= out.len) {
                        return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
                    }
                    out[w] = 10;
                    w = w + 1;
                    last_out = 10;
                }
                pending_space = false;
                pending_newline = false;
            }

            let k = i;
            while (k < end) {
                if (w >= out.len) {
                    return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
                }
                out[w] = input[k];
                w = w + 1;
                k = k + 1;
            }
            last_out = 47; // '/'
            i = end;
            continue;
        }

        // 4. Whitespace
        if (tmIsSpace(ch)) {
            if (opts.collapse_whitespace) {
                pending_space = true;
                if (ch == 10) {
                    pending_newline = true;
                }
                i = i + 1;
                continue;
            }
            if (w >= out.len) {
                return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
            }
            out[w] = ch;
            w = w + 1;
            last_out = ch;
            i = i + 1;
            continue;
        }

        // 5. Regular code character
        if (opts.collapse_whitespace) {
            if (pending_newline && opts.keep_newlines && last_out != 0 && last_out != 10) {
                if (w >= out.len) {
                    return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
                }
                out[w] = 10;
                w = w + 1;
                last_out = 10;
            } else if (pending_space && tmNeedsJsSpace(last_out, ch)) {
                if (w >= out.len) {
                    return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
                }
                out[w] = 32;
                w = w + 1;
                last_out = 32;
            }
            pending_space = false;
            pending_newline = false;
        }

        if (w >= out.len) {
            return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
        }
        out[w] = ch;
        w = w + 1;
        last_out = ch;
        i = i + 1;
    }

    return MinifyStats {
        input_len: input.len,
        output_len: w,
        comments_removed: comments_removed,
        ok: true,
    };
}

/// Minify CSS source code in a single zero-allocation pass:
/// strips `/* ... */` comments, preserves quoted strings, collapses whitespace around
/// `{}:;,>+~`, and drops redundant trailing semicolons before `}`.
pub fn minifyCss(input: string, out: []u8): MinifyStats {
    if (input.len == 0) {
        return MinifyStats {
            input_len: 0,
            output_len: 0,
            comments_removed: 0,
            ok: true,
        };
    }

    let w: usize = 0;
    let i: usize = 0;
    let last_out: u8 = 0;
    let pending_space = false;
    let comments_removed: usize = 0;

    while (i < input.len) {
        const ch = input[i];

        // Quoted CSS strings `"..."` or `'...'`
        if (ch == 34 || ch == 39) {
            if (pending_space && last_out != 0 && !tmIsCssPunct(last_out)) {
                if (w >= out.len) {
                    return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
                }
                out[w] = 32;
                w = w + 1;
                last_out = 32;
            }
            pending_space = false;

            const quote = ch;
            if (w >= out.len) {
                return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
            }
            out[w] = quote;
            w = w + 1;
            i = i + 1;

            while (i < input.len) {
                const sc = input[i];
                if (w >= out.len) {
                    return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
                }
                out[w] = sc;
                w = w + 1;
                i = i + 1;
                if (sc == 92 && i < input.len) {
                    if (w >= out.len) {
                        return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
                    }
                    out[w] = input[i];
                    w = w + 1;
                    i = i + 1;
                    continue;
                }
                if (sc == quote) {
                    break;
                }
            }
            last_out = quote;
            continue;
        }

        // CSS block comment `/* ... */`
        if (ch == 47 && i + 1 < input.len && input[i + 1] == 42) {
            let end = i + 2;
            while (end + 1 < input.len) {
                if (input[end] == 42 && input[end + 1] == 47) {
                    end = end + 2;
                    break;
                }
                end = end + 1;
            }
            if (end < input.len && !(end >= 2 && input[end - 2] == 42 && input[end - 1] == 47)) {
                end = input.len;
            }
            comments_removed = comments_removed + 1;
            pending_space = true;
            i = end;
            continue;
        }

        // Whitespace
        if (tmIsSpace(ch)) {
            pending_space = true;
            i = i + 1;
            continue;
        }

        // Drop redundant trailing `;` immediately before `}`
        if (ch == 125 && w > 0 && out[w - 1] == 59) {
            w = w - 1;
            last_out = if (w > 0) out[w - 1] else 0;
        }

        if (pending_space && last_out != 0 && !tmIsCssPunct(last_out) && !tmIsCssPunct(ch)) {
            if (w >= out.len) {
                return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
            }
            out[w] = 32;
            w = w + 1;
            last_out = 32;
        }
        pending_space = false;

        if (w >= out.len) {
            return MinifyStats { input_len: input.len, output_len: w, comments_removed: comments_removed, ok: false };
        }
        out[w] = ch;
        w = w + 1;
        last_out = ch;
        i = i + 1;
    }

    return MinifyStats {
        input_len: input.len,
        output_len: w,
        comments_removed: comments_removed,
        ok: true,
    };
}

export {
    MinifyOptions,
    MinifyStats,
    minifyJs,
    minifyCss,
};
