// TypeScript Test Suite -> CP `cap:test` Lowering Pass
// Converts TS test files (`*.test.ts`, `*.spec.ts`, `__tests__/*`, or files importing
// `bun:test`, `@playwright/test`, `vitest`, `node:test`) into runnable CP `cap:test` code:
//   - Rewrites test library imports to `import { test, expect_eq } from "cap:test";`
//   - Strips `describe(...)` wrappers while preserving inner `test(...)` calls
//   - Removes `async`/`await` and untyped callback params (`async ({ page }) =>` -> `() =>`)
//   - Lowers `expect(...)` matcher chains into `expect_eq(...)`
//   - Wraps top-level `test(...)` statements into `fn main() { ... }` when no `main` exists

use super::ts::count_braces;

fn is_ident_byte(b: u8) -> bool {
    b.is_ascii_alphanumeric() || b == b'_' || b == b'$'
}

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
        } else if b == b'"' {
            in_double = true;
        } else if b == b'\'' {
            in_single = true;
        } else if b == b'`' {
            in_backtick = true;
        } else if b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
            return (&line[..i], &line[i..]);
        }
        i += 1;
    }
    (line, "")
}

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
        } else if b == b'"' {
            in_double = true;
        } else if b == b'\'' {
            in_single = true;
        } else if b == b'`' {
            in_backtick = true;
        } else if b == open {
            depth += 1;
        } else if b == close {
            depth -= 1;
            if depth == 0 {
                return Some(i);
            }
        }
        i += 1;
    }
    None
}

/// Detect whether a TS file is a test file by path or test framework import.
pub fn is_ts_test_file(rel_path: &str, source: &str) -> bool {
    let lower = rel_path.to_ascii_lowercase();
    if lower.ends_with(".test.ts")
        || lower.ends_with(".spec.ts")
        || lower.ends_with(".test.tsx")
        || lower.ends_with(".spec.tsx")
        || lower.ends_with(".test.mts")
        || lower.ends_with(".spec.mts")
        || lower.ends_with(".test.js")
        || lower.ends_with(".spec.js")
        || lower.contains("__tests__/")
        || lower.starts_with("__tests__/")
    {
        return true;
    }

    const TEST_PKGS: &[&str] = &[
        "\"bun:test\"",
        "'bun:test'",
        "\"@playwright/test\"",
        "'@playwright/test'",
        "\"vitest\"",
        "'vitest'",
        "\"node:test\"",
        "'node:test'",
    ];
    TEST_PKGS.iter().any(|pat| source.contains(pat))
}

fn is_test_framework_import_line(trimmed: &str) -> bool {
    if !trimmed.starts_with("import ") {
        return false;
    }
    const TARGETS: &[&str] = &[
        "from \"cap:test\"",
        "from 'cap:test'",
        "from \"bun:test\"",
        "from 'bun:test'",
        "from \"@playwright/test\"",
        "from '@playwright/test'",
        "from \"vitest\"",
        "from 'vitest'",
        "from \"node:test\"",
        "from 'node:test'",
    ];
    TARGETS.iter().any(|t| trimmed.contains(t))
}

/// Remove standalone `await ` keywords outside of string literals and `//` comments.
fn strip_await_keyword(line: &str) -> String {
    if !line.contains("await") {
        return line.to_string();
    }
    let bytes = line.as_bytes();
    let mut out = String::with_capacity(line.len());
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double
            && !in_single
            && !in_backtick
            && b == b'/'
            && i + 1 < bytes.len()
            && bytes[i + 1] == b'/'
        {
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

        if i + 5 <= bytes.len() && &bytes[i..i + 5] == b"await" {
            let prev_ok = i == 0 || !is_ident_byte(bytes[i - 1]);
            let next_b = bytes.get(i + 5).copied();
            let next_ok = matches!(next_b, Some(b' ') | Some(b'\t') | Some(b'('));
            if prev_ok && next_ok {
                i += 5;
                while i < bytes.len() && (bytes[i] == b' ' || bytes[i] == b'\t') {
                    i += 1;
                }
                continue;
            }
        }

        let ch = line[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Strip `async ` before arrow callbacks `async (...) =>` outside strings/comments.
fn strip_async_arrows(line: &str) -> String {
    if !line.contains("async") {
        return line.to_string();
    }
    let bytes = line.as_bytes();
    let mut out = String::with_capacity(line.len());
    let mut i = 0;
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;

    while i < bytes.len() {
        let b = bytes[i];
        if !in_double
            && !in_single
            && !in_backtick
            && b == b'/'
            && i + 1 < bytes.len()
            && bytes[i + 1] == b'/'
        {
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

        if i + 5 <= bytes.len() && &bytes[i..i + 5] == b"async" {
            let prev_ok = i == 0 || !is_ident_byte(bytes[i - 1]);
            let rest = &line[i + 5..];
            let trimmed_rest = rest.trim_start();
            let is_async_arrow = trimmed_rest.starts_with('(') && rest.contains("=>");
            if prev_ok && is_async_arrow {
                i += 5;
                while i < bytes.len() && (bytes[i] == b' ' || bytes[i] == b'\t') {
                    i += 1;
                }
                continue;
            }
        }

        let ch = line[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }
    out
}

/// Normalize `test("name", async ({ page }) => {` / `it("name", async (_p) => {`
/// to `test("name", () => {`.
fn normalize_test_call_line(line: &str) -> String {
    let indent_len = line.len() - line.trim_start().len();
    let indent = &line[..indent_len];
    let trimmed = line.trim();

    let rest_after_callee = if let Some(r) = trimmed.strip_prefix("test(") {
        Some(r)
    } else if let Some(r) = trimmed.strip_prefix("it(") {
        Some(r)
    } else if let Some(r) = trimmed.strip_prefix("test.only(") {
        Some(r)
    } else if let Some(r) = trimmed.strip_prefix("test.skip(") {
        Some(r)
    } else if let Some(r) = trimmed.strip_prefix("it.only(") {
        Some(r)
    } else if let Some(r) = trimmed.strip_prefix("it.skip(") {
        Some(r)
    } else {
        None
    };

    let Some(args_rest) = rest_after_callee else {
        return line.to_string();
    };

    // Find the top-level comma separating test name and callback
    let bytes = args_rest.as_bytes();
    let mut in_double = false;
    let mut in_single = false;
    let mut in_backtick = false;
    let mut depth_paren = 0i32;
    let mut comma_idx = None;
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
                b'(' => depth_paren += 1,
                b')' => depth_paren -= 1,
                b',' if depth_paren == 0 => {
                    comma_idx = Some(i);
                    break;
                }
                _ => {}
            }
        }
        i += 1;
    }

    let Some(cidx) = comma_idx else {
        return format!("{}test({}", indent, args_rest);
    };

    let name_arg = args_rest[..cidx].trim();
    let mut cb = args_rest[cidx + 1..].trim_start();
    if let Some(r) = cb.strip_prefix("async ") {
        cb = r.trim_start();
    }

    // Rewrite `(...) => <tail>` to `() => <tail>` so CP `test(name, () => { ... })` matches signature
    if cb.starts_with('(') {
        if let Some(close_p) = find_matching_delim(cb, 0, b'(', b')') {
            let after_p = cb[close_p + 1..].trim_start();
            if let Some(after_arrow) = after_p.strip_prefix("=>") {
                return format!(
                    "{}test({}, () =>{}",
                    indent, name_arg, after_arrow
                );
            }
        }
    }

    format!("{}test({}, {}", indent, name_arg, cb)
}

/// Lower `expect(a).<matcher>(b)` and `expect(a).not.<matcher>(b)` chains to CP `expect_eq(...)`.
pub fn rewrite_ts_expect_calls(line: &str) -> String {
    if !line.contains("expect(") {
        return line.to_string();
    }
    let (code_part, comment_part) = split_inline_comment(line);
    let bytes = code_part.as_bytes();
    let mut out = String::with_capacity(code_part.len() + 32);
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

        if i + 7 <= bytes.len() && &bytes[i..i + 7] == b"expect(" {
            let prev_ok = i == 0 || !is_ident_byte(bytes[i - 1]);
            if prev_ok {
                if let Some(close_expect) = find_matching_delim(code_part, i + 6, b'(', b')') {
                    let actual = code_part[i + 7..close_expect].trim();
                    let mut pos = close_expect + 1;
                    while pos < bytes.len() && (bytes[pos] == b' ' || bytes[pos] == b'\t') {
                        pos += 1;
                    }
                    if pos < bytes.len() && bytes[pos] == b'.' {
                        pos += 1;
                        let mut negated = false;
                        if code_part[pos..].starts_with("not.") {
                            negated = true;
                            pos += 4;
                        }
                        let m_start = pos;
                        while pos < bytes.len()
                            && (bytes[pos].is_ascii_alphanumeric() || bytes[pos] == b'_')
                        {
                            pos += 1;
                        }
                        let matcher = &code_part[m_start..pos];
                        if pos < bytes.len() && bytes[pos] == b'(' {
                            if let Some(close_m) = find_matching_delim(code_part, pos, b'(', b')') {
                                let expected = code_part[pos + 1..close_m].trim();
                                let lowered = match matcher {
                                    "toBe" | "toEqual" | "toStrictEqual" => {
                                        if negated {
                                            Some(format!("expect_eq({} != {}, true)", actual, expected))
                                        } else {
                                            Some(format!("expect_eq({}, {})", actual, expected))
                                        }
                                    }
                                    "toBeTruthy" => {
                                        let target = if negated { "false" } else { "true" };
                                        Some(format!("expect_eq({}, {})", actual, target))
                                    }
                                    "toBeFalsy" => {
                                        let target = if negated { "true" } else { "false" };
                                        Some(format!("expect_eq({}, {})", actual, target))
                                    }
                                    "toBeNull" | "toBeUndefined" => {
                                        let op = if negated { "!=" } else { "==" };
                                        Some(format!("expect_eq({} {} null, true)", actual, op))
                                    }
                                    "toBeDefined" => {
                                        let op = if negated { "==" } else { "!=" };
                                        Some(format!("expect_eq({} {} null, true)", actual, op))
                                    }
                                    "toContain" => {
                                        let op = if negated { "==" } else { "!=" };
                                        Some(format!(
                                            "expect_eq(std.mem.indexOf(u8, {}, {}) {} null, true)",
                                            actual, expected, op
                                        ))
                                    }
                                    "toBeGreaterThan" => {
                                        let op = if negated { "<=" } else { ">" };
                                        Some(format!("expect_eq({} {} {}, true)", actual, op, expected))
                                    }
                                    "toBeGreaterThanOrEqual" => {
                                        let op = if negated { "<" } else { ">=" };
                                        Some(format!("expect_eq({} {} {}, true)", actual, op, expected))
                                    }
                                    "toBeLessThan" => {
                                        let op = if negated { ">=" } else { "<" };
                                        Some(format!("expect_eq({} {} {}, true)", actual, op, expected))
                                    }
                                    "toBeLessThanOrEqual" => {
                                        let op = if negated { ">" } else { "<=" };
                                        Some(format!("expect_eq({} {} {}, true)", actual, op, expected))
                                    }
                                    _ => None,
                                };
                                if let Some(rep) = lowered {
                                    out.push_str(&rep);
                                    i = close_m + 1;
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
        }

        let ch = code_part[i..].chars().next().unwrap();
        out.push(ch);
        i += ch.len_utf8();
    }

    out.push_str(comment_part);
    out
}

fn is_describe_open_line(trimmed: &str) -> bool {
    let starts_describe = trimmed.starts_with("describe(")
        || trimmed.starts_with("test.describe(")
        || trimmed.starts_with("describe.only(")
        || trimmed.starts_with("describe.skip(")
        || trimmed.starts_with("test.describe.only(")
        || trimmed.starts_with("test.describe.skip(");
    if !starts_describe {
        return false;
    }
    let (o, c) = count_braces(trimmed);
    o > c
}

fn is_describe_close_line(trimmed: &str) -> bool {
    let (code, _) = split_inline_comment(trimmed);
    let c = code.trim();
    c == "});" || c == "})"
}

/// Wrap top-level `test(...)` statements into `fn main() { ... }` if no `fn main(` exists.
fn wrap_top_level_tests_in_main(code: &str) -> String {
    if code.contains("fn main(") || code.contains("fn main (") {
        return code.to_string();
    }

    let lines: Vec<&str> = code.lines().collect();
    let mut outer_lines: Vec<String> = Vec::with_capacity(lines.len());
    let mut main_lines: Vec<String> = Vec::new();

    let mut brace_depth = 0i32;
    let mut in_test_block = false;
    let mut test_base_indent = 0usize;

    for raw in lines {
        let trimmed = raw.trim();

        if !in_test_block && brace_depth == 0 && trimmed.starts_with("test(") {
            in_test_block = true;
            test_base_indent = raw.len() - raw.trim_start().len();
            let mut stripped = raw
                .get(test_base_indent..)
                .unwrap_or(trimmed)
                .to_string();
            let (o, c) = count_braces(&stripped);
            brace_depth += o - c;
            if brace_depth <= 0 {
                brace_depth = 0;
                if stripped.trim_end().ends_with(')') {
                    stripped.push(';');
                }
                in_test_block = false;
            }
            main_lines.push(format!("    {}", stripped));
            continue;
        }

        if in_test_block {
            let (o, c) = count_braces(raw);
            brace_depth += o - c;
            let stripped = if raw.len() >= test_base_indent
                && raw[..test_base_indent].chars().all(|ch| ch == ' ' || ch == '\t')
            {
                &raw[test_base_indent..]
            } else {
                raw
            };
            if brace_depth <= 0 {
                brace_depth = 0;
                in_test_block = false;
                let mut line_out = stripped.to_string();
                let (code_p, _) = split_inline_comment(&line_out);
                if code_p.trim_end().ends_with("})") {
                    line_out = format!("{};", line_out.trim_end());
                }
                main_lines.push(format!("    {}", line_out));
            } else if stripped.trim().is_empty() {
                main_lines.push(String::new());
            } else {
                main_lines.push(format!("    {}", stripped));
            }
            continue;
        }

        let (o, c) = count_braces(raw);
        brace_depth = (brace_depth + o - c).max(0);
        outer_lines.push(raw.to_string());
    }

    if main_lines.is_empty() {
        return code.to_string();
    }

    let mut result = outer_lines.join("\n");
    if !result.ends_with('\n') {
        result.push('\n');
    }
    if !result.ends_with("\n\n") {
        result.push('\n');
    }
    result.push_str("fn main() {\n");
    for ml in main_lines {
        result.push_str(&ml);
        result.push('\n');
    }
    result.push_str("}\n");
    result
}

/// Full TS test lowering pipeline applied to transpiled CP output before parser self-check.
pub fn lower_ts_test_to_cap_test(cp_code: &str) -> String {
    let mut out_lines: Vec<String> = Vec::new();
    let mut emitted_cap_test_import = false;
    let mut brace_depth = 0i32;
    let mut describe_depths: Vec<i32> = Vec::new();

    for raw_line in cp_code.lines() {
        let trimmed = raw_line.trim();
        let indent_len = raw_line.len() - raw_line.trim_start().len();
        let indent = &raw_line[..indent_len];

        // 1. Rewrite test framework imports to canonical `cap:test` import
        if brace_depth == 0 && is_test_framework_import_line(trimmed) {
            if !emitted_cap_test_import {
                out_lines.push("import { test, expect_eq } from \"cap:test\";".to_string());
                emitted_cap_test_import = true;
            }
            continue;
        }

        // 2. Unwrap `describe("...", () => {` and its matching `});`
        if is_describe_open_line(trimmed) {
            describe_depths.push(brace_depth);
            out_lines.push(format!("{}// [cap-test] {}", indent, trimmed));
            continue;
        }
        if is_describe_close_line(trimmed) && describe_depths.last() == Some(&brace_depth) {
            describe_depths.pop();
            out_lines.push(format!("{}// [cap-test] {}", indent, trimmed));
            continue;
        }

        // 3. Lower `test(...)` callback headers, `async`/`await`, and `expect(...)` chains
        let mut line = normalize_test_call_line(raw_line);
        line = strip_async_arrows(&line);
        line = strip_await_keyword(&line);
        line = rewrite_ts_expect_calls(&line);

        let (o, c) = count_braces(&line);
        brace_depth = (brace_depth + o - c).max(0);
        out_lines.push(line);
    }

    // Ensure `import { test, expect_eq } from "cap:test";` is present if test/expect_eq is used
    let mut joined = out_lines.join("\n");
    if !joined.ends_with('\n') {
        joined.push('\n');
    }
    if !emitted_cap_test_import && (joined.contains("test(") || joined.contains("expect_eq(")) {
        let mut with_import = String::with_capacity(joined.len() + 64);
        let mut inserted = false;
        for line in joined.lines() {
            with_import.push_str(line);
            with_import.push('\n');
            if !inserted && line.starts_with("// Migrated from TypeScript") {
                with_import.push_str("\nimport { test, expect_eq } from \"cap:test\";\n");
                inserted = true;
            }
        }
        if !inserted {
            with_import = format!("import {{ test, expect_eq }} from \"cap:test\";\n{}", joined);
        }
        joined = with_import;
    }

    // 4. Wrap top-level `test(...)` statements into `fn main() { ... }`
    wrap_top_level_tests_in_main(&joined)
}
