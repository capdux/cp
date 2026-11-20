use crate::ast::{Decl, ModuleItem};
use crate::parser::Parser as CPParser;
use std::collections::HashSet;
use std::fs;
use std::path::{Path, PathBuf};

pub fn resolve_file_or_dir(base: &Path) -> Option<PathBuf> {
    if base.is_dir() {
        // 1. Check for pre-compiled types in bin/ (for dependencies)
        if let Some(name) = base.file_name().and_then(|n| n.to_str()) {
            let bin_dcp = base.join("bin").join(format!("{}.d.cp", name));
            if bin_dcp.exists() {
                return Some(bin_dcp);
            }
        }

        // 2. Check directory defaults
        let checks = vec!["index.d.cp", "index.cp", "main.d.cp", "main.cp"];
        for check in checks {
            let p = base.join(check);
            if p.exists() {
                return Some(p);
            }
        }

        // 3. Check cap.json main
        let cap_json = base.join("cap.json");
        if cap_json.exists() {
            if let Ok(content) = fs::read_to_string(&cap_json) {
                if let Ok(json) = serde_json::from_str::<serde_json::Value>(&content) {
                    if let Some(main_file) = json.get("main").and_then(|v| v.as_str()) {
                        let m_path = PathBuf::from(main_file);
                        let m_base = base
                            .join(m_path.parent().unwrap_or(Path::new("")))
                            .join(m_path.file_stem().unwrap_or(std::ffi::OsStr::new("")));

                        let dcp = m_base.with_extension("d.cp");
                        if dcp.exists() {
                            return Some(dcp);
                        }
                        let cp = m_base.with_extension("cp");
                        if cp.exists() {
                            return Some(cp);
                        }
                    }
                }
            }
        }
    }

    // 4. Check <base>.d.cp
    let dcp_file = base.with_extension("d.cp");
    if dcp_file.exists() {
        return Some(dcp_file);
    }
    // 5. Check <base>.cp
    let cp_file = base.with_extension("cp");
    if cp_file.exists() {
        return Some(cp_file);
    }

    None
}

fn resolve_cap_from_manifest(search_source: &str, include_paths: &[PathBuf]) -> Option<PathBuf> {
    let mut check_dirs = Vec::new();
    for inc in include_paths {
        check_dirs.push(inc.clone());
    }
    check_dirs.push(PathBuf::from("/data/dev/cp/lib"));
    check_dirs.push(PathBuf::from("lib"));
    check_dirs.push(PathBuf::from("/root/.cap/lib"));

    for dir in check_dirs {
        let manifest_path = if dir.is_file() && dir.file_name().map_or(false, |n| n == "cap.json") {
            dir.clone()
        } else {
            dir.join("cap.json")
        };

        if !manifest_path.exists() {
            continue;
        }

        let base_dir = manifest_path.parent().unwrap_or(Path::new("."));

        if let Ok(content) = fs::read_to_string(&manifest_path) {
            if let Ok(json) = serde_json::from_str::<serde_json::Value>(&content) {
                if let Some(lib_obj) = json.get("lib").and_then(|v| v.as_object()) {
                    if let Some(entry) = lib_obj.get(search_source) {
                        if let Some(path_val) = entry.get("path").and_then(|v| v.as_str()) {
                            let p = PathBuf::from(path_val);
                            let target = if p.is_absolute() { p } else { base_dir.join(p) };

                            // Point 4: direct file resolution (e.g. "http/xxx.cp")
                            if target.is_file() {
                                return Some(target);
                            }
                            let cp_target = target.with_extension("cp");
                            if cp_target.is_file() {
                                return Some(cp_target);
                            }

                            // Point 3: directory resolution
                            if target.is_dir() {
                                // 1. index.d.cp / index.cp / main.d.cp / main.cp
                                for f in &["index.d.cp", "index.cp", "main.d.cp", "main.cp"] {
                                    let check = target.join(f);
                                    if check.is_file() {
                                        return Some(check);
                                    }
                                }
                                // 2. same name as path dir (.cp / .d.cp)
                                if let Some(dir_name) = target.file_name().and_then(|n| n.to_str())
                                {
                                    let check_dcp = target.join(format!("{}.d.cp", dir_name));
                                    if check_dcp.is_file() {
                                        return Some(check_dcp);
                                    }
                                    let check_cp = target.join(format!("{}.cp", dir_name));
                                    if check_cp.is_file() {
                                        return Some(check_cp);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    None
}

pub fn resolve_source_path(
    path: &Path,
    source: &str,
    include_paths: &[PathBuf],
) -> Option<PathBuf> {
    let mut search_source = source.to_string();
    let is_cap_builtin = if let Some(stripped) = search_source.strip_prefix("cap:") {
        search_source = match stripped {
            "col" | "collections" => "col".to_string(),
            "event" | "events" | "event_loop" => "event".to_string(),
            "sync" | "threading" => "sync".to_string(),
            "shell" | "sh" => "shell".to_string(),
            "image" | "img" | "png" => "image".to_string(),
            "blob" | "file" => "blob".to_string(),
            "dotenv" => "dotenv".to_string(),
            "watcher" | "watch" => "watcher".to_string(),
            "redis" | "valkey" => "redis".to_string(),
            "conf" | "ini" => "conf".to_string(),
            "pack" | "compress" => "pack".to_string(),
            "sys" | "system" => "sys".to_string(),
            "bigint" | "big_int" => "bigint".to_string(),
            "simd" | "vector" => "simd".to_string(),
            "css" => "css".to_string(),
            other => other.to_string(),
        };
        true
    } else if let Some(stripped) = search_source.strip_prefix("std/") {
        search_source = match stripped {
            "col" | "collections" => "col".to_string(),
            "event" | "events" | "event_loop" => "event".to_string(),
            "sync" | "threading" => "sync".to_string(),
            "shell" | "sh" => "shell".to_string(),
            "image" | "img" | "png" => "image".to_string(),
            "blob" | "file" => "blob".to_string(),
            "dotenv" => "dotenv".to_string(),
            "watcher" | "watch" => "watcher".to_string(),
            "redis" | "valkey" => "redis".to_string(),
            "conf" | "ini" => "conf".to_string(),
            "pack" | "compress" => "pack".to_string(),
            "sys" | "system" => "sys".to_string(),
            "bigint" | "big_int" => "bigint".to_string(),
            "simd" | "vector" => "simd".to_string(),
            "css" => "css".to_string(),
            other => other.to_string(),
        };
        true
    } else {
        false
    };

    let mut resolved_path = None;

    if is_cap_builtin {
        if let Some(res) = resolve_cap_from_manifest(&search_source, include_paths) {
            resolved_path = Some(res);
        }

        if resolved_path.is_none() {
            for inc in include_paths {
                let p = inc.join(&search_source);
                if let Some(res) = resolve_file_or_dir(&p) {
                    resolved_path = Some(res);
                    break;
                }
            }
        }
        if resolved_path.is_none() {
            let default_lib = PathBuf::from("/data/dev/cp/lib").join(&search_source);
            if let Some(res) = resolve_file_or_dir(&default_lib) {
                resolved_path = Some(res);
            }
        }
    } else if search_source.starts_with("./")
        || search_source.starts_with("../")
        || search_source.starts_with("/")
    {
        let base_path = path.parent().unwrap().join(&search_source);
        resolved_path = resolve_file_or_dir(&base_path);

        if resolved_path.is_none() {
            let remapped = search_source
                .replace("lib/event_loop", "lib/event")
                .replace("lib/collections", "lib/col")
                .replace("lib/threading", "lib/sync")
                .replace("lib/compress", "lib/pack")
                .replace("lib/events.cp", "lib/event/emitter.cp");
            if remapped != search_source {
                let remapped_base = path.parent().unwrap().join(&remapped);
                resolved_path = resolve_file_or_dir(&remapped_base);
            }
        }

        if resolved_path.is_none() {
            for inc in include_paths {
                let p = inc.join(&search_source);
                if let Some(res) = resolve_file_or_dir(&p) {
                    resolved_path = Some(res);
                    break;
                }
            }
        }
    } else {
        let mut current_dir = path.parent().unwrap();
        loop {
            let dep_path = current_dir.join("cap_modules").join(&search_source);
            if let Some(p) = resolve_file_or_dir(&dep_path) {
                resolved_path = Some(p);
                break;
            }
            if let Some(parent) = current_dir.parent() {
                current_dir = parent;
            } else {
                break;
            }
        }

        if resolved_path.is_none() {
            let home = std::env::var("HOME")
                .or_else(|_| std::env::var("USERPROFILE"))
                .unwrap_or_else(|_| String::from("~"));
            let global_path = PathBuf::from(&home)
                .join(".cap/global/cap_modules")
                .join(&search_source);
            resolved_path = resolve_file_or_dir(&global_path);
        }
    }

    resolved_path
}

pub fn resolve_dependencies(
    path: &Path,
    include_paths: &[PathBuf],
    all_items: &mut Vec<ModuleItem>,
    processed: &mut HashSet<PathBuf>,
    is_root: bool,
) -> anyhow::Result<()> {
    let abs_path = fs::canonicalize(path)?;
    processed.insert(abs_path.clone());

    let code = fs::read_to_string(path)?;
    let mut parser = CPParser::new(&code);
    let (module, p_diags) = parser.parse_module_with_recovery();
    if !p_diags.is_empty() {
        let rendered = p_diags
            .iter()
            .map(|d| d.render(Some(&code), Some(path)))
            .collect::<Vec<_>>()
            .join("\n");
        return Err(anyhow::anyhow!("{}", rendered));
    }

    for item in module.body {
        match item {
            ModuleItem::Import(imp) => {
                if let Some(p) = resolve_source_path(path, &imp.source, include_paths) {
                    resolve_dependencies(&p, include_paths, all_items, processed, false)?;
                }
            }
            ModuleItem::Export(exp) => {
                if let Some(source) = &exp.source {
                    if let Some(p) = resolve_source_path(path, source, include_paths) {
                        resolve_dependencies(&p, include_paths, all_items, processed, false)?;
                    }
                }
            }
            _ => {}
        }
    }

    Ok(())
}

pub fn resolve_imports(
    path: &Path,
    include_paths: &[PathBuf],
    all_items: &mut Vec<ModuleItem>,
    processed: &mut HashSet<PathBuf>,
    is_root: bool,
) -> anyhow::Result<()> {
    let abs_path = fs::canonicalize(path)?;
    if processed.contains(&abs_path) {
        return Ok(());
    }
    processed.insert(abs_path.clone());

    let code = fs::read_to_string(path)?;
    let mut parser = CPParser::new(&code);
    let (module, p_diags) = parser.parse_module_with_recovery();
    if !p_diags.is_empty() {
        let rendered = p_diags
            .iter()
            .map(|d| d.render(Some(&code), Some(path)))
            .collect::<Vec<_>>()
            .join("\n");
        return Err(anyhow::anyhow!("{}", rendered));
    }

    for item in module.body {
        match item {
            ModuleItem::Import(imp) => {
                if let Some(p) = resolve_source_path(path, &imp.source, include_paths) {
                    resolve_imports(&p, include_paths, all_items, processed, false)?;
                    all_items.push(ModuleItem::Import(imp));
                } else {
                    all_items.push(ModuleItem::Import(imp));
                }
            }
            ModuleItem::Export(exp) => {
                if let Some(source) = &exp.source {
                    if let Some(p) = resolve_source_path(path, source, include_paths) {
                        resolve_imports(&p, include_paths, all_items, processed, false)?;
                    } else {
                        all_items.push(ModuleItem::Export(exp));
                    }
                } else {
                    all_items.push(ModuleItem::Export(exp));
                }
            }
            _ => {
                all_items.push(item);
            }
        }
    }

    Ok(())
}
