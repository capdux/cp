use crate::ast::*;
use crate::codegen::Codegen;
use crate::parser::Parser as CPParser;
use crate::resolver::{
    extract_cap_hub_aliases, extract_top_level_zig_pub_names, find_cap_core_file, find_lib_file,
    is_cap_hub_source, is_external_package_source, is_import_used, resolve_cap_hub_target,
    resolve_source_path, scan_cap_hub_members,
};
use crate::sema::Analyzer;
use serde::{Deserialize, Serialize};
use std::collections::{BTreeMap, BTreeSet, HashMap, HashSet, VecDeque};
use std::fs;
use std::io::{BufRead, BufReader};
use std::path::{Component, Path, PathBuf};
use std::time::Instant;

pub struct ModularOptions<'a> {
    pub entry: &'a Path,
    pub emit_dir: &'a Path,
    pub include_paths: &'a [PathBuf],
    pub beam: bool,
    pub lib: bool,
    pub fuzz: bool,
    pub fuzz_target: Option<String>,
    pub force: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct CachedFileScan {
    content_hash: u64,
    raw_imports: Vec<String>,
    c_includes: Vec<String>,
    link_libs: Vec<String>,
    link_paths: Vec<String>,
    exported_symbols: Vec<String>,
    reexport_stars: Vec<String>,
    reexport_named: Vec<(String, Vec<String>)>,
    #[serde(default)]
    uses_cap_hub: bool,
    #[serde(default)]
    cap_hub_members: Vec<String>,
    #[serde(default)]
    used_packages: Vec<String>,
}

#[derive(Debug, Default, Serialize, Deserialize)]
struct IncCacheFile {
    version: u32,
    files: HashMap<String, CachedFileScan>,
}

#[derive(Debug, Clone)]
enum ResolvedTarget {
    Std,
    CapHub,
    CpFile(PathBuf),
    ZigFile(PathBuf),
    External(String),
}

struct ModuleNode {
    abs_path: PathBuf,
    rel_zig_path: PathBuf,
    scan: CachedFileScan,
    resolved_edges: Vec<(String, ResolvedTarget)>,
    inc_hash: String,
    is_dirty: bool,
}

#[derive(Default)]
struct ImportedEnv {
    decls: Vec<Decl>,
    vars: Vec<VarDecl>,
    zig_globals: Vec<String>,
    extra_error_sets: Vec<ErrorSetDecl>,
}

pub fn fnv1a64(bytes: &[u8]) -> u64 {
    let mut hash: u64 = 0xcbf29ce484222325;
    for &b in bytes {
        hash ^= b as u64;
        hash = hash.wrapping_mul(0x100000001b3);
    }
    hash
}

fn fnv1a_update(hash: &mut u64, bytes: &[u8]) {
    for &b in bytes {
        *hash ^= b as u64;
        *hash = hash.wrapping_mul(0x100000001b3);
    }
}

fn path_has_component(path: &Path, name: &str) -> bool {
    path.components()
        .any(|c| matches!(c, Component::Normal(s) if s == name))
}

pub fn detect_project_root(entry_abs: &Path) -> PathBuf {
    let entry_dir = entry_abs.parent().unwrap_or(Path::new("."));
    let mut src_parent: Option<PathBuf> = None;

    let mut cur = Some(entry_dir);
    while let Some(dir) = cur {
        if src_parent.is_none() && dir.file_name().map_or(false, |n| n == "src") {
            if let Some(p) = dir.parent() {
                src_parent = Some(p.to_path_buf());
            }
        }
        let cap_json = dir.join("cap.json");
        if cap_json.exists() {
            if let Ok(rel) = entry_abs.strip_prefix(dir) {
                if !path_has_component(rel, "temp") {
                    return dir.to_path_buf();
                }
            }
        }
        cur = dir.parent();
    }

    if let Some(p) = src_parent {
        return p;
    }
    entry_dir.to_path_buf()
}

fn collect_lib_roots(project_root: &Path, include_paths: &[PathBuf]) -> Vec<PathBuf> {
    let mut roots = Vec::new();
    for inc in include_paths {
        if let Ok(canon) = fs::canonicalize(inc) {
            if canon != project_root && !roots.contains(&canon) {
                roots.push(canon);
            }
        }
    }
    for default_dir in &["/data/dev/cp/lib", "/root/.cap/lib"] {
        if let Ok(canon) = fs::canonicalize(default_dir) {
            if canon != project_root && !roots.contains(&canon) {
                roots.push(canon);
            }
        }
    }
    roots
}

fn replace_cp_ext_with_zig(rel: &Path) -> PathBuf {
    let s = rel.to_string_lossy();
    if let Some(stripped) = s.strip_suffix(".cp") {
        PathBuf::from(format!("{}.zig", stripped))
    } else if s.ends_with(".zig") {
        rel.to_path_buf()
    } else {
        rel.with_extension("zig")
    }
}

pub fn map_module_rel_path(abs_path: &Path, project_root: &Path, lib_roots: &[PathBuf]) -> PathBuf {
    for lib_root in lib_roots {
        if let Ok(rel) = abs_path.strip_prefix(lib_root) {
            return PathBuf::from("_lib").join(replace_cp_ext_with_zig(rel));
        }
    }
    if let Ok(rel) = abs_path.strip_prefix(project_root) {
        return replace_cp_ext_with_zig(rel);
    }
    // Check if inside a cap_modules directory
    let comps: Vec<String> = abs_path
        .components()
        .filter_map(|c| match c {
            Component::Normal(s) => Some(s.to_string_lossy().to_string()),
            _ => None,
        })
        .collect();
    if let Some(pos) = comps.iter().rposition(|c| c == "cap_modules") {
        let mut rel = PathBuf::from("_deps");
        for part in &comps[pos + 1..] {
            rel.push(part);
        }
        return replace_cp_ext_with_zig(&rel);
    }
    let hash_short = fnv1a64(abs_path.to_string_lossy().as_bytes()) & 0xffff_ffff;
    let file_name = abs_path
        .file_name()
        .map(|n| n.to_string_lossy().to_string())
        .unwrap_or_else(|| "mod.cp".to_string());
    replace_cp_ext_with_zig(&PathBuf::from(format!(
        "_ext/{:08x}_{}",
        hash_short, file_name
    )))
}

pub fn compute_zig_import_rel(from_zig_rel: &Path, to_zig_rel: &Path) -> String {
    let from_dir = from_zig_rel.parent().unwrap_or(Path::new(""));
    let from_comps: Vec<_> = from_dir
        .components()
        .filter(|c| matches!(c, Component::Normal(_)))
        .collect();
    let to_comps: Vec<_> = to_zig_rel
        .components()
        .filter(|c| matches!(c, Component::Normal(_)))
        .collect();

    let mut common = 0usize;
    while common < from_comps.len()
        && common < to_comps.len()
        && from_comps[common] == to_comps[common]
    {
        common += 1;
    }

    let mut parts: Vec<String> = Vec::new();
    for _ in common..from_comps.len() {
        parts.push("..".to_string());
    }
    for comp in &to_comps[common..] {
        if let Component::Normal(os) = comp {
            parts.push(os.to_string_lossy().to_string());
        }
    }
    let joined = parts.join("/");
    if joined.starts_with("../") || joined.starts_with("./") {
        joined
    } else {
        format!("./{}", joined)
    }
}

fn resolve_modular_import(
    importer_abs: &Path,
    source: &str,
    include_paths: &[PathBuf],
) -> ResolvedTarget {
    if source == "std" {
        return ResolvedTarget::Std;
    }
    if is_cap_hub_source(source) {
        return ResolvedTarget::CapHub;
    }
    if source.ends_with(".zig")
        && (source.starts_with("./") || source.starts_with("../") || source.starts_with('/'))
    {
        let candidate = importer_abs.parent().unwrap_or(Path::new(".")).join(source);
        if let Ok(canon) = fs::canonicalize(&candidate) {
            return ResolvedTarget::ZigFile(canon);
        }
    }
    if let Some(p) = resolve_source_path(importer_abs, source, include_paths) {
        if let Ok(canon) = fs::canonicalize(&p) {
            return ResolvedTarget::CpFile(canon);
        }
        return ResolvedTarget::CpFile(p);
    }
    ResolvedTarget::External(source.to_string())
}

fn extract_pragma_arg(line: &str, prefix: &str) -> Option<String> {
    let rest = line.strip_prefix(prefix)?;
    let end = rest.find('"')?;
    Some(rest[..end].to_string())
}

fn extract_zig_pub_names(zig_code: &str, out: &mut Vec<String>) {
    for line in zig_code.lines() {
        let trimmed = line.trim();
        let Some(stripped) = trimmed.strip_prefix("pub ") else {
            continue;
        };
        let stripped = stripped.trim_start();
        let rest_opt = stripped
            .strip_prefix("const ")
            .or_else(|| stripped.strip_prefix("var "))
            .or_else(|| stripped.strip_prefix("fn "))
            .or_else(|| stripped.strip_prefix("inline fn "))
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

fn scan_cp_module(path: &Path, raw_bytes: &[u8]) -> anyhow::Result<(CachedFileScan, Module)> {
    let code = String::from_utf8(raw_bytes.to_vec())
        .map_err(|e| anyhow::anyhow!("Invalid UTF-8 in {}: {}", path.display(), e))?;
    let content_hash = fnv1a64(raw_bytes);

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

    let mut raw_imports = Vec::new();
    let mut used_packages = Vec::new();
    let mut c_includes = Vec::new();
    let mut link_libs = Vec::new();
    let mut link_paths = Vec::new();
    let mut exported_symbols = Vec::new();
    let mut reexport_stars = Vec::new();
    let mut reexport_named = Vec::new();
    let referenced_idents = crate::ast_visitor::collect_module_referenced_idents(&module);

    for line in code.lines() {
        let trimmed = line.trim();
        if let Some(comment) = trimmed.strip_prefix("//") {
            let c = comment.trim_start();
            if let Some(p) = extract_pragma_arg(c, "@linkPath(\"") {
                if !link_paths.contains(&p) {
                    link_paths.push(p);
                }
            } else if let Some(l) = extract_pragma_arg(c, "@linkLib(\"")
                .or_else(|| extract_pragma_arg(c, "@linkLibrary(\""))
            {
                if !link_libs.contains(&l) {
                    link_libs.push(l);
                }
            } else if let Some(h) = extract_pragma_arg(c, "@cInclude(\"") {
                if !c_includes.contains(&h) {
                    c_includes.push(h);
                }
            }
        }
    }

    let push_sym = |out: &mut Vec<String>, s: &str| {
        if !s.is_empty() && !out.contains(&s.to_string()) {
            out.push(s.to_string());
        }
    };

    for item in &module.body {
        match item {
            ModuleItem::Import(imp) => {
                if is_external_package_source(&imp.source) {
                    if !is_import_used(imp, &referenced_idents) {
                        continue;
                    }
                    if !used_packages.contains(&imp.source) {
                        used_packages.push(imp.source.clone());
                    }
                } else if imp.source.starts_with("cap:") && is_import_used(imp, &referenced_idents)
                {
                    if !used_packages.contains(&imp.source) {
                        used_packages.push(imp.source.clone());
                    }
                }
                if !raw_imports.contains(&imp.source) {
                    raw_imports.push(imp.source.clone());
                }
            }
            ModuleItem::Export(exp) => {
                if let Some(src) = &exp.source {
                    if is_external_package_source(src) || src.starts_with("cap:") {
                        if !used_packages.contains(src) {
                            used_packages.push(src.clone());
                        }
                    }
                    if !raw_imports.contains(src) {
                        raw_imports.push(src.clone());
                    }
                    if exp.specifiers.is_empty() {
                        if !reexport_stars.contains(src) {
                            reexport_stars.push(src.clone());
                        }
                    } else {
                        let specs: Vec<String> = exp
                            .specifiers
                            .iter()
                            .map(|s| s.exported.sym.clone())
                            .collect();
                        for s in &specs {
                            push_sym(&mut exported_symbols, s);
                        }
                        reexport_named.push((src.clone(), specs));
                    }
                } else {
                    for s in &exp.specifiers {
                        push_sym(&mut exported_symbols, &s.exported.sym);
                    }
                }
            }
            ModuleItem::Decl(decl) => match decl {
                Decl::Func(f) => push_sym(&mut exported_symbols, &f.ident.sym),
                Decl::Class(c) => {
                    if !c.meta.iter().any(|m| m.name.sym == "c") || c.is_pub {
                        push_sym(&mut exported_symbols, &c.ident.sym);
                    }
                }
                Decl::Enum(e) => push_sym(&mut exported_symbols, &e.ident.sym),
                Decl::Trait(t) => push_sym(&mut exported_symbols, &t.ident.sym),
                Decl::ErrorSet(es) => push_sym(&mut exported_symbols, &es.ident.sym),
                Decl::TypeAlias(ta) if ta.is_pub => push_sym(&mut exported_symbols, &ta.ident.sym),
                Decl::CInclude(ci) => {
                    if !c_includes.contains(&ci.header) {
                        c_includes.push(ci.header.clone());
                    }
                }
                _ => {}
            },
            ModuleItem::Stmt(Stmt::Var(v)) => {
                if v.is_pub || v.is_extern {
                    if let Pattern::Ident(id) = &v.pat {
                        push_sym(&mut exported_symbols, &id.sym);
                    }
                }
            }
            ModuleItem::Stmt(Stmt::Expr(Expr::Zig(z))) => {
                extract_zig_pub_names(&z.code, &mut exported_symbols);
            }
            _ => {}
        }
    }

    let (uses_cap_hub, cap_aliases, cap_named_specs) = extract_cap_hub_aliases(&module);
    let mut cap_hub_members: Vec<String> = Vec::new();
    if uses_cap_hub {
        let mut mem_set = scan_cap_hub_members(&code, &cap_aliases);
        for sp in cap_named_specs {
            if resolve_cap_hub_target(&sp).is_some() {
                mem_set.insert(sp);
            }
        }
        cap_hub_members = mem_set.into_iter().collect();
    }

    let scan = CachedFileScan {
        content_hash,
        raw_imports,
        c_includes,
        link_libs,
        link_paths,
        exported_symbols,
        reexport_stars,
        reexport_named,
        uses_cap_hub,
        cap_hub_members,
        used_packages,
    };
    Ok((scan, module))
}

fn read_first_line(path: &Path) -> Option<String> {
    let file = fs::File::open(path).ok()?;
    let mut reader = BufReader::new(file);
    let mut line = String::new();
    reader.read_line(&mut line).ok()?;
    Some(line.trim_end_matches(&['\r', '\n'][..]).to_string())
}

fn check_parallel_cap_json(project_root: &Path) -> bool {
    let p = project_root.join("cap.json");
    if let Ok(content) = fs::read_to_string(p) {
        return content.contains("\"mode\": \"parallel\"")
            || content.contains("\"mode\":\"parallel\"");
    }
    false
}

fn compute_module_inc_hash(
    scan: &CachedFileScan,
    resolved_edges: &[(String, ResolvedTarget)],
    is_entry: bool,
    is_parallel_entry: bool,
    opts: &ModularOptions,
) -> String {
    let mut h: u64 = 0xcbf29ce484222325;
    fnv1a_update(&mut h, b"cpc-modular-v2:");
    fnv1a_update(&mut h, env!("CARGO_PKG_VERSION").as_bytes());
    fnv1a_update(
        &mut h,
        &[
            is_entry as u8,
            is_parallel_entry as u8,
            opts.beam as u8,
            opts.lib as u8,
            opts.fuzz as u8,
        ],
    );
    if let Some(ref ft) = opts.fuzz_target {
        fnv1a_update(&mut h, ft.as_bytes());
    }
    fnv1a_update(&mut h, &scan.content_hash.to_le_bytes());
    for (raw, target) in resolved_edges {
        fnv1a_update(&mut h, raw.as_bytes());
        fnv1a_update(&mut h, b"->");
        match target {
            ResolvedTarget::Std => fnv1a_update(&mut h, b"std"),
            ResolvedTarget::CapHub => fnv1a_update(&mut h, b"cap_hub"),
            ResolvedTarget::CpFile(p) => fnv1a_update(&mut h, p.to_string_lossy().as_bytes()),
            ResolvedTarget::ZigFile(p) => fnv1a_update(&mut h, p.to_string_lossy().as_bytes()),
            ResolvedTarget::External(s) => fnv1a_update(&mut h, s.as_bytes()),
        }
        fnv1a_update(&mut h, b";");
    }
    format!("{:016x}", h)
}

fn collect_transitive_exports(
    mod_path: &Path,
    nodes: &HashMap<PathBuf, ModuleNode>,
    visited: &mut HashSet<PathBuf>,
    out: &mut Vec<String>,
) {
    if !visited.insert(mod_path.to_path_buf()) {
        return;
    }
    let Some(node) = nodes.get(mod_path) else {
        return;
    };
    for sym in &node.scan.exported_symbols {
        if !out.contains(sym) {
            out.push(sym.clone());
        }
    }
    for star_src in &node.scan.reexport_stars {
        if let Some((_, ResolvedTarget::CpFile(target_abs))) =
            node.resolved_edges.iter().find(|(s, _)| s == star_src)
        {
            collect_transitive_exports(target_abs, nodes, visited, out);
        }
    }
}

fn get_or_parse_module<'a>(
    abs_path: &Path,
    parsed_modules: &'a mut HashMap<PathBuf, Module>,
) -> anyhow::Result<&'a Module> {
    if !parsed_modules.contains_key(abs_path) {
        let raw = fs::read(abs_path)?;
        let (_, module) = scan_cp_module(abs_path, &raw)?;
        parsed_modules.insert(abs_path.to_path_buf(), module);
    }
    Ok(parsed_modules.get(abs_path).unwrap())
}

fn collect_exported_env_from_module(
    target_abs: &Path,
    wanted_names: Option<&HashSet<String>>,
    nodes: &HashMap<PathBuf, ModuleNode>,
    parsed_modules: &mut HashMap<PathBuf, Module>,
    visited: &mut HashSet<(PathBuf, Option<Vec<String>>)>,
    env: &mut ImportedEnv,
) -> anyhow::Result<()> {
    let key = (
        target_abs.to_path_buf(),
        wanted_names.map(|w| {
            let mut v: Vec<String> = w.iter().cloned().collect();
            v.sort();
            v
        }),
    );
    if !visited.insert(key) {
        return Ok(());
    }
    let module = get_or_parse_module(target_abs, parsed_modules)?.clone();
    let allow = |name: &str| wanted_names.map_or(true, |w| w.contains(name));

    for item in &module.body {
        match item {
            ModuleItem::Decl(d) => {
                let name_opt = match d {
                    Decl::Func(f) => Some(f.ident.sym.as_str()),
                    Decl::Class(c) => Some(c.ident.sym.as_str()),
                    Decl::Enum(e) => Some(e.ident.sym.as_str()),
                    Decl::Trait(t) => Some(t.ident.sym.as_str()),
                    Decl::ErrorSet(es) => Some(es.ident.sym.as_str()),
                    Decl::TypeAlias(ta) => Some(ta.ident.sym.as_str()),
                    Decl::Dec(dec) => Some(dec.ident.sym.as_str()),
                    Decl::Macro(m) => Some(m.ident.sym.as_str()),
                    Decl::Meta(m) => Some(m.ident.sym.as_str()),
                    _ => None,
                };
                if let Some(n) = name_opt {
                    if allow(n) {
                        env.decls.push(d.clone());
                    } else if let Decl::ErrorSet(es) = d {
                        env.extra_error_sets.push(es.clone());
                    }
                }
            }
            ModuleItem::Stmt(Stmt::Var(v)) => {
                if let Pattern::Ident(id) = &v.pat {
                    if allow(&id.sym) {
                        env.vars.push(v.clone());
                    }
                }
            }
            ModuleItem::Stmt(Stmt::Expr(Expr::Zig(z))) => {
                let mut names = Vec::new();
                extract_zig_pub_names(&z.code, &mut names);
                for n in names {
                    if allow(&n) {
                        env.zig_globals.push(n);
                    }
                }
            }
            ModuleItem::Export(exp) => {
                if let Some(src) = &exp.source {
                    if let Some(node) = nodes.get(target_abs) {
                        if let Some((_, ResolvedTarget::CpFile(sub_abs))) =
                            node.resolved_edges.iter().find(|(s, _)| s == src)
                        {
                            if exp.specifiers.is_empty() {
                                collect_exported_env_from_module(
                                    sub_abs,
                                    wanted_names,
                                    nodes,
                                    parsed_modules,
                                    visited,
                                    env,
                                )?;
                            } else {
                                let mut sub_wanted = HashSet::new();
                                for sp in &exp.specifiers {
                                    if allow(&sp.exported.sym) {
                                        sub_wanted.insert(sp.local.sym.clone());
                                    }
                                }
                                if !sub_wanted.is_empty() {
                                    collect_exported_env_from_module(
                                        sub_abs,
                                        Some(&sub_wanted),
                                        nodes,
                                        parsed_modules,
                                        visited,
                                        env,
                                    )?;
                                }
                            }
                        }
                    }
                }
            }
            _ => {}
        }
    }
    Ok(())
}

fn populate_codegen_from_env(codegen: &mut Codegen, env: &ImportedEnv) {
    for decl in &env.decls {
        match decl {
            Decl::Func(f) => {
                codegen.modular_imported_symbols.insert(f.ident.sym.clone());
                codegen.fn_sigs.insert(f.ident.sym.clone(), f.clone());
            }
            Decl::Class(c) => {
                codegen.modular_imported_symbols.insert(c.ident.sym.clone());
                if (c.is_ref || c.meta.iter().any(|m| m.name.sym == "ref"))
                    && !c.meta.iter().any(|m| m.name.sym == "value")
                {
                    codegen.ref_classes.insert(c.ident.sym.clone());
                }
                if c.is_union || c.meta.iter().any(|m| m.name.sym == "value") {
                    codegen.value_classes.insert(c.ident.sym.clone());
                }
                codegen.class_defs.insert(c.ident.sym.clone(), c.clone());
            }
            Decl::Enum(e) => {
                codegen.modular_imported_symbols.insert(e.ident.sym.clone());
                codegen.enum_defs.insert(e.ident.sym.clone(), e.clone());
            }
            Decl::ErrorSet(es) => {
                codegen
                    .modular_imported_symbols
                    .insert(es.ident.sym.clone());
                codegen
                    .error_sets
                    .insert(es.ident.sym.clone(), es.variants.clone());
            }
            Decl::Trait(t) => {
                codegen.modular_imported_symbols.insert(t.ident.sym.clone());
                codegen.trait_defs.insert(t.ident.sym.clone(), t.clone());
            }
            Decl::TypeAlias(ta) => {
                codegen
                    .modular_imported_symbols
                    .insert(ta.ident.sym.clone());
                codegen
                    .type_aliases
                    .insert(ta.ident.sym.clone(), ta.clone());
            }
            Decl::Dec(d) => {
                codegen.dec_defs.insert(d.ident.sym.clone(), d.clone());
            }
            _ => {}
        }
    }
    for v in &env.vars {
        if let Pattern::Ident(id) = &v.pat {
            codegen.modular_imported_symbols.insert(id.sym.clone());
            if let Some(ty) = &v.ty {
                codegen.var_cp_types.insert(id.sym.clone(), ty.clone());
                codegen.global_var_types.insert(id.sym.clone(), ty.clone());
            }
            if !v.is_extern
                && v.kind == VarDeclKind::Const
                && id
                    .sym
                    .chars()
                    .next()
                    .map_or(false, |c| c.is_ascii_uppercase())
            {
                codegen.value_classes.insert(id.sym.clone());
            }
        }
    }
    for zg in &env.zig_globals {
        codegen.value_classes.insert(zg.clone());
    }
    for es in &env.extra_error_sets {
        codegen
            .error_sets
            .entry(es.ident.sym.clone())
            .or_insert_with(|| es.variants.clone());
    }
}

fn write_if_changed(path: &Path, content: &[u8], force: bool) -> anyhow::Result<bool> {
    if !force {
        if let Ok(existing) = fs::read(path) {
            if existing == content {
                return Ok(false);
            }
        }
    }
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    fs::write(path, content)?;
    Ok(true)
}

pub fn compile_modular(opts: &ModularOptions) -> anyhow::Result<()> {
    let start_time = Instant::now();
    let entry_abs = fs::canonicalize(opts.entry).map_err(|e| {
        anyhow::anyhow!("Failed to resolve entry '{}': {}", opts.entry.display(), e)
    })?;
    let project_root = detect_project_root(&entry_abs);
    let lib_roots = collect_lib_roots(&project_root, opts.include_paths);
    let is_parallel_entry = check_parallel_cap_json(&project_root);

    fs::create_dir_all(opts.emit_dir)?;

    let cache_path = opts.emit_dir.join(".cp_inc_cache.json");
    let mut cache: IncCacheFile = fs::read_to_string(&cache_path)
        .ok()
        .and_then(|s| serde_json::from_str(&s).ok())
        .filter(|c: &IncCacheFile| c.version == 3)
        .unwrap_or_else(|| IncCacheFile {
            version: 3,
            files: HashMap::new(),
        });
    let mut cache_modified = false;

    let mut parsed_modules: HashMap<PathBuf, Module> = HashMap::new();
    let mut nodes: HashMap<PathBuf, ModuleNode> = HashMap::new();
    let mut order: Vec<PathBuf> = Vec::new();
    let mut zig_files_to_sync: HashMap<PathBuf, PathBuf> = HashMap::new();
    let mut queue: VecDeque<PathBuf> = VecDeque::new();
    let mut visited: HashSet<PathBuf> = HashSet::new();
    let mut uses_cap_hub = false;
    let mut cap_core_abs: Option<PathBuf> = None;
    let mut mounted_cap_ns: BTreeMap<String, PathBuf> = BTreeMap::new();

    visited.insert(entry_abs.clone());
    queue.push_back(entry_abs.clone());

    while let Some(abs_path) = queue.pop_front() {
        let raw_bytes = fs::read(&abs_path)
            .map_err(|e| anyhow::anyhow!("Failed to read '{}': {}", abs_path.display(), e))?;
        let content_hash = fnv1a64(&raw_bytes);
        let key_str = abs_path.to_string_lossy().to_string();

        let scan = if let Some(cached_scan) = cache.files.get(&key_str) {
            if cached_scan.content_hash == content_hash {
                cached_scan.clone()
            } else {
                let (s, module) = scan_cp_module(&abs_path, &raw_bytes)?;
                parsed_modules.insert(abs_path.clone(), module);
                cache.files.insert(key_str, s.clone());
                cache_modified = true;
                s
            }
        } else {
            let (s, module) = scan_cp_module(&abs_path, &raw_bytes)?;
            parsed_modules.insert(abs_path.clone(), module);
            cache.files.insert(key_str, s.clone());
            cache_modified = true;
            s
        };

        if scan.uses_cap_hub {
            uses_cap_hub = true;
            if cap_core_abs.is_none() {
                if let Some(core_p) = find_cap_core_file(opts.include_paths) {
                    cap_core_abs = Some(core_p.clone());
                    if visited.insert(core_p.clone()) {
                        queue.push_back(core_p);
                    }
                }
            }
            for ns in &scan.cap_hub_members {
                if let Some(rel_cp) = resolve_cap_hub_target(ns) {
                    if let Some(sub_abs) = find_lib_file(&rel_cp, opts.include_paths) {
                        mounted_cap_ns
                            .entry(ns.clone())
                            .or_insert_with(|| sub_abs.clone());
                        if visited.insert(sub_abs.clone()) {
                            queue.push_back(sub_abs);
                        }
                    }
                }
            }
        }

        let mut resolved_edges = Vec::with_capacity(scan.raw_imports.len());
        for src in &scan.raw_imports {
            let target = resolve_modular_import(&abs_path, src, opts.include_paths);
            match &target {
                ResolvedTarget::CpFile(dep_abs) => {
                    if visited.insert(dep_abs.clone()) {
                        queue.push_back(dep_abs.clone());
                    }
                }
                ResolvedTarget::ZigFile(zig_abs) => {
                    let zig_rel = map_module_rel_path(zig_abs, &project_root, &lib_roots);
                    zig_files_to_sync.insert(zig_abs.clone(), zig_rel);
                }
                _ => {}
            }
            resolved_edges.push((src.clone(), target));
        }

        let rel_zig_path = map_module_rel_path(&abs_path, &project_root, &lib_roots);
        let is_entry = abs_path == entry_abs;
        let inc_hash = compute_module_inc_hash(
            &scan,
            &resolved_edges,
            is_entry,
            is_entry && is_parallel_entry,
            opts,
        );
        let out_zig_path = opts.emit_dir.join(&rel_zig_path);
        let expected_header = format!("// @cp_inc_hash:{}", inc_hash);
        let is_dirty = opts.force
            || !out_zig_path.exists()
            || read_first_line(&out_zig_path).as_deref() != Some(expected_header.as_str());

        order.push(abs_path.clone());
        nodes.insert(
            abs_path,
            ModuleNode {
                abs_path: order.last().unwrap().clone(),
                rel_zig_path,
                scan,
                resolved_edges,
                inc_hash,
                is_dirty,
            },
        );
    }

    // Sync shared runtime __cp_runtime.zig (only writes if content differs or --force)
    let runtime_rel = PathBuf::from("__cp_runtime.zig");
    let runtime_out = opts.emit_dir.join(&runtime_rel);
    let runtime_code = Codegen::generate_shared_runtime();
    let _ = write_if_changed(&runtime_out, runtime_code.as_bytes(), opts.force)?;

    // Sync any directly imported .zig files into emit_dir
    for (zig_abs, zig_rel) in &zig_files_to_sync {
        if let Ok(bytes) = fs::read(zig_abs) {
            let dest = opts.emit_dir.join(zig_rel);
            let _ = write_if_changed(&dest, &bytes, opts.force)?;
        }
    }

    let total = order.len();
    let mut transpiled = 0usize;
    let mut cached = 0usize;

    for mod_path in &order {
        let node = nodes.get(mod_path).unwrap();
        if !node.is_dirty {
            cached += 1;
            continue;
        }
        transpiled += 1;

        let is_entry = node.abs_path == entry_abs;
        let mut import_map: HashMap<String, String> = HashMap::new();
        let mut star_exports: HashMap<String, Vec<String>> = HashMap::new();
        let mut imported_env = ImportedEnv::default();
        let mut env_visited = HashSet::new();

        // Parse module first (needed for AST)
        let mut module = get_or_parse_module(&node.abs_path, &mut parsed_modules)?.clone();
        let referenced_idents = crate::ast_visitor::collect_module_referenced_idents(&module);
        module.body.retain(|item| match item {
            ModuleItem::Import(imp) if is_external_package_source(&imp.source) => {
                is_import_used(imp, &referenced_idents)
            }
            _ => true,
        });

        // Build import_map and collect ImportedEnv for direct imports/re-exports
        let mut per_target_wanted: HashMap<PathBuf, Option<HashSet<String>>> = HashMap::new();
        for item in &module.body {
            match item {
                ModuleItem::Import(imp) => {
                    if let Some((_, ResolvedTarget::CpFile(target_abs))) =
                        node.resolved_edges.iter().find(|(s, _)| s == &imp.source)
                    {
                        if let Some(target_node) = nodes.get(target_abs) {
                            let rel_str = compute_zig_import_rel(
                                &node.rel_zig_path,
                                &target_node.rel_zig_path,
                            );
                            import_map.insert(imp.source.clone(), rel_str);
                        }
                        if imp.default_specifier.is_some() {
                            per_target_wanted.insert(target_abs.clone(), None);
                        } else {
                            let entry_wanted = per_target_wanted
                                .entry(target_abs.clone())
                                .or_insert_with(|| Some(HashSet::new()));
                            if let Some(set) = entry_wanted {
                                for sp in &imp.specifiers {
                                    set.insert(sp.sym.clone());
                                }
                            }
                        }
                    } else if let Some((_, ResolvedTarget::ZigFile(zig_abs))) =
                        node.resolved_edges.iter().find(|(s, _)| s == &imp.source)
                    {
                        if let Some(zig_rel) = zig_files_to_sync.get(zig_abs) {
                            let rel_str = compute_zig_import_rel(&node.rel_zig_path, zig_rel);
                            import_map.insert(imp.source.clone(), rel_str);
                        }
                    } else if imp.source == "std" {
                        import_map.insert(imp.source.clone(), "std".to_string());
                    } else if is_cap_hub_source(&imp.source) {
                        let cap_index_rel = PathBuf::from("_lib/_cap/index.zig");
                        let rel_str = compute_zig_import_rel(&node.rel_zig_path, &cap_index_rel);
                        import_map.insert(imp.source.clone(), rel_str);
                    }
                }
                ModuleItem::Export(exp) => {
                    if let Some(src) = &exp.source {
                        if let Some((_, ResolvedTarget::CpFile(target_abs))) =
                            node.resolved_edges.iter().find(|(s, _)| s == src)
                        {
                            if let Some(target_node) = nodes.get(target_abs) {
                                let rel_str = compute_zig_import_rel(
                                    &node.rel_zig_path,
                                    &target_node.rel_zig_path,
                                );
                                import_map.insert(src.clone(), rel_str);
                            }
                            if exp.specifiers.is_empty() {
                                let mut star_list = Vec::new();
                                let mut star_vis = HashSet::new();
                                collect_transitive_exports(
                                    target_abs,
                                    &nodes,
                                    &mut star_vis,
                                    &mut star_list,
                                );
                                star_exports.insert(src.clone(), star_list);
                                per_target_wanted.insert(target_abs.clone(), None);
                            } else {
                                let entry_wanted = per_target_wanted
                                    .entry(target_abs.clone())
                                    .or_insert_with(|| Some(HashSet::new()));
                                if let Some(set) = entry_wanted {
                                    for sp in &exp.specifiers {
                                        set.insert(sp.local.sym.clone());
                                    }
                                }
                            }
                        } else if let Some((_, ResolvedTarget::ZigFile(zig_abs))) =
                            node.resolved_edges.iter().find(|(s, _)| s == src)
                        {
                            if let Some(zig_rel) = zig_files_to_sync.get(zig_abs) {
                                let rel_str = compute_zig_import_rel(&node.rel_zig_path, zig_rel);
                                import_map.insert(src.clone(), rel_str);
                            }
                        }
                    }
                }
                _ => {}
            }
        }

        if node.scan.uses_cap_hub {
            let cap_index_rel = PathBuf::from("_lib/_cap/index.zig");
            let rel_str = compute_zig_import_rel(&node.rel_zig_path, &cap_index_rel);
            import_map
                .entry("cap".to_string())
                .or_insert_with(|| rel_str.clone());
            import_map.entry("cap:core".to_string()).or_insert(rel_str);
        }

        for (target_abs, wanted_opt) in &per_target_wanted {
            collect_exported_env_from_module(
                target_abs,
                wanted_opt.as_ref(),
                &nodes,
                &mut parsed_modules,
                &mut env_visited,
                &mut imported_env,
            )?;
        }

        // Semantic analysis on this single module with imported declarations pre-registered
        let mut analyzer = Analyzer::new();
        for decl in &imported_env.decls {
            analyzer.register_imported_decl(decl);
        }
        for var in &imported_env.vars {
            analyzer.register_imported_var(var);
        }
        let _ = analyzer.analyze_module(&mut module);
        let mut diags = analyzer.get_diagnostics();
        if !diags.is_empty() {
            let code = fs::read_to_string(&node.abs_path).ok();
            crate::diagnostic::mark_cascades(&mut diags, code.as_deref());
            let mut seen_codes = HashSet::new();
            let rendered = diags
                .iter()
                .filter(|d| d.cascade_of.is_none())
                .map(|d| {
                    let seen = !seen_codes.insert(d.code.clone());
                    d.render_dedup(code.as_deref(), Some(&node.abs_path), seen)
                })
                .collect::<Vec<_>>()
                .join("\n");
            return Err(anyhow::anyhow!("{}", rendered));
        }

        let is_fuzz = opts.fuzz || std::env::var("CP_FUZZ").is_ok();
        let fuzz_tgt = opts
            .fuzz_target
            .clone()
            .or_else(|| std::env::var("CP_FUZZ_TARGET").ok());
        let rt_rel_import = compute_zig_import_rel(&node.rel_zig_path, &runtime_rel);
        let code_str = fs::read_to_string(&node.abs_path).ok();
        let node_abs_clone = node.abs_path.clone();
        let node_hash_clone = node.inc_hash.clone();
        let beam = opts.beam;
        let lib = opts.lib;
        let parallel_entry = is_entry && is_parallel_entry;
        let mod_used_packages: BTreeSet<String> = if is_entry {
            order
                .iter()
                .flat_map(|p| nodes[p].scan.used_packages.iter().cloned())
                .collect()
        } else {
            node.scan.used_packages.iter().cloned().collect()
        };

        let zig_res = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            let mut codegen = Codegen::new();
            codegen.is_beam_mode = beam;
            codegen.is_lib_mode = lib;
            codegen.is_fuzz_mode = is_fuzz;
            codegen.fuzz_target_filter = fuzz_tgt;
            codegen.modular_mode = true;
            codegen.modular_is_entry = is_entry;
            codegen.modular_runtime_import = Some(rt_rel_import);
            codegen.modular_inc_hash = Some(node_hash_clone);
            codegen.modular_import_map = import_map;
            codegen.modular_module_exports = star_exports;
            codegen.used_packages = mod_used_packages;
            if parallel_entry {
                codegen.is_parallel_runtime = true;
            }
            populate_codegen_from_env(&mut codegen, &imported_env);
            codegen.source_dir = node_abs_clone.parent().map(|p| p.to_path_buf());
            if let Some(ref c) = code_str {
                codegen.init_source(Some(&node_abs_clone.display().to_string()), Some(c));
            }
            let code = codegen.generate_module(&module);
            (code, codegen.source_map_entries)
        }));

        let (zig_code, map_entries) = match zig_res {
            Ok(res) => res,
            Err(err) => {
                let msg = if let Some(s) = err.downcast_ref::<&str>() {
                    s.to_string()
                } else if let Some(s) = err.downcast_ref::<String>() {
                    s.clone()
                } else {
                    "Internal compiler error".to_string()
                };
                let code = fs::read_to_string(&node.abs_path).ok();
                let diag = crate::diagnostic::from_compiler_error_msg(&msg, code.as_deref());
                return Err(anyhow::anyhow!(
                    "{}",
                    diag.render(code.as_deref(), Some(&node.abs_path))
                ));
            }
        };

        let out_zig_path = opts.emit_dir.join(&node.rel_zig_path);
        if let Some(parent) = out_zig_path.parent() {
            fs::create_dir_all(parent)?;
        }
        fs::write(&out_zig_path, &zig_code)?;

        let mut sm = crate::source_map::SourceMap::new(
            node.abs_path.display().to_string(),
            out_zig_path.display().to_string(),
        );
        sm.mappings = map_entries;
        let map_path = out_zig_path.with_extension("zig.map");
        let _ = fs::write(map_path, sm.to_json());
    }

    if uses_cap_hub {
        let cap_index_rel = PathBuf::from("_lib/_cap/index.zig");
        let cap_index_out = opts.emit_dir.join(&cap_index_rel);
        let mut hub_zig = String::from(
            "// Generated by CP Compiler — Lazy Cap Hub Index (compile-time mounted)\n",
        );
        let mut emitted_names: HashSet<String> = HashSet::new();

        if let Some(ref core_abs) = cap_core_abs {
            if let Some(core_node) = nodes.get(core_abs) {
                let core_rel_str = compute_zig_import_rel(&cap_index_rel, &core_node.rel_zig_path);
                hub_zig.push_str(&format!(
                    "const __core = @import(\"{}\");\n\n",
                    core_rel_str
                ));

                let core_zig_path = opts.emit_dir.join(&core_node.rel_zig_path);
                if let Ok(core_zig_src) = fs::read_to_string(&core_zig_path) {
                    for sym in extract_top_level_zig_pub_names(&core_zig_src) {
                        if mounted_cap_ns.contains_key(&sym) {
                            continue;
                        }
                        if emitted_names.insert(sym.clone()) {
                            let esc = Codegen::escape_zig_ident(&sym);
                            hub_zig.push_str(&format!("pub const {0} = __core.{0};\n", esc));
                        }
                    }
                }
            }
        }

        if !mounted_cap_ns.is_empty() {
            hub_zig.push_str("\n// Lazily mounted sub-namespaces:\n");
            for (ns, sub_abs) in &mounted_cap_ns {
                if let Some(sub_node) = nodes.get(sub_abs) {
                    if emitted_names.insert(ns.clone()) {
                        let sub_rel_str =
                            compute_zig_import_rel(&cap_index_rel, &sub_node.rel_zig_path);
                        let esc = Codegen::escape_zig_ident(ns);
                        hub_zig.push_str(&format!(
                            "pub const {} = @import(\"{}\");\n",
                            esc, sub_rel_str
                        ));
                    }
                }
            }
        }

        let _ = write_if_changed(&cap_index_out, hub_zig.as_bytes(), opts.force)?;
    }

    if cache_modified {
        if let Ok(json) = serde_json::to_string(&cache) {
            let _ = fs::write(&cache_path, json);
        }
    }

    // Write .cp_modular_manifest.txt and optional __cp_entry.zig wrapper at emit_dir root
    let entry_node = nodes.get(&entry_abs).unwrap();
    let entry_module_zig_path = opts.emit_dir.join(&entry_node.rel_zig_path);
    let entry_has_parent = entry_node
        .rel_zig_path
        .parent()
        .map_or(false, |p| !p.as_os_str().is_empty());

    let entry_zig_path = if entry_has_parent {
        let rel_from_root = entry_node.rel_zig_path.to_string_lossy().replace('\\', "/");
        let wrapper_path = opts.emit_dir.join("__cp_entry.zig");
        let wrapper_code = if opts.fuzz || opts.beam {
            format!(
                "comptime {{\n    _ = @import(\"./{}\");\n}}\n",
                rel_from_root
            )
        } else {
            format!(
                "const __entry = @import(\"./{}\");\npub const main = if (@hasDecl(__entry, \"main\")) __entry.main else struct {{\n    pub fn main() void {{}}\n}}.main;\n",
                rel_from_root
            )
        };
        let _ = write_if_changed(&wrapper_path, wrapper_code.as_bytes(), opts.force)?;
        wrapper_path
    } else {
        entry_module_zig_path.clone()
    };

    let mut link_paths: BTreeSet<String> = Codegen::new().linker_paths.into_iter().collect();
    let mut link_libs: BTreeSet<String> = BTreeSet::new();
    let mut c_includes: BTreeSet<String> = BTreeSet::new();
    let mut all_used_pkgs: BTreeSet<String> = BTreeSet::new();
    for p in &order {
        let node = &nodes[p];
        for lp in &node.scan.link_paths {
            link_paths.insert(lp.clone());
        }
        for ll in &node.scan.link_libs {
            link_libs.insert(ll.clone());
        }
        for ci in &node.scan.c_includes {
            c_includes.insert(ci.clone());
        }
        for pkg in &node.scan.used_packages {
            all_used_pkgs.insert(pkg.clone());
        }
    }

    let mut manifest = String::new();
    manifest.push_str(&format!("entry={}\n", entry_zig_path.display()));
    manifest.push_str(&format!(
        "entry_module={}\n",
        entry_module_zig_path.display()
    ));
    manifest.push_str(&format!("runtime={}\n", runtime_out.display()));
    for lp in &link_paths {
        manifest.push_str(&format!("linkPath={}\n", lp));
    }
    for ll in &link_libs {
        manifest.push_str(&format!("linkLib={}\n", ll));
    }
    for ci in &c_includes {
        manifest.push_str(&format!("cInclude={}\n", ci));
    }
    for pkg in &all_used_pkgs {
        manifest.push_str(&format!("usedPkg={}\n", pkg));
    }
    for p in &order {
        let node = &nodes[p];
        manifest.push_str(&format!(
            "module={} -> {}\n",
            node.abs_path.display(),
            node.rel_zig_path.display()
        ));
    }
    let manifest_path = opts.emit_dir.join(".cp_modular_manifest.txt");
    let _ = write_if_changed(&manifest_path, manifest.as_bytes(), opts.force)?;

    let elapsed_ms = start_time.elapsed().as_millis();
    println!(
        "[cpc-modular] total={} transpiled={} cached={} entry={} ({}ms)",
        total,
        transpiled,
        cached,
        entry_zig_path.display(),
        elapsed_ms
    );

    Ok(())
}
