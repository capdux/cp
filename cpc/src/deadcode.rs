use std::collections::{BTreeSet, HashMap};
use std::fs;
use std::path::{Path, PathBuf};

#[derive(Debug, Clone)]
pub struct ExportedDecl {
    pub file: PathBuf,
    pub file_idx: usize,
    pub line: usize,
    pub kind: &'static str,
    pub name: String,
}

#[derive(Debug, Clone)]
pub struct DeadcodeReport {
    pub files_scanned: usize,
    pub symbols_checked: usize,
    pub unused: Vec<ExportedDecl>,
}

const SKIP_DIRS: &[&str] = &[
    ".git",
    ".cap",
    ".zig-cache",
    "zig-cache",
    "node_modules",
    "target",
    "bin",
    "vendor",
];

fn collect_cp_files_recursive(path: &Path, out: &mut BTreeSet<PathBuf>) {
    let Ok(meta) = fs::metadata(path) else {
        return;
    };
    if meta.is_file() {
        if let Some(name) = path.file_name().and_then(|s| s.to_str()) {
            if name.ends_with(".cp") && !name.ends_with(".d.cp") {
                out.insert(path.to_path_buf());
            }
        }
        return;
    }
    if meta.is_dir() {
        let Ok(entries) = fs::read_dir(path) else {
            return;
        };
        for entry in entries.flatten() {
            let entry_path = entry.path();
            if let Some(name) = entry_path.file_name().and_then(|s| s.to_str()) {
                if entry_path.is_dir() && SKIP_DIRS.contains(&name) {
                    continue;
                }
            }
            collect_cp_files_recursive(&entry_path, out);
        }
    }
}

/// Strip comments (`// ...`, `/* ... */`) and string/char literals while preserving
/// exact newline positions so 1-based line numbers remain identical to the source.
fn strip_comments_and_strings(src: &str) -> String {
    let bytes = src.as_bytes();
    let len = bytes.len();
    let mut out = String::with_capacity(len);
    let mut i = 0usize;
    let mut in_block_comment = false;

    while i < len {
        let b = bytes[i];
        if in_block_comment {
            if b == b'*' && i + 1 < len && bytes[i + 1] == b'/' {
                in_block_comment = false;
                out.push(' ');
                out.push(' ');
                i += 2;
            } else if b == b'\n' {
                out.push('\n');
                i += 1;
            } else {
                out.push(' ');
                i += 1;
            }
            continue;
        }

        // Line comment
        if b == b'/' && i + 1 < len && bytes[i + 1] == b'/' {
            while i < len && bytes[i] != b'\n' {
                out.push(' ');
                i += 1;
            }
            continue;
        }

        // Block comment
        if b == b'/' && i + 1 < len && bytes[i + 1] == b'*' {
            in_block_comment = true;
            out.push(' ');
            out.push(' ');
            i += 2;
            continue;
        }

        // String / char / template literals
        if b == b'"' || b == b'\'' || b == b'`' {
            let quote = b;
            out.push(' ');
            i += 1;
            while i < len {
                let c = bytes[i];
                if c == b'\\' && i + 1 < len {
                    out.push(' ');
                    if bytes[i + 1] == b'\n' {
                        out.push('\n');
                    } else {
                        out.push(' ');
                    }
                    i += 2;
                    continue;
                }
                if c == quote {
                    out.push(' ');
                    i += 1;
                    break;
                }
                if c == b'\n' {
                    out.push('\n');
                } else {
                    out.push(' ');
                }
                i += 1;
            }
            continue;
        }

        // Normal code: preserve UTF-8 safely
        if b.is_ascii() {
            out.push(b as char);
            i += 1;
        } else {
            let ch = src[i..].chars().next().unwrap();
            out.push(ch);
            i += ch.len_utf8();
        }
    }

    out
}

fn is_ident_start(b: u8) -> bool {
    b.is_ascii_alphabetic() || b == b'_'
}

fn is_ident_continue(b: u8) -> bool {
    b.is_ascii_alphanumeric() || b == b'_'
}

fn extract_idents_in_line(line: &str) -> Vec<&str> {
    let bytes = line.as_bytes();
    let len = bytes.len();
    let mut idents = Vec::new();
    let mut i = 0usize;
    while i < len {
        if is_ident_start(bytes[i]) {
            let start = i;
            i += 1;
            while i < len && is_ident_continue(bytes[i]) {
                i += 1;
            }
            idents.push(&line[start..i]);
        } else {
            i += 1;
        }
    }
    idents
}

fn parse_exported_decl(line: &str) -> Option<(&'static str, String)> {
    let trimmed = line.trim_start();
    let rest = trimmed.strip_prefix("pub")?;
    if !rest.starts_with(|c: char| c.is_whitespace()) {
        return None;
    }
    let mut tokens = rest.split_whitespace();
    let mut kw = tokens.next()?;

    // Allow modifiers like `pub async fn`, `pub extern fn`, `pub inline fn`, `pub comptime fn`
    while matches!(kw, "async" | "extern" | "inline" | "comptime" | "export") {
        kw = tokens.next()?;
    }

    let kind: &'static str = match kw {
        "fn" => "fn",
        "struct" => "struct",
        "enum" => "enum",
        "const" => "const",
        _ => return None,
    };

    let raw_name = tokens.next()?;
    let name_bytes = raw_name.as_bytes();
    if name_bytes.is_empty() || !is_ident_start(name_bytes[0]) {
        return None;
    }
    let mut end = 1usize;
    while end < name_bytes.len() && is_ident_continue(name_bytes[end]) {
        end += 1;
    }
    let name = &raw_name[..end];

    if name == "main" || name == "init" || name.starts_with("__cp_") {
        return None;
    }

    Some((kind, name.to_string()))
}

pub fn analyze_deadcode(paths: &[PathBuf]) -> anyhow::Result<DeadcodeReport> {
    let mut file_set: BTreeSet<PathBuf> = BTreeSet::new();
    if paths.is_empty() {
        collect_cp_files_recursive(Path::new("."), &mut file_set);
    } else {
        for p in paths {
            collect_cp_files_recursive(p, &mut file_set);
        }
    }

    let files: Vec<PathBuf> = file_set.into_iter().collect();
    let mut decls: Vec<ExportedDecl> = Vec::new();
    let mut total_ident_counts: HashMap<String, usize> = HashMap::new();
    let mut line_ident_counts: HashMap<(usize, usize, String), usize> = HashMap::new();

    for (file_idx, path) in files.iter().enumerate() {
        let Ok(src) = fs::read_to_string(path) else {
            continue;
        };
        let cleaned = strip_comments_and_strings(&src);

        for (line_idx, line) in cleaned.lines().enumerate() {
            let line_no = line_idx + 1;

            if let Some((kind, name)) = parse_exported_decl(line) {
                decls.push(ExportedDecl {
                    file: path.clone(),
                    file_idx,
                    line: line_no,
                    kind,
                    name,
                });
            }

            for ident in extract_idents_in_line(line) {
                *total_ident_counts.entry(ident.to_string()).or_insert(0) += 1;
                *line_ident_counts
                    .entry((file_idx, line_no, ident.to_string()))
                    .or_insert(0) += 1;
            }
        }
    }

    let mut unused = Vec::new();
    for decl in &decls {
        let total = total_ident_counts.get(&decl.name).copied().unwrap_or(0);
        let on_decl_line = line_ident_counts
            .get(&(decl.file_idx, decl.line, decl.name.clone()))
            .copied()
            .unwrap_or(0);
        let refs = total.saturating_sub(on_decl_line);
        if refs == 0 {
            unused.push(decl.clone());
        }
    }

    Ok(DeadcodeReport {
        files_scanned: files.len(),
        symbols_checked: decls.len(),
        unused,
    })
}

pub fn run_deadcode(paths: &[PathBuf]) -> anyhow::Result<bool> {
    let report = analyze_deadcode(paths)?;
    for item in &report.unused {
        println!(
            "{}:{}: unused pub {} {}",
            item.file.display(),
            item.line,
            item.kind,
            item.name
        );
    }
    println!(
        "deadcode: {} unused exported symbol(s) (checked {} symbols across {} files)",
        report.unused.len(),
        report.symbols_checked,
        report.files_scanned
    );
    Ok(report.unused.is_empty())
}
