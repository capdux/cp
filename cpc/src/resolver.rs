use crate::ast::{Decl, Expr, Lit, Module, ModuleItem, Pattern, Stmt};
use crate::parser::Parser as CPParser;
use std::collections::{BTreeMap, BTreeSet, HashSet};
use std::fs;
use std::path::{Path, PathBuf};

pub fn is_cap_hub_source(source: &str) -> bool {
    source == "cap" || source == "cap:core"
}

pub fn resolve_cap_hub_target(ns: &str) -> Option<String> {
    if ns.is_empty()
        || ns.starts_with('_')
        || !ns.bytes().all(|b| b.is_ascii_alphanumeric() || b == b'_')
    {
        return None;
    }
    // 优先命中真实独立标准库目录或单文件（如 lib/paths/index.cp、lib/collections/index.cp）
    let dir_index = format!("{}/index.cp", ns);
    if find_lib_file(&dir_index, &[]).is_some() {
        return Some(dir_index);
    }
    let flat_file = format!("{}.cp", ns);
    if find_lib_file(&flat_file, &[]).is_some() {
        return Some(flat_file);
    }
    // 真实目录不存在时回退兼容历史别名
    let aliased = match ns {
        "strings" | "string" => Some("string/index.cp"),
        "paths" => Some("path/index.cp"),
        "bits" => Some("meta/bits.cp"),
        "pkg" | "install" => Some("pkg/index.cp"),
        "Semver" => Some("semver/index.cp"),
        "col" | "collections" | "bit_set" => Some("col/index.cp"),
        "encoding" | "base64" => Some("encoding/index.cp"),
        "hash" => Some("hash/index.cp"),
        "fs" => Some("fs.cp"),
        "test" => Some("test.cp"),
        _ => None,
    };
    aliased.map(|target| target.to_string())
}

pub fn find_lib_file(rel_path: &str, include_paths: &[PathBuf]) -> Option<PathBuf> {
    let mut dirs: Vec<PathBuf> = include_paths.to_vec();
    dirs.push(PathBuf::from("/data/dev/cp/lib"));
    dirs.push(PathBuf::from("lib"));
    dirs.push(PathBuf::from("/root/.cap/lib"));

    for dir in dirs {
        let base = if dir.is_file() && dir.file_name().map_or(false, |n| n == "cap.json") {
            dir.parent().unwrap_or(Path::new(".")).to_path_buf()
        } else {
            dir
        };
        let candidate = base.join(rel_path);
        if candidate.is_file() {
            return Some(fs::canonicalize(&candidate).unwrap_or(candidate));
        }
    }
    None
}

pub fn find_cap_core_file(include_paths: &[PathBuf]) -> Option<PathBuf> {
    find_lib_file("_cap/core.cp", include_paths)
}

pub fn is_at_import_cap_hub(expr: &Expr) -> bool {
    if let Expr::Builtin(b) = expr {
        if b.name == "@import" && b.args.len() == 1 {
            if let Expr::Lit(Lit::Str(s)) = &b.args[0] {
                return is_cap_hub_source(s);
            }
        }
    }
    false
}

pub fn extract_cap_hub_aliases(module: &Module) -> (bool, Vec<String>, Vec<String>) {
    let mut uses_cap_hub = false;
    let mut aliases = Vec::new();
    let mut named_specs = Vec::new();

    for item in &module.body {
        match item {
            ModuleItem::Import(imp) if is_cap_hub_source(&imp.source) => {
                uses_cap_hub = true;
                if let Some(def) = &imp.default_specifier {
                    if !aliases.contains(&def.sym) {
                        aliases.push(def.sym.clone());
                    }
                }
                for sp in &imp.specifiers {
                    if !named_specs.contains(&sp.sym) {
                        named_specs.push(sp.sym.clone());
                    }
                }
            }
            ModuleItem::Stmt(Stmt::Var(v)) => {
                if let Some(init) = &v.init {
                    if is_at_import_cap_hub(init) {
                        uses_cap_hub = true;
                        if let Pattern::Ident(id) = &v.pat {
                            if !aliases.contains(&id.sym) {
                                aliases.push(id.sym.clone());
                            }
                        }
                    }
                }
            }
            _ => {}
        }
    }

    (uses_cap_hub, aliases, named_specs)
}

pub fn scan_cap_hub_members(source: &str, aliases: &[String]) -> BTreeSet<String> {
    let mut members = BTreeSet::new();
    if aliases.is_empty() {
        return members;
    }
    let bytes = source.as_bytes();
    let len = bytes.len();
    let mut i = 0usize;

    let is_ident_start = |b: u8| b.is_ascii_alphabetic() || b == b'_';
    let is_ident_char = |b: u8| b.is_ascii_alphanumeric() || b == b'_';

    while i < len {
        let b = bytes[i];
        // Skip line comment //
        if b == b'/' && i + 1 < len && bytes[i + 1] == b'/' {
            i += 2;
            while i < len && bytes[i] != b'\n' {
                i += 1;
            }
            continue;
        }
        // Skip block comment /* ... */
        if b == b'/' && i + 1 < len && bytes[i + 1] == b'*' {
            i += 2;
            while i + 1 < len && !(bytes[i] == b'*' && bytes[i + 1] == b'/') {
                i += 1;
            }
            i = (i + 2).min(len);
            continue;
        }
        // Skip double-quoted string literal
        if b == b'"' {
            i += 1;
            while i < len {
                if bytes[i] == b'\\' {
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
        // Skip backtick template literal
        if b == b'`' {
            i += 1;
            while i < len {
                if bytes[i] == b'\\' {
                    i += 2;
                    continue;
                }
                if bytes[i] == b'`' {
                    i += 1;
                    break;
                }
                i += 1;
            }
            continue;
        }
        // Skip char literal '...'
        if b == b'\'' {
            i += 1;
            while i < len {
                if bytes[i] == b'\\' {
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

        if is_ident_start(b) {
            let prev_is_ident_or_dot =
                i > 0 && (is_ident_char(bytes[i - 1]) || bytes[i - 1] == b'.');
            let start = i;
            i += 1;
            while i < len && is_ident_char(bytes[i]) {
                i += 1;
            }
            if !prev_is_ident_or_dot {
                let ident = &source[start..i];
                if aliases.iter().any(|a| a == ident) {
                    let mut j = i;
                    while j < len && bytes[j].is_ascii_whitespace() {
                        j += 1;
                    }
                    if j < len && bytes[j] == b'.' {
                        j += 1;
                        while j < len && bytes[j].is_ascii_whitespace() {
                            j += 1;
                        }
                        if j < len && is_ident_start(bytes[j]) {
                            let m_start = j;
                            j += 1;
                            while j < len && is_ident_char(bytes[j]) {
                                j += 1;
                            }
                            let member = &source[m_start..j];
                            if resolve_cap_hub_target(member).is_some() {
                                members.insert(member.to_string());
                            }
                        }
                    }
                }
            }
            continue;
        }

        i += 1;
    }

    members
}

#[derive(Debug, Default)]
pub struct CapHubResolution {
    pub uses_cap_hub: bool,
    pub aliases: BTreeSet<String>,
    pub mounted_ns: BTreeMap<String, PathBuf>,
}

pub fn extract_top_level_zig_pub_names(zig_code: &str) -> Vec<String> {
    let mut out = Vec::new();
    let mut brace_depth: usize = 0;

    for line in zig_code.lines() {
        let trimmed = line.trim_start();
        if brace_depth == 0 {
            if let Some(stripped) = trimmed.strip_prefix("pub ") {
                let stripped = stripped.trim_start();
                let rest_opt = stripped
                    .strip_prefix("const ")
                    .or_else(|| stripped.strip_prefix("var "))
                    .or_else(|| stripped.strip_prefix("inline fn "))
                    .or_else(|| stripped.strip_prefix("noinline fn "))
                    .or_else(|| stripped.strip_prefix("fn "))
                    .or_else(|| stripped.strip_prefix("extern fn "))
                    .or_else(|| stripped.strip_prefix("extern \"c\" fn "));
                if let Some(rest) = rest_opt {
                    if let Some(name) = rest.split(&[' ', ':', '=', '(', '<'][..]).next() {
                        let name = name.trim();
                        if !name.is_empty() && !out.contains(&name.to_string()) {
                            out.push(name.to_string());
                        }
                    }
                }
            }
        }

        // Update brace_depth across the line while skipping // comments and "..." / '...' literals
        let bytes = line.as_bytes();
        let mut i = 0usize;
        while i < bytes.len() {
            let b = bytes[i];
            if b == b'/' && i + 1 < bytes.len() && bytes[i + 1] == b'/' {
                break;
            }
            if b == b'"' || b == b'\'' {
                let q = b;
                i += 1;
                while i < bytes.len() {
                    if bytes[i] == b'\\' {
                        i += 2;
                        continue;
                    }
                    if bytes[i] == q {
                        i += 1;
                        break;
                    }
                    i += 1;
                }
                continue;
            }
            if b == b'{' {
                brace_depth += 1;
            } else if b == b'}' {
                brace_depth = brace_depth.saturating_sub(1);
            }
            i += 1;
        }
    }
    out
}

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
            "sync" => "sync".to_string(),
            "shell" | "sh" => "shell".to_string(),
            "image" | "img" | "png" => "image".to_string(),
            "blob" | "file" => "blob".to_string(),
            "dotenv" => "dotenv".to_string(),
            "watcher" | "watch" => "watcher".to_string(),
            "redis" | "valkey" => "redis".to_string(),
            "conf" => "conf".to_string(),
            "pack" | "compress" => "pack".to_string(),
            "sys" | "system" => "sys".to_string(),
            "bigint" | "big_int" => "bigint".to_string(),
            "simd" => "simd".to_string(),
            "css" => "css".to_string(),
            other => other.to_string(),
        };
        true
    } else if let Some(stripped) = search_source.strip_prefix("std/") {
        search_source = match stripped {
            "col" | "collections" => "col".to_string(),
            "event" | "events" | "event_loop" => "event".to_string(),
            "sync" => "sync".to_string(),
            "shell" | "sh" => "shell".to_string(),
            "image" | "img" | "png" => "image".to_string(),
            "blob" | "file" => "blob".to_string(),
            "dotenv" => "dotenv".to_string(),
            "watcher" | "watch" => "watcher".to_string(),
            "redis" | "valkey" => "redis".to_string(),
            "conf" => "conf".to_string(),
            "pack" | "compress" => "pack".to_string(),
            "sys" | "system" => "sys".to_string(),
            "bigint" | "big_int" => "bigint".to_string(),
            "simd" => "simd".to_string(),
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
    let mut cap_hub = CapHubResolution::default();
    resolve_imports_with_cap_hub(
        path,
        include_paths,
        all_items,
        processed,
        is_root,
        &mut cap_hub,
    )
}

pub fn resolve_imports_with_cap_hub(
    path: &Path,
    include_paths: &[PathBuf],
    all_items: &mut Vec<ModuleItem>,
    processed: &mut HashSet<PathBuf>,
    _is_root: bool,
    cap_hub: &mut CapHubResolution,
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

    let (uses_hub, aliases, named_specs) = extract_cap_hub_aliases(&module);
    if uses_hub {
        cap_hub.uses_cap_hub = true;
        for a in &aliases {
            cap_hub.aliases.insert(a.clone());
        }
        let mut members = scan_cap_hub_members(&code, &aliases);
        for sp in named_specs {
            if resolve_cap_hub_target(&sp).is_some() {
                members.insert(sp);
            }
        }
        for ns in members {
            if let Some(rel_cp) = resolve_cap_hub_target(&ns) {
                if let Some(sub_abs) = find_lib_file(&rel_cp, include_paths) {
                    cap_hub.mounted_ns.entry(ns).or_insert(sub_abs);
                }
            }
        }
    }

    for item in module.body {
        match item {
            ModuleItem::Import(imp) => {
                if is_cap_hub_source(&imp.source) {
                    all_items.push(ModuleItem::Import(imp));
                } else if let Some(p) = resolve_source_path(path, &imp.source, include_paths) {
                    resolve_imports_with_cap_hub(
                        &p,
                        include_paths,
                        all_items,
                        processed,
                        false,
                        cap_hub,
                    )?;
                    all_items.push(ModuleItem::Import(imp));
                } else {
                    all_items.push(ModuleItem::Import(imp));
                }
            }
            ModuleItem::Export(exp) => {
                if let Some(source) = &exp.source {
                    if let Some(p) = resolve_source_path(path, source, include_paths) {
                        resolve_imports_with_cap_hub(
                            &p,
                            include_paths,
                            all_items,
                            processed,
                            false,
                            cap_hub,
                        )?;
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
