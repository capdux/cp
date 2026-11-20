pub(crate) fn strip_redundant_comptime(s: &str) -> String {
    let mut res = String::with_capacity(s.len());
    let chars: Vec<char> = s.chars().collect();
    let n = chars.len();
    let mut i = 0;
    let mut in_str = false;
    let mut str_char = ' ';

    while i < n {
        let c = chars[i];
        if in_str {
            res.push(c);
            if c == '\\' && i + 1 < n {
                i += 1;
                res.push(chars[i]);
            } else if c == str_char {
                in_str = false;
            }
            i += 1;
            continue;
        }

        if c == '"' || c == '\'' {
            in_str = true;
            str_char = c;
            res.push(c);
            i += 1;
            continue;
        }

        // Check if keyword "comptime"
        if i + 8 <= n && &chars[i..i + 8] == ['c', 'o', 'm', 'p', 't', 'i', 'm', 'e'] {
            let prev_ok = i == 0
                || (!chars[i - 1].is_alphanumeric()
                    && chars[i - 1] != '_'
                    && chars[i - 1] != '@'
                    && chars[i - 1] != '.');
            let next_ok = i + 8 == n || (!chars[i + 8].is_alphanumeric() && chars[i + 8] != '_');
            if prev_ok && next_ok {
                i += 8;
                if i < n && (chars[i] == ' ' || chars[i] == '\t') {
                    i += 1;
                }
                continue;
            }
        }

        res.push(c);
        i += 1;
    }
    res
}

pub fn lower_struct_body(code: &str) -> String {
    let mut res = String::with_capacity(code.len());
    let chars: Vec<char> = code.chars().collect();
    let n = chars.len();
    let mut i = 0;
    let mut in_str = false;
    let mut str_char = ' ';
    let mut in_line_comment = false;
    let mut in_block_comment = false;

    while i < n {
        let c = chars[i];

        // 1. Comments
        if in_line_comment {
            res.push(c);
            if c == '\n' {
                in_line_comment = false;
            }
            i += 1;
            continue;
        }
        if in_block_comment {
            res.push(c);
            if c == '*' && i + 1 < n && chars[i + 1] == '/' {
                res.push('/');
                i += 2;
                in_block_comment = false;
                continue;
            }
            i += 1;
            continue;
        }

        // 2. Strings
        if in_str {
            res.push(c);
            if c == '\\' && i + 1 < n {
                i += 1;
                res.push(chars[i]);
            } else if c == str_char {
                in_str = false;
            }
            i += 1;
            continue;
        }

        if c == '"' || c == '\'' {
            in_str = true;
            str_char = c;
            res.push(c);
            i += 1;
            continue;
        }

        if c == '/' && i + 1 < n {
            if chars[i + 1] == '/' {
                in_line_comment = true;
                res.push('/');
                res.push('/');
                i += 2;
                continue;
            } else if chars[i + 1] == '*' {
                in_block_comment = true;
                res.push('/');
                res.push('*');
                i += 2;
                continue;
            }
        }

        // 3. Keyword `let` / `let mut`
        if i + 3 <= n && &chars[i..i + 3] == ['l', 'e', 't'] {
            let prev_ok = i == 0
                || (!chars[i - 1].is_alphanumeric()
                    && chars[i - 1] != '_'
                    && chars[i - 1] != '@'
                    && chars[i - 1] != '.');
            let next_ok = i + 3 == n || chars[i + 3].is_whitespace();
            if prev_ok && next_ok {
                i += 3;
                let mut j = i;
                while j < n && (chars[j] == ' ' || chars[j] == '\t') {
                    j += 1;
                }
                if j + 3 <= n
                    && &chars[j..j + 3] == ['m', 'u', 't']
                    && (j + 3 == n || chars[j + 3].is_whitespace())
                {
                    i = j + 3;
                }
                res.push_str("var");
                continue;
            }
        }

        // 4. Return type colon in function signature: `):` followed by return type
        if c == ')' {
            res.push(')');
            i += 1;
            let mut j = i;
            while j < n && (chars[j] == ' ' || chars[j] == '\t') {
                j += 1;
            }
            if j < n && chars[j] == ':' {
                let mut k = j + 1;
                while k < n && (chars[k] == ' ' || chars[k] == '\t') {
                    k += 1;
                }
                if k < n && chars[k] != '(' && chars[k] != '{' && chars[k] != ';' {
                    if k == j + 1 {
                        res.push(' ');
                    }
                    i = j + 1;
                    continue;
                }
            }
            continue;
        }

        // 5. [*]!? and [*]!
        if i + 5 <= n && &chars[i..i + 5] == ['[', '*', ']', '!', '?'] {
            res.push_str("?[*]anyopaque");
            i += 5;
            continue;
        }
        if i + 4 <= n && &chars[i..i + 4] == ['[', '*', ']', '!'] {
            res.push_str("[*]anyopaque");
            i += 4;
            continue;
        }

        // 6. *!? and *!
        if i + 3 <= n && &chars[i..i + 3] == ['*', '!', '?'] {
            res.push_str("?*anyopaque");
            i += 3;
            continue;
        }
        if i + 2 <= n && &chars[i..i + 2] == ['*', '!'] {
            res.push_str("*anyopaque");
            i += 2;
            continue;
        }

        // 7. ?fn(...) and fn(...) function pointer types
        if i + 3 <= n && &chars[i..i + 3] == ['?', 'f', 'n'] {
            let mut k = i + 3;
            while k < n && (chars[k] == ' ' || chars[k] == '\t') {
                k += 1;
            }
            if k < n && chars[k] == '(' {
                res.push_str("?*const fn");
                i = k;
                continue;
            }
        }
        if i + 2 <= n && &chars[i..i + 2] == ['f', 'n'] {
            let prev_ok = i == 0
                || (!chars[i - 1].is_alphanumeric()
                    && chars[i - 1] != '_'
                    && chars[i - 1] != '@'
                    && chars[i - 1] != '.');
            if prev_ok {
                let mut k = i + 2;
                while k < n && (chars[k] == ' ' || chars[k] == '\t') {
                    k += 1;
                }
                if k < n && chars[k] == '(' {
                    let already_ptr =
                        res.trim_end().ends_with("*const") || res.trim_end().ends_with('*');
                    if already_ptr {
                        res.push_str("fn");
                    } else {
                        res.push_str("*const fn");
                    }
                    i = k;
                    continue;
                }
            }
        }

        res.push(c);
        i += 1;
    }
    res
}
