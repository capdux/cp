use crate::codegen::Codegen;
use crate::parser::Parser as CPParser;
use crate::sema::Analyzer;
use std::collections::{BTreeMap, HashSet};
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

pub struct BriefOptions {
    pub max: usize,
    pub zig: Option<PathBuf>,
    pub require_backup: bool,
    pub keep_zig: bool,
    pub files: Vec<PathBuf>,
}

struct BriefErr {
    code: String,
    msg: String,
    line: usize,
    suggestion: String,
}

struct GateResult {
    backup: bool,
    zig_block: usize,
    var_decl: usize,
    tuple_at_n: usize,
    anyopaque: usize,
    ratio: Option<f64>,
    ratio_ok: Option<bool>,
}

fn is_executable_file(path: &Path) -> bool {
    let Ok(meta) = fs::metadata(path) else {
        return false;
    };
    if !meta.is_file() {
        return false;
    }
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        meta.permissions().mode() & 0o111 != 0
    }
    #[cfg(not(unix))]
    {
        true
    }
}

/// Resolve the Zig binary path using strict priority:
/// `--zig` > `$ZIG_BIN` > `./vendor/zig/zig` > `../vendor/zig/zig` > `/data/dev/cp/vendor/zig/zig` > `zig`
pub fn resolve_zig_bin(explicit: Option<&Path>) -> PathBuf {
    if let Some(p) = explicit {
        if !p.as_os_str().is_empty() {
            return p.to_path_buf();
        }
    }
    if let Ok(env_zig) = std::env::var("ZIG_BIN") {
        let trimmed = env_zig.trim();
        if !trimmed.is_empty() {
            return PathBuf::from(trimmed);
        }
    }
    let candidates = [
        Path::new("vendor/zig/zig"),
        Path::new("../vendor/zig/zig"),
        Path::new("/data/dev/cp/vendor/zig/zig"),
    ];
    for cand in candidates {
        if is_executable_file(cand) {
            return cand.to_path_buf();
        }
    }
    PathBuf::from("zig")
}

fn truncate_chars(s: &str, max_chars: usize) -> &str {
    match s.char_indices().nth(max_chars) {
        Some((idx, _)) => &s[..idx],
        None => s,
    }
}

fn count_zig_blocks(txt: &str) -> usize {
    use crate::lexer::Token;
    use logos::Logos;

    let tokens: Vec<Option<Token>> = Token::lexer(txt).map(Result::ok).collect();
    let mut count = 0usize;
    for i in 0..tokens.len() {
        if matches!(tokens[i], Some(Token::ZigEscape)) {
            let is_member_access =
                i > 0 && matches!(tokens[i - 1], Some(Token::Dot | Token::QuestionDot));
            let followed_by_lbrace =
                i + 1 < tokens.len() && matches!(tokens[i + 1], Some(Token::LBrace));
            if !is_member_access && followed_by_lbrace {
                count += 1;
            }
        }
    }
    count
}

fn count_var_decls(txt: &str) -> usize {
    let mut count = 0usize;
    for line in txt.lines() {
        let mut s = line.trim_start();
        if let Some(rest) = s.strip_prefix("pub") {
            if rest.starts_with(char::is_whitespace) {
                s = rest.trim_start();
            }
        }
        if let Some(rest) = s.strip_prefix("var") {
            if rest.starts_with(char::is_whitespace) {
                count += 1;
            }
        }
    }
    count
}

fn count_tuple_at_n(txt: &str) -> usize {
    let bytes = txt.as_bytes();
    let len = bytes.len();
    let mut count = 0usize;
    let mut i = 0usize;
    while i + 4 < len {
        if &bytes[i..i + 3] == b".@\"" {
            let mut j = i + 3;
            let digits_start = j;
            while j < len && bytes[j].is_ascii_digit() {
                j += 1;
            }
            if j > digits_start && j < len && bytes[j] == b'"' {
                count += 1;
                i = j + 1;
                continue;
            }
        }
        i += 1;
    }
    count
}

fn count_anyopaque(txt: &str) -> usize {
    let bytes = txt.as_bytes();
    let len = bytes.len();
    let mut count = 0usize;
    let mut i = 0usize;
    while i < len {
        if bytes[i] == b'*' {
            if bytes[i..].starts_with(b"*anyopaque") {
                count += 1;
                i += 10;
                continue;
            }
            if bytes[i..].starts_with(b"*any!") {
                count += 1;
                i += 5;
                continue;
            }
        }
        i += 1;
    }
    count
}

fn scan_gates(txt: &str, bak: &Path) -> GateResult {
    let backup = bak.exists();
    let (ratio, ratio_ok) = if backup {
        let a = txt.lines().count();
        let b = fs::read_to_string(bak)
            .map(|s| s.lines().count())
            .unwrap_or(0);
        if b > 0 {
            let r = a as f64 / b as f64;
            let rounded = (r * 1000.0).round() / 1000.0;
            (Some(rounded), Some(r <= 1.2))
        } else {
            (Some(0.0), Some(true))
        }
    } else {
        (None, None)
    };

    GateResult {
        backup,
        zig_block: count_zig_blocks(txt),
        var_decl: count_var_decls(txt),
        tuple_at_n: count_tuple_at_n(txt),
        anyopaque: count_anyopaque(txt),
        ratio,
        ratio_ok,
    }
}

fn is_zig_ast_error_line(line: &str) -> bool {
    let Some(zig_idx) = line.find(".zig:") else {
        return false;
    };
    let rest = &line[zig_idx + 5..];
    let mut parts = rest.splitn(3, ':');
    let Some(line_num) = parts.next() else {
        return false;
    };
    let Some(col_num) = parts.next() else {
        return false;
    };
    let Some(tail) = parts.next() else {
        return false;
    };
    !line_num.is_empty()
        && line_num.bytes().all(|b| b.is_ascii_digit())
        && !col_num.is_empty()
        && col_num.bytes().all(|b| b.is_ascii_digit())
        && tail.starts_with(" error")
}

fn format_gates(g: &GateResult, require_backup: bool) -> (String, Vec<&'static str>) {
    let mut bad = Vec::new();
    if require_backup && !g.backup {
        bad.push("backup");
    }
    if g.zig_block > 0 {
        bad.push("zig{}");
    }
    if g.var_decl > 0 {
        bad.push("var");
    }
    if g.tuple_at_n > 0 {
        bad.push("tuple.@N");
    }
    if g.anyopaque > 0 {
        bad.push("anyopaque");
    }
    if g.ratio_ok == Some(false) {
        bad.push("ratio_ok");
    }

    let mut dict = format!(
        "{{'backup': {}, 'zig{{}}': {}, 'var': {}, 'tuple.@N': {}, 'anyopaque': {}",
        if g.backup { "True" } else { "False" },
        g.zig_block,
        g.var_decl,
        g.tuple_at_n,
        g.anyopaque
    );
    if let (Some(r), Some(ok)) = (g.ratio, g.ratio_ok) {
        dict.push_str(&format!(
            ", 'ratio': {}, 'ratio_ok': {}",
            r,
            if ok { "True" } else { "False" }
        ));
    }
    dict.push('}');

    let suffix = if bad.is_empty() {
        "  OK".to_string()
    } else {
        let items: Vec<String> = bad.iter().map(|k| format!("'{}'", k)).collect();
        format!("  VIOLATIONS=[{}]", items.join(", "))
    };

    (format!("   gates: {}{}", dict, suffix), bad)
}

pub fn run_brief(opts: &BriefOptions) -> anyhow::Result<bool> {
    let zig_bin = resolve_zig_bin(opts.zig.as_deref());
    let mut any_failed = false;

    for cp_arg in &opts.files {
        let raw = cp_arg.to_string_lossy();
        let stem = raw.strip_suffix(".cp").unwrap_or(&raw);
        let cp = PathBuf::from(format!("{}.cp", stem));
        let zig = PathBuf::from(format!("{}.zig", stem));
        let bak = PathBuf::from(format!("{}_.zig", stem));

        if !cp.exists() {
            println!("== {}: FAIL(not found)", cp.display());
            any_failed = true;
            continue;
        }

        let code = match fs::read_to_string(&cp) {
            Ok(c) => c,
            Err(e) => {
                println!("== {}: FAIL(read error: {})", cp.display(), e);
                any_failed = true;
                continue;
            }
        };
        let src_lines: Vec<&str> = code.lines().collect();
        let zig_existed = zig.exists();

        // Step 1: Full syntax & semantic check via internal diagnostic engine
        let mut parser = CPParser::new(&code);
        let (mut module, mut all_diags) = parser.parse_module_with_recovery();
        let mut analyzer = Analyzer::new();
        let _ = analyzer.analyze_module(&mut module);
        all_diags.extend(analyzer.get_diagnostics());
        crate::diagnostic::mark_cascades(&mut all_diags, Some(&code));

        let mut errs: Vec<BriefErr> = Vec::new();
        let has_root = all_diags.iter().any(|d| d.cascade_of.is_none());
        for d in &all_diags {
            if has_root && d.cascade_of.is_some() {
                continue;
            }
            let dj = d.to_diagnostic_json(Some(&code), Some(&cp));
            let suggestion = dj
                .suggestion
                .filter(|s| !s.is_empty())
                .or_else(|| dj.help.filter(|s| !s.is_empty()))
                .unwrap_or_default();
            errs.push(BriefErr {
                code: dj.code,
                msg: dj.message,
                line: dj.line,
                suggestion,
            });
        }

        // Step 2: --lib codegen + zig ast-check if check passed
        let mut status = "PASS";
        let mut ast_check_lines: Vec<String> = Vec::new();
        let mut ast_check_total = 0usize;

        if !errs.is_empty() {
            status = "FAIL(cpc)";
        } else {
            let cp_clone = cp.clone();
            let code_clone = code.clone();
            let cg_res = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                let mut codegen = Codegen::new();
                codegen.is_beam_mode = false;
                codegen.is_lib_mode = true;
                codegen.is_fuzz_mode = false;
                codegen.fuzz_target_filter = None;
                codegen.cap_hub_prelude = None;
                codegen.source_dir = cp_clone.parent().map(|p| p.to_path_buf());
                codegen.init_source(Some(&cp_clone.display().to_string()), Some(&code_clone));
                codegen.generate_module(&module)
            }));

            match cg_res {
                Err(err) => {
                    status = "FAIL(codegen)";
                    let msg = if let Some(s) = err.downcast_ref::<&str>() {
                        (*s).to_string()
                    } else if let Some(s) = err.downcast_ref::<String>() {
                        s.clone()
                    } else {
                        "Internal compiler error".to_string()
                    };
                    errs.push(BriefErr {
                        code: "E_CODEGEN".to_string(),
                        msg,
                        line: 0,
                        suggestion: String::new(),
                    });
                }
                Ok(zig_code) => {
                    let mut need_write = true;
                    if let Ok(existing) = fs::read_to_string(&zig) {
                        if existing == zig_code {
                            need_write = false;
                        }
                    }
                    if need_write {
                        if let Err(e) = fs::write(&zig, &zig_code) {
                            status = "FAIL(codegen)";
                            errs.push(BriefErr {
                                code: "E_IO".to_string(),
                                msg: format!("failed to write {}: {}", zig.display(), e),
                                line: 0,
                                suggestion: String::new(),
                            });
                        }
                    }

                    if status == "PASS" {
                        match Command::new(&zig_bin).arg("ast-check").arg(&zig).output() {
                            Err(e) => {
                                status = "FAIL(ast-check)";
                                ast_check_lines.push(format!(
                                    "failed to execute '{}' ast-check: {}",
                                    zig_bin.display(),
                                    e
                                ));
                                ast_check_total = 1;
                            }
                            Ok(out) => {
                                if !out.status.success() {
                                    status = "FAIL(ast-check)";
                                    let combined = format!(
                                        "{}\n{}",
                                        String::from_utf8_lossy(&out.stderr),
                                        String::from_utf8_lossy(&out.stdout)
                                    );
                                    let matched: Vec<String> = combined
                                        .lines()
                                        .filter(|l| is_zig_ast_error_line(l))
                                        .map(|l| l.to_string())
                                        .collect();
                                    if !matched.is_empty() {
                                        ast_check_total = matched.len();
                                        ast_check_lines =
                                            matched.into_iter().take(opts.max).collect();
                                    } else {
                                        let fallback: Vec<String> = combined
                                            .lines()
                                            .map(|l| l.trim())
                                            .filter(|l| !l.is_empty())
                                            .map(|l| l.to_string())
                                            .collect();
                                        ast_check_total = fallback.len();
                                        ast_check_lines =
                                            fallback.into_iter().take(opts.max).collect();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Cleanup newly created .zig / .zig.map unless --keep-zig
        let map_path = PathBuf::from(format!("{}.zig.map", stem));
        if map_path.exists() {
            let _ = fs::remove_file(&map_path);
        }
        if !opts.keep_zig && !zig_existed && zig.exists() {
            let _ = fs::remove_file(&zig);
        }

        // Step 4: Print compact summary
        println!("== {}: {}", cp.display(), status);

        if !ast_check_lines.is_empty() {
            for l in &ast_check_lines {
                println!("    [ast-check] {}", truncate_chars(l, 180));
            }
            println!("    total ast-check errors: {}", ast_check_total);
        }

        if !errs.is_empty() {
            let mut cnt: BTreeMap<&str, usize> = BTreeMap::new();
            for e in &errs {
                *cnt.entry(e.code.as_str()).or_insert(0) += 1;
            }
            let mut sorted_cnt: Vec<(&str, usize)> = cnt.into_iter().collect();
            sorted_cnt.sort_by(|a, b| b.1.cmp(&a.1).then_with(|| a.0.cmp(b.0)));
            let by_code: Vec<String> = sorted_cnt
                .into_iter()
                .map(|(k, v)| format!("{} x{}", k, v))
                .collect();
            println!("   total={} by code: {}", errs.len(), by_code.join(", "));

            let mut seen: HashSet<(String, String)> = HashSet::new();
            let mut shown = 0usize;
            for e in &errs {
                let key = (e.code.clone(), truncate_chars(&e.msg, 60).to_string());
                if !seen.insert(key) {
                    continue;
                }
                let src_line = if e.line > 0 && e.line <= src_lines.len() {
                    truncate_chars(src_lines[e.line - 1].trim(), 100)
                } else {
                    ""
                };
                println!("   {} L{}: {}", e.code, e.line, truncate_chars(&e.msg, 100));
                if !src_line.is_empty() {
                    println!("        | {}", src_line);
                }
                if !e.suggestion.is_empty() {
                    println!(
                        "        -> suggestion: {}",
                        truncate_chars(&e.suggestion, 100)
                    );
                }
                shown += 1;
                if shown >= opts.max {
                    break;
                }
            }
        }

        // Step 3: Static purity gates
        let g = scan_gates(&code, &bak);
        let (gates_line, bad) = format_gates(&g, opts.require_backup);
        println!("{}", gates_line);

        if status != "PASS" || !bad.is_empty() {
            any_failed = true;
        }
    }

    Ok(!any_failed)
}
