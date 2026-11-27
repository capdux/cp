// `cap from -ts/-rs` Project Migration Engine Orchestrator
// Coordinates 3-tier dependency resolution, recursive dependency conversion,
// TS/Rust structural transpilation, in-process `cpc` parse verification,
// `cap.json` emission, and `migrate.md` report generation.

pub mod deps;
pub mod registry;
pub mod rs;
pub mod ts;
pub mod ts_test;

use std::collections::{HashMap, HashSet};
use std::fs;
use std::path::{Path, PathBuf};

use crate::diagnostic::{apply_fixes, mark_cascades, Diagnostic};
use crate::parser::Parser;
use crate::sema::Analyzer;
use deps::{
    generate_cap_json, generate_migrate_md, locate_local_dep_source, parse_cargo_toml_deps,
    parse_package_json_deps, resolve_builtin_or_official, DepMapping, DepTier, FileReport,
    FileVerifyReport, ProjectMigrateSummary, SourceLang, VerifySummary,
};
use rs::transpile_rs_to_cp;
use ts::transpile_ts_to_cp;

pub struct MigrateOptions {
    pub lang: SourceLang,
    pub src_dir: PathBuf,
    pub out_dir: PathBuf,
    pub recursive_deps: bool,
    pub max_depth: usize,
    pub verify: bool,
    pub include_tests: bool,
}

fn sanitize_pkg_dir_name(name: &str) -> String {
    name.trim()
        .trim_start_matches('@')
        .replace('/', "__")
        .replace(' ', "_")
}

fn is_source_file(lang: SourceLang, path: &Path, include_tests: bool) -> bool {
    let ext = path.extension().and_then(|e| e.to_str()).unwrap_or("");
    let fname = path.file_name().and_then(|n| n.to_str()).unwrap_or("");
    match lang {
        SourceLang::Ts => {
            let valid_ext =
                ext == "ts" || ext == "tsx" || ext == "mts" || ext == "js" || ext == "mjs";
            if !valid_ext || fname.ends_with(".d.ts") {
                return false;
            }
            if !include_tests
                && (fname.ends_with(".test.ts")
                    || fname.ends_with(".spec.ts")
                    || fname.ends_with(".test.tsx")
                    || fname.ends_with(".spec.tsx"))
            {
                return false;
            }
            true
        }
        SourceLang::Rs => ext == "rs" && fname != "build.rs",
    }
}

fn should_skip_dir(name: &str, include_tests: bool) -> bool {
    if !include_tests && matches!(name, "test" | "tests" | "__tests__") {
        return true;
    }
    matches!(
        name,
        "node_modules"
            | "target"
            | ".git"
            | ".cap"
            | "dist"
            | "build"
            | "benches"
            | "examples"
            | "docs"
            | ".github"
    )
}

fn collect_source_files(
    lang: SourceLang,
    root: &Path,
    cur: &Path,
    include_tests: bool,
    acc: &mut Vec<PathBuf>,
) {
    let entries = match fs::read_dir(cur) {
        Ok(e) => e,
        Err(_) => return,
    };
    let mut paths: Vec<PathBuf> = entries.filter_map(|e| e.ok().map(|x| x.path())).collect();
    paths.sort();
    for p in paths {
        if p.is_dir() {
            let dname = p.file_name().and_then(|n| n.to_str()).unwrap_or("");
            if should_skip_dir(dname, include_tests) {
                continue;
            }
            collect_source_files(lang, root, &p, include_tests, acc);
        } else if is_source_file(lang, &p, include_tests) {
            if let Ok(rel) = p.strip_prefix(root) {
                acc.push(rel.to_path_buf());
            }
        }
    }
}

fn check_cp_parse(cp_code: &str) -> (bool, Option<String>) {
    use std::sync::atomic::Ordering;
    let prev = crate::lexer::QUIET_LEXER.swap(true, Ordering::Relaxed);
    let mut parser = Parser::new(cp_code);
    let res = parser.parse_module();
    crate::lexer::QUIET_LEXER.store(prev, Ordering::Relaxed);
    match res {
        Ok(_) => (true, None),
        Err(e) => {
            let raw = format!("{:?}", e);
            let single_line = raw.lines().next().unwrap_or(&raw);
            let brief: String = single_line.chars().take(120).collect();
            (false, Some(brief))
        }
    }
}

fn collect_cp_diagnostics(cp_code: &str) -> Vec<Diagnostic> {
    use std::sync::atomic::Ordering;
    let prev = crate::lexer::QUIET_LEXER.swap(true, Ordering::Relaxed);
    let mut parser = Parser::new(cp_code);
    let (mut module, mut all_diags) = parser.parse_module_with_recovery();
    let mut analyzer = Analyzer::new();
    let _ = analyzer.analyze_module(&mut module);
    all_diags.extend(analyzer.get_diagnostics());
    crate::lexer::QUIET_LEXER.store(prev, Ordering::Relaxed);
    mark_cascades(&mut all_diags, Some(cp_code));
    all_diags
}

/// 对单个产出的 `.cp` 文件执行 `cpc` 语法/语义检查，并对可自动修复的机械错误
/// 迭代执行 `cpc::diagnostic::apply_fixes` 原地修复并重检（最多 3 轮）。
pub fn verify_and_autofix_cp_file(
    abs_path: &Path,
    cp_rel_path: &str,
) -> Result<(FileVerifyReport, usize, bool, Option<String>), String> {
    let mut code = fs::read_to_string(abs_path).map_err(|e| {
        format!(
            "Failed to read {} for verification: {}",
            abs_path.display(),
            e
        )
    })?;

    let mut diags = collect_cp_diagnostics(&code);
    let initial_errors = diags.len();
    let mut fixes_applied = 0usize;

    if !diags.is_empty() {
        for _ in 0..3 {
            let (fixed_code, applied) = apply_fixes(&code, &diags);
            if applied == 0 || fixed_code == code {
                break;
            }
            fixes_applied += applied;
            code = fixed_code;
            diags = collect_cp_diagnostics(&code);
            if diags.is_empty() {
                break;
            }
        }
        if fixes_applied > 0 {
            fs::write(abs_path, &code)
                .map_err(|e| format!("Failed to write auto-fixed {}: {}", abs_path.display(), e))?;
        }
    }

    let remaining_errors = diags.len();
    let clean = remaining_errors == 0;
    let first_error = diags.first().map(|d| {
        let brief: String = format!("[{}] {}", d.code, d.message)
            .chars()
            .take(120)
            .collect();
        brief
    });
    let cp_lines = code.lines().count();
    let (parse_ok, parse_err) = check_cp_parse(&code);

    Ok((
        FileVerifyReport {
            cp_rel_path: cp_rel_path.to_string(),
            clean,
            initial_errors,
            remaining_errors,
            fixes_applied,
            first_error,
        },
        cp_lines,
        parse_ok,
        parse_err,
    ))
}

/// 对迁移产出的全部 `.cp` 文件运行 `--verify` 串联自愈闭环，并同步更新 `FileReport`。
pub fn verify_migrated_files(
    out_dir: &Path,
    file_reports: &mut [FileReport],
) -> Result<VerifySummary, String> {
    let mut file_results = Vec::with_capacity(file_reports.len());
    let mut clean_files = 0usize;
    let mut auto_fixed_files = 0usize;
    let mut total_fixes_applied = 0usize;

    for report in file_reports.iter_mut() {
        let abs_path = out_dir.join(&report.cp_rel_path);
        let (verify_rep, cp_lines, parse_ok, parse_err) =
            verify_and_autofix_cp_file(&abs_path, &report.cp_rel_path)?;
        report.cp_lines = cp_lines;
        report.parse_ok = parse_ok;
        report.parse_err = parse_err;

        if verify_rep.clean {
            clean_files += 1;
        }
        if verify_rep.fixes_applied > 0 {
            auto_fixed_files += 1;
            total_fixes_applied += verify_rep.fixes_applied;
        }
        file_results.push(verify_rep);
    }

    Ok(VerifySummary {
        total_files: file_results.len(),
        clean_files,
        auto_fixed_files,
        total_fixes_applied,
        file_results,
    })
}

fn migrate_project_recursive(
    lang: SourceLang,
    src_dir: &Path,
    out_dir: &Path,
    root_cap_modules_dir: &Path,
    recursive_deps: bool,
    depth: usize,
    max_depth: usize,
    verify: bool,
    include_tests: bool,
    visited: &mut HashSet<String>,
) -> Result<ProjectMigrateSummary, String> {
    if !src_dir.exists() {
        return Err(format!(
            "Source path does not exist: {}",
            src_dir.display()
        ));
    }

    let is_single_file = src_dir.is_file();
    let explicit_out_file = is_single_file
        && out_dir
            .extension()
            .and_then(|e| e.to_str())
            .map(|e| e == "cp")
            .unwrap_or(false);
    let effective_out_dir: PathBuf = if explicit_out_file {
        out_dir
            .parent()
            .filter(|p| !p.as_os_str().is_empty())
            .unwrap_or_else(|| Path::new("."))
            .to_path_buf()
    } else {
        out_dir.to_path_buf()
    };
    let manifest_dir: &Path = if is_single_file {
        src_dir
            .parent()
            .filter(|p| !p.as_os_str().is_empty())
            .unwrap_or_else(|| Path::new("."))
    } else {
        src_dir
    };

    fs::create_dir_all(&effective_out_dir).map_err(|e| {
        format!(
            "Failed to create output dir {}: {}",
            effective_out_dir.display(),
            e
        )
    })?;

    // 1. Parse manifest & build 3-tier dependency map
    let mut dep_mappings: Vec<DepMapping> = Vec::new();
    let mut dep_lookup: HashMap<String, DepTier> = HashMap::new();
    let mut converted_dep_pkgs: Vec<String> = Vec::new();

    let (project_name, project_version) = match lang {
        SourceLang::Ts => {
            let (name, ver, raw_deps) = parse_package_json_deps(manifest_dir);
            for (dname, dver, is_dev) in raw_deps {
                let tier = if let Some(t) = resolve_builtin_or_official(lang, &dname) {
                    t
                } else if !is_dev && recursive_deps && depth < max_depth {
                    if let Some(local_dep_src) =
                        locate_local_dep_source(lang, manifest_dir, &dname, None)
                    {
                        let safe_name = sanitize_pkg_dir_name(&dname);
                        let dep_out = root_cap_modules_dir.join(&safe_name);
                        if !visited.contains(&dname) {
                            visited.insert(dname.clone());
                            match migrate_project_recursive(
                                lang,
                                &local_dep_src,
                                &dep_out,
                                root_cap_modules_dir,
                                recursive_deps,
                                depth + 1,
                                max_depth,
                                verify,
                                false,
                                visited,
                            ) {
                                Ok(sub) => {
                                    converted_dep_pkgs.push(dname.clone());
                                    DepTier::RecursiveConverted {
                                        pkg_name: safe_name.clone(),
                                        out_rel_dir: format!("cap_modules/{}", safe_name),
                                        src_dir: local_dep_src.display().to_string(),
                                        file_count: sub.files.len(),
                                    }
                                }
                                Err(err) => DepTier::PendingManual {
                                    reason: format!("递归转换失败: {}", err),
                                },
                            }
                        } else {
                            DepTier::RecursiveConverted {
                                pkg_name: safe_name.clone(),
                                out_rel_dir: format!("cap_modules/{}", safe_name),
                                src_dir: local_dep_src.display().to_string(),
                                file_count: 0,
                            }
                        }
                    } else {
                        DepTier::PendingManual {
                            reason: "未在本地 node_modules/workspace 找到源码；安装依赖后重新运行 `cap from -ts` 可自动递归转换".to_string(),
                        }
                    }
                } else {
                    DepTier::PendingManual {
                        reason: if is_dev {
                            "开发期依赖（已由 cap fmt/lint/test/build 工具链替代或按需忽略）"
                                .to_string()
                        } else {
                            "超过递归深度或禁用依赖转换".to_string()
                        },
                    }
                };
                dep_lookup.insert(dname.clone(), tier.clone());
                dep_mappings.push(DepMapping {
                    orig_name: dname,
                    orig_version: dver,
                    is_dev,
                    tier,
                });
            }
            (name, ver)
        }
        SourceLang::Rs => {
            let (name, ver, raw_deps) = parse_cargo_toml_deps(manifest_dir);
            for (dname, dver, is_dev, explicit_path) in raw_deps {
                let tier = if let Some(t) = resolve_builtin_or_official(lang, &dname) {
                    t
                } else if !is_dev && recursive_deps && depth < max_depth {
                    if let Some(local_dep_src) =
                        locate_local_dep_source(lang, manifest_dir, &dname, explicit_path.as_deref())
                    {
                        let safe_name = sanitize_pkg_dir_name(&dname);
                        let dep_out = root_cap_modules_dir.join(&safe_name);
                        if !visited.contains(&dname) {
                            visited.insert(dname.clone());
                            match migrate_project_recursive(
                                lang,
                                &local_dep_src,
                                &dep_out,
                                root_cap_modules_dir,
                                recursive_deps,
                                depth + 1,
                                max_depth,
                                verify,
                                false,
                                visited,
                            ) {
                                Ok(sub) => {
                                    converted_dep_pkgs.push(dname.clone());
                                    DepTier::RecursiveConverted {
                                        pkg_name: safe_name.clone(),
                                        out_rel_dir: format!("cap_modules/{}", safe_name),
                                        src_dir: local_dep_src.display().to_string(),
                                        file_count: sub.files.len(),
                                    }
                                }
                                Err(err) => DepTier::PendingManual {
                                    reason: format!("递归转换失败: {}", err),
                                },
                            }
                        } else {
                            DepTier::RecursiveConverted {
                                pkg_name: safe_name.clone(),
                                out_rel_dir: format!("cap_modules/{}", safe_name),
                                src_dir: local_dep_src.display().to_string(),
                                file_count: 0,
                            }
                        }
                    } else {
                        DepTier::PendingManual {
                            reason: "未在本地 workspace/vendor/cargo 缓存找到源码；执行 `cargo vendor` 后重跑 `cap from -rs` 可自动递归转换".to_string(),
                        }
                    }
                } else {
                    DepTier::PendingManual {
                        reason: if is_dev {
                            "开发/测试依赖（已由 cap test 工具链替代）".to_string()
                        } else {
                            "超过递归深度或禁用依赖转换".to_string()
                        },
                    }
                };
                dep_lookup.insert(dname.clone(), tier.clone());
                dep_mappings.push(DepMapping {
                    orig_name: dname,
                    orig_version: dver,
                    is_dev,
                    tier,
                });
            }
            (name, ver)
        }
    };

    // 2. Collect and transpile all source files preserving directory layout
    let mut file_reports = Vec::new();
    if is_single_file {
        let source = fs::read_to_string(src_dir)
            .map_err(|e| format!("Failed to read {}: {}", src_dir.display(), e))?;
        let orig_lines = source.lines().count();
        let file_name = src_dir
            .file_name()
            .map(PathBuf::from)
            .unwrap_or_else(|| PathBuf::from("index.ts"));
        let rel_str = file_name.to_string_lossy().replace('\\', "/");

        let cp_rel = if explicit_out_file {
            out_dir
                .file_name()
                .map(PathBuf::from)
                .unwrap_or_else(|| PathBuf::from("output.cp"))
        } else {
            let mut p = file_name.clone();
            p.set_extension("cp");
            p
        };
        let cp_rel_str = cp_rel.to_string_lossy().replace('\\', "/");

        let (cp_code, todo_count, opt_notes) = match lang {
            SourceLang::Ts => {
                let res = transpile_ts_to_cp(&source, &rel_str, &dep_lookup);
                (res.cp_code, res.todo_count, res.opt_notes)
            }
            SourceLang::Rs => {
                let res = transpile_rs_to_cp(&source, &rel_str, &dep_lookup);
                (res.cp_code, res.todo_count, res.opt_notes)
            }
        };

        let cp_lines = cp_code.lines().count();
        let (parse_ok, parse_err) = check_cp_parse(&cp_code);

        let abs_out = effective_out_dir.join(&cp_rel);
        if let Some(parent) = abs_out.parent() {
            let _ = fs::create_dir_all(parent);
        }
        fs::write(&abs_out, &cp_code)
            .map_err(|e| format!("Failed to write {}: {}", abs_out.display(), e))?;

        file_reports.push(FileReport {
            orig_rel_path: rel_str,
            cp_rel_path: cp_rel_str,
            orig_lines,
            cp_lines,
            parse_ok,
            parse_err,
            todo_count,
            opt_notes,
        });
    } else {
        let mut rel_files = Vec::new();
        let scan_root = if src_dir.join("src").is_dir() && !include_tests {
            src_dir.join("src")
        } else {
            src_dir.to_path_buf()
        };
        collect_source_files(lang, src_dir, &scan_root, include_tests, &mut rel_files);

        for rel in rel_files {
            let abs_src = src_dir.join(&rel);
            let source = match fs::read_to_string(&abs_src) {
                Ok(s) => s,
                Err(_) => continue,
            };
            let orig_lines = source.lines().count();
            let rel_str = rel.to_string_lossy().replace('\\', "/");

            let mut cp_rel = rel.clone();
            cp_rel.set_extension("cp");
            let cp_rel_str = cp_rel.to_string_lossy().replace('\\', "/");

            let (cp_code, todo_count, opt_notes) = match lang {
                SourceLang::Ts => {
                    let res = transpile_ts_to_cp(&source, &rel_str, &dep_lookup);
                    (res.cp_code, res.todo_count, res.opt_notes)
                }
                SourceLang::Rs => {
                    let res = transpile_rs_to_cp(&source, &rel_str, &dep_lookup);
                    (res.cp_code, res.todo_count, res.opt_notes)
                }
            };

            let cp_lines = cp_code.lines().count();
            let (parse_ok, parse_err) = check_cp_parse(&cp_code);

            let abs_out = effective_out_dir.join(&cp_rel);
            if let Some(parent) = abs_out.parent() {
                let _ = fs::create_dir_all(parent);
            }
            fs::write(&abs_out, &cp_code)
                .map_err(|e| format!("Failed to write {}: {}", abs_out.display(), e))?;

            file_reports.push(FileReport {
                orig_rel_path: rel_str,
                cp_rel_path: cp_rel_str,
                orig_lines,
                cp_lines,
                parse_ok,
                parse_err,
                todo_count,
                opt_notes,
            });
        }
    }

    let verify_summary = if verify {
        Some(verify_migrated_files(&effective_out_dir, &mut file_reports)?)
    } else {
        None
    };

    let summary = ProjectMigrateSummary {
        project_name,
        version: project_version,
        lang,
        src_dir: src_dir.display().to_string(),
        out_dir: effective_out_dir.display().to_string(),
        deps: dep_mappings,
        files: file_reports,
        converted_dep_pkgs,
        verify: verify_summary,
    };

    // 3. Emit standalone `cap.json` and `migrate.md`
    let cap_json = generate_cap_json(&summary);
    fs::write(effective_out_dir.join("cap.json"), cap_json)
        .map_err(|e| format!("Failed to write cap.json: {}", e))?;

    let migrate_md = generate_migrate_md(&summary);
    fs::write(effective_out_dir.join("migrate.md"), migrate_md)
        .map_err(|e| format!("Failed to write migrate.md: {}", e))?;

    Ok(summary)
}

pub fn run_migrate(opts: MigrateOptions) -> Result<ProjectMigrateSummary, String> {
    let cap_modules_dir = if opts.src_dir.is_file()
        && opts
            .out_dir
            .extension()
            .and_then(|e| e.to_str())
            .map(|e| e == "cp")
            .unwrap_or(false)
    {
        opts.out_dir
            .parent()
            .filter(|p| !p.as_os_str().is_empty())
            .unwrap_or_else(|| Path::new("."))
            .join("cap_modules")
    } else {
        opts.out_dir.join("cap_modules")
    };
    let mut visited = HashSet::new();
    migrate_project_recursive(
        opts.lang,
        &opts.src_dir,
        &opts.out_dir,
        &cap_modules_dir,
        opts.recursive_deps,
        0,
        opts.max_depth,
        opts.verify,
        opts.include_tests,
        &mut visited,
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::time::{SystemTime, UNIX_EPOCH};

    fn make_temp_dir(prefix: &str) -> PathBuf {
        let nanos = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_nanos();
        let dir = std::env::temp_dir().join(format!(
            "cpc_migrate_{}_{}_{}",
            prefix,
            std::process::id(),
            nanos
        ));
        fs::create_dir_all(&dir).expect("create temp dir");
        dir
    }

    #[test]
    fn test_verify_summary_format_line() {
        let summary = VerifySummary {
            total_files: 5,
            clean_files: 4,
            auto_fixed_files: 2,
            total_fixes_applied: 3,
            file_results: Vec::new(),
        };
        assert_eq!(summary.format_line(), "verified: 4/5 clean, 2 auto-fixed");
    }

    #[test]
    fn test_verify_and_autofix_clean_file() {
        let dir = make_temp_dir("clean");
        let file = dir.join("clean.cp");
        let code = "export fn add(a: i32, b: i32): i32 {\n    return a + b;\n}\n";
        fs::write(&file, code).unwrap();

        let (rep, lines, parse_ok, parse_err) =
            verify_and_autofix_cp_file(&file, "clean.cp").unwrap();
        assert!(rep.clean, "expected clean file, got {:?}", rep);
        assert_eq!(rep.initial_errors, 0);
        assert_eq!(rep.remaining_errors, 0);
        assert_eq!(rep.fixes_applied, 0);
        assert_eq!(lines, 3);
        assert!(parse_ok);
        assert!(parse_err.is_none());

        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn test_verify_and_autofix_mechanical_errors() {
        let dir = make_temp_dir("autofix");
        let file = dir.join("fixme.cp");
        // Contains two mechanical errors fixable by cpc::diagnostic::apply_fixes:
        // E0103 (`var` -> `let`) and E0101 (missing parens around `if` condition)
        let broken = "export fn clamp(x: i32): i32 {\n    var y: i32 = x;\n    if y > 10 {\n        return 10;\n    }\n    return y;\n}\n";
        fs::write(&file, broken).unwrap();

        let (rep, _lines, parse_ok, _parse_err) =
            verify_and_autofix_cp_file(&file, "fixme.cp").unwrap();
        assert!(
            rep.initial_errors >= 2,
            "expected >= 2 initial mechanical errors, got {:?}",
            rep
        );
        assert!(
            rep.fixes_applied >= 2,
            "expected >= 2 auto-fixes applied, got {:?}",
            rep
        );
        assert!(
            rep.clean,
            "expected file to become clean after auto-fix, got {:?}",
            rep
        );
        assert_eq!(rep.remaining_errors, 0);
        assert!(parse_ok);

        let fixed_on_disk = fs::read_to_string(&file).unwrap();
        assert!(fixed_on_disk.contains("let y: i32 = x;"));
        assert!(fixed_on_disk.contains("if (y > 10)"));

        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn test_run_migrate_with_verify_end_to_end() {
        let root = make_temp_dir("e2e");
        let src_dir = root.join("ts_app");
        let out_dir = root.join("cp_app");
        fs::create_dir_all(src_dir.join("src")).unwrap();
        fs::write(
            src_dir.join("package.json"),
            r#"{"name": "demo-verify", "version": "1.0.0"}"#,
        )
        .unwrap();
        fs::write(
            src_dir.join("src/math.ts"),
            "export function sum(a: number, b: number): number {\n  return a + b;\n}\n",
        )
        .unwrap();

        let summary = run_migrate(MigrateOptions {
            lang: SourceLang::Ts,
            src_dir: src_dir.clone(),
            out_dir: out_dir.clone(),
            recursive_deps: false,
            max_depth: 1,
            verify: true,
            include_tests: false,
        })
        .expect("run_migrate with verify should succeed");

        let v = summary.verify.expect("verify summary should be present");
        assert_eq!(v.total_files, 1);
        assert_eq!(v.clean_files, 1, "verify results: {:?}", v.file_results);
        assert_eq!(
            v.format_line(),
            format!("verified: 1/1 clean, {} auto-fixed", v.auto_fixed_files)
        );
        assert!(out_dir.join("src/math.cp").exists());

        let _ = fs::remove_dir_all(&root);
    }
}
