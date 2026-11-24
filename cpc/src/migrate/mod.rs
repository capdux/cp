// `cap from -ts/-rs` Project Migration Engine Orchestrator
// Coordinates 3-tier dependency resolution, recursive dependency conversion,
// TS/Rust structural transpilation, in-process `cpc` parse verification,
// `cap.json` emission, and `migrate.md` report generation.

pub mod deps;
pub mod rs;
pub mod ts;

use std::collections::{HashMap, HashSet};
use std::fs;
use std::path::{Path, PathBuf};

use crate::parser::Parser;
use deps::{
    generate_cap_json, generate_migrate_md, locate_local_dep_source, parse_cargo_toml_deps,
    parse_package_json_deps, resolve_builtin_or_official, DepMapping, DepTier, FileReport,
    ProjectMigrateSummary, SourceLang,
};
use rs::transpile_rs_to_cp;
use ts::transpile_ts_to_cp;

pub struct MigrateOptions {
    pub lang: SourceLang,
    pub src_dir: PathBuf,
    pub out_dir: PathBuf,
    pub recursive_deps: bool,
    pub max_depth: usize,
}

fn sanitize_pkg_dir_name(name: &str) -> String {
    name.trim()
        .trim_start_matches('@')
        .replace('/', "__")
        .replace(' ', "_")
}

fn is_source_file(lang: SourceLang, path: &Path) -> bool {
    let ext = path.extension().and_then(|e| e.to_str()).unwrap_or("");
    let fname = path.file_name().and_then(|n| n.to_str()).unwrap_or("");
    match lang {
        SourceLang::Ts => {
            (ext == "ts" || ext == "tsx" || ext == "mts" || ext == "js" || ext == "mjs")
                && !fname.ends_with(".d.ts")
                && !fname.ends_with(".test.ts")
                && !fname.ends_with(".spec.ts")
        }
        SourceLang::Rs => ext == "rs" && fname != "build.rs",
    }
}

fn should_skip_dir(name: &str) -> bool {
    matches!(
        name,
        "node_modules"
            | "target"
            | ".git"
            | ".cap"
            | "dist"
            | "build"
            | "test"
            | "tests"
            | "benches"
            | "examples"
            | "docs"
            | ".github"
    )
}

fn collect_source_files(lang: SourceLang, root: &Path, cur: &Path, acc: &mut Vec<PathBuf>) {
    let entries = match fs::read_dir(cur) {
        Ok(e) => e,
        Err(_) => return,
    };
    let mut paths: Vec<PathBuf> = entries.filter_map(|e| e.ok().map(|x| x.path())).collect();
    paths.sort();
    for p in paths {
        if p.is_dir() {
            let dname = p.file_name().and_then(|n| n.to_str()).unwrap_or("");
            if should_skip_dir(dname) {
                continue;
            }
            collect_source_files(lang, root, &p, acc);
        } else if is_source_file(lang, &p) {
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

fn migrate_project_recursive(
    lang: SourceLang,
    src_dir: &Path,
    out_dir: &Path,
    root_cap_modules_dir: &Path,
    recursive_deps: bool,
    depth: usize,
    max_depth: usize,
    visited: &mut HashSet<String>,
) -> Result<ProjectMigrateSummary, String> {
    if !src_dir.exists() {
        return Err(format!(
            "Source directory does not exist: {}",
            src_dir.display()
        ));
    }
    fs::create_dir_all(out_dir)
        .map_err(|e| format!("Failed to create output dir {}: {}", out_dir.display(), e))?;

    // 1. Parse manifest & build 3-tier dependency map
    let mut dep_mappings: Vec<DepMapping> = Vec::new();
    let mut dep_lookup: HashMap<String, DepTier> = HashMap::new();
    let mut converted_dep_pkgs: Vec<String> = Vec::new();

    let (project_name, project_version) = match lang {
        SourceLang::Ts => {
            let (name, ver, raw_deps) = parse_package_json_deps(src_dir);
            for (dname, dver, is_dev) in raw_deps {
                let tier = if let Some(t) = resolve_builtin_or_official(lang, &dname) {
                    t
                } else if !is_dev && recursive_deps && depth < max_depth {
                    if let Some(local_dep_src) =
                        locate_local_dep_source(lang, src_dir, &dname, None)
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
            let (name, ver, raw_deps) = parse_cargo_toml_deps(src_dir);
            for (dname, dver, is_dev, explicit_path) in raw_deps {
                let tier = if let Some(t) = resolve_builtin_or_official(lang, &dname) {
                    t
                } else if !is_dev && recursive_deps && depth < max_depth {
                    if let Some(local_dep_src) =
                        locate_local_dep_source(lang, src_dir, &dname, explicit_path.as_deref())
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
    let mut rel_files = Vec::new();
    let scan_root = if src_dir.join("src").is_dir() {
        src_dir.join("src")
    } else {
        src_dir.to_path_buf()
    };
    collect_source_files(lang, src_dir, &scan_root, &mut rel_files);

    let mut file_reports = Vec::new();
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

        let abs_out = out_dir.join(&cp_rel);
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

    let summary = ProjectMigrateSummary {
        project_name,
        version: project_version,
        lang,
        src_dir: src_dir.display().to_string(),
        out_dir: out_dir.display().to_string(),
        deps: dep_mappings,
        files: file_reports,
        converted_dep_pkgs,
    };

    // 3. Emit standalone `cap.json` and `migrate.md`
    let cap_json = generate_cap_json(&summary);
    fs::write(out_dir.join("cap.json"), cap_json)
        .map_err(|e| format!("Failed to write cap.json: {}", e))?;

    let migrate_md = generate_migrate_md(&summary);
    fs::write(out_dir.join("migrate.md"), migrate_md)
        .map_err(|e| format!("Failed to write migrate.md: {}", e))?;

    Ok(summary)
}

pub fn run_migrate(opts: MigrateOptions) -> Result<ProjectMigrateSummary, String> {
    let cap_modules_dir = opts.out_dir.join("cap_modules");
    let mut visited = HashSet::new();
    migrate_project_recursive(
        opts.lang,
        &opts.src_dir,
        &opts.out_dir,
        &cap_modules_dir,
        opts.recursive_deps,
        0,
        opts.max_depth,
        &mut visited,
    )
}
