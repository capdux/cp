// Rust -> CP High-Success-Rate Structural Transpiler (`cap from -rs`)
// Designed and verified against real-world Rust crates (e.g., `_ref/axum`).

use std::collections::HashMap;

use crate::parser::Parser;

use super::deps::{rewrite_import_source, DepTier, SourceLang};

pub struct RsTranspileResult {
    pub cp_code: String,
    pub todo_count: usize,
    pub opt_notes: Vec<String>,
}

/// CP reserved keywords that cannot be used directly as plain parameter/field identifiers.
fn sanitize_cp_ident(name: &str) -> String {
    let trimmed = name.trim();
    if trimmed.is_empty() {
        return "_unused".to_string();
    }
    // Strip raw identifier prefix `r#`
    let clean = trimmed.strip_prefix("r#").unwrap_or(trimmed);
    let clean = if clean == "_" { "_unused" } else { clean };
    match clean {
        "in" | "of" | "class" | "interface" | "extends" | "implements" | "new" | "delete"
        | "import" | "export" | "from" | "default" | "switch" | "case" | "catch" | "finally"
        | "throw" | "try" | "var" | "function" | "null" | "undefined" | "this" | "super"
        | "defer" | "errdefer" | "actor" | "spawn" | "receive" | "reply" | "comptime" | "zig"
        | "string" | "void" | "bool" | "err" | "type" | "fn" | "let" | "const" | "mut"
        | "struct" | "enum" | "union" | "trait" | "impl" | "pub" | "priv" | "static" | "async"
        | "await" | "move" | "ref" | "unsafe" | "extern" | "return" | "if" | "else" | "while"
        | "for" | "break" | "continue" | "true" | "false" | "as" => {
            format!("{}_", clean)
        }
        _ => {
            // Ensure valid identifier chars
            let mut out = String::with_capacity(clean.len());
            for (idx, ch) in clean.chars().enumerate() {
                if idx == 0 {
                    if ch.is_ascii_alphabetic() || ch == '_' {
                        out.push(ch);
                    } else {
                        out.push('_');
                    }
                } else if ch.is_ascii_alphanumeric() || ch == '_' {
                    out.push(ch);
                } else {
                    out.push('_');
                }
            }
            if out.is_empty() {
                "_unused".to_string()
            } else {
                out
            }
        }
    }
}

/// Normalize Rust raw strings (`r"..."`, `r#"..."#`, `br#"..."#`), byte strings (`b"..."`),
/// byte char literals (`b'x'`), and nested block comments `/* ... */` into CP-lexer-safe forms.
fn preprocess_rust_source(source: &str) -> String {
    let bytes = source.as_bytes();
    let len = bytes.len();
    let mut out = String::with_capacity(len + 64);
    let mut i = 0;

    while i < len {
        // 1. Line comment `// ...`
        if bytes[i] == b'/' && i + 1 < len && bytes[i + 1] == b'/' {
            while i < len && bytes[i] != b'\n' {
                out.push(bytes[i] as char);
                i += 1;
            }
            continue;
        }

        // 2. Nested block comment `/* ... */` -> convert to `//` line comments or single-line space
        if bytes[i] == b'/' && i + 1 < len && bytes[i + 1] == b'*' {
            let mut depth = 1usize;
            i += 2;
            let start = i;
            while i < len && depth > 0 {
                if bytes[i] == b'/' && i + 1 < len && bytes[i + 1] == b'*' {
                    depth += 1;
                    i += 2;
                } else if bytes[i] == b'*' && i + 1 < len && bytes[i + 1] == b'/' {
                    depth -= 1;
                    i += 2;
                } else {
                    i += 1;
                }
            }
            let comment_end = if depth == 0 && i >= 2 { i - 2 } else { i };
            let body = &source[start..comment_end];
            if body.contains('\n') {
                for line in body.lines() {
                    out.push_str("// ");
                    out.push_str(line.trim());
                    out.push('\n');
                }
            } else {
                out.push(' ');
            }
            continue;
        }

        // 3. Raw byte string `br"..."` / `br#"..."#` or raw string `r"..."` / `r#"..."#`
        let prev_is_ident = i > 0 && (bytes[i - 1].is_ascii_alphanumeric() || bytes[i - 1] == b'_');
        if !prev_is_ident {
            let raw_prefix_len = if bytes[i] == b'b' && i + 1 < len && bytes[i + 1] == b'r' {
                Some(2)
            } else if bytes[i] == b'r' {
                Some(1)
            } else {
                None
            };
            if let Some(plen) = raw_prefix_len {
                let mut j = i + plen;
                let mut hashes = 0usize;
                while j < len && bytes[j] == b'#' {
                    hashes += 1;
                    j += 1;
                }
                if j < len && bytes[j] == b'"' {
                    j += 1; // skip opening quote
                    let content_start = j;
                    let mut found_end = None;
                    while j < len {
                        if bytes[j] == b'"' {
                            let mut h = 0usize;
                            while h < hashes && j + 1 + h < len && bytes[j + 1 + h] == b'#' {
                                h += 1;
                            }
                            if h == hashes {
                                found_end = Some((j, j + 1 + hashes));
                                break;
                            }
                        }
                        j += 1;
                    }
                    if let Some((content_end, next_i)) = found_end {
                        let raw_content = &source[content_start..content_end];
                        out.push('"');
                        for ch in raw_content.chars() {
                            match ch {
                                '\\' => out.push_str("\\\\"),
                                '"' => out.push_str("\\\""),
                                '\n' => out.push_str("\\n"),
                                '\r' => out.push_str("\\r"),
                                '\t' => out.push_str("\\t"),
                                _ => out.push(ch),
                            }
                        }
                        out.push('"');
                        i = next_i;
                        continue;
                    }
                }
            }
        }

        // 4. Byte string `b"..."`
        if !prev_is_ident && bytes[i] == b'b' && i + 1 < len && bytes[i + 1] == b'"' {
            i += 1; // drop `b`, fall through to normal string handling
        }

        // 5. Byte character literal `b'x'` / `b'\n'` / `b'\x1b'`
        if !prev_is_ident && bytes[i] == b'b' && i + 1 < len && bytes[i + 1] == b'\'' {
            let mut j = i + 2;
            let val: Option<u8> = if j < len && bytes[j] == b'\\' {
                j += 1;
                if j < len {
                    let esc = bytes[j];
                    j += 1;
                    match esc {
                        b'n' => Some(b'\n'),
                        b'r' => Some(b'\r'),
                        b't' => Some(b'\t'),
                        b'\\' => Some(b'\\'),
                        b'\'' => Some(b'\''),
                        b'"' => Some(b'"'),
                        b'0' => Some(0),
                        b'x' if j + 2 <= len => {
                            let hex = &source[j..j + 2];
                            j += 2;
                            u8::from_str_radix(hex, 16).ok()
                        }
                        _ => Some(esc),
                    }
                } else {
                    None
                }
            } else if j < len {
                let ch = bytes[j];
                j += 1;
                Some(ch)
            } else {
                None
            };
            if let (Some(byte_val), true) = (val, j < len && bytes[j] == b'\'') {
                out.push_str(&byte_val.to_string());
                i = j + 1;
                continue;
            }
        }

        // 6. Normal double-quoted string `"..."` (preserve verbatim, fold literal newlines safely)
        if bytes[i] == b'"' {
            out.push('"');
            i += 1;
            while i < len {
                let c = bytes[i];
                if c == b'\\' && i + 1 < len {
                    let next_c = bytes[i + 1];
                    if next_c == b'\n' {
                        // Rust line continuation inside string
                        i += 2;
                        while i < len && (bytes[i] == b' ' || bytes[i] == b'\t') {
                            i += 1;
                        }
                        continue;
                    }
                    out.push('\\');
                    let ch = source[i + 1..].chars().next().unwrap();
                    out.push(ch);
                    i += 1 + ch.len_utf8();
                    continue;
                }
                if c == b'"' {
                    out.push('"');
                    i += 1;
                    break;
                }
                if c == b'\n' {
                    out.push_str("\\n");
                    i += 1;
                    continue;
                }
                if c == b'\r' {
                    i += 1;
                    continue;
                }
                let ch = source[i..].chars().next().unwrap();
                out.push(ch);
                i += ch.len_utf8();
            }
            continue;
        }

        // 7. Default UTF-8 char copy
        let ch = source[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }

    out
}

/// Strip Rust lifetimes (`'a`, `'static`, `'_`, `'de`) and loop labels (`'outer:`) outside string literals,
/// while preserving valid char literals (`'a'`, `'\n'`, `'\''`).
fn strip_rust_lifetimes(s: &str) -> String {
    let bytes = s.as_bytes();
    let len = bytes.len();
    let mut out = String::with_capacity(len);
    let mut i = 0;
    let mut in_string = false;

    while i < len {
        // Track double-quoted strings
        if in_string {
            if bytes[i] == b'\\' && i + 1 < len {
                out.push('\\');
                let ch = s[i + 1..].chars().next().unwrap();
                out.push(ch);
                i += 1 + ch.len_utf8();
                continue;
            }
            if bytes[i] == b'"' {
                in_string = false;
                out.push('"');
                i += 1;
                continue;
            }
            let ch = s[i..].chars().next().unwrap();
            out.push(ch);
            i += ch.len_utf8();
            continue;
        }

        if bytes[i] == b'"' {
            in_string = true;
            out.push('"');
            i += 1;
            continue;
        }

        // Preserve `//` line comments
        if bytes[i] == b'/' && i + 1 < len && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }

        if bytes[i] == b'\'' {
            // Check for escaped char literal like '\n', '\'', '\\', '\x1b', '\u{1234}'
            if i + 1 < len && bytes[i + 1] == b'\\' {
                if let Some(close_rel) = s[i + 2..].find('\'') {
                    if close_rel <= 10 {
                        let end = i + 2 + close_rel + 1;
                        out.push_str(&s[i..end]);
                        i = end;
                        continue;
                    }
                }
            }
            // Check for single-char literal `'x'` (including multi-byte UTF-8)
            if i + 1 < len {
                if let Some(ch) = s[i + 1..].chars().next() {
                    let after_ch = i + 1 + ch.len_utf8();
                    if after_ch < len && bytes[after_ch] == b'\'' {
                        out.push('\'');
                        out.push(ch);
                        out.push('\'');
                        i = after_ch + 1;
                        continue;
                    }
                }
            }

            // Otherwise, it's a Rust lifetime (`'a`, `'static`, `'_`) or loop label (`'label:`)
            let mut j = i + 1;
            while j < len && (bytes[j].is_ascii_alphanumeric() || bytes[j] == b'_') {
                j += 1;
            }
            if j > i + 1 {
                let mut k = j;
                while k < len && bytes[k] == b' ' {
                    k += 1;
                }
                // Consume trailing `+`, `,`, or `:` (for `'a +`, `'a,`, `'label:`)
                if k < len && (bytes[k] == b',' || bytes[k] == b'+' || bytes[k] == b':') {
                    // Don't eat `::`
                    if bytes[k] == b':' && k + 1 < len && bytes[k + 1] == b':' {
                        i = j;
                    } else {
                        k += 1;
                        while k < len && bytes[k] == b' ' {
                            k += 1;
                        }
                        i = k;
                    }
                } else {
                    // Also trim leading `+ ` if we had `T + 'static`
                    let trimmed_end = out.trim_end_matches(' ');
                    if trimmed_end.ends_with('+') {
                        let new_len = trimmed_end.len() - 1;
                        out.truncate(new_len);
                    }
                    i = j;
                }
                continue;
            }
        }

        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }

    // Clean up empty angle brackets and dangling commas inside `<...>`
    let mut cleaned = out
        .replace("<, ", "<")
        .replace("<,", "<")
        .replace(", >", ">")
        .replace(",>", ">")
        .replace("<>", "")
        .replace("< >", "");
    while cleaned.contains("  ") {
        cleaned = cleaned.replace("  ", " ");
    }
    cleaned
}

/// Count net delimiter deltas on a code line, ignoring strings and `//` comments.
fn scan_delimiters(line: &str) -> (i32, i32, i32) {
    let bytes = line.as_bytes();
    let len = bytes.len();
    let mut i = 0;
    let mut in_str = false;
    let mut braces = 0i32;
    let mut parens = 0i32;
    let mut brackets = 0i32;

    while i < len {
        let b = bytes[i];
        if in_str {
            if b == b'\\' && i + 1 < len {
                i += 2;
                continue;
            }
            if b == b'"' {
                in_str = false;
            }
            i += 1;
            continue;
        }
        if b == b'/' && i + 1 < len && bytes[i + 1] == b'/' {
            break;
        }
        if b == b'"' {
            in_str = true;
            i += 1;
            continue;
        }
        if b == b'\'' {
            // Skip char literal `'x'` or `'\x'`
            if i + 2 < len && bytes[i + 2] == b'\'' {
                i += 3;
                continue;
            }
            if i + 3 < len && bytes[i + 1] == b'\\' && bytes[i + 3] == b'\'' {
                i += 4;
                continue;
            }
        }
        match b {
            b'{' => braces += 1,
            b'}' => braces -= 1,
            b'(' => parens += 1,
            b')' => parens -= 1,
            b'[' => brackets += 1,
            b']' => brackets -= 1,
            _ => {}
        }
        i += 1;
    }
    (braces, parens, brackets)
}

/// Strip inline `// ...` comment from a line (outside strings).
fn strip_inline_comment(line: &str) -> &str {
    let bytes = line.as_bytes();
    let mut i = 0;
    let mut in_str = false;
    while i < bytes.len() {
        if in_str {
            if bytes[i] == b'\\' && i + 1 < bytes.len() {
                i += 2;
                continue;
            }
            if bytes[i] == b'"' {
                in_str = false;
            }
            i += 1;
            continue;
        }
        if bytes[i] == b'"' {
            in_str = true;
            i += 1;
            continue;
        }
        if bytes[i] == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            return line[..i].trim_end();
        }
        i += 1;
    }
    line.trim_end()
}

/// Split string by top-level commas (respecting `<...>`, `(...)`, `[...]`, `{...}`).
fn split_top_level_commas(s: &str) -> Vec<String> {
    let mut parts = Vec::new();
    let mut cur = String::new();
    let mut depth_angle = 0i32;
    let mut depth_paren = 0i32;
    let mut depth_brack = 0i32;
    let mut depth_brace = 0i32;
    let mut in_str = false;
    let mut prev_bs = false;

    for ch in s.chars() {
        if in_str {
            cur.push(ch);
            if prev_bs {
                prev_bs = false;
            } else if ch == '\\' {
                prev_bs = true;
            } else if ch == '"' {
                in_str = false;
            }
            continue;
        }
        match ch {
            '"' => {
                in_str = true;
                cur.push(ch);
            }
            '<' => {
                depth_angle += 1;
                cur.push(ch);
            }
            '>' => {
                if depth_angle > 0 {
                    depth_angle -= 1;
                }
                cur.push(ch);
            }
            '(' => {
                depth_paren += 1;
                cur.push(ch);
            }
            ')' => {
                if depth_paren > 0 {
                    depth_paren -= 1;
                }
                cur.push(ch);
            }
            '[' => {
                depth_brack += 1;
                cur.push(ch);
            }
            ']' => {
                if depth_brack > 0 {
                    depth_brack -= 1;
                }
                cur.push(ch);
            }
            '{' => {
                depth_brace += 1;
                cur.push(ch);
            }
            '}' => {
                if depth_brace > 0 {
                    depth_brace -= 1;
                }
                cur.push(ch);
            }
            ',' if depth_angle == 0 && depth_paren == 0 && depth_brack == 0 && depth_brace == 0 => {
                parts.push(cur.trim().to_string());
                cur.clear();
            }
            _ => cur.push(ch),
        }
    }
    if !cur.trim().is_empty() {
        parts.push(cur.trim().to_string());
    }
    parts
}

/// Find the matching closing delimiter for `open` starting at `start_idx`.
fn find_matching_delim(s: &str, start_idx: usize, open: char, close: char) -> Option<usize> {
    let mut depth = 0i32;
    let mut in_str = false;
    let mut prev_bs = false;
    for (rel, ch) in s[start_idx..].char_indices() {
        if in_str {
            if prev_bs {
                prev_bs = false;
            } else if ch == '\\' {
                prev_bs = true;
            } else if ch == '"' {
                in_str = false;
            }
            continue;
        }
        if ch == '"' {
            in_str = true;
            continue;
        }
        if ch == open {
            depth += 1;
        } else if ch == close {
            depth -= 1;
            if depth == 0 {
                return Some(start_idx + rel);
            }
        }
    }
    None
}

/// Clean Rust generic parameter list `<'a, T: Send + Sync = (), const N: usize>` -> `<T, N>`.
fn clean_generic_params(raw_generics: &str) -> String {
    let stripped = strip_rust_lifetimes(raw_generics);
    let inner = stripped
        .trim()
        .strip_prefix('<')
        .and_then(|x| x.strip_suffix('>'))
        .unwrap_or(stripped.trim());
    let mut params = Vec::new();
    for part in split_top_level_commas(inner) {
        let p = part.trim();
        if p.is_empty() || p.starts_with('\'') {
            continue;
        }
        let p = p.strip_prefix("const ").unwrap_or(p).trim();
        let name = p
            .split(|c: char| c == ':' || c == '=' || c.is_whitespace())
            .next()
            .unwrap_or("")
            .trim();
        if !name.is_empty() && name.chars().all(|c| c.is_ascii_alphanumeric() || c == '_') {
            params.push(name.to_string());
        }
    }
    if params.is_empty() {
        String::new()
    } else {
        format!("<{}>", params.join(", "))
    }
}

/// Strip outer Rust type wrappers / bounds and map to valid CP type syntax.
fn map_rust_type(raw: &str) -> String {
    let mut t = strip_rust_lifetimes(raw).trim().to_string();
    // Strip trailing commas/semicolons
    t = t.trim_end_matches(&[',', ';'][..]).trim().to_string();
    // Strip Rust `for<...>` HRTB prefix
    if let Some(rest) = t.strip_prefix("for<") {
        if let Some(end) = find_matching_delim(rest, 0, '<', '>') {
            t = rest[end + 1..].trim().to_string();
        } else if let Some(gt) = rest.find('>') {
            t = rest[gt + 1..].trim().to_string();
        }
    }
    // Strip `impl ...` or `dyn ...` trait-object / opaque return types
    if let Some(rest) = t.strip_prefix("impl ") {
        // e.g. `impl Future<Output = T> + Send` -> take primary trait/type name
        let primary = rest.split('+').next().unwrap_or("anyopaque").trim();
        let mapped = map_rust_type(primary);
        if mapped.is_empty() || mapped == "void" {
            return "*anyopaque".to_string();
        }
        return mapped;
    }
    if let Some(rest) = t.strip_prefix("dyn ") {
        let primary = rest.split('+').next().unwrap_or("anyopaque").trim();
        let base = primary
            .split(|c: char| c == '<' || c == '(' || c.is_whitespace())
            .next()
            .unwrap_or("anyopaque")
            .split("::")
            .last()
            .unwrap_or("anyopaque");
        return format!("*{}", base);
    }
    if t.is_empty() || t == "()" || t == "!" {
        return "void".to_string();
    }
    if t == "&str"
        || t == "& str"
        || t == "String"
        || t == "Cow<str>"
        || t == "Box<str>"
        || t == "str"
    {
        return "string".to_string();
    }
    if t == "Self" {
        return "Self".to_string();
    }
    if let Some(inner) = t.strip_prefix("&mut ") {
        let mapped = map_rust_type(inner);
        return if mapped == "void" {
            "*anyopaque".to_string()
        } else {
            format!("*{}", mapped)
        };
    }
    if let Some(inner) = t.strip_prefix("& mut ") {
        let mapped = map_rust_type(inner);
        return if mapped == "void" {
            "*anyopaque".to_string()
        } else {
            format!("*{}", mapped)
        };
    }
    if let Some(inner) = t.strip_prefix('&') {
        let inner = inner.trim();
        if inner == "str" {
            return "string".to_string();
        }
        if inner.starts_with('[') && inner.ends_with(']') {
            let elem = &inner[1..inner.len() - 1];
            return format!("[]{}", map_rust_type(elem));
        }
        let mapped = map_rust_type(inner);
        return if mapped == "void" {
            "*const anyopaque".to_string()
        } else {
            format!("*const {}", mapped)
        };
    }
    if let Some(inner) = t.strip_prefix("*const ") {
        return format!("*const {}", map_rust_type(inner));
    }
    if let Some(inner) = t.strip_prefix("*mut ") {
        return format!("*{}", map_rust_type(inner));
    }

    // Tuple type `(A, B)` -> `[A, B]`
    if t.starts_with('(') && t.ends_with(')') {
        if find_matching_delim(&t, 0, '(', ')') == Some(t.len() - 1) {
            let inner = &t[1..t.len() - 1].trim();
            if inner.is_empty() {
                return "void".to_string();
            }
            let elems: Vec<String> = split_top_level_commas(inner)
                .into_iter()
                .map(|e| map_rust_type(&e))
                .filter(|e| !e.is_empty())
                .collect();
            if elems.len() == 1 {
                return elems[0].clone();
            }
            return format!("[{}]", elems.join(", "));
        }
    }

    // Slice / array type `[T]` or `[T; N]`
    if t.starts_with('[') && t.ends_with(']') {
        let inner = &t[1..t.len() - 1].trim();
        if let Some(semi) = inner.rfind(';') {
            let elem = map_rust_type(&inner[..semi]);
            let count = inner[semi + 1..].trim();
            return format!("[{}]{}", count, elem);
        }
        return format!("[]{}", map_rust_type(inner));
    }

    // Associated type projection `<T as Trait>::Item` -> `Item`
    if t.starts_with('<') {
        if let Some(last_seg) = t.rsplit("::").next() {
            let clean = last_seg.trim();
            if !clean.is_empty() {
                return map_rust_type(clean);
            }
        }
        return "*anyopaque".to_string();
    }

    // Function pointer `fn(A, B) -> C`
    if t.starts_with("fn(") || t.starts_with("unsafe fn(") {
        return "*anyopaque".to_string();
    }

    // Generic containers: Option<T>, Result<T, E>, Vec<T>, Box<T>, Arc<T>, Pin<T>
    if let Some(lt) = t.find('<') {
        if t.ends_with('>') && find_matching_delim(&t, lt, '<', '>') == Some(t.len() - 1) {
            let base_raw = t[..lt].trim();
            let base_last = base_raw.rsplit("::").next().unwrap_or(base_raw).trim();
            let inner = &t[lt + 1..t.len() - 1];
            let args = split_top_level_commas(inner);
            match base_last {
                "Option" if !args.is_empty() => {
                    let ok = map_rust_type(&args[0]);
                    return if ok == "void" {
                        "bool".to_string()
                    } else {
                        format!("{}?", ok)
                    };
                }
                "Result" if !args.is_empty() => {
                    let ok = map_rust_type(&args[0]);
                    // CP uses postfix `T!` (never prefix `?T` which is abolished!)
                    return if ok == "void" {
                        "!".to_string()
                    } else {
                        format!("{}!", ok)
                    };
                }
                "Vec" if !args.is_empty() => {
                    return format!("[]{}", map_rust_type(&args[0]));
                }
                "Box" | "Arc" | "Rc" | "Pin" if !args.is_empty() => {
                    let inner_ty = map_rust_type(&args[0]);
                    return if inner_ty.starts_with('*') || inner_ty == "string" {
                        inner_ty
                    } else {
                        format!("*{}", inner_ty)
                    };
                }
                _ => {
                    // Clean generic type arguments, dropping `Item = T` associated bindings
                    let mapped_args: Vec<String> = args
                        .into_iter()
                        .filter_map(|a| {
                            let a = a.trim();
                            if a.is_empty() {
                                return None;
                            }
                            if let Some(eq) = a.find('=') {
                                let rhs = map_rust_type(&a[eq + 1..]);
                                return if rhs.is_empty() { None } else { Some(rhs) };
                            }
                            let m = map_rust_type(a);
                            if m.is_empty() {
                                None
                            } else {
                                Some(m)
                            }
                        })
                        .collect();
                    let clean_base = sanitize_type_path(base_raw);
                    if mapped_args.is_empty() {
                        return clean_base;
                    }
                    return format!("{}<{}>", clean_base, mapped_args.join(", "));
                }
            }
        }
    }

    // Drop trailing `+ Send + Sync` bounds on bare types
    if let Some(plus) = t.find('+') {
        t = t[..plus].trim().to_string();
    }

    // Drop `Foo(A) -> B` Fn-trait style types
    if t.contains('(') {
        return "*anyopaque".to_string();
    }

    sanitize_type_path(&t)
}

/// Convert Rust module path `foo::bar::Baz` into a valid CP type path (`Baz` or `foo.Bar`).
fn sanitize_type_path(raw: &str) -> String {
    let s = raw.trim().trim_start_matches("::");
    if s.is_empty() {
        return "anyopaque".to_string();
    }
    // Keep last identifier for deeply nested std/crate paths to avoid parser keyword collisions
    let segs: Vec<&str> = s
        .split("::")
        .map(|x| x.trim())
        .filter(|x| !x.is_empty() && *x != "crate" && *x != "super" && *x != "self")
        .collect();
    if segs.is_empty() {
        return "anyopaque".to_string();
    }
    // Validate each segment is a plain identifier; otherwise fall back to last segment
    let all_plain = segs
        .iter()
        .all(|seg| seg.chars().all(|c| c.is_ascii_alphanumeric() || c == '_'));
    if !all_plain {
        let last = segs.last().copied().unwrap_or("anyopaque");
        return sanitize_cp_ident(last);
    }
    // Check if any non-last segment is a CP keyword (like `type`, `in`, `fn`, `from`)
    if segs.len() > 1 {
        let last = *segs.last().unwrap();
        let has_kw = segs[..segs.len() - 1]
            .iter()
            .any(|seg| sanitize_cp_ident(seg) != *seg);
        if has_kw {
            return last.to_string();
        }
    }
    segs.join(".")
}

fn default_value_for_cp_type(ty: &str) -> &'static str {
    let t = ty.trim();
    if t.ends_with('?') {
        return "null";
    }
    match t {
        "bool" => "false",
        "string" => "\"\"",
        "i8" | "i16" | "i32" | "i64" | "isize" | "u8" | "u16" | "u32" | "u64" | "usize" => "0",
        "f32" | "f64" => "0.0",
        _ => "",
    }
}

/// Strip inline `#[...]` attributes inside parameter lists or field lists.
fn strip_inline_attrs(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    let bytes = s.as_bytes();
    let len = bytes.len();
    let mut i = 0;
    while i < len {
        if bytes[i] == b'#' && i + 1 < len && bytes[i + 1] == b'[' {
            if let Some(end) = find_matching_delim(s, i + 1, '[', ']') {
                i = end + 1;
                continue;
            }
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

fn convert_rust_params(
    params_raw: &str,
    in_impl_or_trait: bool,
    allow_receiver_shorthand: bool,
) -> (String, bool) {
    let cleaned = strip_inline_attrs(params_raw);
    let mut out_params = Vec::new();
    let mut is_static_method = in_impl_or_trait;
    let mut anon_idx = 0usize;

    for part in split_top_level_commas(&cleaned) {
        let p = strip_rust_lifetimes(&part).trim().to_string();
        if p.is_empty() {
            continue;
        }
        // Check receiver forms (`self`, `&self`, `&mut self`, `mut self`, `self: Pin<&mut Self>`)
        let is_self_receiver = p == "&self"
            || p == "& self"
            || p == "&mut self"
            || p == "& mut self"
            || p == "self"
            || p == "mut self"
            || p.starts_with("self:")
            || p.starts_with("self :")
            || p.starts_with("mut self:");
        if is_self_receiver {
            is_static_method = false;
            if allow_receiver_shorthand {
                if p == "&self" || p == "& self" {
                    // CP read-only receiver is implicit!
                    continue;
                }
                if p == "&mut self"
                    || p == "& mut self"
                    || p.contains("&mut")
                    || p.contains("& mut")
                {
                    out_params.push("*_".to_string());
                    continue;
                }
                out_params.push("move _".to_string());
                continue;
            } else {
                // In free functions (e.g. hoisted inherent impl), pass explicit `self_` parameter
                if p == "&self" || p == "& self" {
                    out_params.push("self_: *const Self".to_string());
                } else if p == "&mut self" || p == "& mut self" {
                    out_params.push("self_: *Self".to_string());
                } else {
                    out_params.push("self_: Self".to_string());
                }
                continue;
            }
        }

        if let Some(colon) = find_top_level_colon(&p) {
            let lhs = p[..colon].trim();
            let rhs = p[colon + 1..].trim();
            // Handle destructuring patterns like `State(state)` or `(a, b)` or `mut x`
            let raw_name = lhs
                .trim_start_matches("mut ")
                .trim_start_matches("ref ")
                .trim();
            let pname = if raw_name.contains('(')
                || raw_name.contains('{')
                || raw_name.contains('[')
                || raw_name.contains(' ')
                || raw_name == "_"
            {
                anon_idx += 1;
                format!("_arg{}", anon_idx)
            } else {
                sanitize_cp_ident(raw_name)
            };
            let pty = map_rust_type(rhs);
            let pty = if pty == "void" {
                "*anyopaque".to_string()
            } else {
                pty
            };
            out_params.push(format!("{}: {}", pname, pty));
        } else {
            anon_idx += 1;
            out_params.push(format!("_arg{}: *anyopaque", anon_idx));
        }
    }
    (out_params.join(", "), is_static_method)
}

fn find_top_level_colon(s: &str) -> Option<usize> {
    let bytes = s.as_bytes();
    let mut depth_paren = 0i32;
    let mut depth_brack = 0i32;
    let mut depth_brace = 0i32;
    let mut depth_angle = 0i32;
    let mut i = 0;
    while i < bytes.len() {
        match bytes[i] {
            b'(' => depth_paren += 1,
            b')' => depth_paren = (depth_paren - 1).max(0),
            b'[' => depth_brack += 1,
            b']' => depth_brack = (depth_brack - 1).max(0),
            b'{' => depth_brace += 1,
            b'}' => depth_brace = (depth_brace - 1).max(0),
            b'<' => depth_angle += 1,
            b'>' => depth_angle = (depth_angle - 1).max(0),
            b':' if depth_paren == 0
                && depth_brack == 0
                && depth_brace == 0
                && depth_angle == 0 =>
            {
                if i + 1 < bytes.len() && bytes[i + 1] == b':' {
                    i += 2;
                    continue;
                }
                return Some(i);
            }
            _ => {}
        }
        i += 1;
    }
    None
}

/// Recursively extract imported leaf identifiers from a Rust `use` tree (handles nested `{ ... }`).
fn extract_use_symbols(tree: &str, out: &mut Vec<String>) {
    let t = tree.trim();
    if t.is_empty() || t == "*" || t == "self" {
        return;
    }
    if let Some(brace_start) = t.find('{') {
        if let Some(brace_end) = find_matching_delim(t, brace_start, '{', '}') {
            let inner = &t[brace_start + 1..brace_end];
            for item in split_top_level_commas(inner) {
                extract_use_symbols(&item, out);
            }
            return;
        }
    }
    // Handle `foo::bar as Baz` or `foo::bar`
    let leaf = if let Some(as_idx) = t.find(" as ") {
        t[as_idx + 4..].trim()
    } else {
        t.rsplit("::").next().unwrap_or("").trim()
    };
    if !leaf.is_empty()
        && leaf != "self"
        && leaf != "*"
        && leaf != "_"
        && leaf.chars().all(|c| c.is_ascii_alphanumeric() || c == '_')
    {
        if !out.contains(&leaf.to_string()) {
            out.push(leaf.to_string());
        }
    }
}

fn convert_rust_use(stmt: &str, dep_map: &HashMap<String, DepTier>) -> Option<String> {
    let trimmed = stmt.trim();
    let is_pub = trimmed.starts_with("pub");
    // Strip `pub`, `pub(crate)`, `pub(super)`, `pub(in ...)`
    let after_vis = if let Some(rest) = trimmed.strip_prefix("pub") {
        let r = rest.trim_start();
        if r.starts_with('(') {
            if let Some(end) = find_matching_delim(r, 0, '(', ')') {
                r[end + 1..].trim_start()
            } else {
                r
            }
        } else {
            r
        }
    } else {
        trimmed
    };
    let rest = after_vis.strip_prefix("use ")?;
    let path = rest.trim_end_matches(';').trim().trim_start_matches("::");
    if path.is_empty() {
        return None;
    }

    let (pkg_root, remainder) = if let Some(idx) = path.find("::") {
        (&path[..idx], &path[idx + 2..])
    } else {
        (path, "")
    };

    if pkg_root == "std" || pkg_root == "core" || pkg_root == "alloc" {
        let single_line = path.split_whitespace().collect::<Vec<_>>().join(" ");
        return Some(format!(
            "// [cap-migrate] Rust `{}` -> CP built-in primitives / stdlib",
            single_line
        ));
    }

    let first_mod = remainder
        .split(|c: char| c == ':' || c == '{' || c.is_whitespace())
        .find(|s| !s.is_empty())
        .unwrap_or("index");

    let import_src = if pkg_root == "crate" || pkg_root == "self" {
        if first_mod
            .chars()
            .all(|c| c.is_ascii_alphanumeric() || c == '_')
        {
            format!("./{}.cp", first_mod)
        } else {
            "./index.cp".to_string()
        }
    } else if pkg_root == "super" {
        if first_mod
            .chars()
            .all(|c| c.is_ascii_alphanumeric() || c == '_')
        {
            format!("../{}.cp", first_mod)
        } else {
            "../index.cp".to_string()
        }
    } else {
        rewrite_import_source(SourceLang::Rs, pkg_root, dep_map)
    };

    let mut symbols = Vec::new();
    if !remainder.is_empty() {
        extract_use_symbols(remainder, &mut symbols);
    }

    // CP parser supports `import { ... } from "...";` cleanly; for `pub use` emit `import` + re-export comment
    // so we don't hit unsupported `export { ... } from` forms if any.
    if symbols.is_empty() {
        Some(format!(
            "// [cap-migrate] {} * from \"{}\";",
            if is_pub { "export" } else { "import" },
            import_src
        ))
    } else if is_pub {
        Some(format!(
            "import {{ {} }} from \"{}\"; // pub re-export",
            symbols.join(", "),
            import_src
        ))
    } else {
        Some(format!(
            "import {{ {} }} from \"{}\";",
            symbols.join(", "),
            import_src
        ))
    }
}

/// Convert a Rust expression/statement line into CP syntax.
fn convert_rust_expr_line(line: &str) -> String {
    let mut s = strip_rust_lifetimes(line);
    s = s.replace("self.", "_.");
    s = s.replace("let mut ", "let ");
    s = s.replace("println!(", "@print(");
    s = s.replace("eprintln!(", "@print(");
    s = s.replace("panic!(", "@panic(");
    s = s.replace("unreachable!()", "unreachable");
    s = s.replace("unimplemented!()", "@panic(\"unimplemented\")");
    s = s.replace("todo!()", "@panic(\"todo\")");
    // Strip Rust turbofish `::<...>` lightly
    s = s.replace("::<", "<");
    // Replace remaining `::` with `.` outside strings
    if !s.contains("://") {
        s = s.replace("::", ".");
    }
    s
}

fn wrap_rust_control_condition(trimmed: &str, kw: &str) -> Option<String> {
    if kw == "loop" && (trimmed == "loop {" || trimmed.starts_with("loop {")) {
        return Some(trimmed.replacen("loop {", "while (true) {", 1));
    }
    let prefix = format!("{} ", kw);
    if !trimmed.starts_with(&prefix) {
        return None;
    }
    let after = trimmed[prefix.len()..].trim();
    if after.starts_with('(') {
        return None;
    }
    // Handle `if let Some(x) = expr {` -> `if (x := expr) {`
    if kw == "if" {
        if let Some(let_rest) = after.strip_prefix("let Some(") {
            if let Some(close_idx) = let_rest.find(')') {
                let var = sanitize_cp_ident(let_rest[..close_idx].trim());
                let rem = let_rest[close_idx + 1..].trim();
                if let Some(eq_rest) = rem.strip_prefix('=') {
                    if let Some(brace_idx) = eq_rest.rfind('{') {
                        let expr = convert_rust_expr_line(eq_rest[..brace_idx].trim());
                        return Some(format!("if ({} := {}) {{", var, expr));
                    }
                }
            }
        }
    }
    // Handle `for x in iter {` -> `for (x of iter) {`
    if kw == "for" {
        if let Some(brace_idx) = after.rfind('{') {
            let cond = after[..brace_idx].trim();
            if let Some(in_idx) = cond.find(" in ") {
                let var = sanitize_cp_ident(cond[..in_idx].trim());
                let iter = convert_rust_expr_line(cond[in_idx + 4..].trim());
                return Some(format!("for ({} of {}) {{", var, iter));
            }
        }
    }
    // Handle `match expr {` -> `switch (expr) {`
    if kw == "match" {
        if let Some(brace_idx) = after.rfind('{') {
            let cond = convert_rust_expr_line(after[..brace_idx].trim());
            return Some(format!("switch ({}) {{", cond));
        }
    }
    if let Some(brace_idx) = after.rfind('{') {
        let cond = convert_rust_expr_line(after[..brace_idx].trim());
        return Some(format!("{} ({}) {{", kw, cond));
    }
    None
}

/// Normalize `pub(crate)`, `pub(super)`, `pub(in ...)` to `pub`.
fn normalize_rust_vis(s: &str) -> String {
    let trimmed = s.trim();
    if let Some(rest) = trimmed.strip_prefix("pub(") {
        if let Some(close) = find_matching_delim(trimmed, 3, '(', ')') {
            let after = trimmed[close + 1..].trim_start();
            return format!("pub {}", after);
        }
        let _ = rest;
    }
    trimmed.to_string()
}

/// Check if a trimmed line starts a top-level or item-level Rust macro invocation like
/// `define_rejection! {`, `axum_core::__impl_deref!(...)`, `all_the_tuples!(...)`.
fn is_rust_macro_invocation_start(trimmed: &str) -> bool {
    let code = strip_inline_comment(trimmed).trim();
    if code.is_empty() || code.starts_with("//") {
        return false;
    }
    if let Some(bang) = code.find('!') {
        let prefix = code[..bang].trim();
        let after = code[bang + 1..].trim_start();
        if prefix.is_empty() {
            return false;
        }
        // Prefix must be a valid Rust path `ident(::ident)*`
        let valid_path = prefix.split("::").all(|seg| {
            !seg.is_empty() && seg.chars().all(|c| c.is_ascii_alphanumeric() || c == '_')
        });
        if !valid_path {
            return false;
        }
        // Must not be a Rust keyword like `if`, `return`, `let`, `const`, `pub`
        if matches!(
            prefix,
            "if" | "while"
                | "for"
                | "match"
                | "return"
                | "let"
                | "const"
                | "pub"
                | "fn"
                | "struct"
                | "enum"
                | "trait"
                | "impl"
                | "type"
                | "use"
                | "mod"
        ) {
            return false;
        }
        if after.starts_with('(') || after.starts_with('{') || after.starts_with('[') {
            return true;
        }
    }
    false
}

/// Check if a line starts a Rust function declaration.
fn is_rust_fn_start(norm: &str) -> bool {
    let mut s = norm.trim();
    for prefix in [
        "pub ",
        "default ",
        "const ",
        "async ",
        "unsafe ",
        "extern \"C\" ",
        "extern ",
    ] {
        if let Some(rest) = s.strip_prefix(prefix) {
            s = rest.trim_start();
        }
    }
    s.starts_with("fn ")
}

/// Verify a standalone CP snippet parses cleanly with `cpc::parser::Parser`.
fn snippet_parses_ok(cp_snippet: &str) -> bool {
    let mut parser = Parser::new(cp_snippet);
    parser.parse_module().is_ok()
}

/// Collect a balanced delimited block starting at `lines[start_idx]` (where `lines[start_idx]`
/// contains the opening `{`, `(`, or `[`), returning `(collected_lines, next_idx)`.
fn collect_balanced_block<'a>(lines: &[&'a str], start_idx: usize) -> (Vec<&'a str>, usize) {
    let mut collected = Vec::new();
    let mut b_depth = 0i32;
    let mut p_depth = 0i32;
    let mut k_depth = 0i32;
    let mut saw_any = false;
    let mut i = start_idx;

    while i < lines.len() {
        let line = lines[i];
        collected.push(line);
        let (db, dp, dk) = scan_delimiters(line);
        if db != 0 || dp != 0 || dk != 0 {
            saw_any = true;
        }
        b_depth += db;
        p_depth += dp;
        k_depth += dk;
        i += 1;
        if saw_any && b_depth <= 0 && p_depth <= 0 && k_depth <= 0 {
            break;
        }
        // Single-line statement ending in `;` with no open delimiters
        if !saw_any && strip_inline_comment(line).trim_end().ends_with(';') {
            break;
        }
    }
    (collected, i)
}

/// Collect a Rust item header (struct/enum/trait/impl/fn) from `start_idx` until the
/// line containing the terminating `{` or `;` at paren/bracket/angle depth 0.
fn collect_header_until_brace_or_semi<'a>(
    lines: &[&'a str],
    start_idx: usize,
) -> (String, char, usize) {
    let mut header = String::new();
    let mut p_depth = 0i32;
    let mut k_depth = 0i32;
    let mut i = start_idx;

    while i < lines.len() {
        let trimmed = strip_inline_comment(lines[i]).trim();
        if !trimmed.is_empty() && !trimmed.starts_with("//") {
            if !header.is_empty() {
                header.push(' ');
            }
            header.push_str(trimmed);
            let (_, dp, dk) = scan_delimiters(trimmed);
            p_depth += dp;
            k_depth += dk;
            if p_depth <= 0 && k_depth <= 0 {
                if trimmed.ends_with('{') {
                    return (header, '{', i + 1);
                }
                if trimmed.ends_with(';') {
                    return (header, ';', i + 1);
                }
            }
        }
        i += 1;
    }
    (header, ';', i)
}

/// Collect a `{ ... }` body whose opening `{` was already consumed on the previous line.
fn collect_brace_body<'a>(lines: &[&'a str], start_idx: usize) -> (Vec<&'a str>, usize) {
    let mut body = Vec::new();
    let mut depth = 1i32;
    let mut i = start_idx;
    while i < lines.len() {
        let line = lines[i];
        let (db, _, _) = scan_delimiters(line);
        depth += db;
        i += 1;
        if depth <= 0 {
            break;
        }
        body.push(line);
    }
    (body, i)
}

/// Split a header at the top-level `where` keyword (if present).
fn split_where_clause(header: &str) -> (&str, Option<&str>) {
    // Look for ` where ` or ` where\t` outside parens/angles
    let bytes = header.as_bytes();
    let mut p_depth = 0i32;
    let mut a_depth = 0i32;
    let mut i = 0;
    while i < bytes.len() {
        match bytes[i] {
            b'(' => p_depth += 1,
            b')' => p_depth = (p_depth - 1).max(0),
            b'<' => a_depth += 1,
            b'>' => a_depth = (a_depth - 1).max(0),
            _ => {}
        }
        if p_depth == 0 && a_depth == 0 && header[i..].starts_with("where") {
            let prev_ok = i == 0 || !bytes[i - 1].is_ascii_alphanumeric() && bytes[i - 1] != b'_';
            let next_idx = i + 5;
            let next_ok = next_idx >= bytes.len()
                || (!bytes[next_idx].is_ascii_alphanumeric() && bytes[next_idx] != b'_');
            if prev_ok && next_ok {
                return (header[..i].trim(), Some(header[i..].trim()));
            }
        }
        i += 1;
    }
    (header.trim(), None)
}

/// Convert a Rust struct body lines into CP struct field declarations.
fn convert_struct_fields(body_lines: &[&str], indent: &str) -> String {
    let mut out = String::new();
    let mut i = 0;
    while i < body_lines.len() {
        let trimmed = body_lines[i].trim();
        if trimmed.is_empty() {
            i += 1;
            continue;
        }
        if trimmed.starts_with("//") {
            out.push_str(&format!("{}    {}\n", indent, trimmed));
            i += 1;
            continue;
        }
        // Skip multi-line field attribute `#[...]`
        if trimmed.starts_with("#[") {
            let (_, next_i) = collect_balanced_block(body_lines, i);
            i = next_i;
            continue;
        }
        // Accumulate multi-line field until `,` at depth 0
        let mut field_buf = String::new();
        let mut p_d = 0i32;
        let mut k_d = 0i32;
        let mut b_d = 0i32;
        while i < body_lines.len() {
            let ln = strip_inline_comment(body_lines[i]).trim();
            if !ln.is_empty() && !ln.starts_with("//") {
                if !field_buf.is_empty() {
                    field_buf.push(' ');
                }
                field_buf.push_str(ln);
                let (db, dp, dk) = scan_delimiters(ln);
                b_d += db;
                p_d += dp;
                k_d += dk;
                i += 1;
                if b_d <= 0 && p_d <= 0 && k_d <= 0 && ln.ends_with(',') {
                    break;
                }
            } else {
                i += 1;
            }
        }
        let field_norm = normalize_rust_vis(&field_buf);
        let field_clean = field_norm
            .trim_start_matches("pub ")
            .trim_end_matches(',')
            .trim();
        if let Some(colon) = find_top_level_colon(field_clean) {
            let fname = sanitize_cp_ident(&field_clean[..colon]);
            let fty = map_rust_type(&field_clean[colon + 1..]);
            let fty = if fty == "void" {
                "*anyopaque".to_string()
            } else {
                pty_or_fallback(&fty)
            };
            let def = default_value_for_cp_type(&fty);
            if !def.is_empty() {
                out.push_str(&format!("{}    {}: {} = {};\n", indent, fname, fty, def));
            } else {
                out.push_str(&format!("{}    {}: {};\n", indent, fname, fty));
            }
        }
    }
    out
}

fn pty_or_fallback(ty: &str) -> String {
    let t = ty.trim();
    if t.is_empty() || t == "void" {
        "*anyopaque".to_string()
    } else {
        t.to_string()
    }
}

/// Convert a Rust enum body into CP Tagged Union / Enum variants.
fn convert_enum_variants(body_lines: &[&str], indent: &str) -> String {
    let mut out = String::new();
    let mut i = 0;
    while i < body_lines.len() {
        let trimmed = body_lines[i].trim();
        if trimmed.is_empty() {
            i += 1;
            continue;
        }
        if trimmed.starts_with("//") {
            out.push_str(&format!("{}    {}\n", indent, trimmed));
            i += 1;
            continue;
        }
        if trimmed.starts_with("#[") {
            let (_, next_i) = collect_balanced_block(body_lines, i);
            i = next_i;
            continue;
        }
        // Accumulate variant until top-level `,` or end of body
        let mut var_buf = String::new();
        let mut b_d = 0i32;
        let mut p_d = 0i32;
        let mut k_d = 0i32;
        while i < body_lines.len() {
            let ln = strip_inline_comment(body_lines[i]).trim();
            if ln.starts_with("#[") {
                let (_, next_i) = collect_balanced_block(body_lines, i);
                i = next_i;
                continue;
            }
            if !ln.is_empty() && !ln.starts_with("//") {
                if !var_buf.is_empty() {
                    var_buf.push(' ');
                }
                var_buf.push_str(ln);
                let (db, dp, dk) = scan_delimiters(ln);
                b_d += db;
                p_d += dp;
                k_d += dk;
                i += 1;
                if b_d <= 0 && p_d <= 0 && k_d <= 0 && (ln.ends_with(',') || ln.ends_with('}')) {
                    break;
                }
            } else {
                i += 1;
            }
        }
        let v = strip_inline_attrs(var_buf.trim().trim_end_matches(',').trim());
        let v = v.trim();
        if v.is_empty() {
            continue;
        }
        // Struct variant: `Name { a: T1, b: T2 }` -> `Name: { a: CpT1, b: CpT2 },`
        if let Some(lb) = v.find('{') {
            if let Some(rb) = v.rfind('}') {
                let vname = sanitize_cp_ident(&v[..lb]);
                let inner = &v[lb + 1..rb];
                let mut fields = Vec::new();
                for f in split_top_level_commas(inner) {
                    let f = f.trim();
                    if let Some(colon) = find_top_level_colon(f) {
                        let fnm = sanitize_cp_ident(&f[..colon]);
                        let fty = pty_or_fallback(&map_rust_type(&f[colon + 1..]));
                        fields.push(format!("{}: {}", fnm, fty));
                    }
                }
                if fields.is_empty() {
                    out.push_str(&format!("{}    {},\n", indent, vname));
                } else {
                    out.push_str(&format!(
                        "{}    {}: {{ {} }},\n",
                        indent,
                        vname,
                        fields.join(", ")
                    ));
                }
                continue;
            }
        }
        // Tuple variant: `Name(T1, T2)` -> `Name: T1,` or `Name: [T1, T2],`
        if let Some(lp) = v.find('(') {
            if let Some(rp) = v.rfind(')') {
                let vname = sanitize_cp_ident(&v[..lp]);
                let inner = &v[lp + 1..rp];
                let tys: Vec<String> = split_top_level_commas(inner)
                    .into_iter()
                    .map(|t| pty_or_fallback(&map_rust_type(&t)))
                    .collect();
                if tys.is_empty() {
                    out.push_str(&format!("{}    {},\n", indent, vname));
                } else if tys.len() == 1 {
                    out.push_str(&format!("{}    {}: {},\n", indent, vname, tys[0]));
                } else {
                    out.push_str(&format!("{}    {}: [{}],\n", indent, vname, tys.join(", ")));
                }
                continue;
            }
        }
        // Discriminant variant: `Name = 1`
        if let Some(eq) = v.find('=') {
            let vname = sanitize_cp_ident(&v[..eq]);
            let val = convert_rust_expr_line(v[eq + 1..].trim());
            out.push_str(&format!("{}    {} = {},\n", indent, vname, val));
            continue;
        }
        // Unit variant
        let vname = sanitize_cp_ident(v);
        out.push_str(&format!("{}    {},\n", indent, vname));
    }
    out
}

/// Convert a Rust function header + optional body into a guaranteed-parseable CP function.
#[allow(clippy::too_many_arguments)]
fn convert_rust_function(
    header_raw: &str,
    term_char: char,
    body_lines: Option<&[&str]>,
    indent: &str,
    in_trait: bool,
    in_impl: bool,
    hoisted_prefix: Option<&str>,
    todo_count: &mut usize,
) -> String {
    let (head_no_where, where_clause) = split_where_clause(header_raw);
    let head_clean = head_no_where
        .trim_end_matches('{')
        .trim_end_matches(';')
        .trim();
    let norm = normalize_rust_vis(head_clean);

    let is_pub = !in_trait && !in_impl && norm.starts_with("pub ");
    let is_async = !in_trait && norm.contains("async fn ");
    let is_unsafe = norm.contains("unsafe fn ");

    let fn_idx = match norm.find("fn ") {
        Some(idx) => idx,
        None => {
            *todo_count += 1;
            return format!(
                "{}// TODO(cap-migrate): unparsed fn `{}`\n",
                indent,
                header_raw.trim()
            );
        }
    };
    let after_fn = &norm[fn_idx + 3..];
    let lp = match after_fn.find('(') {
        Some(idx) => idx,
        None => {
            *todo_count += 1;
            return format!(
                "{}// TODO(cap-migrate): unparsed fn `{}`\n",
                indent,
                header_raw.trim()
            );
        }
    };
    let rp = match find_matching_delim(after_fn, lp, '(', ')') {
        Some(idx) => idx,
        None => {
            *todo_count += 1;
            return format!(
                "{}// TODO(cap-migrate): unparsed fn `{}`\n",
                indent,
                header_raw.trim()
            );
        }
    };

    // Split function name & generic params
    let name_and_gen = after_fn[..lp].trim();
    let (raw_name, gen_str) = if let Some(lt) = name_and_gen.find('<') {
        (
            name_and_gen[..lt].trim(),
            clean_generic_params(&name_and_gen[lt..]),
        )
    } else {
        (name_and_gen, String::new())
    };
    let base_name = sanitize_cp_ident(raw_name);
    let fn_name = if let Some(pfx) = hoisted_prefix {
        format!("{}_{}", pfx, base_name)
    } else {
        base_name
    };

    let params_raw = &after_fn[lp + 1..rp];
    let allow_shorthand = in_trait || in_impl;
    let (cp_params, is_static) =
        convert_rust_params(params_raw, in_trait || in_impl, allow_shorthand);

    let ret_part = after_fn[rp + 1..].trim();
    let ret_str = if let Some(r) = ret_part.strip_prefix("->") {
        let mapped = map_rust_type(r.trim());
        if mapped.is_empty() || mapped == "void" {
            String::new()
        } else if mapped == "!" {
            ": !".to_string()
        } else {
            format!(": {}", mapped)
        }
    } else {
        String::new()
    };

    let mut sig = String::new();
    if is_pub {
        sig.push_str("pub ");
    }
    if in_impl && is_static {
        sig.push_str("static ");
    }
    if is_async {
        sig.push_str("async ");
    }
    if is_unsafe {
        sig.push_str("unsafe ");
    }
    sig.push_str(&format!(
        "fn {}{}({}){}",
        fn_name, gen_str, cp_params, ret_str
    ));

    let mut prefix_comments = String::new();
    if let Some(wc) = where_clause {
        prefix_comments.push_str(&format!(
            "{}// [rs-where] {}\n",
            indent,
            wc.trim_end_matches('{').trim()
        ));
    }

    if term_char == ';' || body_lines.is_none() {
        let candidate = if in_trait {
            format!("trait __Tmp {{ {}; }}", sig)
        } else {
            format!("{}{} {{}}", prefix_comments, sig)
        };
        if !in_trait && !snippet_parses_ok(&candidate) {
            *todo_count += 1;
            return format!(
                "{}{}// TODO(cap-migrate): fn signature `{}`\n",
                prefix_comments, indent, sig
            );
        }
        if in_trait {
            return format!("{}{}{};\n", prefix_comments, indent, sig);
        }
        return format!("{}{}{} {{}}\n", prefix_comments, indent, sig);
    }

    // Convert function body lines
    let body = body_lines.unwrap();
    let mut converted_body = String::new();
    for &bline in body {
        let btrimmed = bline.trim();
        let bindent_len = bline.len() - bline.trim_start().len();
        let bindent = &bline[..bindent_len];
        if btrimmed.is_empty() {
            converted_body.push('\n');
            continue;
        }
        if btrimmed.starts_with("//") {
            converted_body.push_str(bline);
            converted_body.push('\n');
            continue;
        }
        if btrimmed.starts_with("#[") {
            converted_body.push_str(&format!("{}// [rs-attr] {}\n", bindent, btrimmed));
            continue;
        }
        let norm_b = normalize_rust_vis(btrimmed);
        if let Some(wrapped) = wrap_rust_control_condition(&norm_b, "if")
            .or_else(|| wrap_rust_control_condition(&norm_b, "while"))
            .or_else(|| wrap_rust_control_condition(&norm_b, "for"))
            .or_else(|| wrap_rust_control_condition(&norm_b, "match"))
            .or_else(|| wrap_rust_control_condition(&norm_b, "loop"))
        {
            converted_body.push_str(&format!("{}{}\n", bindent, wrapped));
            continue;
        }
        let conv = convert_rust_expr_line(&norm_b);
        converted_body.push_str(&format!("{}{}\n", bindent, conv));
    }

    let full_fn = format!(
        "{}{}{} {{\n{}{}}}\n",
        prefix_comments, indent, sig, converted_body, indent
    );

    // Self-check parseability of this single function
    let check_wrapper = if in_trait {
        format!("trait __Tmp {{\n{}\n}}", full_fn)
    } else if in_impl {
        format!("impl __Trait for __Target {{\n{}\n}}", full_fn)
    } else {
        full_fn.clone()
    };

    if snippet_parses_ok(&check_wrapper) {
        return full_fn;
    }

    // Fallback: preserve the clean CP signature and comment the Rust body with a TODO stub
    *todo_count += 1;
    let mut fallback = String::new();
    fallback.push_str(&prefix_comments);
    fallback.push_str(&format!("{}{} {{\n", indent, sig));
    fallback.push_str(&format!(
        "{}    // TODO(cap-migrate): review and migrate Rust function body\n",
        indent
    ));
    for &bline in body {
        let bt = bline.trim();
        if !bt.is_empty() {
            fallback.push_str(&format!("{}    // [rs] {}\n", indent, bt));
        }
    }
    fallback.push_str(&format!(
        "{}    @panic(\"TODO(cap-migrate): {}\");\n",
        indent, fn_name
    ));
    fallback.push_str(&format!("{}}}\n", indent));

    // Verify fallback itself parses; if even signature had an exotic type, degrade signature safely
    let fb_wrapper = if in_trait {
        format!("trait __Tmp {{\n{}\n}}", fallback)
    } else if in_impl {
        format!("impl __Trait for __Target {{\n{}\n}}", fallback)
    } else {
        fallback.clone()
    };
    if snippet_parses_ok(&fb_wrapper) {
        fallback
    } else {
        format!(
            "{}{}// TODO(cap-migrate): complex Rust fn `{}`\n",
            prefix_comments, indent, sig
        )
    }
}

/// Convert the items inside a `trait` or `impl` block.
fn convert_member_block(
    body_lines: &[&str],
    indent: &str,
    in_trait: bool,
    in_impl: bool,
    hoisted_prefix: Option<&str>,
    todo_count: &mut usize,
) -> String {
    let mut out = String::new();
    let mut i = 0;
    while i < body_lines.len() {
        let line = body_lines[i];
        let trimmed = line.trim();
        if trimmed.is_empty() {
            out.push('\n');
            i += 1;
            continue;
        }
        if trimmed.starts_with("//") {
            out.push_str(&format!("{}{}\n", indent, trimmed));
            i += 1;
            continue;
        }
        if trimmed.starts_with("#[") || trimmed.starts_with("#![") {
            let (_, next_i) = collect_balanced_block(body_lines, i);
            i = next_i;
            continue;
        }
        let norm = normalize_rust_vis(trimmed);
        // Associated type / const inside trait or impl (unsupported by CP parse_trait/parse_impl)
        if norm.starts_with("type ")
            || norm.starts_with("pub type ")
            || norm.starts_with("const ")
            || norm.starts_with("pub const ")
        {
            let (hdr, _, next_i) = collect_header_until_brace_or_semi(body_lines, i);
            out.push_str(&format!("{}// [rs-assoc] {}\n", indent, hdr));
            i = next_i;
            continue;
        }
        // Macro invocation inside impl/trait
        if is_rust_macro_invocation_start(trimmed) {
            let (blk, next_i) = collect_balanced_block(body_lines, i);
            *todo_count += 1;
            out.push_str(&format!(
                "{}// TODO(cap-migrate): Rust macro `{}`\n",
                indent,
                blk.first().copied().unwrap_or("").trim()
            ));
            i = next_i;
            continue;
        }
        // Function / method inside trait or impl
        if is_rust_fn_start(&norm) {
            let (hdr, term, next_i) = collect_header_until_brace_or_semi(body_lines, i);
            if term == '{' {
                let (fn_body, after_body) = collect_brace_body(body_lines, next_i);
                let fn_cp = convert_rust_function(
                    &hdr,
                    '{',
                    Some(&fn_body),
                    indent,
                    in_trait,
                    in_impl,
                    hoisted_prefix,
                    todo_count,
                );
                out.push_str(&fn_cp);
                i = after_body;
            } else {
                let fn_cp = convert_rust_function(
                    &hdr,
                    ';',
                    None,
                    indent,
                    in_trait,
                    in_impl,
                    hoisted_prefix,
                    todo_count,
                );
                out.push_str(&fn_cp);
                i = next_i;
            }
            continue;
        }
        // Unrecognized line inside trait/impl
        out.push_str(&format!("{}// [rs-member] {}\n", indent, trimmed));
        i += 1;
    }
    out
}

/// Extract clean simple identifier from a Rust type/trait path like `axum_core::extract::FromRequest<S>`.
fn extract_simple_ident(raw: &str) -> Option<String> {
    let stripped = strip_rust_lifetimes(raw);
    let base = stripped.split('<').next().unwrap_or("").trim();
    if base.starts_with('(')
        || base.starts_with('[')
        || base.starts_with('&')
        || base.starts_with('*')
        || base.starts_with("dyn ")
    {
        return None;
    }
    let last = base.rsplit("::").next().unwrap_or("").trim();
    if !last.is_empty() && last.chars().all(|c| c.is_ascii_alphanumeric() || c == '_') {
        Some(sanitize_cp_ident(last))
    } else {
        None
    }
}

pub fn transpile_rs_to_cp(
    source: &str,
    rel_path: &str,
    dep_map: &HashMap<String, DepTier>,
) -> RsTranspileResult {
    let preprocessed = preprocess_rust_source(source);
    let lines: Vec<&str> = preprocessed.lines().collect();

    let mut out = String::with_capacity(source.len() + 512);
    let mut todo_count: usize = 0;
    let mut opt_notes: Vec<String> = Vec::new();
    let mut saw_lifetime = false;
    let mut saw_async_box = false;
    let mut saw_macro_rules = false;

    out.push_str(&format!(
        "// Migrated from Rust ({}) via `cap from -rs`\n",
        rel_path
    ));
    out.push_str("// CP enhancements: lifetime erasure, unified `_`/`*_` receivers, zero-cost stack layout.\n\n");

    let mut in_cfg_test = false;
    let mut i = 0;

    while i < lines.len() {
        let raw_line = lines[i];
        let indent_len = raw_line.len() - raw_line.trim_start().len();
        let indent = &raw_line[..indent_len];
        let trimmed = raw_line.trim();

        if trimmed.contains('\'')
            && (trimmed.contains("<'") || trimmed.contains("&'") || trimmed.contains("'static"))
        {
            saw_lifetime = true;
        }
        if trimmed.contains("Pin<Box<") || trimmed.contains("BoxFuture") {
            saw_async_box = true;
        }

        if trimmed.is_empty() {
            out.push('\n');
            i += 1;
            continue;
        }

        // Preserve doc comments and regular comments
        if trimmed.starts_with("//") {
            out.push_str(raw_line);
            out.push('\n');
            i += 1;
            continue;
        }

        // 1. Multi-line Rust attributes `#[...]` / `#![...]`
        if trimmed.starts_with("#[") || trimmed.starts_with("#![") {
            let (attr_lines, next_i) = collect_balanced_block(&lines, i);
            let joined = attr_lines.join(" ");
            if joined.contains("cfg(test)") {
                in_cfg_test = true;
            }
            i = next_i;
            continue;
        }

        // 2. Skip `#[cfg(test)]` items safely
        if in_cfg_test {
            in_cfg_test = false;
            let (blk, next_i) = collect_balanced_block(&lines, i);
            for bl in blk {
                out.push_str(&format!("{}// [rs-test] {}\n", indent, bl.trim()));
            }
            i = next_i;
            continue;
        }

        // 3. `macro_rules!` definitions
        if trimmed.starts_with("macro_rules!") {
            saw_macro_rules = true;
            todo_count += 1;
            let (blk, next_i) = collect_balanced_block(&lines, i);
            out.push_str(&format!(
                "{}// TODO(cap-migrate): replace Rust macro `{}` with CP comptime generic\n",
                indent,
                blk.first().copied().unwrap_or(trimmed).trim()
            ));
            for bl in blk.iter().skip(1) {
                out.push_str(&format!("{}// [rs-macro] {}\n", indent, bl.trim()));
            }
            i = next_i;
            continue;
        }

        // 4. Multi-line `use ...;` / `pub use ...;` / `pub(crate) use ...;`
        let norm = normalize_rust_vis(trimmed);
        if norm.starts_with("use ") || norm.starts_with("pub use ") {
            let mut use_buf = String::new();
            let mut j = i;
            while j < lines.len() {
                let ln = strip_inline_comment(lines[j]).trim();
                if !ln.is_empty() && !ln.starts_with("//") {
                    if !use_buf.is_empty() {
                        use_buf.push(' ');
                    }
                    use_buf.push_str(ln);
                    j += 1;
                    if ln.ends_with(';') {
                        break;
                    }
                } else {
                    j += 1;
                }
            }
            if let Some(conv) = convert_rust_use(&use_buf, dep_map) {
                if snippet_parses_ok(&conv) {
                    out.push_str(&format!("{}{}\n", indent, conv));
                } else {
                    out.push_str(&format!("{}// [cap-migrate] {}\n", indent, conv));
                }
            }
            i = j;
            continue;
        }

        // 5. `extern crate foo;`
        if norm.starts_with("extern crate ") || norm.starts_with("pub extern crate ") {
            out.push_str(&format!("{}// [cap-migrate] {}\n", indent, trimmed));
            i += 1;
            continue;
        }

        // 6. `mod foo;` and inline `mod foo { ... }`
        if norm.starts_with("mod ") || norm.starts_with("pub mod ") {
            let (hdr, term, next_i) = collect_header_until_brace_or_semi(&lines, i);
            if term == ';' {
                let mname = hdr
                    .trim_start_matches("pub ")
                    .trim_start_matches("mod ")
                    .trim_end_matches(';')
                    .trim();
                out.push_str(&format!(
                    "{}// [cap-migrate] submodule `{}` -> import from \"./{}.cp\"\n",
                    indent, mname, mname
                ));
                i = next_i;
            } else {
                let (mod_body, after_body) = collect_brace_body(&lines, next_i);
                out.push_str(&format!("{}// [cap-migrate] inline {}\n", indent, hdr));
                // Recursively transpile inline module body at top level
                let sub_src = mod_body.join("\n");
                let sub_res = transpile_rs_to_cp(&sub_src, rel_path, dep_map);
                // Strip the 2-line banner from recursive call
                for sub_line in sub_res.cp_code.lines().skip(3) {
                    out.push_str(sub_line);
                    out.push('\n');
                }
                todo_count += sub_res.todo_count;
                out.push_str(&format!("{}// [cap-migrate] end inline mod\n", indent));
                i = after_body;
            }
            continue;
        }

        // 7. Top-level macro invocations (`define_rejection! { ... }`, `all_the_tuples!(...);`, etc.)
        if is_rust_macro_invocation_start(trimmed) {
            todo_count += 1;
            let (blk, next_i) = collect_balanced_block(&lines, i);
            out.push_str(&format!(
                "{}// TODO(cap-migrate): Rust macro invocation `{}`\n",
                indent,
                blk.first().copied().unwrap_or(trimmed).trim()
            ));
            for bl in blk.iter().skip(1) {
                out.push_str(&format!("{}// [rs-macro] {}\n", indent, bl.trim()));
            }
            i = next_i;
            continue;
        }

        // 8. `struct` (unit, tuple, or named fields; single-line or multi-line with `where`)
        if norm.starts_with("pub struct ") || norm.starts_with("struct ") {
            let (hdr, term, next_i) = collect_header_until_brace_or_semi(&lines, i);
            let (head_no_where, where_clause) = split_where_clause(&hdr);
            let hnorm = normalize_rust_vis(head_no_where);
            let is_pub = hnorm.starts_with("pub ");
            let pub_kw = if is_pub { "pub " } else { "" };
            let after_kw = hnorm
                .trim_start_matches("pub ")
                .trim_start_matches("struct ")
                .trim_end_matches('{')
                .trim_end_matches(';')
                .trim();

            // Extract struct name + optional generic params
            let name_end = after_kw
                .find(|c: char| c == '<' || c == '(' || c.is_whitespace())
                .unwrap_or(after_kw.len());
            let sname = sanitize_cp_ident(&after_kw[..name_end]);
            let rest = after_kw[name_end..].trim();
            let (gen_str, after_gen) = if rest.starts_with('<') {
                if let Some(gt) = find_matching_delim(rest, 0, '<', '>') {
                    (clean_generic_params(&rest[..=gt]), rest[gt + 1..].trim())
                } else {
                    (String::new(), rest)
                }
            } else {
                (String::new(), rest)
            };

            let mut snippet = String::new();
            if let Some(wc) = where_clause {
                snippet.push_str(&format!(
                    "{}// [rs-where] {}\n",
                    indent,
                    wc.trim_end_matches('{').trim()
                ));
            }

            if term == ';' {
                // Tuple struct `struct Foo<T>(pub T, U);` or unit struct `struct Foo;`
                if after_gen.starts_with('(') {
                    if let Some(rp) = find_matching_delim(after_gen, 0, '(', ')') {
                        let inner = strip_inline_attrs(&after_gen[1..rp]);
                        let elems = split_top_level_commas(&inner);
                        snippet.push_str(&format!(
                            "{}{}struct {}{} {{\n",
                            indent, pub_kw, sname, gen_str
                        ));
                        for (idx, elem) in elems.iter().enumerate() {
                            let en = normalize_rust_vis(elem);
                            let ety =
                                pty_or_fallback(&map_rust_type(en.trim_start_matches("pub ")));
                            let def = default_value_for_cp_type(&ety);
                            if !def.is_empty() {
                                snippet.push_str(&format!(
                                    "{}    _{}: {} = {};\n",
                                    indent, idx, ety, def
                                ));
                            } else {
                                snippet.push_str(&format!("{}    _{}: {};\n", indent, idx, ety));
                            }
                        }
                        snippet.push_str(&format!("{}}}\n", indent));
                    } else {
                        snippet.push_str(&format!(
                            "{}{}struct {}{} {{}}\n",
                            indent, pub_kw, sname, gen_str
                        ));
                    }
                } else {
                    snippet.push_str(&format!(
                        "{}{}struct {}{} {{}}\n",
                        indent, pub_kw, sname, gen_str
                    ));
                }
                i = next_i;
            } else {
                let (sbody, after_body) = collect_brace_body(&lines, next_i);
                let fields_cp = convert_struct_fields(&sbody, indent);
                snippet.push_str(&format!(
                    "{}{}struct {}{} {{\n{}{}}}\n",
                    indent, pub_kw, sname, gen_str, fields_cp, indent
                ));
                i = after_body;
            }

            if snippet_parses_ok(&snippet) {
                out.push_str(&snippet);
            } else {
                todo_count += 1;
                out.push_str(&format!(
                    "{}// TODO(cap-migrate): review struct `{}`\n{}{}struct {} {{}}\n",
                    indent, sname, indent, pub_kw, sname
                ));
            }
            continue;
        }

        // 9. `enum` (unit, tuple, struct variants; multi-line with `where`)
        if norm.starts_with("pub enum ") || norm.starts_with("enum ") {
            let (hdr, term, next_i) = collect_header_until_brace_or_semi(&lines, i);
            let (head_no_where, where_clause) = split_where_clause(&hdr);
            let hnorm = normalize_rust_vis(head_no_where);
            let is_pub = hnorm.starts_with("pub ");
            let pub_kw = if is_pub { "pub " } else { "" };
            let after_kw = hnorm
                .trim_start_matches("pub ")
                .trim_start_matches("enum ")
                .trim_end_matches('{')
                .trim();
            let ename = sanitize_cp_ident(
                after_kw
                    .split(|c: char| c == '<' || c.is_whitespace())
                    .next()
                    .unwrap_or("Enum"),
            );

            let mut snippet = String::new();
            if let Some(wc) = where_clause {
                snippet.push_str(&format!(
                    "{}// [rs-where] {}\n",
                    indent,
                    wc.trim_end_matches('{').trim()
                ));
            }
            if term == '{' {
                let (ebody, after_body) = collect_brace_body(&lines, next_i);
                let vars_cp = convert_enum_variants(&ebody, indent);
                snippet.push_str(&format!(
                    "{}{}enum {} {{\n{}{}}}\n",
                    indent, pub_kw, ename, vars_cp, indent
                ));
                i = after_body;
            } else {
                snippet.push_str(&format!("{}{}enum {} {{}}\n", indent, pub_kw, ename));
                i = next_i;
            }

            if snippet_parses_ok(&snippet) {
                out.push_str(&snippet);
            } else {
                todo_count += 1;
                out.push_str(&format!(
                    "{}// TODO(cap-migrate): review enum `{}`\n{}{}enum {} {{}}\n",
                    indent, ename, indent, pub_kw, ename
                ));
            }
            continue;
        }

        // 10. `trait` (CP `parse_trait` accepts `pub trait Ident { fn ... }`)
        if norm.starts_with("pub trait ")
            || norm.starts_with("trait ")
            || norm.starts_with("pub unsafe trait ")
            || norm.starts_with("unsafe trait ")
        {
            let (hdr, term, next_i) = collect_header_until_brace_or_semi(&lines, i);
            let (head_no_where, where_clause) = split_where_clause(&hdr);
            let hnorm = normalize_rust_vis(head_no_where);
            let is_pub = hnorm.starts_with("pub ");
            let pub_kw = if is_pub { "pub " } else { "" };
            let after_kw = hnorm
                .trim_start_matches("pub ")
                .trim_start_matches("unsafe ")
                .trim_start_matches("trait ")
                .trim();
            let tname = sanitize_cp_ident(
                after_kw
                    .split(|c: char| c == '<' || c == ':' || c == '{' || c.is_whitespace())
                    .next()
                    .unwrap_or("Trait"),
            );

            let mut snippet = String::new();
            if after_kw.contains('<') || after_kw.contains(':') || where_clause.is_some() {
                snippet.push_str(&format!(
                    "{}// [rs-trait] {}\n",
                    indent,
                    hdr.trim_end_matches('{').trim()
                ));
            }
            if term == '{' {
                let (tbody, after_body) = collect_brace_body(&lines, next_i);
                let inner_indent = format!("{}    ", indent);
                let members =
                    convert_member_block(&tbody, &inner_indent, true, false, None, &mut todo_count);
                snippet.push_str(&format!(
                    "{}{}trait {} {{\n{}{}}}\n",
                    indent, pub_kw, tname, members, indent
                ));
                i = after_body;
            } else {
                snippet.push_str(&format!("{}{}trait {} {{}}\n", indent, pub_kw, tname));
                i = next_i;
            }

            if snippet_parses_ok(&snippet) {
                out.push_str(&snippet);
            } else {
                todo_count += 1;
                out.push_str(&format!(
                    "{}// TODO(cap-migrate): review trait `{}`\n{}{}trait {} {{}}\n",
                    indent, tname, indent, pub_kw, tname
                ));
            }
            continue;
        }

        // 11. `impl` (Trait impl `impl Trait for Target` vs inherent impl `impl Target`)
        if norm.starts_with("impl ")
            || norm.starts_with("impl<")
            || norm.starts_with("unsafe impl ")
            || norm.starts_with("unsafe impl<")
        {
            let (hdr, term, next_i) = collect_header_until_brace_or_semi(&lines, i);
            let (head_no_where, _where_clause) = split_where_clause(&hdr);
            let hclean = head_no_where.trim_end_matches('{').trim();
            let after_impl = hclean
                .trim_start_matches("unsafe ")
                .trim_start_matches("impl")
                .trim();
            // Skip `impl<...>` generic params
            let after_gen = if after_impl.starts_with('<') {
                if let Some(gt) = find_matching_delim(after_impl, 0, '<', '>') {
                    after_impl[gt + 1..].trim()
                } else {
                    after_impl
                }
            } else {
                after_impl
            };

            let (ibody, after_body) = if term == '{' {
                collect_brace_body(&lines, next_i)
            } else {
                (Vec::new(), next_i)
            };
            i = after_body;

            // Detect `Trait for Target`
            let mut for_split = None;
            {
                let bytes = after_gen.as_bytes();
                let mut a_d = 0i32;
                let mut p_d = 0i32;
                for idx in 0..bytes.len() {
                    match bytes[idx] {
                        b'<' => a_d += 1,
                        b'>' => a_d = (a_d - 1).max(0),
                        b'(' => p_d += 1,
                        b')' => p_d = (p_d - 1).max(0),
                        _ => {}
                    }
                    if a_d == 0 && p_d == 0 && after_gen[idx..].starts_with(" for ") {
                        for_split = Some((after_gen[..idx].trim(), after_gen[idx + 5..].trim()));
                        break;
                    }
                }
            }

            if let Some((trait_raw, target_raw)) = for_split {
                if let (Some(trait_id), Some(target_id)) = (
                    extract_simple_ident(trait_raw),
                    extract_simple_ident(target_raw),
                ) {
                    let inner_indent = format!("{}    ", indent);
                    let members = convert_member_block(
                        &ibody,
                        &inner_indent,
                        false,
                        true,
                        None,
                        &mut todo_count,
                    );
                    let snippet = format!(
                        "{}// [rs-impl] {}\n{}impl {} for {} {{\n{}{}}}\n",
                        indent,
                        hdr.trim_end_matches('{').trim(),
                        indent,
                        trait_id,
                        target_id,
                        members,
                        indent
                    );
                    if snippet_parses_ok(&snippet) {
                        out.push_str(&snippet);
                    } else {
                        todo_count += 1;
                        out.push_str(&format!(
                            "{}// TODO(cap-migrate): review impl {} for {}\n{}impl {} for {} {{}}\n",
                            indent, trait_id, target_id, indent, trait_id, target_id
                        ));
                    }
                } else {
                    // Complex target type (tuple, ref, slice, fn ptr) -> comment out safely
                    todo_count += 1;
                    out.push_str(&format!(
                        "{}// TODO(cap-migrate): complex trait impl `{}`\n",
                        indent,
                        hdr.trim_end_matches('{').trim()
                    ));
                    for bl in &ibody {
                        let bt = bl.trim();
                        if !bt.is_empty() {
                            out.push_str(&format!("{}// [rs-impl] {}\n", indent, bt));
                        }
                    }
                }
            } else {
                // Inherent impl `impl Target { ... }`: CP does not allow `impl Target` without `for`.
                // Hoist methods cleanly as `Target_method(...)` free functions!
                let target_id =
                    extract_simple_ident(after_gen).unwrap_or_else(|| "Impl".to_string());
                out.push_str(&format!(
                    "{}// [cap-migrate] inherent `{}` (methods hoisted for CP struct)\n",
                    indent,
                    hdr.trim_end_matches('{').trim()
                ));
                let members = convert_member_block(
                    &ibody,
                    indent,
                    false,
                    false,
                    Some(&target_id),
                    &mut todo_count,
                );
                out.push_str(&members);
            }
            continue;
        }

        // 12. Top-level function declaration
        if is_rust_fn_start(&norm) {
            let (hdr, term, next_i) = collect_header_until_brace_or_semi(&lines, i);
            if term == '{' {
                let (fn_body, after_body) = collect_brace_body(&lines, next_i);
                let fn_cp = convert_rust_function(
                    &hdr,
                    '{',
                    Some(&fn_body),
                    indent,
                    false,
                    false,
                    None,
                    &mut todo_count,
                );
                out.push_str(&fn_cp);
                i = after_body;
            } else {
                let fn_cp = convert_rust_function(
                    &hdr,
                    ';',
                    None,
                    indent,
                    false,
                    false,
                    None,
                    &mut todo_count,
                );
                out.push_str(&fn_cp);
                i = next_i;
            }
            continue;
        }

        // 13. `type Alias = ...;` (supports multi-line & `where`)
        if norm.starts_with("pub type ") || norm.starts_with("type ") {
            let (hdr, _, next_i) = collect_header_until_brace_or_semi(&lines, i);
            let hnorm = normalize_rust_vis(&hdr);
            if let Some(eq_idx) = hnorm.find('=') {
                let lhs_raw = strip_rust_lifetimes(&hnorm[..eq_idx]);
                let is_pub = lhs_raw.trim().starts_with("pub ");
                let pub_kw = if is_pub { "pub " } else { "" };
                let after_type = lhs_raw
                    .trim()
                    .trim_start_matches("pub ")
                    .trim_start_matches("type ")
                    .trim();
                let (tname_raw, gen_str) = if let Some(lt) = after_type.find('<') {
                    (
                        after_type[..lt].trim(),
                        clean_generic_params(&after_type[lt..]),
                    )
                } else {
                    (after_type, String::new())
                };
                let tname = sanitize_cp_ident(tname_raw);
                let rhs = pty_or_fallback(&map_rust_type(
                    hnorm[eq_idx + 1..].trim().trim_end_matches(';'),
                ));
                let candidate =
                    format!("{}{}type {}{} = {};\n", indent, pub_kw, tname, gen_str, rhs);
                if snippet_parses_ok(&candidate) {
                    out.push_str(&candidate);
                } else {
                    out.push_str(&format!("{}// [rs-type] {}\n", indent, hdr));
                }
            } else {
                out.push_str(&format!("{}// [rs-type] {}\n", indent, hdr));
            }
            i = next_i;
            continue;
        }

        // 14. `const` / `static` top-level items (supports multi-line)
        if norm.starts_with("pub const ")
            || norm.starts_with("const ")
            || norm.starts_with("pub static ")
            || norm.starts_with("static ")
        {
            let (hdr, _, next_i) = collect_header_until_brace_or_semi(&lines, i);
            let hnorm = normalize_rust_vis(&hdr);
            let is_pub = hnorm.starts_with("pub ");
            let pub_kw = if is_pub { "pub " } else { "" };
            let after_kw = hnorm
                .trim_start_matches("pub ")
                .trim_start_matches("const ")
                .trim_start_matches("static ")
                .trim_start_matches("mut ")
                .trim();
            if let Some(colon) = find_top_level_colon(after_kw) {
                let cname = sanitize_cp_ident(&after_kw[..colon]);
                let rest = after_kw[colon + 1..].trim();
                if let Some(eq) = rest.find('=') {
                    let cty = pty_or_fallback(&map_rust_type(&rest[..eq]));
                    let cval = convert_rust_expr_line(rest[eq + 1..].trim().trim_end_matches(';'));
                    let candidate =
                        format!("{}{}const {}: {} = {};\n", indent, pub_kw, cname, cty, cval);
                    if snippet_parses_ok(&candidate) {
                        out.push_str(&candidate);
                    } else {
                        out.push_str(&format!("{}// [rs-const] {}\n", indent, hdr));
                    }
                    i = next_i;
                    continue;
                }
            }
            out.push_str(&format!("{}// [rs-const] {}\n", indent, hdr));
            i = next_i;
            continue;
        }

        // 15. Any remaining unrecognized top-level Rust construct -> comment safely instead of breaking parse
        let (blk, next_i) = collect_balanced_block(&lines, i);
        for bl in blk {
            let bt = bl.trim();
            if !bt.is_empty() {
                out.push_str(&format!("{}// [rs-item] {}\n", indent, bt));
            }
        }
        i = next_i;
    }

    if saw_lifetime {
        opt_notes.push("擦除 Rust `'a` 生命周期标注为 CP 词法作用域借用".to_string());
    }
    if saw_async_box {
        opt_notes.push("标记 `Pin<Box<dyn Future>>` 堆装箱异步为 CP 零堆状态机改造点".to_string());
    }
    if saw_macro_rules {
        opt_notes.push("将 `macro_rules!` 标记为 CP 编译期泛型替代点".to_string());
    }
    opt_notes.push("统一 Rust `&self`/`&mut self` 为 CP 零噪声 `_`/`*_` 接收者".to_string());

    RsTranspileResult {
        cp_code: out,
        todo_count,
        opt_notes,
    }
}
