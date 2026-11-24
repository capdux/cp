use std::fs;
use std::path::{Path, PathBuf};

const INDENT_STR: &str = "    ";

const SKIP_DIRS: &[&str] = &[
    ".cap",
    "cap_modules",
    "vendor",
    "bin",
    "temp",
    ".git",
    "target",
    "zig-cache",
    ".zig-cache",
    "zig-out",
    "node_modules",
];

fn is_ident_char(b: u8) -> bool {
    b.is_ascii_alphanumeric() || b == b'_'
}

fn push_indent(out: &mut String, level: usize) {
    for _ in 0..level {
        out.push_str(INDENT_STR);
    }
}

/// Count consecutive leading closing delimiters (`}`, `)`, `]`) at the start of a trimmed line,
/// allowing whitespace between them (e.g. `});`, `])`, `} else {`).
fn count_leading_closers(trimmed: &str) -> usize {
    let mut count = 0usize;
    for b in trimmed.bytes() {
        match b {
            b'}' | b')' | b']' => count += 1,
            b' ' | b'\t' => {}
            _ => break,
        }
    }
    count
}

/// Scan a Zig raw block line (inside `zig { ... }`), updating `brace_depth`.
/// Respects Zig line comments `//`, string literals `"..."`, char literals `'...'`,
/// and Zig multiline string lines starting with `\\`.
fn scan_zig_line(line: &str, brace_depth: &mut usize) {
    let trimmed_start = line.trim_start();
    if trimmed_start.starts_with("\\\\") {
        return;
    }
    let bytes = line.as_bytes();
    let len = bytes.len();
    let mut i = 0usize;
    while i < len {
        let b = bytes[i];
        if b == b'/' && i + 1 < len && bytes[i + 1] == b'/' {
            break;
        }
        if b == b'"' {
            i += 1;
            while i < len {
                if bytes[i] == b'\\' && i + 1 < len {
                    i += 2;
                    continue;
                }
                if bytes[i] == b'"' {
                    i += 1;
                    break;
                }
                i += 1;
            }
            continue;
        }
        if b == b'\'' {
            i += 1;
            while i < len {
                if bytes[i] == b'\\' && i + 1 < len {
                    i += 2;
                    continue;
                }
                if bytes[i] == b'\'' {
                    i += 1;
                    break;
                }
                i += 1;
            }
            continue;
        }
        if b == b'{' {
            *brace_depth += 1;
        } else if b == b'}' {
            *brace_depth = brace_depth.saturating_sub(1);
            if *brace_depth == 0 {
                return;
            }
        }
        i += 1;
    }
}

/// Format a CP source string with 4-space indentation, preserved comments/strings/raw blocks,
/// collapsed blank lines (at most 1), normalized control-flow spacing (`if (`, `while (`,
/// `for (`, `else {`), trimmed trailing whitespace, and a single trailing newline.
///
/// Guaranteed idempotent: `format_source(&format_source(s)) == format_source(s)`.
pub fn format_source(src: &str) -> String {
    let normalized = src.replace("\r\n", "\n").replace('\r', "\n");
    let mut out = String::with_capacity(normalized.len() + 64);

    let mut indent: usize = 0;
    let mut in_block_comment = false;
    let mut in_template_str = false;
    let mut in_zig_block = false;
    let mut zig_brace_depth: usize = 0;
    let mut zig_base_indent: usize = 0;

    let mut pending_blank = false;
    let mut has_emitted_any = false;

    for raw_line in normalized.split('\n') {
        // 1. Inside a multi-line `zig { ... }` raw block: preserve content verbatim
        if in_zig_block {
            if pending_blank && has_emitted_any {
                out.push('\n');
                pending_blank = false;
            }
            let trimmed = raw_line.trim();
            let mut depth_after = zig_brace_depth;
            scan_zig_line(raw_line, &mut depth_after);
            if depth_after == 0 {
                in_zig_block = false;
                zig_brace_depth = 0;
                if trimmed.starts_with('}') {
                    push_indent(&mut out, zig_base_indent);
                    out.push_str(trimmed);
                    out.push('\n');
                } else {
                    out.push_str(raw_line.trim_end_matches(|c| c == ' ' || c == '\t'));
                    out.push('\n');
                }
                indent = zig_base_indent;
                has_emitted_any = true;
            } else {
                zig_brace_depth = depth_after;
                out.push_str(raw_line.trim_end_matches(|c| c == ' ' || c == '\t'));
                out.push('\n');
                has_emitted_any = true;
            }
            continue;
        }

        // 2. Inside a multi-line template string `` `...` ``: preserve verbatim
        if in_template_str {
            if pending_blank && has_emitted_any {
                out.push('\n');
                pending_blank = false;
            }
            out.push_str(raw_line);
            out.push('\n');
            has_emitted_any = true;
            let bytes = raw_line.as_bytes();
            let mut i = 0;
            while i < bytes.len() {
                if bytes[i] == b'\\' && i + 1 < bytes.len() {
                    i += 2;
                    continue;
                }
                if bytes[i] == b'`' {
                    in_template_str = false;
                    // Update indent for any code following the closing backtick
                    let rest = &raw_line[i + 1..];
                    let cur_indent = indent;
                    let _ = process_normal_code(
                        rest,
                        &mut indent,
                        &mut in_block_comment,
                        &mut in_template_str,
                        &mut in_zig_block,
                        &mut zig_brace_depth,
                        &mut zig_base_indent,
                        cur_indent,
                    );
                    break;
                }
                i += 1;
            }
            continue;
        }

        // 3. Inside a multi-line `/* ... */` block comment
        if in_block_comment {
            if pending_blank && has_emitted_any {
                out.push('\n');
                pending_blank = false;
            }
            let trimmed = raw_line.trim();
            if trimmed.is_empty() {
                out.push('\n');
            } else if trimmed.starts_with('*') {
                push_indent(&mut out, indent);
                out.push(' ');
                out.push_str(trimmed);
                out.push('\n');
            } else {
                out.push_str(raw_line.trim_end_matches(|c| c == ' ' || c == '\t'));
                out.push('\n');
            }
            has_emitted_any = true;

            if let Some(end_idx) = raw_line.find("*/") {
                in_block_comment = false;
                let rest = &raw_line[end_idx + 2..];
                if !rest.trim().is_empty() {
                    let cur_indent = indent;
                    let _ = process_normal_code(
                        rest,
                        &mut indent,
                        &mut in_block_comment,
                        &mut in_template_str,
                        &mut in_zig_block,
                        &mut zig_brace_depth,
                        &mut zig_base_indent,
                        cur_indent,
                    );
                }
            }
            continue;
        }

        // 4. Normal CP line
        let trimmed = raw_line.trim();
        if trimmed.is_empty() {
            if has_emitted_any {
                pending_blank = true;
            }
            continue;
        }

        if pending_blank && has_emitted_any {
            out.push('\n');
            pending_blank = false;
        }

        // Zig multiline string literal line `\\...`: align leading indent only, preserve body
        if trimmed.starts_with("\\\\") {
            push_indent(&mut out, indent);
            out.push_str(raw_line.trim_start_matches(|c| c == ' ' || c == '\t'));
            out.push('\n');
            has_emitted_any = true;
            continue;
        }

        let leading_closers = count_leading_closers(trimmed);
        let line_indent = indent.saturating_sub(leading_closers);

        let formatted_line = process_normal_code(
            trimmed,
            &mut indent,
            &mut in_block_comment,
            &mut in_template_str,
            &mut in_zig_block,
            &mut zig_brace_depth,
            &mut zig_base_indent,
            line_indent,
        );

        push_indent(&mut out, line_indent);
        out.push_str(formatted_line.trim_end_matches(|c| c == ' ' || c == '\t'));
        out.push('\n');
        has_emitted_any = true;
    }

    out
}

/// Lexically scan a single normal CP code line (trimmed):
/// - Normalizes control-flow keyword spacing (`if (`, `while (`, `for (`, `else {`)
/// - Updates `indent` based on `{`/`}`/`(`/`)`/`[`/`]` outside comments, strings, and `zig { ... }`
/// - Detects entry into multi-line `/* ... */`, template string, or `zig { ... }` blocks
#[allow(clippy::too_many_arguments)]
fn process_normal_code(
    line: &str,
    indent: &mut usize,
    in_block_comment: &mut bool,
    in_template_str: &mut bool,
    in_zig_block: &mut bool,
    zig_brace_depth: &mut usize,
    zig_base_indent: &mut usize,
    line_indent: usize,
) -> String {
    let bytes = line.as_bytes();
    let len = bytes.len();
    let mut out = String::with_capacity(len + 8);
    let mut i = 0usize;
    let mut prev_non_ws: Option<u8> = None;

    while i < len {
        let b = bytes[i];

        // Single-line comment `// ...` -> copy rest of line verbatim
        if b == b'/' && i + 1 < len && bytes[i + 1] == b'/' {
            out.push_str(&line[i..]);
            break;
        }

        // Block comment `/* ... */`
        if b == b'/' && i + 1 < len && bytes[i + 1] == b'*' {
            if let Some(rel_end) = line[i + 2..].find("*/") {
                let end = i + 2 + rel_end + 2;
                out.push_str(&line[i..end]);
                i = end;
                continue;
            } else {
                out.push_str(&line[i..]);
                *in_block_comment = true;
                break;
            }
        }

        // Double-quoted string literal `"..."`
        if b == b'"' {
            let start = i;
            i += 1;
            while i < len {
                if bytes[i] == b'\\' && i + 1 < len {
                    i += 2;
                    continue;
                }
                if bytes[i] == b'"' {
                    i += 1;
                    break;
                }
                i += 1;
            }
            out.push_str(&line[start..i]);
            prev_non_ws = Some(b'"');
            continue;
        }

        // Single-quoted char literal `'...'`
        if b == b'\'' {
            let start = i;
            i += 1;
            while i < len {
                if bytes[i] == b'\\' && i + 1 < len {
                    i += 2;
                    continue;
                }
                if bytes[i] == b'\'' {
                    i += 1;
                    break;
                }
                i += 1;
            }
            out.push_str(&line[start..i]);
            prev_non_ws = Some(b'\'');
            continue;
        }

        // Backtick template string `` `...` ``
        if b == b'`' {
            let start = i;
            i += 1;
            let mut closed = false;
            while i < len {
                if bytes[i] == b'\\' && i + 1 < len {
                    i += 2;
                    continue;
                }
                if bytes[i] == b'`' {
                    i += 1;
                    closed = true;
                    break;
                }
                i += 1;
            }
            out.push_str(&line[start..i]);
            if !closed {
                *in_template_str = true;
                break;
            }
            prev_non_ws = Some(b'`');
            continue;
        }

        // Identifiers / keywords
        if b.is_ascii_alphabetic() || b == b'_' {
            let start = i;
            while i < len && is_ident_char(bytes[i]) {
                i += 1;
            }
            let word = &line[start..i];
            let preceded_by_dot = matches!(prev_non_ws, Some(b'.'));

            // Detect `zig { ... }` raw block
            if word == "zig" && !preceded_by_dot {
                let mut j = i;
                while j < len && (bytes[j] == b' ' || bytes[j] == b'\t') {
                    j += 1;
                }
                if j < len && bytes[j] == b'{' {
                    out.push_str("zig {");
                    i = j + 1;
                    let mut depth = 1usize;
                    let raw_start = i;
                    // Scan inside zig block on the same line
                    while i < len && depth > 0 {
                        let cb = bytes[i];
                        if cb == b'/' && i + 1 < len && bytes[i + 1] == b'/' {
                            i = len;
                            break;
                        }
                        if cb == b'"' {
                            i += 1;
                            while i < len {
                                if bytes[i] == b'\\' && i + 1 < len {
                                    i += 2;
                                    continue;
                                }
                                if bytes[i] == b'"' {
                                    i += 1;
                                    break;
                                }
                                i += 1;
                            }
                            continue;
                        }
                        if cb == b'\'' {
                            i += 1;
                            while i < len {
                                if bytes[i] == b'\\' && i + 1 < len {
                                    i += 2;
                                    continue;
                                }
                                if bytes[i] == b'\'' {
                                    i += 1;
                                    break;
                                }
                                i += 1;
                            }
                            continue;
                        }
                        if cb == b'{' {
                            depth += 1;
                        } else if cb == b'}' {
                            depth -= 1;
                        }
                        i += 1;
                    }
                    out.push_str(&line[raw_start..i]);
                    if depth > 0 {
                        *in_zig_block = true;
                        *zig_brace_depth = depth;
                        *zig_base_indent = line_indent;
                        break;
                    }
                    prev_non_ws = Some(b'}');
                    continue;
                }
            }

            // Normalize control-flow keyword spacing: `if (`, `while (`, `for (`, `else {`
            if !preceded_by_dot && matches!(word, "if" | "while" | "for") {
                let mut j = i;
                while j < len && (bytes[j] == b' ' || bytes[j] == b'\t') {
                    j += 1;
                }
                if j < len && bytes[j] == b'(' {
                    out.push_str(word);
                    out.push_str(" (");
                    *indent += 1;
                    i = j + 1;
                    prev_non_ws = Some(b'(');
                    continue;
                }
            } else if !preceded_by_dot && word == "else" {
                let mut j = i;
                while j < len && (bytes[j] == b' ' || bytes[j] == b'\t') {
                    j += 1;
                }
                if j < len && bytes[j] == b'{' {
                    out.push_str("else {");
                    *indent += 1;
                    i = j + 1;
                    prev_non_ws = Some(b'{');
                    continue;
                }
            }

            out.push_str(word);
            prev_non_ws = Some(bytes[i - 1]);
            continue;
        }

        // Delimiters that affect indentation
        match b {
            b'{' | b'(' | b'[' => {
                *indent += 1;
                out.push(b as char);
                prev_non_ws = Some(b);
                i += 1;
            }
            b'}' | b')' | b']' => {
                *indent = indent.saturating_sub(1);
                out.push(b as char);
                prev_non_ws = Some(b);
                i += 1;
            }
            _ => {
                // Copy full UTF-8 character safely
                let ch = line[i..].chars().next().unwrap();
                out.push(ch);
                if !ch.is_whitespace() {
                    prev_non_ws = Some(b);
                }
                i += ch.len_utf8();
            }
        }
    }

    out
}

fn should_skip_dir(name: &str) -> bool {
    SKIP_DIRS.contains(&name)
}

fn walk_dir_cp(dir: &Path, out: &mut Vec<PathBuf>) -> anyhow::Result<()> {
    let mut entries: Vec<PathBuf> = Vec::new();
    for entry in fs::read_dir(dir)? {
        let entry = entry?;
        entries.push(entry.path());
    }
    entries.sort();

    for path in entries {
        let meta = match fs::symlink_metadata(&path) {
            Ok(m) => m,
            Err(_) => continue,
        };
        if meta.file_type().is_symlink() {
            continue;
        }
        if meta.is_dir() {
            if let Some(name) = path.file_name().and_then(|s| s.to_str()) {
                if should_skip_dir(name) {
                    continue;
                }
            }
            walk_dir_cp(&path, out)?;
        } else if meta.is_file() {
            if path.extension().and_then(|e| e.to_str()) == Some("cp") {
                out.push(path);
            }
        }
    }
    Ok(())
}

/// Collect `.cp` files from explicit file paths and recursive directories
/// (skipping `.cap/`, `cap_modules/`, `vendor/`, `bin/`, `temp/`, etc.).
pub fn collect_cp_files(paths: &[PathBuf]) -> anyhow::Result<Vec<PathBuf>> {
    let targets: Vec<PathBuf> = if paths.is_empty() {
        vec![PathBuf::from(".")]
    } else {
        paths.to_vec()
    };

    let mut files = Vec::new();
    for p in targets {
        if !p.exists() {
            return Err(anyhow::anyhow!("path not found: {}", p.display()));
        }
        if p.is_dir() {
            walk_dir_cp(&p, &mut files)?;
        } else {
            files.push(p);
        }
    }
    Ok(files)
}

/// Run `cpc fmt` across the given paths.
/// Returns `Ok(true)` if all files are formatted (or were formatted in-place / written to stdout),
/// and `Ok(false)` in `--check` mode when one or more files need formatting.
pub fn run_fmt(paths: &[PathBuf], check: bool, stdout: bool) -> anyhow::Result<bool> {
    let files = collect_cp_files(paths)?;
    let mut all_formatted = true;

    for path in files {
        let original = fs::read_to_string(&path)
            .map_err(|e| anyhow::anyhow!("failed to read {}: {}", path.display(), e))?;
        let formatted = format_source(&original);

        if stdout {
            print!("{}", formatted);
        } else if check {
            if formatted != original {
                println!("{}", path.display());
                all_formatted = false;
            }
        } else if formatted != original {
            fs::write(&path, &formatted)
                .map_err(|e| anyhow::anyhow!("failed to write {}: {}", path.display(), e))?;
        }
    }

    Ok(all_formatted)
}

#[cfg(test)]
mod tests {
    use super::format_source;

    #[test]
    fn test_basic_indent_and_keywords() {
        let input = "\n\nfn main() {   \nlet x: i32 = 1;\nif(x > 0) {\nwhile(x < 10) {\nx += 1;\n}\n} else{\nx = 0;\n}\n}\n\n\n";
        let expected = "fn main() {\n    let x: i32 = 1;\n    if (x > 0) {\n        while (x < 10) {\n            x += 1;\n        }\n    } else {\n        x = 0;\n    }\n}\n";
        let out = format_source(input);
        assert_eq!(out, expected);
        assert_eq!(format_source(&out), out);
    }

    #[test]
    fn test_preserves_comments_strings_and_zig_blocks() {
        let input = r#"// top comment with { brace
fn demo() {
/*
 * block comment with } and if(x)
 */
let s: string = "if(a) { else{ } \"escaped\"";
let c: u8 = '{';
let ml =
\\line 1 with if(x) {
\\  line 2 preserved
;
zig {
  const raw_zig = if(true) 1 else 2;
    const keep_indent = raw_zig;
}
for(let i: i32 = 0; i < 3; i += 1) {
// inline comment
}
}
"#;
        let out1 = format_source(input);
        let out2 = format_source(&out1);
        assert_eq!(out1, out2, "formatter must be strictly idempotent");
        assert!(out1.contains("    let s: string = \"if(a) { else{ } \\\"escaped\\\"\";"));
        assert!(out1.contains("  const raw_zig = if(true) 1 else 2;"));
        assert!(out1.contains("    const keep_indent = raw_zig;"));
        assert!(out1.contains("    for (let i: i32 = 0; i < 3; i += 1) {"));
        assert!(out1.contains("    \\\\  line 2 preserved"));
    }

    #[test]
    fn test_blank_line_collapsing_and_trailing_newline() {
        let input = "fn a() {}\n\n\n\nfn b() {}   ";
        let expected = "fn a() {}\n\nfn b() {}\n";
        assert_eq!(format_source(input), expected);
        assert_eq!(format_source(expected), expected);
    }
}
