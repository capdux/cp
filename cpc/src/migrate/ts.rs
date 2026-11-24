// TypeScript -> CP High-Success-Rate Structural Transpiler (`cap from -ts`)
// Designed and verified against industrial TS codebases (e.g., `_ref/elysia/src/*.ts`).

use std::collections::HashMap;

use super::deps::{rewrite_import_source, DepTier, SourceLang};

pub struct TranspileResult {
    pub cp_code: String,
    pub todo_count: usize,
    pub opt_notes: Vec<String>,
}

// ============================================================================
// Lexical / String-Aware Utilities
// ============================================================================

/// Convert TS single-quoted string literals `'...'` to CP double-quoted `"..."`
/// outside of `//` comments and double-quoted/backtick strings.
fn convert_single_quoted_strings(line: &str) -> String {
    let bytes = line.as_bytes();
    let mut out = String::with_capacity(line.len() + 8);
    let mut i = 0;
    let mut in_double = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&line[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        if b == b'\'' {
            out.push('"');
            i += 1;
            while i < bytes.len() {
                let c = bytes[i];
                if c == b'\\' && i + 1 < bytes.len() {
                    let next = bytes[i + 1];
                    if next == b'\'' {
                        out.push('\'');
                    } else {
                        out.push('\\');
                        out.push(next as char);
                    }
                    i += 2;
                    continue;
                }
                if c == b'\'' {
                    out.push('"');
                    i += 1;
                    break;
                }
                if c == b'"' {
                    out.push('\\');
                    out.push('"');
                } else {
                    out.push(c as char);
                }
                i += 1;
            }
            continue;
        }
        // Preserve multi-byte UTF-8 characters safely
        let ch = line[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Split a line into `(code_part, inline_comment_part)` without breaking strings.
fn split_inline_comment(line: &str) -> (&str, &str) {
    let bytes = line.as_bytes();
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if in_double {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'"' {
                in_double = false;
            }
        } else if in_single {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'\'' {
                in_single = false;
            }
        } else if in_backtick {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'`' {
                in_backtick = false;
            }
        } else {
            if b == b'"' {
                in_double = true;
            } else if b == b'\'' {
                in_single = true;
            } else if b == b'`' {
                in_backtick = true;
            } else if b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
                return (&line[..i], &line[i..]);
            }
        }
        i += 1;
    }
    (line, "")
}

/// Count `{` and `}` outside of strings and `//` comments.
fn count_braces(s: &str) -> (i32, i32) {
    let (code, _) = split_inline_comment(s);
    let bytes = code.as_bytes();
    let mut opens = 0i32;
    let mut closes = 0i32;
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if in_double {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'"' {
                in_double = false;
            }
        } else if in_single {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'\'' {
                in_single = false;
            }
        } else if in_backtick {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'`' {
                in_backtick = false;
            }
        } else {
            match b {
                b'"' => in_double = true,
                b'\'' => in_single = true,
                b'`' => in_backtick = true,
                b'{' => opens += 1,
                b'}' => closes += 1,
                _ => {}
            }
        }
        i += 1;
    }
    (opens, closes)
}

fn is_ident_byte(b: u8) -> bool {
    b.is_ascii_alphanumeric() || b == b'_' || b == b'$'
}

/// Replace whole-word identifier `from` with `to` outside string literals and comments.
fn replace_ident_word(s: &str, from: &str, to: &str) -> String {
    if !s.contains(from) {
        return s.to_string();
    }
    let bytes = s.as_bytes();
    let fbytes = from.as_bytes();
    let flen = fbytes.len();
    let mut out = String::with_capacity(s.len() + 16);
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_single && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'\'' {
            in_single = true;
            out.push('\'');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        if i + flen <= bytes.len() && &bytes[i..i + flen] == fbytes {
            let prev_ok = i == 0 || !is_ident_byte(bytes[i - 1]);
            let next_ok = i + flen == bytes.len() || !is_ident_byte(bytes[i + flen]);
            if prev_ok && next_ok {
                out.push_str(to);
                i += flen;
                continue;
            }
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Replace an exact token substring (like `===` or `!==`) outside strings and comments.
fn replace_outside_strings(s: &str, from: &str, to: &str) -> String {
    if !s.contains(from) {
        return s.to_string();
    }
    let bytes = s.as_bytes();
    let fbytes = from.as_bytes();
    let flen = fbytes.len();
    let mut out = String::with_capacity(s.len() + 8);
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_single && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'\'' {
            in_single = true;
            out.push('\'');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        if i + flen <= bytes.len() && &bytes[i..i + flen] == fbytes {
            out.push_str(to);
            i += flen;
            continue;
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

fn split_top_level(s: &str, delim: char) -> Vec<String> {
    let mut parts = Vec::new();
    let mut cur = String::new();
    let mut depth_angle = 0i32;
    let mut depth_paren = 0i32;
    let mut depth_brack = 0i32;
    let mut depth_brace = 0i32;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;
    let mut prev_bs = false;

    for ch in s.chars() {
        if in_double {
            cur.push(ch);
            if ch == '"' && !prev_bs {
                in_double = false;
            }
            prev_bs = ch == '\\' && !prev_bs;
            continue;
        }
        if in_single {
            cur.push(ch);
            if ch == '\'' && !prev_bs {
                in_single = false;
            }
            prev_bs = ch == '\\' && !prev_bs;
            continue;
        }
        if in_backtick {
            cur.push(ch);
            if ch == '`' && !prev_bs {
                in_backtick = false;
            }
            prev_bs = ch == '\\' && !prev_bs;
            continue;
        }
        prev_bs = false;
        match ch {
            '"' => {
                in_double = true;
                cur.push(ch);
            }
            '\'' => {
                in_single = true;
                cur.push(ch);
            }
            '`' => {
                in_backtick = true;
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
            c if c == delim
                && depth_angle == 0
                && depth_paren == 0
                && depth_brack == 0
                && depth_brace == 0 =>
            {
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

/// Find the index of matching closing delimiter starting at `open_idx`.
fn find_matching_delim(s: &str, open_idx: usize, open: u8, close: u8) -> Option<usize> {
    let bytes = s.as_bytes();
    if open_idx >= bytes.len() || bytes[open_idx] != open {
        return None;
    }
    let mut depth = 0i32;
    let mut i = open_idx;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if in_double {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'"' {
                in_double = false;
            }
        } else if in_single {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'\'' {
                in_single = false;
            }
        } else if in_backtick {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'`' {
                in_backtick = false;
            }
        } else {
            if b == b'"' {
                in_double = true;
            } else if b == b'\'' {
                in_single = true;
            } else if b == b'`' {
                in_backtick = true;
            } else if b == open {
                depth += 1;
            } else if b == close {
                // Do not treat `=>` fat arrow as closing `>` for generic angle brackets
                if open == b'<' && close == b'>' && i > 0 && bytes[i - 1] == b'=' {
                    i += 1;
                    continue;
                }
                depth -= 1;
                if depth == 0 {
                    return Some(i);
                }
            }
        }
        i += 1;
    }
    None
}

// ============================================================================
// TypeScript Type Mapping Engine
// ============================================================================

fn is_complex_ts_type(raw: &str) -> bool {
    let t = raw.trim();
    t.contains(" extends ")
        || t.contains("keyof ")
        || t.contains("typeof ")
        || t.contains("infer ")
        || t.contains(" in ")
        || t.contains("=>")
        || t.contains('`')
        || t.contains('&')
        || t.contains('{')
        || t.contains('}')
        || t.contains("-?")
        || t.contains("is ")
}

/// Simplify TS generic parameter list `<const in out T extends Foo = Bar, U>` -> `<T, U>`.
fn simplify_ts_generics(raw: &str) -> String {
    let t = raw.trim();
    if !t.starts_with('<') || !t.ends_with('>') {
        return String::new();
    }
    let inner = &t[1..t.len() - 1];
    let mut names = Vec::new();
    for part in split_top_level(inner, ',') {
        let mut p = part.trim();
        loop {
            if let Some(r) = p.strip_prefix("const ") {
                p = r.trim();
            } else if let Some(r) = p.strip_prefix("in ") {
                p = r.trim();
            } else if let Some(r) = p.strip_prefix("out ") {
                p = r.trim();
            } else {
                break;
            }
        }
        let name = p
            .split(|c: char| !c.is_ascii_alphanumeric() && c != '_')
            .next()
            .unwrap_or("")
            .trim();
        if !name.is_empty() && name.chars().next().unwrap().is_ascii_alphabetic() {
            names.push(name.to_string());
        }
    }
    if names.is_empty() {
        String::new()
    } else {
        format!("<{}>", names.join(", "))
    }
}

fn map_ts_type(raw: &str, todo_count: &mut usize) -> String {
    let mut t = raw.trim().trim_end_matches(';').trim().to_string();
    if t.is_empty() {
        return "string".to_string();
    }
    // Strip leading `readonly `
    while let Some(rest) = t.strip_prefix("readonly ") {
        t = rest.trim().to_string();
    }
    // Strip wrapping parens `(T)`
    while t.starts_with('(') && t.ends_with(')') {
        if find_matching_delim(&t, 0, b'(', b')') == Some(t.len() - 1) {
            t = t[1..t.len() - 1].trim().to_string();
        } else {
            break;
        }
    }

    // Handle union with null / undefined first: `T | null | undefined` -> `T?`
    let union_parts = split_top_level(&t, '|');
    if union_parts.len() > 1 {
        let mut nullable = false;
        let mut non_null: Vec<String> = Vec::new();
        for part in union_parts {
            let p = part.trim();
            if p == "null" || p == "undefined" || p == "void" {
                nullable = true;
            } else if !p.is_empty() {
                non_null.push(p.to_string());
            }
        }
        if non_null.is_empty() {
            return "string?".to_string();
        }
        if non_null.len() == 1 {
            let base = map_ts_type(&non_null[0], todo_count);
            if nullable && !base.ends_with('?') && !base.contains("/* TODO") {
                return format!("{}?", base);
            }
            return base;
        }
        // String literal union `'a' | 'b'` -> `string` (or `string?`)
        if non_null
            .iter()
            .all(|p| (p.starts_with('"') && p.ends_with('"')) || (p.starts_with('\'') && p.ends_with('\'')))
        {
            return if nullable {
                "string?".to_string()
            } else {
                "string".to_string()
            };
        }
        // Boolean literal union `true | false`
        if non_null.iter().all(|p| p == "true" || p == "false") {
            return if nullable {
                "bool?".to_string()
            } else {
                "bool".to_string()
            };
        }
        // Number literal union
        if non_null.iter().all(|p| p.parse::<f64>().is_ok()) {
            return if nullable {
                "f64?".to_string()
            } else {
                "f64".to_string()
            };
        }
        // Mixed union -> degrade safely to string (or string?)
        *todo_count += 1;
        return if nullable {
            "string? /* TODO(cap-migrate): refine union type */".to_string()
        } else {
            "string /* TODO(cap-migrate): refine union type */".to_string()
        };
    }

    // Complex TS type-level constructs (conditional, mapped, function, intersection, inline object)
    if is_complex_ts_type(&t) {
        *todo_count += 1;
        return "string /* TODO(cap-migrate): refine complex TS type */".to_string();
    }

    // Direct primitive mappings
    match t.as_str() {
        "number" => return "f64".to_string(),
        "boolean" => return "bool".to_string(),
        "string" => return "string".to_string(),
        "void" | "never" => return "void".to_string(),
        "bigint" => return "i64".to_string(),
        "Uint8Array" | "Buffer" => return "[]u8".to_string(),
        "null" | "undefined" => return "string?".to_string(),
        "any" | "unknown" | "object" | "Object" | "Function" => {
            *todo_count += 1;
            return "string /* TODO(cap-migrate): refine dynamic type */".to_string();
        }
        "Date" | "RegExp" | "Error" | "Response" | "Request" | "Headers" | "URL" | "FormData" => {
            return t;
        }
        _ => {}
    }

    // String/number/boolean literal as standalone type
    if (t.starts_with('"') && t.ends_with('"')) || (t.starts_with('\'') && t.ends_with('\'')) {
        return "string".to_string();
    }
    if t == "true" || t == "false" {
        return "bool".to_string();
    }
    if t.parse::<f64>().is_ok() {
        return "f64".to_string();
    }

    // Trailing array syntax `T[]`
    if let Some(elem) = t.strip_suffix("[]") {
        let mapped = map_ts_type(elem, todo_count);
        let clean = mapped.split(" /*").next().unwrap_or(&mapped).trim();
        return format!("[]{}", clean);
    }

    // Tuple syntax `[A, B]` -> `[]string` or `[A, B]`
    if t.starts_with('[') && t.ends_with(']') {
        let inner = &t[1..t.len() - 1];
        let elems: Vec<String> = split_top_level(inner, ',')
            .into_iter()
            .map(|e| {
                let m = map_ts_type(&e, todo_count);
                m.split(" /*").next().unwrap_or(&m).trim().to_string()
            })
            .collect();
        if elems.is_empty() {
            return "[]string".to_string();
        }
        return format!("[{}]", elems.join(", "));
    }

    // Generic wrappers
    if let Some(lt) = t.find('<') {
        if t.ends_with('>') && find_matching_delim(&t, lt, b'<', b'>') == Some(t.len() - 1) {
            let base = t[..lt].trim();
            let inner = &t[lt + 1..t.len() - 1];
            let args = split_top_level(inner, ',');
            match base {
                "Promise" | "PromiseLike" | "Awaited" => {
                    let arg0 = args.first().map(|s| s.as_str()).unwrap_or("void");
                    return map_ts_type(arg0, todo_count);
                }
                "Array" | "ReadonlyArray" => {
                    let arg0 = args.first().map(|s| s.as_str()).unwrap_or("string");
                    let mapped = map_ts_type(arg0, todo_count);
                    let clean = mapped.split(" /*").next().unwrap_or(&mapped).trim();
                    return format!("[]{}", clean);
                }
                "Record" => {
                    let k = args.first().map(|s| s.as_str()).unwrap_or("string");
                    let v = args.get(1).map(|s| s.as_str()).unwrap_or("string");
                    let mk = map_ts_type(k, todo_count);
                    let mv = map_ts_type(v, todo_count);
                    let ck = mk.split(" /*").next().unwrap_or(&mk).trim();
                    let cv = mv.split(" /*").next().unwrap_or(&mv).trim();
                    return format!("Map<{}, {}>", ck, cv);
                }
                "Map" | "WeakMap" => {
                    let k = args.first().map(|s| s.as_str()).unwrap_or("string");
                    let v = args.get(1).map(|s| s.as_str()).unwrap_or("string");
                    let mk = map_ts_type(k, todo_count);
                    let mv = map_ts_type(v, todo_count);
                    let ck = mk.split(" /*").next().unwrap_or(&mk).trim();
                    let cv = mv.split(" /*").next().unwrap_or(&mv).trim();
                    return format!("Map<{}, {}>", ck, cv);
                }
                "Set" | "WeakSet" => {
                    let v = args.first().map(|s| s.as_str()).unwrap_or("string");
                    let mv = map_ts_type(v, todo_count);
                    let cv = mv.split(" /*").next().unwrap_or(&mv).trim();
                    return format!("Set<{}>", cv);
                }
                "Partial" | "Required" | "Readonly" | "Prettify" | "NonNullable" => {
                    let arg0 = args.first().map(|s| s.as_str()).unwrap_or("string");
                    return map_ts_type(arg0, todo_count);
                }
                "Omit" | "Pick" | "Exclude" | "Extract" | "ReturnType" | "Parameters" => {
                    *todo_count += 1;
                    return "string /* TODO(cap-migrate): refine TS utility type */".to_string();
                }
                _ => {
                    let mapped_args: Vec<String> = args
                        .iter()
                        .map(|a| {
                            let m = map_ts_type(a, todo_count);
                            m.split(" /*").next().unwrap_or(&m).trim().to_string()
                        })
                        .collect();
                    return format!("{}<{}>", base, mapped_args.join(", "));
                }
            }
        }
    }

    // Indexed access type `Route['body']` -> degrade safely
    if t.contains('[') && t.contains(']') {
        *todo_count += 1;
        return "string /* TODO(cap-migrate): refine indexed access type */".to_string();
    }

    t
}

fn default_value_for_cp_type(ty: &str) -> &'static str {
    let clean = ty.split(" /*").next().unwrap_or(ty).trim();
    if clean.ends_with('?') {
        return "null";
    }
    if clean.starts_with("[]") {
        return "[]";
    }
    match clean {
        "bool" => "false",
        "string" => "\"\"",
        "i8" | "i16" | "i32" | "i64" | "isize" | "u8" | "u16" | "u32" | "u64" | "usize" => "0",
        "f32" | "f64" => "0.0",
        _ => "",
    }
}

// ============================================================================
// Import / Export Rewriter
// ============================================================================

fn extract_quoted_specifier(s: &str) -> Option<(String, usize, usize)> {
    let bytes = s.as_bytes();
    for (idx, &b) in bytes.iter().enumerate() {
        if b == b'"' || b == b'\'' {
            let quote = b;
            let mut j = idx + 1;
            while j < bytes.len() && bytes[j] != quote {
                if bytes[j] == b'\\' {
                    j += 2;
                    continue;
                }
                j += 1;
            }
            if j < bytes.len() {
                return Some((s[idx + 1..j].to_string(), idx, j + 1));
            }
        }
    }
    None
}

/// Convert a complete (possibly multi-line joined) TS import/export-from statement.
fn convert_import_export(stmt: &str, dep_map: &HashMap<String, DepTier>) -> String {
    let trimmed = stmt.trim().trim_end_matches(';').trim();

    // `import type ...` or `export type { ... } from ...`
    let is_import = trimmed.starts_with("import ");
    let is_export = trimmed.starts_with("export ");
    let is_type_only =
        trimmed.starts_with("import type ") || trimmed.starts_with("export type ");

    // Locate ` from `
    if let Some(from_pos) = trimmed.rfind(" from ") {
        let head = trimmed[..from_pos].trim();
        let tail = trimmed[from_pos + 6..].trim();
        if let Some((raw_pkg, _, _)) = extract_quoted_specifier(tail) {
            let rewritten = rewrite_import_source(SourceLang::Ts, &raw_pkg, dep_map);
            if is_type_only {
                let clean_head = head
                    .replacen("import type ", "import ", 1)
                    .replacen("export type ", "export ", 1);
                return format!("// [cap-migrate] {} from \"{}\";", clean_head, rewritten);
            }

            // Strip inline `type ` modifiers inside `{ a, type b }`
            let mut clean_head = head.to_string();
            if let Some(lb) = clean_head.find('{') {
                if let Some(rb) = clean_head.rfind('}') {
                    let inner = &clean_head[lb + 1..rb];
                    let items: Vec<String> = inner
                        .split(',')
                        .map(|it| {
                            let s = it.trim();
                            s.strip_prefix("type ").unwrap_or(s).trim().to_string()
                        })
                        .filter(|s| !s.is_empty())
                        .collect();
                    clean_head = format!(
                        "{}{{ {} }}{}",
                        &clean_head[..lb],
                        items.join(", "),
                        &clean_head[rb + 1..]
                    );
                }
            }
            return format!("{} from \"{}\";", clean_head, rewritten);
        }
    }

    // Side-effect import `import "pkg"`
    if is_import {
        if let Some((raw_pkg, _, _)) = extract_quoted_specifier(trimmed) {
            let rewritten = rewrite_import_source(SourceLang::Ts, &raw_pkg, dep_map);
            return format!("// [cap-migrate] side-effect import \"{}\";", rewritten);
        }
    }

    if is_export {
        return format!("{};", trimmed);
    }

    format!("// [cap-migrate] {}", trimmed)
}

// ============================================================================
// Parameter & Signature Converters
// ============================================================================

/// Convert TS parameter list `(a: string, b?: number = 0)` -> `(a: string, b: f64? = 0)`.
/// Also returns any TS constructor parameter properties (`private name: string`) as `(field_name, cp_type)`.
fn convert_ts_params(
    params_raw: &str,
    collect_ctor_fields: bool,
    todo_count: &mut usize,
) -> (Vec<String>, Vec<(String, String)>) {
    let mut out_params = Vec::new();
    let mut ctor_fields = Vec::new();

    for part in split_top_level(params_raw, ',') {
        let mut p = part.trim().to_string();
        if p.is_empty() {
            continue;
        }
        let mut is_ctor_prop = false;
        for kw in &["public ", "private ", "protected ", "readonly "] {
            while let Some(rest) = p.strip_prefix(kw) {
                is_ctor_prop = true;
                p = rest.trim().to_string();
            }
        }
        // Rest parameter `...args: T[]`
        if let Some(rest) = p.strip_prefix("...") {
            p = rest.trim().to_string();
        }
        // Destructured parameter `{ a, b }: Opts` -> `_opts: Opts`
        if p.starts_with('{') || p.starts_with('[') {
            *todo_count += 1;
            out_params.push("_param: string".to_string());
            continue;
        }

        // Split default value `=` at top level
        let mut default_val: Option<String> = None;
        let eq_parts = split_top_level(&p, '=');
        let head = if eq_parts.len() >= 2 {
            default_val = Some(eq_parts[1..].join(" = "));
            eq_parts[0].trim().to_string()
        } else {
            p
        };

        // Split `name?: Type`
        let (raw_name, raw_ty) = if let Some(colon) = head.find(':') {
            (head[..colon].trim(), Some(head[colon + 1..].trim()))
        } else {
            (head.as_str(), None)
        };

        let is_optional = raw_name.ends_with('?');
        let mut name = raw_name.trim_end_matches('?').trim().to_string();
        if name.is_empty() || name == "this" {
            continue;
        }
        // Avoid CP reserved identifier `_` as parameter name
        if name == "_" {
            name = "_unused".to_string();
        }

        let mut cp_ty = if let Some(ty_str) = raw_ty {
            let mapped = map_ts_type(ty_str, todo_count);
            mapped.split(" /*").next().unwrap_or(&mapped).trim().to_string()
        } else {
            "auto".to_string()
        };

        if is_optional && cp_ty != "auto" && !cp_ty.ends_with('?') {
            cp_ty.push('?');
        }

        if collect_ctor_fields && is_ctor_prop {
            let field_ty = if cp_ty == "auto" {
                "string".to_string()
            } else {
                cp_ty.clone()
            };
            ctor_fields.push((name.clone(), field_ty));
        }

        if let Some(def) = default_val {
            let clean_def = convert_ts_expr(&def, todo_count);
            out_params.push(format!("{}: {} = {}", name, cp_ty, clean_def));
        } else if is_optional && cp_ty.ends_with('?') {
            out_params.push(format!("{}: {} = null", name, cp_ty));
        } else {
            out_params.push(format!("{}: {}", name, cp_ty));
        }
    }

    (out_params, ctor_fields)
}

/// Format CP return type annotation (`""` for void/omitted, `": R"` otherwise).
fn format_return_type(raw_ret: Option<&str>, todo_count: &mut usize) -> String {
    match raw_ret {
        None => String::new(),
        Some(r) => {
            let r = r.trim();
            if r.is_empty() {
                return String::new();
            }
            let mapped = map_ts_type(r, todo_count);
            let clean = mapped.split(" /*").next().unwrap_or(&mapped).trim();
            if clean == "void" || clean.is_empty() {
                String::new()
            } else {
                format!(": {}", clean)
            }
        }
    }
}

// ============================================================================
// Expression & Statement Rewriter
// ============================================================================

/// Strip TS `as <Type>`, `satisfies <Type>`, and non-null assertions `ident!`.
fn strip_ts_expr_noise(s: &str) -> String {
    let mut out = strip_ts_as_and_satisfies(s);

    // Strip non-null assertion `x!` (ident/closing-bracket followed by `!` not followed by `=`)
    let bytes = out.as_bytes();
    let mut res = String::with_capacity(out.len());
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_single && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            res.push_str(&out[i..]);
            break;
        }
        if in_double {
            res.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                res.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            res.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                res.push(bytes[i] as char);
            } else if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            res.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                res.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            res.push('"');
            i += 1;
            continue;
        }
        if b == b'\'' {
            in_single = true;
            res.push('\'');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            res.push('`');
            i += 1;
            continue;
        }
        if b == b'!' {
            let next_is_eq = i + 1 < bytes.len() && bytes[i + 1] == b'=';
            let prev_is_postfix = i > 0
                && (is_ident_byte(bytes[i - 1]) || bytes[i - 1] == b')' || bytes[i - 1] == b']');
            if prev_is_postfix && !next_is_eq {
                // Drop TS non-null assertion `!`
                i += 1;
                continue;
            }
        }
        let ch = out[i..].chars().next().unwrap();
        res.push(ch);
        i += ch.len_utf8();
    }
    out = res;
    out
}

/// Strip TS ` as <Type>` and ` satisfies <Type>` assertions outside strings and comments.
fn strip_ts_as_and_satisfies(s: &str) -> String {
    if !s.contains(" as ") && !s.contains(" satisfies ") {
        return s.to_string();
    }
    let bytes = s.as_bytes();
    let mut out = String::with_capacity(s.len());
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_single && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'\'' {
            in_single = true;
            out.push('\'');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }

        let kw_len = if i + 4 <= bytes.len() && &bytes[i..i + 4] == b" as " {
            Some(4)
        } else if i + 11 <= bytes.len() && &bytes[i..i + 11] == b" satisfies " {
            Some(11)
        } else {
            None
        };

        if let Some(klen) = kw_len {
            let mut j = i + klen;
            while j < bytes.len() && bytes[j] == b' ' {
                j += 1;
            }
            // Consume type expression: identifiers, `.`, `<...>`, `[...]`, `{...}`, `(...)`, `|`, `&`, `const`, `unknown`, `any`
            let mut depth_angle = 0i32;
            let mut depth_brack = 0i32;
            let mut depth_brace = 0i32;
            let mut depth_paren = 0i32;
            let start_j = j;
            while j < bytes.len() {
                let c = bytes[j];
                if depth_angle == 0 && depth_brack == 0 && depth_brace == 0 && depth_paren == 0 {
                    if matches!(c, b';' | b',' | b')' | b']' | b'}' | b'?' | b':' | b'=') {
                        break;
                    }
                    if c == b'/' && j + 1 < bytes.len() && bytes[j + 1] == b'/' {
                        break;
                    }
                    // Stop before binary operators other than `|` / `&` / `.`
                    if matches!(c, b'+' | b'-' | b'*' | b'%' | b'!' | b'>') {
                        break;
                    }
                    if c == b' ' {
                        // Look ahead: continue only if next token is `|` / `&` / `as ` / `satisfies `
                        let rest_t = s[j..].trim_start();
                        let ws_skip = s[j..].len() - rest_t.len();
                        if rest_t.starts_with('|') || rest_t.starts_with('&') {
                            j += ws_skip + 1;
                            while j < bytes.len() && bytes[j] == b' ' {
                                j += 1;
                            }
                            continue;
                        }
                        if rest_t.starts_with("as ") {
                            j += ws_skip + 3;
                            while j < bytes.len() && bytes[j] == b' ' {
                                j += 1;
                            }
                            continue;
                        }
                        if rest_t.starts_with("satisfies ") {
                            j += ws_skip + 10;
                            while j < bytes.len() && bytes[j] == b' ' {
                                j += 1;
                            }
                            continue;
                        }
                        break;
                    }
                }
                match c {
                    b'<' => depth_angle += 1,
                    b'>' => {
                        if depth_angle > 0 {
                            depth_angle -= 1;
                        } else {
                            break;
                        }
                    }
                    b'[' => depth_brack += 1,
                    b']' => {
                        if depth_brack > 0 {
                            depth_brack -= 1;
                        } else {
                            break;
                        }
                    }
                    b'{' => depth_brace += 1,
                    b'}' => {
                        if depth_brace > 0 {
                            depth_brace -= 1;
                        } else {
                            break;
                        }
                    }
                    b'(' => depth_paren += 1,
                    b')' => {
                        if depth_paren > 0 {
                            depth_paren -= 1;
                        } else {
                            break;
                        }
                    }
                    _ => {}
                }
                j += 1;
            }
            if j > start_j {
                i = j;
                continue;
            }
        }

        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Convert `typeof expr` -> `@typeName(expr)` outside strings and comments.
fn convert_typeof_exprs(s: &str) -> String {
    if !s.contains("typeof") {
        return s.to_string();
    }
    let bytes = s.as_bytes();
    let mut out = String::with_capacity(s.len() + 16);
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_single && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'\'' {
            in_single = true;
            out.push('\'');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        if i + 6 <= bytes.len()
            && &bytes[i..i + 6] == b"typeof"
            && (i == 0 || !is_ident_byte(bytes[i - 1]))
            && (i + 6 < bytes.len() && (bytes[i + 6] == b' ' || bytes[i + 6] == b'('))
        {
            let mut j = i + 6;
            while j < bytes.len() && bytes[j] == b' ' {
                j += 1;
            }
            if j < bytes.len() && bytes[j] == b'(' {
                if let Some(rp) = find_matching_delim(s, j, b'(', b')') {
                    out.push_str("@typeName");
                    out.push_str(&s[j..=rp]);
                    i = rp + 1;
                    continue;
                }
            } else {
                let expr_start = j;
                while j < bytes.len() {
                    let c = bytes[j];
                    if is_ident_byte(c) || c == b'.' {
                        j += 1;
                    } else if c == b'[' {
                        if let Some(rb) = find_matching_delim(s, j, b'[', b']') {
                            j = rb + 1;
                        } else {
                            break;
                        }
                    } else if c == b'?' && j + 1 < bytes.len() && bytes[j + 1] == b'.' {
                        j += 2;
                    } else {
                        break;
                    }
                }
                if j > expr_start {
                    out.push_str("@typeName(");
                    out.push_str(&s[expr_start..j]);
                    out.push(')');
                    i = j;
                    continue;
                }
            }
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Rename TS identifiers that are reserved keywords in CP (e.g. `match` -> `match_val`)
/// when used as standalone identifiers (not after `.`).
fn rename_cp_keyword_idents(s: &str) -> String {
    if !s.contains("match") {
        return s.to_string();
    }
    let bytes = s.as_bytes();
    let fbytes = b"match";
    let flen = fbytes.len();
    let mut out = String::with_capacity(s.len() + 8);
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_single && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'\'' {
            in_single = true;
            out.push('\'');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        if i + flen <= bytes.len() && &bytes[i..i + flen] == fbytes {
            let prev_ok = i == 0 || (!is_ident_byte(bytes[i - 1]) && bytes[i - 1] != b'.');
            let next_ok = i + flen == bytes.len() || !is_ident_byte(bytes[i + flen]);
            if prev_ok && next_ok {
                out.push_str("match_val");
                i += flen;
                continue;
            }
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Strip TS type annotations & default values inside inline arrow functions:
/// `(a: T, b?: U = 0): Ret =>` -> `(a, b) =>`
fn convert_inline_arrows(s: &str) -> String {
    if !s.contains("=>") {
        return s.to_string();
    }
    let bytes = s.as_bytes();
    let mut out = String::with_capacity(s.len());
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_single && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'\'' {
            in_single = true;
            out.push('\'');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        if b == b'(' {
            if let Some(rp) = find_matching_delim(s, i, b'(', b')') {
                let after = s[rp + 1..].trim_start();
                let ws_len = s[rp + 1..].len() - after.len();
                // Check if followed by `=>` or `: RetType =>`
                let arrow_offset = if after.starts_with("=>") {
                    Some(rp + 1 + ws_len)
                } else if after.starts_with(':') {
                    after.find("=>").map(|pos| rp + 1 + ws_len + pos)
                } else {
                    None
                };
                if let Some(arr_idx) = arrow_offset {
                    let inner = &s[i + 1..rp];
                    let clean_params: Vec<String> = split_top_level(inner, ',')
                        .into_iter()
                        .filter_map(|p| {
                            let mut pt = p.trim();
                            if pt.is_empty() {
                                return None;
                            }
                            if let Some(r) = pt.strip_prefix("...") {
                                pt = r.trim();
                            }
                            if pt.starts_with('{') || pt.starts_with('[') {
                                return Some("_p".to_string());
                            }
                            let head = split_top_level(pt, '=').into_iter().next().unwrap_or_default();
                            let name = head
                                .split(':')
                                .next()
                                .unwrap_or("")
                                .trim()
                                .trim_end_matches('?')
                                .trim();
                            if name.is_empty() {
                                None
                            } else if name == "_" {
                                Some("_unused".to_string())
                            } else {
                                Some(name.to_string())
                            }
                        })
                        .collect();
                    out.push('(');
                    out.push_str(&clean_params.join(", "));
                    out.push_str(") =>");
                    i = arr_idx + 2;
                    continue;
                }
            }
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Split an expression string by `delim` at top level of `()`, `[]`, `{}` (ignoring `<` / `>` comparison ops).
fn split_expr_top_level(s: &str, delim: char) -> Vec<String> {
    let mut parts = Vec::new();
    let mut cur = String::new();
    let mut depth_paren = 0i32;
    let mut depth_brack = 0i32;
    let mut depth_brace = 0i32;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;
    let mut prev_bs = false;

    for ch in s.chars() {
        if in_double {
            cur.push(ch);
            if ch == '"' && !prev_bs {
                in_double = false;
            }
            prev_bs = ch == '\\' && !prev_bs;
            continue;
        }
        if in_single {
            cur.push(ch);
            if ch == '\'' && !prev_bs {
                in_single = false;
            }
            prev_bs = ch == '\\' && !prev_bs;
            continue;
        }
        if in_backtick {
            cur.push(ch);
            if ch == '`' && !prev_bs {
                in_backtick = false;
            }
            prev_bs = ch == '\\' && !prev_bs;
            continue;
        }
        prev_bs = false;
        match ch {
            '"' => {
                in_double = true;
                cur.push(ch);
            }
            '\'' => {
                in_single = true;
                cur.push(ch);
            }
            '`' => {
                in_backtick = true;
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
            c if c == delim && depth_paren == 0 && depth_brack == 0 && depth_brace == 0 => {
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

/// Strip TS generic type arguments on call expressions: `foo<Bar, Baz>(x)` -> `foo(x)`.
fn strip_call_type_args(s: &str) -> String {
    if !s.contains('<') || !s.contains(">(") {
        return s.to_string();
    }
    let bytes = s.as_bytes();
    let mut out = String::with_capacity(s.len());
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_single && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'\'' {
            in_single = true;
            out.push('\'');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        if b == b'<' && i > 0 && is_ident_byte(bytes[i - 1]) {
            if let Some(gt) = find_matching_delim(s, i, b'<', b'>') {
                if bytes.get(gt + 1) == Some(&b'(') {
                    let inner = &s[i + 1..gt];
                    if !inner.contains(';')
                        && !inner.contains("==")
                        && !inner.contains("!=")
                        && !inner.contains("&&")
                        && !inner.contains("||")
                    {
                        i = gt + 1;
                        continue;
                    }
                }
            }
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Convert single-line TS ternary `cond ? a : b` (including parenthesized `(cond ? a : b)`) into CP `if (cond) a else b`.
fn convert_ternary_expr(s: &str) -> String {
    let (code, comment) = split_inline_comment(s);
    if !code.contains('?') || !code.contains(':') {
        return s.to_string();
    }
    let step1 = convert_paren_ternaries(code.trim_end());
    let converted = convert_ternary_inner(&step1);
    if comment.is_empty() {
        converted
    } else {
        format!("{} {}", converted, comment)
    }
}

/// Convert parenthesized ternaries `(cond ? a : b)` inside larger expressions.
fn convert_paren_ternaries(s: &str) -> String {
    if !s.contains('(') || !s.contains('?') {
        return s.to_string();
    }
    let bytes = s.as_bytes();
    let mut out = String::with_capacity(s.len() + 16);
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'\'' {
            in_single = true;
            out.push('\'');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        if b == b'(' {
            if let Some(rp) = find_matching_delim(s, i, b'(', b')') {
                let inner = &s[i + 1..rp];
                if inner.contains('?') && inner.contains(':') {
                    let conv_inner = convert_ternary_inner(&convert_paren_ternaries(inner));
                    out.push('(');
                    out.push_str(&conv_inner);
                    out.push(')');
                    i = rp + 1;
                    continue;
                }
            }
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

fn convert_ternary_inner(s: &str) -> String {
    let bytes = s.as_bytes();
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;
    let mut depth_paren = 0i32;
    let mut depth_brack = 0i32;
    let mut depth_brace = 0i32;

    // Find top-level `?` (excluding `?.`, `??`, `?:` optional params)
    let mut q_pos: Option<usize> = None;
    let mut i = 0;
    while i < bytes.len() {
        let b = bytes[i];
        if in_double {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        match b {
            b'"' => in_double = true,
            b'\'' => in_single = true,
            b'`' => in_backtick = true,
            b'(' => depth_paren += 1,
            b')' => depth_paren -= 1,
            b'[' => depth_brack += 1,
            b']' => depth_brack -= 1,
            b'{' => depth_brace += 1,
            b'}' => depth_brace -= 1,
            b'?' if depth_paren == 0 && depth_brack == 0 && depth_brace == 0 => {
                let next = bytes.get(i + 1).copied();
                let prev = if i > 0 { Some(bytes[i - 1]) } else { None };
                let is_ternary_q = !matches!(next, Some(b'.') | Some(b'?') | Some(b':') | Some(b',') | Some(b')') | Some(b';') | Some(b'}') | None)
                    && prev != Some(b'?');
                if is_ternary_q {
                    q_pos = Some(i);
                    break;
                }
            }
            _ => {}
        }
        i += 1;
    }

    let Some(q) = q_pos else {
        return s.to_string();
    };

    // Find matching top-level `:` after `q`
    let mut c_pos: Option<usize> = None;
    let mut nested_q = 0i32;
    in_double = false;
    in_single = false;
    in_backtick = false;
    depth_paren = 0;
    depth_brack = 0;
    depth_brace = 0;
    i = q + 1;
    while i < bytes.len() {
        let b = bytes[i];
        if in_double {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        match b {
            b'"' => in_double = true,
            b'\'' => in_single = true,
            b'`' => in_backtick = true,
            b'(' => depth_paren += 1,
            b')' => depth_paren -= 1,
            b'[' => depth_brack += 1,
            b']' => depth_brack -= 1,
            b'{' => depth_brace += 1,
            b'}' => depth_brace -= 1,
            b'?' if depth_paren == 0 && depth_brack == 0 && depth_brace == 0 => {
                let next = bytes.get(i + 1).copied();
                if !matches!(next, Some(b'.') | Some(b'?') | Some(b':')) {
                    nested_q += 1;
                }
            }
            b':' if depth_paren == 0 && depth_brack == 0 && depth_brace == 0 => {
                if nested_q == 0 {
                    c_pos = Some(i);
                    break;
                } else {
                    nested_q -= 1;
                }
            }
            _ => {}
        }
        i += 1;
    }

    let Some(c) = c_pos else {
        return s.to_string();
    };

    let before_q = &s[..q];
    let then_part = s[q + 1..c].trim();
    let raw_else = s[c + 1..].trim();
    let (else_part, trail_semi) = if let Some(stripped) = raw_else.strip_suffix(';') {
        (stripped.trim(), ";")
    } else if let Some(stripped) = raw_else.strip_suffix(',') {
        (stripped.trim(), ",")
    } else {
        (raw_else, "")
    };

    // Split statement prefix (`return `, `const x = `, `let x = `, `x = `, `key: `) from `before_q`
    let (prefix, cond) = split_ternary_prefix(before_q);
    let conv_then = convert_ternary_inner(then_part);
    let conv_else = convert_ternary_inner(else_part);
    format!(
        "{}if ({}) {} else {}{}",
        prefix,
        cond.trim(),
        conv_then,
        conv_else,
        trail_semi
    )
}

fn split_ternary_prefix(before_q: &str) -> (&str, &str) {
    let trimmed = before_q.trim_start();
    let lead = before_q.len() - trimmed.len();
    if let Some(rest) = trimmed.strip_prefix("return ") {
        let p_end = lead + 7;
        return (&before_q[..p_end], rest);
    }
    // Check for top-level ` = ` (not `==`, `!=`, `<=`, `>=`, `=>`)
    let bytes = before_q.as_bytes();
    let mut depth = 0i32;
    let mut in_str = false;
    let mut last_assign: Option<usize> = None;
    let mut idx = 0;
    while idx < bytes.len() {
        let b = bytes[idx];
        if in_str {
            if b == b'\\' {
                idx += 2;
                continue;
            }
            if b == b'"' {
                in_str = false;
            }
            idx += 1;
            continue;
        }
        match b {
            b'"' => in_str = true,
            b'(' | b'[' | b'{' => depth += 1,
            b')' | b']' | b'}' => depth -= 1,
            b'=' if depth == 0 => {
                let prev = if idx > 0 { bytes[idx - 1] } else { 0 };
                let next = bytes.get(idx + 1).copied().unwrap_or(0);
                if !matches!(prev, b'=' | b'!' | b'<' | b'>') && !matches!(next, b'=' | b'>') {
                    last_assign = Some(idx);
                }
            }
            _ => {}
        }
        idx += 1;
    }
    if let Some(pos) = last_assign {
        let split_at = pos + 1;
        let ws = before_q[split_at..].len() - before_q[split_at..].trim_start().len();
        return (&before_q[..split_at + ws], &before_q[split_at + ws..]);
    }
    // Check for `key: ` in object literal
    if let Some(colon) = before_q.find(':') {
        let lhs = before_q[..colon].trim();
        if !lhs.is_empty() && lhs.chars().all(|c| c.is_ascii_alphanumeric() || c == '_') {
            let split_at = colon + 1;
            let ws = before_q[split_at..].len() - before_q[split_at..].trim_start().len();
            return (&before_q[..split_at + ws], &before_q[split_at + ws..]);
        }
    }
    (&before_q[..lead], trimmed)
}

/// Rewrite TS expressions inside `${...}` interpolations of backtick template strings.
fn convert_template_interpolations(s: &str, todo_count: &mut usize) -> String {
    if !s.contains('`') || !s.contains("${") {
        return s.to_string();
    }
    let bytes = s.as_bytes();
    let mut out = String::with_capacity(s.len() + 16);
    let mut i = 0;
    let mut in_double = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            if b == b'\\' && i + 1 < bytes.len() {
                out.push(b as char);
                i += 1;
                out.push(bytes[i] as char);
                i += 1;
                continue;
            }
            if b == b'$' && i + 1 < bytes.len() && bytes[i + 1] == b'{' {
                if let Some(rb) = find_matching_delim(s, i + 1, b'{', b'}') {
                    let inner = &s[i + 2..rb];
                    let conv = convert_ts_expr(inner, todo_count);
                    out.push_str("${");
                    out.push_str(&conv);
                    out.push('}');
                    i = rb + 1;
                    continue;
                }
            }
            if b == b'`' {
                in_backtick = false;
            }
            let ch = s[i..].chars().next().unwrap();
            out.push(ch);
            i += ch.len_utf8();
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Convert `new Foo(...)` -> `Foo.init(...)` (and `new Foo<T>(...)` -> `Foo.init(...)`).
fn convert_new_expressions(s: &str) -> String {
    if !s.contains("new ") {
        return s.to_string();
    }
    let bytes = s.as_bytes();
    let mut out = String::with_capacity(s.len() + 8);
    let mut i = 0;
    let mut in_double = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        if i + 4 <= bytes.len()
            && &bytes[i..i + 4] == b"new "
            && (i == 0 || !is_ident_byte(bytes[i - 1]))
        {
            let mut j = i + 4;
            while j < bytes.len() && bytes[j] == b' ' {
                j += 1;
            }
            let name_start = j;
            while j < bytes.len() && (is_ident_byte(bytes[j]) || bytes[j] == b'.') {
                j += 1;
            }
            let type_name = &s[name_start..j];
            if !type_name.is_empty() && type_name != "Function" {
                // Skip optional generic arguments `<...>`
                let mut k = j;
                if k < bytes.len() && bytes[k] == b'<' {
                    if let Some(gt) = find_matching_delim(s, k, b'<', b'>') {
                        k = gt + 1;
                    }
                }
                if k < bytes.len() && bytes[k] == b'(' {
                    out.push_str(type_name);
                    out.push_str(".init(");
                    i = k + 1;
                    continue;
                }
            }
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Replace JS regex literals like `/\+/g` with a string placeholder so `cpc` lexer/parser
/// does not fail on unescaped `/` regex delimiters.
fn sanitize_regex_literals(s: &str, todo_count: &mut usize) -> String {
    let bytes = s.as_bytes();
    let mut out = String::with_capacity(s.len());
    let mut i = 0;
    let mut in_double = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        if b == b'/' && i + 1 < bytes.len() && bytes[i + 1] != b'/' && bytes[i + 1] != b'*' {
            // Heuristic: regex literal follows `(`, `,`, `=`, `:`, `?`, `!`, `&`, `|`, `[`, `{`, `;`, or start
            let prev_non_ws = s[..i].trim_end().as_bytes().last().copied();
            let looks_like_regex_pos = matches!(
                prev_non_ws,
                None | Some(b'(')
                    | Some(b',')
                    | Some(b'=')
                    | Some(b':')
                    | Some(b'?')
                    | Some(b'!')
                    | Some(b'&')
                    | Some(b'|')
                    | Some(b'[')
                    | Some(b'{')
                    | Some(b';')
            );
            if looks_like_regex_pos {
                // Scan until closing unescaped `/`
                let mut j = i + 1;
                let mut in_class = false;
                let mut found_end = None;
                while j < bytes.len() {
                    let c = bytes[j];
                    if c == b'\\' {
                        j += 2;
                        continue;
                    }
                    if c == b'[' {
                        in_class = true;
                    } else if c == b']' {
                        in_class = false;
                    } else if c == b'/' && !in_class {
                        found_end = Some(j);
                        break;
                    }
                    j += 1;
                }
                if let Some(end_slash) = found_end {
                    let mut k = end_slash + 1;
                    while k < bytes.len() && bytes[k].is_ascii_alphabetic() {
                        k += 1;
                    }
                    let pat = &s[i + 1..end_slash];
                    let escaped = pat.replace('\\', "\\\\").replace('"', "\\\"");
                    *todo_count += 1;
                    out.push_str(&format!("\"{}\"", escaped));
                    i = k;
                    continue;
                }
            }
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Convert TS local variable type annotation `const x: string[] = ...` -> `const x: []string = ...`
/// or strip complex TS type annotation so CP infers it cleanly.
fn convert_local_var_type_annotation(s: &str, todo_count: &mut usize) -> String {
    let trimmed = s.trim_start();
    let lead = &s[..s.len() - trimmed.len()];
    let (kw, rest) = if let Some(r) = trimmed.strip_prefix("const ") {
        ("const ", r)
    } else if let Some(r) = trimmed.strip_prefix("let ") {
        ("let ", r)
    } else if let Some(r) = trimmed.strip_prefix("pub const ") {
        ("pub const ", r)
    } else if let Some(r) = trimmed.strip_prefix("pub let ") {
        ("pub let ", r)
    } else {
        return s.to_string();
    };
    let rest_t = rest.trim_start();
    if rest_t.starts_with('{') || rest_t.starts_with('[') {
        return s.to_string();
    }
    // Check `ident: Type = ...` or `ident: Type;`
    let name_end = rest_t
        .find(|c: char| !c.is_ascii_alphanumeric() && c != '_')
        .unwrap_or(rest_t.len());
    if name_end == 0 {
        return s.to_string();
    }
    let after_name = rest_t[name_end..].trim_start();
    if !after_name.starts_with(':') {
        return s.to_string();
    }
    let after_colon = after_name[1..].trim_start();
    // Find top-level `=` (not `=>`)
    let bytes = after_colon.as_bytes();
    let mut depth = 0i32;
    let mut eq_idx = None;
    let mut idx = 0;
    while idx < bytes.len() {
        match bytes[idx] {
            b'<' | b'(' | b'[' | b'{' => depth += 1,
            b'>' if idx > 0 && bytes[idx - 1] == b'=' => {}
            b'>' | b')' | b']' | b'}' => {
                if depth > 0 {
                    depth -= 1;
                }
            }
            b'=' if depth == 0 && bytes.get(idx + 1) != Some(&b'>') && bytes.get(idx + 1) != Some(&b'=') => {
                eq_idx = Some(idx);
                break;
            }
            _ => {}
        }
        idx += 1;
    }
    let vname = &rest_t[..name_end];
    if let Some(eq) = eq_idx {
        let raw_ty = after_colon[..eq].trim();
        let rhs = after_colon[eq + 1..].trim();
        if is_complex_ts_type(raw_ty) || raw_ty.contains('|') {
            // Let CP infer the type cleanly
            return format!("{}{}{} = {}", lead, kw, vname, rhs);
        }
        let mapped = map_ts_type(raw_ty, todo_count);
        let clean = mapped.split(" /*").next().unwrap_or(&mapped).trim();
        if clean.is_empty() || clean == "void" {
            return format!("{}{}{} = {}", lead, kw, vname, rhs);
        }
        return format!("{}{}{}: {} = {}", lead, kw, vname, clean, rhs);
    }
    s.to_string()
}

/// Strip TS prefix angle-bracket casts like `<Function[]>[]` or `<Record<string, unknown>>{}`.
fn strip_prefix_angle_casts(s: &str) -> String {
    if !s.contains('<') {
        return s.to_string();
    }
    let bytes = s.as_bytes();
    let mut out = String::with_capacity(s.len());
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_single && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'\'' {
            in_single = true;
            out.push('\'');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        if b == b'<' {
            let prev_non_ws = s[..i].trim_end().as_bytes().last().copied();
            let is_cast_pos = matches!(prev_non_ws, Some(b'=') | Some(b'(') | Some(b',') | Some(b':'));
            if is_cast_pos {
                if let Some(gt) = find_matching_delim(s, i, b'<', b'>') {
                    let after_gt = s[gt + 1..].trim_start().as_bytes().first().copied();
                    if matches!(after_gt, Some(b'{') | Some(b'[') | Some(b'(') | Some(b'"')) {
                        i = gt + 1;
                        continue;
                    }
                }
            }
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Strip JS unary `+expr` numeric coercion (unsupported in CP), e.g. `= +matches[1]` -> `= matches[1]`.
fn strip_unary_plus(s: &str) -> String {
    if !s.contains('+') {
        return s.to_string();
    }
    let bytes = s.as_bytes();
    let mut out = String::with_capacity(s.len());
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_single && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'\'' {
            in_single = true;
            out.push('\'');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        if b == b'+'
            && bytes.get(i + 1) != Some(&b'+')
            && bytes.get(i + 1) != Some(&b'=')
            && (i == 0 || bytes[i - 1] != b'+')
        {
            let prev_non_ws = s[..i].trim_end().as_bytes().last().copied();
            if matches!(
                prev_non_ws,
                None | Some(b'=') | Some(b'(') | Some(b',') | Some(b':') | Some(b'[') | Some(b'?')
            ) {
                i += 1;
                continue;
            }
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Quote bare numeric object literal keys (`101: null,` -> `"101": null,`).
fn quote_numeric_object_key(s: &str) -> String {
    let trimmed = s.trim_start();
    let lead = &s[..s.len() - trimmed.len()];
    if let Some(colon) = trimmed.find(':') {
        let key = trimmed[..colon].trim();
        if !key.is_empty() && key.bytes().all(|b| b.is_ascii_digit()) {
            return format!("{}\"{}\"{}", lead, key, &trimmed[colon..]);
        }
    }
    s.to_string()
}

/// Convert inline JS/TS function expressions `function [name](a: T): R {` -> `(a) => {`.
fn convert_inline_function_exprs(s: &str) -> String {
    if !s.contains("function ") && !s.contains("function(") {
        return s.to_string();
    }
    let bytes = s.as_bytes();
    let mut out = String::with_capacity(s.len());
    let mut i = 0;
    let mut in_double = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            out.push_str(&s[i..]);
            break;
        }
        if in_double {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_backtick {
            out.push(b as char);
            if b == b'\\' && i + 1 < bytes.len() {
                i += 1;
                out.push(bytes[i] as char);
            } else if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if b == b'"' {
            in_double = true;
            out.push('"');
            i += 1;
            continue;
        }
        if b == b'`' {
            in_backtick = true;
            out.push('`');
            i += 1;
            continue;
        }
        if i + 8 <= bytes.len()
            && &bytes[i..i + 8] == b"function"
            && (i == 0 || !is_ident_byte(bytes[i - 1]))
            && (i + 8 < bytes.len() && (bytes[i + 8] == b' ' || bytes[i + 8] == b'(' || bytes[i + 8] == b'*'))
        {
            let mut j = i + 8;
            while j < bytes.len() && (bytes[j] == b' ' || bytes[j] == b'*') {
                j += 1;
            }
            // Skip optional function name
            while j < bytes.len() && is_ident_byte(bytes[j]) {
                j += 1;
            }
            while j < bytes.len() && bytes[j] == b' ' {
                j += 1;
            }
            // Skip optional generics `<...>`
            if j < bytes.len() && bytes[j] == b'<' {
                if let Some(gt) = find_matching_delim(s, j, b'<', b'>') {
                    j = gt + 1;
                }
            }
            while j < bytes.len() && bytes[j] == b' ' {
                j += 1;
            }
            if j < bytes.len() && bytes[j] == b'(' {
                if let Some(rp) = find_matching_delim(s, j, b'(', b')') {
                    let after = &s[rp + 1..];
                    if let Some(lb_rel) = after.find('{') {
                        let params_raw = &s[j + 1..rp];
                        let clean_params: Vec<String> = split_top_level(params_raw, ',')
                            .into_iter()
                            .filter_map(|p| {
                                let mut pt = p.trim();
                                if pt.is_empty() {
                                    return None;
                                }
                                if let Some(r) = pt.strip_prefix("...") {
                                    pt = r.trim();
                                }
                                if pt.starts_with('{') || pt.starts_with('[') {
                                    return Some("_p".to_string());
                                }
                                let head = split_top_level(pt, '=').into_iter().next().unwrap_or_default();
                                let name = head.split(':').next().unwrap_or("").trim().trim_end_matches('?').trim();
                                if name.is_empty() { None } else { Some(name.to_string()) }
                            })
                            .collect();
                        out.push('(');
                        out.push_str(&clean_params.join(", "));
                        out.push_str(") => ");
                        i = rp + 1 + lb_rel;
                        continue;
                    }
                }
            }
        }
        let ch = s[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Apply core TS -> CP expression & statement rewrites on a single line.
fn convert_ts_expr(raw: &str, todo_count: &mut usize) -> String {
    // Sanitize regex literals BEFORE single-quote conversion so regexes containing `'` or `` ` ``
    // do not corrupt string literal scanning!
    let mut s = sanitize_regex_literals(raw, todo_count);
    s = convert_single_quoted_strings(&s);
    s = convert_template_interpolations(&s, todo_count);
    s = convert_inline_function_exprs(&s);
    s = strip_ts_expr_noise(&s);
    s = strip_prefix_angle_casts(&s);
    s = strip_call_type_args(&s);
    s = strip_unary_plus(&s);
    s = quote_numeric_object_key(&s);
    s = replace_ident_word(&s, "var", "let");
    s = replace_outside_strings(&s, "===", "==");
    s = replace_outside_strings(&s, "!==", "!=");
    s = replace_ident_word(&s, "undefined", "null");
    s = replace_outside_strings(&s, "this.", "_.");
    s = replace_outside_strings(&s, "console.log(", "@print(");
    s = replace_outside_strings(&s, "console.error(", "@print(");
    s = replace_outside_strings(&s, "console.warn(", "@print(");
    s = replace_outside_strings(&s, "console.info(", "@print(");
    s = convert_new_expressions(&s);
    // Unsigned right shift `>>>` -> `>>`
    s = replace_outside_strings(&s, ">>>", ">>");
    // `typeof x` -> `@typeName(x)`
    s = convert_typeof_exprs(&s);
    // `instanceof` -> `==` with TODO note
    if s.contains(" instanceof ") {
        *todo_count += 1;
        s = replace_outside_strings(&s, " instanceof ", " == ");
    }
    // JS `key in obj` expression (outside `for (... in ...)`) -> `==` placeholder with TODO
    let trimmed_s = s.trim_start();
    if !trimmed_s.starts_with("for ") && !trimmed_s.starts_with("for(") && s.contains(" in ") {
        *todo_count += 1;
        s = replace_outside_strings(&s, " in ", " == ");
    }
    // Rename CP reserved keywords used as TS variables (`match` -> `match_val`)
    s = rename_cp_keyword_idents(&s);
    // Strip TS annotations in inline arrow functions `(a: T) =>` -> `(a) =>`
    s = convert_inline_arrows(&s);
    // Convert TS variable type annotations `const x: string[] = []` -> `const x: []string = []`
    s = convert_local_var_type_annotation(&s, todo_count);
    // Convert single-line TS ternary `cond ? a : b` -> `if (cond) a else b`
    s = convert_ternary_expr(&s);
    s
}

/// Detect whether a TS statement uses dynamic metaprogramming that cannot map to CP.
fn is_dynamic_metaprogramming(trimmed: &str) -> bool {
    trimmed.contains("new Function(")
        || trimmed.contains("eval(")
        || trimmed.contains("Object.defineProperty(")
        || trimmed.contains("Object.defineProperties(")
        || trimmed.contains("Reflect.")
}

/// Check if a method body (slice of lines) mutates `this.<field>` or `_.<field>`.
fn method_body_mutates_self(lines: &[&str], start_line: usize) -> bool {
    let mut depth = 0i32;
    let mut started = false;
    for &line in &lines[start_line..] {
        let (code, _) = split_inline_comment(line);
        let (o, c) = count_braces(code);
        if o > 0 {
            started = true;
        }
        // Check for `this.foo =` or `_.foo =` (excluding `==` / `===` / `!=`)
        for prefix in &["this.", "_."] {
            let mut search_from = 0;
            while let Some(pos) = code[search_from..].find(prefix) {
                let abs = search_from + pos + prefix.len();
                let rest = &code[abs..];
                let mut k = 0;
                let rbytes = rest.as_bytes();
                while k < rbytes.len()
                    && (is_ident_byte(rbytes[k]) || rbytes[k] == b'.' || rbytes[k] == b'[' || rbytes[k] == b']')
                {
                    k += 1;
                }
                let after = rest[k..].trim_start();
                if (after.starts_with('=') && !after.starts_with("=="))
                    || after.starts_with("+=")
                    || after.starts_with("-=")
                    || after.starts_with("*=")
                    || after.starts_with("/=")
                    || after.starts_with("|=")
                    || after.starts_with("&=")
                    || after.starts_with("??=")
                    || after.starts_with("++")
                    || after.starts_with("--")
                {
                    return true;
                }
                search_from = abs;
            }
        }
        depth += o - c;
        if started && depth <= 0 {
            break;
        }
    }
    false
}

/// Parse a top-level or class function/method header line.
/// Returns `Some(converted_line)` if matched.
fn try_convert_function_decl(
    trimmed: &str,
    indent: &str,
    todo_count: &mut usize,
) -> Option<String> {
    let mut rest = trimmed;
    let is_pub = if let Some(r) = rest.strip_prefix("export default ") {
        rest = r.trim();
        true
    } else if let Some(r) = rest.strip_prefix("export ") {
        rest = r.trim();
        true
    } else {
        false
    };

    let is_async = if let Some(r) = rest.strip_prefix("async ") {
        rest = r.trim();
        true
    } else {
        false
    };

    rest = rest.strip_prefix("function ")?.trim();
    // Generator `function* foo`
    if let Some(r) = rest.strip_prefix('*') {
        rest = r.trim();
    }

    let name_end = rest.find(|c: char| !c.is_ascii_alphanumeric() && c != '_')?;
    let fname = &rest[..name_end];
    if fname.is_empty() {
        return None;
    }
    let after_name = rest[name_end..].trim();

    let (generics, after_gen) = if after_name.starts_with('<') {
        if let Some(gt) = find_matching_delim(after_name, 0, b'<', b'>') {
            (
                simplify_ts_generics(&after_name[..=gt]),
                after_name[gt + 1..].trim(),
            )
        } else {
            (String::new(), after_name)
        }
    } else {
        (String::new(), after_name)
    };

    if !after_gen.starts_with('(') {
        return None;
    }
    let rparen = find_matching_delim(after_gen, 0, b'(', b')')?;
    let params_raw = &after_gen[1..rparen];
    let after_params = after_gen[rparen + 1..].trim();

    let (params, _) = convert_ts_params(params_raw, false, todo_count);

    // Extract optional return type before `{`
    let (ret_str, body_tail) = if let Some(lbrace) = after_params.find('{') {
        let before_brace = after_params[..lbrace].trim();
        let ret = before_brace.strip_prefix(':').map(|s| s.trim());
        (format_return_type(ret, todo_count), &after_params[lbrace..])
    } else {
        let ret = after_params.strip_prefix(':').map(|s| s.trim());
        (format_return_type(ret, todo_count), "")
    };

    let pub_kw = if is_pub { "pub " } else { "" };
    let async_kw = if is_async { "async " } else { "" };
    let tail = if body_tail.is_empty() {
        String::new()
    } else {
        format!(" {}", convert_ts_expr(body_tail, todo_count))
    };

    Some(format!(
        "{}{}{}fn {}{}({}){}{}",
        indent,
        pub_kw,
        async_kw,
        fname,
        generics,
        params.join(", "),
        ret_str,
        tail
    ))
}

/// Try converting a top-level arrow function:
/// `export const foo = [async] [<T>](a: T): R => { ... }` or `=> expr`
fn try_convert_top_level_arrow_fn(
    trimmed: &str,
    indent: &str,
    todo_count: &mut usize,
) -> Option<String> {
    let mut rest = trimmed;
    let is_pub = if let Some(r) = rest.strip_prefix("export ") {
        rest = r.trim();
        true
    } else {
        false
    };

    rest = rest
        .strip_prefix("const ")
        .or_else(|| rest.strip_prefix("let "))?
        .trim();

    // Must have `= ... =>`
    let eq_pos = rest.find('=')?;
    // Ignore `==`
    if rest.as_bytes().get(eq_pos + 1) == Some(&b'=') {
        return None;
    }
    let lhs = rest[..eq_pos].trim();
    // Skip destructuring or complex typed const
    let fname = lhs.split(':').next()?.trim();
    if fname.is_empty()
        || !fname
            .chars()
            .all(|c| c.is_ascii_alphanumeric() || c == '_')
    {
        return None;
    }

    let mut rhs = rest[eq_pos + 1..].trim();
    let is_async = if let Some(r) = rhs.strip_prefix("async ") {
        rhs = r.trim();
        true
    } else {
        false
    };

    let (generics, after_gen) = if rhs.starts_with('<') {
        if let Some(gt) = find_matching_delim(rhs, 0, b'<', b'>') {
            (simplify_ts_generics(&rhs[..=gt]), rhs[gt + 1..].trim())
        } else {
            return None;
        }
    } else {
        (String::new(), rhs)
    };

    if !after_gen.starts_with('(') {
        return None;
    }
    let rparen = find_matching_delim(after_gen, 0, b'(', b')')?;
    let params_raw = &after_gen[1..rparen];
    let after_params = after_gen[rparen + 1..].trim();

    // Find top-level `=>` (ignoring `=>` inside function return types like `(() => Response) | undefined =>`)
    let arrow_pos = {
        let ab = after_params.as_bytes();
        let mut dp = 0i32;
        let mut da = 0i32;
        let mut db = 0i32;
        let mut dbr = 0i32;
        let mut found = None;
        let mut idx = 0;
        while idx + 1 < ab.len() {
            match ab[idx] {
                b'(' => dp += 1,
                b')' => dp = (dp - 1).max(0),
                b'<' => da += 1,
                b'>' if idx > 0 && ab[idx - 1] == b'=' => {}
                b'>' => da = (da - 1).max(0),
                b'[' => db += 1,
                b']' => db = (db - 1).max(0),
                b'{' => dbr += 1,
                b'}' => dbr = (dbr - 1).max(0),
                b'=' if ab[idx + 1] == b'>' && dp == 0 && da == 0 && db == 0 && dbr == 0 => {
                    found = Some(idx);
                    break;
                }
                _ => {}
            }
            idx += 1;
        }
        found?
    };
    let before_arrow = after_params[..arrow_pos].trim();
    let ret = before_arrow.strip_prefix(':').map(|s| s.trim());
    let ret_str = format_return_type(ret, todo_count);
    let body = after_params[arrow_pos + 2..].trim();

    let (params, _) = convert_ts_params(params_raw, false, todo_count);
    let pub_kw = if is_pub { "pub " } else { "" };
    let async_kw = if is_async { "async " } else { "" };

    if body.starts_with('{') {
        Some(format!(
            "{}{}{}fn {}{}({}){} {}",
            indent,
            pub_kw,
            async_kw,
            fname,
            generics,
            params.join(", "),
            ret_str,
            convert_ts_expr(body, todo_count)
        ))
    } else if !body.is_empty() {
        let expr = convert_ts_expr(body.trim_end_matches(';'), todo_count);
        Some(format!(
            "{}{}{}fn {}{}({}){} {{ return {}; }}",
            indent,
            pub_kw,
            async_kw,
            fname,
            generics,
            params.join(", "),
            ret_str,
            expr
        ))
    } else {
        None
    }
}

/// Try converting a class method / constructor / getter / setter inside `class C { ... }`.
fn try_convert_class_member(
    trimmed: &str,
    indent: &str,
    class_name: &str,
    mutates_self: bool,
    todo_count: &mut usize,
) -> Option<(String, Vec<(String, String)>)> {
    let mut rest = trimmed;
    let mut is_static = false;
    let mut is_async = false;
    let mut is_pub = true;

    loop {
        if let Some(r) = rest.strip_prefix("public ") {
            is_pub = true;
            rest = r.trim();
        } else if let Some(r) = rest.strip_prefix("private ") {
            is_pub = false;
            rest = r.trim();
        } else if let Some(r) = rest.strip_prefix("protected ") {
            is_pub = false;
            rest = r.trim();
        } else if let Some(r) = rest.strip_prefix("static ") {
            is_static = true;
            rest = r.trim();
        } else if let Some(r) = rest.strip_prefix("async ") {
            is_async = true;
            rest = r.trim();
        } else if let Some(r) = rest.strip_prefix("override ") {
            rest = r.trim();
        } else if let Some(r) = rest.strip_prefix("abstract ") {
            rest = r.trim();
        } else if let Some(r) = rest.strip_prefix("readonly ") {
            rest = r.trim();
        } else {
            break;
        }
    }

    let pub_kw = if is_pub { "pub " } else { "" };

    // 1. `constructor(args) {`
    if let Some(after_ctor) = rest.strip_prefix("constructor") {
        let after_ctor = after_ctor.trim();
        if after_ctor.starts_with('(') {
            if let Some(rparen) = find_matching_delim(after_ctor, 0, b'(', b')') {
                let params_raw = &after_ctor[1..rparen];
                let tail = after_ctor[rparen + 1..].trim();
                let (params, ctor_fields) = convert_ts_params(params_raw, true, todo_count);
                let tail_str = if tail.is_empty() {
                    "{".to_string()
                } else {
                    convert_ts_expr(tail, todo_count)
                };
                let line = format!(
                    "{}pub static fn init({}): {} {}",
                    indent,
                    params.join(", "),
                    class_name,
                    tail_str
                );
                return Some((line, ctor_fields));
            }
        }
    }

    // 2. Getter / Setter: `get foo(): T {` / `set foo(v: T) {`
    let mut accessor_prefix = "";
    if let Some(r) = rest.strip_prefix("get ") {
        if r.contains('(') {
            accessor_prefix = "get_";
            rest = r.trim();
        }
    } else if let Some(r) = rest.strip_prefix("set ") {
        if r.contains('(') {
            accessor_prefix = "set_";
            rest = r.trim();
        }
    }

    // 3. Regular method: `foo[<T>](args)[: Ret] {`
    let name_end = rest.find(|c: char| !c.is_ascii_alphanumeric() && c != '_')?;
    let mname = &rest[..name_end];
    if mname.is_empty()
        || matches!(
            mname,
            "if" | "for" | "while" | "switch" | "catch" | "return" | "const" | "let" | "var"
        )
    {
        return None;
    }
    let after_name = rest[name_end..].trim();
    let (generics, after_gen) = if after_name.starts_with('<') {
        if let Some(gt) = find_matching_delim(after_name, 0, b'<', b'>') {
            (
                simplify_ts_generics(&after_name[..=gt]),
                after_name[gt + 1..].trim(),
            )
        } else {
            return None;
        }
    } else {
        (String::new(), after_name)
    };

    if !after_gen.starts_with('(') {
        return None;
    }
    let rparen = find_matching_delim(after_gen, 0, b'(', b')')?;
    let params_raw = &after_gen[1..rparen];
    let after_params = after_gen[rparen + 1..].trim();
    // Must end with `{` or have `{` on this line to be a method definition
    let lbrace = after_params.find('{')?;
    let before_brace = after_params[..lbrace].trim();
    let ret = before_brace.strip_prefix(':').map(|s| s.trim());
    let ret_str = format_return_type(ret, todo_count);
    let body_tail = &after_params[lbrace..];

    let (mut params, _) = convert_ts_params(params_raw, false, todo_count);
    if !is_static && (mutates_self || accessor_prefix == "set_") {
        params.insert(0, "*_".to_string());
    }

    let static_kw = if is_static { "static " } else { "" };
    let async_kw = if is_async { "async " } else { "" };
    let full_name = format!("{}{}", accessor_prefix, mname);
    let line = format!(
        "{}{}{}{}fn {}{}({}){} {}",
        indent,
        pub_kw,
        static_kw,
        async_kw,
        full_name,
        generics,
        params.join(", "),
        ret_str,
        convert_ts_expr(body_tail, todo_count)
    );
    Some((line, Vec::new()))
}

/// Convert a single interface or class field line (`a: string;`, `b?: number`, `readonly x: T = v`).
fn convert_field_line(
    trimmed: &str,
    indent: &str,
    is_struct: bool,
    todo_count: &mut usize,
) -> Option<String> {
    let mut rest = trimmed.trim_end_matches(';').trim_end_matches(',').trim();
    let mut is_pub = is_struct;
    loop {
        if let Some(r) = rest.strip_prefix("public ") {
            is_pub = true;
            rest = r.trim();
        } else if let Some(r) = rest.strip_prefix("private ") {
            is_pub = false;
            rest = r.trim();
        } else if let Some(r) = rest.strip_prefix("protected ") {
            is_pub = false;
            rest = r.trim();
        } else if let Some(r) = rest.strip_prefix("readonly ") {
            rest = r.trim();
        } else if let Some(r) = rest.strip_prefix("override ") {
            rest = r.trim();
        } else if let Some(r) = rest.strip_prefix("declare ") {
            rest = r.trim();
        } else {
            break;
        }
    }

    // Index signature `[key: string]: any` -> comment
    if rest.starts_with('[') {
        *todo_count += 1;
        return Some(format!(
            "{}// TODO(cap-migrate): index signature `{}` -> use Map<K, V>",
            indent, trimmed
        ));
    }

    // Interface method signature `foo(a: T): R` -> comment
    if rest.contains('(') && !rest.contains('=') {
        let before_paren = rest.split('(').next().unwrap_or("").trim();
        if !before_paren.contains(':') {
            *todo_count += 1;
            return Some(format!(
                "{}// TODO(cap-migrate): interface method `{}` -> move to trait/impl",
                indent, trimmed
            ));
        }
    }

    // Split optional `= init` at top level (only for class fields, never split `=>` function types!)
    let (decl_part, init_val) = if is_struct {
        (rest, None)
    } else {
        let mut eq_idx = None;
        let rb = rest.as_bytes();
        let mut depth = 0i32;
        let mut idx = 0;
        while idx < rb.len() {
            match rb[idx] {
                b'(' | b'[' | b'{' | b'<' => depth += 1,
                b'>' if idx > 0 && rb[idx - 1] == b'=' => {}
                b')' | b']' | b'}' | b'>' => depth = (depth - 1).max(0),
                b'=' if depth == 0 && rb.get(idx + 1) != Some(&b'>') && rb.get(idx + 1) != Some(&b'=') => {
                    eq_idx = Some(idx);
                    break;
                }
                _ => {}
            }
            idx += 1;
        }
        if let Some(p) = eq_idx {
            (rest[..p].trim(), Some(rest[p + 1..].trim().to_string()))
        } else {
            (rest, None)
        }
    };

    let (raw_name, raw_ty) = if let Some(colon) = decl_part.find(':') {
        (decl_part[..colon].trim(), Some(decl_part[colon + 1..].trim()))
    } else {
        (decl_part, None)
    };

    let is_optional = raw_name.ends_with('?');
    let fname = raw_name
        .trim_end_matches('?')
        .trim_end_matches('!')
        .trim_matches('"')
        .trim_matches('\'')
        .trim();
    if fname.is_empty()
        || !fname
            .chars()
            .all(|c| c.is_ascii_alphanumeric() || c == '_')
    {
        return None;
    }

    let mut cp_ty = if let Some(ty_s) = raw_ty {
        let mapped = map_ts_type(ty_s, todo_count);
        mapped.split(" /*").next().unwrap_or(&mapped).trim().to_string()
    } else {
        "string".to_string()
    };

    if is_optional && !cp_ty.ends_with('?') {
        cp_ty.push('?');
    }

    // CP struct fields are implicitly pub (no redundant `pub` keyword); class fields default private
    let pub_kw = if !is_struct && is_pub { "pub " } else { "" };

    if let Some(val) = init_val {
        let conv_val = convert_ts_expr(&val, todo_count);
        Some(format!(
            "{}{}{}: {} = {};",
            indent, pub_kw, fname, cp_ty, conv_val
        ))
    } else {
        let def = default_value_for_cp_type(&cp_ty);
        if !def.is_empty() {
            Some(format!("{}{}{}: {} = {};", indent, pub_kw, fname, cp_ty, def))
        } else {
            Some(format!("{}{}{}: {};", indent, pub_kw, fname, cp_ty))
        }
    }
}

/// Convert a `try` / `catch` / `finally` line safely without triggering CP `E0103`.
fn try_convert_try_catch(trimmed: &str, indent: &str, todo_count: &mut usize) -> Option<String> {
    // Single-line `try { ... } catch ... { ... }`
    if trimmed.starts_with("try {") && trimmed.contains("catch") {
        *todo_count += 1;
        return Some(format!(
            "{}// TODO(cap-migrate): replace try/catch with CP ?T / ?: {}",
            indent, trimmed
        ));
    }
    if trimmed == "try {" || trimmed == "try{" {
        *todo_count += 1;
        return Some(format!(
            "{}{{ // TODO(cap-migrate): replace try/catch with CP ?T / ?",
            indent
        ));
    }
    if trimmed.starts_with("} catch") || trimmed.starts_with("catch") {
        *todo_count += 1;
        let prefix = if trimmed.starts_with('}') { "} " } else { "" };
        if trimmed.ends_with("{}") {
            return Some(format!(
                "{}{}// TODO(cap-migrate): replace try/catch with CP ?T / ? ({})",
                indent, prefix, trimmed
            ));
        }
        if trimmed.ends_with('{') {
            return Some(format!(
                "{}{}{{ // TODO(cap-migrate): replace try/catch with CP ?T / ? ({})",
                indent, prefix, trimmed
            ));
        }
        return Some(format!(
            "{}{}// TODO(cap-migrate): replace try/catch with CP ?T / ? ({})",
            indent, prefix, trimmed
        ));
    }
    if trimmed.starts_with("} finally") || trimmed.starts_with("finally") {
        *todo_count += 1;
        let prefix = if trimmed.starts_with('}') { "} " } else { "" };
        if trimmed.ends_with('{') {
            return Some(format!(
                "{}{}{{ // TODO(cap-migrate): replace finally with CP defer",
                indent, prefix
            ));
        }
    }
    None
}

/// Count `(` / `)` and `[` / `]` outside of strings and `//` comments.
fn count_parens_brackets(s: &str) -> (i32, i32) {
    let (code, _) = split_inline_comment(s);
    let bytes = code.as_bytes();
    let mut p_delta = 0i32;
    let mut b_delta = 0i32;
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;
    while i < bytes.len() {
        let b = bytes[i];
        if in_double {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'"' {
                in_double = false;
            }
        } else if in_single {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'\'' {
                in_single = false;
            }
        } else if in_backtick {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'`' {
                in_backtick = false;
            }
        } else {
            match b {
                b'"' => in_double = true,
                b'\'' => in_single = true,
                b'`' => in_backtick = true,
                b'(' => p_delta += 1,
                b')' => p_delta -= 1,
                b'[' => b_delta += 1,
                b']' => b_delta -= 1,
                _ => {}
            }
        }
        i += 1;
    }
    (p_delta, b_delta)
}

/// Count unescaped backticks outside double/single strings and `//` comments.
fn count_backticks(s: &str, mut in_backtick: bool) -> bool {
    let bytes = s.as_bytes();
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    while i < bytes.len() {
        let b = bytes[i];
        if !in_double && !in_single && !in_backtick && b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            break;
        }
        if in_backtick {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'`' {
                in_backtick = false;
            }
            i += 1;
            continue;
        }
        if in_double {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'"' {
                in_double = false;
            }
            i += 1;
            continue;
        }
        if in_single {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'\'' {
                in_single = false;
            }
            i += 1;
            continue;
        }
        match b {
            b'"' => in_double = true,
            b'\'' => in_single = true,
            b'`' => in_backtick = true,
            _ => {}
        }
        i += 1;
    }
    in_backtick
}

/// Find the body `{` of a TS `interface` / `class` header, skipping any `<...>` generic default `{}`.
fn has_header_body_lbrace(header: &str) -> bool {
    let bytes = header.as_bytes();
    let mut angle = 0i32;
    let mut paren = 0i32;
    let mut in_str = false;
    let mut i = 0;
    while i < bytes.len() {
        let b = bytes[i];
        if in_str {
            if b == b'\\' {
                i += 2;
                continue;
            }
            if b == b'"' || b == b'\'' {
                in_str = false;
            }
            i += 1;
            continue;
        }
        match b {
            b'"' | b'\'' => in_str = true,
            b'<' => angle += 1,
            b'>' if i > 0 && bytes[i - 1] == b'=' => {}
            b'>' => {
                if angle > 0 {
                    angle -= 1;
                }
            }
            b'(' => paren += 1,
            b')' => {
                if paren > 0 {
                    paren -= 1;
                }
            }
            b'{' if angle == 0 && paren == 0 => return true,
            _ => {}
        }
        i += 1;
    }
    false
}

/// Try converting C-style `for (init; cond; step) ...` into CP `init; while (cond) ...`.
fn try_convert_c_for_loop(trimmed: &str, indent: &str, todo_count: &mut usize) -> Option<String> {
    let rest = trimmed.strip_prefix("for")?.trim_start();
    if !rest.starts_with('(') {
        return None;
    }
    let rp = find_matching_delim(rest, 0, b'(', b')')?;
    let header = &rest[1..rp];
    // Use expression-level split (ignoring `<` / `>` comparisons like `i < len`)
    let parts = split_expr_top_level(header, ';');
    if parts.len() != 3 {
        return None;
    }
    let init = convert_ts_expr(parts[0].trim(), todo_count);
    let cond = if parts[1].trim().is_empty() {
        "true".to_string()
    } else {
        convert_ts_expr(parts[1].trim(), todo_count)
    };
    let step = convert_ts_expr(parts[2].trim(), todo_count);
    let tail = rest[rp + 1..].trim();
    let init_prefix = if init.is_empty() {
        String::new()
    } else {
        format!("{}{};\n", indent, init.trim_end_matches(';'))
    };
    if tail.is_empty() || tail == "{" {
        let brace = if tail == "{" { " {" } else { "" };
        Some(format!(
            "{}{}while ({}){} // [cap-migrate] for-step: {}",
            init_prefix, indent, cond, brace, step
        ))
    } else {
        let conv_tail = convert_ts_expr(tail.trim_end_matches(';'), todo_count);
        if conv_tail.starts_with('{') {
            Some(format!(
                "{}{}while ({}) {} // [cap-migrate] for-step: {}",
                init_prefix, indent, cond, conv_tail, step
            ))
        } else {
            Some(format!(
                "{}{}while ({}) {{ {}; {}; }}",
                init_prefix, indent, cond, conv_tail, step
            ))
        }
    }
}

/// Wrap single-line braceless `if (cond) stmt` / `else if (cond) stmt` / `else stmt` into `{ stmt; }`.
fn wrap_single_line_control_flow(line: &str) -> String {
    let (code, comment) = split_inline_comment(line);
    let trimmed = code.trim();
    let indent_len = code.len() - code.trim_start().len();
    let indent = &code[..indent_len];

    let wrap_tail = |head: &str, tail: &str| -> Option<String> {
        let t = tail.trim();
        if t.is_empty()
            || t.starts_with('{')
            || t.starts_with("if ")
            || t.starts_with("if(")
            // Do NOT wrap CP ternary-style `if (cond) a else b` expressions!
            || t.contains(" else ")
        {
            return None;
        }
        let body = if t.ends_with(';') || t.ends_with('}') {
            t.to_string()
        } else {
            format!("{};", t)
        };
        if comment.is_empty() {
            Some(format!("{}{} {{ {} }}", indent, head, body))
        } else {
            Some(format!("{}{} {{ {} }} {}", indent, head, body, comment))
        }
    };

    if trimmed.starts_with("if ") || trimmed.starts_with("if(") {
        if let Some(lp) = trimmed.find('(') {
            if let Some(rp) = find_matching_delim(trimmed, lp, b'(', b')') {
                if let Some(res) = wrap_tail(&trimmed[..=rp], &trimmed[rp + 1..]) {
                    return res;
                }
            }
        }
    } else if let Some(after_else) = trimmed.strip_prefix("else ") {
        let ae = after_else.trim_start();
        if ae.starts_with("if ") || ae.starts_with("if(") {
            if let Some(lp) = trimmed.find('(') {
                if let Some(rp) = find_matching_delim(trimmed, lp, b'(', b')') {
                    if let Some(res) = wrap_tail(&trimmed[..=rp], &trimmed[rp + 1..]) {
                        return res;
                    }
                }
            }
        } else if let Some(res) = wrap_tail("else", ae) {
            return res;
        }
    } else if let Some(after_else) = trimmed.strip_prefix("} else ") {
        let ae = after_else.trim_start();
        if ae.starts_with("if ") || ae.starts_with("if(") {
            if let Some(lp) = trimmed.find('(') {
                if let Some(rp) = find_matching_delim(trimmed, lp, b'(', b')') {
                    if let Some(res) = wrap_tail(&trimmed[..=rp], &trimmed[rp + 1..]) {
                        return res;
                    }
                }
            }
        } else if let Some(res) = wrap_tail("} else", ae) {
            return res;
        }
    }
    line.to_string()
}

/// Convert JS object literal method shorthand `foo(a, b) {` -> `foo: (a, b) => {`.
fn try_convert_object_method_shorthand(trimmed: &str, indent: &str, todo_count: &mut usize) -> Option<String> {
    let mut rest = trimmed;
    let is_async = if let Some(r) = rest.strip_prefix("async ") {
        rest = r.trim();
        true
    } else {
        false
    };
    let name_end = rest.find(|c: char| !c.is_ascii_alphanumeric() && c != '_')?;
    let mname = &rest[..name_end];
    if mname.is_empty()
        || matches!(
            mname,
            "if" | "for" | "while" | "switch" | "catch" | "function" | "return" | "const" | "let" | "var" | "else" | "fn" | "pub"
        )
    {
        return None;
    }
    let after_name = rest[name_end..].trim();
    if !after_name.starts_with('(') {
        return None;
    }
    let rp = find_matching_delim(after_name, 0, b'(', b')')?;
    let params_raw = &after_name[1..rp];
    let after_params = after_name[rp + 1..].trim();
    let lbrace = after_params.find('{')?;
    let body_tail = &after_params[lbrace..];
    let clean_params: Vec<String> = split_top_level(params_raw, ',')
        .into_iter()
        .filter_map(|p| {
            let mut pt = p.trim();
            if pt.is_empty() {
                return None;
            }
            if let Some(r) = pt.strip_prefix("...") {
                pt = r.trim();
            }
            if pt.starts_with('{') || pt.starts_with('[') {
                return Some("_p".to_string());
            }
            let head = split_top_level(pt, '=').into_iter().next().unwrap_or_default();
            let name = head.split(':').next().unwrap_or("").trim().trim_end_matches('?').trim();
            if name.is_empty() {
                None
            } else {
                Some(name.to_string())
            }
        })
        .collect();
    let async_kw = if is_async { "async " } else { "" };
    Some(format!(
        "{}{}: {}({}) => {}",
        indent,
        mname,
        async_kw,
        clean_params.join(", "),
        convert_ts_expr(body_tail, todo_count)
    ))
}

/// Peek the next non-empty, non-comment trimmed line starting at `idx`.
fn peek_next_code_line<'a>(lines: &'a [&'a str], mut idx: usize) -> &'a str {
    while idx < lines.len() {
        let t = lines[idx].trim();
        if !t.is_empty() && !t.starts_with("//") && !t.starts_with("/*") && !t.starts_with('*') {
            return t;
        }
        idx += 1;
    }
    ""
}

/// Ensure simple statement lines end with `;` when needed by CP parser,
/// avoiding semicolons inside parens/brackets, object literals, or multi-line expressions.
fn maybe_append_semicolon(
    line: &str,
    paren_depth: i32,
    bracket_depth: i32,
    next_code_line: &str,
) -> String {
    if paren_depth > 0 || bracket_depth > 0 {
        return line.to_string();
    }
    let (code, comment) = split_inline_comment(line);
    let trimmed = code.trim_end();
    if trimmed.is_empty() {
        return line.to_string();
    }
    let last = trimmed.as_bytes().last().copied().unwrap_or(0);
    if matches!(
        last,
        b';' | b'{' | b'}' | b',' | b':' | b'(' | b'[' | b'\\' | b'+' | b'-' | b'*' | b'/'
            | b'?' | b'&' | b'|' | b'=' | b'<' | b'>' | b'.'
    ) {
        return line.to_string();
    }
    // Continuation on next line -> do not terminate statement here
    if next_code_line.starts_with('.')
        || next_code_line.starts_with("?.")
        || next_code_line.starts_with('?')
        || next_code_line.starts_with(':')
        || next_code_line.starts_with("&&")
        || next_code_line.starts_with("||")
        || next_code_line.starts_with("??")
        || next_code_line.starts_with('+')
        || next_code_line.starts_with('-')
        || next_code_line.starts_with('*')
        || next_code_line.starts_with('/')
        || next_code_line.starts_with(')')
        || next_code_line.starts_with(']')
        || next_code_line.starts_with(',')
    {
        return line.to_string();
    }
    let t = trimmed.trim_start();
    if t.starts_with("if ")
        || t.starts_with("if(")
        || t.starts_with("else")
        || t.ends_with(" else")
        || t == "} else"
        || t.starts_with("for ")
        || t.starts_with("for(")
        || t.starts_with("while ")
        || t.starts_with("while(")
        || t.starts_with("switch ")
        || t.starts_with("case ")
        || t.starts_with("default:")
        || t.starts_with("pub ")
        || t.starts_with("fn ")
        || t.starts_with("async ")
        || t.starts_with("struct ")
        || t.starts_with("class ")
        || t.starts_with("enum ")
        || t.starts_with("//")
        || t.starts_with("/*")
        || t.starts_with('*')
        || t.starts_with("...")
    {
        return line.to_string();
    }
    // Object literal trailing property (`key: value` or bare shorthand ident `foo`) right before `}`
    if next_code_line.starts_with('}') {
        let is_stmt_kw = t.starts_with("return")
            || t.starts_with("let ")
            || t.starts_with("const ")
            || t.starts_with("break")
            || t.starts_with("continue")
            || t.starts_with("defer ");
        if !is_stmt_kw {
            // Bare identifier shorthand or `key: val` inside object literal
            let is_bare_ident = t.chars().all(|c| c.is_ascii_alphanumeric() || c == '_');
            let is_kv = t.find(':').map_or(false, |colon| {
                let k = t[..colon].trim();
                !k.is_empty()
                    && (k.chars().all(|c| c.is_ascii_alphanumeric() || c == '_')
                        || (k.starts_with('"') && k.ends_with('"')))
            });
            if is_bare_ident || is_kv {
                return line.to_string();
            }
        }
    }
    if comment.is_empty() {
        format!("{};", trimmed)
    } else {
        format!("{}; {}", trimmed, comment)
    }
}

struct SwitchState {
    depth: i32,
    pending_cases: Vec<String>,
    in_arm: bool,
}

// ============================================================================
// Main Entry Point
// ============================================================================

pub fn transpile_ts_to_cp(
    source: &str,
    rel_path: &str,
    dep_map: &HashMap<String, DepTier>,
) -> TranspileResult {
    let mut out = String::with_capacity(source.len() + 512);
    let mut todo_count: usize = 0;
    let mut opt_notes: Vec<String> = Vec::new();

    let mut saw_interface = false;
    let mut saw_var = false;
    let mut saw_try_catch = false;
    let mut saw_complex_type = false;
    let mut saw_arrow_to_fn = false;
    let mut saw_mut_self = false;

    out.push_str(&format!(
        "// Migrated from TypeScript ({}) via cap from -ts\n\n",
        rel_path
    ));

    let lines: Vec<&str> = source.lines().collect();
    let total = lines.len();
    let mut i = 0;

    let mut brace_depth: i32 = 0;
    let mut paren_depth: i32 = 0;
    let mut bracket_depth: i32 = 0;
    let mut in_block_comment = false;
    let mut in_backtick_template = false;
    let mut in_interface = false;
    let mut interface_depth: i32 = 0;
    let mut in_class = false;
    let mut class_depth: i32 = 0;
    let mut current_class_name = String::new();
    let mut skip_brace_depth: i32 = 0;
    let mut namespace_depths: Vec<(i32, String)> = Vec::new();
    let mut switch_stack: Vec<SwitchState> = Vec::new();

    while i < total {
        let raw_line = lines[i];
        let indent_len = raw_line.len() - raw_line.trim_start().len();
        let indent = &raw_line[..indent_len];
        let trimmed = raw_line.trim();

        // 0. Inside multi-line backtick template string: preserve verbatim
        if in_backtick_template {
            let conv = convert_template_interpolations(raw_line, &mut todo_count);
            out.push_str(&conv);
            out.push('\n');
            in_backtick_template = count_backticks(raw_line, true);
            i += 1;
            continue;
        }

        // 1. Block comments `/* ... */`
        if in_block_comment {
            out.push_str(raw_line);
            out.push('\n');
            if trimmed.contains("*/") {
                in_block_comment = false;
            }
            i += 1;
            continue;
        }
        if trimmed.starts_with("/*") {
            out.push_str(raw_line);
            out.push('\n');
            if !trimmed.contains("*/") {
                in_block_comment = true;
            }
            i += 1;
            continue;
        }

        // 2. Skip commented-out TS `declare` / dynamic blocks safely
        if skip_brace_depth > 0 {
            let (o, c) = count_braces(trimmed);
            skip_brace_depth += o - c;
            out.push_str(&format!("{}// [ts-skip] {}\n", indent, trimmed));
            if skip_brace_depth <= 0 {
                skip_brace_depth = 0;
            }
            i += 1;
            continue;
        }

        // 3. Blank lines and line comments
        if trimmed.is_empty() {
            out.push('\n');
            i += 1;
            continue;
        }
        if trimmed.starts_with("//") {
            out.push_str(raw_line);
            out.push('\n');
            i += 1;
            continue;
        }

        // 3.5 Unwrap `export namespace Foo {` / `namespace Foo {` and its closing `}`
        if brace_depth == 0
            && (trimmed.starts_with("export namespace ") || trimmed.starts_with("namespace "))
        {
            let rest = trimmed
                .trim_start_matches("export ")
                .trim_start_matches("namespace ")
                .trim();
            let ns_name = rest
                .split(|c: char| !c.is_ascii_alphanumeric() && c != '_')
                .next()
                .unwrap_or("ns")
                .to_string();
            namespace_depths.push((brace_depth, ns_name.clone()));
            out.push_str(&format!("{}// [cap-migrate] namespace {} {{\n", indent, ns_name));
            i += 1;
            continue;
        }
        if trimmed == "}" && !in_interface && !in_class {
            if let Some(&(ns_depth, _)) = namespace_depths.last() {
                if brace_depth == ns_depth {
                    let (_, ns_name) = namespace_depths.pop().unwrap();
                    out.push_str(&format!("{}// [cap-migrate] }} // end namespace {}\n", indent, ns_name));
                    i += 1;
                    continue;
                }
            }
        }

        // 4. Multi-line `import ... from "..."` and `export { ... } from "..."` / `export * from "..."`
        if brace_depth == 0
            && (trimmed.starts_with("import ")
                || trimmed.starts_with("export * ")
                || trimmed.starts_with("export {")
                || trimmed.starts_with("export type {"))
        {
            let mut combined = trimmed.to_string();
            let mut j = i;
            let needs_from = combined.starts_with("import ")
                || combined.starts_with("export * ")
                || combined.contains('{');
            if needs_from
                && !combined.contains(" from ")
                && !combined.starts_with("import \"")
                && !combined.starts_with("import '")
            {
                while j + 1 < total {
                    j += 1;
                    let next_t = lines[j].trim();
                    combined.push(' ');
                    combined.push_str(next_t);
                    if next_t.contains(" from ")
                        || next_t.ends_with(';')
                        || next_t.ends_with('"')
                        || next_t.ends_with('\'')
                    {
                        break;
                    }
                }
            }
            let converted = convert_import_export(&combined, dep_map);
            out.push_str(indent);
            out.push_str(&converted);
            out.push('\n');
            i = j + 1;
            continue;
        }

        // 5. `declare ...` TS ambient declarations -> comment out safely (including multi-line headers)
        if brace_depth == 0
            && (trimmed.starts_with("declare ") || trimmed.starts_with("export declare "))
        {
            let (mut bo, mut bc) = count_braces(trimmed);
            let (mut pd, mut bd) = count_parens_brackets(trimmed);
            let mut angle = trimmed.bytes().filter(|&b| b == b'<').count() as i32
                - trimmed.bytes().filter(|&b| b == b'>').count() as i32;
            out.push_str(&format!("{}// [ts-declare] {}\n", indent, trimmed));
            let mut j = i;
            while (pd > 0 || bd > 0 || angle > 0) && bo <= bc && j + 1 < total {
                j += 1;
                let nt = lines[j].trim();
                let (dp, db) = count_parens_brackets(nt);
                let (do_, dc_) = count_braces(nt);
                pd += dp;
                bd += db;
                bo += do_;
                bc += dc_;
                angle += nt.bytes().filter(|&b| b == b'<').count() as i32
                    - nt.bytes().filter(|&b| b == b'>').count() as i32;
                out.push_str(&format!("{}// [ts-declare] {}\n", indent, nt));
            }
            if bo > bc {
                skip_brace_depth = bo - bc;
            }
            i = j + 1;
            continue;
        }

        // 6. `export interface Foo` / `interface Foo` -> `pub struct Foo`
        if brace_depth == 0
            && (trimmed.starts_with("export interface ") || trimmed.starts_with("interface "))
        {
            saw_interface = true;
            let is_pub = trimmed.starts_with("export ");
            let mut header = trimmed.to_string();
            let mut j = i;
            while !has_header_body_lbrace(&header) && j + 1 < total {
                j += 1;
                header.push(' ');
                header.push_str(lines[j].trim());
            }
            let rest = header
                .trim_start_matches("export ")
                .trim_start_matches("interface ")
                .trim();
            let name_end = rest
                .find(|c: char| !c.is_ascii_alphanumeric() && c != '_')
                .unwrap_or(rest.len());
            let name = &rest[..name_end];
            let after_name = rest[name_end..].trim();
            let (generics, after_gen) = if after_name.starts_with('<') {
                if let Some(gt) = find_matching_delim(after_name, 0, b'<', b'>') {
                    (
                        simplify_ts_generics(&after_name[..=gt]),
                        after_name[gt + 1..].trim(),
                    )
                } else {
                    (String::new(), after_name)
                }
            } else {
                (String::new(), after_name)
            };
            let pub_kw = if is_pub { "pub " } else { "" };
            let (o, c) = count_braces(after_gen);
            if o > c {
                in_interface = true;
                interface_depth = brace_depth + (o - c);
                brace_depth += o - c;
                out.push_str(&format!(
                    "{}{}struct {}{} {{\n",
                    indent, pub_kw, name, generics
                ));
            } else {
                out.push_str(&format!(
                    "{}{}struct {}{} {{}}\n",
                    indent, pub_kw, name, generics
                ));
            }
            i = j + 1;
            continue;
        }

        // 7. Inside interface (now CP struct)
        if in_interface && brace_depth == interface_depth {
            if trimmed == "}" || trimmed == "};" {
                in_interface = false;
                brace_depth -= 1;
                out.push_str(&format!("{}}}\n", indent));
                i += 1;
                continue;
            }
            // Collect multi-line field signature (e.g. `error?: (\n  this: Server,\n) => Response`)
            let mut field_src = trimmed.to_string();
            let mut j = i;
            let (mut pd, mut bd) = count_parens_brackets(&field_src);
            let (mut bo, mut bc) = count_braces(&field_src);
            if bo > bc && pd == 0 && bd == 0 {
                // Nested inline object type inside interface -> degrade to string field and skip inner lines
                let mut raw_fn = trimmed
                    .split(':')
                    .next()
                    .unwrap_or("field")
                    .trim();
                for kw in &["public ", "private ", "protected ", "readonly "] {
                    while let Some(r) = raw_fn.strip_prefix(kw) {
                        raw_fn = r.trim();
                    }
                }
                let field_name = raw_fn
                    .trim_end_matches('?')
                    .trim_matches('"')
                    .trim_matches('\'')
                    .trim();
                todo_count += 1;
                skip_brace_depth = bo - bc;
                if !field_name.is_empty()
                    && field_name
                        .chars()
                        .all(|c| c.is_ascii_alphanumeric() || c == '_')
                {
                    out.push_str(&format!(
                        "{}{}: string = \"\"; // TODO(cap-migrate): flatten nested inline type\n",
                        indent, field_name
                    ));
                } else {
                    out.push_str(&format!(
                        "{}// [ts-iface] {} (flattened nested inline type)\n",
                        indent, trimmed
                    ));
                }
                i += 1;
                continue;
            }
            while (pd > 0 || bd > 0 || bo > bc) && j + 1 < total {
                j += 1;
                let nt = lines[j].trim();
                field_src.push(' ');
                field_src.push_str(nt);
                let (dp, db) = count_parens_brackets(nt);
                let (do_, dc_) = count_braces(nt);
                pd += dp;
                bd += db;
                bo += do_;
                bc += dc_;
            }
            if let Some(field_line) = convert_field_line(&field_src, indent, true, &mut todo_count) {
                out.push_str(&field_line);
                out.push('\n');
            } else {
                out.push_str(&format!("{}// [ts-iface] {}\n", indent, field_src));
            }
            i = j + 1;
            continue;
        }

        // 8. `export type Foo = ...` / `type Foo = ...`
        if brace_depth == 0
            && (trimmed.starts_with("export type ") || trimmed.starts_with("type "))
        {
            let is_pub = trimmed.starts_with("export ");
            let mut full = trimmed.to_string();
            let mut j = i;
            let mut b_brace = 0i32;
            let mut b_paren = 0i32;
            let mut b_angle = 0i32;
            let mut b_brack = 0i32;
            let update_depths =
                |s: &str, br: &mut i32, pa: &mut i32, an: &mut i32, bk: &mut i32| {
                    let (code, _) = split_inline_comment(s);
                    let mut prev = 0u8;
                    let mut in_d = false;
                    let mut in_s = false;
                    let mut in_bt = false;
                    for &b in code.as_bytes() {
                        if in_d {
                            if b == b'"' && prev != b'\\' {
                                in_d = false;
                            }
                            prev = b;
                            continue;
                        }
                        if in_s {
                            if b == b'\'' && prev != b'\\' {
                                in_s = false;
                            }
                            prev = b;
                            continue;
                        }
                        if in_bt {
                            if b == b'`' && prev != b'\\' {
                                in_bt = false;
                            }
                            prev = b;
                            continue;
                        }
                        match b {
                            b'"' => in_d = true,
                            b'\'' => in_s = true,
                            b'`' => in_bt = true,
                            b'{' => *br += 1,
                            b'}' => *br -= 1,
                            b'(' => *pa += 1,
                            b')' => *pa -= 1,
                            b'<' => *an += 1,
                            b'>' if prev != b'=' => {
                                if *an > 0 {
                                    *an -= 1;
                                }
                            }
                            b'[' => *bk += 1,
                            b']' => *bk -= 1,
                            _ => {}
                        }
                        prev = b;
                    }
                };
            update_depths(trimmed, &mut b_brace, &mut b_paren, &mut b_angle, &mut b_brack);
            while j + 1 < total {
                let next_t = lines[j + 1].trim();
                let full_t = full.trim_end();
                let continues = b_brace > 0
                    || b_paren > 0
                    || b_angle > 0
                    || b_brack > 0
                    || !full.contains('=')
                    || full_t.ends_with('=')
                    || full_t.ends_with('|')
                    || full_t.ends_with('&')
                    || full_t.ends_with('?')
                    || full_t.ends_with(':')
                    || full_t.ends_with("extends")
                    || next_t.starts_with('|')
                    || next_t.starts_with('&')
                    || next_t.starts_with('?')
                    || next_t.starts_with(':')
                    || next_t.starts_with("extends ");
                if !continues {
                    break;
                }
                j += 1;
                full.push(' ');
                full.push_str(next_t);
                update_depths(next_t, &mut b_brace, &mut b_paren, &mut b_angle, &mut b_brack);
            }

            let rest = full
                .trim_start_matches("export ")
                .trim_start_matches("type ")
                .trim();
            let name_end = rest
                .find(|c: char| !c.is_ascii_alphanumeric() && c != '_')
                .unwrap_or(rest.len());
            let tname = &rest[..name_end];
            let after_name = rest[name_end..].trim();
            let (generics, after_gen) = if after_name.starts_with('<') {
                if let Some(gt) = find_matching_delim(after_name, 0, b'<', b'>') {
                    (
                        simplify_ts_generics(&after_name[..=gt]),
                        after_name[gt + 1..].trim(),
                    )
                } else {
                    (String::new(), after_name)
                }
            } else {
                (String::new(), after_name)
            };

            let rhs = after_gen
                .find('=')
                .map(|p| after_gen[p + 1..].trim().trim_end_matches(';').trim())
                .unwrap_or("string");
            let pub_kw = if is_pub { "pub " } else { "" };
            let gen_note = if generics.is_empty() {
                String::new()
            } else {
                format!(" [generics: {}]", generics)
            };

            // Note: CP `type Name = Target;` does not accept generic params `<T>` on type aliases.
            if !generics.is_empty() || is_complex_ts_type(rhs) || split_top_level(rhs, '|').len() > 2 {
                saw_complex_type = true;
                todo_count += 1;
                out.push_str(&format!(
                    "{}{}type {} = string; // TODO(cap-migrate): complex TS conditional/mapped type{}\n",
                    indent, pub_kw, tname, gen_note
                ));
            } else {
                let mapped = map_ts_type(rhs, &mut todo_count);
                let clean = mapped.split(" /*").next().unwrap_or(&mapped).trim();
                let clean = if clean == "void" || clean.is_empty() {
                    "string"
                } else {
                    clean
                };
                out.push_str(&format!(
                    "{}{}type {} = {};\n",
                    indent, pub_kw, tname, clean
                ));
            }
            i = j + 1;
            continue;
        }

        // 9. `export enum E` / `const enum E` / `enum E`
        if brace_depth == 0
            && (trimmed.starts_with("export enum ")
                || trimmed.starts_with("export const enum ")
                || trimmed.starts_with("const enum ")
                || trimmed.starts_with("enum "))
        {
            let is_pub = trimmed.starts_with("export ");
            let rest = trimmed
                .trim_start_matches("export ")
                .trim_start_matches("const ")
                .trim_start_matches("enum ")
                .trim();
            let pub_kw = if is_pub { "pub " } else { "" };
            let conv = convert_single_quoted_strings(rest);
            let (o, c) = count_braces(&conv);
            brace_depth += o - c;
            out.push_str(&format!("{}{}enum {}\n", indent, pub_kw, conv));
            i += 1;
            continue;
        }

        // 10. `export class C` / `export abstract class C` / `class C`
        if brace_depth == 0
            && (trimmed.starts_with("export class ")
                || trimmed.starts_with("export default class ")
                || trimmed.starts_with("export abstract class ")
                || trimmed.starts_with("abstract class ")
                || trimmed.starts_with("class "))
        {
            let is_pub = trimmed.starts_with("export ");
            let mut header = trimmed.to_string();
            let mut j = i;
            while !has_header_body_lbrace(&header) && j + 1 < total {
                j += 1;
                header.push(' ');
                header.push_str(lines[j].trim());
            }
            let rest = header
                .trim_start_matches("export ")
                .trim_start_matches("default ")
                .trim_start_matches("abstract ")
                .trim_start_matches("class ")
                .trim();
            let name_end = rest
                .find(|c: char| !c.is_ascii_alphanumeric() && c != '_')
                .unwrap_or(rest.len());
            let cname = &rest[..name_end];
            let after_name = rest[name_end..].trim();
            let (generics, after_gen) = if after_name.starts_with('<') {
                if let Some(gt) = find_matching_delim(after_name, 0, b'<', b'>') {
                    (
                        simplify_ts_generics(&after_name[..=gt]),
                        after_name[gt + 1..].trim(),
                    )
                } else {
                    (String::new(), after_name)
                }
            } else {
                (String::new(), after_name)
            };

            // Extract optional `extends Base` (single identifier only for CP parser)
            let extends_clause = if let Some(ext_pos) = after_gen.find("extends ") {
                let ext_rest = after_gen[ext_pos + 8..].trim();
                let base_end = ext_rest
                    .find(|c: char| !c.is_ascii_alphanumeric() && c != '_')
                    .unwrap_or(ext_rest.len());
                let base = &ext_rest[..base_end];
                if !base.is_empty() && base != "Error" {
                    format!(" extends {}", base)
                } else {
                    String::new()
                }
            } else {
                String::new()
            };

            let pub_kw = if is_pub { "pub " } else { "" };
            let (o, c) = count_braces(after_gen);
            if o > c {
                in_class = true;
                class_depth = brace_depth + (o - c);
                brace_depth += o - c;
                current_class_name = cname.to_string();
                out.push_str(&format!(
                    "{}{}class {}{}{} {{\n",
                    indent, pub_kw, cname, generics, extends_clause
                ));
            } else {
                out.push_str(&format!(
                    "{}{}class {}{}{} {{}}\n",
                    indent, pub_kw, cname, generics, extends_clause
                ));
            }
            i = j + 1;
            continue;
        }

        // 11. Direct members of `class C` (fields, constructor, methods, getters/setters)
        if in_class && brace_depth == class_depth {
            if trimmed == "}" || trimmed == "};" {
                in_class = false;
                current_class_name.clear();
                brace_depth -= 1;
                out.push_str(&format!("{}}}\n", indent));
                i += 1;
                continue;
            }

            // Collect multi-line constructor/method signature if `(` is opened without `)` on this line
            let mut sig = trimmed.to_string();
            let mut j = i;
            if sig.contains('(') && !sig.contains('{') && !sig.ends_with(';') {
                while j + 1 < total && !sig.contains('{') && !sig.ends_with(';') {
                    j += 1;
                    sig.push(' ');
                    sig.push_str(lines[j].trim());
                }
            }

            let mutates = method_body_mutates_self(&lines, j);
            if let Some((converted_member, ctor_fields)) = try_convert_class_member(
                &sig,
                indent,
                &current_class_name,
                mutates,
                &mut todo_count,
            ) {
                if mutates {
                    saw_mut_self = true;
                }
                for (fname, fty) in ctor_fields {
                    let def = default_value_for_cp_type(&fty);
                    if !def.is_empty() {
                        out.push_str(&format!("{}{}: {} = {};\n", indent, fname, fty, def));
                    } else {
                        out.push_str(&format!("{}{}: {};\n", indent, fname, fty));
                    }
                }
                let (o, c) = count_braces(&converted_member);
                brace_depth += o - c;
                out.push_str(&converted_member);
                out.push('\n');
                i = j + 1;
                continue;
            }

            // Otherwise treat as class field
            if let Some(field_line) = convert_field_line(&sig, indent, false, &mut todo_count) {
                let (o, c) = count_braces(&field_line);
                brace_depth += o - c;
                out.push_str(&field_line);
                out.push('\n');
                i = j + 1;
                continue;
            }
        }

        // 12. Top-level or nested `function` declarations
        if trimmed.starts_with("export function ")
            || trimmed.starts_with("export async function ")
            || trimmed.starts_with("export default function ")
            || trimmed.starts_with("export default async function ")
            || trimmed.starts_with("async function ")
            || trimmed.starts_with("function ")
        {
            let mut sig = trimmed.to_string();
            let mut j = i;
            let (mut pd, _) = count_parens_brackets(&sig);
            let has_body_brace = |s: &str| -> bool {
                if let Some(lp) = s.find('(') {
                    if let Some(rp) = find_matching_delim(s, lp, b'(', b')') {
                        return s[rp + 1..].contains('{');
                    }
                }
                false
            };
            while (pd > 0 || (!has_body_brace(&sig) && !sig.ends_with(';'))) && j + 1 < total {
                j += 1;
                let nt = lines[j].trim();
                sig.push(' ');
                sig.push_str(nt);
                let (dp, _) = count_parens_brackets(nt);
                pd += dp;
            }
            if let Some(conv_fn) = try_convert_function_decl(&sig, indent, &mut todo_count) {
                let (o, c) = count_braces(&conv_fn);
                brace_depth += o - c;
                out.push_str(&conv_fn);
                out.push('\n');
                i = j + 1;
                continue;
            }
        }

        // 13. Top-level arrow functions (single-line or multi-line params/expr body)
        if brace_depth == 0
            && (trimmed.starts_with("export const ")
                || trimmed.starts_with("const ")
                || trimmed.starts_with("export let ")
                || trimmed.starts_with("let "))
            && trimmed.contains('=')
        {
            let mut candidate = trimmed.to_string();
            let mut j = i;
            let (mut pd, mut bd) = count_parens_brackets(&candidate);
            let mut lookahead_limit = 0;
            // Collect multi-line parameter list until `=>` is found (bounded lookahead)
            while !candidate.contains("=>") && (pd > 0 || bd > 0 || candidate.trim_end().ends_with('=')) && j + 1 < total && lookahead_limit < 25 {
                j += 1;
                lookahead_limit += 1;
                let nt = lines[j].trim();
                candidate.push(' ');
                candidate.push_str(nt);
                let (dp, db) = count_parens_brackets(nt);
                pd += dp;
                bd += db;
            }
            // If `=>` is at the end of line, pull the next line (either `{` or expression)
            if candidate.trim_end().ends_with("=>") && j + 1 < total {
                j += 1;
                let nt = lines[j].trim();
                candidate.push(' ');
                candidate.push_str(nt);
                let (dp, db) = count_parens_brackets(nt);
                pd += dp;
                bd += db;
            }
            // If expression body (not starting with `{` after `=>`) spans multiple lines, collect it fully
            if let Some(arr) = candidate.find("=>") {
                let body_start = candidate[arr + 2..].trim_start();
                if !body_start.starts_with('{') {
                    let mut expr_lines = 0;
                    while j + 1 < total && expr_lines < 20 {
                        let ct = candidate.trim_end();
                        let nt = lines[j + 1].trim();
                        let continues = pd > 0
                            || bd > 0
                            || ct.ends_with("&&")
                            || ct.ends_with("||")
                            || ct.ends_with("??")
                            || ct.ends_with('+')
                            || ct.ends_with('-')
                            || ct.ends_with('?')
                            || ct.ends_with(':')
                            || nt.starts_with("&&")
                            || nt.starts_with("||")
                            || nt.starts_with("??")
                            || nt.starts_with("? ")
                            || nt.starts_with(": ");
                        if !continues {
                            break;
                        }
                        j += 1;
                        expr_lines += 1;
                        candidate.push(' ');
                        candidate.push_str(nt);
                        let (dp, db) = count_parens_brackets(nt);
                        pd += dp;
                        bd += db;
                    }
                }
            }
            if candidate.contains("=>") {
                if let Some(conv_arrow) =
                    try_convert_top_level_arrow_fn(&candidate, indent, &mut todo_count)
                {
                    saw_arrow_to_fn = true;
                    let (o, c) = count_braces(&conv_arrow);
                    brace_depth += o - c;
                    let (dp, db) = count_parens_brackets(&conv_arrow);
                    paren_depth = (paren_depth + dp).max(0);
                    bracket_depth = (bracket_depth + db).max(0);
                    out.push_str(&conv_arrow);
                    out.push('\n');
                    i = j + 1;
                    continue;
                }
            }
        }

        // 14. Top-level `export const` / `export let` -> `pub const` / `pub let`
        if brace_depth == 0 && (trimmed.starts_with("export const ") || trimmed.starts_with("export let ")) {
            // Join multi-line ternary if next line starts with `?` or `:`
            let mut joined = trimmed.strip_prefix("export ").unwrap().to_string();
            let mut j = i;
            while j + 1 < total {
                let nt = lines[j + 1].trim();
                if nt.starts_with("? ") || nt.starts_with(": ") || joined.trim_end().ends_with('=') {
                    j += 1;
                    joined.push(' ');
                    joined.push_str(nt);
                } else {
                    break;
                }
            }
            let conv = convert_ts_expr(&joined, &mut todo_count);
            let (dp, db) = count_parens_brackets(&conv);
            paren_depth = (paren_depth + dp).max(0);
            bracket_depth = (bracket_depth + db).max(0);
            let next_code = peek_next_code_line(&lines, j + 1);
            let line = maybe_append_semicolon(
                &format!("{}pub {}", indent, conv),
                paren_depth,
                bracket_depth,
                next_code,
            );
            let (o, c) = count_braces(&line);
            brace_depth += o - c;
            in_backtick_template = count_backticks(&conv, in_backtick_template);
            out.push_str(&line);
            out.push('\n');
            i = j + 1;
            continue;
        }

        // 15. `try {` / `} catch (e) {` / `} finally {`
        if let Some(tc_line) = try_convert_try_catch(trimmed, indent, &mut todo_count) {
            saw_try_catch = true;
            let (o, c) = count_braces(&tc_line);
            brace_depth += o - c;
            out.push_str(&tc_line);
            out.push('\n');
            i += 1;
            continue;
        }

        // 15.5 C-style `for (init; cond; step)` -> `init; while (cond)`
        if trimmed.starts_with("for ") || trimmed.starts_with("for(") {
            if let Some(for_line) = try_convert_c_for_loop(trimmed, indent, &mut todo_count) {
                let (o, c) = count_braces(&for_line);
                brace_depth += o - c;
                out.push_str(&for_line);
                out.push('\n');
                i += 1;
                continue;
            }
        }

        // 15.6 `switch (expr) {` and `case X:` / `default:` arms -> CP switch syntax
        if (trimmed.starts_with("switch ") || trimmed.starts_with("switch(")) && trimmed.ends_with('{') {
            let conv = convert_ts_expr(trimmed, &mut todo_count);
            let (o, c) = count_braces(&conv);
            brace_depth += o - c;
            switch_stack.push(SwitchState {
                depth: brace_depth,
                pending_cases: Vec::new(),
                in_arm: false,
            });
            out.push_str(&format!("{}{}\n", indent, conv));
            i += 1;
            continue;
        }
        if let Some(sw) = switch_stack.last_mut() {
            if brace_depth == sw.depth {
                if trimmed == "}" {
                    if sw.in_arm {
                        out.push_str(&format!("{}    }}\n", indent));
                    } else if !sw.pending_cases.is_empty() {
                        let pats = sw.pending_cases.join(", ");
                        out.push_str(&format!("{}    {} => {{}}\n", indent, pats));
                    }
                    switch_stack.pop();
                    brace_depth -= 1;
                    out.push_str(&format!("{}}}\n", indent));
                    i += 1;
                    continue;
                }
                if let Some(rest) = trimmed.strip_prefix("case ") {
                    if sw.in_arm {
                        out.push_str(&format!("{}}},\n", indent));
                        sw.in_arm = false;
                    }
                    if let Some(colon) = rest.find(':') {
                        let pat = convert_ts_expr(rest[..colon].trim(), &mut todo_count);
                        let after = rest[colon + 1..].trim();
                        sw.pending_cases.push(pat);
                        if !after.is_empty() {
                            let pats = sw.pending_cases.drain(..).collect::<Vec<_>>().join(", ");
                            let stmt = convert_ts_expr(after.trim_end_matches(';'), &mut todo_count);
                            out.push_str(&format!("{}{} => {{ {}; }},\n", indent, pats, stmt));
                        }
                    }
                    i += 1;
                    continue;
                }
                if let Some(rest) = trimmed.strip_prefix("default:") {
                    if sw.in_arm {
                        out.push_str(&format!("{}}},\n", indent));
                        sw.in_arm = false;
                    }
                    sw.pending_cases.clear();
                    let after = rest.trim();
                    if after.is_empty() {
                        out.push_str(&format!("{}else => {{\n", indent));
                        sw.in_arm = true;
                    } else {
                        let stmt = convert_ts_expr(after.trim_end_matches(';'), &mut todo_count);
                        out.push_str(&format!("{}else => {{ {}; }},\n", indent, stmt));
                    }
                    i += 1;
                    continue;
                }
                if (trimmed == "break" || trimmed == "break;") && sw.in_arm {
                    out.push_str(&format!("{}// break;\n", indent));
                    i += 1;
                    continue;
                }
                if !sw.pending_cases.is_empty() {
                    let pats = sw.pending_cases.drain(..).collect::<Vec<_>>().join(", ");
                    out.push_str(&format!("{}{} => {{\n", indent, pats));
                    sw.in_arm = true;
                }
            }
        }

        // 16. `throw ...` (including multi-line `throw new Foo(\n ... \n)`)
        if trimmed.starts_with("throw ") || trimmed == "throw;" {
            todo_count += 1;
            let (mut pd, mut bd) = count_parens_brackets(trimmed);
            let (mut bo, mut bc) = count_braces(trimmed);
            let mut j = i;
            out.push_str(&format!(
                "{}return; // TODO(cap-migrate): throw -> error return ({})\n",
                indent, trimmed
            ));
            while (pd > 0 || bd > 0 || bo > bc) && j + 1 < total {
                j += 1;
                let nt = lines[j].trim();
                let (dp, db) = count_parens_brackets(nt);
                let (do_, dc_) = count_braces(nt);
                pd += dp;
                bd += db;
                bo += do_;
                bc += dc_;
                out.push_str(&format!("{}// [ts-throw] {}\n", indent, nt));
            }
            i = j + 1;
            continue;
        }

        // 17. Dynamic TS metaprogramming (`new Function(...)`, `eval(...)`, etc.)
        if is_dynamic_metaprogramming(trimmed) {
            todo_count += 1;
            let (o, c) = count_braces(trimmed);
            if o > c {
                skip_brace_depth = o - c;
            }
            out.push_str(&format!(
                "{}// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: {}\n",
                indent, trimmed
            ));
            i += 1;
            continue;
        }

        // 17.5 Object literal method shorthand `foo(a, b) {` -> `foo: (a, b) => {`
        if brace_depth > 0
            && !(in_class && brace_depth == class_depth)
            && !in_interface
            && trimmed.ends_with('{')
            && trimmed.contains('(')
        {
            if let Some(obj_method) = try_convert_object_method_shorthand(trimmed, indent, &mut todo_count) {
                let (o, c) = count_braces(&obj_method);
                brace_depth += o - c;
                out.push_str(&obj_method);
                out.push('\n');
                i += 1;
                continue;
            }
        }

        // 18. General statement / expression line
        if trimmed.starts_with("var ") || trimmed.contains(" var ") {
            saw_var = true;
        }
        // Join multi-line ternary (`? ` / `: `) or multi-line `as <...>` / `as { ... }`
        let mut stmt_src = trimmed.to_string();
        let mut j = i;
        while j + 1 < total {
            let nt = lines[j + 1].trim();
            if nt.starts_with("? ") || nt.starts_with(": ") {
                j += 1;
                stmt_src.push(' ');
                stmt_src.push_str(nt);
            } else {
                break;
            }
        }
        // If line contains ` as ` with unclosed `<` or `{`, collect until balanced so `strip_ts_as_and_satisfies` strips it cleanly
        if let Some(as_pos) = stmt_src.find(" as ") {
            let after_as = &stmt_src[as_pos + 4..];
            let (mut bo, mut bc) = count_braces(after_as);
            let mut ang = after_as.bytes().filter(|&b| b == b'<').count() as i32
                - after_as.bytes().filter(|&b| b == b'>').count() as i32;
            let mut as_lines = 0;
            while (bo > bc || ang > 0) && j + 1 < total && as_lines < 25 {
                j += 1;
                as_lines += 1;
                let nt = lines[j].trim();
                stmt_src.push(' ');
                stmt_src.push_str(nt);
                let (do_, dc_) = count_braces(nt);
                bo += do_;
                bc += dc_;
                ang += nt.bytes().filter(|&b| b == b'<').count() as i32
                    - nt.bytes().filter(|&b| b == b'>').count() as i32;
            }
        }

        // Convert JS multi-line comma variable declaration `const a = 1,\n b = 2` -> separate `const` / `let` statements
        if paren_depth == 0
            && bracket_depth == 0
            && (stmt_src.starts_with("const ") || stmt_src.starts_with("let ") || stmt_src.starts_with("var "))
            && stmt_src.ends_with(',')
            && count_braces(&stmt_src).0 == count_braces(&stmt_src).1
            && count_parens_brackets(&stmt_src) == (0, 0)
        {
            let decl_kw = if stmt_src.starts_with("const ") { "const" } else { "let" };
            let first = convert_ts_expr(stmt_src.trim_end_matches(','), &mut todo_count);
            out.push_str(&format!("{}{};\n", indent, first.trim_end_matches(';')));
            while j + 1 < total {
                let nt = lines[j + 1].trim();
                if nt.is_empty() || nt.starts_with("//") {
                    break;
                }
                let has_more = nt.ends_with(',');
                let clean_nt = nt.trim_end_matches(',').trim_end_matches(';');
                let conv_item = convert_ts_expr(clean_nt, &mut todo_count);
                out.push_str(&format!("{}{} {};\n", indent, decl_kw, conv_item));
                j += 1;
                if !has_more {
                    break;
                }
            }
            i = j + 1;
            continue;
        }

        let prev_paren_depth = paren_depth;
        let conv = convert_ts_expr(&stmt_src, &mut todo_count);
        let (dp, db) = count_parens_brackets(&conv);
        paren_depth = (paren_depth + dp).max(0);
        bracket_depth = (bracket_depth + db).max(0);
        let next_code = peek_next_code_line(&lines, j + 1);
        let with_cf = wrap_single_line_control_flow(&format!("{}{}", indent, conv));
        // Avoid appending `;` on closing `)` of a multi-line `if (...)` / `while (...)` / `for (...)` header
        let closes_multiline_ctrl = prev_paren_depth > 0
            && paren_depth == 0
            && conv.trim() == ")"
            && out.rsplit('\n').take(12).any(|prev_l| {
                let pt = prev_l.trim();
                pt.starts_with("if (")
                    || pt.starts_with("if(")
                    || pt.starts_with("else if (")
                    || pt.starts_with("while (")
                    || pt.starts_with("for (")
            });
        let final_line = if closes_multiline_ctrl {
            with_cf
        } else {
            maybe_append_semicolon(&with_cf, paren_depth, bracket_depth, next_code)
        };
        let (o, c) = count_braces(&final_line);
        brace_depth += o - c;
        if brace_depth < 0 {
            brace_depth = 0;
        }
        in_backtick_template = count_backticks(&conv, in_backtick_template);
        out.push_str(&final_line);
        out.push('\n');
        i = j + 1;
    }

    // Auto-close any unclosed braces (e.g. from skipped TS ambient/complex blocks) to avoid E0199
    while brace_depth > 0 {
        out.push_str("} // [cap-migrate] auto-closed brace\n");
        brace_depth -= 1;
    }

    // Record CP migration optimization notes
    if saw_interface {
        opt_notes.push("消除 interface 动态派发为栈布局 struct（零虚表开销）".to_string());
    }
    if saw_var {
        opt_notes.push("彻底消除废弃关键字 `var` 为块级可变绑定 `let`".to_string());
    }
    if saw_arrow_to_fn {
        opt_notes.push("顶层箭头函数提升为静态零闭包开销 `fn` 声明".to_string());
    }
    if saw_mut_self {
        opt_notes.push("自动推断字段写操作并为类方法注入安全可变接收者 `*_`".to_string());
    }
    if saw_try_catch {
        opt_notes.push("剥离 `try/catch` 异常块并标注切换至 CP 零成本 `?T` / `?` 错误传播".to_string());
    }
    if saw_complex_type {
        opt_notes.push("复杂 TS 条件/映射类型安全降级为显式 `TODO` 别名以保障 `cpc` 解析".to_string());
    }

    TranspileResult {
        cp_code: out,
        todo_count,
        opt_notes,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_interface_and_types() {
        let ts = r#"
export interface UserConfig {
    name: string;
    retries?: number;
    tags: string[];
    headers: Record<string, string>;
    payload: Uint8Array;
}

export type Complex<T> = T extends string ? number : never;
"#;
        let dep_map = HashMap::new();
        let res = transpile_ts_to_cp(ts, "src/config.ts", &dep_map);
        assert!(res.cp_code.contains("// Migrated from TypeScript (src/config.ts) via cap from -ts"));
        assert!(res.cp_code.contains("pub struct UserConfig {"));
        assert!(res.cp_code.contains("name: string = \"\";"));
        assert!(res.cp_code.contains("retries: f64? = null;"));
        assert!(res.cp_code.contains("tags: []string = [];"));
        assert!(res.cp_code.contains("headers: Map<string, string>;"));
        assert!(res.cp_code.contains("payload: []u8 = [];"));
        assert!(res.cp_code.contains("pub type Complex = string; // TODO(cap-migrate): complex TS conditional/mapped type"));
        assert!(res.todo_count >= 1);
        assert!(res.opt_notes.iter().any(|n| n.contains("struct")));
    }

    #[test]
    fn test_class_mut_self_and_statements() {
        let ts = r#"
export class Counter {
    count: number = 0;

    constructor(initial: number) {
        this.count = initial;
    }

    async increment(step: number): Promise<number> {
        var next = this.count + step;
        if (next === undefined) {
            throw new Error('invalid');
        }
        try {
            this.count = next;
        } catch (e) {
            console.error('failed');
        }
        return this.count;
    }

    getCount(): number {
        return this.count;
    }
}

export const makeCounter = (n: number): Counter => new Counter(n);
"#;
        let dep_map = HashMap::new();
        let res = transpile_ts_to_cp(ts, "src/counter.ts", &dep_map);
        assert!(res.cp_code.contains("pub class Counter {"));
        assert!(res.cp_code.contains("pub static fn init(initial: f64): Counter {"));
        assert!(res.cp_code.contains("pub async fn increment(*_, step: f64): f64 {"));
        assert!(res.cp_code.contains("pub fn getCount(): f64 {"));
        assert!(res.cp_code.contains("let next = _.count + step;"));
        assert!(res.cp_code.contains("if (next == null) {"));
        assert!(res.cp_code.contains("return; // TODO(cap-migrate): throw -> error return"));
        assert!(res.cp_code.contains("@print(\"failed\");"));
        assert!(res.cp_code.contains("pub fn makeCounter(n: f64): Counter { return Counter.init(n); }"));
    }
}
